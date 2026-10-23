## Deferred JVM discovery. This runs at the first .jinit(),
## never at namespace load. The Java home is the first of:
##  1. getOption("java.home") (Windows only, as before) or JAVA_HOME; a value
##     that is not an existing directory is ignored with a warning, as rJava
##     did on Windows and macOS before deferred loading
##  2. on Windows, the CurrentVersion of the JavaSoft registry keys
##  3. on macOS, /usr/libexec/java_home
##  4. on Linux, R CMD config JAVA_HOME (the javareconf setting)
##  5. the java.home property reported by `java` on PATH
## An existing Java home without a JVM library is an error, not a fallback.
.find.java.runtime <- function() {
    windows <- .Platform$OS.type == "windows"
    javahome <- if (windows && !is.null(getOption("java.home"))) getOption("java.home") else Sys.getenv("JAVA_HOME")
    if (!is.character(javahome) || length(javahome) != 1L || is.na(javahome)) javahome <- ""
    if (nzchar(javahome) && !dir.exists(javahome)) {
        if (windows) message("java.home option: ", getOption("java.home"))
        message("JAVA_HOME environment variable: ", Sys.getenv("JAVA_HOME"))
        warning("Java home setting is INVALID, it will be ignored.\nPlease do NOT set it unless you want to override system settings.")
        javahome <- ""
    }
    if (!nzchar(javahome) && windows) javahome <- .find.java.registry()
    if (!nzchar(javahome) && file.exists("/usr/libexec/java_home"))
        javahome <- .java.output("/usr/libexec/java_home")
    if (!nzchar(javahome) && identical(Sys.info()[["sysname"]], "Linux")) {
        configured <- .java.output(file.path(R.home("bin"), "R"), c("CMD", "config", "JAVA_HOME"))
        if (nzchar(configured) && dir.exists(configured)) javahome <- configured
    }
    if (!nzchar(javahome))
        javahome <- .java.output("java", c("-cp", shQuote(system.file("java", package="rJava")), "getsp", "java.home"))
    if (nzchar(javahome) && dir.exists(javahome)) javahome else ""
}

## first line of a command's standard output, or "" if it fails
.java.output <- function(command, args=character()) {
    out <- tryCatch(suppressWarnings(system2(command, args, stdout=TRUE, stderr=FALSE)),
                    error=function(...) character())
    if (!length(out) || !is.null(attr(out, "status")) || is.na(out[1L])) "" else out[1L]
}

.find.java.registry <- function() {
    for (root in c("HLM", "HCU")) {
        for (key in c(
            "Software\\JavaSoft\\JRE",
            "Software\\JavaSoft\\JDK",
            "Software\\JavaSoft\\Java Runtime Environment",
            "Software\\JavaSoft\\Java Development Kit"
        )) {
            hive <- try(utils::readRegistry(key, root, 2), silent = TRUE)
            if (!inherits(hive, "try-error") && length(hive$CurrentVersion)) {
                home <- hive[[hive$CurrentVersion]]$JavaHome
                if (is.character(home) && length(home) == 1L && nzchar(home)) return(home)
            }
        }
    }
    ""
}

## JVM libraries in a Java home, server VM first. Known layouts are checked
## before a recursive search: lib/server (JDK 9+ on Linux and macOS),
## jre/lib/<arch>/server (JDK 8 on Linux), jre/lib/server (JDK 8 on macOS),
## bin/server and jre/bin/server (Windows).
.find.jvm.library <- function(home) {
    if (.Platform$OS.type == "windows") {
        lib <- "jvm.dll"
        base <- file.path(home, c("bin", "jre/bin"))
    } else {
        lib <- if (length(grep("^darwin", R.version$os))) "libjvm.dylib" else "libjvm.so"
        base <- file.path(home, c("lib", "jre/lib"))
        base <- c(base, list.dirs(base, recursive=FALSE))
    }
    found <- c(file.path(base, "server", lib), file.path(base, "client", lib))
    found <- found[file.exists(found)]
    if (!length(found)) {
        found <- list.files(home, paste0("^", gsub(".", "[.]", lib, fixed=TRUE), "$"),
                            recursive=TRUE, full.names=TRUE)
        found <- c(found[grepl("/server/", found, fixed=TRUE)], found[!grepl("/server/", found, fixed=TRUE)])
    }
    found
}
