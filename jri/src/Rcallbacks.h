#ifndef __R_CALLBACKS__H__
#define __R_CALLBACKS__H__

#include <R.h>
#include <Rinternals.h>
#include <Rversion.h>

/* functions provided as R callbacks */

#if R_VERSION < R_Version(2,7,0)
#define RCCONST
#else
#define RCCONST const
#endif
/* ReadConsole API has been changed (unannounced and undocumented!) for Windows in r81626 */
#if defined (WIN32) && (R_VERSION < R_Version(4,2,0))
#define RCSIGN
#else
#define RCSIGN unsigned
#endif

int  Re_ReadConsole(RCCONST char *prompt, RCSIGN char *buf, int len, int addtohistory);
void Re_Busy(int which);
void Re_WriteConsole(RCCONST char *buf, int len);
void Re_WriteConsoleEx(RCCONST char *buf, int len, int oType);
void Re_ResetConsole(void);
void Re_FlushConsole(void);
void Re_ClearerrConsole(void);
int  Re_ChooseFile(int new, char *buf, int len);
void Re_ShowMessage(RCCONST char *buf);
void Re_read_history(char *buf);
void Re_loadhistory(SEXP call, SEXP op, SEXP args, SEXP env);
void Re_savehistory(SEXP call, SEXP op, SEXP args, SEXP env);
int  Re_ShowFiles(int nfile, RCCONST char **file, RCCONST char **headers, RCCONST char *wtitle, Rboolean del, RCCONST char *pager);

#endif
