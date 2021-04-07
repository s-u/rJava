# rJava

[![CRAN](https://rforge.net/do/cransvg/rJava)](https://cran.r-project.org/package=rJava)
[![RForge](https://rforge.net/do/versvg/rJava)](https://RForge.net/rJava)
[![rJava check](https://github.com/s-u/rJava/actions/workflows/check.yaml/badge.svg)](https://github.com/s-u/rJava/actions/workflows/check.yaml)

R/Java interface allowing the use of Java from R as well as embedding
R into Java (via JRI)

Please visit the [main rJava project page on RForge.net](http://rforge.net) for details.

### Installation

Recommended installation of the CRAN version is via

    install.packages("rJava")

in R. If you have all tools (and knowledge) necessary to compile
R packages from sources, you can install the latest development
version with

    install.packages("rJava", repos="http://rforge.net")

The RForge.net repository is updated automatically on each
commit. On macOS/Windows you may need to add `type='source'`.

### Sources

When checking out the sources, you *must* use

    git clone --recursive https://github.com/s-u/rJava.git

since rJava includes REngine as a submodule. If you want to create a
package from the source checkout, you __must__ use `sh mkdist` to do so
since the checkout is _not_ the actual R package but a source to
generate one (which involves compilation of Java code).

### Bug reports

Please use [rJava GitHub issues page](https://github.com/s-u/rJava/issues) to
report bugs.
