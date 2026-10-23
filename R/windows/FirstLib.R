.onLoad <- function(libname, pkgname) {
    library.dynam("rJava", pkgname, libname)
    .jfirst(libname, pkgname)
}
