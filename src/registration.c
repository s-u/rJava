#include <Rinternals.h>
#include <R_ext/Rdynload.h>

/* only to avoid NOTEs from broken checks,
   never called */
int dummy__(void) {
    return R_registerRoutines(0, 0, 0, 0, 0);
}

static DllInfo *dll;

/* registration is done in R code, so it has
   to have a way to disable dynamic symbols when done */
SEXP useDynamicSymbols(SEXP sDo) {
    if (dll) {
	R_useDynamicSymbols(dll, asInteger(sDo));
	return ScalarLogical(1);
    }
    return ScalarLogical(0);
}

/* record our dll so we can call useDynamicSymbols() later */
void R_init_rJava(DllInfo *dll_) {
    dll = dll_;
}
