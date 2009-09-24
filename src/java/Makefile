JFLAGS=-source 1.2 -target 1.2
JAVAC=javac
JAVA=java

all: run

RJavaArrayTools_Test.class: RJavaArrayTools_Test.java
	$(JAVAC) $(JFLAGS) $^

RJavaArrayTools.class: RJavaArrayTools.java
	$(JAVAC) $(JFLAGS) $^

RJavaTools.class: RJavaTools.java
	$(JAVAC) $(JFLAGS) $^

RJavaTools_Test.class: RJavaTools_Test.java
	$(JAVAC) $(JFLAGS) $^

RJavaClassLoader.class: RJavaClassLoader.java
	$(JAVAC) $(JFLAGS) $^

compile: RJavaTools.class RJavaClassLoader.class RJavaArrayTools.class

cleanfortest: 
	rm -frv RJavaTools*.class
	rm -frv RJavaArrayTools*.class

test: cleanfortest RJavaTools.class RJavaTools_Test.class RJavaArrayTools_Test.class
	$(JAVA) RJavaTools_Test
	$(JAVA) RJavaArrayTools_Test

run: compile 

clean:
	rm -rfv *.class *~

.PHONY: run clean

