/* LibMemcached
 * Copyright (C) 2006-2009 Brian Aker
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 *
 * Summary: Localized copy of WATCHPOINT debug symbols
 *
 */

#ifndef __LIBMEMCACHED_WATCHPOINT_H__
#define __LIBMEMCACHED_WATCHPOINT_H__

/* Some personal debugging functions */
#if defined(DEBUG)

#ifdef TARGET_OS_LINUX
static inline void libmemcached_stack_dump(void)
{
  void *array[10];
  int size;
  char **strings;

  size= backtrace(array, 10);
  strings= backtrace_symbols(array, size);

  fprintf(stderr, "Found %d stack frames.\n", size);

  for (int x= 0; x < size; x++)
    fprintf(stderr, "%s\n", strings[x]);

  free (strings);

  fflush(stderr);
}

#elif defined(__sun)
#include <ucontext.h>

static inline void libmemcached_stack_dump(void)
{
   fflush(stderr);
   printstack(fileno(stderr));
}

#else

static inline void libmemcached_stack_dump(void)
{ }

#endif // libmemcached_stack_dump()

#include <assert.h>

#define WATCHPOINT do { fprintf(stderr, "\nWATCHPOINT %s:%d (%s)\n", __FILE__, __LINE__,__func__);fflush(stdout); } while (0)
#define WATCHPOINT_ERROR(A) do {fprintf(stderr, "\nWATCHPOINT %s:%d %s\n", __FILE__, __LINE__, memcached_strerror(NULL, A));fflush(stdout); } while (0)
#define WATCHPOINT_IFERROR(A) do { if(A != MEMCACHED_SUCCESS)fprintf(stderr, "\nWATCHPOINT %s:%d %s\n", __FILE__, __LINE__, memcached_strerror(NULL, A));fflush(stdout); } while (0)
#define WATCHPOINT_STRING(A) do { fprintf(stderr, "\nWATCHPOINT %s:%d (%s) %s\n", __FILE__, __LINE__,__func__,A);fflush(stdout); } while (0)
#define WATCHPOINT_STRING_LENGTH(A,B) do { fprintf(stderr, "\nWATCHPOINT %s:%d (%s) %.*s\n", __FILE__, __LINE__,__func__,(int)B,A);fflush(stdout); } while (0)
#define WATCHPOINT_NUMBER(A) do { fprintf(stderr, "\nWATCHPOINT %s:%d (%s) %zu\n", __FILE__, __LINE__,__func__,(size_t)(A));fflush(stdout); } while (0)
#define WATCHPOINT_LABELED_NUMBER(A,B) do { fprintf(stderr, "\nWATCHPOINT %s:%d (%s) %s:%zu\n", __FILE__, __LINE__,__func__,(A),(size_t)(B));fflush(stdout); } while (0)
#define WATCHPOINT_IF_LABELED_NUMBER(A,B,C) do { if(A) {fprintf(stderr, "\nWATCHPOINT %s:%d (%s) %s:%zu\n", __FILE__, __LINE__,__func__,(B),(size_t)(C));fflush(stdout);} } while (0)
#define WATCHPOINT_ERRNO(A) do { fprintf(stderr, "\nWATCHPOINT %s:%d (%s) %s\n", __FILE__, __LINE__,__func__, strerror(A));fflush(stdout); } while (0)
#define WATCHPOINT_ASSERT_PRINT(A,B,C) do { if(!(A)){fprintf(stderr, "\nWATCHPOINT ASSERT %s:%d (%s) ", __FILE__, __LINE__,__func__);fprintf(stderr, (B),(C));fprintf(stderr,"\n");fflush(stdout); libmemcached_stack_dump(); } assert((A)); } while (0)
#define WATCHPOINT_ASSERT(A) do { if (! (A)) {libmemcached_stack_dump();} assert((A)); } while (0)
#define WATCHPOINT_ASSERT_INITIALIZED(A) do { if (! (A)) { libmemcached_stack_dump(); } assert(memcached_is_initialized((A))); } while (0);
#define WATCHPOINT_SET(A) do { A; } while(0);

#else

#define WATCHPOINT
#define WATCHPOINT_ERROR(A)
#define WATCHPOINT_IFERROR(A)
#define WATCHPOINT_STRING(A)
#define WATCHPOINT_NUMBER(A)
#define WATCHPOINT_LABELED_NUMBER(A,B)
#define WATCHPOINT_IF_LABELED_NUMBER(A,B,C)
#define WATCHPOINT_ERRNO(A)
#define WATCHPOINT_ASSERT_PRINT(A,B,C)
#define WATCHPOINT_ASSERT(A)
#define WATCHPOINT_ASSERT_INITIALIZED(A)
#define WATCHPOINT_SET(A)

#endif /* DEBUG */

#endif /* __LIBMEMCACHED_WATCHPOINT_H__ */
