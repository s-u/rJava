#' converts a java class name to jni notation
tojni <- function( cl = "java.lang.Object" ){
	gsub( "[.]", "/", cl )
}

#' converts jni notation to java notation
tojava <- function( cl = "java/lang/Object" ){
	gsub( "/", ".", cl )
}

