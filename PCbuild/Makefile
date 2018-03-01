
ProjectDir = ..
externalsDir = $(ProjectDir)\externals

CC = cl.exe
LINK = link.exe

!include common_arch.mak

INCLUDES = /I "$(ProjectDir)\Python" /I "$(ProjectDir)\Modules\zlib" /I "$(ProjectDir)\Include" /I "$(ProjectDir)\PC" /I "$(ProjectDir)\Stackless" 
DEFINES = /D "Py_BUILD_CORE" /D "Py_ENABLE_SHARED" /D "WIN32" /D "_CONSOLE" /D "NDEBUG" 
CFLAGS = /O2 /Ob1 /Oi /GL $(INCLUDES) $(DEFINES) $(ARCH_DEFINES) /GF /FD /MT /Gy /W3 /nologo /Zi

!IF "$(BUILD_TARGET_CPU)" == "x64"
LFLAGS = /NOLOGO /LTCG /NODEFAULTLIB:"libc" /IMPLIB:"python27.lib" /SUBSYSTEM:CONSOLE /STACK:2000000 /MACHINE:X64
!ELSE
LFLAGS = /NOLOGO /LTCG /NODEFAULTLIB:"libc" /IMPLIB:"python27.lib" /SUBSYSTEM:CONSOLE /STACK:2000000 /MACHINE:X86
!ENDIF

!IF "$(WINDOWS_PE)" == "0"
ADDITIONAL_LIBS = _msi.lib
!ELSE
ADDITIONAL_LIBS =
!ENDIF
LIBS = _bsddb.lib _ctypes.lib _elementtree.lib _hashlib.lib _multiprocessing.lib _socket.lib _sqlite3.lib \
       _ssl.lib bz2.lib pyexpat.lib pythoncore.lib select.lib sqlite3.lib unicodedata.lib winsound.lib pythoncore.lib \
	   $(ADDITIONAL_LIBS)

EXTERNAL_LIBS = shell32.lib user32.lib advapi32.lib ws2_32.lib ole32.lib oleaut32.lib gdi32.lib crypt32.lib msi.lib cabinet.lib rpcrt4.lib winmm.lib \
          $(ProjectDir)\PCbuild\libeay.lib $(ProjectDir)\PCbuild\ssleay.lib

!IF "$(BUILD_TARGET_CPU)" == "x86"
TARGET = python.exe
!ELSEIF "$(WINDOWS_PE)" == "0"
TARGET = python64.exe
!ELSE
TARGET = python64_pe.exe
!ENDIF


all: $(TARGET)

.c.obj:
	$(CC) /c $< $(CFLAGS) /Fo$@

$(TARGET): $(ProjectDir)\Modules\python.obj lib
	$(LINK) /OUT:$(TARGET) $(LFLAGS) $(ProjectDir)\Modules\python.obj $(LIBS) $(EXTERNAL_LIBS)

lib:
	$(MAKE) /$(MAKEFLAGS) /f Makefile_ssleay
	$(MAKE) /$(MAKEFLAGS) /f Makefile_libeay
	$(MAKE) /$(MAKEFLAGS) /f Makefile__bsddb
	$(MAKE) /$(MAKEFLAGS) /f Makefile__ctypes
	$(MAKE) /$(MAKEFLAGS) /f Makefile__elementtree
	$(MAKE) /$(MAKEFLAGS) /f Makefile__hashlib
	$(MAKE) /$(MAKEFLAGS) /f Makefile__msi
	$(MAKE) /$(MAKEFLAGS) /f Makefile__multiprocessing
	$(MAKE) /$(MAKEFLAGS) /f Makefile__socket
	$(MAKE) /$(MAKEFLAGS) /f Makefile__sqlite3
	$(MAKE) /$(MAKEFLAGS) /f Makefile__ssl
	$(MAKE) /$(MAKEFLAGS) /f Makefile_bz2
	$(MAKE) /$(MAKEFLAGS) /f Makefile_pyexpat
	$(MAKE) /$(MAKEFLAGS) /f Makefile_select
	$(MAKE) /$(MAKEFLAGS) /f Makefile_sqlite3
	$(MAKE) /$(MAKEFLAGS) /f Makefile_unicodedata
	$(MAKE) /$(MAKEFLAGS) /f Makefile_winsound
	$(MAKE) /$(MAKEFLAGS) /f Makefile_pythoncore

clean:
	-$(MAKE) /$(MAKEFLAGS) /f Makefile_ssleay clean
	-$(MAKE) /$(MAKEFLAGS) /f Makefile_libeay clean
	-$(MAKE) /$(MAKEFLAGS) /f Makefile__bsddb clean
	-$(MAKE) /$(MAKEFLAGS) /f Makefile__ctypes clean
	-$(MAKE) /$(MAKEFLAGS) /f Makefile__elementtree clean
	-$(MAKE) /$(MAKEFLAGS) /f Makefile__hashlib clean
	-$(MAKE) /$(MAKEFLAGS) /f Makefile__msi clean
	-$(MAKE) /$(MAKEFLAGS) /f Makefile__multiprocessing clean
	-$(MAKE) /$(MAKEFLAGS) /f Makefile__socket clean
	-$(MAKE) /$(MAKEFLAGS) /f Makefile__sqlite3 clean
	-$(MAKE) /$(MAKEFLAGS) /f Makefile__ssl clean
	-$(MAKE) /$(MAKEFLAGS) /f Makefile_bz2 clean
	-$(MAKE) /$(MAKEFLAGS) /f Makefile_pyexpat clean
	-$(MAKE) /$(MAKEFLAGS) /f Makefile_pythoncore clean
	-$(MAKE) /$(MAKEFLAGS) /f Makefile_select clean
	-$(MAKE) /$(MAKEFLAGS) /f Makefile_sqlite3 clean
	-$(MAKE) /$(MAKEFLAGS) /f Makefile_unicodedata clean
	-$(MAKE) /$(MAKEFLAGS) /f Makefile_winsound clean
	-$(MAKE) /$(MAKEFLAGS) /f Makefile_pythoncore clean
	-del $(ProjectDir)\Modules\python.obj

