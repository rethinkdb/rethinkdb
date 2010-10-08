dnl  Copyright (C) 2009 Sun Microsystems
dnl This file is free software; Sun Microsystems
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.
dnl--------------------------------------------------------------------
dnl PANDORA_PLUGINS
dnl Declare our plugin modules
dnl--------------------------------------------------------------------

AC_DEFUN([PANDORA_PLUGINS],[

  dnl We do this to prime the files from a fresh checkout. Normally we want
  dnl these commands to be executed by make. Perhaps we should split them into
  dnl a few shell script snippets in config and make Make call them... we're
  dnl going to get there...
  dnl ANYWAY - syscmd gets called during aclocal - so before automake. It will
  dnl get called probably during autoconf too, so it's important to protect
  dnl with test -f ... if the files exist, we don't have the chicken/egg 
  dnl problem and therefore don't need to do anything here
  m4_syscmd([python config/pandora-plugin > /dev/null])
  m4_syscmd([test -f config/plugin.stamp || touch config/plugin.stamp aclocal.m4])

  m4_sinclude(config/pandora-plugin.ac)

  dnl Add code here to read set plugin lists and  set drizzled_default_plugin_list
  pandora_builtin_list=`echo $pandora_builtin_list | sed 's/, *$//'`
  pandora_builtin_symbols_list=`echo $pandora_builtin_symbols_list | sed 's/, *$//'`
  AS_IF([test "x$pandora_builtin_symbols_list" = "x"], pandora_builtin_symbols_list="NULL")
  AC_SUBST([PANDORA_BUILTIN_LIST],[$pandora_builtin_list])
  AC_SUBST([PANDORA_BUILTIN_SYMBOLS_LIST],[$pandora_builtin_symbols_list])
  AC_SUBST([PANDORA_PLUGIN_LIST],[$pandora_default_plugin_list])
  m4_ifval(m4_normalize([$1]),[
    AC_CONFIG_FILES($*)
    ],[
    AC_DEFINE_UNQUOTED([PANDORA_BUILTIN_LIST],["$pandora_builtin_list"],
                       [List of plugins to be built in])
    AC_DEFINE_UNQUOTED([PANDORA_BUILTIN_SYMBOLS_LIST],["$pandora_builtin_symbols_list"],
                       [List of builtin plugin symbols to be built in])
    AC_DEFINE_UNQUOTED([PANDORA_PLUGIN_LIST],["$pandora_default_plugin_list"],
                       [List of plugins that should be loaded on startup if no
                        value is given for --plugin-load])
  ])


  AC_SUBST(pandora_plugin_test_list)
  AC_SUBST(pandora_plugin_libs)

  pandora_plugin_defs=`echo $pandora_plugin_defs | sed 's/, *$//'`
  AC_SUBST(pandora_plugin_defs)

  AC_SUBST(PANDORA_PLUGIN_DEP_LIBS)
  AC_SUBST(pkgplugindir,"\$(pkglibdir)")
])

AC_DEFUN([PANDORA_ADD_PLUGIN_DEP_LIB],[
  PANDORA_PLUGIN_DEP_LIBS="${PANDORA_PLUGIN_DEP_LIBS} $*"
])
