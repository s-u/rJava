.jinit <- function(classpath=NULL) {
  .External("RinitJVM",classpath,PACKAGE="rJava")
}

.jnew <- function(class, ...) {
  o<-.External("RcreateObject", class, ..., PACKAGE="rJava")
  .C("checkExceptions")
  if (!is.null(o)) {
    o<-list(jobj=o, jclass=class)
    class(o)<-"jobjRef"
  }
  o
}

.jcall <- function(obj, returnSig="V", method, ..., evalArray=FALSE) {
  r<-NULL
  if (inherits(obj,"jobjRef")) {
    if (returnSig=="S")
      returnSig<-"Ljava/lang/String;"
    if (returnSig=="[S")
      returnSig<-"[Ljava/lang/String;"
    r<-.External("RcallMethod",obj$jobj,returnSig, method, ..., PACKAGE="rJava")
    if (substr(returnSig,1,1)=="[") {
      if (evalArray) {
        if (returnSig=="[I")
          return(.External("RgetIntArrayCont", r))
        else if (returnSig=="[D")
          return(.External("RgetDoubleArrayCont", r))
        else if (substr(returnSig,1,2)=="[L") # this one is wrong - we'll get all the jobjects, but we'd have to convert them to R objects first
          return(.External("RgetObjectArrayCont", r))
      }
      r<-list(jobj=r, jclass=NULL, jsig=returnSig)
      class(r)<-c("jarrayRef","jobjRef")
    } else if (substr(returnSig,1,1)=="L") {
      r<-list(jobj=r, jclass=substr(returnSig,2,nchar(returnSig)-1))
      class(r)<-"jobjRef"
    }
    
    .C("checkExceptions")
  } else { # call for static methods
  }
  r
}

.jfree <- function(obj) {
  if (!inherits(obj,"jobjRef"))
    stop("obj is not a Java object")
  .External("RfreeObject",obj$obj)
  invisible()
}

.jstrVal <- function(obj) {
  # .jstrVal(.jstrVal(...)) = .jstrVal(...)
  if (is.character(obj))
    return(obj)
  r<-NULL
  if (!inherits(obj,"jobjRef"))
    stop("can get value of Java objects only")
  if (obj$jclass=="lang/java/String")
    r<-.External("RgetStringValue",obj$jobj)
  else
    r<-.External("RtoString",obj$jobj)
  r
}

  
