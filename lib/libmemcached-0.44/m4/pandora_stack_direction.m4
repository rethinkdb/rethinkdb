dnl  Copyright (C) 2009 Sun Microsystems
dnl This file is free software; Sun Microsystems
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

#--------------------------------------------------------------------
# Check what direction the stack runs in
#--------------------------------------------------------------------

AC_DEFUN([PANDORA_STACK_DIRECTION],[
 AC_REQUIRE([AC_FUNC_ALLOCA])
 AC_CACHE_CHECK([stack direction], [ac_cv_c_stack_direction],[
  AC_RUN_IFELSE([AC_LANG_PROGRAM([[
#include <stdlib.h>
 int find_stack_direction ()
 {
   static char *addr = 0;
   auto char dummy;
   if (addr == 0)
     {
       addr = &dummy;
       return find_stack_direction ();
     }
   else
     return (&dummy > addr) ? 1 : -1;
 }
  ]],[[
    exit (find_stack_direction() < 0);
  ]])],[
   ac_cv_c_stack_direction=1
  ],[
   ac_cv_c_stack_direction=-1
  ])
 ])
 AC_DEFINE_UNQUOTED(STACK_DIRECTION, $ac_cv_c_stack_direction)
])



