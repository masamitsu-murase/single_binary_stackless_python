
CC = cl.exe
LIB = lib.exe
ASM = ml64.exe

!include common_arch.mak

all: $(TARGET)

$(TARGET): $(OBJS)
	$(LIB) /OUT:$(TARGET) $(LFLAGS) $(OBJS)

.c.obj:
	$(CC) /c $< $(CFLAGS) $(ARCH_DEFINES) /Fo$@

.asm.obj:
	$(ASM) /Fo$@ /c $< $(AFLAGS)

clean:
	-<<delete.bat
del $(TARGET)
del $(OBJS:.obj =.obj^
del )
<<

