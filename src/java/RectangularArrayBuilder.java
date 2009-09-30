// :tabSize=2:indentSize=2:noTabs=false:folding=explicit:collapseFolds=1:

import java.lang.reflect.Array ; 

/**
 * Builds rectangular java arrays
 */
public class RectangularArrayBuilder {

	private Object array ; 
	
	/**
	 * Return the built multi dim array
	 */
	public Object getArray(){
		return array ;
	}
	
	private int[] dim; 
	
	private int[] index ;
	private int increment; 
	
	// {{{ constructors
	/**
	 * constructor
	 *
	 * @param payload one dimensional array
	 * @param dim target dimensions
	 * @throws NotAnArrayException if payload is not an array
	 */
	public RectangularArrayBuilder( Object payload, int[] dim) throws NotAnArrayException, ArrayDimensionException {
		
		if( !RJavaArrayTools.isArray(payload) ){
			throw new NotAnArrayException( payload.getClass() ) ;
		}
		if( !RJavaArrayTools.isSingleDimensionArray(payload)){
			throw new ArrayDimensionException( "not a single dimension array : " + payload.getClass() ) ;
		}
		this.dim = dim; 
		
		if( dim.length == 1 ){
			array = payload ;
		} else{
			
			int n = dim.length - 1 ;
			index = new int[n]; 
			for( int i=0; i<n; i++) index[i]=0;
			
			increment = 1 ;
			for( int i=0; i<n; i++){
				increment *= dim[i] ;
			}
		
			String typeName = RJavaArrayTools.getObjectTypeName( payload ); 
			Class clazz = null ;
			try{
				clazz = RJavaArrayTools.getClassForSignature( typeName, payload.getClass().getClassLoader() );  
			} catch( ClassNotFoundException e){/* should not happen */}
			
			array = Array.newInstance( clazz , dim ) ;
		  if( typeName.equals( "I" ) ){
		  	fill_int( (int[])payload ) ;
		  } else if( typeName.equals( "Z" ) ){
		  	fill_boolean( (boolean[])payload ) ;
		  } else if( typeName.equals( "B" ) ){
		  	fill_byte( (byte[])payload ) ;
		  } else if( typeName.equals( "J" ) ){
		  	fill_long( (long[])payload ) ;
		  } else if( typeName.equals( "S" ) ){
		  	fill_short( (short[])payload ) ;
		  } else if( typeName.equals( "D" ) ){
		  	fill_double( (double[])payload ) ;
		  } else if( typeName.equals( "C" ) ){
		  	fill_char( (char[])payload ) ;
		  } else if( typeName.equals( "F" ) ){
		  	fill_float( (float[])payload ) ;
		  } else{
		  	fill_Object( (Object[])payload ) ;
		  }
			
		}
	}
	public RectangularArrayBuilder( Object payload, int length ) throws NotAnArrayException, ArrayDimensionException{
		this( payload, new int[]{ length } ) ;
	}

	// java < 1.5 kept happy
	public RectangularArrayBuilder(int x    , int[] dim ) throws NotAnArrayException { throw new NotAnArrayException("primitive type : int     ") ; }
	public RectangularArrayBuilder(boolean x, int[] dim ) throws NotAnArrayException { throw new NotAnArrayException("primitive type : boolean ") ; }
	public RectangularArrayBuilder(byte x   , int[] dim ) throws NotAnArrayException { throw new NotAnArrayException("primitive type : byte    ") ; }
	public RectangularArrayBuilder(long x   , int[] dim ) throws NotAnArrayException { throw new NotAnArrayException("primitive type : long    ") ; }
	public RectangularArrayBuilder(short x  , int[] dim ) throws NotAnArrayException { throw new NotAnArrayException("primitive type : short   ") ; }
	public RectangularArrayBuilder(double x , int[] dim ) throws NotAnArrayException { throw new NotAnArrayException("primitive type : double  ") ; }
	public RectangularArrayBuilder(char x   , int[] dim ) throws NotAnArrayException { throw new NotAnArrayException("primitive type : char    ") ; }
	public RectangularArrayBuilder(float x  , int[] dim ) throws NotAnArrayException { throw new NotAnArrayException("primitive type : float   ") ; }

	public RectangularArrayBuilder(int x    , int length ) throws NotAnArrayException { throw new NotAnArrayException("primitive type : int     ") ; }
	public RectangularArrayBuilder(boolean x, int length ) throws NotAnArrayException { throw new NotAnArrayException("primitive type : boolean ") ; }
	public RectangularArrayBuilder(byte x   , int length ) throws NotAnArrayException { throw new NotAnArrayException("primitive type : byte    ") ; }
	public RectangularArrayBuilder(long x   , int length ) throws NotAnArrayException { throw new NotAnArrayException("primitive type : long    ") ; }
	public RectangularArrayBuilder(short x  , int length ) throws NotAnArrayException { throw new NotAnArrayException("primitive type : short   ") ; }
	public RectangularArrayBuilder(double x , int length ) throws NotAnArrayException { throw new NotAnArrayException("primitive type : double  ") ; }
	public RectangularArrayBuilder(char x   , int length ) throws NotAnArrayException { throw new NotAnArrayException("primitive type : char    ") ; }
	public RectangularArrayBuilder(float x  , int length ) throws NotAnArrayException { throw new NotAnArrayException("primitive type : float   ") ; }
	// }}}
	
	// {{{ fill_**
	private void fill_int( int[] payload ){
		int i, k; 
			
		i=0;
		while( i<increment ){
			int[] current = (int[])getObjectArray( index ) ;
			k = getStart(index) ;
			for( int j=0; j<current.length; j++, k+=increment){
				current[j] = payload[k];
			}
			index = increment( index ) ;
			i++; 
		}
	}
	
	private void fill_boolean( boolean[] payload ){
		int i, k; 
			
		i=0;
		while( i<increment ){
			boolean[] current = (boolean[])getObjectArray( index ) ;
			k = getStart(index) ;
			for( int j=0; j<current.length; j++, k+=increment){
				current[j] = payload[k];
			}
			index = increment( index ) ;
			i++; 
		}
	}
	
	private void fill_byte( byte[] payload ){
		int i, k; 
			
		i=0;
		while( i<increment ){
			byte[] current = (byte[])getObjectArray( index ) ;
			k = getStart(index) ;
			for( int j=0; j<current.length; j++, k+=increment){
				current[j] = payload[k];
			}
			index = increment( index ) ;
			i++; 
		}
	}
	
	private void fill_long( long[] payload ){
		int i, k; 
			
		i=0;
		while( i<increment ){
			long[] current = (long[])getObjectArray( index ) ;
			k = getStart(index) ;
			for( int j=0; j<current.length; j++, k+=increment){
				current[j] = payload[k];
			}
			index = increment( index ) ;
			i++; 
		}
	}

	private void fill_short( short[] payload ){
		int i, k; 
			
		i=0;
		while( i<increment ){
			short[] current = (short[])getObjectArray( index ) ;
			k = getStart(index) ;
			for( int j=0; j<current.length; j++, k+=increment){
				current[j] = payload[k];
			}
			index = increment( index ) ;
			i++; 
		}
	}

	private void fill_double( double[] payload ){
		int i, k; 
			
		i=0;
		while( i<increment ){
			double[] current = (double[])getObjectArray( index ) ;
			k = getStart(index) ;
			for( int j=0; j<current.length; j++, k+=increment){
				current[j] = payload[k];
			}
			index = increment( index ) ;
			i++; 
		}
	}

	private void fill_char( char[] payload ){
		int i, k; 
			
		i=0;
		while( i<increment ){
			char[] current = (char[])getObjectArray( index ) ;
			k = getStart(index) ;
			for( int j=0; j<current.length; j++, k+=increment){
				current[j] = payload[k];
			}
			index = increment( index ) ;
			i++; 
		}
	}

	private void fill_float( float[] payload ){
		int i, k; 
			
		i=0;
		while( i<increment ){
			float[] current = (float[])getObjectArray( index ) ;
			k = getStart(index) ;
			for( int j=0; j<current.length; j++, k+=increment){
				current[j] = payload[k];
			}
			index = increment( index ) ;
			i++; 
		}
	}
	
	private void fill_Object( Object[] payload ){
		int i, k; 
			
		i=0;
		while( i<increment ){
			Object[] current = (Object[])getObjectArray( index ) ;
			k = getStart(index) ;
			for( int j=0; j<current.length; j++, k+=increment){
				current[j] = payload[k];
			}
			index = increment( index ) ;
			i++; 
		}
	}

	// }}}
	
	/* all below is the same as in the ArrayWrapper class */
	
	private int getStart( int[] index ){
		int start = 0;
		int product = 1 ; 
		for( int i=0; i<index.length; i++){
			start += index[i]*product;
			product = dim[i]*product ;
		}
		return start ;
	}

	private int[] increment(int[] index){
		for( int i=index.length-1; i>=0; i--){
			if( (index[i] + 1) == dim[i] ){
				index[i] = 0 ; 
			} else{
				index[i] = index[i] + 1 ;
				return index; 
			}
		}
		return index; 
	}
	
	private Object getObjectArray( int[] index ){
		int[] res ;
		Object o = array ;
		for( int i=0; i<index.length; i++){
			o = Array.get( o, index[i] ) ;
		}
		return o; 
	}

}

