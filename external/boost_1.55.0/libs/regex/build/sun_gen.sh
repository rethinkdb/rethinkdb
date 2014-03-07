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
# section for Sun Forte 6.1 (5.1 compiler)
#
#######################################################################

function sun_gen_lib()
{
	all_dep="$all_dep $subdir/$libname $subdir/$libname.a"

#
# set up section comments:
	cat >> $tout << EOF
########################################################
#
# section for $libname.a
#
########################################################
EOF
#
#	process source files:
	all_obj=""
	all_lib_obj=""
	for file in $src
	do
		obj=`echo "$file" | sed 's/.*src\/\(.*\)cpp/\1o/g'`
		obj="$subdir/$libname/$obj"
		all_obj="$all_obj $obj"
		all_lib_obj="$all_lib_obj $obj"
		echo "$obj: $file \$(ALL_HEADER)" >> $tout
		echo "	CC -c \$(INCLUDES) $opts \$(CXXFLAGS) -o $obj $file" >> $tout
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
	echo "	rm -f $subdir/$libname/"'*.o' >> $tout
	echo "	rm -fr $subdir/$libname/\$(SUNWS_CACHE_NAME)" >> $tout
	echo "" >> $tout
#
#	 now for the main target for this library:
	echo $subdir/$libname.a : $all_obj >> $tout
	echo "	CC -xar \$(CXXFLAGS) \$(LDFLAGS) -o $subdir/$libname.a $all_lib_obj" >> $tout
	echo "" >> $tout
}

function sun_gen_dll()
{
	all_dep="$all_dep $subdir/shared_$libname $subdir/$libname.so"
#
# set up section comments:
	cat >> $tout << EOF
########################################################
#
# section for $libname.so
#
########################################################
EOF
#
#	process source files:
	all_obj=""
	for file in $src
	do
		obj=`echo "$file" | sed 's/.*src\/\(.*\)cpp/\1o/g'`
		obj="$subdir/shared_$libname/$obj"
		all_obj="$all_obj $obj"
		echo "$obj: $file \$(ALL_HEADER)" >> $tout
		echo "	CC -c \$(INCLUDES) $opts \$(CXXFLAGS) -o $obj $file" >> $tout
		echo "" >> $tout
	done
#
#	 now for the directories for this library:
	echo "$subdir/shared_$libname :" >> $tout
	echo "	mkdir -p $subdir/shared_$libname" >> $tout
	echo "" >> $tout
#
#	 now for the clean options for this library:
	all_clean="$all_clean $libname""_clean_shared"
	echo "$libname"_clean_shared : >> $tout
	echo "	rm -f $subdir/shared_$libname/"'*.o' >> $tout
	echo "	rm -fr $subdir/shared_$libname/\$(SUNWS_CACHE_NAME)" >> $tout
	echo "" >> $tout
#
#	 now for the main target for this library:
	echo $subdir/$libname.so : $all_obj >> $tout
	echo "	CC $opts -G -o $subdir/$libname.so \$(LDFLAGS) $all_obj \$(LIBS)" >> $tout
	echo "" >> $tout
}



function sun_gen()
{
	tout="temp"
	iout="temp_install"
	all_dep="$subdir"
	all_clean=""
	echo > $out
	echo > $tout
	rm -f $iout

	libname="libboost_regex\$(LIBSUFFIX)"
	opts="-O2 -I../../../"
	sun_gen_lib

	libname="libboost_regex_mt\$(LIBSUFFIX)"
	opts="-O2 -mt -I../../../"
	sun_gen_lib
	
	libname="libboost_regex\$(LIBSUFFIX)"
	opts="-KPIC -O2 -I../../../"
	sun_gen_dll

	libname="libboost_regex_mt\$(LIBSUFFIX)"
	opts="-KPIC -O2 -mt -I../../../"
	sun_gen_dll
	
	
	cat > $out << EOF
# copyright John Maddock 2006
# Distributed under the Boost Software License, Version 1.0. 
# (See accompanying file LICENSE_1_0.txt or copy at 
# http://www.boost.org/LICENSE_1_0.txt.
#
# auto generated makefile for Sun Forte 6.1
#
# usage:
# make
#   brings libraries up to date
# make clean
#   removes all temporary files.

#
# Add additional compiler options here:
#
CXXFLAGS=
#
# Add additional include directories here:
#
INCLUDES=
#
# add additional linker flags here:
#
LDFLAGS=
#
# add additional libraries to link to here:
#
LIBS=
#
# lib suffix string:
#
LIBSUFFIX=
#
# template cache path:
#
SUNWS_CACHE_NAME=SunWS_cache


EOF
	echo "" >> $out
	echo "ALL_HEADER=$header" >> $out
	echo "" >> $out
	echo "all : $all_dep" >> $out
	echo >> $out
	echo "clean : $all_clean" >> $out
	echo >> $out
	echo "install : all" >> $out
#	cat $iout >> $out
	echo >> $out
	echo $subdir : >> $out
	echo "	mkdir -p $subdir" >> $out
	echo "" >> $out

	cat $tout >> $out
}

. common.sh

#
# generate Sun 6.1 makefile:
out="sunpro.mak"
subdir="sunpro"
sun_gen


#
# remove tmep files;
rm -f $tout $iout




