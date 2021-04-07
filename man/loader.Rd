\name{loader}
\alias{.jaddClassPath}
\alias{.jclassPath}
\alias{.jclassLoader}
\title{
  Java Class Loader
}
\description{
  \code{.jaddClassPath} adds directories or JAR files to the class
  path.

  \code{.jclassPath} returns a vector containing the current entries in
  the class path
}
\usage{
.jaddClassPath(path, class.loader=.rJava.class.loader)
.jclassPath(class.loader=.rJava.class.loader)
.jclassLoader(package=NULL)
}
\arguments{
  \item{path}{character string vector listing the paths to add to the
    class path}
  \item{class.loader}{Java class loader to use for the query of
    madification. Defaults to global class loader.}
  \item{package}{string, name of a package or \code{NULL} for the global
  class loader}
}
\value{
  \code{.jclassPath} returns a character vector listing the class path sequence.
}
\details{
  Whenever a class needs to be instantiated in Java it is referred by
  name which is used to locate a file with the bytecode for the
  class. The mechanism to map a name to an actual bytecode to load ind
  instantiate is habdled by the Java class loader. It typically keeps a
  list of directories and JAR files to search for the class names.

  The \code{.jaddClassPath()} function allows the user to append new
  locations to the list of places which will be searched. The function
  \code{.jclassPath} retrieves the current sarch list from the loader.

  When rJava is initialized, it instantiates the global class loader
  which is responsible for finding classes in functions such as
  \code{.jnew()}. In addition to the global class loader, R packages can
  create their own class loaders to avoid conflicts between packages
  such that they can be sure to use their own files to look for
  classes. See \code{\link{.jpackage}} for details on how that works.
  If the \code{package} argument is supplied \code{.jclassLoader} will
  look in that package to see if it has a custom loader and will return
  it, otherwise it returns the global loader. Note that is will fail with
  an error when supplied a non-existing package name.

  If you want to trace issues related to missing classes, you can enable
  debugging in the class loader by using the \code{setDebug} method, for
  example: \code{.jclassLoader()$setDebug(1L)}
}
\seealso{
  \code{\link{.jpackage}}
}
\examples{
\dontrun{
.jaddClassPath("/my/jars/foo.jar","/my/classes/")
print(.jclassPath())
}
}
\keyword{interface}
