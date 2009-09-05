\name{.jinstanceof}
\Rdversion{1.1}
\alias{\%instanceof\%}
\alias{.jinstanceof}
\title{
Is a java object an instance of a given java class
}
\description{
Is a java object an instance of a given java class
}
\usage{
o \%instanceof\% cl
.jinstanceof( o, cl )
}
\arguments{
  \item{o}{java object reference}
  \item{cl}{java class. This can be a character vector of length one
  giving the name of the class, or another java object, or an instance
  of the Class class.}
}
\value{
TRUE if o is an instance of cl
}
\author{
Romain Francois <francoisromain@free.fr>
}
\examples{
	\dontrun{
		label <- .jnew( "javax/swing/JLabel" )
		
		# character
		label \%instanceof\% "javax.swing.JLabel"
		label \%instanceof\% "javax/swing/JComponent"
		
		# instance of Class
		jlabel.class <- .jfindClass( "javax/swing/JLabel" )
		label \%instanceof\% jlabel.class
		
		# other object
		other.label <- .jnew( "javax/swing/JLabel" )
		label \%instanceof\% other.label
	}
}
\keyword{ manip }

