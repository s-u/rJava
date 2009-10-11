import java.lang.Comparable ; 

public class RJavaComparator {
	
	public static int compare( Object a, Object b ) throws NotComparableException{
		int res ; 
		if( a.equals( b ) ) return 0 ;
		
		if( ! ( a instanceof Comparable ) ) throw new NotComparableException( a ); 
		if( ! ( b instanceof Comparable ) ) throw new NotComparableException( b ); 
		
		try{
			res = ( (Comparable)a ).compareTo( b ) ; 
		} catch( ClassCastException e){
			try{
				res = - ((Comparable)b).compareTo( a ) ;
			} catch( ClassCastException f){
				throw new NotComparableException( a, b ); 
			}
		}
		return res ;
	}
	
	public static class NotComparableException extends Exception{
		public NotComparableException(Object a, Object b){
			super( "objects of class " + a.getClass().getName() + 
				" and " + b.getClass().getName() + " are not comparable"  ) ;
		}
		public NotComparableException( Object o){
			super( "class " + o.getClass().getName() + " does not implement java.util.Comparable" ) ; 
		}
		public NotComparableException( Class cl){
			super( "primitive" + cl.getName() + " not comparable to object" ); 
		}
	}
}

