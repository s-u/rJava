.jaddClassPath <- function(path, class.loader=.rJava.class.loader) {
    if (!length(path)) return(invisible(NULL))
    if (!is.jnull(class.loader))
        invisible(.jcall(class.loader, "V", "addClassPath", as.character(path)))
    else {
        cpr <- try(.jmergeClassPath(paste(path, collapse=.Platform$path.sep)), silent=TRUE)
        invisible(!inherits(cpr, "try-error"))
    }
}

.jclassPath <- function(class.loader=.rJava.class.loader) {
    if (is.jnull(class.loader)) {
        cp <- .jcall("java/lang/System", "S", "getProperty", "java.class.path")
        unlist(strsplit(cp, .Platform$path.sep))
    } else {
        .jcall(class.loader,"[Ljava/lang/String;","getClassPath")
    }
}

.jaddLibrary <- function(name, path, class.loader=.rJava.class.loader) {
    if (!is.jnull(class.loader))
        invisible(.jcall(class.loader, "V", "addRLibrary", as.character(name)[1], as.character(path)[1]))
}

.jrmLibrary <- function(name) {
  ## FIXME: unimplemented
}

.jclassLoader <- function(package=NULL) {
    if (!is.null(package)) {
        loader <- asNamespace(package)$.rJava.class.loader
        if (!is.jnull(loader)) return(loader)
    }
    .rJava.class.loader
}

.jpackage <- function(name, jars='*', morePaths='', nativeLibrary=FALSE, lib.loc=NULL,
                      parameters=getOption("java.parameters"), own.loader=FALSE) {
    if (!.jniInitialized)
        .jinit(parameters=parameters)
    loader <- .rJava.class.loader
    if (isTRUE(own.loader)) {
        lib <- "libs"
        if (nchar(.Platform$r_arch))
            lib <- file.path("libs", .Platform$r_arch)
        ns <- asNamespace(name)
        loader <- ns$.rJava.class.loader <-
            .jnew("RJavaClassLoader", .rJava.base.path,
                  file.path(.rJava.base.path, lib), .rJava.class.loader, check = FALSE)
    }

    classes <- system.file("java", package=name, lib.loc=lib.loc)
    if (nchar(classes)) {
        .jaddClassPath(classes, class.loader=loader)
        if (length(jars)) {
            if (length(jars) == 1 && jars == '*') {
                jars <- grep(".*\\.jar", list.files(classes, full.names=TRUE), TRUE, value=TRUE)
                if (length(jars)) .jaddClassPath(jars, class.loader=loader)
            } else .jaddClassPath(paste(classes, jars, sep=.Platform$file.sep), class.loader=loader)
        }
    }
    if (any(nchar(morePaths))) {
        cl <- as.character(morePaths)
        cl <- cl[nchar(cl)>0]
        .jaddClassPath(cl, class.loader=loader)
    }
    if (is.logical(nativeLibrary)) {
        if (nativeLibrary) {
            libs <- "libs"
            if (nchar(.Platform$r_arch)) lib <- file.path("libs", .Platform$r_arch)
            lib <- system.file(libs, paste(name, .Platform$dynlib.ext, sep=''), package=name, lib.loc=lib.loc)
            if (nchar(lib))
                .jaddLibrary(name, lib, class.loader=loader)
            else
                warning("Native library for `",name,"' could not be found.")
        }
    } else {
        .jaddLibrary(name, nativeLibrary, class.loader=loader)
    }
    invisible(TRUE)
}
