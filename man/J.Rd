\name{.J}
\alias{.J}
\title{
High level constructor for java objects
}
\description{
\code{.J} creates a new Java object
}
\usage{
.J(class, ...)
}
\arguments{
  \item{class}{
fully qualified class name in JNI notation (e.g "java/lang/String" ) or standard java notation (e.g "java.lang.String")
 }
  \item{\dots}{
Any parameters that will be passed to the corresponding constructor. 
}
}
\details{
The \code{RJavaTools} class is used to find the constructor that matches
the types of the supplied parameters
}
\value{
Returns the reference to the newly created object. If a constructor 
could not be found, an error is generated.
}
\author{
Romain Francois <francoisromain@free.fr> 
}
\note{
\code{.J} is more flexible than .jnew but uses reflection so is expected
to be slower
}
\seealso{
\code{\link{.jnew}} for the standard, more efficient constructor
}
\examples{
\dontrun{

f <- .J( "java.awt.Frame", "Hello" ) 
f$setVisible( TRUE )

# String[] strings = new String[]{ "string", "array" } ;
strings <- .jarray("string", "array" )
# this uses the JList( Object[] ) constructor 
# even though the "strings" parameter is a String[] 
l <- .J( "javax.swing.JList", strings )

}
}
\keyword{interface}

