import java.lang.reflect.Array ; 

/** 
 * Utility class to deal with arrays
 */
public class ArrayWrapper {

	/**
	 * The array we are checking
	 */
	private Object array ;
	
	/**
	 * The dimensions of the array - if it is rectangular
	 */
	private int[] dimensions ;
	
	/**
	 * is this array rectangular
	 */ 
	private boolean isRect ; 
	
	/**
	 * Constructor
	 *
	 * @param array the array to check
	 * @throws NotAnArrayException if array is not an array
	 */
	public ArrayWrapper(Object array) throws NotAnArrayException {
		this.array = array ;
		dimensions = RJavaArrayTools.getDimensions(array);
		if( dimensions.length == 1){
			isRect = true ;
		} else{
			isRect = isRectangular_( array, 0 );
		}
		// reset the dimensions if the array is not rectangular
		dimensions = null ;
	}
	
	// making java < 1.5 happy
	public ArrayWrapper(int x)      throws NotAnArrayException { throw new NotAnArrayException("primitive type") ; }
	public ArrayWrapper(boolean x)  throws NotAnArrayException { throw new NotAnArrayException("primitive type") ; }
	public ArrayWrapper(byte x)     throws NotAnArrayException { throw new NotAnArrayException("primitive type") ; }
	public ArrayWrapper(long x)     throws NotAnArrayException { throw new NotAnArrayException("primitive type") ; }
	public ArrayWrapper(short x)    throws NotAnArrayException { throw new NotAnArrayException("primitive type") ; }
	public ArrayWrapper(double x)   throws NotAnArrayException { throw new NotAnArrayException("primitive type") ; }
	public ArrayWrapper(char x)     throws NotAnArrayException { throw new NotAnArrayException("primitive type") ; }
	public ArrayWrapper(float x)    throws NotAnArrayException { throw new NotAnArrayException("primitive type") ; }
	
	
	/**
	 * @return true if the array is rectangular
	 */
	public boolean isRectangular( ){
		return isRect ;
	}
	
	/**
	 * @return the dimensions of the array if it is rectangular, and null otherwise
	 */
	public int[] getDimensions(){
		return dimensions ; 
	}
	
	/**
	 * Recursively check all dimensions to see if an array is rectangular
	 */
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
	
