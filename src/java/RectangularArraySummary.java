// :tabSize=2:indentSize=2:noTabs=false:folding=explicit:collapseFolds=1:

import java.lang.reflect.Array ; 

/** 
 * Utility class to extract something from a rectangular array
 */
public class RectangularArraySummary extends RJavaArrayIterator {
	
	private int length ; 
	
	private String typeName ;
	
	private boolean isprimitive ;
	
	/**
	 * Constructor
	 *
	 * @param array the array to check
	 * @throws NotAnArrayException if array is not an array
	 */
	public RectangularArraySummary(Object array, int[] dimensions) throws NotAnArrayException {
		super( dimensions );
		this.array = array ;
		typeName = RJavaArrayTools.getObjectTypeName(array );
		isprimitive = RJavaArrayTools.isPrimitiveTypeName( typeName ) ;
	}
	
	public RectangularArraySummary(Object array, int length ) throws NotAnArrayException{
		this( array, new int[]{ length } ); 
	}
	
	/** 
	 * Iterates over the array to find the minimum value
	 * (in the sense of the Comparable interface)
	 */
	public Object min( boolean narm ) throws NotComparableException {
		if( isprimitive ){
			return null ; // TODO :implement
		}
		checkComparableObjects() ;
		
		Object smallest = null ;
		Object current ;
		boolean found_min = false ;
		
		if( dimensions.length == 1 ){
			return( min( (Object[])array, narm ) ) ;
		} else{
			
			/* need to iterate */
			while( hasNext() ){
				current = min( (Object[])next(), narm ) ;
				if( current == null ){
					if( !narm ) return null ;
				} else{
					if( !found_min ){
						smallest = current ;
						found_min = true ;
					} else if( ((Comparable)smallest).compareTo(current) > 0 ) {
						smallest = current ;
					}					
				}
			}
			return smallest ;
		}
		
	}
	
	/**
	 * returns the minimum (in the sense of Comparable) of the 
	 * objects in the one dimensioned array
	 */ 
	private static Object min( Object[] x, boolean narm ){
		
		int n = x.length ;
		Object smallest = null ; 
		Object current ;
		boolean found_min = false; 
		
		// find somewhere to start from ()
		for( int i=0; i<n; i++){
			current = x[i] ;
			if( current == null ){
				if( !narm ) return null ;
			} else{
				if( !found_min ){
					smallest = current ;
					found_min = true ;
				} else if( ((Comparable)smallest).compareTo(current) > 0 ) {
					smallest = current ;
				}
			}
		}
		return smallest ; 
		
	}
	
	public void checkComparableObjects() throws NotComparableException {
		if( ! containsComparableObjects() ) throw new NotComparableException( typeName ) ;
	}
	
	public boolean containsComparableObjects(){
		Class cl ; 
		try{ 
			cl = RJavaArrayTools.getClassForSignature( typeName , array.getClass().getClassLoader() ) ;
		} catch( ClassNotFoundException e){
			return false ;
		}
		return Comparable.class.isAssignableFrom( cl ) ;
	}
	
	
}

