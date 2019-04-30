\name{.jgc}
\alias{.jgc}
\title{
   Invoke Java Garbage Collection
}
\description{
  \code{.jgc} invokes the R and Java garbage collectors.
}
\usage{
.jgc(R.gc = TRUE, ...)
}
\arguments{
  \item{R.gc}{logical, if \code{TRUE} then \code{gc(\dots)} is called
  first, if \code{FALSE} only Java garbage collector is called}
  \item{\dots}{any additional parameters passed to \code{gc()}}
}
\details{
  \code{.jgc} invokes the R garbage collector (unless
  \code{R.gc=FALSE}) which removes any unused Java references and then
  invokes the Java garbage collector to reclaim Java heap space.
}
\author{
Simon Urbanek
}
\keyword{interface}
