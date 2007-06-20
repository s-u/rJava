\name{jfield}
\alias{.jfield}
\title{
  Obtains the value of a field 
}
\description{
  \code{.jfield} returns the value of the specified field on an object.
}
\usage{
.jfield(o, name, sig = NULL, true.class = is.null(sig), convert = TRUE)
}
\arguments{
  \item{o}{Class name or object (Java reference) whose field is to be
    accessed. Static fields are supported both by specifying the class
    name or using an instance.}
  \item{name}{name of the field to access}
  \item{sig}{signature (JNI type) of the field. If set to \code{NULL}
    rJava attempts to determine the signature using reflection. For
    efficiency it is recommended to specify the signature, because
    the reflection lookup is quite expensive.}
  \item{true.class}{by default the class of the resulting object matches
    the siganture of the field. Setting this flag to \code{TRUE} causes
    \code{.jfield} to use true class name of the resulting object
    instead. (this flag has no effect on scalar fields)}
  \item{convert}{when set to \code{TRUE} all references are converted to
    native types (where possible). Otherwise Java references are
    returned directly.}
}
\value{
  Contents of the field.
}
%\details{
%}
\seealso{
  \code{\link{.jnew}}, \code{\link{.jsimplify}}
}
\examples{
\dontrun{
.jfield("java/lang/Boolean", "TYPE")
}
}
\keyword{interface}
