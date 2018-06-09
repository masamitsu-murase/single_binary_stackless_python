@echo off
setlocal enabledelayedexpansion

cd /d "%~dp0.."

set VC_PYTHON=0
if "%2" == "2008" goto SET_VC_VERSION
if "%2" == "python2008" (
    set VC_VERSION=2008
    set VC_PYTHON=1
    goto SET_BUILD_TOOL
)
if "%2" == "2012" goto SET_VC_VERSION
if "%2" == "2013" goto SET_VC_VERSION
if "%2" == "2015" goto SET_VC_VERSION
set VC_VERSION=2015
goto SET_BUILD_TOOL

:SET_VC_VERSION
set VC_VERSION=%2


REM --------------------------------
:SET_BUILD_TOOL
if "%1" == "x86" (
    set BUILD_TARGET_CPU=x86
    set VC_CPU=x86
    set WINDOWS_PE=0
) else if "%1" == "x64" (
    set BUILD_TARGET_CPU=x64
    if "%VC_VERSION%" == "2012" (
        set VC_CPU=x86_amd64
    ) else (
        set VC_CPU=amd64
    )
    set WINDOWS_PE=0
) else if "%1" == "x64_pe" (
    set BUILD_TARGET_CPU=x64
    if "%VC_VERSION%" == "2012" (
        set VC_CPU=x86_amd64
    ) else (
        set VC_CPU=amd64
    )
    set WINDOWS_PE=1
) else (
    set BUILD_TARGET_CPU=x86
    set VC_CPU=x86
    set WINDOWS_PE=0
)

echo VC_CPU %VC_CPU%
echo VC_VERSION %VC_VERSION%
echo VC_PYTHON %VC_PYTHON%
echo WINDOWS_PE %WINDOWS_PE%


REM --------------------------------
REM VC environment
if NOT "%VC_PYTHON%" == "0" goto USE_PYTHON_VC
if "%VC_VERSION%" == "2008" (
    set VSTOOLS=!VS90COMNTOOLS!
) else if "%VC_VERSION%" == "2012" (
    set VSTOOLS=!VS110COMNTOOLS!
) else if "%VC_VERSION%" == "2013" (
    set VSTOOLS=!VS120COMNTOOLS!
) else (
    set VSTOOLS=!VS140COMNTOOLS!
)
call "%VSTOOLS%..\..\VC\vcvarsall.bat" %VC_CPU%
if ERRORLEVEL 1 exit /b 1
goto EXTRACT_EXTERNALS

:USE_PYTHON_VC
call "%LOCALAPPDATA%\Programs\Common\Microsoft\Visual C++ for Python\9.0\vcvarsall.bat" %VC_CPU%
if ERRORLEVEL 1 exit /b 1


:EXTRACT_EXTERNALS
REM --------------------------------
REM externals
set IncludeTkinter=false
if EXIST externals (
    echo externals already exists.
) else if EXIST externals.exe (
    echo Extracting externals...
    externals.exe -o. -y > NUL
    if ERRORLEVEL 1 exit /b 1
) else (
    call PCbuild\get_externals.bat --no-tkinter
    if ERRORLEVEL 1 exit /b 1
)

REM --------------------------------
REM embeddedimport
if EXIST C:\Ruby23\bin\ruby.exe (
    set RUBY_EXE=C:\Ruby23\bin\ruby.exe
) else (
    set RUBY_EXE=ruby.exe
)
echo Creating embeddedimport_data.c...
%RUBY_EXE% create_embeddedimporter_data.rb
if ERRORLEVEL 1 (
    echo Failed to create embeddedimport_data.c
    exit /b 1
)


REM --------------------------------
REM build
cd /d "%~dp0"

nmake
if ERRORLEVEL 1 exit /b 1

exit /b 0


