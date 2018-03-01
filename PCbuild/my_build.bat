@echo off
setlocal enabledelayedexpansion

if "%1" == "x86" (
    set BUILD_TARGET_CPU=x86
    set VC_CPU=x86
    set WINDOWS_PE=0
) else if "%1" == "x64" (
    set BUILD_TARGET_CPU=x64
    set VC_CPU=amd64
    set WINDOWS_PE=0
) else if "%1" == "x64_pe" (
    set BUILD_TARGET_CPU=x64
    set VC_CPU=amd64
    set WINDOWS_PE=1
) else (
    set BUILD_TARGET_CPU=x86
    set VC_CPU=x86
    set WINDOWS_PE=0
)

if "%2" == "2012" goto SET_VC_VERSION
if "%2" == "2013" goto SET_VC_VERSION
if "%2" == "2015" goto SET_VC_VERSION
set VC_VERSION=2013
goto SET_BUILD_TOOL

:SET_VC_VERSION
set VC_VERSION=%2


:SET_BUILD_TOOL
echo VC_CPU %VC_CPU%
echo VC_VERSION %VC_VERSION%
echo WINDOWS_PE %WINDOWS_PE%

if "%VC_VERSION%" == "2012" (
    set VSTOOLS=!VS110COMNTOOLS!
) else if "%VC_VERSION%" == "2013" (
    set VSTOOLS=!VS120COMNTOOLS!
) else (
    set VSTOOLS=!VS140COMNTOOLS!
)

call "%VSTOOLS%..\..\VC\vcvarsall.bat" %VC_CPU%


REM externals
set IncludeTkinter=false
if EXIST externals.exe (
    echo Extracting externals...
    externals.exe -o. -y > NUL
    if ERRORLEVEL 1 exit /b 1
) else (
    call get_externals.bat
    if ERRORLEVEL 1 exit /b 1
)

REM build
cd /d "%~dp0"
prebuilt_lib.exe -o. -y
if ERRORLEVEL 1 exit /b 1

nmake
