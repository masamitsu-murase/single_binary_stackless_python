@echo off
setlocal enabledelayedexpansion

cd /d "%~dp0.."

if "%2" == "2015" goto SET_VC_VERSION
if "%2" == "2017" goto SET_VC_VERSION
echo Unknown VC version
exit /b 1

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
    set VC_CPU=amd64
    set WINDOWS_PE=0
) else if "%1" == "x64_pe" (
    set BUILD_TARGET_CPU=x64
    set VC_CPU=amd64
    set WINDOWS_PE=1
) else (
    echo Unknown target CPU
    exit /b 1
)
REM Windows SDK Version
if "%3" == "" (
    REM "10.0.14393.0" can also be accepted.
    set WINDOWS_SDK_VERSION=10.0.15063.0
) else (
    set WINDOWS_SDK_VERSION=%3
)

echo VC_CPU %VC_CPU%
echo VC_VERSION %VC_VERSION%
echo WINDOWS_SDK_VERSION %WINDOWS_SDK_VERSION%
echo WINDOWS_PE %WINDOWS_PE%


REM --------------------------------
REM VC environment
if "%VC_VERSION%" == "2015" (
    set "VSTOOLS=!VS140COMNTOOLS!..\..\VC\vcvarsall.bat"
) else if "%VC_VERSION%" == "2017" (
    if EXIST "C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\VC" (
        set "VSTOOLS=C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\VC\Auxiliary\Build\vcvarsall.bat"
    ) else if EXIST "C:\Program Files (x86)\Microsoft Visual Studio\2017\Professional\VC" (
        set "VSTOOLS=C:\Program Files (x86)\Microsoft Visual Studio\2017\Professional\VC\Auxiliary\Build\vcvarsall.bat"
    ) else (
        set "VSTOOLS=C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat"
    )
) else (
    echo Unknown VC_VERSION
    exit /b 1
)
pushd .
call "%VSTOOLS%" %VC_CPU% %WINDOWS_SDK_VERSION%
if ERRORLEVEL 1 (
    popd
    exit /b 1
)
popd


REM --------------------------------
REM for openssl
if EXIST C:\my_program\perl\perl\bin\perl.exe (
    set PERL_EXE=C:\my_program\perl\perl\bin\perl.exe
) else (
    set PERL_EXE=perl.exe
)
%PERL_EXE% -v

REM --------------------------------
REM for embedded_import and tclEmbeddedFilesystem
set PYTHON_EXE=%~dp0my_tools\python.exe

REM --------------------------------
REM embeddedimport
echo Creating embeddedimport_data.c...
%PYTHON_EXE% my_tools/create_embeddedimporter_data.py
if ERRORLEVEL 1 (
    echo Failed to create embeddedimport_data.c
    exit /b 1
)

REM --------------------------------
REM build
cd /d "%~dp0"

nmake
if ERRORLEVEL 1 exit /b 1

cd /d "%~dp0.."
if not "%GITHUB_ACTIONS%" == "" (
    PCbuild\my_tools\7za.exe a Lib.7z -mx=9 -bd -ir^^!Lib\*.py -xr^^!test -xr^^!__pycache__ > NUL
)

exit /b 0
