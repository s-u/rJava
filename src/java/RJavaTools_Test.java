// :tabSize=2:indentSize=2:noTabs=false:folding=explicit:collapseFolds=1:
import javax.swing.JFrame ;
import java.awt.Point ;
import java.lang.reflect.Constructor ;
import javax.swing.JButton ;
import javax.swing.ImageIcon ;

public class RJavaTools_Test {

	// {{{ main 
	public static void main( String[] args){
		
		System.out.println( "Testing RJavaTools.getConstructor" ) ;
		try{
			constructors() ;
		} catch( TestException e ){
			System.err.println( "FAILED" ) ; 
		}
		System.out.println( "PASSED" ) ;
		
		System.out.println( "Testing RJavaTools.getMethod" ) ;
		try{
			methods() ;
		} catch( TestException e ){
			System.err.println( "FAILED" ) ; 
		}
		System.out.println( "PASSED" ) ;
		
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

	// {{{ TestException class
	private static class TestException extends Exception{
		public TestException( String message ){
			super( message ) ;
		}
	}
	// }}}
	

}
