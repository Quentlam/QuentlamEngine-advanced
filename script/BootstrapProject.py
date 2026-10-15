#!/usr/bin/env python3
"""BootstrapProject.py - cross-platform dependency bootstrap for QuentlamEngine.

Single implementation shared by both platform entry points:

    Windows : script\\BootstrapProject.bat  (wraps this file)
    Linux   : script/bootstrap.sh          (wraps this file)

Steps performed:
  1. Normalize proxy environment variables (MSBuild MSB6001 workaround).
  2. Download premake5 into vendor/bin when it is missing.
  3. Configure and build assimp (Debug/Release) with CMake.
  4. Verify the assimp artifacts that premake5.lua links against.
  5. Generate the workspace/solution files with premake5.

Toolchain alignment note
------------------------
premake5.lua hardcodes MSVC library names (assimp-vc143-*.lib) for a VS2022
workspace. assimp therefore has to be configured with the "Visual Studio 17
2022" generator (v143 toolset). Letting CMake pick the newest installed
generator (e.g. "Visual Studio 18 2026" => vc145) produces library names the
premake solution cannot resolve: LNK1181 assimp-vc143-mtd.lib.
"""

from __future__ import annotations

import argparse
import glob
import os
import shutil
import subprocess
import sys
import tarfile
import urllib.request
import zipfile
from pathlib import Path

PREMAKE_VERSION = "5.0.0-beta2"
PREMAKE_URL_BASE = (
    "https://github.com/premake/premake-core/releases/download/"
    f"v{PREMAKE_VERSION}"
)

# Proxy variables that must exist exactly once in the child environment. MSBuild
# merges its child environment into a case-insensitive dictionary and aborts
# with MSB6001 ("CL.exe" invalid command line switch /
# System.ArgumentException: an item with the same key has already been added)
# when both HTTPS_PROXY and https_proxy are present.
PROXY_VARS = frozenset(
    {"HTTP_PROXY", "HTTPS_PROXY", "ALL_PROXY", "NO_PROXY", "FTP_PROXY"}
)

IS_WINDOWS = os.name == "nt"

# Windows: multi-config generator => one build directory, per-config output
# subfolders. Linux: single-config generator => one build directory per config.
VS_GENERATOR = "Visual Studio 17 2022"
VS_TOOLSET_TAG = "vc143"
NIX_GENERATOR = "Unix Makefiles"


def log_step(message: str) -> None:
    print(f"==> {message}", flush=True)


def run(command: list[str], cwd: Path | None = None, env: dict | None = None) -> None:
    """Run a command, echoing it first, and abort on a non-zero exit code."""
    printable = subprocess.list2cmdline(command) if IS_WINDOWS else " ".join(command)
    print(printable, flush=True)
    result = subprocess.run(command, cwd=cwd, env=env)
    if result.returncode != 0:
        raise SystemExit(
            f"Command failed with exit code {result.returncode}: {printable}"
        )


def build_clean_env() -> dict:
    """Return the environment with proxy variables deduplicated by upper casing.

    Anything else is passed through untouched. Collapsing the case variants onto
    a single canonical key gives MSBuild an environment it can copy without
    tripping over duplicate, differing-only-by-case entries.
    """
    env: dict[str, str] = {}
    for key, value in os.environ.items():
        env[key.upper() if key.upper() in PROXY_VARS else key] = value
    return env


# --------------------------------------------------------------------------- #
# Tool discovery
# --------------------------------------------------------------------------- #

def find_vswhere() -> Path | None:
    program_files_x86 = os.environ.get("ProgramFiles(x86)")
    if not program_files_x86:
        return None
    candidate = (
        Path(program_files_x86)
        / "Microsoft Visual Studio"
        / "Installer"
        / "vswhere.exe"
    )
    return candidate if candidate.is_file() else None


def vswhere_installation_path(vswhere: Path, version_range: str | None) -> str | None:
    args = [
        str(vswhere),
        "-latest",
        "-products",
        "*",
        "-property",
        "installationPath",
    ]
    if version_range:
        args += ["-version", version_range]
    try:
        completed = subprocess.run(args, capture_output=True, text=True, check=False)
    except OSError:
        return None
    lines = [line.strip() for line in completed.stdout.splitlines() if line.strip()]
    return lines[0] if lines else None


def find_cmake() -> str:
    """Locate cmake, preferring the VS2022 bundled copy on Windows."""
    if IS_WINDOWS:
        vswhere = find_vswhere()
        if vswhere:
            # Prefer the VS2022 (v17) instance: its bundled CMake generates a
            # plain .sln and keeps generator/toolset aligned with the premake5
            # "vs2022" workspace. Fall back to the newest instance if absent.
            for version_range in ("[17.0,18.0)", None):
                installation = vswhere_installation_path(vswhere, version_range)
                if not installation:
                    continue
                cmake = (
                    Path(installation)
                    / "Common7"
                    / "IDE"
                    / "CommonExtensions"
                    / "Microsoft"
                    / "CMake"
                    / "CMake"
                    / "bin"
                    / "cmake.exe"
                )
                if cmake.is_file():
                    return str(cmake)

    cmake = shutil.which("cmake")
    if cmake:
        return cmake

    raise SystemExit(
        "cmake was not found. Install CMake (or the Visual Studio CMake tools) "
        "and make sure it is on PATH."
    )


def ensure_premake(vendor_bin: Path) -> Path:
    """Return the premake5 executable, downloading it on first use."""
    exe_name = "premake5.exe" if IS_WINDOWS else "premake5"
    premake = vendor_bin / exe_name
    if premake.is_file():
        return premake

    log_step("Downloading premake5")
    vendor_bin.mkdir(parents=True, exist_ok=True)

    if IS_WINDOWS:
        archive_url = f"{PREMAKE_URL_BASE}/premake-{PREMAKE_VERSION}-windows.zip"
        archive = vendor_bin / "premake5-windows.zip"
    else:
        archive_url = f"{PREMAKE_URL_BASE}/premake-{PREMAKE_VERSION}-linux.tar.gz"
        archive = vendor_bin / "premake5-linux.tar.gz"

    urllib.request.urlretrieve(archive_url, archive)

    if archive.suffix == ".zip":
        with zipfile.ZipFile(archive) as handle:
            handle.extractall(vendor_bin)
    else:
        with tarfile.open(archive) as handle:
            handle.extractall(vendor_bin)

    if not premake.is_file():
        # Some archives nest the binary in a subdirectory; relocate it.
        for found in vendor_bin.rglob(exe_name):
            shutil.move(str(found), str(premake))
            break

    if not premake.is_file():
        raise SystemExit(
            f"premake5 was downloaded but '{exe_name}' was not found in {vendor_bin}"
        )

    if not IS_WINDOWS:
        premake.chmod(0o755)

    return premake


# --------------------------------------------------------------------------- #
# assimp build
# --------------------------------------------------------------------------- #

def read_cached_generator(cache_file: Path) -> str | None:
    try:
        content = cache_file.read_text(encoding="utf-8", errors="ignore")
    except OSError:
        return None
    for line in content.splitlines():
        if line.startswith("CMAKE_GENERATOR:INTERNAL="):
            return line.split("=", 1)[1].strip()
    return None


def purge_on_generator_mismatch(build_dir: Path, generator: str) -> None:
    """Drop a CMake cache that belongs to a different generator.

    A cache is bound to the generator that created it and reconfiguring with
    another one fails outright. Everything under the build directory is
    generated, so a clean rebuild is safe whenever the generator changed.
    """
    build_dir.mkdir(parents=True, exist_ok=True)
    cache_file = build_dir / "CMakeCache.txt"
    if not cache_file.is_file():
        return

    cached = read_cached_generator(cache_file)
    if cached and cached != generator:
        log_step(
            f"Generator mismatch ('{cached}' -> '{generator}'), "
            f"purging {build_dir}"
        )
        shutil.rmtree(build_dir)
        build_dir.mkdir(parents=True, exist_ok=True)


def build_dir_for(assimp_root: Path, config: str) -> Path:
    if IS_WINDOWS:
        # Multi-config generator: every configuration shares one build tree.
        return assimp_root / "build"
    # Single-config generator: each configuration needs its own build tree.
    return assimp_root / "build" / config.lower()


def configure_assimp(
    cmake: str, source_dir: Path, build_dir: Path, generator: str, config: str, env: dict
) -> None:
    purge_on_generator_mismatch(build_dir, generator)

    command = [
        cmake,
        # Absolute path: on Linux the build directory is nested (build/debug),
        # so a relative ".." would not point back at the source tree.
        str(source_dir),
        "-G",
        generator,
        "-DBUILD_SHARED_LIBS=OFF",
        "-DASSIMP_BUILD_TESTS=OFF",
        "-DASSIMP_INSTALL=OFF",
    ]
    if IS_WINDOWS:
        command += [
            "-A",
            "x64",
            # Match premake5.lua's `staticruntime "on"`.
            "-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded$<$<CONFIG:Debug>:Debug>",
        ]
    else:
        # CMAKE_BUILD_TYPE only applies to single-config generators.
        command += [f"-DCMAKE_BUILD_TYPE={config}"]

    run(command, cwd=build_dir, env=env)


def build_assimp(cmake: str, build_dir: Path, config: str, env: dict) -> None:
    command = [cmake, "--build", str(build_dir)]
    if IS_WINDOWS:
        command += ["--config", config]
    else:
        command += ["-j", str(os.cpu_count() or 4)]
    run(command, env=env)


def expected_artifacts(assimp_root: Path, configs: list[str]) -> list[str]:
    """Glob patterns for the libraries premake5.lua expects to link.

    Windows uses exact paths mirroring premake5.lua's hardcoded names. Linux
    paths follow assimp's default install layout and are matched by wildcard
    because they have not been pinned down against a real build yet.
    """
    if IS_WINDOWS:
        patterns = []
        for config in configs:
            suffix = "mtd" if config == "Debug" else "mt"
            patterns.append(f"build/lib/{config}/assimp-{VS_TOOLSET_TAG}-{suffix}.lib")
        patterns.append("build/contrib/zlib/Debug/zlibstaticd.lib")
        patterns.append("build/contrib/zlib/Release/zlibstatic.lib")
        return patterns

    return [f"build/{config.lower()}/lib/libassimp*.a" for config in configs]


def verify_artifacts(assimp_root: Path, patterns: list[str]) -> None:
    missing: list[str] = []
    for pattern in patterns:
        matches = sorted(glob.glob(str(assimp_root / pattern)))
        if matches:
            print(f"  ok {matches[0]}", flush=True)
        else:
            print(f"  MISSING {pattern}", flush=True)
            missing.append(pattern)

    if missing:
        raise SystemExit(
            "Missing assimp artifacts required by premake5.lua:\n  "
            + "\n  ".join(missing)
        )


# --------------------------------------------------------------------------- #

def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    default_root = Path(__file__).resolve().parent.parent
    parser = argparse.ArgumentParser(
        description="Prepare third-party dependencies and generate the workspace.",
    )
    parser.add_argument(
        "--repo-root",
        default=str(default_root),
        help="Repository root (defaults to the parent of this script's directory).",
    )
    parser.add_argument(
        "--configs",
        default="Debug,Release",
        help="Comma-separated assimp configurations to build (default: Debug,Release).",
    )
    parser.add_argument(
        "--generator",
        default=None,
        help=f"Override the CMake generator (default: '{VS_GENERATOR}' on Windows, "
        f"'{NIX_GENERATOR}' elsewhere).",
    )
    parser.add_argument(
        "--premake-action",
        default=None,
        help="Override the premake5 action (default: 'vs2022' on Windows, 'gmake2' elsewhere).",
    )
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)

    repo_root = Path(args.repo_root).resolve()
    if not (repo_root / "premake5.lua").is_file():
        raise SystemExit(f"premake5.lua not found under {repo_root}")

    configs = [item.strip() for item in args.configs.split(",") if item.strip()]
    if not configs:
        raise SystemExit("--configs must list at least one configuration")

    generator = args.generator or (VS_GENERATOR if IS_WINDOWS else NIX_GENERATOR)
    premake_action = args.premake_action or ("vs2022" if IS_WINDOWS else "gmake2")

    env = build_clean_env()
    # Propagate the sanitized environment to this process too, so any nested
    # tool invocation started without an explicit env still sees it.
    os.environ.clear()
    os.environ.update(env)

    vendor_bin = repo_root / "vendor" / "bin"
    assimp_root = repo_root / "QuentlamEngine" / "vendor" / "assimp"

    if not assimp_root.is_dir():
        raise SystemExit(f"assimp source tree not found: {assimp_root}")

    vendor_bin.mkdir(parents=True, exist_ok=True)

    premake = ensure_premake(vendor_bin)
    cmake = find_cmake()

    log_step(f"Configuring assimp with CMake ({generator})")
    # On Windows all configurations share one build tree, so configure it once;
    # on Linux each configuration owns a separate tree and needs its own run.
    configured: set[Path] = set()
    for config in configs:
        build_dir = build_dir_for(assimp_root, config)
        if build_dir in configured:
            continue
        configured.add(build_dir)
        configure_assimp(cmake, assimp_root, build_dir, generator, config, env)

    log_step("Building assimp")
    for config in configs:
        build_assimp(cmake, build_dir_for(assimp_root, config), config, env)

    log_step("Verifying assimp artifacts required by premake5.lua")
    verify_artifacts(assimp_root, expected_artifacts(assimp_root, configs))

    log_step(f"Generating workspace files with premake5 ({premake_action})")
    run([str(premake), premake_action], cwd=repo_root, env=env)

    log_step("Bootstrap complete")
    return 0


if __name__ == "__main__":
    sys.exit(main())
