#define GC_DEBUG
#include "gc.h"
#define malloc(n) GC_MALLOC(n)
#define calloc(m,n) GC_MALLOC((m)*(n))
#define free(p) GC_FREE(p)
#define realloc(p,n) GC_REALLOC((p),(n))
#undef strdup
#define strdup(s) GC_STRDUP((s))
#define CHECK_LEAKS() GC_gcollect()
