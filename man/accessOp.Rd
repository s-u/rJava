\name{JavaAccess}
\alias{$,jobjRef-method}
\alias{$<-,jobjRef-method}
\title{
  Field/method operator for Java objects
}
\description{
  The \code{$} operator for \code{jobjRef} Java object references provides convenience access to object attributes and calling Java methods.
}
\section{Methods}{
  \describe{
    \item{\code{$}}{\code{signature(x = "jobjRef")}: ... }
    \item{\code{$<-}}{\code{signature(x = "jobjRef")}: ... }
	 }
}
\details{
  rJava provies two levels of API: low-level JNI-API in the form of \code{\link{.jcall}} function and high-level reflection API based on the \code{$} operator. The former is very fast, but inflexible. The latter is a convenient way to use Java-like programming at the cost of performance. The reflection API is build around the \code{$} operator on \code{\link{jobjRef-class}} objects that allows to access Java attributes and call object methods.

 \code{$} returns either the value of the attribute or calls a method, depending on which name matches first.

 \code{$<-} assigns a value to the corresponding Java attribute.

 This is just a convenience API. Internally all calls are mapped into \code{\link{.jcall}} calls, therefore the calling conventions and returning objects use the smae rules. For time-critical Java calls \code{\link{.jcall}} should be used directly.

NOTE: This interface is still very experimental! Some type conversions may not work or it may fail to find the method you want. You may need to use \code{\link{.jcall}}, consult \code{\link{.jmethods}}, \code{\link{.jfields}} and possibly apply \code{\link{.jcast}} in such cases. If it is a general problem, please report such issues whith a small reproducible example.
}
\seealso{
  \code{\link{.jcall}}, \code{\link{.jnew}}, \code{\link{jobjRef-class}}
}
\examples{
\dontrun{
v <- .jnew("java/lang/String","Hello World!")
v$length()
v$indexOf("World")
}
}
\keyword{interface}
