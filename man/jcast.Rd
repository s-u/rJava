\name{jcast}
\alias{.jcast}
\title{
  Cast a Java object to another class
}
\description{
  \code{.jcast} returns a Java object reference cast to another Java class.
}
\usage{
.jcast(obj, new.class)
}
\arguments{
  \item{obj}{a Java object reference}
  \item{new.class}{fully qualified class name in JNI notation
    (e.g. \code{"java/lang/String"}). Although rJava itself performs no
    type check in \code{.jcast}, Java will produce an exception on the
    first use if the cast is illegal.}
}
\value{
  Returns a Java object reference (\code{jobjRef}) to the object
  \code{obj}, changing the object class.
}
\details{
  This function is necesary if the argument of \code{\link{.jcall}} or
  \code{link{.jnew}} is defined as the superclass of the object to be
  passed. (See \code{\link{.jcall}}) No type check is performed and the
  original object is not modified.
}
\seealso{
  \code{\link{.jcall}}
}
\examples{
\dontrun{
v <- .jnew("java/util/Vector")
.jcall("java/lang/System","I","identityHashCode",.jcast(v, "java/lang/Object"))
}
}
\keyword{interface}
