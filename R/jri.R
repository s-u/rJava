## bindings into JRI

## warning: JRI REXP class has currently no finalizers! (RReleaseREXP must be used manually for now)
.r2j <- function(x, engine = NULL, convert = TRUE) {
  if (is.null(engine)) engine <- .jcall("org/rosuda/JRI/Rengine","Lorg/rosuda/JRI/Rengine;","getMainEngine")
  if (!is(engine, "jobjRef")) stop("invalid or non-existent engine")
  new("jobjRef",jobj=.Call("PushToREXP","org/rosuda/JRI/REXP",engine@jobj,engine@jclass,x,convert,PACKAGE="rJava"),jclass="org/rosuda/JRI/REXP")
}

.setupJRI <- function(new=TRUE) {
  ec <- .jfindClass("org.rosuda.JRI.Rengine", silent=TRUE)
  if (is.jnull(ec)) {
    .jcheck(TRUE)
    .jaddClassPath(system.file("jri","JRI.jar",package="rJava"))
    ec <- .jfindClass("org.rosuda.JRI.Rengine", silent=TRUE)
    .jcheck(TRUE)
    if (is.jnull(ec))
      stop("Cannot find JRI classes")
  }
  me <- .jcall("org/rosuda/JRI/Rengine","Lorg/rosuda/JRI/Rengine;","getMainEngine", check=FALSE)
  .jcheck(TRUE)
  if (!is.jnull(me)) {
    if (!new) return(TRUE)
    warning("JRI engine is already running.")
    return(FALSE)
  }
  e <- .jnew("org/rosuda/JRI/Rengine")
  !is.jnull(e)
}

.jengine <- function(start=FALSE, silent=FALSE) {
  me <- NULL
  ec <- .jfindClass("org.rosuda.JRI.Rengine", silent=TRUE)
  .jcheck(TRUE)
  if (!is.jnull(ec)) {
    me <- .jcall("org/rosuda/JRI/Rengine","Lorg/rosuda/JRI/Rengine;","getMainEngine", check=FALSE)
    .jcheck(TRUE)
  }
  if (is.jnull(me)) {
    if (!start) {
      if (silent) return(NULL)
      stop("JRI engine is not running.")
    }
    .setupJRI(FALSE)
    me <- .jcall("org/rosuda/JRI/Rengine","Lorg/rosuda/JRI/Rengine;","getMainEngine", check=FALSE)
    .jcheck(TRUE)
  }
  if (is.jnull(me) && !silent)
    stop("JRI engine is not running.")
  me
}
