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

.jcall <- function(obj, returnSig="V", method, ..., evalArray=FALSE, evalString=TRUE) {
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

  
