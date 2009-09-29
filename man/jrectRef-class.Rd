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
    \item{\code{dimension}:}{dimension vector of the array}
  }
}
\section{Extends}{
Class \code{"\linkS4class{jarrayRef}"}, directly.
Class \code{"\linkS4class{jobjRef}"}, by class "jarrayRef", distance 2.
}
\section{Methods}{
No methods defined with class "jrectRef" in the signature.
}
\examples{
\dontshow{
# these examples are only unit tests so far
.jinit()

# 2d
dim2d <- c(5L, 2L)

x <- .jcall( "RJavaArrayTools", "[[Z", "getBooleanDoubleRectangularArrayExample",  evalArray = FALSE )
stopifnot( identical( typeof( x ), "logical" ) )
stopifnot( identical( dim(x) , dim2d ) )
stopifnot( identical( as.vector(x), rep( c(FALSE,TRUE), 5 ) ) )

x <- .jcall( "RJavaArrayTools", "[[I", "getIntDoubleRectangularArrayExample",  evalArray = FALSE )
stopifnot( identical( typeof( x ), "integer" ) )
stopifnot( identical( dim(x) , dim2d ) )
stopifnot( identical( as.vector(x), 0:9 ) )
 
x <- .jcall( "RJavaArrayTools", "[[B", "getByteDoubleRectangularArrayExample",  evalArray = FALSE )
stopifnot( identical( typeof( x ), "raw" ) )
stopifnot( identical( dim(x) , dim2d ) )
stopifnot( identical( as.vector(x), as.raw(0:9) ) )

x <- .jcall( "RJavaArrayTools", "[[J", "getLongDoubleRectangularArrayExample",  evalArray = FALSE )
stopifnot( identical( typeof( x ), "double" ) )
stopifnot( identical( dim(x) , dim2d ) )
stopifnot( identical( as.vector(x), as.numeric(0:9) ) )

x <- .jcall( "RJavaArrayTools", "[[S", "getShortDoubleRectangularArrayExample",  evalArray = FALSE )
stopifnot( identical( typeof( x ), "integer" ) )
stopifnot( identical( dim(x) , dim2d ) )
stopifnot( identical( as.vector(x), 0:9 ) )

x <- .jcall( "RJavaArrayTools", "[[D", "getDoubleDoubleRectangularArrayExample",  evalArray = FALSE )
stopifnot( identical( typeof( x ), "double" ) )
stopifnot( identical( dim(x) , dim2d ) )
stopifnot( identical( as.vector(x), as.numeric(0:9) ) )

x <- .jcall( "RJavaArrayTools", "[[C", "getCharDoubleRectangularArrayExample",  evalArray = FALSE )
stopifnot( identical( typeof( x ), "integer" ) )
stopifnot( identical( dim(x) , dim2d ) )
stopifnot( identical( as.vector(x), 0:9 ) )

x <- .jcall( "RJavaArrayTools", "[[F", "getFloatDoubleRectangularArrayExample",  evalArray = FALSE )
stopifnot( identical( typeof( x ), "double" ) )
stopifnot( identical( dim(x) , dim2d ) )
stopifnot( identical( as.vector(x), as.numeric(0:9) ) )

x <- .jcall( "RJavaArrayTools", "[[Ljava/lang/String;", "getStringDoubleRectangularArrayExample",  evalArray = FALSE )
stopifnot( identical( typeof( x ), "character" ) )
stopifnot( identical( dim(x) , dim2d ) )
stopifnot( identical( as.vector(x), as.character(0:9) ) )


# 3d

dim3d <- c(5L, 3L, 2L) 

x <- .jcall( "RJavaArrayTools", "[[[Z", "getBooleanTripleRectangularArrayExample",  evalArray = FALSE )
stopifnot( identical( typeof( x ), "logical" ) )
stopifnot( identical( dim(x) , dim3d ) )
stopifnot( identical( as.vector(x), rep( c(FALSE,TRUE), 15L ) ) )

x <- .jcall( "RJavaArrayTools", "[[[I", "getIntTripleRectangularArrayExample",  evalArray = FALSE )
stopifnot( identical( typeof( x ), "integer" ) )
stopifnot( identical( dim(x) , dim3d ) )
stopifnot( identical( as.vector(x), 0:29 ) )
 
x <- .jcall( "RJavaArrayTools", "[[[B", "getByteTripleRectangularArrayExample",  evalArray = FALSE )
stopifnot( identical( typeof( x ), "raw" ) )
stopifnot( identical( dim(x) , dim3d ) )
stopifnot( identical( as.vector(x), as.raw(0:29) ) )

x <- .jcall( "RJavaArrayTools", "[[[J", "getLongTripleRectangularArrayExample",  evalArray = FALSE )
stopifnot( identical( typeof( x ), "double" ) )
stopifnot( identical( dim(x) , dim3d ) )
stopifnot( identical( as.vector(x), as.numeric(0:29) ) )

x <- .jcall( "RJavaArrayTools", "[[[S", "getShortTripleRectangularArrayExample",  evalArray = FALSE )
stopifnot( identical( typeof( x ), "integer" ) )
stopifnot( identical( dim(x) , dim3d ) )
stopifnot( identical( as.vector(x), 0:29 ) )

x <- .jcall( "RJavaArrayTools", "[[[D", "getDoubleTripleRectangularArrayExample",  evalArray = FALSE )
stopifnot( identical( typeof( x ), "double" ) )
stopifnot( identical( dim(x) , dim3d ) )
stopifnot( identical( as.vector(x), as.numeric(0:29) ) )

x <- .jcall( "RJavaArrayTools", "[[[C", "getCharTripleRectangularArrayExample",  evalArray = FALSE )
stopifnot( identical( typeof( x ), "integer" ) )
stopifnot( identical( dim(x) , dim3d ) )
stopifnot( identical( as.vector(x), 0:29 ) )

x <- .jcall( "RJavaArrayTools", "[[[F", "getFloatTripleRectangularArrayExample",  evalArray = FALSE )
stopifnot( identical( typeof( x ), "double" ) )
stopifnot( identical( dim(x) , dim3d ) )
stopifnot( identical( as.vector(x), as.numeric(0:29) ) )

x <- .jcall( "RJavaArrayTools", "[[[Ljava/lang/String;", "getStringTripleRectangularArrayExample",  evalArray = FALSE )
stopifnot( identical( typeof( x ), "character" ) )
stopifnot( identical( dim(x) , dim3d ) )
stopifnot( identical( as.vector(x), as.character(0:29) ) )


}
}
\keyword{classes}
