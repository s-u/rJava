\name{length}
\alias{length,jobjRef-method}
\alias{length,jarrayRef-method}
\title{Number of elements of a java array}
\description{
The \code{length} method returns the number of java objects
contained in the java array. An error is generated
if the object is not a java array.
}
\section{Methods}{
  \describe{
    \item{length}{\code{signature(object = "jobjRef")}: Number of java objects in the java array }
    \item{length}{\code{signature(object = "jarrayRef")}: Number of java objects in the java array }
  }
}
\examples{
\dontshow{.jinit()}
v <- new( J("java.util.Vector") )
v$add( "hello" )
v$add( new( J("java.lang.Double"), "10.2" ) )
array <- v$toArray()
length( array )
\dontshow{stopifnot(length(array) == 2L)}
# also works as a pseudo field as in java
array$length
\dontshow{stopifnot(array$length == 2L)}
}

\keyword{interface}

