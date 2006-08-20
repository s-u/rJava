# this part is common to all platforms and must be invoked
# from .First.lib after library.dynam

.jfirst <- function(libname, pkgname) {
  je <- as.environment(match("package:rJava", search()))
  assign(".rJava.base.path", paste(libname, pkgname, sep=.Platform$file.sep), je)
  assign(".jzeroRef", .Call("RgetNullReference", PACKAGE="rJava"), je)

  ## S4 classes
  setClass("jobjRef", representation(jobj="externalptr", jclass="character"), prototype=list(jobj=.jzeroRef, jclass="java/lang/Object"), where=je)
  #setClass("jarrayRef", representation("jobjRef", jsig="character"))
  #setClass("jfloat", representation("numeric"))
}
