### reflection functions - convenience function relying on the low-level
### functions .jcall/.jnew and friends

### reflection tools (inofficial so far, because it returns strings
### instead of the reflection objects - it's useful for quick checks,
### though)
.jmethods <- function(o, name=NULL) {
  if (is.null(o)) return (NULL)
  if (is.character(o) & length(o)==1) {
    o<-gsub("/",".",o)
    cl<-.jfindClass(o)
  } else if (inherits(o, "jobjRef") || inherits(o, "jarrayRef")) {
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
    cl<-.jfindClass(o)
  } else if (inherits(o, "jobjRef") || inherits(o, "jarrayRef")) {
    cl<-.jcall(o, "Ljava/lang/Class;", "getClass")
  } else stop("Can operate on a single string or Java object only.")
  cs<-.jcall(cl,"[Ljava/lang/reflect/Constructor;","getConstructors")
  unlist(lapply(cs,function(x) .jcall(x,"S","toString")))
}

### this list maps R class names to Java class names for which the constructor does the necessary conversion (for use in .jrcall)
.class.to.jclass <-    c(character= "java/lang/String",
                         jbyte    = "java/lang/Byte",
                         integer  = "java/lang/Integer",
                         numeric  = "java/lang/Double",
                         logical  = "java/lang/Boolean",
                         jlong    = "java/lang/Long",
                         jchar    = "java/lang/Character",
                         jshort   = "java/lang/Short",
                         jfloat   = "java/lang/Float")

### Java classes that have a corresponding primitive type and thus a corresponding TYPE field to use with scalars
.primitive.classes = c("java/lang/Byte", "java/lang/Integer", "java/lang/Double", "java/lang/Boolean",
                       "java/lang/Long", "java/lang/Character", "java/lang/Short", "java/lang/Float")

### creates a list of valid java parameters, used in both .J and .jrcall
._java_valid_objects_list <- function( ... ){
  p <- lapply(list(...), function(a) {
    if (inherits(a, "jobjRef") || inherits(a, "jarrayRef")) a 
    else if (is.null(a)) .jnull() else {
      cm <- match(class(a)[1], names(.class.to.jclass))
      if (!any(is.na(cm))) { 
      	if (length(a) == 1) { 
      		y <- .jnew(.class.to.jclass[cm], a)
      		if (.class.to.jclass[cm] %in% .primitive.classes) attr(y, "primitive") <- TRUE
      		y 
      	} else .jarray(a)
      } else {
        stop("Sorry, parameter type `", cm ,"' is ambiguous or not supported.")
      }
    }
  })
  p
}

### returns a list of Class objects
### this is used in both .J and .jrcall
._java_class_list <- function( objects_list ){
	lapply(objects_list, function(x) if (isTRUE(attr(x, "primitive"))) .jfield(x, "Ljava/lang/Class;", "TYPE") else .jcall(x, "Ljava/lang/Class;", "getClass"))
}
                       
### reflected call - this high-level call uses reflection to call a method
### it is much less efficient than .jcall but doesn't require return type
### specification or exact matching of parameter types
.jrcall <- function(o, method, ..., simplify=TRUE) {
  if (!is.character(method) | length(method) != 1)
    stop("Invalid method name - must be exactly one character string.")
  if (inherits(o, "jobjRef") || inherits(o, "jarrayRef"))
    cl <- .jcall(o, "Ljava/lang/Class;", "getClass")
  else
    cl <- .jfindClass(o)
  if (is.null(cl))
    stop("Cannot find class of the object.")
  
  # p is a list of parameters that are formed solely by valid Java objects
  p <- ._java_valid_objects_list(...)
  
  # pc is a list of class objects
  pc <- ._java_class_list( p )

  # use RJavaTools.getMethod instead of reflection since we need to match parameters (thanks to Romain Francois for the idea and code)
  m <- .jcall("RJavaTools", "Ljava/lang/reflect/Method;", "getMethod", cl, method, .jarray(pc,"java/lang/Class"))
  if (is.null(m))
    stop("Cannot find Java method `",method,"' matching the supplied parameters.")
  ret <- .jcall(m, "Ljava/lang/Class;", "getReturnType")
  r <- .jcall(m, "Ljava/lang/Object;", "invoke", .jcast(if(inherits(o,"jobjRef") || inherits(o, "jarrayRef")) o else cl, "java/lang/Object"), .jarray(p, "java/lang/Object"))
  if (simplify && !is.jnull(r)) .jsimplify(r) else
  if (is.jnull(r) && .jcall(m, "Ljava/lang/Class;", "getReturnType") == .jclass.void) invisible(NULL)
  else r
}

### reflected construction of java objects
### This uses reflection to call a suitable constructor based 
### on the classes of the ... it does not require exact match between 
### the objects and the constructor parameters
### This is to .jnew what .jrcall is to .jcall
.J <- function(class, ...) {
  # try .jnew first
  o <- try( .jnew(class, ..., check = FALSE) , silent = TRUE)
  if( ! inherits( o, "try-error" ) ){
	return(o)
  }
  
  # allow non-JNI specifiation
  class <- gsub("\\.","/",class) 
  
  # p is a list of parameters that are formed solely by valid Java objects
  p <- ._java_valid_objects_list(...)
  
  # pc is a list of class objects
  pc <- ._java_class_list( p )

  # use RJavaTools to find the best constructor
  cons <- .jcall("RJavaTools", "Ljava/lang/reflect/Constructor;", 
  	"getConstructor", .jfindClass(class), .jarray(pc,"java/lang/Class") )
  if (is.null(cons))
    stop("Cannot find Java constructor matching the supplied parameters.")
  
  # use the constructor
  .jcall( cons, "Ljava/lang/Object;", "newInstance", 
  	.jarray(p, "java/lang/Object") )
  
}


### simplify non-scalar reference to a scalar object if possible
.jsimplify <- function(o) {
  if (!inherits(o, "jobjRef") && !inherits(o, "jarrayRef"))
    return(o)
  cn <- .jclass(o, true=TRUE)
  if (cn == "java.lang.Boolean") .jcall(o, "Z", "booleanValue") else
  if (cn == "java.lang.Integer" || cn == "java.lang.Short" || cn == "java.lang.Character" || cn == "java.lang.Byte") .jcall(o, "I", "intValue") else
  if (cn == "java.lang.Number" || cn == "java.lang.Double" || cn == "java.lang.Long" || cn == "java.lang.Float") .jcall(o, "D", "doubleValue") else
  if (cn == "java.lang.String") .jstrVal(.jcast(o, "java/lang/String")) else
  o
}

### get the value of a field (static class fields are not supported yet)
.jrfield <- function(o, name, simplify=TRUE, true.class=TRUE) {
  if (!inherits(o, "jobjRef") && !inherits(o, "jarrayRef") && !is.character(o))
    stop("Object must be a Java reference or class name.")
  if (is.character(o)) {
    cl <- .jfindClass(o)
    .jcheck(silent=TRUE)
    if (is.null(cl))
      stop("class not found")
    o <- .jnull()
  } else {
    cl <- .jcall(o, "Ljava/lang/Class;", "getClass")
    o <- .jcast(o, "java/lang/Object")
  }
  f <- .jcall(cl, "Ljava/lang/reflect/Field;", "getField", name)
  r <- .jcall(f,"Ljava/lang/Object;","get",o)
  if (simplify) r <- .jsimplify(r)
  if (true.class && (inherits(r, "jobjRef") || inherits(r, "jarrayRef"))) {
    cl <- .jcall(r, "Ljava/lang/Class;", "getClass")
    cn <- .jcall(cl, "Ljava/lang/String;", "getName")
    if (substr(cn,1,1) != '[')
      r@jclass <- gsub("\\.","/",cn)
  }
  r
}

### list the fields of a class or object
.jfields <- function(o) {
  if (inherits(o, "jobjRef") || inherits(o, "jarrayRef"))
    cl <- .jcall(o, "Ljava/lang/Class;", "getClass")
  else
    cl <- .jfindClass(as.character(o))
  f <- .jcall(cl, "[Ljava/lang/reflect/Field;", "getFields")
  unlist(lapply(f, function(x) .jcall(x, "S", "toString")))
}

### syntactic sugar to allow object$field and object$methods(...)
### first attempts to find a field of that name and then a method
setMethod("$", c(x="jobjRef"), function(x, name) {
  cl <- .jclassRef(x)
  f <- .jcall(cl, "Ljava/lang/reflect/Field;", "getField", name)
  .jcheck(silent=TRUE)
  if (is.null(f))
    function(...) .jrcall(x, name, ...)
  else
    .jfield(x, , name)
})

### support for object$field<-...
setMethod("$<-", c(x="jobjRef"), function(x, name, value) .jfield(x, name) <- value)

# get a class name for an object
.jclass <- function(o, true=TRUE) {
  if (true) .jcall(.jcall(o, "Ljava/lang/Class;", "getClass"), "S", "getName")
  else o@jclass
}

### support for names (useful for completion, thanks to Romain Francois)
setMethod("names", c(x="jobjRef"), function(x) {
  cl <- .jcall(x, "Ljava/lang/Class;", "getClass")

  fields <- .jcall(cl, "[Ljava/lang/reflect/Field;", "getFields")
  fieldnames <- sapply(fields, function(f) .jcall( f, "Ljava/lang/String;", "getName"))

  methodz <- .jcall(cl, "[Ljava/lang/reflect/Method;", "getMethods")
  methodnames <- sapply(methodz, function(m) .jcall( m, "Ljava/lang/String;", "getName"))

  nargs  <- sapply(methodz, function(m) length(.jcall(m, "[Ljava/lang/Class;", "getParameterTypes" )))
  methodnames <- paste(methodnames, ifelse( nargs == 0 , "()", "(" ), sep = "")
  c(fieldnames, methodnames)
})
