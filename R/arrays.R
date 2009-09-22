#' Indicates if a object refers to a java array
#' 
#' @param o object 
#' @return TRUE if the object is a java array, FALSE if not
#'         (including when the object is not even a java reference)
isJavaArray <- function( o ){
	if( is( o, "jobjRef" ) || is( o, "jarrayRef" ) ){
		.jcall( "RJavaTools", "Z", "isArray", .jcast(o) )
	} else FALSE
}

#' reflectively get the length of the array
._length_java_array <- function(x){
	if( isJavaArray( x ) ){
		.jcall( "java.lang.reflect.Array", "I", "getLength", .jcast( x ) )
	} else{
		stop( "the supplied object is not a java array" ) 
	}
}

setMethod( "length", "jobjRef", ._length_java_array )
setMethod( "length", "jarrayRef", ._length_java_array )

# indexing of .jarrayRef
# is is not quite clear what the proper result should be, because technically
# [ should always return a jarrayRef, but it's not the most useful thing to do.
# the code below (ab)uses drop to try to deal with that, but it's not optimal ... 

# ._jctype <- function(x) if (is.jnull(x)) NA else if(is(x, "jarrayRef")) x@jsig else paste("L", x@jclass, ";", sep='')
# 
# ## so far only 1d-subsetting is supported
# 
# ._jarrayRef_single_indexer <- function( x, i, drop ){
# 	ja <- .jevalArray( x )
# 	if (!is.list(ja)) { # native type - use R subsetting
# 		o <- ja[i]
# 		return( if (drop) o else .jarray(o) )
# 	}
# 	# the result is more complex - an array of java objects
# 	sl <- ja[i]
# 	# find out whether they all have a common type
# 	nel <- unique(sapply(sl, ._jctype))
# 	if (any(is.na(nel))) nel <- nel[!is.na(nel)]
# 	if (length(nel) == 1L) { # common type
# 		if (!drop || !length(grep("^\\[", nel))) return(.jarray(sl, nel))
# 		r <- drop(sapply(sl, function(x) if (is.null(x)) NA else .jevalArray(x)))
# 		if (is.list(r)) .jarray(sl, nel) else r
# 	} else .jarray(sl)
# }
# 
# ## this is all weird - we need to distinguish between x[i] and x[i,] yet S4 fails to do so ...
# 
# setMethod( "[", signature( x = "jarrayRef", i = "ANY", j = "missing" ), 
# function(x, i, j, ..., drop = TRUE){
# 	._jarrayRef_single_indexer( x, i, drop )
# } )
# 
# setMethod( "[[", signature( x = "jarrayRef", i = "ANY", j = "missing"), function(x, i, j, ...) .jevalArray(x)[[i]])


