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

#' index a java array
#' 
#' @param x a reference to a java array
#' @param i indexer (only 1D indexing supported so far)
#' @param drop if the result if of length 1, just return the java object instead of an array of length one
#' @param simplify further simplify the result
._java_array_single_indexer <- function( x, i, drop, simplify = FALSE ){
	# arrays only
	
	# we cannot use the dispatch on jarrayRef because sometimes java arrays 
	# are not recognized specifically as jarrayRef so we just ignore the
	# jarrayRef class for now
	# we use reflection instead to identify if x is a java array
	if( !isJavaArray( x ) ){
		stop( "`x` is not a reference to a java array" ) 
	}
	
	# 'eval' the array
	ja <- .jevalArray( x )
	
	# native type - use R subsetting and maybe remap to java 
	if (!is.list(ja)) { 
		o <- ja[i]
		if( simplify ){
			return(o) # return native type
		}
		
		if( length(o) == 0L) {
			# hmmm ... not sure about this
			# maybe this should be an array of length 0
			return( .jnull() )
		} else if( length(o) == 1L ){
			# wrap it as a java object or java array if ! drop
			valid_obj <- ._java_valid_object(o)
			return( if(drop) valid_obj else .jarray(valid_obj) ) 
		} else {
			# then we ignore drop
			return( ._java_valid_object( o ) )
		}
	}
	
	# the result an array of java objects
	sl <- ja[i]
	
	if( length( sl ) == 0L ){
		return( .jnull() ) # hmmm not sure 
	} else if(length(sl) == 1L){
		if( drop ){ # return the object
			java_obj <- sl[[1]]
			if( simplify ){
				return( .jsimplify(java_obj) ) 
			} else{
				return( java_obj )
			}
			
		} 
	}
	
	# just return the array
	# maybe we should use the Class#getComponentType() method  to check if 
	# we need to simplify
	return( .jarray( sl ) )
	
}

# not yet

# ## this is all weird - we need to distinguish between x[i] and x[i,] yet S4 fails to do so ...
# setMethod( "[", signature( x = "jobjRef", i = "ANY", j = "missing" ), 
# 	function(x, i, j, ..., drop = TRUE){
# 		# try to extract simplify argument from ...
# 		dots <- list(...)
# 		pm <- pmatch( "simplify", names( dots ), duplicates.ok = FALSE )
# 		if( any(!is.na(pm) ) ){
# 			simplify <- dots[ !is.na(pm) ][[1]]
# 			if( isTRUE(simplify ) ){
# 				return( ._java_array_single_indexer( x, i, drop, simplify = TRUE ) )
# 			}
# 		}
# 		._java_array_single_indexer( x, i, drop, simplify = FALSE )
# 	} )
# # 
# # setMethod( "[[", signature( x = "jarrayRef", i = "ANY", j = "missing"), function(x, i, j, ...) .jevalArray(x)[[i]])


