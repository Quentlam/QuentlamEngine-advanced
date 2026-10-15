@echo off
setlocal
pushd "%~dp0.."
rem Kept as the historical entry point; delegates to the bootstrap script.
call ".\script\BootstrapProject.bat"
set "EXIT_CODE=%ERRORLEVEL%"
popd
exit /b %EXIT_CODE%
