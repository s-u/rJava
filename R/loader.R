.jaddClassPath <- function(path) {
  invisible(.jcall(rJava:::.rJava.class.loader,"V","addClassPath",as.character(path)))
}

.jclassPath <- function() {
  .jcall(rJava:::.rJava.class.loader,"[Ljava/lang/String;","getClassPath")
}

.jaddLibrary <- function(name, path) {
  invisible(.jcall(rJava:::.rJava.class.loader, "V", "addRLibrary", as.character(name)[1], as.character(path)[1]))
}

.jrmLibrary <- function(name) {
  ## FIXME: unimplemented
}

