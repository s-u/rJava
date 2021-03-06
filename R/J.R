setClass("jclassName", representation(name="character", jobj="jobjRef"))

jclassName <- function(class, class.loader=.rJava.class.loader) {
	if (.need.init()) .jinit()
	if( is( class, "jobjRef" ) && .jinherits(class, "java/lang/Class" ) ){
		jobj <- class
		name <- .jcall( class, "Ljava/lang/String;", "getName", evalString = TRUE )
	} else{
		name <- gsub("/",".",as.character(class))
		jobj <- .jfindClass(as.character(class), class.loader=class.loader)
	}
	new("jclassName", name=name, jobj=jobj)
}

setGeneric("new")
setMethod("new", signature(Class="jclassName"), function(Class, ...) .J(Class, ...))

## FIXME: this is not quite right - it looks at static method/fields only,
## but that prevents things like x = J("foo"); x$equals(x) from working
## while x$class$equals(x) works.
setMethod("$", c(x="jclassName"), function(x, name) {
	if( name == "class" ){
		x@jobj
	} else if (classHasField(x@jobj, name, TRUE)){
		.jfield(x, , name)
	} else if (classHasMethod(x@jobj, name, TRUE)){
		function(...) .jrcall(x, name, ...)
	} else if( classHasClass(x@jobj, name, FALSE) ){
		inner.cl <- .jcall( "RJavaTools", "Ljava/lang/Class;", "getClass", x@jobj, name, FALSE ) 
		new("jclassName", name=.jcall(inner.cl, "S", "getName"), jobj=inner.cl)
	} else {
		stop("no static field, method or inner class called `", name, "' in `", x@name, "'")
	}
})
setMethod("$<-", c(x="jclassName"), function(x, name, value) .jfield(x, name) <- value)
setMethod("show", c(object="jclassName"), function(object) invisible(show(paste("Java-Class-Name:",object@name))))
setMethod("as.character", c(x="jclassName"), function(x, ...) x@name)

## the magic `J'
J<-function(class, method, ..., class.loader=.rJava.class.loader) if (nargs() <= 2L && missing(method)) jclassName(class, class.loader=class.loader) else .jrcall(class, method, ..., class.loader=class.loader)
