JAVA_SRC=$(wildcard *.java)
JFLAGS=-source 1.2 -target 1.2
JAVAC=javac
JAVA=java
JAVADOC=javadoc
JAVADOCFLAGS=-author -version -breakiterator -link http://java.sun.com/j2se/1.4.2/docs/api

all: compile test

compile: $(JAVA_SRC)
	$(JAVAC) $(JFLAGS) $(JAVA_SRC)

test: compile
	$(JAVA) RJavaTools_Test
	$(JAVA) RJavaArrayTools_Test
	$(JAVA) ArrayWrapper_Test
	$(JAVA) RectangularArrayBuilder_Test

javadoc:
	$(JAVADOC) $(JAVADOCFLAGS) -d javadoc $(JAVA_SRC)

clean:
	rm -rfv *.class *~

.PHONY: all clean

