## This file is part of the rJava package - low-level R/Java interface
## (C)2006 Simon Urbanek <simon.urbanek@r-project.org>
## For license terms see DESCRIPTION and/or LICENSE
##
## $Id$

## initialization

.jinit <- function(classpath=NULL, parameters=NULL, ..., silent=FALSE) {
  # determine path separator
  if (.Platform$OS.type=="windows")
    path.sep<-";"
  else
    path.sep<-":"

  if (!is.null(classpath)) {
    classpath<-as.character(classpath)
    if (length(classpath)>1)
      classpath<-paste(classpath,collapse=path.sep)
  }
  
  # merge CLASSPATH environment variable if present
  cp<-Sys.getenv("CLASSPATH")
  if (!is.null(cp)) {
    if (is.null(classpath))
      classpath<-cp
    else
      classpath<-paste(classpath,cp,sep=path.sep)
  }
  
  if (is.null(classpath)) classpath<-""
  # call the corresponding C routine to initialize JVM
  xr<-.External("RinitJVM", classpath, parameters, PACKAGE="rJava")
  if (xr==-1) stop("Unable to initialize JVM.")
  if (xr==-2) stop("Another VM is already running and rJava was unable to attach to that VM.")
  # we'll handle xr==1 later because we need fully initialized rJava for that

  # this should remove any lingering .jclass objects from the global env
  # left there by previous versions of rJava
  pj <- grep("^\\.jclass",ls(1,all=TRUE),value=T)
  if (length(pj)>0) { 
    rm(list=pj,pos=1)
    if (exists(".jniInitialized",1)) rm(list=".jniInitialized",pos=1)
    if (!silent) warning("rJava found hidden Java objects in your workspace. Internal objects from previous versions of rJava were deleted. Please note that Java objects cannot be saved in the workspace.")
  }
  
  # first, get our environment from the search list
  je <- as.environment(match("package:rJava", search()))
  assign(".jniInitialized", TRUE, je)
  # get cached class objects for reflection
  assign(".jclassObject", .jcall("java/lang/Class","Ljava/lang/Class;","forName","java.lang.Object"), je)
  assign(".jclassClass", .jcall("java/lang/Class","Ljava/lang/Class;","forName","java.lang.Class"), je)
  assign(".jclassString", .jcall("java/lang/Class","Ljava/lang/Class;","forName","java.lang.String"), je)

  ic <- .jcall("java/lang/Class","Ljava/lang/Class;","forName","java.lang.Integer")
  f<-.jcall(ic,"Ljava/lang/reflect/Field;","getField", "TYPE")
  assign(".jclass.int", .jcast(.jcall(f,"Ljava/lang/Object;","get",.jcast(ic,"java/lang/Object")),"java/lang/Class"), je)
  ic <- .jcall("java/lang/Class","Ljava/lang/Class;","forName","java.lang.Double")
  f<-.jcall(ic,"Ljava/lang/reflect/Field;","getField", "TYPE")
  assign(".jclass.double", .jcast(.jcall(f,"Ljava/lang/Object;","get",.jcast(ic,"java/lang/Object")),"java/lang/Class"), je)
  ic <- .jcall("java/lang/Class","Ljava/lang/Class;","forName","java.lang.Float")
  f<-.jcall(ic,"Ljava/lang/reflect/Field;","getField", "TYPE")
  assign(".jclass.float", .jcast(.jcall(f,"Ljava/lang/Object;","get",.jcast(ic,"java/lang/Object")),"java/lang/Class"), je)
  ic <- .jcall("java/lang/Class","Ljava/lang/Class;","forName","java.lang.Boolean")
  f<-.jcall(ic,"Ljava/lang/reflect/Field;","getField", "TYPE")
  assign(".jclass.boolean", .jcast(.jcall(f,"Ljava/lang/Object;","get",.jcast(ic,"java/lang/Object")),"java/lang/Class"), je)

  if (xr==1 && nchar(classpath)>0) {
    # it's a hack, so we run it in try(..) in case BadThings(TM) happen ...
    cpr <- try(.jmergeClassPath(classpath), silent=TRUE)
    if (inherits(cpr, "try-error")) {
      .jcheck(silent=TRUE)
      if (!silent) warning("Another VM is running already and the VM did not allow me to append paths to the class path.")
      assign(".jinit.merge.error", cpr, je)
    }
    if (length(parameters)>0 && !silent)
      warning("Cannot set VM parameters, because VM is running already.")
  }

  invisible(xr)
}

.jmergeClassPath <- function(cp) {
  ccp <- .jcall("java/lang/System","S","getProperty","java.class.path")
  ccpc <- strsplit(ccp, .Platform$path.sep)[[1]]
  cpc <- strsplit(cp, .Platform$path.sep)[[1]]
  rcp <- unique(cpc[!(cpc %in% ccpc)])
  if (length(rcp) > 0) {
    # the loader requires directories to include trailing slash
    # Windows: need / or \ ? (untested)
    dirs <- which(file.info(rcp)$isdir)
    for (i in dirs)
      if (substr(rcp[i],nchar(rcp[i]),nchar(rcp[i]))!=.Platform$file.sep)
        rcp[i]<-paste(rcp[i], .Platform$file.sep, sep='')

    ## this is a hack, really, that exploits the fact that the system class loader
    ## is in fact a subclass of URLClassLoader and it also subverts protection
    ## of the addURL class using reflection - yes, bad hack, but we cannot
    ## replace the system loader with our own, because we may need to attach to
    ## an existing VM
    ## The original discussion and code was at:
    ## http://forum.java.sun.com/thread.jspa?threadID=300557&start=15&tstart=0

    ## it should probably be run in try(..) because chances are that it will
    ## break if Sun changes something...
    cl <- .jcall("java/lang/ClassLoader", "Ljava/lang/ClassLoader;", "getSystemClassLoader")
    urlc <- .jcall("java/lang/Class", "Ljava/lang/Class;", "forName", "java.net.URL")
    clc <- .jcall("java/lang/Class", "Ljava/lang/Class;", "forName", "java.net.URLClassLoader")
    ar <- .jcall("java/lang/reflect/Array", "Ljava/lang/Object;",
                         "newInstance", .jclassClass, 1:1)
    .jcall("java/lang/reflect/Array", "V", "set",
                  .jcast(ar, "java/lang/Object"), 0:0,
                  .jcast(urlc, "java/lang/Object"))
    m<-.jcall(clc, "Ljava/lang/reflect/Method;", "getDeclaredMethod", "addURL", .jcast(ar,"[Ljava/lang/Class;"))
    .jcall(m, "V", "setAccessible", TRUE)

    ar <- .jcall("java/lang/reflect/Array", "Ljava/lang/Object;",
                 "newInstance", .jclassObject, 1:1)
    
    for (fn in rcp) {
      f <- .jnew("java/io/File", fn)
      url <- .jcall(f, "Ljava/net/URL;", "toURL")
      .jcall("java/lang/reflect/Array", "V", "set",
             .jcast(ar, "java/lang/Object"), 0:0,
             .jcast(url, "java/lang/Object"))
      .jcall(m, "Ljava/lang/Object;", "invoke",
             .jcast(cl, "java/lang/Object"), .jcast(ar, "[Ljava/lang/Object;"))
    }

    # also adjust the java.class.path property to not confuse others
    if (length(ccp)>1 || (length(ccp)==1 && nchar(ccp[1])>0))
      rcp <- c(ccp, rcp)
    acp <- paste(rcp, collapse=.Platform$path.sep)
    .jcall("java/lang/System","S","setProperty","java.class.path",as.character(acp))
  } # if #rcp>0
  invisible(.jcall("java/lang/System","S","getProperty","java.class.path"))
}
