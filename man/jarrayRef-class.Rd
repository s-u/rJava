\name{jarrayRef-class}
\docType{class}
\alias{jarrayRef-class}
\alias{[,jarrayRef-method}
\alias{[[,jarrayRef-method}
\alias{[[<-,jarrayRef-method}
\alias{head,jarrayRef-method}
\alias{tail,jarrayRef-method}
\alias{length,jarrayRef-method}
\alias{str,jarrayRef-method}

\title{Class "jarrayRef" Reference to an array Java object }
\description{ This class is a subclass of \link{jobjRef-class} 
and represents a reference to an array Java object. }
\section{Objects from the Class}{
Objects cannot be created directly, but only as the return
value of \code{\link{.jcall}} function.
}
\section{Slots}{
  \describe{
    \item{\code{jsig}:}{JNI signature of the array type}
    \item{\code{jobj}:}{Internal identifier of the object}
    \item{\code{jclass}:}{Inherited from \code{jobjRef}, but unspecified}
  }
}
\section{Methods}{
  \describe{
    \item{[}{\code{signature(x = "jarrayRef")}: R indexing of java arrays }
    \item{[[}{\code{signature(x = "jarrayRef")}: R indexing of java arrays }
    \item{[[<-}{\code{signature(x = "jarrayRef")}: replacement method }
    \item{\code{head}}{\code{signature(x = "jarrayRef")}: head of the java array }
    \item{\code{tail}}{\code{signature(x = "jarrayRef")}: tail of the java array }
    \item{length}{\code{signature(object = "jarrayRef")}: Number of java objects in the java array }
    \item{str}{\code{signature(object = "jarrayRef")}: ... }
  }
}
\section{Extends}{
Class \code{"\linkS4class{jobjRef}"}, directly.
}
\author{ Simon Urbanek }
\seealso{
  \code{\link{.jcall}} or \code{\link{jobjRef-class}}
}
\section{Warning}{
At the moment, only one dimensionnal indexing is implemented. 
}
\examples{
\dontshow{.jinit()}
v <- new( J("java.util.Vector") )
v$add( "hello" )
v$add( "world" )
v$add( new( J("java.lang.Double"), "10.2" ) )
array <- v$toArray()

array[ c(TRUE,FALSE,TRUE) ]
array[ 1:2 ]
array[ -3 ]

# length 
length( array )
\dontshow{stopifnot(length(array) == 3L)}

# also works as a pseudo field as in java
array$length
\dontshow{stopifnot(array$length == 3L)}

% # also with with
% with( array, length )
% \dontshow{
% 	stopifnot( with(array, length) == 3L )
% }

}
\keyword{classes}
