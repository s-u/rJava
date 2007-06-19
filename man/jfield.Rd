\name{jfield}
\alias{.jfield}
\title{
  Obtains the value of a field 
}
\description{
  \code{.jfield} returns the value of the specified field on an object.
}
\usage{
.jfield(o, name, simplify=TRUE, true.class=TRUE)
}
\arguments{
  \item{o}{Class name or object (Java reference) whose field is to be
    accessed. Static fields are supported both by specifying the class
    name or using an instance.}
  \item{name}{name of the field to access}
  \item{simplify}{field accessor methods always return Java objects. If
    set to \code{TRUE}, such object will be converted to scalar type if
    possible (see \code{\link{.jsimplify}}).}
  \item{true.class}{field accessor methods always return objects of the
    class \code{java.lang.Object}. If set to \code{TRUE}, the true Java
    class is determined and the returned object's class will be
    adjusted.}
}
\value{
  Contents of the field.
}
\details{
  \code{.jfield} uses reflection to access value of a field of a given
  object.
}
\seealso{
  \code{\link{.jnew}}, \code{\link{.jsimplify}}
}
\examples{
\dontrun{
z <- .jnew("java/lang/Boolean", TRUE)
.jfield(z, "TYPE")
}
}
\keyword{interface}
