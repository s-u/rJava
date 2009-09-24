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
setGeneric( "str" )
setMethod("str", "jobjRef", function(object, ...){
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
._java_array_single_indexer <- function( x, i, drop, simplify = FALSE ){
	# arrays only
	
	# we cannot use the dispatch on jarrayRef because sometimes java arrays 
	# are not recognized specifically as jarrayRef so we just ignore the
	# jarrayRef class for now
	# we use reflection instead to identify if x is a java array
	._must_be_java_array( x )
	
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
			if( drop ){
				# that sounds alright ?
				return( .jnull( ) )
			} else{
				# return an array of the same component type as the original array
				# but of length 0
				return( .jcall( "java/lang/reflect/Array", "Ljava/lang/Object;", "newInstance", component.type, 0L  ) )
			}
		} else if( length(o) == 1L ){
			# wrap it as a java object or java array if ! drop
			valid_obj <- ._java_valid_object(o)
			if( drop ){
				return(valid_obj)
			} else {
				return( .jarray(valid_obj) )
			}
		} else {
			# drop makes no sense here
			return( .jarray( o ) )
		}
	}
	
	# the result an array of java objects
	sl <- ja[i]
	
	if( length( sl ) == 0L ){
		if( simplify ){
			return( NULL )
		}
		if( drop ){
			return( .jnull( ) )
		} else{
			return( .jcall( "java/lang/reflect/Array", "Ljava/lang/Object;", "newInstance", component.type, 0L  ) )
		} 
	} else if(length(sl) == 1L && drop ){
		java_obj <- sl[[1]]
		if( simplify ){
			return( .jsimplify(java_obj) ) 
		} else{
			return( java_obj )
		}
	} else{
		# just return the array
		# maybe we should use the Class#getComponentType() method  to check if 
		# we can simplify further
		# for example: 
		# Object[] objs = new Object[3]
		# objs[0] = "string"
		# objs[1] = "string"
		# objs[2] = new java.awt.Point( )
		# what should this return :
		# objs[1:2, simplify = TRUE ]
		return( .jarray( sl ) )
	}
}

# ## this is all weird - we need to distinguish between x[i] and x[i,] yet S4 fails to do so ...
setMethod( "[", signature( x = "jobjRef", i = "ANY", j = "missing" ), 
	function(x, i, j, ..., drop = FALSE){
		._java_array_single_indexer( x, i, drop = drop, ... )
	} )
# }}}

# {{{ double bracket indexing : [[
._java_array_double_indexer <- function( x, i ){
	# initial checks
	._must_be_java_array( x )
	i <- as.integer(i)[1L] - 1L # only one integer (shift one for java style indexing)
	
	cl <- .jcall( x, "Ljava/lang/Class;", "getClass" )
	clname <- .jcall( cl, "Ljava/lang/String;", "getName") 
	
	Array <- "java/lang/reflect/Array"
	o <- .jcast( x, "java/lang/Object" )
	obj <- switch( clname, 
		# deal with array of primitive first
		"[I"                  = .jcall( Array, "I",                  "getInt"         , o, i )  ,
		"[J"                  = .jcall( Array, "J",                  "getLong"        , o, i )  , # should I jlong this
		"[Z"                  = .jcall( Array, "Z",                  "getBoolean"     , o, i )  , 
		"[B"                  = .jcall( Array, "B",                  "getByte"        , o, i )  ,
		"[D"                  = .jcall( Array, "D",                  "getDouble"      , o, i )  ,
		"[S"                  = .jcall( Array, "T",                  "getShort"       , o, i )  , # should I jshort this
		"[C"                  = .jcall( Array, "C",                  "getChar"        , o, i )  , # int or character ?
		"[F"                  = .jcall( Array, "F",                  "getFloat"       , o, i )  , 
		"[Ljava.lang.String;" = .jsimplify( .jcall( Array, "Ljava/lang/Object;", "get", o, i ) ),
		
		# otherwise, just get the object
			                    .jcall( Array, "Ljava/lang/Object;", "get"            , o, i ) )
	obj
}

# this is the only case that makes sense: i is an integer or a numeric of length one
# we cannot use logical indexing or indexing by name because there is no such thing in java
setMethod( "[[", signature( x = "jobjRef", i = "integer", j = "missing"), 
	function(x, i, j, ...){
		._java_array_double_indexer( x, i, ... )
	} )
setMethod( "[[", signature( x = "jobjRef", i = "numeric", j = "missing"), 
	function(x, i, j, ...){
		._java_array_double_indexer( x, as.integer(i), ... )
	} )
# }}}

# {{{ head and tail
setGeneric( "head" )
setMethod("head", signature( x = "jobjRef" ), function(x, n = 6L, ... ){
	if( !isJavaArray( x ) ){
		stop( "not a java array" )
	}
	n_objs <- length(x)
	if( abs( n ) >= n_objs ){
		return( x )
	}
	len <- if( n > 0L ) n else n_objs + n
	x[seq_len(n), ... ]
} )

setGeneric( "tail" )
setMethod("tail", signature( x = "jobjRef" ), function(x, n = 6L, ... ){
	if( !isJavaArray( x ) ){
		stop( "not a java array" )
	}
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

