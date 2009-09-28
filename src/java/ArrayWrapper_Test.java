// :tabSize=2:indentSize=2:noTabs=false:folding=explicit:collapseFolds=1:

import java.awt.Point; 

/**
 * Test suite for ArrayWrapper 
 */
public class ArrayWrapper_Test {

	// {{{ main
	public static void main(String[] args ){
		try{
			runtests() ;
		} catch( TestException e){
			e.printStackTrace(); 
			System.exit(1); 
		}
		System.out.println( "\nALL PASSED\n" ) ; 
		System.exit( 0 ); 
	}
	// }}}
	
	// {{{ runtests
	public static void runtests() throws TestException {
		
		// {{{ multi dim array of primitives 
		
		// {{{ flat_int
		System.out.println( "flatten int[]" ); 
		flatten_int_1(); 
		System.out.println( "PASSED" ); 
		
		System.out.println( "flatten int[][]" ); 
		flatten_int_2(); 
		System.out.println( "PASSED" ); 
		
		System.out.println( "flatten int[][][]" ); 
		flatten_int_3(); 
		System.out.println( "PASSED" );
		// }}}
		
		// {{{ flat_boolean
		System.out.println( "flatten boolean[]" ); 
		flatten_boolean_1(); 
		System.out.println( "PASSED" ); 
		
		System.out.println( "flatten boolean[][]" ); 
		flatten_boolean_2(); 
		System.out.println( "PASSED" ); 
		
		System.out.println( "flatten boolean[][][]" ); 
		flatten_boolean_3(); 
		System.out.println( "PASSED" );
		// }}}
		
		// {{{ flat_byte
		System.out.println( "flatten byte[]" ); 
		flatten_byte_1(); 
		System.out.println( "PASSED" ); 
		
		System.out.println( "flatten byte[][]" ); 
		flatten_byte_2(); 
		System.out.println( "PASSED" ); 
		
		System.out.println( "flatten byte[][][]" ); 
		flatten_byte_3(); 
		System.out.println( "PASSED" );
		// }}}
		
		// {{{ flat_long
		System.out.println( "flatten long[]" ); 
		flatten_long_1(); 
		System.out.println( "PASSED" ); 
		
		System.out.println( "flatten long[][]" ); 
		flatten_long_2(); 
		System.out.println( "PASSED" ); 
		
		System.out.println( "flatten long[][][]" ); 
		flatten_long_3(); 
		System.out.println( "PASSED" );
		// }}}
		
		// {{{ flat_long
		System.out.println( "flatten short[]" ); 
		flatten_short_1(); 
		System.out.println( "PASSED" ); 
		
		System.out.println( "flatten short[][]" ); 
		flatten_short_2(); 
		System.out.println( "PASSED" ); 
		
		System.out.println( "flatten short[][][]" ); 
		flatten_short_3(); 
		System.out.println( "PASSED" );
		// }}}

		// {{{ flat_double
		System.out.println( "flatten double[]" ); 
		flatten_double_1(); 
		System.out.println( "PASSED" ); 
		
		System.out.println( "flatten double[][]" ); 
		flatten_double_2(); 
		System.out.println( "PASSED" ); 
		
		System.out.println( "flatten double[][][]" ); 
		flatten_double_3(); 
		System.out.println( "PASSED" );
		// }}}

		// {{{ flat_char
		System.out.println( "flatten char[]" ); 
		flatten_char_1(); 
		System.out.println( "PASSED" ); 
		
		System.out.println( "flatten char[][]" ); 
		flatten_char_2(); 
		System.out.println( "PASSED" ); 
		
		System.out.println( "flatten char[][][]" ); 
		flatten_char_3(); 
		System.out.println( "PASSED" );
		// }}}
		
		// {{{ flat_float
		System.out.println( "flatten float[]" ); 
		flatten_float_1(); 
		System.out.println( "PASSED" ); 
		
		System.out.println( "flatten float[][]" ); 
		flatten_float_2(); 
		System.out.println( "PASSED" ); 
		
		System.out.println( "flatten float[][][]" ); 
		flatten_float_3(); 
		System.out.println( "PASSED" );
		// }}}
		// }}}
		
		// {{{ multi dim array of Object
		// {{{ flat_String
		System.out.println( "flatten String[]" ); 
		flatten_String_1(); 
		System.out.println( "PASSED" ); 
		
		System.out.println( "flatten String[][]" ); 
		flatten_String_2(); 
		System.out.println( "PASSED" ); 
		
		System.out.println( "flatten String[][][]" ); 
		flatten_String_3(); 
		System.out.println( "PASSED" );
		// }}}
		
			// {{{ flat_String
		System.out.println( "flatten Point[]" ); 
		flatten_Point_1(); 
		System.out.println( "PASSED" ); 
		
		System.out.println( "flatten Point[][]" ); 
		flatten_Point_2(); 
		System.out.println( "PASSED" ); 
		
		System.out.println( "flatten Point[][][]" ); 
		flatten_Point_3(); 
		System.out.println( "PASSED" );
		// }}}
	
		// }}}
	}
	//}}}
	
	// {{{ flat multi dimen array of java primitives
	
	// {{{ flatten_int_1
	private static void flatten_int_1() throws TestException{
		
		int[] o = new int[5] ;
		for( int i=0;i<5;i++) o[i] = i ;
		
		ArrayWrapper wrapper = null ; 
		System.out.print( "  >> new ArrayWrapper( int[] ) " ); 
		try{
			wrapper = new ArrayWrapper(o); 
		} catch( NotAnArrayException e){
			throw new TestException("new ArrayWrapper(int[]) >> NotAnArrayException ") ; 
		} 
		System.out.println( "ok"); 
		
		System.out.print( "  >> isPrimitive()" ) ;
		if( !wrapper.isPrimitive() ){
			throw new TestException( "ArrayWrapper(int[]) not primitive" ) ; 
		}
		System.out.println( " true : ok" ); 
		
		System.out.print( "  >> getObjectTypeName()" ) ;
		if( !wrapper.getObjectTypeName().equals("I") ){
			throw new TestException( "ArrayWrapper(int[]).getObjectTypeName() != 'I'" ) ;
		}
		System.out.println( " I : ok" ); 
		
		System.out.print( "  >> flat_int()" ) ;
		int[] flat;
		try{
			flat = wrapper.flat_int() ; 
		} catch( PrimitiveArrayException e){
			throw new TestException( "PrimitiveArrayException" ) ;
		} catch( FlatException e){
			throw new TestException("new ArrayWrapper(int[]) >> FlatException") ;
		}
		
		for( int i=0; i<5; i++){
			if( flat[i] != i ) throw new TestException( "flat[" + i + "] = " + flat [i] + "!=" + i); 
		}
		System.out.println( "  ok" ) ;
		
	}
	// }}}
	
	// {{{ flatten_int_2
	private static void flatten_int_2() throws TestException{
		
		int[][] o = new int[2][5] ;
		int k = 0; 
		for( int i=0;i<5;i++,k++) o[0][i] = k ;
		for( int i=0;i<5;i++,k++) o[1][i] = k ;
		
		ArrayWrapper wrapper = null ; 
		System.out.print( "  >> new ArrayWrapper( int[][] ) " ); 
		try{
			wrapper = new ArrayWrapper(o); 
		} catch( NotAnArrayException e){
			throw new TestException("new ArrayWrapper(int[][]) >> NotAnArrayException ") ; 
		} 
		System.out.println( "ok"); 
		
		System.out.print( "  >> isPrimitive()" ) ;
		if( !wrapper.isPrimitive() ){
			throw new TestException( "ArrayWrapper(int[][]) not primitive" ) ; 
		}
		System.out.println( " true : ok" ); 
		
		System.out.print( "  >> getObjectTypeName()" ) ;
		if( !wrapper.getObjectTypeName().equals("I") ){
			throw new TestException( "ArrayWrapper(int[][]).getObjectTypeName() != 'I'" ) ;
		}
		System.out.println( " I : ok" ); 
		
		System.out.print( "  >> flat_int()" ) ;
		int[] flat;
		try{
			flat = wrapper.flat_int() ; 
		} catch( PrimitiveArrayException e){
			throw new TestException( "PrimitiveArrayException" ) ;
		} catch( FlatException e){
			throw new TestException("new ArrayWrapper(int[][]) >> FlatException") ;
		}
		
		
		for( int i=0; i<10; i++){
			if( flat[i] != i ) throw new TestException( "flat[" + i + "] = " + flat [i] + "!=" + i); 
		}
		System.out.println( "  ok" ) ;
		
	}
	// }}}
	
  // {{{ flatten_int_3
	private static void flatten_int_3() throws TestException{
		
		int[][][] o = new int[2][2][5] ;
		int k = 0;
		for( int i=0;i<2;i++){
			for( int j=0; j<2; j++){
				for( int jjj=0; jjj<5; jjj++,k++){
					o[i][j][jjj] = k ; 
				}
			}
		}
		
		ArrayWrapper wrapper = null ; 
		System.out.print( "  >> new ArrayWrapper( int[][][] ) " ); 
		try{
			wrapper = new ArrayWrapper(o); 
		} catch( NotAnArrayException e){
			throw new TestException("new ArrayWrapper(int[][][]) >> NotAnArrayException ") ; 
		}
		System.out.println( "ok"); 
		
		System.out.print( "  >> isPrimitive()" ) ;
		if( !wrapper.isPrimitive() ){
			throw new TestException( "ArrayWrapper(int[][][]) not primitive" ) ; 
		}
		System.out.println( " true : ok" ); 
		
		System.out.print( "  >> getObjectTypeName()" ) ;
		if( !wrapper.getObjectTypeName().equals("I") ){
			throw new TestException( "ArrayWrapper(int[][][]).getObjectTypeName() != 'I'" ) ;
		}
		System.out.println( " I : ok" ); 
		
		System.out.print( "  >> flat_int()" ) ;
		int[] flat;
		try{
			flat = wrapper.flat_int() ; 
		} catch( PrimitiveArrayException e){
			throw new TestException( "PrimitiveArrayException" ) ;
		} catch( FlatException e){
			throw new TestException("new ArrayWrapper(int[][][]) >> FlatException") ;
		}
		
		
		for( int i=0; i<20; i++){
			if( flat[i] != i ) throw new TestException( "flat[" + i + "] = " + flat [i] + "!=" + i); 
		}
		System.out.println( "  ok" ) ;
		
	}
	// }}}

	
	
		
	// {{{ flatten_boolean_1
	private static void flatten_boolean_1() throws TestException{
		
		boolean[] o = new boolean[5] ; 
		boolean current = false; 
		for( int i=0;i<5;i++){
			o[i] = current ;
			current = !current ;
		}
		
		ArrayWrapper wrapper = null ; 
		System.out.print( "  >> new ArrayWrapper( boolean[] ) " ); 
		try{
			wrapper = new ArrayWrapper(o); 
		} catch( NotAnArrayException e){
			throw new TestException("new ArrayWrapper(boolean[]) >> NotAnArrayException ") ; 
		} 
		System.out.println( "ok"); 
		
		System.out.print( "  >> isPrimitive()" ) ;
		if( !wrapper.isPrimitive() ){
			throw new TestException( "ArrayWrapper(boolean[]) not primitive" ) ; 
		}
		System.out.println( " true : ok" ); 
		
		System.out.print( "  >> getObjectTypeName()" ) ;
		if( !wrapper.getObjectTypeName().equals("Z") ){
			throw new TestException( "ArrayWrapper(boolean[]).getObjectTypeName() != 'Z'" ) ;
		}
		System.out.println( " Z : ok" ); 
		
		System.out.print( "  >> flat_boolean()" ) ;
		boolean[] flat;
		try{
			flat = wrapper.flat_boolean() ; 
		} catch( PrimitiveArrayException e){
			throw new TestException( "PrimitiveArrayException" ) ;
		} catch( FlatException e){
			throw new TestException("new ArrayWrapper(int[]) >> FlatException") ;
		}
		
		current = false ;
		for( int i=0; i<5; i++){
			if( flat[i] != current ) throw new TestException( "flat[" + i + "] = " + flat [i] );
			current = !current ;
		}
		System.out.println( "  ok" ) ;
		
	}
	// }}}

	// {{{ flatten_boolean_2
	private static void flatten_boolean_2() throws TestException{
		
		boolean[][] o = new boolean[2][5] ;
		boolean current = false; 
		for( int i=0; i<2; i++){
			for( int j=0; j<5; j++){
				o[i][j] = current ; 
				current = !current ;
			}
		}
			
		ArrayWrapper wrapper = null ; 
		System.out.print( "  >> new ArrayWrapper( boolean[][] ) " ); 
		try{
			wrapper = new ArrayWrapper(o); 
		} catch( NotAnArrayException e){
			throw new TestException("new ArrayWrapper(boolean[][]) >> NotAnArrayException ") ; 
		} 
		System.out.println( "ok"); 
		
		System.out.print( "  >> isPrimitive()" ) ;
		if( !wrapper.isPrimitive() ){
			throw new TestException( "ArrayWrapper(boolean[][]) not primitive" ) ; 
		}
		System.out.println( " true : ok" ); 
		
		System.out.print( "  >> getObjectTypeName()" ) ;
		if( !wrapper.getObjectTypeName().equals("Z") ){
			throw new TestException( "ArrayWrapper(boolean[][]).getObjectTypeName() != 'Z'" ) ;
		}
		System.out.println( " Z : ok" ); 
		
		System.out.print( "  >> flat_boolean()" ) ;
		boolean[] flat;
		try{
			flat = wrapper.flat_boolean() ; 
		} catch( PrimitiveArrayException e){
			throw new TestException( "PrimitiveArrayException" ) ;
		} catch( FlatException e){
			throw new TestException("new ArrayWrapper(boolean[][]) >> FlatException") ;
		}
		
		current = false ;
		for( int i=0; i<5; i++){
			if( flat[i] != current ) throw new TestException( "flat[" + i + "] = " + flat [i] );
			current = !current ;
		}
		System.out.println( "  ok" ) ;
		
	}
	// }}}
	
  // {{{ flatten_boolean_3
	private static void flatten_boolean_3() throws TestException{
		
		boolean[][][] o = new boolean[2][3][5] ;
		boolean current = false ;
		for( int i=0;i<2;i++){
			for( int j=0; j<2; j++){
				for( int k=0; k<5; k++){
					o[i][j][k] = current ;
					current = !current ;
				}
			}
		}
		
		ArrayWrapper wrapper = null ; 
		System.out.print( "  >> new ArrayWrapper( boolean[][][] ) " ); 
		try{
			wrapper = new ArrayWrapper(o); 
		} catch( NotAnArrayException e){
			throw new TestException("new ArrayWrapper(boolean[][][]) >> NotAnArrayException ") ; 
		}
		System.out.println( "ok"); 
		
		System.out.print( "  >> isPrimitive()" ) ;
		if( !wrapper.isPrimitive() ){
			throw new TestException( "ArrayWrapper(boolean[][][]) not primitive" ) ; 
		}
		System.out.println( " true : ok" ); 
		
		System.out.print( "  >> getObjectTypeName()" ) ;
		if( !wrapper.getObjectTypeName().equals("Z") ){
			throw new TestException( "ArrayWrapper(int[][][]).getObjectTypeName() != 'Z'" ) ;
		}
		System.out.println( " Z : ok" ); 
		
		System.out.print( "  >> flat_boolean()" ) ;
		boolean[] flat;
		try{
			flat = wrapper.flat_boolean() ; 
		} catch( PrimitiveArrayException e){
			throw new TestException( "PrimitiveArrayException" ) ;
		} catch( FlatException e){
			throw new TestException("new ArrayWrapper(boolean[][][]) >> FlatException") ;
		}
		
		current = false ;
		for( int i=0; i<5; i++){
			if( flat[i] != current ) throw new TestException( "flat[" + i + "] = " + flat [i] );
			current = !current ;
		}
		System.out.println( "  ok" ) ;
		
	}
	// }}}


	
	
	// {{{ flatten_byte_1
	private static void flatten_byte_1() throws TestException{
		
		byte[] o = new byte[5] ;
		for( int i=0;i<5;i++) o[i] = (byte)i ;
		
		ArrayWrapper wrapper = null ; 
		System.out.print( "  >> new ArrayWrapper( byte[] ) " ); 
		try{
			wrapper = new ArrayWrapper(o); 
		} catch( NotAnArrayException e){
			throw new TestException("new ArrayWrapper(byte[]) >> NotAnArrayException ") ; 
		} 
		System.out.println( "ok"); 
		
		System.out.print( "  >> isPrimitive()" ) ;
		if( !wrapper.isPrimitive() ){
			throw new TestException( "ArrayWrapper(byte[]) not primitive" ) ; 
		}
		System.out.println( " true : ok" ); 
		
		System.out.print( "  >> getObjectTypeName()" ) ;
		if( !wrapper.getObjectTypeName().equals("B") ){
			throw new TestException( "ArrayWrapper(byte[]).getObjectTypeName() != 'I'" ) ;
		}
		System.out.println( " B : ok" ); 
		
		System.out.print( "  >> flat_byte()" ) ;
		byte[] flat;
		try{
			flat = wrapper.flat_byte() ; 
		} catch( PrimitiveArrayException e){
			throw new TestException( "PrimitiveArrayException" ) ;
		} catch( FlatException e){
			throw new TestException("new ArrayWrapper(byte[]) >> FlatException") ;
		}
		
		for( int i=0; i<5; i++){
			if( flat[i] != (byte)i ) throw new TestException( "flat[" + i + "] = " + flat [i] + "!=" + i); 
		}
		System.out.println( "  ok" ) ;
		
	}
	// }}}
	
	// {{{ flatten_byte_2
	private static void flatten_byte_2() throws TestException{
		
		byte[][] o = new byte[2][5] ;  
		int k = 0 ; 
		for( int i=0;i<2;i++){
			for( int j=0;j<5;j++,k++) {
				o[i][j] = (byte)k ;
			}
		}
		
		
		ArrayWrapper wrapper = null ; 
		System.out.print( "  >> new ArrayWrapper( byte[][] ) " ); 
		try{
			wrapper = new ArrayWrapper(o); 
		} catch( NotAnArrayException e){
			throw new TestException("new ArrayWrapper(byte[][]) >> NotAnArrayException ") ; 
		} 
		System.out.println( "ok"); 
		
		System.out.print( "  >> isPrimitive()" ) ;
		if( !wrapper.isPrimitive() ){
			throw new TestException( "ArrayWrapper(byte[][]) not primitive" ) ; 
		}
		System.out.println( " true : ok" ); 
		
		System.out.print( "  >> getObjectTypeName()" ) ;
		if( !wrapper.getObjectTypeName().equals("B") ){
			throw new TestException( "ArrayWrapper(byte[][]).getObjectTypeName() != 'B'" ) ;
		}
		System.out.println( " B : ok" ); 
		
		System.out.print( "  >> flat_byte()" ) ;
		byte[] flat;
		try{
			flat = wrapper.flat_byte() ; 
		} catch( PrimitiveArrayException e){
			throw new TestException( "PrimitiveArrayException" ) ;
		} catch( FlatException e){
			throw new TestException("new ArrayWrapper(byte[][]) >> FlatException") ;
		}
		
		
		for( int i=0; i<10; i++){
			if( flat[i] != (byte)i ) throw new TestException( "flat[" + i + "] = " + flat [i] + "!=" + i); 
		}
		System.out.println( "  ok" ) ;
		
	}
	// }}}
	
  // {{{ flatten_byte_3
	private static void flatten_byte_3() throws TestException{
		
		byte[][][] o = new byte[2][2][5] ;
		int k = 0;
		for( int i=0;i<2;i++){
			for( int j=0; j<2; j++){
				for( int jjj=0; jjj<5; jjj++,k++){
					o[i][j][jjj] = (byte)k ; 
				}
			}
		}
		
		ArrayWrapper wrapper = null ; 
		System.out.print( "  >> new ArrayWrapper( byte[][][] ) " ); 
		try{
			wrapper = new ArrayWrapper(o); 
		} catch( NotAnArrayException e){
			throw new TestException("new ArrayWrapper(byte[][][]) >> NotAnArrayException ") ; 
		}
		System.out.println( "ok"); 
		
		System.out.print( "  >> isPrimitive()" ) ;
		if( !wrapper.isPrimitive() ){
			throw new TestException( "ArrayWrapper(byte[][][]) not primitive" ) ; 
		}
		System.out.println( " true : ok" ); 
		
		System.out.print( "  >> getObjectTypeName()" ) ;
		if( !wrapper.getObjectTypeName().equals("B") ){
			throw new TestException( "ArrayWrapper(int[][][]).getObjectTypeName() != 'B'" ) ;
		}
		System.out.println( " B : ok" ); 
		
		System.out.print( "  >> flat_byte()" ) ;
		byte[] flat;
		try{
			flat = wrapper.flat_byte() ; 
		} catch( PrimitiveArrayException e){
			throw new TestException( "PrimitiveArrayException" ) ;
		} catch( FlatException e){
			throw new TestException("new ArrayWrapper(byte[][][]) >> FlatException") ;
		}
		
		
		for( int i=0; i<20; i++){
			if( flat[i] != (byte)i ) throw new TestException( "flat[" + i + "] = " + flat [i] + "!=" + i); 
		}
		System.out.println( "  ok" ) ;
		
	}
	// }}}
  
	
	// {{{ flatten_long_1
	private static void flatten_long_1() throws TestException{
		
		long[] o = new long[5] ;
		for( int i=0;i<5;i++) o[i] = (long)i ;
		
		ArrayWrapper wrapper = null ; 
		System.out.print( "  >> new ArrayWrapper( long[] ) " ); 
		try{
			wrapper = new ArrayWrapper(o); 
		} catch( NotAnArrayException e){
			throw new TestException("new ArrayWrapper(long[]) >> NotAnArrayException ") ; 
		} 
		System.out.println( "ok"); 
		
		System.out.print( "  >> isPrimitive()" ) ;
		if( !wrapper.isPrimitive() ){
			throw new TestException( "ArrayWrapper(long[]) not primitive" ) ; 
		}
		System.out.println( " true : ok" ); 
		
		System.out.print( "  >> getObjectTypeName()" ) ;
		if( !wrapper.getObjectTypeName().equals("J") ){
			throw new TestException( "ArrayWrapper(long[]).getObjectTypeName() != 'J'" ) ;
		}
		System.out.println( " J : ok" ); 
		
		System.out.print( "  >> flat_long()" ) ;
		long[] flat;
		try{
			flat = wrapper.flat_long() ; 
		} catch( PrimitiveArrayException e){
			throw new TestException( "PrimitiveArrayException" ) ;
		} catch( FlatException e){
			throw new TestException("new ArrayWrapper(long[]) >> FlatException") ;
		}
		
		for( int i=0; i<5; i++){
			if( flat[i] != (long)i ) throw new TestException( "flat[" + i + "] = " + flat [i] + "!=" + i); 
		}
		System.out.println( "  ok" ) ;
		
	}
	// }}}
	
	// {{{ flatten_long_2
	private static void flatten_long_2() throws TestException{
		
		long[][] o = new long[2][5] ;  
		int k = 0 ; 
		for( int i=0;i<2;i++){
			for( int j=0;j<5;j++,k++) {
				o[i][j] = (long)k ;
			}
		}
		
		
		ArrayWrapper wrapper = null ; 
		System.out.print( "  >> new ArrayWrapper( long[][] ) " ); 
		try{
			wrapper = new ArrayWrapper(o); 
		} catch( NotAnArrayException e){
			throw new TestException("new ArrayWrapper(long[][]) >> NotAnArrayException ") ; 
		} 
		System.out.println( "ok"); 
		
		System.out.print( "  >> isPrimitive()" ) ;
		if( !wrapper.isPrimitive() ){
			throw new TestException( "ArrayWrapper(long[][]) not primitive" ) ; 
		}
		System.out.println( " true : ok" ); 
		
		System.out.print( "  >> getObjectTypeName()" ) ;
		if( !wrapper.getObjectTypeName().equals("J") ){
			throw new TestException( "ArrayWrapper(long[][]).getObjectTypeName() != 'J'" ) ;
		}
		System.out.println( " J : ok" ); 
		
		System.out.print( "  >> flat_long()" ) ;
		long[] flat;
		try{
			flat = wrapper.flat_long() ; 
		} catch( PrimitiveArrayException e){
			throw new TestException( "PrimitiveArrayException" ) ;
		} catch( FlatException e){
			throw new TestException("new ArrayWrapper(long[][]) >> FlatException") ;
		}
		
		
		for( int i=0; i<10; i++){
			if( flat[i] != (long)i ) throw new TestException( "flat[" + i + "] = " + flat [i] + "!=" + i); 
		}
		System.out.println( "  ok" ) ;
		
	}
	// }}}
	
  // {{{ flatten_long_3
	private static void flatten_long_3() throws TestException{
		
		long[][][] o = new long[2][2][5] ;
		int k = 0;
		for( int i=0;i<2;i++){
			for( int j=0; j<2; j++){
				for( int jjj=0; jjj<5; jjj++,k++){
					o[i][j][jjj] = (long)k ; 
				}
			}
		}
		
		ArrayWrapper wrapper = null ; 
		System.out.print( "  >> new ArrayWrapper( long[][][] ) " ); 
		try{
			wrapper = new ArrayWrapper(o); 
		} catch( NotAnArrayException e){
			throw new TestException("new ArrayWrapper(long[][][]) >> NotAnArrayException ") ; 
		}
		System.out.println( "ok"); 
		
		System.out.print( "  >> isPrimitive()" ) ;
		if( !wrapper.isPrimitive() ){
			throw new TestException( "ArrayWrapper(long[][][]) not primitive" ) ; 
		}
		System.out.println( " true : ok" ); 
		
		System.out.print( "  >> getObjectTypeName()" ) ;
		if( !wrapper.getObjectTypeName().equals("J") ){
			throw new TestException( "ArrayWrapper(long[][][]).getObjectTypeName() != 'J'" ) ;
		}
		System.out.println( " J : ok" ); 
		
		System.out.print( "  >> flat_long()" ) ;
		long[] flat;
		try{
			flat = wrapper.flat_long() ; 
		} catch( PrimitiveArrayException e){
			throw new TestException( "PrimitiveArrayException" ) ;
		} catch( FlatException e){
			throw new TestException("new ArrayWrapper(long[][][]) >> FlatException") ;
		}
		
		
		for( int i=0; i<20; i++){
			if( flat[i] != (long)i ) throw new TestException( "flat[" + i + "] = " + flat [i] + "!=" + i); 
		}
		System.out.println( "  ok" ) ;
		
	}
	// }}}



	// {{{ flatten_short_1
	private static void flatten_short_1() throws TestException{
		
		short[] o = new short[5] ;
		for( int i=0;i<5;i++) o[i] = (short)i ;
		
		ArrayWrapper wrapper = null ; 
		System.out.print( "  >> new ArrayWrapper( short[] ) " ); 
		try{
			wrapper = new ArrayWrapper(o); 
		} catch( NotAnArrayException e){
			throw new TestException("new ArrayWrapper(short[]) >> NotAnArrayException ") ; 
		} 
		System.out.println( "ok"); 
		
		System.out.print( "  >> isPrimitive()" ) ;
		if( !wrapper.isPrimitive() ){
			throw new TestException( "ArrayWrapper(short[]) not primitive" ) ; 
		}
		System.out.println( " true : ok" ); 
		
		System.out.print( "  >> getObjectTypeName()" ) ;
		if( !wrapper.getObjectTypeName().equals("S") ){
			throw new TestException( "ArrayWrapper(long[]).getObjectTypeName() != 'S'" ) ;
		}
		System.out.println( " S : ok" ); 
		
		System.out.print( "  >> flat_short()" ) ;
		short[] flat;
		try{
			flat = wrapper.flat_short() ; 
		} catch( PrimitiveArrayException e){
			throw new TestException( "PrimitiveArrayException" ) ;
		} catch( FlatException e){
			throw new TestException("new ArrayWrapper(short[]) >> FlatException") ;
		}
		
		for( int i=0; i<5; i++){
			if( flat[i] != (double)i ) throw new TestException( "flat[" + i + "] = " + flat [i] + "!=" + i); 
		}
		System.out.println( "  ok" ) ;
		
	}
	// }}}
	
	// {{{ flatten_short_2
	private static void flatten_short_2() throws TestException{
		
		short[][] o = new short[2][5] ;  
		int k = 0 ; 
		for( int i=0;i<2;i++){
			for( int j=0;j<5;j++,k++) {
				o[i][j] = (short)k ;
			}
		}
		
		
		ArrayWrapper wrapper = null ; 
		System.out.print( "  >> new ArrayWrapper( short[][] ) " ); 
		try{
			wrapper = new ArrayWrapper(o); 
		} catch( NotAnArrayException e){
			throw new TestException("new ArrayWrapper(short[][]) >> NotAnArrayException ") ; 
		} 
		System.out.println( "ok"); 
		
		System.out.print( "  >> isPrimitive()" ) ;
		if( !wrapper.isPrimitive() ){
			throw new TestException( "ArrayWrapper(short[][]) not primitive" ) ; 
		}
		System.out.println( " true : ok" ); 
		
		System.out.print( "  >> getObjectTypeName()" ) ;
		if( !wrapper.getObjectTypeName().equals("S") ){
			throw new TestException( "ArrayWrapper(short[][]).getObjectTypeName() != 'S'" ) ;
		}
		System.out.println( " S : ok" ); 
		
		System.out.print( "  >> flat_short()" ) ;
		short[] flat;
		try{
			flat = wrapper.flat_short() ; 
		} catch( PrimitiveArrayException e){
			throw new TestException( "PrimitiveArrayException" ) ;
		} catch( FlatException e){
			throw new TestException("new ArrayWrapper(short[][]) >> FlatException") ;
		}
		
		
		for( int i=0; i<10; i++){
			if( flat[i] != (double)i ) throw new TestException( "flat[" + i + "] = " + flat [i] + "!=" + i); 
		}
		System.out.println( "  ok" ) ;
		
	}
	// }}}
	
  // {{{ flatten_short_3
	private static void flatten_short_3() throws TestException{
		
		short[][][] o = new short[2][2][5] ;
		int k = 0;
		for( int i=0;i<2;i++){
			for( int j=0; j<2; j++){
				for( int jjj=0; jjj<5; jjj++,k++){
					o[i][j][jjj] = (short)k ; 
				}
			}
		}
		
		ArrayWrapper wrapper = null ; 
		System.out.print( "  >> new ArrayWrapper( short[][][] ) " ); 
		try{
			wrapper = new ArrayWrapper(o); 
		} catch( NotAnArrayException e){
			throw new TestException("new ArrayWrapper(short[][][]) >> NotAnArrayException ") ; 
		}
		System.out.println( "ok"); 
		
		System.out.print( "  >> isPrimitive()" ) ;
		if( !wrapper.isPrimitive() ){
			throw new TestException( "ArrayWrapper(short[][][]) not primitive" ) ; 
		}
		System.out.println( " true : ok" ); 
		
		System.out.print( "  >> getObjectTypeName()" ) ;
		if( !wrapper.getObjectTypeName().equals("S") ){
			throw new TestException( "ArrayWrapper(short[][][]).getObjectTypeName() != 'S'" ) ;
		}
		System.out.println( " S : ok" ); 
		
		System.out.print( "  >> flat_short()" ) ;
		short[] flat;
		try{
			flat = wrapper.flat_short() ; 
		} catch( PrimitiveArrayException e){
			throw new TestException( "PrimitiveArrayException" ) ;
		} catch( FlatException e){
			throw new TestException("new ArrayWrapper(short[][][]) >> FlatException") ;
		}
		
		
		for( int i=0; i<20; i++){
			if( flat[i] != (double)i ) throw new TestException( "flat[" + i + "] = " + flat [i] + "!=" + i); 
		}
		System.out.println( "  ok" ) ;
		
	}
	// }}}

	

	// {{{ flatten_double_1
	private static void flatten_double_1() throws TestException{
		
		double[] o = new double[5] ;
		for( int i=0;i<5;i++) o[i] = i+0.0 ;
		
		ArrayWrapper wrapper = null ; 
		System.out.print( "  >> new ArrayWrapper( double[] ) " ); 
		try{
			wrapper = new ArrayWrapper(o); 
		} catch( NotAnArrayException e){
			throw new TestException("new ArrayWrapper(double[]) >> NotAnArrayException ") ; 
		} 
		System.out.println( "ok"); 
		
		System.out.print( "  >> isPrimitive()" ) ;
		if( !wrapper.isPrimitive() ){
			throw new TestException( "ArrayWrapper(double[]) not primitive" ) ; 
		}
		System.out.println( " true : ok" ); 
		
		System.out.print( "  >> getObjectTypeName()" ) ;
		if( !wrapper.getObjectTypeName().equals("D") ){
			throw new TestException( "ArrayWrapper(double[]).getObjectTypeName() != 'D'" ) ;
		}
		System.out.println( " D : ok" ); 
		
		System.out.print( "  >> flat_double()" ) ;
		double[] flat;
		try{
			flat = wrapper.flat_double() ; 
		} catch( PrimitiveArrayException e){
			throw new TestException( "PrimitiveArrayException" ) ;
		} catch( FlatException e){
			throw new TestException("new ArrayWrapper(double[]) >> FlatException") ;
		}
		
		for( int i=0; i<5; i++){
			if( flat[i] != (i+0.0) ) throw new TestException( "flat[" + i + "] = " + flat [i] + "!=" + i); 
		}
		System.out.println( "  ok" ) ;
		
	}
	// }}}
	
	// {{{ flatten_double_2
	private static void flatten_double_2() throws TestException{
		
		double[][] o = new double[2][5] ;  
		int k = 0 ; 
		for( int i=0;i<2;i++){
			for( int j=0;j<5;j++,k++) {
				o[i][j] = k + 0.0 ;
			}
		}
		
		
		ArrayWrapper wrapper = null ; 
		System.out.print( "  >> new ArrayWrapper( double[][] ) " ); 
		try{
			wrapper = new ArrayWrapper(o); 
		} catch( NotAnArrayException e){
			throw new TestException("new ArrayWrapper(double[][]) >> NotAnArrayException ") ; 
		} 
		System.out.println( "ok"); 
		
		System.out.print( "  >> isPrimitive()" ) ;
		if( !wrapper.isPrimitive() ){
			throw new TestException( "ArrayWrapper(double[][]) not primitive" ) ; 
		}
		System.out.println( " true : ok" ); 
		
		System.out.print( "  >> getObjectTypeName()" ) ;
		if( !wrapper.getObjectTypeName().equals("D") ){
			throw new TestException( "ArrayWrapper(double[][]).getObjectTypeName() != 'D'" ) ;
		}
		System.out.println( " D : ok" ); 
		
		System.out.print( "  >> flat_double()" ) ;
		double[] flat;
		try{
			flat = wrapper.flat_double() ; 
		} catch( PrimitiveArrayException e){
			throw new TestException( "PrimitiveArrayException" ) ;
		} catch( FlatException e){
			throw new TestException("new ArrayWrapper(double[][]) >> FlatException") ;
		}
		
		
		for( int i=0; i<10; i++){
			if( flat[i] != (i+0.0) ) throw new TestException( "flat[" + i + "] = " + flat [i] + "!=" + i); 
		}
		System.out.println( "  ok" ) ;
		
	}
	// }}}
	
  // {{{ flatten_double_3
	private static void flatten_double_3() throws TestException{
		
		double[][][] o = new double[2][2][5] ;
		int k = 0;
		for( int i=0;i<2;i++){
			for( int j=0; j<2; j++){
				for( int jjj=0; jjj<5; jjj++,k++){
					o[i][j][jjj] = (double)k ; 
				}
			}
		}
		
		ArrayWrapper wrapper = null ; 
		System.out.print( "  >> new ArrayWrapper( double[][][] ) " ); 
		try{
			wrapper = new ArrayWrapper(o); 
		} catch( NotAnArrayException e){
			throw new TestException("new ArrayWrapper(double[][][]) >> NotAnArrayException ") ; 
		}
		System.out.println( "ok"); 
		
		System.out.print( "  >> isPrimitive()" ) ;
		if( !wrapper.isPrimitive() ){
			throw new TestException( "ArrayWrapper(double[][][]) not primitive" ) ; 
		}
		System.out.println( " true : ok" ); 
		
		System.out.print( "  >> getObjectTypeName()" ) ;
		if( !wrapper.getObjectTypeName().equals("D") ){
			throw new TestException( "ArrayWrapper(double[][][]).getObjectTypeName() != 'D'" ) ;
		}
		System.out.println( " D : ok" ); 
		
		System.out.print( "  >> flat_double()" ) ;
		double[] flat;
		try{
			flat = wrapper.flat_double() ; 
		} catch( PrimitiveArrayException e){
			throw new TestException( "PrimitiveArrayException" ) ;
		} catch( FlatException e){
			throw new TestException("new ArrayWrapper(double[][][]) >> FlatException") ;
		}
		
		
		for( int i=0; i<20; i++){
			if( flat[i] != (i+0.0) ) throw new TestException( "flat[" + i + "] = " + flat [i] + "!=" + i); 
		}
		System.out.println( "  ok" ) ;
		
	}
	// }}}
	
	
	// {{{ flatten_char_1
	private static void flatten_char_1() throws TestException{
		
		char[] o = new char[5] ;
		for( int i=0;i<5;i++) o[i] = (char)i ;
		
		ArrayWrapper wrapper = null ; 
		System.out.print( "  >> new ArrayWrapper( char[] ) " ); 
		try{
			wrapper = new ArrayWrapper(o); 
		} catch( NotAnArrayException e){
			throw new TestException("new ArrayWrapper(char[]) >> NotAnArrayException ") ; 
		} 
		System.out.println( "ok"); 
		
		System.out.print( "  >> isPrimitive()" ) ;
		if( !wrapper.isPrimitive() ){
			throw new TestException( "ArrayWrapper(char[]) not primitive" ) ; 
		}
		System.out.println( " true : ok" ); 
		
		System.out.print( "  >> getObjectTypeName()" ) ;
		if( !wrapper.getObjectTypeName().equals("C") ){
			throw new TestException( "ArrayWrapper(char[]).getObjectTypeName() != 'C'" ) ;
		}
		System.out.println( " C : ok" ); 
		
		System.out.print( "  >> flat_char()" ) ;
		char[] flat;
		try{
			flat = wrapper.flat_char() ; 
		} catch( PrimitiveArrayException e){
			throw new TestException( "PrimitiveArrayException" ) ;
		} catch( FlatException e){
			throw new TestException("new ArrayWrapper(char[]) >> FlatException") ;
		}
		
		for( int i=0; i<5; i++){
			if( flat[i] != (char)i ) throw new TestException( "flat[" + i + "] = " + flat [i] + "!=" + i); 
		}
		System.out.println( "  ok" ) ;
		
	}
	// }}}
	
	// {{{ flatten_char_2
	private static void flatten_char_2() throws TestException{
		
		char[][] o = new char[2][5] ;  
		int k = 0 ; 
		for( int i=0;i<2;i++){
			for( int j=0;j<5;j++,k++) {
				o[i][j] = (char)k ;
			}
		}
		
		
		ArrayWrapper wrapper = null ; 
		System.out.print( "  >> new ArrayWrapper( char[][] ) " ); 
		try{
			wrapper = new ArrayWrapper(o); 
		} catch( NotAnArrayException e){
			throw new TestException("new ArrayWrapper(char[][]) >> NotAnArrayException ") ; 
		} 
		System.out.println( "ok"); 
		
		System.out.print( "  >> isPrimitive()" ) ;
		if( !wrapper.isPrimitive() ){
			throw new TestException( "ArrayWrapper(char[][]) not primitive" ) ; 
		}
		System.out.println( " true : ok" ); 
		
		System.out.print( "  >> getObjectTypeName()" ) ;
		if( !wrapper.getObjectTypeName().equals("C") ){
			throw new TestException( "ArrayWrapper(char[][]).getObjectTypeName() != 'C'" ) ;
		}
		System.out.println( " C : ok" ); 
		
		System.out.print( "  >> flat_char()" ) ;
		char[] flat;
		try{
			flat = wrapper.flat_char() ; 
		} catch( PrimitiveArrayException e){
			throw new TestException( "PrimitiveArrayException" ) ;
		} catch( FlatException e){
			throw new TestException("new ArrayWrapper(char[][]) >> FlatException") ;
		}
		
		
		for( int i=0; i<10; i++){
			if( flat[i] != (i+0.0) ) throw new TestException( "flat[" + i + "] = " + flat [i] + "!=" + i); 
		}
		System.out.println( "  ok" ) ;
		
	}
	// }}}
	
  // {{{ flatten_char_3
	private static void flatten_char_3() throws TestException{
		
		char[][][] o = new char[2][2][5] ;
		int k = 0;
		for( int i=0;i<2;i++){
			for( int j=0; j<2; j++){
				for( int jjj=0; jjj<5; jjj++,k++){
					o[i][j][jjj] = (char)k ; 
				}
			}
		}
		
		ArrayWrapper wrapper = null ; 
		System.out.print( "  >> new ArrayWrapper( char[][][] ) " ); 
		try{
			wrapper = new ArrayWrapper(o); 
		} catch( NotAnArrayException e){
			throw new TestException("new ArrayWrapper(char[][][]) >> NotAnArrayException ") ; 
		}
		System.out.println( "ok"); 
		
		System.out.print( "  >> isPrimitive()" ) ;
		if( !wrapper.isPrimitive() ){
			throw new TestException( "ArrayWrapper(char[][][]) not primitive" ) ; 
		}
		System.out.println( " true : ok" ); 
		
		System.out.print( "  >> getObjectTypeName()" ) ;
		if( !wrapper.getObjectTypeName().equals("C") ){
			throw new TestException( "ArrayWrapper(char[][][]).getObjectTypeName() != 'C'" ) ;
		}
		System.out.println( " C : ok" ); 
		
		System.out.print( "  >> flat_char()" ) ;
		char[] flat;
		try{
			flat = wrapper.flat_char() ; 
		} catch( PrimitiveArrayException e){
			throw new TestException( "PrimitiveArrayException" ) ;
		} catch( FlatException e){
			throw new TestException("new ArrayWrapper(char[][][]) >> FlatException") ;
		}
		
		
		for( int i=0; i<20; i++){
			if( flat[i] != (char)i ) throw new TestException( "flat[" + i + "] = " + flat [i] + "!=" + i); 
		}
		System.out.println( "  ok" ) ;
		
	}
	// }}}

	

	// {{{ flatten_float_1
	private static void flatten_float_1() throws TestException{
		
		float[] o = new float[5] ;
		for( int i=0;i<5;i++) o[i] = (float)(i+0.0) ;
		
		ArrayWrapper wrapper = null ; 
		System.out.print( "  >> new ArrayWrapper( float[] ) " ); 
		try{
			wrapper = new ArrayWrapper(o); 
		} catch( NotAnArrayException e){
			throw new TestException("new ArrayWrapper(float[]) >> NotAnArrayException ") ; 
		} 
		System.out.println( "ok"); 
		
		System.out.print( "  >> isPrimitive()" ) ;
		if( !wrapper.isPrimitive() ){
			throw new TestException( "ArrayWrapper(float[]) not primitive" ) ; 
		}
		System.out.println( " true : ok" ); 
		
		System.out.print( "  >> getObjectTypeName()" ) ;
		if( !wrapper.getObjectTypeName().equals("F") ){
			throw new TestException( "ArrayWrapper(float[]).getObjectTypeName() != 'F'" ) ;
		}
		System.out.println( " F : ok" ); 
		
		System.out.print( "  >> flat_float()" ) ;
		float[] flat;
		try{
			flat = wrapper.flat_float() ; 
		} catch( PrimitiveArrayException e){
			throw new TestException( "PrimitiveArrayException" ) ;
		} catch( FlatException e){
			throw new TestException("new ArrayWrapper(float[]) >> FlatException") ;
		}
		
		for( int i=0; i<5; i++){
			if( flat[i] != (i+0.0) ) throw new TestException( "flat[" + i + "] = " + flat [i] + "!=" + i); 
		}
		System.out.println( "  ok" ) ;
		
	}
	// }}}
	
	// {{{ flatten_float_2
	private static void flatten_float_2() throws TestException{
		
		float[][] o = new float[2][5] ;  
		int k = 0 ; 
		for( int i=0;i<2;i++){
			for( int j=0;j<5;j++,k++) {
				o[i][j] = (float)(k + 0.0) ;
			}
		}
		
		
		ArrayWrapper wrapper = null ; 
		System.out.print( "  >> new ArrayWrapper( float[][] ) " ); 
		try{
			wrapper = new ArrayWrapper(o); 
		} catch( NotAnArrayException e){
			throw new TestException("new ArrayWrapper(float[][]) >> NotAnArrayException ") ; 
		} 
		System.out.println( "ok"); 
		
		System.out.print( "  >> isPrimitive()" ) ;
		if( !wrapper.isPrimitive() ){
			throw new TestException( "ArrayWrapper(float[][]) not primitive" ) ; 
		}
		System.out.println( " true : ok" ); 
		
		System.out.print( "  >> getObjectTypeName()" ) ;
		if( !wrapper.getObjectTypeName().equals("F") ){
			throw new TestException( "ArrayWrapper(float[][]).getObjectTypeName() != 'F'" ) ;
		}
		System.out.println( " F : ok" ); 
		
		System.out.print( "  >> flat_float()" ) ;
		float[] flat;
		try{
			flat = wrapper.flat_float() ; 
		} catch( PrimitiveArrayException e){
			throw new TestException( "PrimitiveArrayException" ) ;
		} catch( FlatException e){
			throw new TestException("new ArrayWrapper(float[][]) >> FlatException") ;
		}
		
		
		for( int i=0; i<10; i++){
			if( flat[i] != (i+0.0) ) throw new TestException( "flat[" + i + "] = " + flat [i] + "!=" + i); 
		}
		System.out.println( "  ok" ) ;
		
	}
	// }}}
	
  // {{{ flatten_float_3
	private static void flatten_float_3() throws TestException{
		
		float[][][] o = new float[2][2][5] ;
		int k = 0;
		for( int i=0;i<2;i++){
			for( int j=0; j<2; j++){
				for( int jjj=0; jjj<5; jjj++,k++){
					o[i][j][jjj] = (float)k ; 
				}
			}
		}
		
		ArrayWrapper wrapper = null ; 
		System.out.print( "  >> new ArrayWrapper( float[][][] ) " ); 
		try{
			wrapper = new ArrayWrapper(o); 
		} catch( NotAnArrayException e){
			throw new TestException("new ArrayWrapper(float[][][]) >> NotAnArrayException ") ; 
		}
		System.out.println( "ok"); 
		
		System.out.print( "  >> isPrimitive()" ) ;
		if( !wrapper.isPrimitive() ){
			throw new TestException( "ArrayWrapper(float[][][]) not primitive" ) ; 
		}
		System.out.println( " true : ok" ); 
		
		System.out.print( "  >> getObjectTypeName()" ) ;
		if( !wrapper.getObjectTypeName().equals("F") ){
			throw new TestException( "ArrayWrapper(float[][][]).getObjectTypeName() != 'F'" ) ;
		}
		System.out.println( " F : ok" ); 
		
		System.out.print( "  >> flat_float()" ) ;
		float[] flat;
		try{
			flat = wrapper.flat_float() ; 
		} catch( PrimitiveArrayException e){
			throw new TestException( "PrimitiveArrayException" ) ;
		} catch( FlatException e){
			throw new TestException("new ArrayWrapper(float[][][]) >> FlatException") ;
		}
		
		
		for( int i=0; i<20; i++){
			if( flat[i] != (float)(i+0.0) ) throw new TestException( "flat[" + i + "] = " + flat [i] + "!=" + i); 
		}
		System.out.println( "  ok" ) ;
		
	}
	// }}}
		
	// }}}

	// {{{ flat array of String
	
	// {{{ flatten_String_1
	private static void flatten_String_1() throws TestException{
		
		String[] o = new String[5] ;
		for( int i=0;i<5;i++) o[i] = ""+i ;
		
		ArrayWrapper wrapper = null ; 
		System.out.print( "  >> new ArrayWrapper( String[] ) " ); 
		try{
			wrapper = new ArrayWrapper(o); 
		} catch( NotAnArrayException e){
			throw new TestException("new ArrayWrapper(String[]) >> NotAnArrayException ") ; 
		} 
		System.out.println( "ok"); 
		
		System.out.print( "  >> isPrimitive()" ) ;
		if( wrapper.isPrimitive() ){
			throw new TestException( "ArrayWrapper(String[]) is primitive" ) ; 
		}
		System.out.println( " false : ok" ); 
		
		System.out.print( "  >> getObjectTypeName()" ) ;
		if( !wrapper.getObjectTypeName().equals("java.lang.String") ){
			throw new TestException( "ArrayWrapper(float[]).getObjectTypeName() != 'java.lang.String'" ) ;
		}
		System.out.println( " java.lang.String : ok" ); 
		
		System.out.print( "  >> flat_String()" ) ;
		String[] flat;
		try{
			flat = wrapper.flat_String() ; 
		} catch( PrimitiveArrayException e){
			throw new TestException( "PrimitiveArrayException" ) ;
		} catch( FlatException e){
			throw new TestException("new ArrayWrapper(String[]) >> FlatException") ;
		}
		
		for( int i=0; i<5; i++){
			if( ! flat[i].equals(""+i) ) throw new TestException( "flat[" + i + "] = " + flat [i] + "!=" + i); 
		}
		System.out.println( "  ok" ) ;
		
	}
	// }}}
	
	// {{{ flatten_String_2
	private static void flatten_String_2() throws TestException{
		
		String[][] o = new String[2][5] ;  
		int k = 0 ; 
		for( int i=0;i<2;i++){
			for( int j=0;j<5;j++,k++) {
				o[i][j] = ""+k  ;
			}
		}
		
		
		ArrayWrapper wrapper = null ; 
		System.out.print( "  >> new ArrayWrapper( String[][] ) " ); 
		try{
			wrapper = new ArrayWrapper(o); 
		} catch( NotAnArrayException e){
			throw new TestException("new ArrayWrapper(String[][]) >> NotAnArrayException ") ; 
		} 
		System.out.println( "ok"); 
		
		System.out.print( "  >> isPrimitive()" ) ;
		if( wrapper.isPrimitive() ){
			throw new TestException( "ArrayWrapper(String[][]) is primitive" ) ; 
		}
		System.out.println( " true : ok" ); 
		
		System.out.print( "  >> getObjectTypeName()" ) ;
		if( !wrapper.getObjectTypeName().equals("java.lang.String") ){
			throw new TestException( "ArrayWrapper(float[][]).getObjectTypeName() != 'java.lang.String'" ) ;
		}
		System.out.println( " java.lang.String : ok" ); 
		
		System.out.print( "  >> flat_String()" ) ;
		String[] flat;
		try{
			flat = wrapper.flat_String() ; 
		} catch( PrimitiveArrayException e){
			throw new TestException( "PrimitiveArrayException" ) ;
		} catch( FlatException e){
			throw new TestException("new ArrayWrapper(String[][]) >> FlatException") ;
		}
		
		
		for( int i=0; i<10; i++){
			if( ! flat[i].equals( ""+i) ) throw new TestException( "flat[" + i + "] = " + flat [i] + "!=" + i); 
		}
		System.out.println( "  ok" ) ;
		
	}
	// }}}
	
  // {{{ flatten_String_3
	private static void flatten_String_3() throws TestException{
		
		String[][][] o = new String[2][2][5] ;
		int k = 0;
		for( int i=0;i<2;i++){
			for( int j=0; j<2; j++){
				for( int jjj=0; jjj<5; jjj++,k++){
					o[i][j][jjj] = ""+k ; 
				}
			}
		}
		
		ArrayWrapper wrapper = null ; 
		System.out.print( "  >> new ArrayWrapper( String[][][] ) " ); 
		try{
			wrapper = new ArrayWrapper(o); 
		} catch( NotAnArrayException e){
			throw new TestException("new ArrayWrapper(String[][][]) >> NotAnArrayException ") ; 
		}
		System.out.println( "ok"); 
		
		System.out.print( "  >> isPrimitive()" ) ;
		if( wrapper.isPrimitive() ){
			throw new TestException( "ArrayWrapper(String[][][]) is primitive" ) ; 
		}
		System.out.println( " true : ok" ); 
		
		System.out.print( "  >> getObjectTypeName()" ) ;
		if( !wrapper.getObjectTypeName().equals("java.lang.String") ){
			throw new TestException( "ArrayWrapper(String[][][]).getObjectTypeName() != 'java.lang.String'" ) ;
		}
		System.out.println( " java.lang.String : ok" ); 
		
		System.out.print( "  >> flat_String()" ) ;
		String[] flat;
		try{
			flat = wrapper.flat_String() ; 
		} catch( PrimitiveArrayException e){
			throw new TestException( "PrimitiveArrayException" ) ;
		} catch( FlatException e){
			throw new TestException("new ArrayWrapper(String[][][]) >> FlatException") ;
		}
		
		
		for( int i=0; i<20; i++){
			if( !flat[i].equals( ""+i) ) throw new TestException( "flat[" + i + "] = " + flat [i] + "!=" + i); 
		}
		System.out.println( "  ok" ) ;
		
	}
	// }}}
	// }}}
	
	// {{{ flat array of Point
	// {{{ flatten_Point_1
	private static void flatten_Point_1() throws TestException{
		
		Point[] o = new Point[5] ;
		for( int i=0;i<5;i++) o[i] = new Point(i,i) ;
		
		ArrayWrapper wrapper = null ; 
		System.out.print( "  >> new ArrayWrapper( Point[] ) " ); 
		try{
			wrapper = new ArrayWrapper(o); 
		} catch( NotAnArrayException e){
			throw new TestException("new ArrayWrapper(Point[]) >> NotAnArrayException ") ; 
		} 
		System.out.println( "ok"); 
		
		System.out.print( "  >> isPrimitive()" ) ;
		if( wrapper.isPrimitive() ){
			throw new TestException( "ArrayWrapper(Point[]) is primitive" ) ; 
		}
		System.out.println( " false : ok" ); 
		
		System.out.print( "  >> getObjectTypeName()" ) ;
		if( !wrapper.getObjectTypeName().equals("java.awt.Point") ){
			throw new TestException( "ArrayWrapper(Point[]).getObjectTypeName() != 'java.awt.Point'" ) ;
		}
		System.out.println( " java.awt.Point : ok" ); 
		
		System.out.print( "  >> flat_Object()" ) ;
		Point[] flat ;
		try{
			flat = (Point[])wrapper.flat_Object() ;
		} catch( ObjectArrayException e){
			throw new TestException( "ObjectArrayException" ) ;
		} catch( FlatException e){
			throw new TestException("new ArrayWrapper(Point[]) >> FlatException") ;
		}
		
		Point p ; 
		for( int i=0; i<5; i++){
			p = flat[i] ;
			if( p.x != i || p.y != i) throw new TestException( "flat[" + i + "].x = " + p.x + "!=" + i); 
		}
		System.out.println( "  ok" ) ;
		
	}
	// }}}
	
	// {{{ flatten_Point_2
	private static void flatten_Point_2() throws TestException{
		
		Point[][] o = new Point[2][5] ;  
		int k = 0 ; 
		for( int i=0;i<2;i++){
			for( int j=0;j<5;j++,k++) {
				o[i][j] = new Point( k, k )  ;
			}
		}
		
		
		ArrayWrapper wrapper = null ; 
		System.out.print( "  >> new ArrayWrapper( Point[][] ) " ); 
		try{
			wrapper = new ArrayWrapper(o); 
		} catch( NotAnArrayException e){
			throw new TestException("new ArrayWrapper(Point[][]) >> NotAnArrayException ") ; 
		} 
		System.out.println( "ok"); 
		
		System.out.print( "  >> isPrimitive()" ) ;
		if( wrapper.isPrimitive() ){
			throw new TestException( "ArrayWrapper(Point[][]) is primitive" ) ; 
		}
		System.out.println( " false : ok" ); 
		
		System.out.print( "  >> getObjectTypeName()" ) ;
		if( !wrapper.getObjectTypeName().equals("java.awt.Point") ){
			throw new TestException( "ArrayWrapper(Point[][]).getObjectTypeName() != 'java.awt.Point'" ) ;
		}
		System.out.println( " java.awt.Point : ok" ); 
		
		System.out.print( "  >> flat_Object()" ) ;
		Point[] flat;
		try{
			flat = (Point[])wrapper.flat_Object() ; 
		} catch( ObjectArrayException e){
			throw new TestException( "ObjectArrayException" ) ;
		} catch( FlatException e){
			throw new TestException("new ArrayWrapper(Point[][]) >> FlatException") ;
		}
		
		Point p; 
		for( int i=0; i<10; i++){
			p = flat[i] ;
			if( p.x != i || p.y != i) throw new TestException( "flat[" + i + "].x = " + p.x + "!=" + i); 
		}
		System.out.println( "  ok" ) ;
		
	}
	// }}}
	
  // {{{ flatten_Point_3
	private static void flatten_Point_3() throws TestException{
		
		Point[][][] o = new Point[2][2][5] ;
		int k = 0;
		for( int i=0;i<2;i++){
			for( int j=0; j<2; j++){
				for( int jjj=0; jjj<5; jjj++,k++){
					o[i][j][jjj] = new Point(k,k) ; 
				}
			}
		}
		
		ArrayWrapper wrapper = null ; 
		System.out.print( "  >> new ArrayWrapper( Point[][][] ) " ); 
		try{
			wrapper = new ArrayWrapper(o); 
		} catch( NotAnArrayException e){
			throw new TestException("new ArrayWrapper(Point[][][]) >> NotAnArrayException ") ; 
		}
		System.out.println( "ok"); 
		
		System.out.print( "  >> isPrimitive()" ) ;
		if( wrapper.isPrimitive() ){
			throw new TestException( "ArrayWrapper(Point[][][]) is primitive" ) ; 
		}
		System.out.println( " false : ok" ); 
		
		System.out.print( "  >> getObjectTypeName()" ) ;
		if( !wrapper.getObjectTypeName().equals("java.awt.Point") ){
			throw new TestException( "ArrayWrapper(Point[][][]).getObjectTypeName() != 'java.awt.Point'" ) ;
		}
		System.out.println( " java.awt.Point : ok" ); 
		
		System.out.print( "  >> flat_Object()" ) ;
		Point[] flat;
		try{
			flat = (Point[])wrapper.flat_Object() ; 
		} catch( ObjectArrayException e){
			throw new TestException( "ObjectArrayException" ) ;
		} catch( FlatException e){
			throw new TestException("new ArrayWrapper(Object[][][]) >> FlatException") ;
		}
		
		Point p; 
		for( int i=0; i<20; i++){
			p = flat[i]; 
			if( p.x != i || p.y != i ) throw new TestException( "flat[" + i + "].x = " + p.x + "!=" + i); 
		}
		System.out.println( "  ok" ) ;
		
	}
	// }}}
	
	// }}}
	// }}}
}
