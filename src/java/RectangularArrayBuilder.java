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
		
			String typeName = RJavaArrayTools.getObjectTypeName( payload ); 
			String sig = RJavaArrayTools.makeArraySignature( typeName, dim.length ) ; 
			Class clazz = null ;
			try{
				clazz = Class.forName( sig, true, payload.getClass().getClassLoader() ) ; 
			} catch( ClassNotFoundException e){/* should not happen*/}
			
			array = Array.newInstance( clazz , dim ) ;
		  if( typeName.equals( "I" ) ){
		  	fill_int( (int[])payload ) ;
		  } // TODO : the othrs
			
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
	
	private void fill_int( int[] payload ){
		
	}
	
}

