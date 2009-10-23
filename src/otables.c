#include "rJava.h"

#define RJAVA_LOOKUP 23

/* This uses the mechanism of the RObjectTables package
   see: http://www.omegahat.org/RObjectTables/ */

/**
 * Returns the R_UnboundValue
 */
HIDE SEXP R_getUnboundValue() {
    return(R_UnboundValue);
}

/**
 * @param name name of a java class
 * @param canCache Can R cache this object
 * @param tb 
 * @return TRUE if the a class called name exists in on of the packages
 */
/* not actually used by R */
HIDE Rboolean rJavaLookupTable_exists(const char * const name, Rboolean *canCache, R_ObjectTable *tb){

#ifdef LOOKUP_DEBUG
 Rprintf( "<rJavaLookupTable_exists>\n" ); 
#endif
	
 if(tb->active == FALSE)
    return(FALSE);

 tb->active = FALSE;
 Rboolean val = classNameLookupExists( tb, name );
 tb->active = TRUE;
 
#ifdef LOOKUP_DEBUG
 Rprintf( "</rJavaLookupTable_exists>\n" ); 
#endif

 return( val );
}

/**
 * Returns a new jclassName object if the class exists or the 
 * unbound value if it does not
 *
 * @param name class name
 * @param canCache ?? 
 * @param tb lookup table
 */
SEXP rJavaLookupTable_get(const char * const name, Rboolean *canCache, R_ObjectTable *tb){

#ifdef LOOKUP_DEBUG
 Rprintf( "<rJavaLookupTable_get>\n" ); 
#endif

 SEXP val;
 if(tb->active == FALSE)
    return(R_UnboundValue);

 tb->active = FALSE;
 val = PROTECT( classNameLookup( tb, name ) );
 tb->active = TRUE;

#ifdef LOOKUP_DEBUG
	Rprintf( "</rJavaLookupTable_get>\n" ); 
#endif

 UNPROTECT(1); 
 return(val);
}

/**
 * Does nothing. Not applicable to java packages
 */
int rJavaLookupTable_remove(const char * const name,  R_ObjectTable *tb){
#ifdef LOOKUP_DEBUG
 Rprintf( "<rJavaLookupTableremove />\n" ); 
#endif
	error( "cannot remove from java package" ) ;
}

/**
 * Indicates if R can cahe the variable name. Currently allways return FALSE
 *
 * @param name name of the class
 * @param tb lookup table
 * @return allways FALSE (for now)
 */ 
HIDE Rboolean rJavaLookupTable_canCache(const char * const name, R_ObjectTable *tb){
#ifdef LOOKUP_DEBUG
 Rprintf( "<rJavaLookupTable_canCache />\n" ); 
#endif
	return( FALSE );
}

/**
 * Generates an error. assign is not possible on our lookup table
 */
HIDE SEXP rJavaLookupTable_assign(const char * const name, SEXP value, R_ObjectTable *tb){
#ifdef LOOKUP_DEBUG
 Rprintf( "<rJavaLookupTable_assign />\n" ); 
#endif
    error("can't assign to java package lookup");
}

/**
 * Returns the list of classes known to be included in the 
 * packages. Currently returns NULL
 *
 * @param tb lookup table
 */ 
HIDE SEXP rJavaLookupTable_objects(R_ObjectTable *tb) {
#ifdef LOOKUP_DEBUG
 Rprintf( "<rJavaLookupTable_objects />\n" ); 
#endif
	return( getKnownClasses( tb ) ); 
}


REPC SEXP newRJavaLookupTable(SEXP importer){
#ifdef LOOKUP_DEBUG
 Rprintf( "<newRJavaLookupTable>\n" ); 
#endif

 R_ObjectTable *tb;
 SEXP val, klass;

  tb = (R_ObjectTable *) malloc(sizeof(R_ObjectTable));
  if(!tb)
      error( "cannot allocate space for an internal R object table" );
  /* this is also checked on the R side, but well .. */
  
  tb->type = RJAVA_LOOKUP ; /* FIXME: not sure what this should be */
  tb->cachedNames = NULL;
  
  R_PreserveObject(importer); 
  tb->privateData = importer; 

  tb->exists = rJavaLookupTable_exists;
  tb->get = rJavaLookupTable_get;
  tb->remove = rJavaLookupTable_remove;
  tb->assign = rJavaLookupTable_assign;
  tb->objects = rJavaLookupTable_objects;
  tb->canCache = rJavaLookupTable_canCache;

  tb->onAttach = NULL;
  tb->onDetach = NULL;

  PROTECT(val = R_MakeExternalPtr(tb, install("UserDefinedDatabase"), R_NilValue));
  PROTECT(klass = NEW_CHARACTER(1));
   SET_STRING_ELT(klass, 0, COPY_TO_USER_STRING("UserDefinedDatabase"));
   SET_CLASS(val, klass);
  UNPROTECT(2);

#ifdef LOOKUP_DEBUG
 Rprintf( "</newRJavaLookupTable>\n" ); 
#endif
  return(val);
}


HIDE jobject getImporterReference(R_ObjectTable *tb ){
	jobject res = (jobject)EXTPTR_PTR( GET_SLOT( (SEXP)(tb->privateData), install( "jobj" ) ) );
	
#ifdef LOOKUP_DEBUG
	Rprintf( " getImporterReference : [%d]\n", res ); 
#endif
	return res ;
}

HIDE SEXP getKnownClasses( R_ObjectTable *tb ){
#ifdef LOOKUP_DEBUG
 Rprintf( "<getKnownClasses>\n" ); 
#endif
	jobject importer = getImporterReference(tb); 
	
	JNIEnv *env=getJNIEnv();
	jarray a ;
	
	jmethodID mid = (*env)->GetMethodID(env, rj_RJavaImport_Class, 
		"getKnownClasses", "()[Ljava/lang/String;");
#ifdef LOOKUP_DEBUG
	Rprintf( "  <RJavaImport.getKnownClasses [%d] />\n", mid );
	Rprintf( "  rj_RJavaImport_Class = [%d] />\n", rj_RJavaImport_Class );
	
#endif
	a = (jarray) (*env)->CallObjectMethod(env, importer, mid ) ;
	
#ifdef LOOKUP_DEBUG
 Rprintf( "</getKnownClasses>\n" ); 
#endif
	SEXP res = PROTECT( getStringArrayCont( a ) ) ;
	
#ifdef LOOKUP_DEBUG
 Rprintf( "  %d known classes\n", LENGTH(res) ); 
#endif
	
	UNPROTECT(1); 
	return res ;
}

HIDE SEXP classNameLookup( R_ObjectTable *tb, const char * const name ){
#ifdef LOOKUP_DEBUG
 Rprintf( "<classNameLookup>\n" ); 
#endif
	JNIEnv *env=getJNIEnv();
	
	jobject importer = getImporterReference(tb); 

	jobject clazz = newString(env, name ) ;
	jstring s ; /* Class */
	jmethodID mid = (*env)->GetMethodID(env, rj_RJavaImport_Class, 
		"lookup", "(Ljava/lang/String;)Ljava/lang/Class;");
#ifdef LOOKUP_DEBUG
 Rprintf( "  RJavaImport.lookup [%d]\n", mid ); 
#endif

	s = (jstring) (*env)->CallObjectMethod(env, importer, mid, clazz ) ;
	SEXP res ;
	int np ;
	if( !s ){
		res = R_getUnboundValue() ;
		np = 0; 
	} else{
		PROTECT( res = new_jclassName( env, s ) );
		np = 1; 
	}

	releaseObject(env, clazz);
	releaseObject(env, s);
    
	if( np ) UNPROTECT(1); 
#ifdef LOOKUP_DEBUG
 Rprintf( "</classNameLookup>\n" ); 
#endif
	return res ; 
}

HIDE Rboolean classNameLookupExists(R_ObjectTable *tb, const char * const name ){
#ifdef LOOKUP_DEBUG
 Rprintf( "<classNameLookupExists>\n" ); 
#endif
	
	JNIEnv *env=getJNIEnv();
	
	jobject importer = getImporterReference(tb); 
	jobject clazz = newString(env, name ) ;
	
	jboolean s ; /* Class */
	
	jmethodID mid = (*env)->GetMethodID(env, rj_RJavaImport_Class, 
		"exists", "(Ljava/lang/String;)Z");

#ifdef LOOKUP_DEBUG
 Rprintf( "  RJavaImport.exists [%d]\n", mid ); 
#endif
	
	s = (jboolean) (*env)->CallBooleanMethod(env, importer, mid , clazz ) ;
	Rboolean res = (s) ? TRUE : FALSE; 

#ifdef LOOKUP_DEBUG
 Rprintf( "  exists( %s ) = %d \n", name, res ); 
#endif
	
	releaseObject(env, clazz);
    
#ifdef LOOKUP_DEBUG
 Rprintf( "</classNameLookupExists>\n" ); 
#endif
    return res ;
}

