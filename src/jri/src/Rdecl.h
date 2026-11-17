#ifndef __RDECL_H__
#define __RDECL_H__

/* declarations from R internals or other include files */
/* last update: R 4.6.0 */

#include <Rversion.h>

/* some have been added to R 4.6.0 */
#if R_VERSION >= R_Version(4,6,0)

/* Rinterface.h is not available on Windows */
#ifndef Win32
#define R_INTERFACE_PTRS 1
#define CSTACK_DEFNS 1
#include <Rinterface.h> /* R_ReadConsole since 4.6.0 */
#endif

#include <Rembedded.h>  /* run_Rmainloop (since 4.6.0), Rf_initialize_R (since 2.4.0) */

/* required on Windows (since 4.6.0) */
#ifdef Win32
int R_ReadConsole(const char *, unsigned char *, int, int);
#endif

#else
void run_Rmainloop(void); /* main/main.c */
int  R_ReadConsole(char*, unsigned char*, int, int); /* include/Defn.h */
int  Rf_initialize_R(int ac, char **av); /* include/Rembedded.h - exists since 2.4.0 */
#endif /* R < 4.6.0 */

#endif /* __RDECL_H__ */
