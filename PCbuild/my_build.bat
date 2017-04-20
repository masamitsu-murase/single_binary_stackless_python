
set VSTOOLS=%VS110COMNTOOLS%
call "%VSTOOLS%..\..\VC\vcvarsall.bat" %*

cd /d "%~dp0"
set IncludeTkinter=false
call get_externals.bat

nmake
