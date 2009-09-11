// :tabSize=2:indentSize=2:noTabs=false:folding=explicit:collapseFolds=1:
import javax.swing.JFrame ;
import java.awt.Point ;
import java.lang.reflect.Constructor ;
import javax.swing.JButton ;
import javax.swing.ImageIcon ;

public class RJavaTools_Test {

	/* so that we can check about access to private fields and methods */
	private int bogus = 0 ; 
	private int getBogus(){ return bogus ; }
	
	// {{{ main 
	public static void main( String[] args){
		
		System.out.println( "Testing RJavaTools.getConstructor" ) ;
		try{
			constructors() ;
		} catch( TestException e ){
			System.err.println( "FAILED" ) ; 
		}
		System.out.println( "PASSED" ) ;
		
		System.out.println( "Testing RJavaTools.hasField" ) ;
		try{
			hasfield() ;
		} catch( TestException e ){
			System.err.println( "FAILED" ) ; 
		}
		System.out.println( "PASSED" ) ;
		
		System.out.println( "Testing RJavaTools.hasMethod" ) ;
		try{
			hasmethod() ;
		} catch( TestException e ){
			System.err.println( "FAILED" ) ; 
		}
		System.out.println( "PASSED" ) ;
		
		System.out.println( "Testing RJavaTools.getMethod" ) ;
		System.out.println( "NOT YET AVAILABLE" ) ;
		
		System.out.println( "Testing RJavaTools.newInstance" ) ;
		System.out.println( "NOT YET AVAILABLE" ) ;
		
		System.out.println( "Testing RJavaTools.invokeMethod" ) ;
		System.out.println( "NOT YET AVAILABLE" ) ;
		
	}
	// }}}

  // {{{ @Test constructors 
	private static void constructors() throws TestException {
		/* constructors */ 
		Constructor cons ;
		boolean error ; 
		
		// {{{ getConstructor( String, null )
		System.out.print( "    * getConstructor( String, null )" ) ;
		try{
			cons = RJavaTools.getConstructor( String.class, (Class[])null ) ;
		} catch( Exception e ){
			throw new TestException( "getConstructor( String, null )" ) ; 
		}
		System.out.println( " : ok " ) ;
		// }}}
		
		// {{{ getConstructor( Integer, { String.class } ) 
		System.out.print( "    * getConstructor( Integer, { String.class } )" ) ;
		try{
			cons = RJavaTools.getConstructor( Integer.class, new Class[]{ String.class } ) ;
		} catch( Exception e){
			throw new TestException( "getConstructor( Integer, { String.class } )" ) ; 
		}
		System.out.println( " : ok " ) ;
		// }}}
		
		// {{{ getConstructor( JButton, { String.class, ImageIcon.class } )
		System.out.print( "    * getConstructor( JButton, { String.class, ImageIcon.class } )" ) ;
		try{
			cons = RJavaTools.getConstructor( JButton.class, new Class[]{ String.class, ImageIcon.class } ) ;
		} catch( Exception e){
			throw new TestException( "getConstructor( JButton, { String.class, ImageIcon.class } )" ) ; 
		}
		System.out.println( " : ok " ) ;
		// }}}
		
		// {{{ getConstructor( Integer, null ) -> exception 
		error = false ; 
		System.out.print( "    * getConstructor( Integer, null )" ) ;
		try{
			cons = RJavaTools.getConstructor( Integer.class, (Class[])null ) ;
		} catch( Exception e){
			 error = true ; 
		}
		if( !error ){
			throw new TestException( "getConstructor( Integer, null ) did not generate error" ) ;
		}
		System.out.println( " -> exception : ok " ) ;
		// }}}
		
		// {{{ getConstructor( JButton, { String.class, JButton.class } ) -> exception
		error = false ; 
		System.out.print( "    * getConstructor( JButton, { String.class, JButton.class } )" ) ;
		try{
			cons = RJavaTools.getConstructor( JButton.class, new Class[]{ String.class, JButton.class } ) ;
		} catch( Exception e){
			 error = true ; 
		}
		if( !error ){
			throw new TestException( "getConstructor( JButton, { String.class, JButton.class } ) did not generate error" ) ;
		}
		System.out.println( " -> exception : ok " ) ;
		// }}}
		
	}
	// }}}
	
	// {{{ @Test methods
	private static void methods() throws TestException{
		
	}
	// }}}
	
	// {{{ @Test fields
	private static void hasfield() throws TestException{
		
		Point p = new Point() ; 
		System.out.println( "    java> Point p = new Point()" ) ; 
		System.out.print( "    * hasField( p, 'x' ) " ) ; 
		if( !RJavaTools.hasField( p, "x" ) ){
			throw new TestException( " hasField( Point, 'x' ) == false" ) ;
		}
		System.out.println( " true : ok" ) ;
		
		System.out.print( "    * hasField( p, 'iiiiiiiiiiiii' ) " ) ; 
		if( RJavaTools.hasField( p, "iiiiiiiiiiiii" ) ){
			throw new TestException( " hasField( Point, 'iiiiiiiiiiiii' ) == true" ) ;
		}
		System.out.println( "  false : ok" ) ;
		
		/* testing a private field */
		RJavaTools_Test ob = new RJavaTools_Test(); 
		System.out.print( "    * testing a private field " ) ; 
		if( RJavaTools.hasField( ob, "bogus" ) ){
			throw new TestException( " hasField returned true on private field" ) ;
		}
		System.out.println( "  false : ok" ) ;
		
		
	}
	// }}}
	
	// {{{ @Test hasmethod
	private static void hasmethod() throws TestException{
		
		Point p = new Point() ; 
		System.out.println( "    java> Point p = new Point()" ) ; 
		System.out.print( "    * hasMethod( p, 'move' ) " ) ; 
		if( !RJavaTools.hasMethod( p, "move" ) ){
			throw new TestException( " hasField( Point, 'move' ) == false" ) ;
		}
		System.out.println( " true : ok" ) ;
		
		System.out.print( "    * hasMethod( p, 'iiiiiiiiiiiii' ) " ) ; 
		if( RJavaTools.hasMethod( p, "iiiiiiiiiiiii" ) ){
			throw new TestException( " hasMethod( Point, 'iiiiiiiiiiiii' ) == true" ) ;
		}
		System.out.println( "  false : ok" ) ;
		
		/* testing a private method */
		RJavaTools_Test ob = new RJavaTools_Test(); 
		System.out.print( "    * testing a private method " ) ; 
		if( RJavaTools.hasField( ob, "getBogus" ) ){
			throw new TestException( " hasMethod returned true on private method" ) ;
		}
		System.out.println( "  false : ok" ) ;

	}
	// }}}

	// {{{ TestException class
	private static class TestException extends Exception{
		public TestException( String message ){
			super( message ) ;
		}
	}
	// }}}
	

}
