Sys.getRegistrySZ <- function (key, entry, root="HKEY_LOCAL_MACHINE") {
  .Call("RegGetStrValue",as.character(c(as.character(key),as.character(entry),as.character(root))))
}

.First.lib <-
function(libname, pkgname) {
    javahome <- Sys.getenv("JAVA_HOME")
    if(!nchar(javahome)) {
	# let's try to fetch the paths from registry via WinRegistry.dll
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
	    cat("using Java Runtime version",dispver,"\n")
	    javahome <- .Call("RegGetStrValue",c(key,"JavaHome"))
	    if (!is.null(javahome)) { # ok, let's try to get the real lib path
		p <- .Call("RegGetStrValue",c(key,"RuntimeLib"))
		if (!is.null(p)) {
		    # the following assumes that the entry is of the form
		    # ...\jvm.dll - this should be ok since if it's not,
		    # then we won't find the DLL either.
		    # Note that we just add it to the PATH so if this fails
		    # we still fall back to the JavaHome entry.
		    Sys.putenv(PATH=paste(Sys.getenv("PATH"),
					  substr(p,1,nchar(p)-8),sep=";"))
		}
	    }
	}
	if (is.null(javahome))
	    stop("JAVA_HOME is not set")
    }
    if(!nchar(javahome)) stop("JAVA_HOME is not set")
    else cat("using JAVA_HOME =", javahome, "\n")
    Sys.putenv(PATH=paste(Sys.getenv("PATH"),
               file.path(javahome, "bin", "client"), sep=";"))
    library.dynam("rJava", pkgname, libname)
}
