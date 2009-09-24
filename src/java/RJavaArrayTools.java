// :tabSize=2:indentSize=2:noTabs=false:folding=explicit:collapseFolds=1:

import java.lang.reflect.Array ; 

public class RJavaArrayTools {

	// {{{ isRectangularArray
	/**
	 * Indicates if o is a rectangular array
	 * 
	 * @param o an array
	 * @throws NotAnArrayException if o is not an array
	 */
	public static boolean isRectangularArray(Object o) {
		if( !isArray(o) ) return false; 
		boolean res = false; 
		try{
			if( getDimensionLength( o ) == 1 ) return true ;
			res = ( new RectangularArray(o) ).isRectangular() ;
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
	
	/** 
	 * Utility class used to check if an array is rectangular
	 *
	 */
	private static class RectangularArray {
	
		/**
		 * The array we are checking
		 */
		private Object array ;
		
		/**
		 * The dimensions of the array
		 */
		private int[] dimensions ; 
		
		/**
		 * Constructor
		 *
		 * @param array the array to check
		 * @throws NotAnArrayException if array is not an array
		 */
		public RectangularArray(Object array) throws NotAnArrayException {
			this.array = array ;
			dimensions = getDimensions(array); 
		}
		
		public boolean isRectangular( ){
			return isRectangular_( array, 0 ) ;
		}
		
		private boolean isRectangular_(Object o, int depth){
			if( depth == dimensions.length ) return true ; 
			int n = Array.getLength(o) ;
			if( n != dimensions[depth] ) return false ;
			for( int i=0; i<n; i++){
				if( !isRectangular_(Array.get(o, i),  depth+1) ){
					return false;
				}
			}
			return true ;
		}
	}
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
	 * @Deprecated use RJavaArrayTools#isArray
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
	
	// {{{ NotAnArrayException class
	public static class NotAnArrayException extends Exception{
		public NotAnArrayException(Class clazz){
			super( "not an array : " + clazz.getName() ) ;
		}
		public NotAnArrayException(String message){
			super( message ) ;
		}
	}
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
}
