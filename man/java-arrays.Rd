\name{java-arrays}
\alias{[,jobjRef,ANY,missing-method}
\alias{[[,jobjRef,numeric,missing-method}
\alias{[[,jobjRef,integer,missing-method}
\alias{head,jobjRef-method}
\alias{tail,jobjRef-method}
\title{Indexing of java arrays}
\description{ R indexing of java arrays }
\section{Methods}{
  \describe{
    \item{[}{\code{signature(x = "jobjRef", i = "ANY", j = "missing")}: R indexing of java arrays }
    \item{[[}{\code{signature(x = "jobjRef", i = "numeric", j = "missing")}: R indexing of java arrays }
    \item{[[}{\code{signature(x = "jobjRef", i = "integer", j = "missing")}: R indexing of java arrays }
    \item{\code{head}}{\code{signature(x = "jobjRef")}: head of the java array }
    \item{\code{tail}}{\code{signature(x = "jobjRef")}: tail of the java array }
  }
}
\seealso{
  \code{\link{length}} to query the number of objects in a java array
}
\section{warning}{Only 1 dimensional array indexing is currently supported.}

\details{ 
	Arguments \code{drop} and \code{simplify} may be used in the 
	indexer. 
	
	\code{drop} is used when the resulting array only contains one object, 
	in which case only the object is returned if drop is \code{TRUE} instead 
	of an array of length one.
	
	\code{simplify} controls whether the result should be simplified 
	to R data structures if possible.
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

array[ 1, drop = FALSE ] # an array of length 1
array[ 1 ] # a java String
array[ 1, simplify = TRUE ] # an R character vector

}
\keyword{classes}
