\name{jfloat}
\alias{.jfloat}
\alias{.jlong}
\alias{.jbyte}
\alias{.jchar}
\alias{.jshort}
\alias{jfloat}
\alias{jlong}
\alias{jbyte}
\alias{jchar}
\alias{jshort}
\title{
  Wrap numeric vector as flat Java parameter
}
\description{
  \code{.jfloat} marks a numeric vector as an object that can be used
  as parameter to Java calls that require \code{float} parameters.
  Similarly, \code{.jlong} marks a numeric vector as \code{long}
  parameter, \code{.jshort} as \code{short} and \code{.jbyte} as
  \code{byte}.
}
\usage{
.jfloat(x)
.jlong(x)
.jbyte(x)
.jchar(x)
.jshort(x)
}
\arguments{
  \item{x}{numeric vector}
}
\value{
  Returns a numeric vector of the class \code{jfloat}, \code{jlong},
  \code{jbyte}, \code{jshort} or \code{jchar}
  that can be used as parameter to Java calls that require
  \code{float}, \code{long}, \code{byte}, \code{short} or \code{char}
  parameters respectively.
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

  \code{.jbyte} is used when a scalar byte is to be passed to Java. Note
  that byte arrays are natively passed as raw vectors, not as
  \code{.jbyte} arrays, although non-scalar \code{.jbyte} is equivalent
  except for using four-times as much memory.

  \code{.jchar} is strictly experimental and uses integer vector as
  storage class. The type \code{char} in Java
  represents 16-bit Unicode code points (not to be confused with
  \code{char} in C which is \code{byte} in Java!), see Java
  documentation for details. \code{x} can also be a non-\code{NA} string
  in which case \code{.jchar(x)} is just a shorthand for
  \code{.jnew("java.lang.String", x)$toCharArray()} and thus performs a
  Java call (unlike all other functions mentioned here).
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
