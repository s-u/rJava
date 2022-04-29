# rJava

[![CRAN](https://rforge.net/do/cransvg/rJava)](https://cran.r-project.org/package=rJava)
[![RForge](https://rforge.net/do/versvg/rJava)](https://RForge.net/rJava)
[![rJava check](https://github.com/s-u/rJava/actions/workflows/check.yaml/badge.svg)](https://github.com/s-u/rJava/actions/workflows/check.yaml)

R/Java interface allowing the use of Java from R as well as embedding
R into Java (via JRI)

Please visit the [main rJava project page on RForge.net](https://rforge.net/rJava) for details on the project. For some FAQs and troubleshooting see below - read before reporting bugs!

### Installation

Recommended installation of the CRAN version is via

    install.packages("rJava")

in R. If you have all tools (and knowledge) necessary to compile
R packages from sources, you can install the latest development
version with

    install.packages("rJava", repos="https://rforge.net")

The RForge.net repository is updated automatically on each
commit. On macOS/Windows you may need to add `type='source'`.

__IMPORTANT__: You must have Java installed and it must be of the same architecture as the R you are using. See below for some troubleshooting help.

### Sources

When checking out the sources, you *must* use

    git clone --recursive https://github.com/s-u/rJava.git

since rJava includes REngine as a submodule. If you want to create a
package from the source checkout, you __must__ use `sh mkdist` to do so
since the checkout is _not_ the actual R package but a source to
generate one (which involves compilation of Java code).

### Bug reports

Please use [rJava GitHub issues page](https://github.com/s-u/rJava/issues) to
report bugs, but read the following documentation and search previous issues before you do so.

## Troubleshooting

Rule #1: do __not__ set `JAVA_HOME` unless you are an expert. rJava attempts to find the correct settings automatically on most platforms, so setting `JAVA_HOME` incorrecty will just break things.

### Windows

Please make sure you install Java that matches your R architecture. R from CRAN is installed by default both in 32-bit and 64-bit versions so if in doubt, install both 32-bit and 64-bit Java. The most common mistake is to use 64-bit R but only have 32-bit Java installed.

rJava determines the Java location from the registry, so make sure you use the official Oracle installer so that your Java installation can be found.

### macOS

On modern macOS versions Apple no longer supplies Java, so it must be downloaded from 3rd parties. Probably the most commonly used distributions on macOS are [adoptium.net](https://adoptium.net) and [Azul Zulu](https://www.azul.com/downloads/). Please note that if you are using arm64 R on Apple silicon (M1+) based Macs you will need at least R-4.1.2 or else you will get `trap R` errors when loading Java (see [#267](https://github.com/s-u/rJava/issues/267) for details).

When installing from a zip or tar ball, put your Java installation in `/Library/Java/JavaVirtualMachines`. For example, if installing Zulu, unpack/move it such that it results in `/Library/Java/JavaVirtualMachines/zulu-11.jdk`.

Most recent rJava version will try to automatically detect the Java location and load it dynamically. You can also check the version selected by your settings via `/usr/libexec/java_home` in the Terminal.

If you have multiple versions and want to pick one without changing the macOS Java settings, you can set `JAVA_HOME` but it must point to the `Home` directory inside the JDK, so, for example, for that above Zulu JDK that would be `JAVA_HOME=/Library/Java/JavaVirtualMachines/zulu-11.jdk/Contents/Home`. Again, don't do this unless you want to change the default behavior.

### Linux

There is no standard location of JDK on Linux, so you must configure R with Java support before you can use rJava. It is usually done by running `R CMD javareconf` which detects all necessary settings and modifies the Java configuration in `$R_HOME/etc/javaconf`. Note that you must have sufficient privileges to update that file in order to configure R.

Also note that `sudo` may change environment variables, so if you need to run with elevated privileges, try `sudo -i` first then check if you still have access to the Java you want to use and then run `R CMD javareconf`. Alternatively you can temporarily change permissions on `$R_HOME/etc` to allow you to update it.

The way Java R configuration on Linux works is for the `R` start script to modify `LD_LIBRARY_PATH` to make sure the JVM libraries can be loaded (it does so according to the `javaconf` settings). Therefore if you use a process embbedding R you need to run it via `R CMD <program>` such that those setting are honored, otherwise you're on your own.

If you are installing rJava from sources, make sure you have the full JDK installed and all the necessary libraries needed to compile packages. For example, on Debian/Ubuntu that would require at least `r-base-dev`. If you run into issues, please check `config.log` which gives a clue as to what went wrong - usually some missing R dependency such as `pcre2`. The `config.log` file will be in the directory you used to build rJava in which is claned by R by default, so to keep it you can use e.g.:

```
curl -LO https://rforge.net/rJava/snapshot/rJava_1.0-6.tar.gz
tar fxz rJava_1.0-6.tar.gz
R CMD INSTALL rJava
## on failure check rJava/config.log
```
