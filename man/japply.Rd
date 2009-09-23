\name{japply}
\alias{japply}
\title{
Apply a function to a sequence of java objects
}
\description{
Apply a function to a sequence of java objects. in this case, "sequence"
means either a java array or a java object that implements the 
\code{Iterable} interface (such as \code{Vector})
}
\usage{
japply(X, FUN = if( simplify ) force else "toString" , simplify = FALSE, ...)
}
\arguments{
  \item{X}{
Either a regular R list, a reference to a java array, or 
a reference to a java object that implements the 
\code{Iterable} interface
}
  \item{FUN}{
An R function or a character vector of length one. See details below.
}
\item{simplify}{
logical. If \code{TRUE}, each element is simplified before the function 
is applied to it. For example a reference to a java String becomes
a character vector, ...
}
  \item{\dots}{
Additional arguments for the function that is applied
}
}
\details{
When \code{FUN} is a character vector, it is first checked that the 
current object has a method of this name, if this is not the case, then
\code{\link{match.fun}} is used
}
\value{
A list with each element being the result of the evaluation of the 
function on one element of the iteration
}
\author{
Romain Francois <francoisromain@free.fr>
}
\seealso{
	\code{\link{lapply}}
}
\examples{
\dontshow{.jinit()}
# works like lapply on usual r lists
li <- list( x = 1:10, y = 1:10 )
japply( li, mean )

### java vector (implements Iterable)
vec <- new( J("java.util.Vector" ) )
for( i in letters ){ 
	vec$add( i )
}
# apply the R function casefold to each element of the vector
# it works because the elements are simplified to R objects
japply( vec, casefold, upper = TRUE, simplify = TRUE )
japply( vec, "casefold", upper = TRUE, simplify = TRUE )

# no simplification, using a method of the String objects
japply( vec, "hashCode" )
japply( vec, "toString" )
japply( vec, function( item ){
	item$toString()
})

### java arrays
arr <- vec$toArray()
japply( vec, "toString" )

}
\keyword{ manip }

