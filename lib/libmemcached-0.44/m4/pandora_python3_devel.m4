dnl -*- mode: m4; c-basic-offset: 2; indent-tabs-mode: nil; -*-
dnl vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
dnl   
dnl pandora-build: A pedantic build system
dnl
dnl   Copyright (C) 2009 Sun Microsystems, Inc.
dnl   Copyright (c) 2008 Sebastian Huber <sebastian-huber@web.de>
dnl   Copyright (c) 2008 Alan W. Irwin <irwin@beluga.phys.uvic.ca>
dnl   Copyright (c) 2008 Rafael Laboissiere <rafael@laboissiere.net>
dnl   Copyright (c) 2008 Andrew Collier <colliera@ukzn.ac.za>
dnl   Copyright (c) 2008 Matteo Settenvini <matteo@member.fsf.org>
dnl   Copyright (c) 2008 Horst Knorr <hk_classes@knoda.org>
dnl
dnl   This program is free software: you can redistribute it and/or modify it
dnl   under the terms of the GNU General Public License as published by the
dnl   Free Software Foundation, either version 3 of the License, or (at your
dnl   option) any later version.
dnl
dnl   This program is distributed in the hope that it will be useful, but
dnl   WITHOUT ANY WARRANTY; without even the implied warranty of
dnl   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
dnl   Public License for more details.
dnl
dnl   You should have received a copy of the GNU General Public License along
dnl   with this program. If not, see <http://www.gnu.org/licenses/>.
dnl
dnl   As a special exception, the respective Autoconf Macro's copyright owner
dnl   gives unlimited permission to copy, distribute and modify the configure
dnl   scripts that are the output of Autoconf when processing the Macro. You
dnl   need not follow the terms of the GNU General Public License when using
dnl   or distributing such scripts, even though portions of the text of the
dnl   Macro appear in them. The GNU General Public License (GPL) does govern
dnl   all other use of the material that constitutes the Autoconf Macro.
dnl
dnl   This special exception to the GPL applies to versions of the Autoconf
dnl   Macro released by the Autoconf Macro Archive. When you make and
dnl   distribute a modified version of the Autoconf Macro, you may extend this
dnl   special exception to the GPL to apply to your modified version as well.

dnl SYNOPSIS
dnl
dnl   PANDORA_PYTHON3_DEVEL([version])
dnl
dnl DESCRIPTION
dnl
dnl   Note: Defines as a precious variable "PYTHON3_VERSION". Don't override it
dnl   in your configure.ac.
dnl
dnl   This macro checks for Python and tries to get the include path to
dnl   'Python.h'. It provides the $(PYTHON3_CPPFLAGS) and $(PYTHON3_LDFLAGS)
dnl   output variables. It also exports $(PYTHON3_EXTRA_LIBS) and
dnl   $(PYTHON3_EXTRA_LDFLAGS) for embedding Python in your code.
dnl
dnl   You can search for some particular version of Python by passing a
dnl   parameter to this macro, for example ">= '2.3.1'", or "== '2.4'". Please
dnl   note that you *have* to pass also an operator along with the version to
dnl   match, and pay special attention to the single quotes surrounding the
dnl   version number. Don't use "PYTHON3_VERSION" for this: that environment
dnl   variable is declared as precious and thus reserved for the end-user.
dnl
dnl LAST MODIFICATION
dnl
dnl   2009-08-23

AC_DEFUN([PANDORA_PYTHON3_DEVEL],[
	#
	# Allow the use of a (user set) custom python version
	#
	AC_ARG_VAR([PYTHON3_VERSION],[The installed Python
		version to use, for example '3.0'. This string
		will be appended to the Python interpreter
		canonical name.])


  AS_IF([test -z "$PYTHON3"],[
  	AC_PATH_PROG([PYTHON3],[python[$PYTHON3_VERSION]])
  ])
	AS_IF([test -z "$PYTHON3"],[
	  AC_MSG_ERROR([Cannot find python$PYTHON3_VERSION in your system path])
	  PYTHON3_VERSION=""
  ])

	#
	# if the macro parameter ``version'' is set, honour it
	#
	if test -n "$1"; then
		AC_MSG_CHECKING([for a version of Python $1])
		ac_supports_python3_ver=`$PYTHON3 -c "import sys, string; \
			ver = string.split(sys.version)[[0]]; \
			print(ver $1)"`
		if test "$ac_supports_python3_ver" = "True"; then
	   	   AC_MSG_RESULT([yes])
		else
			AC_MSG_RESULT([no])
			AC_MSG_ERROR([this package requires Python $1.
If you have it installed, but it isn't the default Python
interpreter in your system path, please pass the PYTHON3_VERSION
variable to configure. See ``configure --help'' for reference.
])
			PYTHON_VERSION=""
		fi
	fi

	#
	# Check if you have distutils, else fail
	#
	AC_MSG_CHECKING([for Python3 distutils package])
	ac_python3_distutils_result=`$PYTHON3 -c "import distutils" 2>&1`
	if test -z "$ac_python3_distutils_result"; then
		AC_MSG_RESULT([yes])
	else
		AC_MSG_RESULT([no])
		AC_MSG_ERROR([cannot import Python3 module "distutils".
Please check your Python3 installation. The error was:
$ac_python3_distutils_result])
		PYTHON3_VERSION=""
	fi

	#
	# Check for Python include path
	#
	AC_MSG_CHECKING([for Python3 include path])
	if test -z "$PYTHON3_CPPFLAGS"; then
		python3_path=`$PYTHON3 -c "import distutils.sysconfig; \
           		print(distutils.sysconfig.get_python_inc());"`
		if test -n "${python3_path}"; then
		   	python3_path="-I$python3_path"
		fi
		PYTHON3_CPPFLAGS=$python3_path
	fi
	AC_MSG_RESULT([$PYTHON3_CPPFLAGS])
	AC_SUBST([PYTHON3_CPPFLAGS])

	#
	# Check for Python library path
	#
	AC_MSG_CHECKING([for Python3 library path])
	if test -z "$PYTHON3_LDFLAGS"; then
		# (makes two attempts to ensure we've got a version number
		# from the interpreter)
		py3_version=`$PYTHON3 -c "from distutils.sysconfig import *; \
			print(' '.join(get_config_vars('VERSION')))"`
		if test "$py3_version" == "[None]"; then
			if test -n "$PYTHON3_VERSION"; then
				py3_version=$PYTHON3_VERSION
			else
				py3_version=`$PYTHON3 -c "import sys; \
					print(sys.version[[:3]])"`
			fi
		fi

		PYTHON3_LDFLAGS=`$PYTHON3 -c "from distutils.sysconfig import *; \
			print('-L' + get_python_lib(0,1), \
		      	'-lpython');"`$py3_version
	fi
	AC_MSG_RESULT([$PYTHON3_LDFLAGS])
	AC_SUBST([PYTHON3_LDFLAGS])

	#
	# Check for site packages
	#
	AC_MSG_CHECKING([for Python3 site-packages path])
	if test -z "$PYTHON3_SITE_PKG"; then
		PYTHON3_SITE_PKG=`$PYTHON3 -c "import distutils.sysconfig; \
		        print(distutils.sysconfig.get_python_lib(0,0));"`
	fi
	AC_MSG_RESULT([$PYTHON3_SITE_PKG])
	AC_SUBST([PYTHON3_SITE_PKG])

	#
	# libraries which must be linked in when embedding
	#
	AC_MSG_CHECKING(for Python3 embedding libraries)
	if test -z "$PYTHON3_EMBED_LIBS"; then
	   PYTHON3_EMBED_LIBS=`$PYTHON3 -c "import distutils.sysconfig; \
                conf = distutils.sysconfig.get_config_var; \
                print(conf('LOCALMODLIBS'), conf('LIBS'))"`
	fi
	AC_MSG_RESULT([$PYTHON3_EMBED_LIBS])
	AC_SUBST(PYTHON3_EMBED_LIBS)

	#
	# linking flags needed when embedding
	#
	AC_MSG_CHECKING(for Python3 embedding linking flags)
	if test -z "$PYTHON3_EMBED_LDFLAGS"; then
		PYTHON3_EMBED_LDFLAGS=`$PYTHON3 -c "import distutils.sysconfig; \
			conf = distutils.sysconfig.get_config_var; \
			print(conf('LINKFORSHARED'))"`
	fi
	AC_MSG_RESULT([$PYTHON3_EMBED_LDFLAGS])
	AC_SUBST(PYTHON3_EMBED_LDFLAGS)

	#
	# final check to see if everything compiles alright
	#
	AC_MSG_CHECKING([for Python3 development environment consistency])
	AC_LANG_PUSH([C])
	# save current global flags
  ac_save_LIBS="$LIBS"
  ac_save_CPPFLAGS="$CPPFLAGS"
	LIBS="$ac_save_LIBS $PYTHON3_LDFLAGS"
	CPPFLAGS="$ac_save_CPPFLAGS $PYTHON3_CPPFLAGS"
	AC_TRY_LINK([
		#include <Python.h>
	],[
		Py_Initialize();
	],[python3exists=yes],[python3exists=no])

	AC_MSG_RESULT([$python3exists])

        if test ! "$python3exists" = "yes"; then
	   AC_MSG_WARN([
  Could not link test program to Python3.
  Maybe the main Python3 library has been installed in some non-standard
  library path. If so, pass it to configure, via the LDFLAGS environment
  variable.
  Example: ./configure LDFLAGS="-L/usr/non-standard-path/python3/lib"
  ============================================================================
   ERROR!
   You probably have to install the development version of the Python3 package
   for your distribution.  The exact name of this package varies among them.
  ============================================================================
	   ])
	  PYTHON3_VERSION=""
	fi
	AC_LANG_POP
	# turn back to default flags
	CPPFLAGS="$ac_save_CPPFLAGS"
	LIBS="$ac_save_LIBS"

	#
	# all done!
	#
])

