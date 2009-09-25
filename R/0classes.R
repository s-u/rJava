## S4 classes (jobjRef is re-defined in .First.lib to contain valid jobj)                                                                                                  
setClass("jobjRef", representation(jobj="externalptr", jclass="character"), prototype=list(jobj=NULL, jclass="java/lang/Object"))
setClass("jarrayRef", representation("jobjRef", jsig="character"))
setClass("jfloat", representation("numeric"))
setClass("jlong", representation("numeric"))
setClass("jbyte", representation("integer"))
setClass("jshort", representation("integer"))
setClass("jchar", representation("integer"))

#' rectangular java arrays double[][] d = new double[m][n]
setClass("jrectRef", representation("jarrayRef", dimension="integer") ) 

