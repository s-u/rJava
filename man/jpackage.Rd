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
.jpackage(name, moreClasses='', nativeLibrary=FALSE)
}
\arguments{
  \item{name}{name of the package. It should correspond to the
    \code{pkgname} parameter of \code{.onLoad} or \code{.First.lib}
    function}
  \item{moreClasses}{vector listing any additional entries that should
    be added to the class path}
  \item{nativeLibrary}{a logical determining whether rJava should look
    for native code in the R package's shared object or not}
}
\value{
  The return value is an invisible TRUE if the initialization was successful.  
}
\details{
  \code{.packages} initializes a Java R package as follows: first the
  JVM is initialized via \code{\link{.jinit}} (if it is not running
  already). Then the \code{classes} directory of the package is added to
  the class path. Then \code{.jpackage} checks for the existence of a
  Java archive of the same name as the package in the \code{classes}
  directory. If it exists, it is added to the class path (e.g. for the
  package \code{foo} the path would be \code{classes/foo.jar} in the
  package). Finally the \code{moreClasses} parameter is simply passed to
  a call to \code{\link{.jaddClassPath}}.
  
  Therefore the easiest way to create a Java package is to place
  \code{.jpackage(pkgname)} in \code{.onLoad} or \code{.First.lib}, and
  copy all necessary classes to a JAR file which is placed in
  \code{inst/classes/foo.jar} where \code{foo} is the package name.
}
\seealso{
  \code{\link{.jpackage}}
}
\examples{
\dontrun{
.onLoad <- function(libname, pkgname) {
.jpackage(pkgname)
}
}
}
\keyword{interface}
