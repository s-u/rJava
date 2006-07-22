\name{jfloat-class}
\docType{class}
\alias{jfloat-class}
\alias{jlong-class}
\title{Classes "jfloat" and "jlong" - numeric vector marekd as float or
  long respectively}
\description{ This class wraps a numeric vector to be treated as
  \code{float} or \code{long} argument when passed to Java. R doesn't
  distinguish between \code{double} and \code{float}, but Java does. In
  order to satisfy object types, numeric vectors that should be
  converted to floats or long on the Java side must be wrapped in this
  class.}
\section{Objects from the Class}{
  Objects can be created by calling \code{\link{.jfloat}} or
  \code{\link{.jlong}} respectively.
}
\section{Slots}{
  \describe{
    \item{\code{.Data}:}{Payload}
  }
}
\section{Extends}{
  Class \code{"numeric"}, from data part.
  Class \code{"vector"}, by class \code{"numeric"}.
}
\section{Methods}{
  "jfloat" and "jlong" have no methods other than those inherited from "numeric".
}
%\references{ ~put references to the literature/web site here ~ }
\author{ Simon Urbanek }
%\note{ ~~further notes~~ }

% ~Make other sections like Warning with \section{Warning }{....} ~

\seealso{
  \code{\link{.jfloat}}, \code{\link{.jlong}} and \code{\link{.jcall}}
}
%\examples{
%##---- Should be DIRECTLY executable !! ----
%}
\keyword{classes}
