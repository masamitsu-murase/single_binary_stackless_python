
ProjectDir = ..
externalsDir = $(ProjectDir)\externals
!IF "$(VC_VERSION)" == "2015"
PlatformToolSet = v140
!ELSE
PlatformToolSet = v141
!ENDIF

CC = cl.exe
LINK = link.exe
MT = mt.exe

!include common_arch.mak

INCLUDES = /I "$(ProjectDir)\Python" /I "$(ProjectDir)\Modules\zlib" /I "$(ProjectDir)\Include" /I "$(ProjectDir)\PC" /I "$(ProjectDir)\Stackless" 
DEFINES = /D "Py_BUILD_CORE" /D "Py_ENABLE_SHARED" /D "WIN32" /D "_CONSOLE" /D "NDEBUG" /D "_MBCS"
CFLAGS = /O2 /Ob1 /Oi /GL $(INCLUDES) $(DEFINES) $(ARCH_DEFINES) /GF /FD /MT /Gy /W3 /nologo /Zi

!IF "$(BUILD_TARGET_CPU)" == "x86"
LFLAGS = /NOLOGO /LTCG /NODEFAULTLIB:"libc" /IMPLIB:"python.lib" /STACK:2000000 /MACHINE:X86 /SAFESEH /OPT:NOREF,NOICF /LIBPATH:$(ProjectDir)\PC\external\$(PlatformToolSet)\win32
!ELSE
LFLAGS = /NOLOGO /LTCG /NODEFAULTLIB:"libc" /IMPLIB:"python.lib" /STACK:2000000 /MACHINE:X64 /OPT:NOREF,NOICF /LIBPATH:$(ProjectDir)\PC\external\$(PlatformToolSet)\amd64
!ENDIF

!IF "$(WINDOWS_PE)" == "0"
ADDITIONAL_LIBS = _msi.lib
!ELSE
ADDITIONAL_LIBS =
!ENDIF
LIBS = _asyncio.lib _bz2.lib _cchardet.lib _ctypes.lib _decimal.lib _elementtree.lib _hashlib.lib _lzma.lib _multiprocessing.lib _overlapped.lib _socket.lib _sqlite3.lib \
        _ssl.lib _testcapi.lib _tkinter.lib _yaml.lib libuchardet.lib libyaml.lib liblzma.lib pyexpat.lib pythoncore.lib select.lib sqlite3.lib unicodedata.lib winsound.lib xxlimited.lib \
 	   $(ADDITIONAL_LIBS)

EXTERNAL_LIBS = shell32.lib user32.lib advapi32.lib ws2_32.lib ole32.lib oleaut32.lib gdi32.lib crypt32.lib msi.lib cabinet.lib rpcrt4.lib winmm.lib Shlwapi.lib Version.lib \
          $(ProjectDir)\PCbuild\libeay.lib $(ProjectDir)\PCbuild\ssleay.lib $(ProjectDir)\PCbuild\tcl86ts.lib $(ProjectDir)\PCbuild\tk86ts.lib $(ProjectDir)\PCbuild\tclstub86.lib netapi32.lib #Mincore_downlevel.lib
RESOURCE = $(ProjectDir)\PCbuild\tk_base.res
RESOURCEW = $(ProjectDir)\PCbuild\tk_basew.res

!IF "$(BUILD_TARGET_CPU)" == "x86"
TARGET = python.exe
TARGETW = pythonw.exe
!ELSEIF "$(WINDOWS_PE)" == "0"
TARGET = python64.exe
TARGETW = pythonw64.exe
!ELSE
TARGET = python64_pe.exe
TARGETW = pythonw64_pe.exe
!ENDIF


all: $(TARGET) $(TARGETW)

.c.obj:
	$(CC) /c $< $(CFLAGS) /Fo$@

$(TARGET): $(ProjectDir)\Programs\python.obj lib $(ProjectDir)\PC\python.manifest
	$(LINK) /OUT:$(TARGET) $(LFLAGS) /SUBSYSTEM:CONSOLE $(ProjectDir)\Programs\python.obj $(LIBS) $(EXTERNAL_LIBS) $(RESOURCE)
	$(MT) -manifest $(ProjectDir)\PC\python.manifest -outputresource:$(TARGET);1

$(TARGETW): $(ProjectDir)\PC\WinMain.obj lib $(ProjectDir)\PC\python.manifest
	$(LINK) /OUT:$(TARGETW) $(LFLAGS) /SUBSYSTEM:WINDOWS $(ProjectDir)\PC\WinMain.obj $(LIBS) $(EXTERNAL_LIBS) $(RESOURCEW)
	$(MT) -manifest $(ProjectDir)\PC\python.manifest -outputresource:$(TARGETW);1

lib:
	$(MAKE) /$(MAKEFLAGS) /f Makefile_ssleay
	$(MAKE) /$(MAKEFLAGS) /f Makefile_libeay
	$(MAKE) /$(MAKEFLAGS) /f Makefile_libyaml
	$(MAKE) /$(MAKEFLAGS) /f Makefile_libuchardet
	$(MAKE) /$(MAKEFLAGS) /f Makefile_tcl
	$(MAKE) /$(MAKEFLAGS) /f Makefile_tk
	$(MAKE) /$(MAKEFLAGS) /f Makefile__asyncio
	$(MAKE) /$(MAKEFLAGS) /f Makefile__bz2
	$(MAKE) /$(MAKEFLAGS) /f Makefile__cchardet
	$(MAKE) /$(MAKEFLAGS) /f Makefile__ctypes
	$(MAKE) /$(MAKEFLAGS) /f Makefile__decimal
	$(MAKE) /$(MAKEFLAGS) /f Makefile__elementtree
	$(MAKE) /$(MAKEFLAGS) /f Makefile__hashlib
	$(MAKE) /$(MAKEFLAGS) /f Makefile__lzma
	$(MAKE) /$(MAKEFLAGS) /f Makefile__msi
	$(MAKE) /$(MAKEFLAGS) /f Makefile__multiprocessing
	$(MAKE) /$(MAKEFLAGS) /f Makefile__overlapped
	$(MAKE) /$(MAKEFLAGS) /f Makefile__socket
	$(MAKE) /$(MAKEFLAGS) /f Makefile__sqlite3
	$(MAKE) /$(MAKEFLAGS) /f Makefile__ssl
	$(MAKE) /$(MAKEFLAGS) /f Makefile__testcapi
	$(MAKE) /$(MAKEFLAGS) /f Makefile__tkinter
	$(MAKE) /$(MAKEFLAGS) /f Makefile__yaml
	$(MAKE) /$(MAKEFLAGS) /f Makefile_liblzma
	$(MAKE) /$(MAKEFLAGS) /f Makefile_pyexpat
	$(MAKE) /$(MAKEFLAGS) /f Makefile_select
	$(MAKE) /$(MAKEFLAGS) /f Makefile_sqlite3
	$(MAKE) /$(MAKEFLAGS) /f Makefile_unicodedata
	$(MAKE) /$(MAKEFLAGS) /f Makefile_winsound
	$(MAKE) /$(MAKEFLAGS) /f Makefile_xxlimited
	$(MAKE) /$(MAKEFLAGS) /f Makefile_pythoncore

clean:
	-$(MAKE) /$(MAKEFLAGS) /f Makefile_ssleay clean
	-$(MAKE) /$(MAKEFLAGS) /f Makefile_libeay clean
	-$(MAKE) /$(MAKEFLAGS) /f Makefile_libyaml clean
	-$(MAKE) /$(MAKEFLAGS) /f Makefile_libuchardet clean
	-$(MAKE) /$(MAKEFLAGS) /f Makefile_tcl clean
	-$(MAKE) /$(MAKEFLAGS) /f Makefile_tk clean
	-$(MAKE) /$(MAKEFLAGS) /f Makefile__asyncio clean
	-$(MAKE) /$(MAKEFLAGS) /f Makefile__bz2 clean
	-$(MAKE) /$(MAKEFLAGS) /f Makefile__cchardet clean
	-$(MAKE) /$(MAKEFLAGS) /f Makefile__ctypes clean
	-$(MAKE) /$(MAKEFLAGS) /f Makefile__decimal clean
	-$(MAKE) /$(MAKEFLAGS) /f Makefile__elementtree clean
	-$(MAKE) /$(MAKEFLAGS) /f Makefile__hashlib clean
	-$(MAKE) /$(MAKEFLAGS) /f Makefile__lzma clean
	-$(MAKE) /$(MAKEFLAGS) /f Makefile__msi clean
	-$(MAKE) /$(MAKEFLAGS) /f Makefile__multiprocessing clean
	-$(MAKE) /$(MAKEFLAGS) /f Makefile__overlapped clean
	-$(MAKE) /$(MAKEFLAGS) /f Makefile__socket clean
	-$(MAKE) /$(MAKEFLAGS) /f Makefile__sqlite3 clean
	-$(MAKE) /$(MAKEFLAGS) /f Makefile__ssl clean
	-$(MAKE) /$(MAKEFLAGS) /f Makefile__testcapi clean
	-$(MAKE) /$(MAKEFLAGS) /f Makefile__tkinter clean
	-$(MAKE) /$(MAKEFLAGS) /f Makefile__yaml clean
	-$(MAKE) /$(MAKEFLAGS) /f Makefile_liblzma clean
	-$(MAKE) /$(MAKEFLAGS) /f Makefile_pyexpat clean
	-$(MAKE) /$(MAKEFLAGS) /f Makefile_select clean
	-$(MAKE) /$(MAKEFLAGS) /f Makefile_sqlite3 clean
	-$(MAKE) /$(MAKEFLAGS) /f Makefile_unicodedata clean
	-$(MAKE) /$(MAKEFLAGS) /f Makefile_winsound clean
	-$(MAKE) /$(MAKEFLAGS) /f Makefile_xxlimited clean
	-$(MAKE) /$(MAKEFLAGS) /f Makefile_pythoncore clean
	-del $(ProjectDir)\Modules\python.obj

