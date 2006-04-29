# helper tools and libs for building and running rJava for Windows
# Author: Simon Urbanek

TARGETS=libjvm.dll.a findjava.exe WinRegistry.dll

# libjvm.dll.a    - wrapper lib for jvm.dll from Java
# findjava.exe    - helper tool to find the current JDK from the registry
# WinRegistry.dll - helper lib to provide registry access to rJava before
#                   rJava.dll is loaded, because at that point path to
#                   jvm.dll must be known. We don't hard-code the JVM path,
#                   because updates of JRE/JDK can re-locate it.

all: $(TARGETS)

libjvm.dll.a: jvm.def
	dlltool --input-def jvm.def --kill-at --dllname jvm.dll --output-lib libjvm.dll.a

# compile findjava.exe from source - no magic here, no special libs necessary
findjava.o: findjava.c
	$(CC) -O2 -c -o $@ $^

findjava.exe: findjava.o
	$(CC) -s -o $@ $^

# compile WinRegistry.dll - this can be tricky, because it should emulate
# the way R builds package DLLs. I know of no easier or more reliable way
# to do that ..
WinRegistry.o: WinRegistry.c
	$(CC) -O2 -c -o $@ $^ -I$(RHOME)/include

WinRegistry.dll: WinRegistry.o
	$(CC) --shared -s -o $@ $^ -lR -L$(RHOME)/src/gnuwin32 -L$(RHOME)/bin

# just cleanup everything
clean:
	rm -f *.o *~ $(TARGETS)

.PHONY: all clean
