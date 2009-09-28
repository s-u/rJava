JAVA_SRC=$(wildcard *.java)
JFLAGS=-source 1.2 -target 1.2
JAVAC=javac
JAVA=java

all: compile test

compile: $(JAVA_SRC)
	$(JAVAC) $(JFLAGS) $(JAVA_SRC)

test: compile
	$(JAVA) RJavaTools_Test
	$(JAVA) RJavaArrayTools_Test

clean:
	rm -rfv *.class *~

.PHONY: all clean

