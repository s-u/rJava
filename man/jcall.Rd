\name{jcall}
\alias{.jcall}
\title{
  Call a Java method
}
\description{
  \code{.jcall} calls a Java method with the supplied arguments.
}
\usage{
.jcall(obj, returnSig = "V", method, ..., evalArray = TRUE, 
    evalString = TRUE, check = TRUE, interface = "RcallMethod")
}
\arguments{
  \item{obj}{Java object (\code{jobjRef} as returned by
    \code{\link{.jcall}} or \code{\link{.jnew}}) or fully qualified
    class name in JNI notation (e.g. \code{"java/lang/String"}).}
  \item{returnSig}{Return signature in JNI notation (e.g. "V" for void,
    "[I" for \code{int[]} etc.). For convenience additional type
    \code{"S"} is supported and expanded to
    \code{"Ljava/lang/String;"}, re-mapping \code{"T"} to represent the
    type \code{short}.}
  \item{method}{The name of the method to be called}
  \item{...}{
    Any parametes that will be passed to the Java method. The parameter
    types are determined automatically and/or taken from the
    \code{jobjRef} object. All named parameters are discarded.}
  \item{evalArray}{This flag determines whether the array return value
    is evaluated (\code{TRUE}) or passed back as Java object reference
    (\code{FALSE}).}
  \item{evalString}{This flag determines whether string result is returned
    as characters or as Java object reference.}
  \item{check}{If set to \code{TRUE} then checks for exceptions are
    performed before and after the call using
    \code{\link{.jcheck}(silent=FALSE)}. This is usually the desired
    behavior, because all calls fail until an expection is cleared.}
  \item{interface}{This option is experimental and specifies the
    interface used for calling the Java method; the current
    implementation supports two interfaces:
    \itemize{
      \item{\code{"RcallMethod"}}{the default interface.}
      \item{\code{"RcallSyncMethod"}}{synchronized call of a
	method. This has simmilar effect as using \code{synchronize} in
	Java.}
    }
  }
}
\value{
  Returns the result of the method.
}
\details{
  \code{.jcall} requires exact match of argument and return types. For
  higher efficiency \code{.jcall} doens't perform any lookup in the
  reflection tables. This means that passing subclasses of the classes
  present in the method definition requires explicit casting using
  \code{\link{.jcast}}. Passing \code{null} arguments also needs a
  proper class specification with \code{\link{.jnull}}.

  Java types \code{long} and \code{float} have no corresponding types in
  R and therefore any such parameters must be flagged as such using
  \code{\link{.jfloat}} and \code{\link{.jlong}} functions respectively.

  Java also distinguishes scalar and array types whereas R doesn't have
  the concept of a scalar. In R a scalar is basically a vector (called
  array in Java-speak) of the length 1. Therefore passing vectors of the
  length 1 is ambiguous. \code{.jcall} assumes that any vector of the
  length 1 that corresponds to a native Java type is a scalar. All other
  vectors are passed as arrays. Therefore it is important to use
  \code{\link{.jarray}} if an arbitrary vector (including those of the
  length 1) is to be passed as an array parameter.
}
\seealso{
  \code{\link{.jnew}}, \code{\link{.jcast}}, \code{\link{.jnull}},
  \code{\link{.jarray}}
}
\examples{
\dontrun{
.jcall("java/lang/System","S","getProperty","os.name")
f <- .jnew("java/awt/Frame","Hello")
.jcall(f,,"setVisible",TRUE)
}
}
\keyword{interface}
