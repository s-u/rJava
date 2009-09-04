\name{jnew}
\alias{.jnew}
\alias{.jrnew}
\title{
  Create a Java object
}
\description{
	\code{.jnew} create a new Java object. \code{.jrnew} creates a
	new Java object using a reflection mechanism to find the best constructor.
}
\usage{
.jnew(class, ..., check=TRUE, silent=!check, use.reflection=TRUE)
.jrnew(class, ...)
}
\arguments{
  \item{class}{fully qualified class name in JNI notation (e.g. \code{"java/lang/String"}).}
  \item{...}{
    Any parametes that will be passed to the corresponding
    constructor. The parameter types are determined automatically and/or
    taken from the \code{jobjRef} object. For details see
    \code{\link{.jcall}}. Note that all named parameters are discarded.}
  \item{check}{
    If set to \code{TRUE} then \code{\link{.jcheck}} is invoked before
    and after the call to the constructor to clear any pending Java
    exceptions.}
  \item{silent}{
    If set to \code{FALSE} then \code{.jnew} will fail with an error if
    the object cannot be created, otherwise a null-reference is returned
    instead. In addition, this flag is also passed to final
    \code{.jcheck} if \code{check} above is set to \code{TRUE}. Note
    that the error handling also clears exceptions, so
    \code{check=FALSE, silent=FALSE} is usually not a meaningful
    combination.
  }
  \item{use.reflection}{
  	  logical. If \code{TRUE}, the \code{.jrnew} construction function will be used as 
  a fallback if the direct constructor fails
  }
}
\note{
	The user is not expected to call \code{.jrnew} directly since it is 
	used by default as a fallback in \code{.jnew}
}
\value{
  Returns the reference (\code{jobjRef}) to the newly created object or
  \code{null}-reference (see \code{\link{.jnull}}) if something went wrong.
}
\seealso{
  \code{\link{.jcall}}, \code{\link{.jnull}}
}
\examples{
\dontrun{
f <- .jnew("java/awt/Frame","Hello")
.jcall(f,,"setVisible",TRUE)

# this falls back on .jrnew because sa is a String[] rather than an Object[]
sa <- .jarray("string", "array" )
l <- .jnew( "javax/swing/JList", sa )
l <- .jrnew( "javax/swing/JList", sa )
# this fails: l <- .jnew( "javax/swing/JList", sa, use.reflection = FALSE )
}
}
\keyword{interface}
