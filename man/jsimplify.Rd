\name{jsimplify}
\alias{.jsimplify}
\title{
  Converts Java object to a simple scalar if possible
}
\description{
  \code{.jsimplify} attempts to convert Java objects that represent
  simple scalars into corresponding scalar representation in R.
}
\usage{
.jsimplify(o)
}
\arguments{
  \item{o}{arbitrary object}
}
\value{
  Simple scalar or \code{o} unchanged.
}
\details{
  If \code{o} is not a Java object reference, \code{o} is returned
  as-is. If \code{o} is a reference to a scalar object (such as single
  integer, number, string or boolean) then the value of that object is
  returned as R vector of the corresponding type and length one.

  This function is used by \code{\link{.jfield}} to simplify the results
  of field access if required.

  Currently there is no function inverse to this, the usual way to wrap
  scalar values in Java references is to use \code{\link{.jnew}} as the
  corresponding constructor.
}
\seealso{
  \code{\link{.jfield}}
}
\examples{
\dontrun{
i <- .jnew("java/lang/Integer", as.integer(10))
print(i)
print(.jsimplify(i))
}
}
\keyword{interface}
