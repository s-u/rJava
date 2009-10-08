
import java.util.regex.Pattern ;
import java.util.regex.Matcher ;

import java.util.Vector; 
import java.util.HashMap;
import java.util.Set ;
import java.util.Iterator ;

import java.io.Serializable; 

public class RJavaImport implements Serializable {

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
	
  public void importPackage( String[] packages ){
  	for( int i=0; i<packages.length; i++){
  		importPackage( packages[i] ) ;
  	}
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

