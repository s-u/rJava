\name{with.jobjRef}
\alias{with.jobjRef}
\alias{within.jobjRef}
\title{
with and within methods for Java objects
}
\description{
Convenience wrapper that allow calling methods of 
Java object from within the object.
}
\usage{
\S3method{with}{jobjRef}(data, expr, ...)
\S3method{within}{jobjRef}(data, expr, ...)
}
\arguments{
 \item{data}{
A Java object reference. See \link{jobjRef-class}
}
 \item{expr}{
expression to evaluate
}
 \item{\dots}{
ignored
}
}
\details{
The expression is evaluated in an environment 
that contains a mapping between the public fields 
and methods of the object. 

The methods of the object are mapped to standard R functions
in the environment. 

The fields of the object are mapped to active bindings
(see \link{makeActiveBinding}) so that they can be accessed
and modified from within the environment.
}
\value{
	\code{with} returns the value of the expression and 
	\code{within} returns the \code{data} argument
}
\author{
Romain Francois <francoisromain@free.fr>
}
\references{
	the \code{java.lang.reflect} package: 
	\url{http://java.sun.com/j2se/1.5.0/docs/api/java/lang/reflect/package-summary.html}
}
\examples{
\dontrun{

p <- .jnew( "java/awt/Point", 0L, 0L )
with( p, {
	# x and y and now 0
	move( 10L, 10L )
	# x and y are now 10
	x <- x + y
} )

f <- within( .jnew( "javax/swing/JFrame" ) , {
	layout <- .jnew( "java/awt/BorderLayout" )
	setLayout( layout )
	add( .jnew( "javax/swing/JLabel", "north" ), layout$NORTH )
	add( .jnew( "javax/swing/JLabel", "south" ), layout$SOUTH )
	add( .jnew( "javax/swing/JLabel", "west" ), layout$WEST )
	add( .jnew( "javax/swing/JLabel", "east" ), layout$EAST )
	setSize( .jnew( "java/awt/Dimension", 400L, 400L ) )
	setVisible( TRUE )
} )
}

}
\keyword{ classes }
