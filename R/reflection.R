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

### reflected call - this high-level call uses reflection to call a method
### it is much less efficient than .jcall but doesn't require return type
### specification or exact matching of parameter types
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

### get the value of a field (static class fields are not supported yet)
.jfield <- function(o, name) {
  if (!inherits(o, "jobjRef"))
    stop("Object must be a Java reference.")
  cl <- .jcall(o, "Ljava/lang/Class;", "getClass")
  f <- .jcall(cl, "Ljava/lang/reflect/Field;", "getField", m)
  .jcall(f,"Ljava/lang/Object;","get",.jcast(o,"java/lang/Object"))
}

### list the fields of a class or object
.jfields <- function(o) {
  if (inherits(o, "jobjRef"))
    cl <- .jcall(o, "Ljava/lang/Class;", "getClass")
  else
    cl <- .jcall("java/lang/Class", "Ljava/lang/Class;", "forName", gsub("/",".",as.character(o)))
  f <- .jcall(cl, "[Ljava/lang/reflect/Field;", "getFields")
  unlist(lapply(f, function(x) .jcall(x, "S", "toString")))
}

### syntactic sugar to allow objet$field and object$methods(...)
### first attempts to find a field of that name and then a method
"$.jobjRef" <- function(o, m, ...) {
  cl <- .jcall(o, "Ljava/lang/Class;", "getClass")
  f <- .jcall(cl, "Ljava/lang/reflect/Field;", "getField", m)
  .jcheck(silent=TRUE)
  if (is.null(f))
    function(...) .jrcall(o, m, ...)
  else
    .jcall(f,"Ljava/lang/Object;","get",.jcast(o,"java/lang/Object"))
}

### support for object$field<-...
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
