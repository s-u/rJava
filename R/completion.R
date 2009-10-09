# :tabSize=4:indentSize=4:noTabs=false:folding=explicit:collapseFolds=1:

# {{{ bring .DollarNames from the future if necessary
if( !exists( ".DollarNames", envir =  ) ){
	.DollarNames <- function(x, pattern)
    	UseMethod(".DollarNames")
} # otherwise if is imported in the namespace
# }}}

# {{{ support function to retrieve completion names from RJavaTools
### get completion names from RJavaTools
classNamesMethod <- function (cl, static.only = TRUE ) {
	# TODO: return both from java instead of two java calls
  fieldnames <- .jcall( "RJavaTools", "[Ljava/lang/String;", 
  	"getFieldNames", cl, static.only ) 
  methodnames <- .jcall( "RJavaTools", "[Ljava/lang/String;", 
  	"getMethodNames", cl, static.only )
  c(fieldnames, methodnames)
}
# }}}

# {{{ jclassName
._names_jclassName <- function(x){
	c( "class", classNamesMethod(x@jobj, static.only = TRUE ) )
}
._dollarnames_jclassName <- function(x, pattern = "" ){
	grep( pattern, ._names_jclassName(x), value = TRUE ) 
}

setMethod("names", c(x="jclassName"), ._names_jclassName )
setMethod(".DollarNames", c(x="jclassName"), ._names_jclassName )
# }}}

# {{{ jobjRef
._names_jobjRef <- function(x){
	classNamesMethod(.jcall(x, "Ljava/lang/Class;", "getClass"), static.only = FALSE )
}
._dollarnames_jclassName <- function(x, pattern = "" ){
	grep( pattern, ._names_jobjRef(x), value = TRUE )
}
setMethod("names", c(x="jobjRef"), ._names_jobjRef )
setMethod(".DollarNames", c(x="jobjRef"), ._dollarnames_jclassName )
# }}}

# {{{ jarrayRef and jrectRef
._names_jarrayRef <- function(x ){
	c("length", classNamesMethod(.jcall(x, "Ljava/lang/Class;", "getClass"), static.only = FALSE ) )
}
._dollarnames_jarrayRef <- function(x, pattern = ""){
	grep( pattern, ._names_jarrayRef(x), value = TRUE )
}

setMethod("names", c(x="jarrayRef"), ._names_jarrayRef )
setMethod("names", c(x="jrectRef"), ._names_jarrayRef )

setMethod(".DollarNames", c(x="jarrayRef"), ._dollarnames_jarrayRef )
setMethod(".DollarNames", c(x="jrectRef"), ._dollarnames_jarrayRef )

# }}}
