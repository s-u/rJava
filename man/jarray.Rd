\name{jarray}
\alias{.jarray}
\title{
  Creates a Java array from a vector
}
\description{
  \code{.jarray} takes a vector (or a list of Java references) as its
  argument, creates a Java array containing the elements of the vector
  (or list) and returns a reference to such newly created array.
}
\usage{
.jarray(x)
}
\arguments{
  \item{x}{vector or a list of Java references}
}
\value{
  Returns a Java object reference (\code{jobjRef}) of a \code{null}
  object having the specified object class.
}
\details{
  The input can be either a vector of some sort (such as numeric,
  integer, logical, ...) or a list of Java references. The contents is
  pushed to the Java side and a corresponding array is created. The type
  of the array depends on the input vector type. For example numeric
  vector creates \code{double[]} array, integer vector creates
  \code{int[]} array, character vector \code{String[]} array and so
  on. If \code{x} is a list, it must contain Java references only (or
  \code{NULL}s which will be treaded as \code{NULL} references).

  The result is a reference to the newly created array.

  The inverse function which fetches the elements of an array reference
  is \code{.jevalArray}.
}
\examples{
\dontrun{
a <- .jarray(1:10)
a
.jevalArray(a)
b <- .jarray(c("hello","world"))
b
c <- .jarray(list(a,b))
c
}
}
\keyword{interface}
