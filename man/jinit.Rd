\name{jinit}
\alias{.jinit}
\title{
  Initialize Java VM
}
\description{
  \code{.jinit} initializes the Java Virtual Machine (JVM). This
  function must be called before any rJava functions can be used.
}
\usage{
.jinit(classpath = NULL, parameters = NULL, ..., silent = FALSE)
}
\arguments{
  \item{classpath}{Any additional classes to include in the Java class
    paths (i.e. locations of Java classes to use). This path will be
    prepended to paths specified in the \code{CLASSPATH} environment
    variable.}
  \item{parameters}{character vector of parameters to be passed to
    the virtual machine. They are implementation dependent and apply
    to JDK version 1.2 or higher only. Please note that each parameter
    must be in a separate element of the array, you cannot use a
    space-separated string with multiple parameters.}
  \item{...}{Other optional Java initialization parameters (implementation-dependent).}
  \item{silent}{If set to \code{TRUE} no warnings are issued.}
}
\value{
  The return value is an integer specifying whether and how the VM was
  initialized. Negative values indicate failure, zero denotes successful
  initialization and positive values signify partially successful
  initilization (i.e. the VM is up, but parameters or class path could
  not be set due to an existing or incompatible VM).
}
\details{
  Stating with version 0.3-8 rJava is now capable of modifying the class
  path on the fly for certain Sun-based Java virtual machines, even when
  attaching to an existing VM. However, this is done by exploiting the
  way ClassLoader is implemented and may fail in the future. In general
  it is officially not possible to change the class path of a running
  VM.
  
  At any rate, it is impossible to change any other VM parameters of a
  running VM, so when using \code{.jinit} in a package, be generous with
  limits and don't use VM parameters to unnecessarily restrict
  resources.
}
\seealso{
}
\examples{
\dontrun{
## set heap size limit to 512MB (see java -X) and
## use "myClasses.jar" as the class path
.jinit(classpath="myClasses.jar", parameters="-Xmx512m")
}
}
\keyword{interface}
