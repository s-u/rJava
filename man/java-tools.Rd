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
}
}
