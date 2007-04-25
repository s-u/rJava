.jaddClassPath <- function(path) {
    if (!length(path)) return(invisible(NULL))
    if (!is.jnull(.rJava.class.loader))
        invisible(.jcall(.rJava.class.loader,"V","addClassPath",as.character(path)))
    else {
        cpr <- try(.jmergeClassPath(paste(path,collapse=.Platform$path.sep), silent=TRUE))
        invisible(!inherits(cpr, "try-error"))
    }
}

.jclassPath <- function() {
    if (is.jnull(.rJava.class.loader)) {
        cp <- .jcall("java/lang/System", "S", "getProperty", "java.class.path")
        unlist(strsplit(cp, .Platform$path.sep))
    } else {
        .jcall(.rJava.class.loader,"[Ljava/lang/String;","getClassPath")
    }
}

.jaddLibrary <- function(name, path) {
    if (!is.jnull(.rJava.class.loader))
        invisible(.jcall(.rJava.class.loader, "V", "addRLibrary", as.character(name)[1], as.character(path)[1]))
}

.jrmLibrary <- function(name) {
  ## FIXME: unimplemented
}

.jclassLoader <- function() {
    .rJava.class.loader
}

.jpackage <- function(name, moreClasses='', nativeLibrary=FALSE) {
  if (!.jniInitialized) .jinit()
  classes <- system.file("java", package=name)
  if (nchar(classes)) .jaddClassPath(classes)
  jar <- system.file("java", paste(name,".jar",sep=''), package=name)
  if (nchar(jar)) .jaddClassPath(jar)
  
  if (any(nchar(moreClasses))) {
    cl <- as.character(moreClasses)
    cl <- cl[nchar(cl)>0]
    .jaddClassPath(cl)
  }
  if (is.logical(nativeLibrary)) {
    if (nativeLibrary) {
      libs <- "libs"
      if (nchar(.Platform$r_arch)) lib <- file.path("libs", .Platform$r_arch)
      lib <- system.file(libs,paste(name, .Platform$dynlib.ext, sep=''),package=name)
      if (nchar(lib))
        .jaddLibrary(name, lib)
      else
        warning("Native library for `",name,"' was not be found.")
    }
  } else {
    .jaddLibrary(name, nativeLibrary)
  }
  invisible(TRUE)
}
