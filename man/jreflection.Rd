\name{jreflection}
\alias{.jmethods}
\alias{.jfields}
\alias{.jconstructors}
\title{
  Simple helper functions for Java reflection
}
\description{
  \code{.jconstructors} returns a character vector with all constructors for
  a given class or object.
  \code{.jmethods} returns a character vector with all methods for
  a given class or object.
  \code{.jfields} returns a character vector with all fields (aka attributes) for a given class or object.
}
\usage{
.jconstructors(o, as.obj = FALSE, class.loader=.rJava.class.loader)
.jmethods(o, name = NULL, as.obj = FALSE, class.loader=.rJava.class.loader)
.jfields(o, name = NULL, as.obj = FALSE, class.loader=.rJava.class.loader)
}
\arguments{
  \item{o}{Name of a class (either notation is fine) or an object whose
    class will be queried}
  \item{name}{string, regular expression of the method/field to look for}
  \item{as.obj}{if \code{TRUE} then a list of Java objects is
    returned, otherwise a character vector (obtained by calling
    \code{toString()} on each entry).}
  \item{class.loader}{optional, class loader to use for class look up if
    needed (i.e., if \code{o} is a string)}
}
\value{
  Returns a character vector (if \code{as.obj} is \code{FALSE}) or a
  list of Java objects. Each entry corresponds to the
  \code{Constructor} resp. \code{Method} resp. \code{Field}
  object. The string result is constructed by calling
  \code{toString()} on the objects.
}
\details{
  There first two functions are intended to help with finding correct
  signatures for methods and constructors. Since the low-level API in
  rJava doesn't use reflection automatically, it is necessary to
  provide a proper signature. That is somewhat easier using the above
  methods.
}
\seealso{
  \code{\link{.jcall}}, \code{\link{.jnew}}, \code{\link{.jcast}} or \code{\link{$,jobjRef-method}}
}
\examples{
\dontrun{
.jconstructors("java.util.Vector")
v <- .jnew("java.util.Vector")
.jmethods(v, "add")
}
}
\keyword{interface}
