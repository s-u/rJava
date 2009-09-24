# :tabSize=4:indentSize=4:noTabs=false:folding=explicit:collapseFolds=1:
 
# {{{ utilities to deal with arrays
#' Indicates if a object refers to a java array
#' 
#' @param o object 
#' @return TRUE if the object is a java array, FALSE if not
#'         (including when the object is not even a java reference)
isJavaArray <- function( o ){
	if( ( is( o, "jobjRef" ) || is( o, "jarrayRef" ) ) && !is.jnull(o) ){
		.jcall( "RJavaArrayTools", "Z", "isArray", .jcast(o) )
	} else FALSE
}
._must_be_java_array <- function( o, message = "object is not a java array" ){
	if( !isJavaArray(o ) ){
		stop( message )
	}
}

#' get the component type of a java array
getComponentType <- function( o, check = TRUE ){
	if( check ) ._must_be_java_array( o )
	.jcall( .jcall( o, "Ljava/lang/Class;", "getClass" ), "Ljava/lang/Class;", "getComponentType" )
}

._jarray_simplify <- function( x ){
	._must_be_java_array( x )
	clname <- .jclass(x, true = TRUE )
	
	Array <- "java/lang/reflect/Array"
	obj <- switch( clname, 
		# deal with array of primitive first
		"[I"                  = .Call("RgetIntArrayCont"   , x@jobj, PACKAGE="rJava"), 
		"[J"                  = .Call("RgetLongArrayCont"  , x@jobj, PACKAGE="rJava"), 
		"[Z"                  = .Call("RgetBoolArrayCont"  , x@jobj, PACKAGE="rJava") , 
		"[B"                  = .Call("RgetByteArrayCont"  , x@jobj, PACKAGE="rJava") ,
		"[D"                  = .Call("RgetDoubleArrayCont", x@jobj, PACKAGE="rJava") ,
		"[S"                  = .Call("RgetShortArrayCont" , x@jobj, PACKAGE="rJava") , 
		"[C"                  = .Call("RgetCharArrayCont"  , x@jobj, PACKAGE="rJava") ,
		"[F"                  = .Call("RgetFloatArrayCont" , x@jobj, PACKAGE="rJava") , 
		"[Ljava.lang.String;" = .Call("RgetStringArrayCont", x@jobj, PACKAGE="rJava"),
		
		# otherwise, just get the object
		x )
	obj
}
# }}}

# {{{ length
#' reflectively get the true length of the array, this
#' is the product of the dimensions of the array (not just the length 
#' of the first dimension)
._length_java_array <- function(x){
	if( isJavaArray( x ) ){
		.jcall( "RJavaArrayTools", "I", "getTrueLength", .jcast(x) )
	} else{
		stop( "the supplied object is not a java array" ) 
	}
}

setMethod( "length", "jarrayRef", ._length_java_array )
setGeneric( "str" )
setMethod("str", "jarrayRef", function(object, ...){
	# FIXME: need something better
	show( object )
} )
# }}}

# {{{ single bracket indexing : [

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
._java_array_single_indexer <- function( x, i, j, drop, simplify = FALSE, silent = FALSE, ... ){
	# arrays only
	
	if( !silent ){
		if( ! missing( j ) ){
			warning( "only one dimensional indexing is currently supported in i, ignoring j argument" )
		}
		dots <- list( ... )
		if( length(dots) ){
			unnamed.dots <- dots[ names(dots) == "" ]
			if( length( unnamed.dots ) ){
				warning( "only one dimensional indexing is currently supported in [, ignoring ... arguments" ) 
			}
		}
	}
	
	# the component type of the array - maybe used to make 
	# arrays with the same component type, but of length 0
	component.type <- getComponentType( x, check = FALSE )
	
	# 'eval' the array
	ja <- .jevalArray( x )
	
	# native type - use R subsetting and maybe remap to java 
	if (!is.list(ja)) { 
		# perform the subset
		o <- ja[i]
		
		# return native type if simplify
		if( simplify ){
			return(o) 
		}
		
		if( length(o) == 0L) {
				# return an array of the same component type as the original array
				# but of length 0
				return( .jcall( "java/lang/reflect/Array", "Ljava/lang/Object;", "newInstance", component.type, 0L  ) )
		} else {
			# drop makes no sense here
			return( .jarray( o ) )
		}
	}
	
	# the result an array of java objects
	sl <- ja[i]
	
	if( length( sl ) == 0L ){
		# TODO: make simplify influencial here
		#       for example if x is int[] then we want to get integer(0)
		return( .jcall( "java/lang/reflect/Array", "Ljava/lang/Object;", "newInstance", component.type, 0L  ) )
	} else{
		# just return the array
		return( .jarray( sl ) )
	}
}

# ## this is all weird - we need to distinguish between x[i] and x[i,] yet S4 fails to do so ...
setMethod( "[", signature( x = "jarrayRef", i = "ANY" ), 
	function(x, i, j, ..., drop = FALSE){
		._java_array_single_indexer( x, i, j, drop = drop, ... )
	} )
# }}}

# {{{ double bracket indexing : [[
._java_array_double_indexer <- function( x, i, j, ..., simplify = TRUE ){
	# initial checks
	._must_be_java_array( x )
	
	dots <- list( ... )
	unnamed.dots <- if( length( dots ) ){
		dots[ names(dots) == "" ]
	}
	
	firstInteger <- function(.) as.integer(.)[1]
	firstIntegerOfEach <- function(.) sapply( ., firstInteger )
	
	index <- c( 
		if( !missing(i) ) firstInteger(i), 
		if( !missing(j) ) firstInteger(j), 
		if( !is.null(unnamed.dots) && length(unnamed.dots) ) firstIntegerOfEach( unnamed.dots )
		)
	
	if( !length(index) || is.null(index) ){
		# return the full object
		x
	} else{
		# subset
		index <- index - 1L # shift one left (java style indexing starts from 0
		RJavaArrayTools <- J("RJavaArrayTools")
		RJavaArrayTools$get( x, index )
	}

}

# this is the only case that makes sense: i is an integer or a numeric of length one
# we cannot use logical indexing or indexing by name because there is no such thing in java
setMethod( "[[", signature( x = "jarrayRef" ), 
	function(x, i, j, ...){
		._java_array_double_indexer( x, i, j, ... )
	} )
# }}}

# {{{ head and tail
setGeneric( "head" )
setMethod("head", signature( x = "jarrayRef" ), function(x, n = 6L, ... ){
	if( !isJavaArray( x ) ){
		stop( "not a java array" )
	}
	# FIXME : this only makes sense for 1d arays
	n_objs <- length(x)
	if( abs( n ) >= n_objs ){
		return( x )
	}
	len <- if( n > 0L ) n else n_objs + n
	x[seq_len(n), ... ]
} )

setGeneric( "tail" )
setMethod("tail", signature( x = "jarrayRef" ), function(x, n = 6L, ... ){
	if( !isJavaArray( x ) ){
		stop( "not a java array" )
	}
	# FIXME : this only makes sense for 1d arays
	n_objs <- length(x)
	if( abs( n ) >= n_objs ) return(x)
	if( n < 0L){ 
		n <- n_objs + n
	}
	return( x[ seq.int( n_objs-n+1, n_objs ) , ... ] )
} )
# }}}

# {{{ japply - apply a function to each element of a java array
japply <- function( X, FUN = if( simplify) force else "toString", simplify = FALSE, ...){
	
	callfun <- function( o, ... ){
		# o might not be a java object
		if( !is( o, "jobjRef" ) ){
			FUN <- match.fun(FUN)
			return( FUN( o, ... ) )
		}
		
		# but if it is one, then FUN might represent one of its methods
		if( is.character( FUN ) && hasJavaMethod( o, FUN) ){
			return( .jrcall( o, FUN, ... ) )
		}
		
		# or try to match.fun
		f <- try( match.fun( FUN ), silent = TRUE )
		if( inherits( f, "try-error" ) ){
			NULL
		} else{
			f( o, ... )
		}
	}
	
	simplifier <- function( o ){
		if( ! simplify ) return(o)
		
		o <- .jsimplify( o )
		if( isJavaArray( o ) ){
			o <- ._jarray_simplify( o )
		}
		o
	}
	
	if( !is(X, "jobjRef" ) ){
		lapply( X, FUN, ... )
	} else if( isJavaArray( X ) ){
		lapply( seq( along = X ), function(i){
			o <- simplifier( .jcall( "java.lang.reflect.Array", "Ljava/lang/Object;", "get", X, (i-1L) ) )
			callfun( o )
		} )
	} else if( X %instanceof% "java.lang.Iterable" ){
		iterator <- X$iterator()
		res <- NULL
		hasNext <- function(){
			.jcall( iterator, "Z", "hasNext" ) 
		}
		while( hasNext() ){
			o <- simplifier( .jcall( iterator, "Ljava/lang/Object;", "next" ) )
			res <- append( res, list( callfun(o, ...) ) ) 
		}
		res	
	} else{
		stop( "don't know how to japply" ) 
	}
}
# }}}

