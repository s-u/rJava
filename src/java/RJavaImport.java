
import java.util.regex.Pattern ;
import java.util.regex.Matcher ;

import java.util.Vector; 
import java.util.HashMap; 

public class RJavaImport {

	/* TODO: vector is not good enough, we need to control the order
	         in which the packages appear */
	private Vector importedPackages ;
	
	public RJavaImport( ){
		importedPackages = new Vector(); 
	}
	
	public Class lookup( String clazz ){
		Class res = null ;
		
		/* first try to see if the class does not exist verbatim */
		try{
			res = Class.forName( clazz ) ;
		} catch( Exception e){}
		if( res != null ) return res; 
		
		int npacks = importedPackages.size() ;
		if( npacks > 0 ){
			for( int i=0; i<npacks; i++){
				try{
					Package p = (Package)importedPackages.get(i); 
					res = Class.forName( p.getName() + "." + clazz ) ;
				} catch( Exception e){}
				if( res != null ){
					return res ; 
				}
			}
		}
		return null ;
	}

  public void importPackage( String packageName ){
  	Package p  = Package.getPackage( packageName ) ;
  	if( p != null ){
  		importedPackages.add( p ) ; 
  	}
  }
	
}

