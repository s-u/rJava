## define S4 classes
setClass("jobjRef", representation(jobj="externalptr", jclass="character"), prototype=list(jobj=NULL, jclass=NULL))
setClass("jarrayRef", representation("jobjRef", jsig="character"))
setClass("jfloat", representation("numeric"))

.jinit <- function(classpath=NULL, ..., silent=FALSE) {
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
  xr<-.External("RinitJVM",classpath,PACKAGE="rJava")
  if (xr==-1) stop("Unable to initialize JVM.")
  if (xr==-2) stop("Another JVM is already running and rJava was unable to attach itself to that JVM.")
  if (xr==1 && classpath!="" && !silent) warning("Since another JVM is already running, it's not possible to change its class path. Therefore the value of the speficied classpath was ignored.")
  #.jniInitialized<<-TRUE # hack hack hack - we should use something better ..

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
  
  invisible(xr)
}

# create a new object
.jnew <- function(class, ...) {
  class <- gsub("\\.","/",class) # allow non-JNI specifiation
  .jcheck()
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
  else if (sig=="[D")
    return(.External("RgetDoubleArrayCont", jobj, PACKAGE="rJava"))
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
  if (returnSig=="S")
    returnSig<-"Ljava/lang/String;"
  if (returnSig=="[S")
    returnSig<-"[Ljava/lang/String;"
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

#.jfree <- rm

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

# creates a new "null" object of the specified class
# althought it sounds weird, the class is important when passed as
# a parameter (you can even cast the result)
.jnull <- function(class="java/lang/Object") { 
  new("jobjRef", jobj=.jzeroRef, jclass=class)
}

.jcheck <- function(silent=FALSE) {
  if (!exists(".jniInitialized") || !.jniInitialized)
    stop("Java VM was not initialized. Please use .jinit to initialize JVM.")
  r <- .C("RJavaCheckExceptions", silent, FALSE, PACKAGE="rJava")
  invisible(r[[2]])
}

.jproperty <- function(key) {
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

.jarray <- function(x) {
  .Call("RcreateArray", x, PACKAGE="rJava")
}
