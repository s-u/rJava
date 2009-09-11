JFLAGS=-source 1.2 -target 1.2
JAVAC=javac
JAVA=java

all: run

RJavaTools.class: RJavaTools.java
	$(JAVAC) $(JFLAGS) $^

RJavaTools_Test.class: RJavaTools_Test.java
	$(JAVAC) $(JFLAGS) $^

RJavaClassLoader.class: RJavaClassLoader.java
	$(JAVAC) $(JFLAGS) $^

compile: RJavaTools.class RJavaClassLoader.class

cleanfortest: 
	rm -frv RJavaTools*.class

test: cleanfortest RJavaTools.class RJavaTools_Test.class
	$(JAVA) RJavaTools_Test

run: compile 

clean:
	rm -rfv *.class *~

.PHONY: run clean

