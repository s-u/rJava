## Java serialization/unserialization

.jserialize <- function(o) {
  if (!inherits(o, "jobjRef"))
    stop("can serialize Java objects only")
  .jcall("RJavaClassLoader","[B","toByte",.jcast(o, "java.lang.Object"))
}

.junserialize <- function(data) {
  if (!is.raw(data))
    stop("can de-serialize raw vectors only")
  o <- .jcall("RJavaClassLoader","Ljava/lang/Object;","toObjectPL",.jarray(data))
  if (!is.jnull(o)) {
    cl<-try(.jclass(o), silent=TRUE)
    if (all(class(cl) == "character"))
      o@jclass <- gsub("\\.","/",cl)
  }
  o
}
