# this part is common to all platforms and must be invoked
# from .First.lib after library.dynam

# actual namespace environment of this package
.env <- environment()

# variables in the rJava environment that will be initialized *after* the package is loaded
# they need to be pre-created at load time and populated later by .jinit
.delayed.variables <- c(".jniInitialized", ".jclassObject", ".jclassString",
                        ".jclass.int", ".jclass.double", ".jclass.float", ".jclass.boolean",
                        ".jinit.merge.error")

.jfirst <- function(libname, pkgname) {
  assign(".rJava.base.path", paste(libname, pkgname, sep=.Platform$file.sep), .env)
  assign(".jzeroRef", .Call("RgetNullReference", PACKAGE="rJava"), .env)

  for (x in .delayed.variables) assign(x, NULL, .env)
  assign(".jniInitialized", FALSE, .env)

  ## S4 classes update - all classes are created earlier in classes.R, but jobjRef's prototype is only valid after the dylib is loaded
  setClass("jobjRef", representation(jobj="externalptr", jclass="character"), prototype=list(jobj=.jzeroRef, jclass="java/lang/Object"), where=.env)
}
