\name{jarray}
\alias{.jarray}
\alias{.jevalArray}
\title{
  Java array handling functions
}
\description{
  \code{.jarray} takes a vector (or a list of Java references) as its
  argument, creates a Java array containing the elements of the vector
  (or list) and returns a reference to such newly created array.

  \code{.jevalArray} takes a reference to a Java array and returns its
  contents (if possible).
}
\usage{
.jarray(x, contents.class = NULL)
.jevalArray(obj, rawJNIRefSignature = NULL, silent = FALSE)
}
\arguments{
  \item{x}{vector or a list of Java references}
  \item{contents.class}{common class of the contained objects, see
    details}
  \item{obj}{Java object reference to an array that is to be evaluated}
  \item{rawJNIRefSignature}{JNI signature that whould be used for
    conversion. If set to \code{NULL}, the signature is detected
    automatically.}
  \item{silent}{if set to true, warnings are suppressed}
}
\value{
  \code{.jarray} returns a Java array reference (\code{jarrayRef}) to an
  array created with the supplied contents.

  \code{.jevalArray} returns the contents of the array object.
}
\details{
  \code{.jarray}: The input can be either a vector of some sort (such as
  numeric, integer, logical, ...) or a list of Java references. The
  contents is pushed to the Java side and a corresponding array is
  created. The type of the array depends on the input vector type. For
  example numeric vector creates \code{double[]} array, integer vector
  creates \code{int[]} array, character vector \code{String[]} array and
  so on. If \code{x} is a list, it must contain Java references only (or
  \code{NULL}s which will be treaded as \code{NULL} references).

  The \code{contents.class} parameter is used only if \code{x} is a list
  of Java object references and it can specify the class that will be
  used for all objects in the array. If set to \code{NULL} no assumption
  is made and \code{java/lang/Object} will be used. Use with care and
  only if you know what you're doing - you can always use
  \code{\link{.jcast}} to cast the entire array to another type even if
  you use a more general object type.

  The result is a reference to the newly created array.

  The inverse function which fetches the elements of an array reference
  is \code{.jevalArray}.

  \code{.jevalArray} currently supports only a subset of all possible
  array types. Recursive arrays are handled by returning a list of
  references which can then be evaluated separately.
}
\examples{
\dontrun{
a <- .jarray(1:10)
print(a)
.jevalArray(a)
b <- .jarray(c("hello","world"))
print(b)
c <- .jarray(list(a,b))
print(c)
}
}
\keyword{interface}
