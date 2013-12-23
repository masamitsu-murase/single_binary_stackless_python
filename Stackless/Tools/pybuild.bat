@ECHO off

REM Building of Stackless Pyton distribution
REM 
REM Original script file by Richard Tew
REM Modified by Kristjan Valur
REM This should generate the following:
REM - The MSI installer.
REM - The "published binaries".
REM - The HTML documentation zip archive.
REM If RELEASE is NO, then it will choke on all but the building.
REM add argument NOCLEAN to skip the cleaning stage.

REM TODO:
REM Profile guided optimization
REM X64 build

REM Requirements:
REM - Visual Studio 2008
REM - Python 2.7 (location set below)
REM - 7-Zip (location set below)
REM - PythonForWindows (http://sourceforge.net/projects/pywin32)
REM - Microsoft HTML HELP workshop (should come as part of visual studio)
REM - SVN, e.g. tortoisesvn with command line tools, to get externals from svn.python.org

REM Settings:
SET RELEASE=YES
SET PY_VERSION=2.7.6
SET PY_VERSIONX=276
SET PYTHON=c:\python27\python.exe
SET SEVENZIP=%ProgramFiles%\7-Zip\7z.exe
SET HTMLHELP=%ProgramFiles%\HTML Help Workshop\hhc.exe
IF NOT EXIST "%HTMLHELP%" SET HTMLHELP=%ProgramFiles(x86)%\HTML Help Workshop\hhc.exe
set WHATSMISSING=Python
IF NOT EXIST "%PYTHON%" goto MISSING
set WHATSMISSING=7Zip
IF NOT EXIST "%SEVENZIP%" goto MISSING
set WHATSMISSING=HTML Help Workshop
IF NOT EXIST "%HTMLHELP%" goto MISSING

REM Optionally use perl for ssl makefile, otherwise, use default
REM SET PERL=c:\Perl\bin\perl.exe

rem set these to choose SC program
set P4=
set HG=1

REM Set CD to batch file location and enable delayed expansion of vars
setlocal ENABLEDELAYEDEXPANSION
set OLDDIR=%CD%
cd /d %~dp0
set BATDIR=%CD%
REM this file lives in Stackless/Tools.  Find the main directory
cd ..\..
set PYDIR=%CD%

REM Parse options
:LOOP
IF NOT "%1"=="" (
    IF /I "%1"=="-NOCLEAN" (
        SET NOCLEAN=1
        SHIFT
        GOTO :LOOP
    )
    IF /I "%1"=="-SNAPSHOT" (
        SET SNAPSHOT=1
        SHIFT
        GOTO :LOOP
    )
    echo invalid arguments
    exit /b 1
)


SET SCRIPTDIR=Tools\buildbot
SET MSIDIR=Tools\msi
SET SCRIPT_BUILDX86=%SCRIPTDIR%\build.bat
SET SCRIPT_EXTERNALX86=%SCRIPTDIR%\external.bat
SET SCRIPT_BUILDAMD64=%SCRIPTDIR%\build-amd64.bat
SET SCRIPT_EXTERNALAMD64=%SCRIPTDIR%\external-amd64.bat

rem non-default place for externals, relative to main python dir.
rem note, if they are non a non-default place, the external-common.bat file needs changing.
rem SET EXTERNSDIR=../externals/
rem set EXTERNSPATH=%PYDIR%\..\externals\
SET EXTERNSDIR=../
set EXTERNSPATH=%PYDIR%\..\

rem need this for nmake and for the compile steps in this script
call "%VS90COMNTOOLS%vsvars32.bat"

CD %PYDIR%
IF NOT EXIST %SCRIPT_BUILDX86% GOTO NOBUILDSCRIPT

FOR /F "tokens=2* delims=_=" %%A IN ('SET SCRIPT_') DO (
    ECHO Processing: converting %%B to %%B.release.bat
    REM create .release.bat buld scripts from the default debug ones in the buildbot dir
	IF %RELEASE%X==YESX (
		REM change solution to build and preprocessor settings
		%PYTHON% -c "f=open(r'%%B', 'r');s=f.read();s=s.replace('DEBUG=1','');s=s.replace('Debug|','Release|');f.close();f=open(r'%%B.release.bat', 'w');f.write(s);f.close()"  || goto FAIL
		REM change the inclusion of the common .bat file
		%PYTHON% -c "f=open(r'%%B.release.bat', 'r');s=f.read();s=s.replace('external.bat', 'external.bat.release.bat'); s=s.replace('external-amd64.bat', 'external-amd64.bat.release.bat'); f=open(r'%%B.release.bat', 'w');f.write(s);f.close()"  || goto FAIL
		REM rename the tcl libraries from debug to release
		%PYTHON% -c "f=open(r'%%B.release.bat', 'r');s=f.read();s=s.replace('85g.dll', '85.dll'); f=open(r'%%B.release.bat', 'w');f.write(s);f.close()"  || goto FAIL
	)
)

REM DEBUGGING SHORTCUTS:
REM GOTO MAKEPUBLISHEDBINARIES
REM GOTO JUSTBUILD

REM avoid using the build scripts directly since they always "clean"
IF "%NOCLEAN%" == "1" goto NOCLEAN
echo **** Cleaning
cmd /c %SCRIPTDIR%\clean.bat
:NOCLEAN
echo **** Getting externals
cmd /c %SCRIPT_EXTERNALX86% || goto FAIL
cmd /c %SCRIPT_EXTERNALX86%.release.bat || goto FAIL
:JUSTBUILD
echo **** Building Release.
vcbuild /useenv PCbuild\pcbuild.sln "Release|Win32" || goto FAIL
echo **** Building Debug.
vcbuild /useenv PCbuild\pcbuild.sln "Debug|Win32" || goto FAIL

:BUILDINSTALLER
echo **** Generating documentation.
set DOCDIR=Doc
cd %DOCDIR%
cmd /c make.bat checkout || goto FAIL
cmd /c make.bat html  || goto FAIL
rem Generate zip HTML documentation.
set BASEFILENAME=%OLDDIR%\stackless-python-%PY_VERSION%-docs-html
del %BASEFILENAME%.zip
"%SEVENZIP%" a -r %BASEFILENAME%.zip html
%PYTHON% %BATDIR%\gensum.py %BASEFILENAME%.zip  || goto FAIL
rem Generate tbz HTML documentation.
del %BASEFILENAME%.tar
"%SEVENZIP%" a -r %BASEFILENAME%.tar html  || goto FAIL
"%SEVENZIP%" a %BASEFILENAME%.tar.bz2 %BASEFILENAME%.tar  || goto FAIL
del %BASEFILENAME%.tar
%PYTHON% %BATDIR%\gensum.py %BASEFILENAME%.tar.bz2  || goto FAIL
cmd /c make.bat htmlhelp
copy build\htmlhelp\python%PY_VERSIONX%.chm %OLDDIR%\python%PY_VERSIONX%sl.chm
%PYTHON% %BATDIR%\gensum.py %OLDDIR%\python%PY_VERSIONX%sl.chm  || goto FAIL
cd ..

echo **** Generating installer.
SET PCDIR=PC
cd %PCDIR%
nmake -f icons.mak  || goto FAIL
cd ..

echo Making dummy tix folder: %EXTERNSPATH%tix-nothere
IF NOT EXIST "%EXTERNSPATH%tix-nothere" (
	rem TIX is required
    mkdir %EXTERNSPATH%tix-nothere
    echo > %EXTERNSPATH%tix-nothere\license.terms
    )

IF defined HG hg revert --no-backup %MSIDIR%
cd %MSIDIR%
if defined P4 p4 revert msi.py
if defined P4 p4 edit msi.py

REM snapshot mode does not require a tools/msi/uuids.py entry for the current version
IF "%SNAPSHOT%"=="1" goto SNAPSHOT
%PYTHON% -c "f=open(r'msi.py', 'r');s=f.read();s=s.replace('snapshot = 1','snapshot = 0');f.close();f=open(r'msi.py', 'w');f.write(s);f.close()"  || goto FAIL
:SNAPSHOT
%PYTHON% -c "f=open(r'msi.py', 'r');s=f.read();s=s.replace('tcl8*','tcl-8*');s=s.replace('tk8*','tk-8*');fcv=\"%PY_VERSION%\".strip();s=s.replace('full_current_version = None','full_current_version = \"'+fcv+'\"');f.close();f=open(r'msi.py', 'w');f.write(s);f.close()"  || goto FAIL
%PYTHON% -c "f=open(r'msi.py', 'r');s=f.read();s=s.replace(r'srcdir+\"/../\"', r'srcdir+\"/%EXTERNSDIR%\"');f=open(r'msi.py', 'w');f.write(s);f.close()"  || goto FAIL

nmake -f msisupport.mak
%PYTHON% msi.py  || goto FAIL
copy python-%PY_VERSION%-stackless.msi %OLDDIR%\
%PYTHON% %BATDIR%\gensum.py %OLDDIR%\python-%PY_VERSION%-stackless.msi  || goto FAIL
copy python-%PY_VERSION%-pdb-stackless.zip %OLDDIR%\python-%PY_VERSION%-pdb-stackless.zip
%PYTHON% %BATDIR%\gensum.py %OLDDIR%\python-%PY_VERSION%-pdb-stackless.zip  || goto FAIL
cd ..\..

:MAKEPUBLISHEDBINARIES
echo **** Make published binaries
set PCBUILDDIR=PCbuild
cd %PCBUILDDIR%
python.exe publish_binaries.py || goto FAIL
copy stackless-python-%PY_VERSIONX%.zip %OLDDIR%\
%PYTHON% %BATDIR%\gensum.py %OLDDIR%\stackless-python-%PY_VERSIONX%.zip || goto FAIL
cd ..

echo "%SEVENZIP%" a Python.tar *.zip *.py *.bz2 *.msi *.chm
echo pscp Python.tar root@stackless.com:/var/www/stackless/binaries/incoming/

:CLEANUP
cd %PYDIR%
IF defined HG (
	hg revert --no-backup %SCRIPTDIR%
	hg revert --no-backup %MSIDIR%
	)
if defined P4 p4 revert %MSIDIR%\msi.py
GOTO :EOF

:FAIL
ECHO Batch file failed with error level %ERRORLEVEL%
set err=%ERRORLEVEL%
CALL :CLEANUP
exit /b %err%

:MISSING
echo Component %WHATSMISSING% not found
exit /b 1
