.First.lib <- function(libname, pkgname) {
  # For MacOS X we have to remove /usr/X11R6/lib from the DYLD_LIBRARY_PATH
  # because it would override Apple's OpenGL framework (which is loaded
  # by JavaVM implicitly)
  Sys.putenv("DYLD_LIBRARY_PATH"=sub("/usr/X11R6/lib","",Sys.getenv("DYLD_LIBRARY_PATH")))
  library.dynam("rJava", pkgname, libname)
}
