\name{jrectRef-class}
\Rdversion{1.1}
\docType{class}
\alias{jrectRef-class}
\title{Rectangular java arrays}
\description{References to java arrays that are guaranteed to be rectangular, i.e similar 
to R arrays}
\section{Objects from the Class}{
Objects of this class should *not* be created directly. 
Instead, they usually come as a result of a java method call. 
}
\section{Slots}{
  \describe{
    \item{\code{jsig}:}{JNI signature of the array type}
    \item{\code{jobj}:}{Internal identifier of the object}
    \item{\code{jclass}:}{Inherited from \code{jobjRef}, but unspecified}
    \item{\code{dimension}:}{Object of class \code{"integer"} dimension of the }
  }
}
\section{Extends}{
Class \code{"\linkS4class{jarrayRef}"}, directly.
Class \code{"\linkS4class{jobjRef}"}, by class "jarrayRef", distance 2.
}
\section{Methods}{
No methods defined with class "jrectRef" in the signature.
}
% \examples{
% showClass("jrectRef")
% }
\keyword{classes}
