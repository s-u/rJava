.jgc <- function(R.gc=TRUE, ...) {
     if (R.gc) gc(...)
     .jcall(.jcall("java.lang.Runtime","Ljava/lang/Runtime;","getRuntime"), "V", "gc")
}
