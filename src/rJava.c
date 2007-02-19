#include <R.h>
#include "rJava.h"
#include <stdlib.h>
#include <string.h>

#define PATH_SEPARATOR ':'
#define USER_CLASSPATH "."

JavaVM *jvm;

jclass javaStringClass;
jclass javaObjectClass;
jclass javaClassClass;

#ifdef JNI_VERSION_1_2 
static JavaVMOption *vm_options;
static JavaVMInitArgs vm_args;
#else  
#error "Java/JNI 1.2 or higher is required!"
#endif

/* determine whether eenv chache should be used (has no effect if JNI_CACHE is not set) */
int use_eenv = 1;

/* cached environment. Do NOT use directly! Always use getJNIEnv()! */
JNIEnv *eenv;

#ifdef JNI_CACHE
JNIEnv *getJNIEnvSafe();
JNIEnv *getJNIEnv() {
  return (use_eenv)?eenv:getJNIEnvSafe();
}

JNIEnv *getJNIEnvSafe()
#else
JNIEnv *getJNIEnv()
#endif
  {
    JNIEnv *env;
    jsize l;
    jint res;
    
    if (!jvm) { /* we're hoping that the JVM pointer won't change :P we fetch it just once */
        res= JNI_GetCreatedJavaVMs(&jvm, 1, &l);
        if (res!=0) {
            Rf_error("JNI_GetCreatedJavaVMs failed! (result:%d)",(int)res); return 0;
        }
        if (l<1)
	    Rf_error("JNI_GetCreatedJavaVMs said there's no JVM running! Maybe .jinit() would help.");
    }
    res = (*jvm)->AttachCurrentThread(jvm, (void**) &env, 0);
    if (res!=0) {
      Rf_error("AttachCurrentThread failed! (result:%d)", (int)res); return 0;
    }
    if (env && !eenv) eenv=env;
    
    /* if (eenv!=env)
        fprintf(stderr, "Warning! eenv=%x, but env=%x - different environments encountered!\n", eenv, env); */
    return env;
}

int initJVM(char *user_classpath, int opts, char **optv) {
  int total_num_properties, propNum = 0;
  jint res;
  char *classpath;
  
  if(!user_classpath)
    /* use the CLASSPATH environment variable as default */
    user_classpath = (char*) getenv("CLASSPATH");
  if(!user_classpath) user_classpath = "";
  
  vm_args.version = JNI_VERSION_1_2;
  if(JNI_GetDefaultJavaVMInitArgs(&vm_args) != JNI_OK) {
    Rf_error("JNI 1.2 or higher is required");
    return -1;    
  }
    
  /* leave room for class.path, and optional jni args */
  total_num_properties = 3 + opts;
    
  vm_options = (JavaVMOption *) calloc(total_num_properties, sizeof(JavaVMOption));
  vm_args.version = JNI_VERSION_1_2; /* should we do that or keep the drfault? */
  vm_args.options = vm_options;
  vm_args.ignoreUnrecognized = JNI_TRUE;
  
  classpath = (char*) calloc(24 + strlen(user_classpath), sizeof(char));
  sprintf(classpath, "-Djava.class.path=%s", user_classpath);
  
  vm_options[propNum++].optionString = classpath;   
  
  /*   print JNI-related messages */
  /* vm_options[propNum++].optionString = "-verbose:class,jni"; */
  
  if (optv) {
    int i=0;
    while (i<opts) {
      if (optv[i]) vm_options[propNum++].optionString = optv[i];
      i++;
    }
  }
  vm_args.nOptions = propNum;

  /* Create the Java VM */
  res = JNI_CreateJavaVM(&jvm,(void **)&eenv, &vm_args);
  
  if (res != 0 || eenv == NULL) {
    Rf_error("Cannot create Java Virtual Machine");
    return -1;
  }
  return 0;
}

void doneJVM() {
  (*jvm)->DestroyJavaVM(jvm);
  jvm = 0;
  eenv = 0;
}

void RuseJNICache(int *flag) {
  if (flag) use_eenv=*flag;
}
