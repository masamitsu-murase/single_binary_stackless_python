
CC = cl.exe
LIB = lib.exe
!IF "$(BUILD_TARGET_CPU)" == "x86"
ASM = ml.exe
!ELSE
ASM = ml64.exe
!ENDIF


all: $(TARGET)

$(TARGET): $(OBJS)
	$(LIB) /OUT:$(TARGET) $(LFLAGS) @<<
$(OBJS)
<<

.c.obj:
	$(CC) /c $< $(CFLAGS) $(ARCH_DEFINES) /Fo$@

.cpp.obj:
	$(CC) /c $< $(CFLAGS) $(ARCH_DEFINES) /Fo$@

.asm.obj:
	$(ASM) /Fo$@ /c $< $(AFLAGS)

clean:
	-<<delete.bat
del $(TARGET)
del $(OBJS:.obj =.obj^
del )
<<

