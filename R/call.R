.jinit <- function(classpath=NULL) {
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
  .External("RinitJVM",classpath,PACKAGE="rJava")
  .jniInitialized<<-TRUE # hack hack hack - we should use something better ..
}

# create a new object
.jnew <- function(class, ...) {
  .jcheck()
  o<-.External("RcreateObject", class, ..., PACKAGE="rJava")
  .C("checkExceptions")
  if (!is.null(o)) {
    o<-list(jobj=o, jclass=class)
    class(o)<-"jobjRef"
  }
  o
}

# evaluates an array reference. If rawJNIRefSignature is set, then obj is not assumed to be
# jarrayRef, but rather direct JNI reference with the corresponding signature
.jevalArray <- function(obj, rawJNIRefSignature=NULL, silent=FALSE) {
  jobj<-obj
  sig<-rawJNIRefSignature
  if (is.null(rawJNIRefSignature)) {
    if(!inherits(obj,"jarrayRef"))
      stop("The object is not an array reference (jarrayRef).")
    jobj<-obj$jobj
    sig<-obj$jsig
  }
  if (sig=="[I")
    return(.External("RgetIntArrayCont", jobj))
  else if (sig=="[D")
    return(.External("RgetDoubleArrayCont", jobj))
  else if (sig=="[Ljava/lang/String;" || sig=="[S")
    return(.External("RgetStringArrayCont", jobj))
  else if (substr(sig,1,2)=="[L")
    return(lapply(.External("RgetObjectArrayCont", r),
                  function(x) { a<-list(jobj=x, jclass=NULL); class(a)<-"jobjRef"; a } ))
  # if we don't know how to evaluate this, issue a warning and return the jarrayRef
  if (!silent)
    warning(paste("I don't know how to evaluate an array with signature",sig,". Returning a reference."))
  r<-list(jobj=jobj, jclass=NULL, jsig=sig)
  class(r)<-c("jarrayRef","jobjRef")
  r
}

.jcall <- function(obj, returnSig="V", method, ..., evalArray=TRUE, evalString=TRUE, interface="RcallMethod") {
  .jcheck()
  r<-NULL
  if (returnSig=="S")
    returnSig<-"Ljava/lang/String;"
  if (returnSig=="[S")
    returnSig<-"[Ljava/lang/String;"
  if (inherits(obj,"jobjRef"))
    r<-.External("RcallMethod",obj$jobj,returnSig, method, ..., PACKAGE="rJava")
  else
    r<-.External("RcallStaticMethod",as.character(obj), returnSig, method, ..., PACKAGE="rJava")
  if (substr(returnSig,1,1)=="[") {
    if (evalArray)
      r<-.jevalArray(r,rawJNIRefSignature=returnSig)
    else {
      r<-list(jobj=r, jclass=NULL, jsig=returnSig)
      class(r)<-c("jarrayRef","jobjRef")
    }
  } else if (substr(returnSig,1,1)=="L") {
    if (r==0)
      return(NULL)
    
    if (returnSig=="Ljava/lang/String;" && evalString) {
      s<-.External("RgetStringValue",r)
      .External("RfreeObject",r)
      return(s)
    }
    r<-list(jobj=r, jclass=substr(returnSig,2,nchar(returnSig)-1))
    class(r)<-"jobjRef"
  }
  .C("checkExceptions")
  r
}

.jfree <- function(obj) {
  if (!inherits(obj,"jobjRef"))
    stop("obj is not a Java object")
  .External("RfreeObject",obj$jobj)
  .C("checkExceptions");
  invisible()
}

.jstrVal <- function(obj) {
  # .jstrVal(.jstrVal(...)) = .jstrVal(...)
  if (is.character(obj))
    return(obj)
  r<-NULL
  if (!inherits(obj,"jobjRef"))
    stop("can get value of Java objects only")
  if (!is.null(obj$jclass) && obj$jclass=="lang/java/String")
    r<-.External("RgetStringValue",obj$jobj)
  else
    r<-.External("RtoString",obj$jobj)
  r
}

# casts java object into new.class - without(!) type checking
.jcast <- function(obj, new.class) {
  if (!inherits(obj, "jobjRef"))
    stop("connot cast anything but Java objects")
  r<-obj
  r$jclass<-new.class
  r
}

# creates a new "null" object of the specified class
# althought it sounds weird, the class is important when passed as
# a parameter (you can even cast the result)
.jnull <- function(class="java/lang/Object") { 
  r<-list(jobj=0, jclass=class)
  class(r)<-"jobjRef"
  r
}

.jcheck <- function() {
  if (!exists(".jniInitialized") || !.jniInitialized)
    stop("Java VM was not initialized. Please use .jinit to initialize JVM.")
  .C("checkExceptions")
  invisible()
}
