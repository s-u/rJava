\name{jfloat}
\alias{.jfloat}
\alias{.jlong}
\alias{jfloat}
\alias{jlong}
\title{
  Wrap numeric vector as flat Java parameter
}
\description{
  \code{.jfloat} marks a numberic vector as an object that can be used
  as parameter to Java calls that require \code{float} parameters.
  Similarly, \code{.jlong} marks a numeric vector as \code{long} parameter.
}
\usage{
.jfloat(x)
.jlong(x)
}
\arguments{
  \item{x}{numeric vector}
}
\value{
  Returns a numeric vector of the class \code{jfloat} or \code{jlong}
  that can be used as parameter to Java calls that require
  \code{float} or \code{long} parameters respectively. 
}
\details{
  R has no native \code{float} or \code{long} type. Numeric vectors are
  stored as \code{double}s, hence there is no native way to pass float
  numbers to Java methods. The \code{.jfloat} call marks a numeric
  vector as having the Java type \code{float} by wrapping it in the
  \code{jfloat} class. The class is still a subclass of \code{numeric},
  therefore all regular R operations are unaffected by this.

  Similarly, \code{.jlong} is used to mark a numeric vector as a
  parameter of the \code{long} Java type. Please note that in general R
  has no native type that will hold a \code{long} value, so conversion
  between Java's \code{long} type and R's numeric is potentially lossy.
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
