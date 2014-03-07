#! /bin/bash

# copyright John Maddock 2003
# Distributed under the Boost Software License, Version 1.0. 
# (See accompanying file LICENSE_1_0.txt or copy at 
# http://www.boost.org/LICENSE_1_0.txt.

libname=""
src=""
header=""
all_dep=""

# current makefile:
out=""
# temporary file:
tout=""
# install target temp file:
iout=""
# debug flag:
debug="no"
# compile options:
opts=""
# main output sub-directory:
subdir=""
# vcl flag:
use_vcl="yes"

###############################################################
#
# gcc generator section:
#
###############################################################

gcc_shared="no"

function gcc_gen_lib()
{
	if test "$gcc_shared" == "yes"; then
		obj_dir="$libname""_shared"
		all_dep="$all_dep $subdir $subdir/$obj_dir ./$subdir/lib$libname.so"
	else
		obj_dir="$libname"
		all_dep="$all_dep $subdir $subdir/$obj_dir ./$subdir/lib$libname.a"
	fi
#
# set up section comments:
	cat >> $tout << EOF
########################################################
#
# section for lib$libname.a
#
########################################################
EOF
#
#	process source files:
	all_obj=""
	for file in $src
	do
		obj=`echo "$file" | sed 's/.*src\/\(.*\)cpp/\1o/g'`
		obj="$subdir/$obj_dir/$obj"
		all_obj="$all_obj $obj"
		echo "$obj: $file \$(ALL_HEADER)" >> $tout
		echo "	\$(CXX) \$(INCLUDES) -o $obj $opts \$(CXXFLAGS) \$(ICU_CXXFLAGS) $file" >> $tout
		echo "" >> $tout
	done
#
#	 now for the directories for this library:
	echo "$subdir/$obj_dir : " >> $tout
	echo "	mkdir -p $subdir/$obj_dir" >> $tout
	echo "" >> $tout
#
#	 now for the clean options for this library:
	all_clean="$all_clean $libname""_clean"
	echo "$libname"_clean : >> $tout
	echo "	rm -f $subdir/$obj_dir/*.o" >> $tout
	echo "" >> $tout
#
#	 now for the main target for this library:
          if test "$gcc_shared" == "yes"; then
		echo ./$subdir/lib$libname.so : $all_obj >> $tout
		echo "	\$(LINKER) -o $subdir/lib$libname.so \$(LDFLAGS)  \$(ICU_LDFLAGS) $all_obj \$(ICU_LIBS) \$(LIBS)" >> $tout
	else
		echo ./$subdir/lib$libname.a : $all_obj >> $tout
		echo "	ar -r $subdir/lib$libname.a $all_obj" >> $tout
		echo "	-ar -s $subdir/lib$libname.a" >> $tout
	fi
	echo "" >> $tout
}

function gcc_gen()
{
	out="gcc.mak"
	tout="temp"
	iout="temp_install"
	subdir="gcc"
	all_dep=""
	all_clean=""
	echo > $out
	echo > $tout
	echo > $iout

	libname="boost_regex-gcc-${boost_version}"
	opts="\$(C1)"
	gcc_gen_lib
	libname="boost_regex-gcc-d-${boost_version}"
	opts="\$(C2)"
	gcc_gen_lib
	
	
	cat > $out << EOF
# copyright John Maddock 2006-7
# Distributed under the Boost Software License, Version 1.0. 
# (See accompanying file LICENSE_1_0.txt or copy at 
# http://www.boost.org/LICENSE_1_0.txt.
#
# auto generated makefile for gcc compiler
#
# usage:
# make
#   brings libraries up to date
# make clean
#   deletes temporary object files (but not archives).
#

#
# the following environment variables are recognised:
# ICU_PATH= Path to ICU installation.
# CXXFLAGS= extra compiler options - note applies to all build variants
# INCLUDES= additional include directories
# LDFLAGS=  additional linker options
# LIBS=     additional library files

# compiler:
CXX?=g++
LINKER=\$(CXX) -shared

#
# compiler options for release build:
#
C1=-c -O2 -I../../..
#
# compiler options for debug build:
#
C2=-c -g -I../../..

ifeq "\$(ICU_PATH)" ""
\$(warning "Building Boost.Regex without ICU / Unicode support:")
\$(warning "Hint: set ICU_PATH on the nmake command line to point ")
\$(warning "to your ICU installation if you have one.")
else
ICU_CXXFLAGS= -DBOOST_HAS_ICU=1 -I\$(ICU_PATH)/include
ICU_LDFLAGS= -L\$(ICU_PATH)/lib
ICU_LIBS= -licui18n -licuuc
\$(warning "Building Boost.Regex with ICU in \$(ICU_PATH)")
endif

EOF
	echo "" >> $out
	echo "ALL_HEADER=$header" >> $out
	echo "" >> $out
	echo "all : $subdir $all_dep" >> $out
	echo >> $out
	echo "$subdir :" >> $out
	echo "	mkdir -p $subdir" >> $out
	echo >> $out
	echo "clean : $all_clean" >> $out
	echo >> $out
	echo "install : all" >> $out
	cat $iout >> $out
	echo >> $out
	cat $tout >> $out
}

function gcc_gen_shared()
{
	out="gcc-shared.mak"
	tout="temp"
	iout="temp_install"
	subdir="gcc"
	all_dep=""
	all_clean=""
	echo > $out
	echo > $tout
	echo > $iout

	libname="boost_regex-gcc-${boost_version}"
	opts="\$(C1)"
	gcc_gen_lib
	libname="boost_regex-gcc-d-${boost_version}"
	opts="\$(C2)"
	gcc_gen_lib
	
	
	cat > $out << EOF
# copyright John Maddock 2006-7
# Distributed under the Boost Software License, Version 1.0. 
# (See accompanying file LICENSE_1_0.txt or copy at 
# http://www.boost.org/LICENSE_1_0.txt.
#
# auto generated makefile for gcc compiler
#
# usage:
# make
#   brings libraries up to date
# make clean
#   deletes temporary object files (but not archives).
#

#
# the following environment variables are recognised:
# ICU_PATH= Path to ICU installation.
# CXXFLAGS= extra compiler options - note applies to all build variants
# INCLUDES= additional include directories
# LDFLAGS=  additional linker options
# LIBS=     additional library files

# compiler:
CXX?=g++
LINKER=\$(CXX) -shared

#
# compiler options for release build:
#
C1=-c -O2 -I../../.. -fPIC
#
# compiler options for debug build:
#
C2=-c -g -I../../.. -fPIC

ifeq "\$(ICU_PATH)" ""
\$(warning "Building Boost.Regex without ICU / Unicode support:")
\$(warning "Hint: set ICU_PATH on the nmake command line to point ")
\$(warning "to your ICU installation if you have one.")
else
ICU_CXXFLAGS= -DBOOST_HAS_ICU=1 -I\$(ICU_PATH)/include
ICU_LDFLAGS= -L\$(ICU_PATH)/lib
ICU_LIBS= -licui18n -licuuc
\$(warning "Building Boost.Regex with ICU in \$(ICU_PATH)")
endif

EOF
	echo "" >> $out
	echo "ALL_HEADER=$header" >> $out
	echo "" >> $out
	echo "all : $subdir $all_dep" >> $out
	echo >> $out
	echo "$subdir :" >> $out
	echo "	mkdir -p $subdir" >> $out
	echo >> $out
	echo "clean : $all_clean" >> $out
	echo >> $out
	echo "install : all" >> $out
	cat $iout >> $out
	echo >> $out
	cat $tout >> $out
}

#
# locate source files:
#
. common.sh

#
# generate gcc makefile:
gcc_gen
gcc_shared="yes"
gcc_gen_shared


#
# remove tmep files;
rm -f $tout $iout






