\name{jinit}
\alias{.jinit}
\alias{.jvmState}
\title{
  Initialize Java VM
}
\description{
  \code{.jinit} initializes the Java Virtual Machine (JVM). This
  function must be called before any rJava functions can be used.

  \code{.jvmState() returns the state of the current JVM.}
}
\usage{
.jinit(classpath = NULL, parameters = getOption("java.parameters", NA), ...,
       silent = FALSE, force.init = FALSE)
.jvmState()
}
\arguments{
  \item{classpath}{Any additional classes to include in the Java class
    paths (i.e. locations of Java classes to use). This path will be
    prepended to paths specified in the \code{CLASSPATH} environment
    variable. Do NOT set this system class path initializing a package,
    use \code{\link{.jpackage}} instead, see details.}
  \item{parameters}{character vector of parameters to be passed to
    the virtual machine. They are implementation dependent and apply
    to JDK version 1.2 or higher only. Please note that each parameter
    must be in a separate element of the array, you cannot use a
    space-separated string with multiple parameters. The value of
    \code{NA} means "use rJava default" which is \code{"-Xmx1g"}
    currently. \code{NULL} is equivalent to \code{character()}.
  }
  \item{...}{Other optional Java initialization parameters (implementation-dependent).}
  \item{silent}{If set to \code{TRUE} no warnings are issued.}
  \item{force.init}{If set to \code{TRUE} JVM is re-initialized even if
    it is already running.}
}
\value{
  The return value is an integer specifying whether and how the VM was
  initialized. Negative values indicate failure, zero denotes successful
  initialization and positive values signify partially successful
  initilization (i.e. the VM is up, but parameters or class path could
  not be set due to an existing or incompatible VM).

  \code{.jvmState} returns a named list with at least the following
  elements:
  \item{initialized}{\code{TRUE} if rJava is initialized and has a
    runing JVM, \code{FALSE} otherwise.}
  \item{state}{string representing the current state of the JVM. One of
    the following values:
    \code{"none"} if there is no JVM, \code{"created"} if the current
    JVM has been created by rJava, \code{"attached"} if rJava attached
    into an existing JVM (typically when R is embedded into a running
    JVM via JRI), \code{"detached"} if there is a JVM (such as embedded
    R), but rJava has not been initialized to use it, \code{"dead"} if
    the process is about to die due to the JVM forcing en exit or
    \code{"destroyed"} if a JVM existed before, but was destroyed.}
}
\details{
  Starting with version 0.5 rJava provides a custom class loader that can
  automatically track classes and native libraries that are provided in
  R packages. Therefore R packages should NOT use \code{.jinit}, but
  call \code{\link{.jpackage}} instead. In addition this allows the use
  of class path modifying function \code{\link{.jaddClassPath}}.

  Important note: if a class is found on the system class path (i.e. on
  the \code{classpath} specified to \code{.jinit}) then the system class
  loader is used instead of the rJava loader, which can lead to problems
  with reflection and native library support is not enabled. Therefore
  it is highly recommended to use \code{.jpackage} or
  \code{.jaddClassPath} instead of \code{classpath} (save for system
  classes).
  
  Stating with version 0.3-8 rJava is now capable of modifying the class
  path on the fly for certain Sun-based Java virtual machines, even when
  attaching to an existing VM. However, this is done by exploiting the
  way ClassLoader is implemented and may fail in the future. In general
  it is officially not possible to change the class path of a running
  VM.
  
  At any rate, it is impossible to change any other VM parameters of a
  running VM, so when using \code{.jinit} in a package, be generous with
  limits and don't use VM parameters to unnecessarily restrict
  resources (or preferably use \code{\link{.jpackage}} instead). JVM
  parameters can only be set if the initial state of the JVM is
  \code{"none"}.

  There is a subtle difference between \code{"initialized"} and the JVM
  state. It is in theory possible for \code{"initialized"} to be
  \code{FALSE} and still \code{"state"} to be \code{"created"} or
  \code{"attached"} in case where JVM was created but rJava has not been
  able to initialize for other reasons, although such state should be
  rare and problematic in either case. Behavior of rJava functions other
  than \code{.jinit} and \code{.jvmState} is undefined unless
  \code{.jvmState()$initialized} is \code{TRUE}.

  NOTE: \code{.jinit()} can be called implicitly by other functions such
  as \code{\link{.jpackage}} or \code{\link{J}}.
}
\seealso{
  \code{\link{.jpackage}}
}
\examples{
\dontrun{
## set heap size limit to 512MB (see java -X)
.jinit(parameters="-Xmx512m")
.jvmState()
}
}
\keyword{interface}
