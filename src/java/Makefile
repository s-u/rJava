JFLAGS=-source 1.2 -target 1.2
JAVAC=javac

all: run

RJavaTools.class: RJavaTools.java
	$(JAVAC) $(JFLAGS) $^

RJavaClassLoader.class: RJavaClassLoader.java
	$(JAVAC) $(JFLAGS) $^

run: RJavaTools.class RJavaClassLoader.class

clean:
	rm -rfv *.class *~

.PHONY: run clean

