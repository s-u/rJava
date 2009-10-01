## S4 classes (jobjRef is re-defined in .First.lib to contain valid jobj)                                                                                                  
#' java object reference
setClass("jobjRef", representation(jobj="externalptr", jclass="character"), 
	prototype=list(jobj=NULL, jclass="java/lang/Object"))

#' rugged arrays
setClass("jarrayRef", representation("jobjRef", jsig="character"))

#' rectangular java arrays double[][] d = new double[m][n]
setClass("jrectRef", 
	representation("jarrayRef", dimension="integer" ) ) 

setClass("jfloat", representation("array" ) )
setClass("jlong", representation("array" )  )
setClass("jbyte", representation("array" )  )
setClass("jshort", representation("array" ) )
setClass("jchar", representation("array" )  )



