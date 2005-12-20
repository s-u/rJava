\name{print.jobjRef}
\alias{print.jobjRef}
\alias{print.jarrayRef}
\title{
  Print a Java object
}
\description{
  Prints a Java object by calling its \code{toString()} method
}
\usage{
print.jobjRef(x, ...)
print.jarrayRef(x, ...)
}
\arguments{
  \item{x}{Java object reference to print}
  \item{...}{Additional parameters passed to chained \code{print}}
}
\value{
  Returns the object \code{x} invisibly.
}
\seealso{
  \code{\link{print}}
}
\keyword{interface}
