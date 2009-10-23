
import java.util.regex.Pattern ;
import java.util.regex.Matcher ;

import java.util.Vector; 
import java.util.HashMap;
import java.util.Map;
import java.util.Collection;
import java.util.Set ;
import java.util.Iterator ;

import java.io.Serializable; 

public class RJavaImport implements Serializable {

	public static boolean DEBUG = false ; 
	
	/* TODO: vector is not good enough, we need to control the order
	         in which the packages appear */
	private Vector/*<Package>*/ importedPackages ;
	
	/* maps a simple name to a fully qualified name
		String -> java.lang.String 
	*/
	/* should we cache the Class instead ? */
	private Map/*<String,String>*/ cache ;
	
	public RJavaImport( ){
		importedPackages = new Vector/*<Package>*/(); 
		cache = new HashMap/*<String,String>*/() ;
	}
	
	private Class lookup_( String clazz ){
		Class res = null ;
		
		if( cache.containsKey( clazz ) ){
			try{
				String fullname = (String)cache.get( clazz ) ;
				Class cl = Class.forName( fullname ) ; 
				return cl ;
			} catch( Exception e ){
				/* does not happen */
			}
		}
		
		/* first try to see if the class does not exist verbatim */
		try{
			res = Class.forName( clazz ) ;
		} catch( Exception e){}
		if( res != null ) {
			cache.put( clazz, clazz ) ;
			return res;
		}
		
		int npacks = importedPackages.size() ;
		if( npacks > 0 ){
			for( int i=0; i<npacks; i++){
				try{
					Package p = (Package)importedPackages.get(i); 
					res = Class.forName( p.getName() + "." + clazz ) ;
				} catch( Exception e){}
				if( res != null ){
					cache.put( clazz, res.getName() ) ;
					return res ; 
				}
			}
		}
		return null ;
	}
	
	public Class lookup( String clazz){
		Class res = lookup_(clazz) ;
		if( DEBUG ) System.out.println( "  [J] lookup( '" + clazz + "' ) = " + (res == null ? " " : ("'" + res.getName() + "'" ) ) ) ;
		return res ;
	}

	/* TODO: we don't actually have to instantiate the Class */
	public boolean exists_( String clazz ){
		if( cache.containsKey( clazz ) ) return true ;
		return ( lookup( clazz ) != null );
	}
	
	public boolean exists( String clazz){
		boolean res = exists_(clazz) ;
		if( DEBUG ) System.out.println( "  [J] exists( '" + clazz + "' ) = " + res ) ;
		return res ;
	}

	
  public void importPackage( String packageName ){
  	Package p  = Package.getPackage( packageName ) ;
  	if( p != null ){
  		importedPackages.add( p ) ; 
  	}
  }
	
  public void importPackage( String[] packages ){
  	for( int i=0; i<packages.length; i++){
  		importPackage( packages[i] ) ;
  	}
  }
  
  /**
   * @return the full names of the classes currently known
   * by this importer
   */
  public String[] getKnownClasses(){
  	Set/*<String>*/ set = cache.keySet() ; 
  	int size = set.size() ;
  	String[] res = new String[size];
  	set.toArray( res );
  	if( DEBUG ) System.out.println( "  [J] getKnownClasses().length = "  + res.length ) ;
  	return res ;
  }
  
  public static Class lookup( String clazz , Set importers ){
  	Class res  ;
  	Iterator iterator = importers.iterator() ;
  	while( iterator.hasNext()){
  		RJavaImport importer = (RJavaImport)iterator.next() ;
  		res = importer.lookup( clazz ) ;
  		if( res != null ) return res ;
  	}
  	return null ;
  }
  
}

