dnl  Copyright (C) 2009 Sun Microsystems
dnl This file is free software; Sun Microsystems
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl Try to define a macro to dump the current callstack.
AC_DEFUN([PANDORA_PRINT_CALLSTACK],[
  AC_CHECK_HEADERS([ucontext.h])
  AS_IF([test "x$ac_cv_header_ucontext_h" = "xyes"],
  	[ AC_CHECK_FUNCS([printstack]) ])


  AS_IF([ test "x$ac_cv_func_printstack" != "xyes"],
        [ AC_CHECK_HEADERS([dlfcn.h])
          AC_CHECK_HEADERS([execinfo.h])
          AC_CHECK_FUNCS([backtrace])
          AC_CHECK_FUNCS([backtrace_symbols_fd]) ])

  AH_BOTTOM([
#ifdef __cplusplus
#include <cstdio>
#define PANDORA_PRINTSTACK_STD_PREFIX std::
#else
#include <stdio.h>
#define PANDORA_PRINTSTACK_STD_PREFIX
#endif

#if defined(HAVE_UCONTEXT_H) && defined(HAVE_PRINTSTACK)
#include <ucontext.h>
#define pandora_print_callstack(a) \
printstack(PANDORA_PRINTSTACK_STD_PREFIX fileno(a))
#elif defined(HAVE_EXECINFO_H) && defined(HAVE_BACKTRACE) && defined(HAVE_BACKTRACE_SYMBOLS_FD)

#include <execinfo.h>

#define pandora_print_callstack(a) \
{ \
  void *stack[100];  \
  int depth = backtrace(stack, 100); \
  backtrace_symbols_fd(stack, depth, PANDORA_PRINTSTACK_STD_PREFIX fileno(a)); \
}
#elif defined(HAVE_EXECINFO_H) && defined(HAVE_BACKTRACE) && defined(HAVE_BACKTRACE_SYMBOLS) && !defined(HAVE_BACKTRACE_SYMBOLS_FD)

#include <execinfo.h>

#define pandora_print_callstack(a) \
{ \
  void *stack[100];  \
  int depth= backtrace(stack, 100); \
  char **symbol= backtrace_symbols(stack, depth); \
  for (int x= 0; x < size; ++x) \
    PANDORA_PRINTSTACK_STD_PREFIX fprintf(a, "%s\n", symbol[x]); \
}
#else
#define pandora_print_callstack(a) \
    PANDORA_PRINTSTACK_STD_PREFIX fprintf(a, \
      "Stackdump not supported for this platform\n");
#endif
  ])

])
