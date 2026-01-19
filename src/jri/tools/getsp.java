public class getsp {
    public static void main(String[] args) {
	if (args!=null && args.length>0) {
	    if (args[0].compareTo("-test")==0) {
		System.out.println("Test1234OK");
	    } else
	    if (args[0].compareTo("-libs")==0) {
		String prefix="-L";
		if (args.length>1) prefix=args[1];
		String lp=System.getProperty("java.library.path");
		// we're not using StringTokenizer in case the JVM is very crude
		int i=0,j,k=0;
		String r=null;
		String pss=System.getProperty("path.separator");
		char ps=':';
		if (pss!=null && pss.length()>0) ps=pss.charAt(0);
		j=lp.length();
		while (i<=j) {
		    if (i==j || lp.charAt(i)==ps) {
			String lib=lp.substring(k,i);
			k=i+1;
			if (lib.compareTo(".")!=0)
			    r=(r==null)?(prefix+lib):(r+" "+prefix+lib);
		    }
		    i++;
		}
		if (r!=null) System.out.println(r);
	    } else
	    if (args[0].equals("-minver")) {
		boolean meets = false;
		String jv = System.getProperty("java.version");
		if (jv.startsWith("1."))
		    jv = jv.substring(2);
		try {
		    int i = 0;
		    while (i < jv.length() && jv.charAt(i) >= '0' && jv.charAt(i) < '9')
			i++;
		    jv = jv.substring(0, i);
		    if (args.length > 1) {
			int req = Integer.parseInt(args[1]);
			int cv  = Integer.parseInt(jv);
			meets = cv >= req;
		    }
		} catch (Exception e) {
		}
		System.out.println(meets ? "yes" : "no");
	    } else
		System.out.println(System.getProperty(args[0]));
	}
    }
}
