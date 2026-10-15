@echo off
setlocal
pushd "%~dp0.."
rem %CD% is the repository root after pushd.

set "PYTHON_CMD="
where py >nul 2>nul && set "PYTHON_CMD=py -3"
if not defined PYTHON_CMD where python >nul 2>nul && set "PYTHON_CMD=python"
if not defined PYTHON_CMD (
    echo ERROR: Python 3 was not found on PATH. Install it from https://www.python.org/downloads/
    popd
    exit /b 1
)

%PYTHON_CMD% ".\script\BootstrapProject.py" --repo-root "%CD%" %*
set "EXIT_CODE=%ERRORLEVEL%"

popd
exit /b %EXIT_CODE%
