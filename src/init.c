#include "rJava.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static int    jvm_opts=0;
static char **jvm_optv=0;

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

#define H_OUT  1
#define H_EXIT 2

static int default_hooks = H_OUT|H_EXIT;

static int vfprintf_hook(FILE *f, const char *fmt, va_list ap) {
  if (f==stderr) {
    REvprintf(fmt, ap);
    return 0;
  } else if (f==stdout) {
    Rvprintf(fmt, ap);
    return 0;
  }
  return vfprintf(f, fmt, ap);
}

static void exit_hook(int status) {
  REprintf("\nJava requested System.exit(%d), closing R.\n", status);
  /* FIXME: we could do something smart here such as running a call-back
     into R ... jump into R event loop ... at any rate we cannot return,
     but we don't want to kill R ... */
  exit(status);
}

static int initJVM(const char *user_classpath, int opts, char **optv, int hooks) {
  int total_num_properties, propNum = 0;
  jint res;
  char *classpath;
  
  if(!user_classpath)
    /* use the CLASSPATH environment variable as default */
    user_classpath = getenv("CLASSPATH");
  if(!user_classpath) user_classpath = "";
  
  vm_args.version = JNI_VERSION_1_2;
  if(JNI_GetDefaultJavaVMInitArgs(&vm_args) != JNI_OK) {
    error("JNI 1.2 or higher is required");
    return -1;    
  }
    
  /* leave room for class.path, and optional jni args */
  total_num_properties = 6 + opts;
    
  vm_options = (JavaVMOption *) calloc(total_num_properties, sizeof(JavaVMOption));
  vm_args.version = JNI_VERSION_1_2; /* should we do that or keep the default? */
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
  if (hooks&H_OUT) {
    vm_options[propNum].optionString = "vfprintf";
    vm_options[propNum++].extraInfo  = vfprintf_hook;
  }
  if (hooks&H_EXIT) {
    vm_options[propNum].optionString = "exit";
    vm_options[propNum++].extraInfo  = exit_hook;
  }
  vm_args.nOptions = propNum;

  /* Create the Java VM */
  res = JNI_CreateJavaVM(&jvm,(void **)&eenv, &vm_args);
  
  if (res != 0)
    error("Cannot create Java virtual machine (%d)", res);
  if (!eenv)
    error("Cannot obtain JVM environemnt");
  return 0;
}

#ifdef THREADS
#include <pthread.h>

#ifdef XXDARWIN
#include <Carbon/Carbon.h>
#include <CoreFoundation/CoreFoundation.h>
#include <CoreFoundation/CFRunLoop.h>
#endif

pthread_t initJVMpt;
pthread_mutex_t initMutex = PTHREAD_MUTEX_INITIALIZER;

int thInitResult = 0;
int initAWT = 0;

void *initJVMthread(void *classpath)
{
  int ws;
  jclass c;
  JNIEnv *lenv;

  thInitResult=initJVM((char*)classpath, jvm_opts, jvm_optv, default_hooks);
  if (thInitResult) return 0;

  init_rJava();

  lenv = eenv; /* we make a local copy before unlocking just in case
		  someone messes with it before we can use it */

  _dbg(rjprintf("initJVMthread: unlocking\n"));
  pthread_mutex_unlock(&initMutex);

  if (initAWT) {
    _dbg(rjprintf("initJVMthread: get AWT class\n"));
    /* we are still on the same thread, so we can safely use eenv */
    c = (*lenv)->FindClass(lenv, "java/awt/Frame");
  }

  _dbg(rjprintf("initJVMthread: returning from the init thread.\n"));
  return 0;
}

#endif

/** RinitJVM(classpath)
    initializes JVM with the specified class path */
SEXP RinitJVM(SEXP par)
{
  const char *c=0;
  SEXP e=CADR(par);
  int r=0;
  JavaVM *jvms[32];
  jsize vms=0;
  
  jvm_opts=0;
  jvm_optv=0;
  
  if (TYPEOF(e)==STRSXP && LENGTH(e)>0)
	  c=CHAR(STRING_ELT(e,0));

  e = CADDR(par);
  if (TYPEOF(e)==STRSXP && LENGTH(e)>0) {
	  int len = LENGTH(e);
	  jvm_optv=(char**)malloc(sizeof(char*)*len);
	  while (jvm_opts < len) {
		  jvm_optv[jvm_opts] = strdup(CHAR(STRING_ELT(e, jvm_opts)));
		  jvm_opts++;
	  }
  }
  
  r=JNI_GetCreatedJavaVMs(jvms, 32, &vms);
  if (r) {
    Rf_error("JNI_GetCreatedJavaVMs returned %d\n", r);
  } else {
    if (vms>0) {
      int i=0;
      _dbg(rjprintf("RinitJVM: Total %d JVMs found. Trying to attach the current thread.\n", (int)vms));
      while (i<vms) {
	if (jvms[i]) {
	  if (!(*jvms[i])->AttachCurrentThread(jvms[i], (void**)&eenv, NULL)) {
            _dbg(rjprintf("RinitJVM: Attached to existing JVM #%d.\n", i+1));
	    break;
	  }
	}
	i++;
      }
      if (i==vms) Rf_error("Failed to attach to any existing JVM.");
      else {
        init_rJava();
      }
      PROTECT(e=allocVector(INTSXP,1));
      INTEGER(e)[0]=(i==vms)?-2:1;
      UNPROTECT(1);
      return e;
    }
  }

#ifdef THREADS
  if (getenv("R_GUI_APP_VERSION") || getenv("RJAVA_INIT_AWT"))
    initAWT=1;

  _dbg(rjprintf("RinitJVM(threads): launching thread\n"));
  pthread_mutex_lock(&initMutex);
  pthread_create(&initJVMpt, 0, initJVMthread, c);
  _dbg(rjprintf("RinitJVM(threads): waiting for mutex\n"));
  pthread_mutex_lock(&initMutex);
  pthread_mutex_unlock(&initMutex);
  /* pthread_join(initJVMpt, 0); */
  _dbg(rjprintf("RinitJVM(threads): attach\n"));
  /* since JVM was initialized by another thread, we need to attach ourselves */
  (*jvm)->AttachCurrentThread(jvm, (void**)&eenv, NULL);
  _dbg(rjprintf("RinitJVM(threads): done.\n"));
  r = thInitResult;
#else
  profStart();
  r=initJVM(c, jvm_opts, jvm_optv, default_hooks);
  init_rJava();
  _prof(profReport("init_rJava:"));
  _dbg(rjprintf("RinitJVM(non-threaded): initJVM returned %d\n", r));
#endif
  if (jvm_optv) free(jvm_optv);
  jvm_opts=0;
  PROTECT(e=allocVector(INTSXP,1));
  INTEGER(e)[0]=r;
  UNPROTECT(1);
  return e;
}

void doneJVM() {
  (*jvm)->DestroyJavaVM(jvm);
  jvm = 0;
  eenv = 0;
}

