\name{jnull}
\alias{.jnull}
\title{
  Creates a null object reference
}
\description{
  \code{.jnull} returns a \code{null} reference of a specified class
  type.
}
\usage{
.jnull(class = "java/lang/Object")
}
\arguments{
  \item{class}{fully qualified target class name in JNI notation
    (e.g. \code{"java/lang/String"}).}
}
\value{
  Returns a Java object reference (\code{jobjRef}) of a \code{null}
  object having the specified object class.
}
\details{
  This function is necesary if \code{null} is to be passed as an
  argument of \code{\link{.jcall}} or \code{\link{.jnew}}, in order to be
  able to find the correct method/constructor.

  Example: given the following method definitions of the class \code{A}:
  \itemize{
    \item{o}{public static void run(String a);}
    \item{o}{public static void run(Double n);}
  }
  Calling \code{.jcall("A",,"run",NULL)} is ambiguous, because it is
  unclear which method is to be used. Therefore rJava requires class
  information with each argument to \code{\link{.jcall}}. If we wanted
  to run the String-version, we could use \code{.jcall("A",,"run",.jnull("java/lang/String"))}.
}
\seealso{
  \code{\link{.jcall}}, \code{\link{.jcast}}
}
\examples{
\dontrun{
.jcall("java/lang/System","I","identityHashCode",.jnull())
}
}
\keyword{interface}
