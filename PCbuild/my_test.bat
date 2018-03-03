@echo off

if "%1" == "x86" (
    set BUILD_TARGET_CPU=x86
    set WINDOWS_PE=0
) else if "%1" == "x64" (
    set BUILD_TARGET_CPU=x64
    set WINDOWS_PE=0
) else if "%1" == "x64_pe" (
    set BUILD_TARGET_CPU=x64
    set WINDOWS_PE=1
) else (
    set BUILD_TARGET_CPU=x86
    set WINDOWS_PE=0
)

if "%BUILD_TARGET_CPU%" == "x86" (
    set PYTHON=python.exe
) else if "%WINDOWS_PE%" == "0" (
    set PYTHON=python64.exe
) else (
    set PYTHON=python64_pe.exe
)

echo BUILD_TARGET_CPU %BUILD_TARGET_CPU%
echo WINDOWS_PE %WINDOWS_PE%
echo PYTHON %PYTHON%

cd /d "%~dp0"
%PYTHON% -c "import sys; print(sys.version)"

%PYTHON% my_test\test_original_changes.py
if ERRORLEVEL 1 (
    echo "Test failed."
    exit /b 1
)

%PYTHON% -m test.regrtest -x test_argparse test_asynchat test_asyncore test_calendar test_class test_cmath test_class test_cmath test_codecs test_ctypes test_distutils test_email test_email_renamed test_ftplib test_hash test_json test_lib2to3 test_macpath test_ssl test_unittest
exit /b 0
