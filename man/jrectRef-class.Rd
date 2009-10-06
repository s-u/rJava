\name{jrectRef-class}
\Rdversion{1.1}
\docType{class}
\alias{jrectRef-class}
\alias{[,jrectRef-method}
\alias{str,jrectRef-method}
\alias{dim,jrectRef-method}
\alias{dim<-,jrectRef-method}

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
    \item{\code{dimension}:}{dimension vector of the array}
  }
}
\section{Extends}{
Class \code{"\linkS4class{jarrayRef}"}, directly.
Class \code{"\linkS4class{jobjRef}"}, by class "jarrayRef", distance 2.
}
\section{Methods}{
  \describe{
    \item{[}{\code{signature(x = "jrectRef")}: R indexing of rectangular java arrays }
    \item{str}{\code{signature(object = "jrectRef")}: ... }
    \item{dim}{\code{signature(object = "jrectRef")}: extracts the dimensions of the array }
    \item{dim<-}{\code{signature(object = "jrectRef")}: sets the dimensions of the array }
  }
}
\examples{
\dontshow{
# these examples are only unit tests so far
.jinit()

# # 2d
dim2d <- c(5L, 2L)

x <- .jcall( "RectangularArrayExamples", "[[Z", "getBooleanDoubleRectangularArrayExample",  evalArray = FALSE )
stopifnot( identical( typeof( x ), "logical" ) )
stopifnot( identical( dim(x) , dim2d ) )
stopifnot( identical( as.vector(x), rep( c(FALSE,TRUE), 5 ) ) )

x <- .jcall( "RectangularArrayExamples", "[[I", "getIntDoubleRectangularArrayExample",  evalArray = FALSE )
stopifnot( identical( typeof( x ), "integer" ) )
stopifnot( identical( dim(x) , dim2d ) )
stopifnot( identical( as.vector(x), 0:9 ) )
 
x <- .jcall( "RectangularArrayExamples", "[[B", "getByteDoubleRectangularArrayExample",  evalArray = FALSE )
stopifnot( identical( typeof( x ), "raw" ) )
stopifnot( identical( dim(x) , dim2d ) )
stopifnot( identical( as.vector(x), as.raw(0:9) ) )

x <- .jcall( "RectangularArrayExamples", "[[J", "getLongDoubleRectangularArrayExample",  evalArray = FALSE )
stopifnot( identical( typeof( x ), "double" ) )
stopifnot( identical( dim(x) , dim2d ) )
stopifnot( identical( as.vector(x), as.numeric(0:9) ) )

x <- .jcall( "RectangularArrayExamples", "[[S", "getShortDoubleRectangularArrayExample",  evalArray = FALSE )
stopifnot( identical( typeof( x ), "integer" ) )
stopifnot( identical( dim(x) , dim2d ) )
stopifnot( identical( as.vector(x), 0:9 ) )

x <- .jcall( "RectangularArrayExamples", "[[D", "getDoubleDoubleRectangularArrayExample",  evalArray = FALSE )
stopifnot( identical( typeof( x ), "double" ) )
stopifnot( identical( dim(x) , dim2d ) )
stopifnot( identical( as.vector(x), as.numeric(0:9) ) )

x <- .jcall( "RectangularArrayExamples", "[[C", "getCharDoubleRectangularArrayExample",  evalArray = FALSE )
stopifnot( identical( typeof( x ), "integer" ) )
stopifnot( identical( dim(x) , dim2d ) )
stopifnot( identical( as.vector(x), 0:9 ) )

x <- .jcall( "RectangularArrayExamples", "[[F", "getFloatDoubleRectangularArrayExample",  evalArray = FALSE )
stopifnot( identical( typeof( x ), "double" ) )
stopifnot( identical( dim(x) , dim2d ) )
stopifnot( identical( as.vector(x), as.numeric(0:9) ) )

x <- .jcall( "RectangularArrayExamples", "[[Ljava/lang/String;", "getStringDoubleRectangularArrayExample",  evalArray = FALSE )
stopifnot( identical( typeof( x ), "character" ) )
stopifnot( identical( dim(x) , dim2d ) )
stopifnot( identical( as.vector(x), as.character(0:9) ) )


# 3d

dim3d <- c(5L, 3L, 2L) 

x <- .jcall( "RectangularArrayExamples", "[[[Z", "getBooleanTripleRectangularArrayExample",  evalArray = FALSE )
stopifnot( identical( typeof( x ), "logical" ) )
stopifnot( identical( dim(x) , dim3d ) )
stopifnot( identical( as.vector(x), rep( c(FALSE,TRUE), 15L ) ) )

x <- .jcall( "RectangularArrayExamples", "[[[I", "getIntTripleRectangularArrayExample",  evalArray = FALSE )
stopifnot( identical( typeof( x ), "integer" ) )
stopifnot( identical( dim(x) , dim3d ) )
stopifnot( identical( as.vector(x), 0:29 ) )
 
x <- .jcall( "RectangularArrayExamples", "[[[B", "getByteTripleRectangularArrayExample",  evalArray = FALSE )
stopifnot( identical( typeof( x ), "raw" ) )
stopifnot( identical( dim(x) , dim3d ) )
stopifnot( identical( as.vector(x), as.raw(0:29) ) )

x <- .jcall( "RectangularArrayExamples", "[[[J", "getLongTripleRectangularArrayExample",  evalArray = FALSE )
stopifnot( identical( typeof( x ), "double" ) )
stopifnot( identical( dim(x) , dim3d ) )
stopifnot( identical( as.vector(x), as.numeric(0:29) ) )

x <- .jcall( "RectangularArrayExamples", "[[[S", "getShortTripleRectangularArrayExample",  evalArray = FALSE )
stopifnot( identical( typeof( x ), "integer" ) )
stopifnot( identical( dim(x) , dim3d ) )
stopifnot( identical( as.vector(x), 0:29 ) )

x <- .jcall( "RectangularArrayExamples", "[[[D", "getDoubleTripleRectangularArrayExample",  evalArray = FALSE )
stopifnot( identical( typeof( x ), "double" ) )
stopifnot( identical( dim(x) , dim3d ) )
stopifnot( identical( as.vector(x), as.numeric(0:29) ) )

x <- .jcall( "RectangularArrayExamples", "[[[C", "getCharTripleRectangularArrayExample",  evalArray = FALSE )
stopifnot( identical( typeof( x ), "integer" ) )
stopifnot( identical( dim(x) , dim3d ) )
stopifnot( identical( as.vector(x), 0:29 ) )

x <- .jcall( "RectangularArrayExamples", "[[[F", "getFloatTripleRectangularArrayExample",  evalArray = FALSE )
stopifnot( identical( typeof( x ), "double" ) )
stopifnot( identical( dim(x) , dim3d ) )
stopifnot( identical( as.vector(x), as.numeric(0:29) ) )

x <- .jcall( "RectangularArrayExamples", "[[[Ljava/lang/String;", "getStringTripleRectangularArrayExample",  evalArray = FALSE )
stopifnot( identical( typeof( x ), "character" ) )
stopifnot( identical( dim(x) , dim3d ) )
stopifnot( identical( as.vector(x), as.character(0:29) ) )
          

# testing the indexing

xj <- .jarray( x )
stopifnot( dim( xj[ ,, ] ) == c( 5L, 3L, 2L ) )
stopifnot( dim( xj[ ] ) == c( 5L, 3L, 2L ) )
stopifnot( dim( xj[ ,,1,drop= TRUE] ) == c( 5L, 3L ) )
stopifnot( dim( xj[ ,,1,drop= FALSE] ) == c( 5L, 3L, 1L ) )
stopifnot( dim( xj[ ,1,,drop= TRUE] ) == c( 5L, 2L ) )
stopifnot( dim( xj[ ,1,,drop= FALSE] ) == c( 5L, 1L, 2L ) )
stopifnot( dim( xj[ 1,,,drop= TRUE] ) == c( 3L, 2L ) )
stopifnot( dim( xj[ 1,,,drop= FALSE] ) == c( 1L, 3L, 2L ) )
stopifnot( dim( xj[ ,1,1,drop= TRUE] ) == c( 5L ) )
stopifnot( dim( xj[ ,1,1,drop= FALSE] ) == c( 5L, 1L, 1L ) )
stopifnot( dim( xj[ 1,1,1,drop= TRUE] ) == c( 1L ) )
stopifnot( dim( xj[ 1,1,1,drop= FALSE] ) == c( 1L, 1L, 1L ) )

# testing simplify
stopifnot( identical( xj[simplify=TRUE], x) )
stopifnot( identical( xj[,1,,simplify=TRUE], x[,1,]) )
stopifnot( identical( xj[,1,-1,simplify=TRUE], x[,1,-1]) )
stopifnot( identical( xj[4,1,c(TRUE,FALSE),simplify=TRUE], x[4,1,c(TRUE,FALSE)]) )
stopifnot( identical( xj[1:10,simplify=TRUE], x[1:10]) )

# test dim<-
dim( xj ) <- c( 15, 2 )
stopifnot( xj@jsig == "[[Ljava/lang/String;" )
stopifnot( dim( xj ) == c(15L, 2L ) )

}
}
\keyword{classes}
