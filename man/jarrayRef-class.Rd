\name{jarrayRef-class}
\docType{class}
\alias{jarrayRef-class}

\title{Class "jarrayRef" Reference to an array Java object }
\description{ This class is a subclass of \link{jobjRef-class} and represents a reference to an array Java object. }
\section{Objects from the Class}{
Objects cannot be created directly, but only as the retun value of \code{\link{.jcall}} function.
}
\section{Slots}{
  \describe{
    \item{\code{jsig}:}{JNI signature of the array type}
    \item{\code{jobj}:}{Internal identifier of the object}
    \item{\code{jclass}:}{Inherited from \code{jobjRef}, but unspecified}
  }
}
\section{Extends}{
Class \code{"jobjRef"}, directly.
}
\section{Methods}{
No methods defined with class "jarrayRef" in the signature.
}
%\references{ ~put references to the literature/web site here ~ }
\author{ Simon Urbanek }
%\note{ ~~further notes~~ }

% ~Make other sections like Warning with \section{Warning }{....} ~

\seealso{
  \code{\link{.jcall}} or \code{\link{jobjRef-class}}
}
%\examples{
%##---- Should be DIRECTLY executable !! ----
%}
\keyword{classes}
