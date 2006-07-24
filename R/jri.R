## bindings into JRI

## warning: JRI REXP class has currently no finalizers! (RReleaseREXP must be used manually for now)
.r2j <- function(x, engine = NULL) {
  if (is.null(engine)) engine <- .jcall("org/rosuda/JRI/Rengine","Lorg/rosuda/JRI/Rengine;","getMainEngine")
  if (!is(engine, "jobjRef")) stop("invalid or non-existent engine")
  new("jobjRef",jobj=.Call("PushToREXP","org/rosuda/JRI/REXP",engine@jobj,engine@jclass,x,PACKAGE="rJava"),jclass="org/rosuda/JRI/REXP")
}

