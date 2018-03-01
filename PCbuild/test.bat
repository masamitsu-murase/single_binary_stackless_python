@echo off

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

if "%BUILD_TARGET_CPU%" == "x86" (
    set PYTHON=python.exe
) else if "%WINDOWS_PE%" == "0" (
    set PYTHON=python64.exe
) else (
    set PYTHON=python64_pe.exe
)


%PYTHON% --version
