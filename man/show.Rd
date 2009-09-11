\name{show}
\alias{show,jobjRef-method}
\alias{show,jarrayRef-method}
\alias{show,jclassName-method}
\title{Show a Java Object Reference}
\description{
  Display a Java object reference in a descriptive, textual form. The
  default implementation calls \code{toString} Java method to obtain
  object's printable value and uses calls \code{show} on the resulting
  string garnished with additional details.
}
\section{Methods}{
  \describe{
    \item{show}{\code{signature(object = "jobjRef")}: ... }
    \item{show}{\code{signature(object = "jarrayRef")}: ... }
    \item{show}{\code{signature(object = "jclassName")}: ... }
  }
}
\keyword{interface}
