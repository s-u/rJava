# rJava
R/Java interface allowing the use of Java from R as well as embedding
R into Java (via JRI)

Please visit the [main rJava project page on RForge.net](http://rforge.net) for details.

### Installation

Recommended installation of the latest development version is via

    install.packages("rJava",,"http://rforge.net")

in R. The RForge.net repository is updated automatically on each
commit. On OS X you may need to add `type='source'`.

### Sources

When checking out the sources, you *must* use

    git clone --recursive https://github.com/s-u/rJava.git

since rJava includes REngine as a submodule. To build this package so that 
it can be installed, you *must* use `sh mkdist` to do so
since the checkout is not the actual R package but a source to
generate one (which involves compilation of Java code). Note this package 
is not build with the normal unix sequence of ./configure, make. Successful 
building produces an R source package as a .tar.gz one level above the source 
directory. 

### Bug reports

Please use [rJava GitHub issues page](https://github.com/s-u/rJava/issues) to
report bugs.
