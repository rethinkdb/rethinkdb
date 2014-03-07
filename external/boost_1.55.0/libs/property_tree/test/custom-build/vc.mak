# Boost.PropertyTree
#
# Copyright (c) 2009 Sebastian Redl
#
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

CC=cl
CFLAGSREL=-O2 -Ox -EHsc -DBOOST_DISABLE_WIN32 -nologo
CFLAGSDBG=-EHsc -DBOOST_DISABLE_WIN32 -nologo
EXTINCLUDE=
EXTLIBS=

-include Makefile-Common
