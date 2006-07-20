## This file is part of the rJava package - low-level R/Java interface
## (C)2006 Simon Urbanek <simon.urbanek@r-project.org>
## For license terms see DESCRIPTION and/or LICENSE
##
## $Id$

## define S4 classes
setClass("jobjRef", representation(jobj="externalptr", jclass="character"), prototype=list(jobj=NULL, jclass=NULL))
setClass("jarrayRef", representation("jobjRef", jsig="character"))
setClass("jfloat", representation("numeric"))

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

  assign(".jzeroRef", .Call("RgetNullReference", PACKAGE="rJava"), je)

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

# create a new object
.jnew <- function(class, ...) {
  class <- gsub("\\.","/",class) # allow non-JNI specifiation
  .jcheck(silent=TRUE)
  o<-.External("RcreateObject", class, ..., PACKAGE="rJava")
  .jcheck()
  if (is.null(o))
    warning(paste("Unable to create object of the class",class,", returning null reference."))
  new("jobjRef", jobj=o, jclass=class)
}

# create a new object reference manually (avoid! for backward compat only)
.jmkref <- function(jobj, jclass="java/lang/Object") {
  new("jobjRef", jobj=jobj, jclass=jclass)
}

# evaluates an array reference. If rawJNIRefSignature is set, then obj is not assumed to be
# jarrayRef, but rather direct JNI reference with the corresponding signature
.jevalArray <- function(obj, rawJNIRefSignature=NULL, silent=FALSE) {
  jobj<-obj
  sig<-rawJNIRefSignature
  if (is.null(rawJNIRefSignature)) {
    if(!inherits(obj,"jarrayRef"))
      stop("The object is not an array reference (jarrayRef).")
    jobj<-obj@jobj
    sig<-obj@jsig
  }
  if (sig=="[I")
    return(.External("RgetIntArrayCont", jobj, PACKAGE="rJava"))
  else if (sig=="[J")
    return(.External("RgetLongArrayCont", jobj, PACKAGE="rJava"))
  else if (sig=="[B")
	return(.External("RgetByteArrayCont", jobj, PACKAGE="rJava"))
  else if (sig=="[D")
    return(.External("RgetDoubleArrayCont", jobj, PACKAGE="rJava"))
  else if (sig=="[F")
    return(.External("RgetFloatArrayCont", jobj, PACKAGE="rJava"))
  else if (sig=="[Ljava/lang/String;")
    return(.External("RgetStringArrayCont", jobj, PACKAGE="rJava"))
  else if (substr(sig,1,2)=="[L")
    return(lapply(.External("RgetObjectArrayCont", jobj, PACKAGE="rJava"),
                  function(x) new("jobjRef", jobj=x, jclass=substr(sig,3,nchar(sig)-1)) ))
  else if (substr(sig,1,2)=="[[")
    return(lapply(.External("RgetObjectArrayCont", jobj, PACKAGE="rJava"),
                  function(x) new("jarrayRef", jobj=x, jclass="", jsig=substr(sig,2,nchar(sig))) ))
  # if we don't know how to evaluate this, issue a warning and return the jarrayRef
  if (!silent)
    warning(paste("I don't know how to evaluate an array with signature",sig,". Returning a reference."))
  new("jarrayRef", jobj=jobj, jclass="", jsig=sig)
}

.jcall <- function(obj, returnSig="V", method, ..., evalArray=TRUE, evalString=TRUE, interface="RcallMethod") {
  .jcheck()
  r<-NULL
  # S is a shortcut for Ljava/lang/String;
  if (returnSig=="S")
    returnSig<-"Ljava/lang/String;"
  if (returnSig=="[S")
    returnSig<-"[Ljava/lang/String;"
  # original S (short) is now mapped to T so we need to re-map it (we don't really support short, though)
  if (returnSig=="T") returnSig <- "S"
  if (returnSig=="[T") returnSig <- "[S"
  if (inherits(obj,"jobjRef") || inherits(obj,"jarrayRef"))
    r<-.External("RcallMethod",obj@jobj,returnSig, method, ..., PACKAGE="rJava")
  else
    r<-.External("RcallStaticMethod",as.character(obj), returnSig, method, ..., PACKAGE="rJava")
  if (substr(returnSig,1,1)=="[") {
    if (evalArray)
      r<-.jevalArray(r,rawJNIRefSignature=returnSig)
    else
      r <- new("jarrayRef", jobj=r, jclass="", jsig=returnSig)
  } else if (substr(returnSig,1,1)=="L") {
    if (is.null(r)) return(NULL)
    
    if (returnSig=="Ljava/lang/String;" && evalString)
      return(.External("RgetStringValue",r, PACKAGE="rJava"))
    r <- new("jobjRef", jobj=r, jclass=substr(returnSig,2,nchar(returnSig)-1))
  }
  .jcheck()
  r
}

.jstrVal <- function(obj) {
  # .jstrVal(.jstrVal(...)) = .jstrVal(...)
  if (is.character(obj))
    return(obj)
  r<-NULL
  if (!inherits(obj,"jobjRef") && !inherits(obj,"jarrayRef"))
        stop("can get value of Java objects only")
  if (!is.null(obj@jclass) && obj@jclass=="lang/java/String")
    r<-.External("RgetStringValue", obj@jobj, PACKAGE="rJava")
  else
    r<-.External("RtoString", obj@jobj, PACKAGE="rJava")
  r
}

# casts java object into new.class - without(!) type checking
.jcast <- function(obj, new.class) {
  if (!inherits(obj,"jobjRef") && !inherits(obj,"jarrayRef"))
    stop("connot cast anything but Java objects")
  r<-obj
  new.class <- gsub("\\.","/", new.class) # allow non-JNI specifiation
  r@jclass<-new.class
  r
}

# makes sure that a given object is jarrayRef 
.jcastToArray <- function(obj, signature=NULL, class="", quiet=FALSE) {
  if (!inherits(obj, "jobjRef") && !inherits(obj, "jarrayRef"))
    return(.jarray(obj))
  if (is.null(signature)) {
    cl <- .jcall(obj, "Ljava/lang/Class;", "getClass")
    cn <- .jcall(cl, "Ljava/lang/String;", "getName")
    if (substr(cn,1,1) != "[") {
      if (quiet)
        return(obj)
      else
        stop("cannot cast to array, object signature is unknown and class name is not an array")
    }
    signature <- cn
  }
  if (inherits(obj, "jarrayRef")) {
    obj@jsig <- signature
    return(obj)
  }
  new("jarrayRef",jobj=obj@jobj,jsig=signature,jclass=class)
}

# creates a new "null" object of the specified class
# althought it sounds weird, the class is important when passed as
# a parameter (you can even cast the result)
.jnull <- function(class="java/lang/Object") { 
  new("jobjRef", jobj=.jzeroRef, jclass=class)
}

.jcheck <- function(silent=FALSE) {
  r <- .C("RJavaCheckExceptions", silent, FALSE, PACKAGE="rJava")
  invisible(r[[2]])
}

.jproperty <- function(key) {
  if (length(key)>1)
    sapply(key, .jproperty)
  else
    .jcall("java/lang/System", "S", "getProperty", as.character(key)[1])
}

print.jobjRef <- function(x, ...) {
  print(paste("Java-Object: ", .jstrVal(x), sep=''), ...)
  invisible(x)
}

print.jarrayRef <- function(x, ...) {
  print(paste("Java-Array-Object",x@jsig,": ", .jstrVal(x), sep=''), ...)
  invisible(x)
}

.jarray <- function(x, contents.class=NULL) {
# common mistake is to not specify a list but just a single Java object
# but, well, people just keep doing it so we may as well support it 
	if (inherits(x,"jobjRef")||inherits(x,"jarrayRef"))
		x <- list(x)
	.Call("RcreateArray", x, contents.class, PACKAGE="rJava")
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
