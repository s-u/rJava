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

with.jobjRef <- function( data, expr, ...){
  env <- new.env( parent = environment() )
  clazz <- .jcall( data, "Ljava/lang/Class;", "getClass")
  fields <- .jcall( clazz,  "[Ljava/lang/reflect/Field;", "getFields" )
  lapply( fields, function(x ){
    n <- .jcall( x, "S", "getName" )
    makeActiveBinding( n, function(v){
      if( missing(v) ){
        ## get
        .jsimplify( .jcall( x, "Ljava/lang/Object;", "get", .jcast( data ) ) )
      } else {
        .jfield( data, n ) <- v
      }
    }, env )
  } )
  methods <- .jcall( clazz,
                    "[Ljava/lang/reflect/Method;", "getMethods" )
  lapply( methods, function(m){
    n <- .jcall( m, "S", "getName" )
    if(! exists( n, envir = env, mode = "function" ) ){
      fallback <- tryCatch( match.fun( n ), error = function(e) NULL )
      assign( n, function(...) {
        tryCatch( .jrcall( data, n, ...), error = function(e){
          if( !is.null(fallback) && inherits(fallback, "function") ){
            fallback( ... )
          }
        } )
      }, env = env )
    }
  } )
  assign( "this", data, env = env )
  eval( substitute( expr ), env = env )
}

within.jobjRef <- function(data, expr, ... ){
  call <- match.call()
  call[[1]] <- as.name("with")
  eval( call, parent.frame() )
  data
}
