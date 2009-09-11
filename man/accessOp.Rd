\name{JavaAccess}
\alias{$,jobjRef-method}
\alias{$,jclassName-method}
\alias{$<-,jobjRef-method}
\alias{$<-,jclassName-method}
\alias{names,jobjRef-method}
\alias{names,jclassName-method}
\title{
  Field/method operator for Java objects
}
\description{
  The \code{$} operator for \code{jobjRef} Java object references provides convenience access to object attributes and calling Java methods.
}
\section{Methods}{
  \describe{
    \item{\code{$}}{\code{signature(x = "jobjRef")}: ... }
    \item{\code{$}}{\code{signature(x = "jclassName")}: ... }
    \item{\code{$<-}}{\code{signature(x = "jobjRef")}: ... }
    \item{\code{$<-}}{\code{signature(x = "jclassName")}: ... }
    \item{\code{names}}{\code{signature(x = "jobjRef")}: ... }
    \item{\code{names}}{\code{signature(x = "jclassName")}: ... }
	 }
}
\details{
  rJava provies two levels of API: low-level JNI-API in the form of \code{\link{.jcall}} function and high-level reflection API based on the \code{$} operator. The former is very fast, but inflexible. The latter is a convenient way to use Java-like programming at the cost of performance. The reflection API is build around the \code{$} operator on \code{\link{jobjRef-class}} objects that allows to access Java attributes and call object methods.

 \code{$} returns either the value of the attribute or calls a method, depending on which name matches first.

 \code{$<-} assigns a value to the corresponding Java attribute.

 \code{names} returns all fields and methods associated with the object. Method names are followed by \code{(} or \code{()} depending on arity. This use of names is mainly useful for code completion, it is not intended to be used programmatically.

 This is just a convenience API. Internally all calls are mapped into \code{\link{.jcall}} calls, therefore the calling conventions and returning objects use the same rules. For time-critical Java calls \code{\link{.jcall}} should be used directly.
}
\seealso{
  \code{\link{.jcall}}, \code{\link{.jnew}}, \code{\link{jobjRef-class}}
}
\examples{
\dontrun{
v <- new(J("java.lang.String"), "Hello World!")
v$length()
v$indexOf("World")
names(v)

J("java.lang.String")$valueOf(10)
}
}
\keyword{interface}
