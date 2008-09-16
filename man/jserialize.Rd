\name{jserialize}
\alias{.jserialize}
\alias{.junserialize}
\title{
  Java object serialization
}
\description{
  \code{.jserialize} serializes a Java object into raw vector using
  Java serialization.

  \code{.junserialize} re-constructs a Java object from its serialized
  (raw-vector) form.
}
\usage{
.jserialize(o)
.junserialize(data)
}
\arguments{
  \item{o}{Java object}
  \item{data}{serialized Java object as a raw vector}
}
\value{
  \code{.jserialize} returns a raw vector

  \code{.junserialize} return a Java object or \code{NULL} if an error
  occurred (currently you may use \code{.jcheck()} to further
  investigate the error)
}
\details{
  Not all Java objects support serialization, see Java documentation
  for details. Note that Java serialization and serialization of R
  objects are two entirely different mechanisms that cannot be
  interchanged.
}
%\seealso{
%}
%\examples{
%\dontrun{
%}
%}
\keyword{interface}
