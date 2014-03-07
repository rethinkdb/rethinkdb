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


#######################################################################
#
# section for generic compiler
#
#######################################################################


function gen_gen_lib()
{
	all_dep="$all_dep $subdir $subdir/$libname ./$subdir/lib$libname.so"
#
# set up section comments:
	cat >> $tout << EOF
########################################################
#
# section for lib$libname.so
#
########################################################
EOF
#
#	process source files:
	all_obj=""
	for file in $src
	do
		obj=`echo "$file" | sed 's/.*src\/\(.*\)cpp/\1o/g'`
		obj="$subdir/$libname/$obj"
		all_obj="$all_obj $obj"
		echo "$obj: $file \$(ALL_HEADER)" >> $tout
		echo "	\$(CXX) \$(INCLUDES) -o $obj $opts \$(CXXFLAGS) $file" >> $tout
		echo "" >> $tout
	done
#
#	 now for the directories for this library:
	echo "$subdir/$libname : " >> $tout
	echo "	mkdir -p $subdir/$libname" >> $tout
	echo "" >> $tout
#
#	 now for the clean options for this library:
	all_clean="$all_clean $libname""_clean"
	echo "$libname"_clean : >> $tout
	echo "	rm -f $subdir/$libname/*.o" >> $tout
	echo "" >> $tout
#
#	 now for the main target for this library:
	echo ./$subdir/lib$libname.so : $all_obj >> $tout
	echo "	\$(LINKER) \$(LDFLAGS) -o $subdir/lib$libname.so $all_obj \$(LIBS)" >> $tout
	echo "" >> $tout
}

function gen_gen()
{
	out="generic.mak"
	tout="temp"
	iout="temp_install"
	subdir="\$(DIRNAME)"
	all_dep=""
	all_clean=""
	echo > $out
	echo > $tout
	echo > $iout

	libname="boost_regex"
	opts="\$(C1)"
	gen_gen_lib
	
	cat > $out << EOF
# copyright John Maddock 2006
# Distributed under the Boost Software License, Version 1.0. 
# (See accompanying file LICENSE_1_0.txt or copy at 
# http://www.boost.org/LICENSE_1_0.txt.
#
# auto generated makefile for generic compiler
#
# usage:
# make
#   brings libraries up to date
# make clean
#   deletes temporary object files (but not archives).
#

#
# the following environment variables are recognised:
# CXXFLAGS= extra compiler options - note applies to all build variants
# INCLUDES= additional include directories
# LDFLAGS=  additional linker options
# LIBS=     additional library files
# CXX=      compiler to use
# LINKER=   linker/archiver to use
# name of subdirectory to use for object/archive files:
DIRNAME=generic

#
# default compiler options for release build:
#
C1=-c -O2 -I../../../


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

. common.sh

#
# generate generic makefile:
gen_gen

#
# remove tmep files;
rm -f $tout $iout



