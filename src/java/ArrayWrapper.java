// :tabSize=2:indentSize=2:noTabs=false:folding=explicit:collapseFolds=1:

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
	 * total length - number of elements
	 */
	private int length ;
	
	/**
	 * is this array rectangular
	 */ 
	private boolean isRect ;
	
	/**
	 * The type name of the objects stored
	 */
	private String typeName ;
	
	/**
	 * true if the array stores primitive types
	 */
	private boolean primitive ;
	
	/**
	 * Constructor
	 *
	 * @param array the array to check
	 * @throws NotAnArrayException if array is not an array
	 */
	public ArrayWrapper(Object array) throws NotAnArrayException {
		this.array = array ;
		dimensions = RJavaArrayTools.getDimensions(array);
		typeName = RJavaArrayTools.getObjectTypeName(array );
		primitive = RJavaArrayTools.isPrimitiveTypeName( typeName ) ;
		if( dimensions.length == 1){
			isRect = true ;
		} else{
			isRect = isRectangular_( array, 0 );
		}
		// reset the dimensions if the array is not rectangular
		if( !isRect ){
			dimensions = null ;
			length = -1; 
		} else{
			length = 1; 
			for( int i=0; i<dimensions.length; i++) {
				length *= dimensions[i] ;
			}
		}
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
	
	/**
	 * @return the type name of the objects stored in the wrapped array
	 */
	public String getObjectTypeName(){
		return typeName; 
	}
	
	/** 
	 * @return true if the array contains java primitive types
	 */ 
	public boolean isPrimitive(){
		return primitive ; 
	}
	
	// used by all the flat_* methods below
	private int[] increment(int[] index){
		for( int i=index.length-1; i>=0; i--){
			if( (index[i] + 1) == dimensions[i] ){
				index[i] = 0 ; 
			} else{
				index[i] = index[i] + 1 ;
				return index; 
			}
		}
		return index; 
	}
	
	private int getStart( int[] index ){
		int start = 0;
		int product = 1 ; 
		for( int i=0; i<index.length; i++){
			start += index[i]*product;
			product = dimensions[i]*product ;
		}
		return start ;
	}
	
	// {{{ flat_int
	/**
	 * Flattens the array into a single dimensionned int array
	 * 
	 */ 
	public int[] flat_int() throws PrimitiveArrayException,FlatException {
		
		if( ! "I".equals(typeName) ) throw new PrimitiveArrayException("int"); 
		if( !isRect ) throw new FlatException(); 
		if( dimensions.length == 1 ){
			return (int[])array ;
		} else{
			int[] payload_int = new int[length] ;
			int i = 0, k ;
			int depth = dimensions.length - 1 ;
			int increment = 1 ;
			for( i=0; i<depth; i++){
				increment *= dimensions[i] ;
			}
			
			int[] index = new int[ depth ]; 
			// init
			for( i=0; i< depth; i++){
				index[i] = 0 ; 
			}
			
			i=0;
			while( i<increment ){
				int[] current = getIntArray( index ) ;
				k = getStart(index) ;
				for( int j=0; j<current.length; j++, k+=increment){
					payload_int[k] = current[j];
				}
				index = increment( index ) ;
				i++; 
			}
			return payload_int; 
		}
	}
	
	private int[] getIntArray( int[] index ){
		int[] res ;
		Object o = array ;
		for( int i=0; i<index.length; i++){
			o = Array.get( o, index[i] ) ;
		}
		return (int[]) o; 
	}
	// }}}
	
	// {{{ flat_boolean
	/**
	 * Flattens the array into a single dimensionned boolean array
	 * 
	 */ 
	public boolean[] flat_boolean() throws PrimitiveArrayException,FlatException {
		
		if( ! "Z".equals(typeName) ) throw new PrimitiveArrayException("boolean"); 
		if( !isRect ) throw new FlatException(); 
		if( dimensions.length == 1 ){
			return (boolean[])array ;
		} else{
			boolean[] payload_boolean = new boolean[length] ;
			int i = 0, k ;
			int depth = dimensions.length - 1 ;
			int increment = 1 ;
			for( i=0; i<depth; i++){
				increment *= dimensions[i] ;
			}
			
			int[] index = new int[ depth ]; 
			// init
			for( i=0; i< depth; i++){
				index[i] = 0 ; 
			}
			
			i=0;
			while( i<increment ){
				boolean[] current = getBooleanArray( index ) ;
				k = getStart(index) ;
				for( int j=0; j<current.length; j++, k+=increment){
					payload_boolean[k] = current[j];
				}
				index = increment( index ) ;
				i++; 
			}
			return payload_boolean ; 
		}
	}
	
	private boolean[] getBooleanArray( int[] index ){
		int[] res ;
		Object o = array ;
		for( int i=0; i<index.length; i++){
			o = Array.get( o, index[i] ) ;
		}
		return (boolean[]) o; 
	}
	// }}}
	
	// {{{ flat_byte
	/**
	 * Flattens the array into a single dimensionned byte array
	 * 
	 */ 
	public byte[] flat_byte() throws PrimitiveArrayException,FlatException {
		
		if( ! "B".equals(typeName) ) throw new PrimitiveArrayException("byte"); 
		if( !isRect ) throw new FlatException(); 
		if( dimensions.length == 1 ){
			return (byte[])array ;
		} else{
			byte[] payload_byte = new byte[length] ;
			int i = 0, k ;
			int depth = dimensions.length - 1 ;
			int increment = 1 ;
			for( i=0; i<depth; i++){
				increment *= dimensions[i] ;
			}
			
			int[] index = new int[ depth ]; 
			// init
			for( i=0; i< depth; i++){
				index[i] = 0 ; 
			}
			
			i=0;
			while( i<increment ){
				byte[] current = getByteArray( index ) ;
				k = getStart(index) ;
				for( int j=0; j<current.length; j++, k+=increment){
					payload_byte[k] = current[j];
				}
				index = increment( index ) ;
				i++; 
			}
			return payload_byte; 
		}
	}

	private byte[] getByteArray( int[] index ){
		int[] res ;
		Object o = array ;
		for( int i=0; i<index.length; i++){
			o = Array.get( o, index[i] ) ;
		}
		return (byte[]) o; 
	}
	// }}}
	
	// {{{ flat_long
	/**
	 * Flattens the array into a single dimensionned long array
	 * 
	 */ 
	public long[] flat_long() throws PrimitiveArrayException,FlatException {
		
		if( ! "J".equals(typeName) ) throw new PrimitiveArrayException("long"); 
		if( !isRect ) throw new FlatException(); 
		if( dimensions.length == 1 ){
			return (long[])array ;
		} else{
			long[] payload_long = new long[length] ;
			int i = 0, k ;
			int depth = dimensions.length - 1 ;
			int increment = 1 ;
			for( i=0; i<depth; i++){
				increment *= dimensions[i] ;
			}
			
			int[] index = new int[ depth ]; 
			// init
			for( i=0; i< depth; i++){
				index[i] = 0 ; 
			}
			
			i=0;
			while( i<increment ){
				long[] current = getLongArray( index ) ;
				k = getStart(index) ;
				for( int j=0; j<current.length; j++, k+=increment){
					payload_long[k] = current[j];
				}
				index = increment( index ) ;
				i++; 
			}
			return payload_long ; 
		}
	}
	
	private long[] getLongArray( int[] index ){
		int[] res ;
		Object o = array ;
		for( int i=0; i<index.length; i++){
			o = Array.get( o, index[i] ) ;
		}
		return (long[]) o; 
	}
	// }}}
	
	// {{{ flat_short
	/**
	 * Flattens the array into a single dimensionned short array
	 * 
	 */ 
		public short[] flat_short() throws PrimitiveArrayException,FlatException {
		
		if( ! "S".equals(typeName) ) throw new PrimitiveArrayException("short"); 
		if( !isRect ) throw new FlatException(); 
		if( dimensions.length == 1 ){
			return (short[])array ;
		} else{
			short[] payload_short = new short[length] ;
			int i = 0, k ;
			int depth = dimensions.length - 1 ;
			int increment = 1 ;
			for( i=0; i<depth; i++){
				increment *= dimensions[i] ;
			}
			
			int[] index = new int[ depth ]; 
			// init
			for( i=0; i< depth; i++){
				index[i] = 0 ; 
			}
			
			i=0;
			while( i<increment ){
				short[] current = getShortArray( index ) ;
				k = getStart(index) ;
				for( int j=0; j<current.length; j++, k+=increment){
					payload_short[k] = current[j];
				}
				index = increment( index ) ;
				i++; 
			}
			return payload_short; 
		}
	}

	private short[] getShortArray( int[] index ){
		int[] res ;
		Object o = array ;
		for( int i=0; i<index.length; i++){
			o = Array.get( o, index[i] ) ;
		}
		return (short[]) o; 
	}
	// }}}

	// {{{ flat_double
	/**
	 * Flattens the array into a single dimensionned double array
	 * 
	 */ 
	public double[] flat_double() throws PrimitiveArrayException,FlatException {
		
		if( ! "D".equals(typeName) ) throw new PrimitiveArrayException("double"); 
		if( !isRect ) throw new FlatException(); 
		if( dimensions.length == 1 ){
			return (double[])array ;
		} else{
			double[] payload_double = new double[length] ;
			int i = 0, k ;
			int depth = dimensions.length - 1 ;
			int increment = 1 ;
			for( i=0; i<depth; i++){
				increment *= dimensions[i] ;
			}
			
			int[] index = new int[ depth ]; 
			// init
			for( i=0; i< depth; i++){
				index[i] = 0 ; 
			}
			
			i=0;
			while( i<increment ){
				double[] current = getDoubleArray( index ) ;
				k = getStart(index) ;
				for( int j=0; j<current.length; j++, k+=increment){
					payload_double[k] = current[j];
				}
				index = increment( index ) ;
				i++; 
			}
			return payload_double; 
		}
	}

	private double[] getDoubleArray( int[] index ){
		int[] res ;
		Object o = array ;
		for( int i=0; i<index.length; i++){
			o = Array.get( o, index[i] ) ;
		}
		return (double[]) o; 
	}
	// }}}
	
	// {{{ flat_char
	/**
	 * Flattens the array into a single dimensionned double array
	 * 
	 */ 
	public char[] flat_char() throws PrimitiveArrayException,FlatException {
		
		if( ! "C".equals(typeName) ) throw new PrimitiveArrayException("char"); 
		if( !isRect ) throw new FlatException(); 
		if( dimensions.length == 1 ){
			return (char[])array ;
		} else{
			char[] payload_char = new char[length] ;
			int i = 0, k ;
			int depth = dimensions.length - 1 ;
			int increment = 1 ;
			for( i=0; i<depth; i++){
				increment *= dimensions[i] ;
			}
			
			int[] index = new int[ depth ]; 
			// init
			for( i=0; i< depth; i++){
				index[i] = 0 ; 
			}
			
			i=0;
			while( i<increment ){
				char[] current = getCharArray( index ) ;
				k = getStart(index) ;
				for( int j=0; j<current.length; j++, k+=increment){
					payload_char[k] = current[j];
				}
				index = increment( index ) ;
				i++; 
			}
			return payload_char ; 
		}
	}
	
	private char[] getCharArray( int[] index ){
		int[] res ;
		Object o = array ;
		for( int i=0; i<index.length; i++){
			o = Array.get( o, index[i] ) ;
		}
		return (char[]) o; 
	}
	// }}}

	// {{{ flat_float
	/**
	 * Flattens the array into a single dimensionned float array
	 * 
	 */ 
	public float[] flat_float() throws PrimitiveArrayException,FlatException {
		
		if( ! "F".equals(typeName) ) throw new PrimitiveArrayException("float"); 
		if( !isRect ) throw new FlatException(); 
		if( dimensions.length == 1 ){
			return (float[])array ;
		} else{
			float[] payload_float = new float[length] ;
			int i = 0, k ;
			int depth = dimensions.length - 1 ;
			int increment = 1 ;
			for( i=0; i<depth; i++){
				increment *= dimensions[i] ;
			}
			
			int[] index = new int[ depth ]; 
			// init
			for( i=0; i< depth; i++){
				index[i] = 0 ; 
			}
			
			i=0;
			while( i<increment ){
				float[] current = getFloatArray( index ) ;
				k = getStart(index) ;
				for( int j=0; j<current.length; j++, k+=increment){
					payload_float[k] = current[j];
				}
				index = increment( index ) ;
				i++; 
			}
			return payload_float; 
		}
	}
	
	private float[] getFloatArray( int[] index ){
		int[] res ;
		Object o = array ;
		for( int i=0; i<index.length; i++){
			o = Array.get( o, index[i] ) ;
		}
		return (float[]) o; 
	}
	// }}}
	
	// {{{ flat_Object
	public Object flat_Object() throws FlatException, ObjectArrayException {
		if( isPrimitive() ) throw new ObjectArrayException( typeName) ; 
		if( !isRect ) throw new FlatException(); 
		if( dimensions.length == 1 ){
			return array ;
		} else{
			ClassLoader loader = array.getClass().getClassLoader() ;
			Class type = Object.class; 
			try{
				type = Class.forName( typeName, true, array.getClass().getClassLoader() );
			} catch( ClassNotFoundException e){}
			
			Object res = Array.newInstance( type,  length ) ; 
			int i = 0, k ;
			int depth = dimensions.length - 1 ;
			int increment = 1 ;
			for( i=0; i<depth; i++){
				increment *= dimensions[i] ;
			}
			
			int[] index = new int[ depth ]; 
			// init
			for( i=0; i< depth; i++){
				index[i] = 0 ; 
			}
			
			i=0;
			while( i<increment ){
				Object[] current = getObjectArray( index ) ;
				k = getStart(index) ;
				for( int j=0; j<current.length; j++, k+=increment){
					Array.set( res, k, type.cast( current[j] ) );
				}
				index = increment( index ) ;
				i++; 
			}
			return res ; 
		}
	}
	
	private Object[] getObjectArray( int[] index ){
		int[] res ;
		Object o = array ;
		for( int i=0; i<index.length; i++){
			o = Array.get( o, index[i] ) ;
		}
		return (Object[]) o; 
	}
	// }}}
	
	// {{{ flat_String
	/**
	 * Flattens the array into a single dimensionned String array
	 * 
	 */ 
	// this is technically not required as this can be handled
	// by flat_Object but this is slightly more efficient so ...
	public String[] flat_String() throws PrimitiveArrayException,FlatException {
		
		if( ! "java.lang.String".equals(typeName) ) throw new PrimitiveArrayException("java.lang.String"); 
		if( !isRect ) throw new FlatException(); 
		if( dimensions.length == 1 ){
			return (String[])array ;
		} else{
			String[] payload_String = new String[length] ;
			int i = 0, k ;
			int depth = dimensions.length - 1 ;
			int increment = 1 ;
			for( i=0; i<depth; i++){
				increment *= dimensions[i] ;
			}
			
			int[] index = new int[ depth ]; 
			// init
			for( i=0; i< depth; i++){
				index[i] = 0 ; 
			}
			
			i=0;
			while( i<increment ){
				String[] current = getStringArray( index ) ;
				k = getStart(index) ;
				for( int j=0; j<current.length; j++, k+=increment){
					payload_String[k] = current[j];
				}
				index = increment( index ) ;
				i++; 
			}
			return payload_String; 
		}
	}
	
	private String[] getStringArray( int[] index ){
		int[] res ;
		Object o = array ;
		for( int i=0; i<index.length; i++){
			o = Array.get( o, index[i] ) ;
		}
		return (String[]) o; 
	}
	// }}}
	
}

