\name{jinit}
\alias{.jinit}
\title{
  Initialize Java VM
}
\description{
  \code{.jinit} initializes the Java Virtual Machine (JVM). This
  function must be called before any rJava functions can be used.
}
\usage{
.jinit(classpath = NULL, ..., silent = FALSE)
}
\arguments{
  \item{classpath}{Any additional classes to include in the Java class
    paths (i.e. locations of Java classes to use). This path will be
    prepended to paths specified in the \code{CLASSPATH} environment
    variable.}
  \item{...}{Optional Java initialization parameters (implementation-dependent).}
  \item{silent}{If set to \code{TRUE} no warnings are issued.}
}
\value{
  Currently the return value is implementation dependent, mostly just
  the return code as returned by the low-level JVM function.
}
\seealso{
}
\examples{
\dontrun{
.jinit(classpath="myClasses.jar")
}
}
\keyword{interface}
