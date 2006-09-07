### reflection functions - convenience function relying on the low-level
### functions .jcall/.jnew and friends

### reflection tools (inofficial so far, because it returns strings
### instead of the reflection objects - it's useful for quick checks,
### though)
.jmethods <- function(o, name=NULL) {
  if (is.null(o)) return (NULL)
  if (is.character(o) & length(o)==1) {
    o<-gsub("/",".",o)
    cl<-.jcall("java/lang/Class","Ljava/lang/Class;","forName",o)
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
    o<-gsub("/",".",o)
    cl<-.jcall("java/lang/Class","Ljava/lang/Class;","forName",o)
  } else if (inherits(o, "jobjRef") || inherits(o, "jarrayRef")) {
    cl<-.jcall(o, "Ljava/lang/Class;", "getClass")
  } else stop("Can operate on a single string or Java object only.")
  cs<-.jcall(cl,"[Ljava/lang/reflect/Constructor;","getConstructors")
  unlist(lapply(cs,function(x) .jcall(x,"S","toString")))
}

### reflected call - this high-level call uses reflection to call a method
### it is much less efficient than .jcall but doesn't require return type
### specification or exact matching of parameter types
.jrcall <- function(o, method, ...) {
  if (!is.character(method) | length(method)!=1)
    stop("Invalid method name - must be exactly one character string.")
  if (inherits(o, "jobjRef") || inherits(o, "jarrayRef"))
    cl <- .jcall(o, "Ljava/lang/Class;", "getClass")
  else
    cl <- .jcall("java/lang/Class", "Ljava/lang/Class;", "forName", gsub("/",".",o))
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
  if (!is.null(r)) {
    rcl <- .jcall(r, "Ljava/lang/Class;", "getClass")
    rcn <- .jcall(rcl, "S", "getName")
    if (rcn=="java.lang.Integer") r <- .jcall(r, "I", "intValue")
    else if (rcn=="java.lang.Number" | rcn=="java.lang.Double" | rcn=="java.lang.Float")
      r <- .jcall(r, "D", "doubleValue")
    else if (rcn=="java.lang.String") r <- .jstrVal(r)
    else if (rcn=="java.lang.Boolean") r <- .jcall(r, "Z", "booleanValue")
  }
  r
}

### simplify non-scalar reference to a scalar object if possible
.jsimplify <- function(o) {
  if (!inherits(o, "jobjRef") && !inherits(o, "jarrayRef"))
    return(o)
  cl <- .jcall(o, "Ljava/lang/Class;", "getClass")
  cn <- .jcall(cl, "Ljava/lang/String;", "getName")
  if (cn == "java.lang.Boolean") .jcall(o, "Z", "booleanValue") else
  if (cn == "java.lang.Integer") .jcall(o, "I", "intValue") else
  if (cn == "java.lang.Number" || cn == "java.lang.Double" || cn == "java.lang.Float") .jcall(o, "D", "doubleValue") else
  if (cn == "java.lang.String") .jstrVal(.jcast(o, "java/lang/String")) else
  o
}

### get the value of a field (static class fields are not supported yet)
.jfield <- function(o, name, simplify=TRUE, true.class=TRUE) {
  if (!inherits(o, "jobjRef") && !inherits(o, "jarrayRef") && !is.character(o))
    stop("Object must be a Java reference or class name.")
  if (is.character(o)) {
    cl <- .jcall("java/lang/Class", "Ljava/lang/Class;", "forName", gsub("/",".",o))
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
    cl <- .jcall("java/lang/Class", "Ljava/lang/Class;", "forName", gsub("/",".",as.character(o)))
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
    .jsimplify(.jcall(f,"Ljava/lang/Object;","get",.jcast(x,"java/lang/Object")))
})

### support for object$field<-...
setMethod("$<-", c(x="jobjRef"), function(x, name, value) {
  cl <- .jcall(x, "Ljava/lang/Class;", "getClass")
  f <- .jcall(cl, "Ljava/lang/reflect/Field;", "getField", name)
  .jcheck(silent=TRUE)
  if (is.null(f))
    stop("Field `",name,"' doesn't exist.")
  if (!inherits(value, "jobjRef")) {
    if (is.integer(value) && length(value)==1) value <- .jnew("java/lang/Integer", value) else
    if (is.numeric(value) && length(value)==1) value <- .jnew("java/lang/Double", as.double(value)) else
    if (is.character(value) && length(value)==1) value <- .jnew("java/lang/String", value) else
    if (is.logical(value) && length(value)==1) value <- .jnew("java/lang/Boolean", value)
    if (!inherits(value, "jobjRef"))
      stop("Sorry, cannot convert `value' to connesponding Java object. Please use Java objects in field assignments.")
  }
  .jcall(f,"V","set",.jcast(x,"java/lang/Object"),.jcast(value,"java/lang/Object"))
  invisible(x)
})

# get the class name for an object
.jclass <- function(o) .jcall(.jcall(o, "Ljava/lang/Class;", "getClass"), "S", "getName")
