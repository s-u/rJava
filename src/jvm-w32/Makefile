# Build libjvm.dll.a from jvm.def based on jvm.dll (JDK 1.2 or later)
# (Windows only!)

all: libjvm.dll.a

libjvm.dll.a: jvm.def
	dlltool --input-def jvm.def --kill-at --dllname jvm.dll --output-lib libjvm.dll.a

clean:
	rm -f libjvm.dll.a

.PHONY: all clean
