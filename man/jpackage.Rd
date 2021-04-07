\name{jpackage}
\alias{.jpackage}
\title{
  Initialize an R package containing Java code
}
\description{
  \code{.jpackage} initializes the Java Virtual Machine (JVM) for an R
  package. In addition to starting the JVM it also registers Java
  classes and native code contained in the package with the JVM.
  function must be called before any rJava functions can be used.
}
\usage{
.jpackage(name, jars='*', morePaths='', nativeLibrary=FALSE,
          lib.loc=NULL, parameters = getOption("java.parameters"),
          own.loader = FALSE)
}
\arguments{
  \item{name}{name of the package. It should correspond to the
    \code{pkgname} parameter of \code{.onLoad} or \code{.First.lib}
    function.}
  \item{jars}{Java archives in the \code{java} directory of the package
    that should be added to the class path. The paths must be relative
    to package's \code{java} directory. A special value of
    \code{'*'} adds all \code{.jar} files from the \code{java} the
    directory.}
  \item{morePaths}{vector listing any additional entries that should
    be added to the class path.}
  \item{nativeLibrary}{a logical determining whether rJava should look
    for native code in the R package's shared object or not.}
  \item{lib.loc}{a character vector with path names of R libraries, or
    \code{NULL} (see \code{\link{system.file}} and examples below).}
  \item{parameters}{optional JVM initialization parameters which will be
    used if JVM is not initilized yet (see \code{\link{.jinit}}).}
  \item{own.loader}{if \code{TRUE} then a new, separate class loader
    will be initilized for the package and assigned to the
    \code{.pkg.class.loader} variable in the package namespace. New
    packages should make use of this feature.}
}
\value{
  The return value is an invisible TRUE if the initialization was successful.  
}
\details{
  \code{.jpackage} initializes a Java R package as follows: first the
  JVM is initialized via \code{\link{.jinit}} (if it is not running
  already). Then the \code{java} directory of the package is added to
  the class path. Then \code{.jpackage} prepends \code{jars} with the
  path to the \code{java} directory of the package and adds them to the
  class path (or all \code{.jar} files if \code{'*'} was specified).
  Finally the \code{morePaths} parameter (if set) is passed to a call
  to \code{\link{.jaddClassPath}}.

  Therefore the easiest way to create a Java package is to add
  \code{.jpackage(pkgname, lib.loc=libname)} in \code{.onLoad} or
  \code{.First.lib}, and copy all necessary classes to a JAR file(s)
  which is placed in the \code{inst/java/} directory of the source
  package.

  If a package needs special Java parameters, \code{"java.parameters"}
  option can be used to set them on initialization. Note, however, that
  Java parameters can only be used during JVM initialization and other
  package may have intialized JVM already.

  Since rJava 0.9-14 there is support of package-specific class
  loaders using the \code{own.loader=TRUE} option. This is important for
  packages that may be using classes that conflict with other packages
  are therefore is highly recommended for new packages. Before this
  feature, there was only one global class loader which means that the
  class path was shared for all class look ups. If two packages
  use the same (fully qualified) class name, even in a dependency, they
  are likely to clash with each if they don't use exactly the same
  version. Therefore it is safer for each package use use a private
  class loader for its classes to guarantee that the only the classes
  supplied with the package will be used. To do that, a package will set
  \code{own.loader=TRUE} which instructs rJava to not change the global
  loader, but instead create a separate one for the package and assign
  it to \code{.rJava.class.loader} in the package namespace. Then if
  package wants to instantiate a new class, it would use
  \code{.jnew("myClass", class.loader=.rJava.class.loader)} to use its
  own loader instead of the global one. The global loader's class path
  won't be touched, so it won't find the package's classes. It is
  possible to get the loader used in a package using
  \code{.jclassLoader(package="foo")} which will return the global one if
  the package has not registered its own. Similarly, to retrieve the
  class path used by a package, one would use
  \code{.jclassPath(.jclassLoader(package="foo"))}.

  Note that with the advent of multiple class loaders the value of the
  \code{java.class.path} property is no longer meaningful as it can
  reflect only one of the loaders.
}
\seealso{
  \code{\link{.jinit}}
}
\examples{
\dontrun{
.onLoad <- function(libname, pkgname) {
  .jpackage(pkgname, lib.loc=libname, own.loader=TRUE)
  ## do not use, just an illustration of the concept:
  cat("my Java class path: ")
  print(.jclassPath(.jclassLoader(package=pkgname)))
}
}
}
\keyword{interface}
