\name{jnew}
\alias{.jnew}
\title{
  Create a Java object
}
\description{
  \code{.jnew} create a new Java object
}
\usage{
.jnew(class, ...)
}
\arguments{
  \item{class}{fully qualified class name in JNI notation (e.g. \code{"java/lang/String"}).}
  \item{...}{
    Any parametes that will be passed to the corresponding
    constructor. The parameter types are determined automatically and/or
    taken from the \code{jobjRef} object. For details see \code{\link{.jcall}}.}
}
\value{
  Returns the reference (\code{jobjRef}) to the newly created object or
  \code{NULL} if something went wrong.
}
\seealso{
  \code{\link{.jcall}}
}
\examples{
\dontrun{
f <- .jnew("java/awt/Frame","Hello")
.jcall(f,,"setVisible",TRUE)
}
}
\keyword{interface}
