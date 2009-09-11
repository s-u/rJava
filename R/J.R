setClass("jclassName",representation(name="character"))
jclassName <- function(class) new("jclassName", name=gsub("/",".",as.character(class)))

setGeneric("new")
setMethod("new", signature(Class="jclassName"), function(Class, ...) .J(Class@name, ...))
## this may need some more sophisticated implementation - we just ignore fields for now ...
setMethod("$", c(x="jclassName"), function(x, name) function(...) .jrcall(x@name, name, ...))
setMethod("$<-", c(x="jclassName"), function(x, name, value) function(...) stop("sorry, unimplemented (yet)")) ## FIXME: unimplemented
setMethod("names", c(x="jclassName"), function(x) character(0)) ## FIXME: unimplemented
setMethod("show", c(object="jclassName"), function(object) invisible(show(paste("Java-Class-Name:",object@name))))
setMethod("as.character", c(x="jclassName"), function(x, ...) x@name)

## the magic `J'
J<-function(class, method, ...) if (nargs() == 1L && missing(method)) jclassName(class) else .jrcall(class, method, ...)

