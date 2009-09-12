setClass("jclassName", representation(name="character", jobj="jobjRef"))
jclassName <- function(class) new("jclassName", name=gsub("/",".",as.character(class)), jobj=.jfindClass(as.character(class)))

setGeneric("new")
setMethod("new", signature(Class="jclassName"), function(Class, ...) .J(Class@name, ...))
## this may need some more sophisticated implementation - we just ignore fields for now ...
setMethod("$", c(x="jclassName"), function(x, name) {
	if (classHasField(x@jobj, name, TRUE)) .jfield(x@name, , name) else if (classHasMethod(x@jobj, name, TRUE)) function(...) .jrcall(x@name, name, ...) else stop("no static field or method called `", name, "' in `", x@name, "'")
})
setMethod("$<-", c(x="jclassName"), function(x, name, value) .jfield(x@name, name) <- value)
setMethod("names", c(x="jclassName"), function(x) classNamesMethod(x@jobj, static.only = TRUE ) )
setMethod("show", c(object="jclassName"), function(object) invisible(show(paste("Java-Class-Name:",object@name))))
setMethod("as.character", c(x="jclassName"), function(x, ...) x@name)

## the magic `J'
J<-function(class, method, ...) if (nargs() == 1L && missing(method)) jclassName(class) else .jrcall(class, method, ...)
