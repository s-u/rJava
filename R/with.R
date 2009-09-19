## This program is free software: you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation, either version 2 of the License, or
## (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program. If not, see <http://www.gnu.org/licenses/>.
##
## Author: Romain Francois <francoisromain@free.fr>

._populate_with_fields_and_methods <- function( env, fields, methods, data, only.static = FALSE ){
	object <- if( only.static ) .jnull() else .jcast( data )
	
	lapply( fields, function(x ){
  	  n <- .jcall( x, "S", "getName" )
  	  makeActiveBinding( n, function(v){
  	    if( missing(v) ){
  	      ## get
  	      .jsimplify( .jcall( x, "Ljava/lang/Object;", "get", object ) )
  	    } else {
  	      .jcall( x, "V", "set", object, v )
  	    }
  	  }, env )
  	} )

  	lapply( methods, function(m){
  	  n <- .jcall( m, "S", "getName" )
  	  if(! exists( n, envir = env, mode = "function" ) ){
  	    fallback <- tryCatch( match.fun( n ), error = function(e) NULL )
  	    assign( n, function(...) {
  	      tryCatch( .jrcall( if(only.static) data@name else data , n, ...), error = function(e){
  	        if( !is.null(fallback) && inherits(fallback, "function") ){
  	          fallback( ... )
  	        }
  	      } )
  	    }, env = env )
  	  }
  	} )
}

with.jobjRef <- function( data, expr, ...){
  env <- new.env( parent = environment() )
  clazz <- .jcall( data, "Ljava/lang/Class;", "getClass")
  
  fields <- .jcall( clazz,  "[Ljava/lang/reflect/Field;", "getFields" )
  methods <- .jcall( clazz, "[Ljava/lang/reflect/Method;", "getMethods" )
  ._populate_with_fields_and_methods( env, fields, methods, data, only.static = FALSE )

  assign( "this", data, env = env )

  eval( substitute( expr ), env = env )
}

within.jobjRef <- function(data, expr, ... ){
  call <- match.call()
  call[[1]] <- as.name("with.jobjRef")
  eval( call, parent.frame() )
  data
}

with.jclassName <- function( data, expr, ... ){
	env <- new.env( parent = environment() )
	clazz <- data@jobj
	
	static_fields  <- .jcall( "RJavaTools", "[Ljava/lang/reflect/Field;",  "getStaticFields",  clazz )
	static_methods <- .jcall( "RJavaTools", "[Ljava/lang/reflect/Method;",  "getStaticMethods",  clazz )
	
	._populate_with_fields_and_methods( env, static_fields, 
		static_methods, data, only.static = TRUE )

	eval( substitute( expr ), env = env )
}

within.jclassName <- function(data, expr, ... ){
  call <- match.call()
  call[[1]] <- as.name("with.jclassName")
  eval( call, parent.frame() )
  data
}


