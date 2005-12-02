#include <windows.h>
#include <winreg.h>
#include <stdio.h>

static char RegStrBuf[32768], dbuf[32768];

int main(int argc, char **argv) {
  DWORD t,s=32767;
  HKEY k;
  HKEY root=HKEY_LOCAL_MACHINE;
  char *javakey="Software\\JavaSoft\\Java Runtime Environment";

  /* JAVA_HOME can override our detection */
  if (getenv("JAVA_HOME")) {
    puts(getenv("JAVA_HOME"));
    return 0;
  }

  if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,javakey,0,KEY_QUERY_VALUE,&k)!=ERROR_SUCCESS ||
      RegQueryValueEx(k,"CurrentVersion",0,&t,RegStrBuf,&s)!=ERROR_SUCCESS) {
    javakey="Software\\JavaSoft\\Java Development Kit"; s=32767;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,javakey,0,KEY_QUERY_VALUE,&k)!=ERROR_SUCCESS ||
	RegQueryValueEx(k,"CurrentVersion",0,&t,RegStrBuf,&s)!=ERROR_SUCCESS) {
      fprintf(stderr, "ERROR*> JavaSoft\\{JRE|JDK} can't open registry keys.\n");
      /* MessageBox(wh, "Can't find Sun's Java runtime.\nPlease install Sun's J2SE JRE or JDK 1.4.2 or later (see http://java.sun.com/).","Can't find Sun's Java",MB_OK|MB_ICONERROR); */
      return -1;
    }
  }
  RegCloseKey(k); s=32767;

  strcpy(dbuf,javakey);
  strcat(dbuf,"\\");
  strcat(dbuf,RegStrBuf);
  javakey=(char*) malloc(strlen(dbuf)+1);
  strcpy(javakey, dbuf);

  if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,javakey,0,KEY_QUERY_VALUE,&k)!=ERROR_SUCCESS ||
      RegQueryValueEx(k,"JavaHome",0,&t,RegStrBuf,&s)!=ERROR_SUCCESS) {
    fprintf(stderr, "There's no JavaHome value in the JDK/JRE registry key.\n");
    /* MessageBox(wh, "Can't find Java home path. Maybe your JRE is too old.\nPlease install Sun's J2SE JRE or SDK 1.4.2 (see http://java.sun.com/).","Can't find Sun's Java",MB_OK|MB_ICONERROR); */
    return -1;
  }
  RegCloseKey(k);
  
  puts(RegStrBuf);
  return 0;
}

