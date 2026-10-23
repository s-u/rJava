\name{jinit}
\alias{.jinit}
\alias{.jvmState}
\alias{.jloadJVM}
\alias{.junloadJVM}
\alias{.jfindAndLoadJVM}
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
.jloadJVM(path, silent = FALSE)
.junloadJVM()
.jfindAndLoadJVM()
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
  \item{path}{Path to a JVM run-time library for explicit selection.}
  \item{silent}{If set to \code{TRUE} no initialization warnings are issued.
    For \code{.jloadJVM}, return the native status code instead of signaling
    a load error: zero means success and negative values mean failure.}
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
    \code{"not-loaded"} if no JVM run-time library has been selected yet,
    \code{"none"} if a run-time library is loaded, but there is no JVM, \code{"created"} if the current
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

  Loading the rJava namespace does not select or load a JVM run-time.
  The first call to \code{.jinit} selects one, in this order:
  the only JVM library already mapped in the process (e.g. by JavaGD),
  even if no VM exists yet; if a VM exists (e.g. when R is embedded in
  Java via JRI), rJava attaches regardless of other settings;
  otherwise the \code{java.home} option (Windows only) or the
  \code{JAVA_HOME} environment variable; then, on Windows, the
  \code{CurrentVersion} of the JavaSoft registry keys; on macOS,
  \code{/usr/libexec/java_home}; on Linux, the existing Java home reported
  by \code{R CMD config JAVA_HOME} (set by \code{R CMD javareconf});
  and finally the \code{java} executable
  found on \code{PATH}. A Java home setting that is not an existing
  directory is ignored with a warning. If no run-time can be found or
  loaded, \code{.jinit} fails and can be called again after correcting
  the setting. Once a run-time library is loaded, it remains selected
  for the life of the R process: a different JVM requires a new R
  process. On Linux with musl (e.g. Alpine), the JDK's own libraries
  find the JVM library by name only through \code{LD_LIBRARY_PATH} as
  set when R started, so only the run-time whose server directory was
  on it can be used: start R with \code{JAVA_HOME}, or
  \code{R_JAVA_LD_LIBRARY_PATH}, naming the run-time. Selecting another
  is refused before a JVM is created, but the refused library stays
  mapped, so retrying with any run-time then requires a new R process.
  If a JVM library is already mapped into the process, a JVM library
  path, such as one given to \code{.jloadJVM}, selects it only if it
  names that same file, also when a JVM was created in it. Automatic
  selection by \code{.jinit} adopts the mapped image without loading another
  JVM and, after successful selection, warns if Java discovery selects a
  different path. Loader lookups may inspect the recorded path when acquiring
  a reference to the mapped image.
  To select another JVM, start a new R process before loading packages
  that map a JVM library.

  \code{.jloadJVM} selects a library without starting a VM and returns
  \code{TRUE} on success, or a status code when \code{silent = TRUE}.
  \code{.jfindAndLoadJVM} performs the automatic selection used by
  \code{.jinit}. \code{.junloadJVM} returns \code{TRUE} if no library has
  been selected and otherwise signals an error: unloading does not make
  replacing a JVM in the same process safe.

  On Linux, filesystem device numbers from \code{stat} and
  \code{/proc/self/maps} can differ (e.g. btrfs subvolumes and overlayfs).
  Matching inodes then also require the dynamic loader to confirm the
  already mapped image without loading anything. If identity cannot be
  verified, explicit selection fails; attaching to an existing VM remains
  possible without a pathname comparison.

  On Windows, rJava retains MSVC runtime DLLs for later native Java
  loads. Already loaded runtimes keep precedence. Otherwise, for each
  runtime the JDK bundles, rJava selects the newer of System32 and the
  bundled copy before loading \code{jvm.dll}, preferring System32 on a
  tie or unknown version. Unbundled, unloaded runtimes are left to the
  Windows loader. Retained copies stay loaded for the process lifetime;
  this also determines which copies later native packages use. No
  persistent change to \code{PATH} or DLL search directories is made.
  A JDK that omits a required runtime may still need a suitable system
  redistributable. A runtime already loaded by a host is not replaced,
  even if a newer copy exists.


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
