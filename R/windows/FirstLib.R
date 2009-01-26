.onLoad <-
function(libname, pkgname) {
    require(methods)  ## we should not need this since it should be automatic
    .setenv <- if (exists("Sys.setenv")) Sys.setenv else Sys.putenv
    javahome <- Sys.getenv("JAVA_HOME")
    add.paths <- character(0)
    if(!nchar(javahome)) { ## JAVA_HOME was not set explicitly
	## let's try to fetch the paths from registry via WinRegistry.dll
	javahome <- NULL
	library.dynam("WinRegistry", pkgname, libname)
	key<-"Software\\JavaSoft\\Java Runtime Environment"
	jrever <- .Call("RegGetStrValue",c(key,"CurrentVersion"))
	if (is.null(jrever)) { # try JDK if JRE fails
	    key<-"Software\\JavaSoft\\Java Development Kit"
	    jrever <- .Call("RegGetStrValue",c(key,"CurrentVersion"))
	}
	if (!is.null(jrever)) {
	    dispver <- jrever
	    key<-paste(key,jrever,sep="\\")
	    micro <- .Call("RegGetStrValue", c(key,"MicroVersion"))
	    if (!is.null(micro)) dispver <- paste(dispver,micro,sep=".")
	    #cat("using Java Runtime version",dispver,"\n")
	    javahome <- .Call("RegGetStrValue",c(key,"JavaHome"))
	    if (!is.null(javahome)) { # ok, let's try to get the real lib path
		p <- .Call("RegGetStrValue",c(key,"RuntimeLib"))
		if (!is.null(p)) {
		    # the following assumes that the entry is of the form
		    # ...\jvm.dll - this should be ok since if it's not,
		    # then we won't find the DLL either.
		    # Note that we just add it to the PATH so if this fails
		    # we still fall back to the JavaHome entry.
		    add.paths <- c(add.paths, substr(p,1,nchar(p)-8))
		}
	    }
	}
      }
    if(!nchar(javahome))
        stop("JAVA_HOME is not set and could not be determined from the registry")
    #else cat("using JAVA_HOME =", javahome, "\n")

    ## we need to add Java-related library paths to PATH
    curPath <- Sys.getenv("PATH")
    cpc <- strsplit(curPath, ";", fixed=TRUE)[[1]] ## split it up so we can check presence/absence of a path

    ## for some strange reason file.path uses / on Windows so it's useless
    add.paths <- c(add.paths, # we add a few fall-back defaults:
                   paste(javahome, "bin", sep="\\"), # needed for msvcr71.dll in JRE 1.6
                   paste(javahome, "bin", "client", sep="\\"),
                   paste(javahome, "jre", "bin", "client", sep="\\")) # old JRE, won't work for more recent ones that use a separate location

    ## add paths only if they are not in already and they exist
    for (path in unique(add.paths))
        if (!path %in% cpc && file.exists(path)) curPath <- paste(curPath, path, sep=";")

    ## set PATH only if it's not correct already (cannot use identical/isTRUE because of PATH name attribute)
    if (curPath != Sys.getenv("PATH")) {
      .setenv(PATH=curPath)
      # check the resulting PATH - if they don't match then Windows has truncated it
      if (curPath != Sys.getenv("PATH"))
        warning("*** WARNING: your Windows system seems to suffer from truncated PATH bug which will likely prevent rJava from loading.\n      Either reduce your PATH or read http://support.microsoft.com/kb/906469 on how to fix your system.")
    }
    
    library.dynam("rJava", pkgname, libname)
    .jfirst(libname, pkgname)
}
