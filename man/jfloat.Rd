\name{jfloat}
\alias{.jfloat}
\title{
  Wrap numeric vector as flat Java parameter
}
\description{
  \code{.jfloat} marks a numberic vector as an object that can be used as parameter to Java calls that require \code{float} parameters.
}
\usage{
.jfloat(x)
}
\arguments{
  \item{x}{numeric vector}
}
\value{
  Returns a numeric vector fo the class \code{jfloat} that can be used as parameter to Java calls that require \code{float} parameters. 
}
\details{
  R has no native \code{float} type. Numeric vectors are stored as \code{double}s, hence there is no native way to pass float numbers to Java methods. The \code{.jfloat} call marks a numeric vector as having the Java type \code{float} by wrapping it in the \code{jfloat} class. The class is still a subclass of \code{numeric}, therefore all regular R operations are unaffected by this.
}
\seealso{
  \code{\link{.jcall}}, \code{\link{jfloat-class}}
}
%\examples{
%\dontrun{
%v <- .jnew("java/util/Vector")
%.jcall("java/lang/System","I","identityHashCode",.jcast(v, "java/lang/Object"))
%}
%}
\keyword{interface}
