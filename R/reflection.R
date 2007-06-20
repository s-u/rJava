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

### reflected call - this high-level call uses reflection to call a method
### it is much less efficient than .jcall but doesn't require return type
### specification or exact matching of parameter types
.jrcall <- function(o, method, ..., simplify=TRUE) {
  if (!is.character(method) | length(method)!=1)
    stop("Invalid method name - must be exactly one character string.")
  if (inherits(o, "jobjRef") || inherits(o, "jarrayRef"))
    cl <- .jcall(o, "Ljava/lang/Class;", "getClass")
  else
    cl <- .jfindClass(o)
  if (is.null(cl))
    stop("Cannot find class of the object.")
  p <- list(...)
  ar <- .jcall("java/lang/reflect/Array", "Ljava/lang/Object;", "newInstance", .jclassClass, as.integer(length(p)))
  op <- .jcall("java/lang/reflect/Array", "Ljava/lang/Object;", "newInstance", .jclassObject, as.integer(length(p)))
  if (length(p)>0) for (i in 1:length(p)) {
    if (inherits(p[[i]], "jobjRef") || inherits(o, "jarrayRef")) {
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
  if (is.null(m))
    stop("Cannot find Java method `",method,"' matching the supplied parameters.")
  r<-.jcall(m, "Ljava/lang/Object;", "invoke", .jcast(if(inherits(o,"jobjRef") || inherits(o, "jarrayRef")) o else cl, "java/lang/Object"), .jcast(op, "[Ljava/lang/Object;"))
  if (simplify && !is.jnull(r)) r <- .jsimplify(r)
  r
}

### simplify non-scalar reference to a scalar object if possible
.jsimplify <- function(o) {
  if (!inherits(o, "jobjRef") && !inherits(o, "jarrayRef"))
    return(o)
  cn <- .jclass(o, true=TRUE)
  if (cn == "java.lang.Boolean") .jcall(o, "Z", "booleanValue") else
  if (cn == "java.lang.Integer" || cn == "java.lang.Short" || cn == "java.lang.Charabter" || cn == "java.lang.Byte") .jcall(o, "I", "intValue") else
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
    .jfield(x, name)
})

### support for object$field<-...
setMethod("$<-", c(x="jobjRef"), function(x, name, value) .jfield(x, name) <- value)

# get a class name for an object
.jclass <- function(o, true=TRUE) {
  if (true) .jcall(.jcall(o, "Ljava/lang/Class;", "getClass"), "S", "getName")
  else o@jclass
}
