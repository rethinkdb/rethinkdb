dnl  Copyright (C) 2009 Sun Microsystems
dnl This file is free software; Sun Microsystems
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

#--------------------------------------------------------------------
# Check for GCC Atomic Support
#--------------------------------------------------------------------


AC_DEFUN([PANDORA_HAVE_GCC_ATOMICS],[
	
  AC_CACHE_CHECK(
    [whether the compiler provides atomic builtins],
    [ac_cv_gcc_atomic_builtins],
    [AC_LINK_IFELSE(
      [AC_LANG_PROGRAM([],[[
        int foo= -10; int bar= 10;
        if (!__sync_fetch_and_add(&foo, bar) || foo)
          return -1;
        bar= __sync_lock_test_and_set(&foo, bar);
        if (bar || foo != 10)
          return -1;
        bar= __sync_val_compare_and_swap(&bar, foo, 15);
        if (bar)
          return -1;
        return 0;
        ]])],
      [ac_cv_gcc_atomic_builtins=yes],
      [ac_cv_gcc_atomic_builtins=no])])

  AS_IF([test "x$ac_cv_gcc_atomic_builtins" = "xyes"],[
    AC_DEFINE(HAVE_GCC_ATOMIC_BUILTINS, 1,
              [Define to 1 if compiler provides atomic builtins.])
  ])

])
