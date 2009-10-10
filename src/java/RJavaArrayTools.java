// :tabSize=2:indentSize=2:noTabs=false:folding=explicit:collapseFolds=1:

import java.lang.reflect.Array ; 
import java.util.Map; 
import java.util.HashMap;
import java.util.Vector ;

public class RJavaArrayTools {

	// TODO: maybe factor this out of this class
	private static Map primitiveClasses = initPrimitiveClasses() ;
	private static Map initPrimitiveClasses(){
		Map primitives = new HashMap(); 
		primitives.put( "I", Integer.TYPE ); 
		primitives.put( "Z", Boolean.TYPE );
		primitives.put( "B", Byte.TYPE );
		primitives.put( "J", Long.TYPE );
		primitives.put( "S", Short.TYPE );
		primitives.put( "D", Double.TYPE );
		primitives.put( "C", Character.TYPE );
		primitives.put( "F", Float.TYPE );
		return primitives; 
	}
	
	// {{{ getObjectTypeName
	/**
	 * Get the object type name of an multi dimensional array.
	 * 
	 * @param o object
	 * @throws NotAnArrayException if the object is not an array
	 */
	public static String getObjectTypeName(Object o) throws NotAnArrayException {
		Class o_clazz = o.getClass();
		if( !o_clazz.isArray() ) throw new NotAnArrayException( o_clazz ); 
		
		String cl = o_clazz.getName();
		return cl.replaceFirst("\\[+L?", "").replace(";", "") ; 
	}
	public static int getObjectTypeName(int x)     throws NotAnArrayException { throw new NotAnArrayException("primitive type : int     ") ; }
	public static int getObjectTypeName(boolean x) throws NotAnArrayException { throw new NotAnArrayException("primitive type : boolean ") ; }
	public static int getObjectTypeName(byte x)    throws NotAnArrayException { throw new NotAnArrayException("primitive type : byte    ") ; }
	public static int getObjectTypeName(long x)    throws NotAnArrayException { throw new NotAnArrayException("primitive type : long    ") ; }
	public static int getObjectTypeName(short x)   throws NotAnArrayException { throw new NotAnArrayException("primitive type : short   ") ; }
	public static int getObjectTypeName(double x)  throws NotAnArrayException { throw new NotAnArrayException("primitive type : double  ") ; }
	public static int getObjectTypeName(char x)    throws NotAnArrayException { throw new NotAnArrayException("primitive type : char    ") ; }
	public static int getObjectTypeName(float x)   throws NotAnArrayException { throw new NotAnArrayException("primitive type : float   ") ; }
	// }}}
	
	// {{{ makeArraySignature
	// TODO: test
	public static String makeArraySignature( String typeName, int depth ){
		StringBuffer buffer = new StringBuffer() ;
		for( int i=0; i<depth; i++){
			buffer.append( '[' ) ; 
		}
		buffer.append( typeName ); 
		if( ! isPrimitiveTypeName( typeName ) ){
			buffer.append( ';') ;
		}
		return buffer.toString(); 
	}
	// }}}
	
	// {{{ getClassForSignature
	public static Class getClassForSignature(String signature, ClassLoader loader) throws ClassNotFoundException {
		if( primitiveClasses.containsKey(signature) ){
			return (Class)primitiveClasses.get( signature ) ;
		}
		return Class.forName(signature, true, loader) ;
	}
	// }}}
	
	// {{{ isSingleDimensionArray
	public static boolean isSingleDimensionArray( Object o) throws NotAnArrayException{
		if( !isArray(o) ) throw new NotAnArrayException( o.getClass() ) ;
		
		String cn = o.getClass().getName() ; 
		if( cn.lastIndexOf('[') != 0 ) return false; 
		return true ; 
	}
	// }}}
	
	// {{{ isPrimitiveTypeName
	public static boolean isPrimitiveTypeName(String name){
		if( name.length() > 1 ) return false; 
		if( name.equals("I") ) return true ;
		if( name.equals("Z") ) return true ;
		if( name.equals("B") ) return true ;
		if( name.equals("J") ) return true ;
		if( name.equals("S") ) return true ;
		if( name.equals("D") ) return true ;
		if( name.equals("C") ) return true ;
		if( name.equals("F") ) return true ;
		return false; 
	}
	// }}}
	
	// {{{ isRectangularArray
	/**
	 * Indicates if o is a rectangular array
	 * 
	 * @param o an array
	 * @throws NotAnArrayException if o is not an array
	 * @deprecated use new ArrayWrapper(o).isRectangular() instead
	 */
	public static boolean isRectangularArray(Object o) {
		if( !isArray(o) ) return false; 
		boolean res = false; 
		try{
			if( getDimensionLength( o ) == 1 ) return true ;
			res = ( new ArrayWrapper(o) ).isRectangular() ;
		} catch( NotAnArrayException e){
			res = false; 
		}
		return res ;
	}
	
	
	// thoose below make java < 1.5 happy and me unhappy ;-)
	public static boolean isRectangularArray(int x)      { return false ; }
	public static boolean isRectangularArray(boolean x)  { return false ; }
	public static boolean isRectangularArray(byte x)     { return false ; }
	public static boolean isRectangularArray(long x)     { return false ; }
	public static boolean isRectangularArray(short x)    { return false ; }
	public static boolean isRectangularArray(double x)   { return false ; }
	public static boolean isRectangularArray(char x)     { return false ; }
	public static boolean isRectangularArray(float x)    { return false ; }
	
	// }}}
	
	// {{{ getDimensionLength
	/** 
	 * Returns the number of dimensions of an array
	 *
	 * @param o an array
	 * @throws NotAnArrayException if this is not an array
	 */
	public static int getDimensionLength( Object o) throws NotAnArrayException, NullPointerException {
		if( o == null ) throw new NullPointerException( "array is null" ) ;
		Class clazz = o.getClass();
		if( !clazz.isArray() ) throw new NotAnArrayException(clazz) ;
		int n = 0; 
		while( clazz.isArray() ){
			n++ ; 
			clazz = clazz.getComponentType() ;
		}
		return n ; 
	}
	// thoose below make java < 1.5 happy and me unhappy ;-)
	public static int getDimensionLength(int x)     throws NotAnArrayException { throw new NotAnArrayException("primitive type : int     ") ; }
	public static int getDimensionLength(boolean x) throws NotAnArrayException { throw new NotAnArrayException("primitive type : boolean ") ; }
	public static int getDimensionLength(byte x)    throws NotAnArrayException { throw new NotAnArrayException("primitive type : byte    ") ; }
	public static int getDimensionLength(long x)    throws NotAnArrayException { throw new NotAnArrayException("primitive type : long    ") ; }
	public static int getDimensionLength(short x)   throws NotAnArrayException { throw new NotAnArrayException("primitive type : short   ") ; }
	public static int getDimensionLength(double x)  throws NotAnArrayException { throw new NotAnArrayException("primitive type : double  ") ; }
	public static int getDimensionLength(char x)    throws NotAnArrayException { throw new NotAnArrayException("primitive type : char    ") ; }
	public static int getDimensionLength(float x)   throws NotAnArrayException { throw new NotAnArrayException("primitive type : float   ") ; }
	// }}}                                                                                          
	
	// {{{ getDimensions
	/** 
	 * Returns the dimensions of an array
	 *
	 * @param o an array
	 * @throws NotAnArrayException if this is not an array
	 * @return the dimensions of the array or null if the object is null
	 */
	public static int[] getDimensions( Object o) throws NotAnArrayException, NullPointerException {
		if( o == null ) throw new NullPointerException( "array is null" )  ;
		
		Class clazz = o.getClass();
		if( !clazz.isArray() ) throw new NotAnArrayException(clazz) ;
		Object a = o ;
		
		int n = getDimensionLength( o ) ; 
		int[] dims = new int[n] ;
		int i=0;
		int current ; 
		while( clazz.isArray() ){
			current = Array.getLength( a ) ;
			dims[i] = current ;
			i++;
			if( current == 0 ){
				break ; // the while loop 
			} else {
				a = Array.get( a, 0 ) ;
				clazz = clazz.getComponentType() ;
			}
		}
		
		/* in case of premature stop, we fill the rest of the array with 0 */
		// this might not be true: 
		// Object[][] = new Object[0][10] will return c(0,0)
		while( i < dims.length){
			dims[i] = 0 ;
			i++ ;
		}
		return dims ; 
	}
	// thoose below make java < 1.5 happy and me unhappy ;-)
	public static int[] getDimensions(int x)     throws NotAnArrayException { throw new NotAnArrayException("primitive type : int     ") ; }
	public static int[] getDimensions(boolean x) throws NotAnArrayException { throw new NotAnArrayException("primitive type : boolean ") ; }
	public static int[] getDimensions(byte x)    throws NotAnArrayException { throw new NotAnArrayException("primitive type : byte    ") ; }
	public static int[] getDimensions(long x)    throws NotAnArrayException { throw new NotAnArrayException("primitive type : long    ") ; }
	public static int[] getDimensions(short x)   throws NotAnArrayException { throw new NotAnArrayException("primitive type : short   ") ; }
	public static int[] getDimensions(double x)  throws NotAnArrayException { throw new NotAnArrayException("primitive type : double  ") ; }
	public static int[] getDimensions(char x)    throws NotAnArrayException { throw new NotAnArrayException("primitive type : char    ") ; }
	public static int[] getDimensions(float x)   throws NotAnArrayException { throw new NotAnArrayException("primitive type : float   ") ; }
	// }}}                                                                                          
	
	// {{{ getTrueLength
	/** 
	 * Returns the true length of an array (the product of its dimensions)
	 *
	 * @param o an array
	 * @throws NotAnArrayException if this is not an array
	 * @return the number of objects in the array (the product of its dimensions).
	 */
	public static int getTrueLength( Object o) throws NotAnArrayException, NullPointerException {
		if( o == null ) throw new NullPointerException( "array is null" ) ;
		
		Class clazz = o.getClass();
		if( !clazz.isArray() ) throw new NotAnArrayException(clazz) ;
		Object a = o ;
		
		int len = 1 ;
		int i = 0; 
		while( clazz.isArray() ){
			len = len * Array.getLength( a ) ;
			if( len == 0 ) return 0 ; /* no need to go further */
			i++;
			a = Array.get( a, 0 ) ;
			clazz = clazz.getComponentType() ;
		}
		return len ; 
	}
	// thoose below make java < 1.5 happy and me unhappy ;-)
	public static int getTrueLength(int x)     throws NotAnArrayException { throw new NotAnArrayException("primitive type : int     ") ; }
	public static int getTrueLength(boolean x) throws NotAnArrayException { throw new NotAnArrayException("primitive type : boolean ") ; }
	public static int getTrueLength(byte x)    throws NotAnArrayException { throw new NotAnArrayException("primitive type : byte    ") ; }
	public static int getTrueLength(long x)    throws NotAnArrayException { throw new NotAnArrayException("primitive type : long    ") ; }
	public static int getTrueLength(short x)   throws NotAnArrayException { throw new NotAnArrayException("primitive type : short   ") ; }
	public static int getTrueLength(double x)  throws NotAnArrayException { throw new NotAnArrayException("primitive type : double  ") ; }
	public static int getTrueLength(char x)    throws NotAnArrayException { throw new NotAnArrayException("primitive type : char    ") ; }
	public static int getTrueLength(float x)   throws NotAnArrayException { throw new NotAnArrayException("primitive type : float   ") ; }
	// }}}                                                                                          
	
	// {{{ isArray
	/**
	 * Indicates if a java object is an array
	 * 
	 * @param o object
	 * @return true if the object is an array
	 * @deprecated use RJavaArrayTools#isArray
	 */
	public static boolean isArray(Object o){
		if( o == null) return false ; 
		return o.getClass().isArray() ; 
	}
	// thoose below make java < 1.5 happy and me unhappy ;-)
	public static boolean isArray(int x){ return false ; }
	public static boolean isArray(boolean x){ return false ; }
	public static boolean isArray(byte x){ return false ; }
	public static boolean isArray(long x){ return false ; }
	public static boolean isArray(short x){ return false ; }
	public static boolean isArray(double x){ return false ; }
	public static boolean isArray(char x){ return false ; }
	public static boolean isArray(float x){ return false ; }
	// }}}
	
	// {{{ ArrayDimensionMismatchException
	public static class ArrayDimensionMismatchException extends Exception {
		public ArrayDimensionMismatchException( int index_dim, int actual_dim ){
			super( "dimension of indexer (" + index_dim + ") too large for array (depth ="+ actual_dim+ ")") ;
		}
	}
	// }}}
	
	// {{{ get
	/**
	 * Gets a single object from a multi dimensional array
	 *
	 * @param array java array
	 * @param position
	 */
	public static Object get( Object array, int[] position ) throws NotAnArrayException, ArrayDimensionMismatchException {
		int poslength = position.length ;
		int actuallength = getDimensionLength(array); 
		if( poslength > actuallength ){
			throw new ArrayDimensionMismatchException( poslength, actuallength ) ; 
		}
		Object o = array ;
		int i=0 ;
		while( i<poslength){
			o = Array.get( o, position[i] ) ;
			i++ ;
		}
		return(o); 
	}
	
	public static Object get( Object array, int position ) throws NotAnArrayException, ArrayDimensionMismatchException {
		return get( array, new int[]{position} ) ;
	}
	// }}}

	// {{{ unique
	public static Object[] unique( Object[] array ){
		int n = array.length ;
		boolean[] unique = new boolean[ array.length ];
		for( int i=0; i<array.length; i++){
			unique[i] = true ; 
		}
		
		Vector res = new Vector();
		boolean added ;
		for( int i=0; i<n; i++){
			if( !unique[i] ) continue ;
			Object current = array[i];
			added = false; 
			
			for( int j=i+1; j<n; j++){
				Object o_j = array[j] ;
				if( unique[j] && current.equals( o_j ) ){
					if( !added ){
						unique[i] = false; 
						res.add( current ); 
						added = true ;
					}
					unique[j] = false;
				}
			}
		}
		
		return res.toArray(); 
		
	}
	// }}}
	
}
