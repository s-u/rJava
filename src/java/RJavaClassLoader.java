import java.io.*;
import java.net.URL;
import java.util.HashMap;
import java.util.Vector;
import java.util.Enumeration;
import java.util.zip.*;

public class RJavaClassLoader extends ClassLoader {
    String rJavaPath, rJavaLibPath;
    HashMap libMap;
    Vector classPath;

    public RJavaClassLoader(String path, String libpath) {
	super();
	libMap = new HashMap();
	classPath = new Vector();
	rJavaPath = path;
	rJavaLibPath = libpath;
	classPath.add(path+"/classes");
	libMap.put("rJava", rJavaLibPath+"/rJava.so");
	System.out.println("new RJavaClassLoader(\""+path+"\", \""+libpath+"\")");
    }

    String classNameToFile(String cls) {
	// FIXME: convert . to /
	return cls;
    }

    InputStream findClassInJAR(String jar, String cl) {
	String cfn = classNameToFile(cl)+".class";
        try {
            ZipInputStream ins = new ZipInputStream(new FileInputStream(jar));
	    
            ZipEntry e;
            while ((e=ins.getNextEntry())!=null) {
		if (e.getName().equals(cfn))
		    return ins;
            }
        } catch(Exception e) {
        }
	return null;
    }
    
    protected Class findClass(String name) throws ClassNotFoundException {
	Class cl = null;
	System.out.println("RJavaClassLoaaer.findClass(\""+name+"\")");

	InputStream ins = null;

	for (Enumeration e = classPath.elements() ; e.hasMoreElements() ;) {
	    String cp = (String) e.nextElement();
	 
	    System.out.println(" - trying class path \""+cp+"\"");
	    try {
		ins = findClassInJAR(cp, name);
		if (ins == null) {
		    String classFN = cp+"/"+classNameToFile(name)+".class";
		    ins = new FileInputStream(classFN);
		}
		if (ins != null) {
		    byte fc[] = new byte[65536];
		    int n = ins.read(fc);
		    ins.close();
		    System.out.println(" - class length: "+n);
		    cl = defineClass(name, fc, 0, n);
		    System.out.println(" - class = "+cl);
		    return cl;
		}
	    } catch (Exception ex) {
		System.out.println(" * won't work: "+ex.getMessage());
	    }
	}
	System.out.println("=== giving up");
	if (cl == null) {
	    throw (new ClassNotFoundException());
	}
	return cl;
    }

    protected URL findResource(String name) {
	System.out.println("RJavaClassLoaaer.findResource(\""+name+"\")");
	return null;
    }

    /** add a library to path mapping for a native library */
    public void addRLibrary(String name, String path) {
	libMap.put(name, path);
    }

    public void addClassPath(String cp) {
	classPath.add(cp);
    }

    public void addClassPath(String[] cp) {
	int i = 0;
	while (i < cp.length) addClassPath(cp[i++]);
    }

    public String[] getClassPath() {
	int j = classPath.size();
	String[] s = new String[j];
	int i = 0;
	while (i < j) {
	    s[i] = (String) classPath.elementAt(i);
	    i++;
	}
	return s;
    }

    protected String findLibrary(String name) {
	System.out.println("RJavaClassLoaaer.findLibrary(\""+name+"\")");
	//if (name.equals("rJava"))
	//    return rJavaLibPath+"/"+name+".so";

	// we should provide a way to locate other package's libs
	String s = (String) libMap.get(name);
	System.out.println(" - mapping to "+((s==null)?"<none>":s));

	return s;
    }
}
