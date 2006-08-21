\name{jcheck}
\alias{.jcheck}
\title{
  Check JVM for pending expections and clear them
}
\description{
  \code{.jcheck} checks the Java VM for any pending exceptions and
  clears them.
}
\usage{
.jcheck(silent = FALSE)
}
\arguments{
  \item{silent}{If set to \code{FALSE} then Java is instructed to print
    the exception on \code{stderr}. Note that Windows Rgui doesn't show
    \code{stderr} so it will not appear there (the printing is done
    internally in the JVM, so there is no simple way to use R's I/O).}
}
\value{
  Returns \code{TRUE} if an exception occurred or \code{FALSE} otherwise.
}
\details{
  The exception handling is currently rather crude as there is no way to
  obtain programmatical access to thrown exceptions. In addition, some
  internal code clears exceptions (e.g. after some failed JNI calls), so
  it may occasionally not behave as expected. However, this applies only
  to code called as a side-effect (such as when converting an object), so
  exceptions thrown as a result of a directly invoked code are not
  affected.

  Nevertheless more elaborate exception handling system is planned
  for rJava 0.5.
}
\seealso{
  \code{\link{.jcall}}
}
\examples{
\dontrun{
# we try to create a bogus object and
# instruct .jnew to not clear the exception
v <- .jnew("foo/bar", check=FALSE)

# expect TRUE result here because the class doesn't exist
print(.jcheck(silent=TRUE))
# next invocation will be FALSE because the exception is now cleared
print(.jcheck(silent=TRUE))
}
}
\keyword{interface}
