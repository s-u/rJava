## This file is part of the rJava package - low-level R/Java interface
## (C)2006 Simon Urbanek <simon.urbanek@r-project.org>
## For license terms see DESCRIPTION and/or LICENSE
##
## $Id$

# create a new object
.jnew <- function(class, ..., check=TRUE, silent=!check) {
  class <- gsub("\\.","/",class) # allow non-JNI specifiation
  if (check) .jcheck(silent=TRUE)
  o<-.External("RcreateObject", class, ..., silent=silent, PACKAGE="rJava")
  if (check) .jcheck(silent=silent)
  if (is.null(o)) {
  	  if (!silent) {
    	  stop("Failed to create object of class `",class,"'")
      } else {
    	  o <- .jzeroRef
      }
  }
  new("jobjRef", jobj=o, jclass=class)
}

# create a new object reference manually (avoid! for backward compat only!) the problem with this is that you need a valid `jobj' which is implementation-dependent so it is undefined outside rJava internals
# it is now used by JRI.createRJavaRef, though
.jmkref <- function(jobj, jclass="java/lang/Object") {
  new("jobjRef", jobj=jobj, jclass=gsub('\\.','/',jclass))
}

# evaluates an array reference. If rawJNIRefSignature is set, then obj is not assumed to be
# jarrayRef, but rather direct JNI reference with the corresponding signature
.jevalArray <- function(obj, rawJNIRefSignature=NULL, silent=FALSE) {
  jobj<-obj
  sig<-rawJNIRefSignature
  if (is.null(rawJNIRefSignature)) {
    if(!inherits(obj,"jarrayRef")) {
      if (!inherits(obj,"jobjRef"))
        stop("object is not a Java object reference (jobjRef/jarrayRef).")
      cl <- .jclass(obj)
      if (is.null(cl) || substr(cl,1,1) != "[")
        stop("object is not a Java array.")
      sig <- cl
    } else sig <- obj@jsig
    jobj<-obj@jobj
  } else if (is(obj, "jobjRef")) jobj<-obj@jobj
  if (sig=="[I")
    return(.Call("RgetIntArrayCont", jobj, PACKAGE="rJava"))
  else if (sig=="[J")
    return(.Call("RgetLongArrayCont", jobj, PACKAGE="rJava"))
  else if (sig=="[Z")
    return(.Call("RgetBoolArrayCont", jobj, PACKAGE="rJava"))
  else if (sig=="[B")
    return(.Call("RgetByteArrayCont", jobj, PACKAGE="rJava"))
  else if (sig=="[D")
    return(.Call("RgetDoubleArrayCont", jobj, PACKAGE="rJava"))
  else if (sig=="[S")
    return(.Call("RgetShortArrayCont", jobj, PACKAGE="rJava"))
  else if (sig=="[C")
    return(.Call("RgetCharArrayCont", jobj, PACKAGE="rJava"))
  else if (sig=="[F")
    return(.Call("RgetFloatArrayCont", jobj, PACKAGE="rJava"))
  else if (sig=="[Ljava/lang/String;")
    return(.Call("RgetStringArrayCont", jobj, PACKAGE="rJava"))
  else if (substr(sig,1,2)=="[L")
    return(lapply(.Call("RgetObjectArrayCont", jobj, PACKAGE="rJava"),
                  function(x) new("jobjRef", jobj=x, jclass=substr(sig,3,nchar(sig)-1)) ))
  else if (substr(sig,1,2)=="[[")
    return(lapply(.Call("RgetObjectArrayCont", jobj, PACKAGE="rJava"),
                  function(x) new("jarrayRef", jobj=x, jclass="", jsig=substr(sig,2,nchar(sig))) ))
  # if we don't know how to evaluate this, issue a warning and return the jarrayRef
  if (!silent)
    warning(paste("I don't know how to evaluate an array with signature",sig,". Returning a reference."))
  new("jarrayRef", jobj=jobj, jclass="", jsig=sig)
}

.jcall <- function(obj, returnSig="V", method, ..., evalArray=TRUE, evalString=TRUE, check=TRUE, interface="RcallMethod") {
  if (check) .jcheck()
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
    r<-.External(interface, obj@jobj, returnSig, method, ..., PACKAGE="rJava")
  else
    r<-.External(interface, as.character(obj), returnSig, method, ..., PACKAGE="rJava")
  if (returnSig=="V") return(invisible(NULL))
  if (substr(returnSig,1,1)=="[") {
    if (evalArray)
      r<-.jevalArray(r,rawJNIRefSignature=returnSig)
    else
      r <- new("jarrayRef", jobj=r, jclass="", jsig=returnSig)
  } else if (substr(returnSig,1,1)=="L") {
    if (is.null(r)) return(r)
    
    if (returnSig=="Ljava/lang/String;" && evalString)
      return(.External("RgetStringValue", r, PACKAGE="rJava"))
    r <- new("jobjRef", jobj=r, jclass=substr(returnSig,2,nchar(returnSig)-1))
  }
  if (check) .jcheck()
  if (.conv.in$.) .convert.in(r) else r
}

.jstrVal <- function(obj) {
  # .jstrVal(.jstrVal(...)) = .jstrVal(...)
  if (is.character(obj))
    return(obj)
  r<-NULL
  if (!is(obj,"jobjRef"))
    stop("can get value of Java objects only")
  if (!is.null(obj@jclass) && obj@jclass=="lang/java/String")
    r<-.External("RgetStringValue", obj@jobj, PACKAGE="rJava")
  else
    r<-.External("RtoString", obj@jobj, PACKAGE="rJava")
  r
}

# casts java object into new.class - without(!) type checking
.jcast <- function(obj, new.class="java/lang/Object") {
  if (!is(obj,"jobjRef"))
    stop("connot cast anything but Java objects")
  r<-obj
  new.class <- gsub("\\.","/", new.class) # allow non-JNI specifiation
  r@jclass<-new.class
  r
}

# makes sure that a given object is jarrayRef 
.jcastToArray <- function(obj, signature=NULL, class="", quiet=FALSE) {
  if (!is(obj, "jobjRef"))
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
  signature <- gsub('\\.', '/', signature)
  if (inherits(obj, "jarrayRef")) {
    obj@jsig <- signature
    return(obj)
  }
  new("jarrayRef",jobj=obj@jobj,jsig=signature,jclass=class)
}

# creates a new "null" object of the specified class
# although it sounds weird, the class is important when passed as
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

.jarray <- function(x, contents.class=NULL) {
# common mistake is to not specify a list but just a single Java object
# but, well, people just keep doing it so we may as well support it 
	if (inherits(x,"jobjRef")||inherits(x,"jarrayRef"))
		x <- list(x)
	.Call("RcreateArray", x, contents.class, PACKAGE="rJava")
}

# works on EXTPTR or jobjRef or NULL. NULL is always silently converted to .jzeroRef
.jidenticalRef <- function(a,b) {
  if (is(a,"jobjRef")) a<-a@jobj
  if (is(b,"jobjRef")) b<-b@jobj
  if (is.null(a)) a <- .jzeroRef
  if (is.null(b)) b <- .jzeroRef
  if (!inherits(a,"externalptr") || !inherits(b,"externalptr")) stop("Invalid argument to .jidenticalRef, must be a pointer or jobjRef")
  .Call("RidenticalRef",a,b,PACKAGE="rJava")
}

# returns TRUE only for NULL or jobjRef with jobj=0x0
is.jnull <- function(x) {
  (is.null(x) || (is(x,"jobjRef") && .jidenticalRef(x@jobj,.jzeroRef)))
}

# should we move this to C?
.jclassRef <- function(x, silent=FALSE) {
  if (is.jnull(x)) {
    if (silent) return(NULL) else stop("null reference has no class")
  }
  if (!is(x, "jobjRef")) {
    if (silent) return(NULL) else stop("invalid object")
  }
  cl <- NULL
  try(cl <- .jcall(x, "Ljava/lang/Class;", "getClass", check=FALSE))
  .jcheck(silent=TRUE)
  if (is.jnull(cl) && !silent) stop("cannot get class object")
  cl
}

# return class object for a given class name; silent determines whether an error should be thrown on failure (FALSE) or just null reference (TRUE)
.jfindClass <- function(cl, silent=FALSE) {
  if (!is.character(cl) || length(cl)!=1)
    stop("invalid class name")
  cl<-gsub("/",".",cl)
  a <- NULL
  if (!is.jnull(.rJava.class.loader))
    try(a <- .jcall("java/lang/Class","Ljava/lang/Class;","forName",cl,TRUE,.jcast(.rJava.class.loader,"java.lang.ClassLoader"), check=FALSE))
  else
    try(a <- .jcall("java/lang/Class","Ljava/lang/Class;","forName",cl,check=FALSE))
  .jcheck(silent=TRUE)
  if (!silent && is.jnull(a)) stop("class not found")
  a
}

# Java-side inheritance check; NULL inherits from any class, because it can be cast to any class type; cl can be a class name or a jobjRef to a class object
.jinherits <- function(o, cl) {
  if (is.jnull(o)) return(TRUE)
  if (!is(o, "jobjRef")) stop("invalid object")
  if (is.character(cl)) cl <- .jfindClass(cl)
  if (!is(cl, "jobjRef")) stop("invalid class object")  
  ocl <- .jclassRef(o)
  .Call("RisAssignableFrom", ocl@jobj, cl@jobj, PACKAGE="rJava")
}

# compares two things which may be Java objects. invokes Object.equals if applicable and thus even different pointers can be equal. if one parameter is not Java object, but scalar string/int/number/boolean then a corresponding Java object is created for comparison
# strict comparison returns FALSE if Java-reference is compared with non-reference. otherwise conversion into Java scalar object is attempted
.jequals <- function(a, b, strict=FALSE) {
  if (is.null(a)) a <- new("jobjRef")
  if (is.null(b)) b <- new("jobjRef")
  if (is(a,"jobjRef")) o <- a else
    if (is(b,"jobjRef")) { o <- b; b <- a } else
    return(all.equal(a,b))
  if (!is(b,"jobjRef")) {
    if (strict) return(FALSE)
    if (length(b)!=1) { warning("comparison of non-scalar values is always FALSE"); return(FALSE) }
    if (is.character(b)) b <- .jnew("java/lang/String",b) else
    if (is.integer(b)) b <- .jnew("java/lang/Integer",b) else
    if (is.numeric(b)) b <- .jnew("java/lang/Double",b) else
    if (is.logical(b)) b <- .jnew("java/lang/Boolean", b) else
    { warning("comparison of non-trivial values to Java objects is always FALSE"); return(FALSE) }
  }
  if (is.jnull(a))
    is.jnull(b)
  else
    .jcall(o, "Z", "equals", .jcast(b, "java/lang/Object"))
}

.jfield <- function(o, sig=NULL, name, true.class=is.null(sig), convert=TRUE) {
  if (length(sig)) {
    if (sig=='S') sig<-"Ljava/lang/String;"
    if (sig=='T') sig<-"S"
    if (sig=='[S') sig<-"[Ljava/lang/String;"
    if (sig=='[T') sig<-"[S"
  }
  r <- .Call("RgetField", o, sig, as.character(name), as.integer(true.class), PACKAGE="rJava")
  if (inherits(r, "jobjRef")) {
    if (substr(r@jclass,1,1) == "[") {
      if (convert)
        r <- .jevalArray(r, rawJNIRefSignature=r@jclass)
      else
        r <- new("jarrayRef", jobj=r@jobj, jclass=r@jclass, jsig=r@jclass)
    }
    if (convert && inherits(r, "jobjRef")) {
      if (r@jclass == "java/lang/String")
        return(.External("RgetStringValue", r@jobj, PACKAGE="rJava"))
      if (.conv.in$.) return(.convert.in(r))
    }
  }
  r
}

".jfield<-" <- function(o, name, value)
  .Call("RsetField", o, name, value, PACKAGE="rJava")

# there is no way to distinguish between double and float in R, so we need to mark floats specifically
.jfloat <- function(x) new("jfloat", as.numeric(x))
# the same applies to long
.jlong <- function(x) new("jlong", as.numeric(x))
# and byte
.jbyte <- function(x) new("jbyte", as.integer(x))
# and short
.jshort <- function(x) new("jshort", as.integer(x))
# and char (experimental)
.jchar <- function(x) new("jchar", as.integer(x))
