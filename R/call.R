## define S4 classes
setClass("jobjRef", representation(jobj="integer", jclass="character"), prototype=list(jobj=0:0, jclass=NULL))
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
  .jniInitialized<<-TRUE # hack hack hack - we should use something better ..

  # get caches class objects for reflection
  je <- parent.env(environment())
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
  
  invisible(xr)
}

# create a new object
.jnew <- function(class, ...) {
  .jcheck()
  o<-.External("RcreateObject", class, ..., PACKAGE="rJava")
  .C("checkExceptions",PACKAGE="rJava")
  if (!is.null(o)) {
    if (o==0)
      warning(paste("Unable to create object of the class",class,", returning null reference."))
    o<-new("jobjRef", jobj=o, jclass=class)
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
    jobj<-obj@jobj
    sig<-obj@jsig
  }
  if (sig=="[I")
    return(.External("RgetIntArrayCont", jobj, PACKAGE="rJava"))
  else if (sig=="[D")
    return(.External("RgetDoubleArrayCont", jobj, PACKAGE="rJava"))
  else if (sig=="[Ljava/lang/String;" || sig=="[S")
    return(.External("RgetStringArrayCont", jobj, PACKAGE="rJava"))
  else if (substr(sig,1,2)=="[L")
    return(lapply(.External("RgetObjectArrayCont", jobj, PACKAGE="rJava"),
                  function(x) new("jobjRef", jobj=x, jclass=substr(sig,3,nchar(sig))) ))
  # if we don't know how to evaluate this, issue a warning and return the jarrayRef
  if (!silent)
    warning(paste("I don't know how to evaluate an array with signature",sig,". Returning a reference."))
  new("jarrayRef", jobj=jobj, jclass=NULL, jsig=sig)
}

.jcall <- function(obj, returnSig="V", method, ..., evalArray=TRUE, evalString=TRUE, interface="RcallMethod") {
  .jcheck()
  r<-NULL
  if (returnSig=="S")
    returnSig<-"Ljava/lang/String;"
  if (returnSig=="[S")
    returnSig<-"[Ljava/lang/String;"
  if (inherits(obj,"jobjRef"))
    r<-.External("RcallMethod",obj@jobj,returnSig, method, ..., PACKAGE="rJava")
  else
    r<-.External("RcallStaticMethod",as.character(obj), returnSig, method, ..., PACKAGE="rJava")
  if (substr(returnSig,1,1)=="[") {
    if (evalArray)
      r<-.jevalArray(r,rawJNIRefSignature=returnSig)
    else
      r <- new("jarrayRef", jobj=r, jclass=NULL, jsig=returnSig)
  } else if (substr(returnSig,1,1)=="L") {
    if (r==0)
      return(NULL)
    
    if (returnSig=="Ljava/lang/String;" && evalString) {
      s<-.External("RgetStringValue",r, PACKAGE="rJava")
      .External("RfreeObject",r, PACKAGE="rJava")
      return(s)
    }
    r <- new("jobjRef", jobj=r, jclass=substr(returnSig,2,nchar(returnSig)-1))
  }
  .C("checkExceptions",PACKAGE="rJava")
  r
}

.jfree <- function(obj) {
  if (!inherits(obj,"jobjRef"))
    stop("obj is not a Java object")
  .External("RfreeObject",obj@jobj, PACKAGE="rJava")
  .C("checkExceptions",PACKAGE="rJava");
  invisible()
}

.jstrVal <- function(obj) {
  # .jstrVal(.jstrVal(...)) = .jstrVal(...)
  if (is.character(obj))
    return(obj)
  r<-NULL
  if (!inherits(obj,"jobjRef"))
    stop("can get value of Java objects only")
  if (!is.null(obj@jclass) && obj@jclass=="lang/java/String")
    r<-.External("RgetStringValue", obj@jobj, PACKAGE="rJava")
  else
    r<-.External("RtoString", obj@jobj, PACKAGE="rJava")
  r
}

# casts java object into new.class - without(!) type checking
.jcast <- function(obj, new.class) {
  if (!inherits(obj, "jobjRef"))
    stop("connot cast anything but Java objects")
  r<-obj
  r@jclass<-new.class
  r
}

# creates a new "null" object of the specified class
# althought it sounds weird, the class is important when passed as
# a parameter (you can even cast the result)
.jnull <- function(class="java/lang/Object") { 
  new("jobjRef", jobj=0:0, jclass=class)
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

### reflection tools (inofficial so far, because it returns strings
### instead of the reflection objects - it's useful for quick checks,
### though)

.jmethods <- function(o, name=NULL) {
  if (is.null(o)) return (NULL)
  if (is.character(o) & length(o)==1) {
    o<-gsub("/",".",o)
    cl<-.jcall("java/lang/Class","Ljava/lang/Class;","forName",o)
  } else if (inherits(o, "jobjRef")) {
    cl<-.jcall(o, "Ljava/lang/Class;", "getClass")
  } else stop("Can operate on a single string or Java object only.")
  ms<-.jcall(cl,"[Ljava/lang/reflect/Method;","getMethods")
  ss<-unlist(lapply(ms,function(x) .jcall(x,"S","toString")))
  if (!is.null(name))
    grep(paste("\\.",name,"\\(",sep=''),ss,value=TRUE)
  else
    ss
}

.jconstructors <- function(o) {
  if (is.null(o)) return (NULL)
  if (is.character(o) & length(o)==1) {
    o<-gsub("/",".",o)
    cl<-.jcall("java/lang/Class","Ljava/lang/Class;","forName",o)
  } else if (inherits(o, "jobjRef")) {
    cl<-.jcall(o, "Ljava/lang/Class;", "getClass")
  } else stop("Can operate on a single string or Java object only.")
  cs<-.jcall(cl,"[Ljava/lang/reflect/Constructor;","getConstructors")
  unlist(lapply(cs,function(x) .jcall(x,"S","toString")))
}

.jrcall <- function(o, method, ...) {
  if (!is.character(method) | length(method)!=1)
    stop("Invalid method name - must be exactly one character string.")
  if (inherits(o, "jobjRef"))
    cl <- .jcall(o, "Ljava/lang/Class;", "getClass")
  else
    cl <- .jcall("java/lang/Class", "Ljava/lang/Class;", "forName", gsub("/",".",o))
  if (is.null(cl))
    stop("Cannot find class of the object.")
  p <- list(...)
  ar <- .jcall("java/lang/reflect/Array", "Ljava/lang/Object;", "newInstance", .jclassClass, as.integer(length(p)))
  op <- .jcall("java/lang/reflect/Array", "Ljava/lang/Object;", "newInstance", .jclassObject, as.integer(length(p)))
  if (length(p)>0) for (i in 1:length(p)) {
    if (inherits(p[[i]], "jobjRef")) {
      .jcall("java/lang/reflect/Array","V","set",.jcast(ar,"java/lang/Object"), as.integer(i-1),.jcast(.jcall(p[[i]],"Ljava/lang/Class;","getClass"),"java/lang/Object"))
      .jcall("java/lang/reflect/Array","V","set",.jcast(op,"java/lang/Object"), as.integer(i-1),.jcast(p[[i]],"java/lang/Object"))
    } else {
      pc <- class(p[[i]])
      if (pc == "character" & length(p[[i]]) == 1) {
        .jcall("java/lang/reflect/Array","V","set",.jcast(ar,"java/lang/Object"), as.integer(i-1),.jcast(.jclassString, "java/lang/Object"))
        .jcall("java/lang/reflect/Array","V","set",.jcast(op,"java/lang/Object"), as.integer(i-1),.jcast(.jnew("java/lang/String",p[[i]]), "java/lang/Object"))
      } else if (pc == "integer" & length(p[[i]]) == 1) {
        .jcall("java/lang/reflect/Array","V","set",.jcast(ar,"java/lang/Object"), as.integer(i-1),.jcast(.jclass.int, "java/lang/Object"))
        .jcall("java/lang/reflect/Array","V","set",.jcast(op,"java/lang/Object"), as.integer(i-1),.jcast(.jnew("java/lang/Integer",p[[i]]), "java/lang/Object"))
      } else if (pc == "jfloat" & length(p[[i]]) == 1) {
        .jcall("java/lang/reflect/Array","V","set",.jcast(ar,"java/lang/Object"), as.integer(i-1),.jcast(.jclass.float, "java/lang/Object"))
        .jcall("java/lang/reflect/Array","V","set",.jcast(op,"java/lang/Object"), as.integer(i-1),.jcast(.jnew("java/lang/Float",p[[i]]), "java/lang/Object"))
      } else if (pc == "numeric" & length(p[[i]]) == 1) {
        .jcall("java/lang/reflect/Array","V","set",.jcast(ar,"java/lang/Object"), as.integer(i-1),.jcast(.jclass.double, "java/lang/Object"))
        .jcall("java/lang/reflect/Array","V","set",.jcast(op,"java/lang/Object"), as.integer(i-1),.jcast(.jnew("java/lang/Double",p[[i]]), "java/lang/Object"))
      } else if (pc == "logical" & length(p[[i]]) == 1) {
        .jcall("java/lang/reflect/Array","V","set",.jcast(ar,"java/lang/Object"), as.integer(i-1),.jcast(.jclass.boolean, "java/lang/Object"))
        .jcall("java/lang/reflect/Array","V","set",.jcast(op,"java/lang/Object"), as.integer(i-1),.jcast(.jnew("java/lang/Boolean",p[[i]]), "java/lang/Object"))
      } else
      stop("Sorry, parameter type `",pc,"' is not supported.")
    }
  }
  m<-.jcall(cl, "Ljava/lang/reflect/Method;", "getMethod", method, .jcast(ar,"[Ljava/lang/Class;"))
  r<-.jcall(m, "Ljava/lang/Object;", "invoke", .jcast(if(inherits(o,"jobjRef")) o else cl, "java/lang/Object"), .jcast(op, "[Ljava/lang/Object;"))
  if (!is.null(r)) {
    rcl <- .jcall(r, "Ljava/lang/Class;", "getClass")
    rcn <- .jcall(rcl, "S", "getName")
    if (rcn=="java.lang.Integer") r <- .jcall(r, "I", "intValue")
    else if (rcn=="java.lang.Number" | rcn=="java.lang.Double" | rcn=="java.lang.Float")
      r <- .jcall(r, "D", "doubleValue")
    else if (rcn=="java.lang.String") r <- .jstrVal(s)
  }
  r
}

.jfield <- function(o, name) {
  if (!inherits(o, "jobjRef"))
    stop("Object must be a Java reference.")
  cl <- .jcall(o, "Ljava/lang/Class;", "getClass")
  f <- .jcall(cl, "Ljava/lang/reflect/Field;", "getField", m)
  .jcall(f,"Ljava/lang/Object;","get",.jcast(o,"java/lang/Object"))
}

.jfields <- function(o) {
  if (inherits(o, "jobjRef"))
    cl <- .jcall(o, "Ljava/lang/Class;", "getClass")
  else
    cl <- .jcall("java/lang/Class", "Ljava/lang/Class;", "forName", gsub("/",".",as.character(o)))
  f <- .jcall(cl, "[Ljava/lang/reflect/Field;", "getFields")
  unlist(lapply(f, function(x) .jcall(x, "S", "toString")))
}

"$.jobjRef" <- function(o, m, ...) {
  cl <- .jcall(o, "Ljava/lang/Class;", "getClass")
  f <- .jcall(cl, "Ljava/lang/reflect/Field;", "getField", m)
  .jcheck(silent=TRUE)
  if (is.null(f))
    function(...) .jrcall(o, m, ...)
  else
    .jcall(f,"Ljava/lang/Object;","get",.jcast(o,"java/lang/Object"))
}

"$<-.jobjRef" <- function(o, field, value) {
  cl <- .jcall(o, "Ljava/lang/Class;", "getClass")
  f <- .jcall(cl, "Ljava/lang/reflect/Field;", "getField", field)
  .jcheck(silent=TRUE)
  if (is.null(f))
    stop("Field `",field,"' doesn't exist.")
  if (!inherits(value, "jobjRef"))
    stop("Sorry, currently only fields with non-primitive types are supported.")
  .jcall(f,"Ljava/lang/Object;","set",.jcast(o,"java/lang/Object"),.jcast(value,"java/lang/Object"))
  invisible()
}

# there is no way to distinguish between double and float in R, so we need to mark floats specifically
.jfloat <- function(x) new("jfloat", as.numeric(x))

# get the class name for an object
.jclass <- function(o) .jcall(.jcall(o, "Ljava/lang/Class;", "getClass"), "S", "getName")
