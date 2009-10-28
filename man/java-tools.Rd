\name{java-tools}
\alias{java-tools}
\title{java tools used internally in rJava}
\description{java tools used internally in rJava}
\examples{
\dontshow{
# running the java unit tests from the R examples
.jinit()
J("RJavaTools_Test")$runtests()
J("RJavaArrayTools_Test")$runtests()
J("ArrayWrapper_Test")$runtests()
J("RectangularArrayBuilder_Test")$runtests()

# now testing some jni functions
p <- .jnew( "java/lang/Double", 0.0 )
classes <- .Call( "RgetSimpleClassNames", p@jobj, TRUE, PACKAGE = "rJava" )
stopifnot( all( c( "Double", "Number", "Object", "Exception", "error", "condition" ) \%in\% classes ) )
classes <- .Call( "RgetSimpleClassNames", p@jobj, FALSE, PACKAGE = "rJava" )
stopifnot( all( c( "Double", "Number", "Object" )  \%in\% classes ) )
}
}
