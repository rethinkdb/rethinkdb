#! /bin/bash

# copyright John Maddock 2005
# Use, modification and distribution are subject to the 
# Boost Software License, Version 1.0. (See accompanying file 
# LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

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

#######################################################################
#
# section for C++ Builder
#
#######################################################################

function bcb_gen_lib()
{
	all_dep="$all_dep $subdir\\$libname $subdir\\$libname.lib $subdir\\$libname.exe"
	echo "	copy $subdir\\$libname.lib \$(BCROOT)\\lib" >> $iout
#
# set up section comments:
	cat >> $tout << EOF
########################################################
#
# section for $libname.lib
#
########################################################
EOF
#
#	process source files:
	all_obj=""
	all_lib_obj=""
	for file in $src
	do
		obj=`echo "$file" | sed 's/\(.*\)cpp/\1obj/g'`
		obj="$subdir\\$libname\\$obj"
		all_obj="$all_obj $obj"
		all_lib_obj="$all_lib_obj \"$obj\""
		echo "$obj: $file \$(ALL_HEADER)" >> $tout
		echo "	bcc32 @&&|" >> $tout
		echo "-c \$(INCLUDES) $opts \$(CXXFLAGS) -o$obj $file" >> $tout
		echo "|" >> $tout
		echo "" >> $tout
	done
#
#	 now for the directories for this library:
	echo "$subdir\\$libname : " >> $tout
	echo "	-@mkdir $subdir\\$libname" >> $tout
	echo "" >> $tout
#
#	 now for the clean options for this library:
	all_clean="$all_clean $libname""_clean"
	echo "$libname"_clean : >> $tout
	echo "	del $subdir\\$libname\\"'*.obj' >> $tout
	echo "	del $subdir\\$libname\\"'*.il?' >> $tout
	echo "	del $subdir\\$libname\\"'*.csm' >> $tout
	echo "	del $subdir\\$libname\\"'*.tds' >> $tout
	echo "" >> $tout
#
#	 now for the main target for this library:
	echo $subdir\\$libname.lib : $all_obj >> $tout
	echo "	tlib @&&|" >> $tout
	echo "/P128 /C /u /a \$(XSFLAGS) \"$subdir\\$libname.lib\" $all_lib_obj" >> $tout
	echo "|" >> $tout
	echo "" >> $tout
#  now the test program:
	echo "$subdir\\$libname.exe : main.cpp $subdir\\$libname.lib" >> $tout
	echo "	bcc32 \$(INCLUDES) $opts /DBOOST_LIB_DIAGNOSTIC=1 \$(CXXFLAGS) -L./$subdir -e./$subdir/$libname.exe main.cpp" >> $tout
	echo "   echo running test progam $subdir"'\'"$libname.exe" >> $tout
	echo "   $subdir"'\'"$libname.exe" >> $tout
	echo "" >> $tout
}

function bcb_gen_dll()
{
	all_dep="$all_dep $subdir\\$libname $subdir\\$libname.lib $subdir\\$libname.exe"
	echo "	copy $subdir\\$libname.lib \$(BCROOT)\\lib" >> $iout
	echo "	copy $subdir\\$libname.dll \$(BCROOT)\\bin" >> $iout
	echo "	copy $subdir\\$libname.tds \$(BCROOT)\\bin" >> $iout
#
# set up section comments:
	cat >> $tout << EOF
########################################################
#
# section for $libname.lib
#
########################################################
EOF
#
#	process source files:
	all_obj=""
	for file in $src
	do
		obj=`echo "$file" | sed 's/\(.*\)cpp/\1obj/g'`
		obj="$subdir\\$libname\\$obj"
		all_obj="$all_obj $obj"
		echo "$obj: $file \$(ALL_HEADER)" >> $tout
		echo "	bcc32 @&&|" >> $tout
		echo "-c \$(INCLUDES) $opts \$(CXXFLAGS) -DBOOST_DYN_LINK -o$obj $file" >> $tout
		echo "|" >> $tout
		echo "" >> $tout
	done
#
#	 now for the directories for this library:
	echo "$subdir\\$libname :" >> $tout
	echo "	-@mkdir $subdir\\$libname" >> $tout
	echo "" >> $tout
#
#	 now for the clean options for this library:
	all_clean="$all_clean $libname""_clean"
	echo "$libname"_clean : >> $tout
	echo "	del $subdir\\$libname\\"'*.obj' >> $tout
	echo "	del $subdir\\$libname\\"'*.il?' >> $tout
	echo "	del $subdir\\$libname\\"'*.csm' >> $tout
	echo "	del $subdir\\$libname\\"'*.tds' >> $tout
	echo "	del $subdir\\"'*.tds' >> $tout
	echo "" >> $tout
#
#	 now for the main target for this library:
	echo $subdir\\$libname.lib : $all_obj >> $tout
	echo "	bcc32 @&&|" >> $tout
	echo "-lw-dup -lw-dpl $opts -e$subdir\\$libname.dll \$(XLFLAGS) $all_obj \$(LIBS)" >> $tout
	echo "|" >> $tout
	echo "	implib -w $subdir\\$libname.lib $subdir\\$libname.dll" >> $tout
	echo "" >> $tout
#  now the test program:
	echo "$subdir\\$libname.exe : main.cpp $subdir\\$libname.lib" >> $tout
	echo "	bcc32 \$(INCLUDES) $opts /DBOOST_LIB_DIAGNOSTIC=1 \$(CXXFLAGS) -DBOOST_DYN_LINK -L./$subdir -e./$subdir/$libname.exe main.cpp" >> $tout
	echo "   echo running test program $subdir"'\'"$libname.exe" >> $tout
	echo "   $subdir"'\'"$libname.exe" >> $tout
	echo "" >> $tout
}



function bcb_gen()
{
	tout="temp"
	iout="temp_install"
	all_dep="$subdir"
	all_clean=""
	echo > $out
	echo > $tout
	rm -f $iout

	libname="liblink_test-${subdir}-s-${boost_version}"
	opts="-tWM- -D_NO_VCL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8037 -w-8057 -DSTRICT; -I\$(BCROOT)\include;../../../../"
	bcb_gen_lib

	libname="liblink_test-${subdir}-mt-s-${boost_version}"
	opts="-tWM -D_NO_VCL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I\$(BCROOT)\include;../../../../"
	bcb_gen_lib
	
	libname="link_test-${subdir}-mt-${boost_version}"
	opts="-tWD -tWM -tWR -D_NO_VCL -D_RTLDLL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I\$(BCROOT)\include;../../../../ -L\$(BCROOT)\lib;\$(BCROOT)\lib\release;"
	bcb_gen_dll

	libname="link_test-${subdir}-${boost_version}"
	opts="-tWD -tWR -tWM- -D_NO_VCL -D_RTLDLL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I\$(BCROOT)\include;../../../../ -L\$(BCROOT)\lib;\$(BCROOT)\lib\release;"
	bcb_gen_dll
	
	libname="liblink_test-${subdir}-mt-${boost_version}"
	opts="-tWD -tWM -tWR -DBOOST_REGEX_STATIC_LINK -D_NO_VCL -D_RTLDLL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I\$(BCROOT)\include;../../../../ -L\$(BCROOT)\lib;\$(BCROOT)\lib\release;"
	bcb_gen_lib

	libname="liblink_test-${subdir}-${boost_version}"
	opts="-tWD -tWR -tWM- -DBOOST_REGEX_STATIC_LINK -D_NO_VCL -D_RTLDLL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I\$(BCROOT)\include;../../../../ -L\$(BCROOT)\lib;\$(BCROOT)\lib\release;"
	bcb_gen_lib
	#
	# debug versions:
	libname="liblink_test-${subdir}-sd-${boost_version}"
	opts="-tWM- -D_NO_VCL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8037 -w-8057 -DSTRICT; -I\$(BCROOT)\include;../../../../"
	bcb_gen_lib

	libname="liblink_test-${subdir}-mt-sd-${boost_version}"
	opts="-tWM -D_NO_VCL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I\$(BCROOT)\include;../../../../"
	bcb_gen_lib
	
	libname="link_test-${subdir}-mt-d-${boost_version}"
	opts="-tWD -tWM -tWR -D_NO_VCL -D_RTLDLL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I\$(BCROOT)\include;../../../../ -L\$(BCROOT)\lib;\$(BCROOT)\lib\release;"
	bcb_gen_dll

	libname="link_test-${subdir}-d-${boost_version}"
	opts="-tWD -tWR -tWM- -D_NO_VCL -D_RTLDLL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I\$(BCROOT)\include;../../../../ -L\$(BCROOT)\lib;\$(BCROOT)\lib\release;"
	bcb_gen_dll
	
	libname="liblink_test-${subdir}-mt-d-${boost_version}"
	opts="-tWD -tWM -tWR -DBOOST_REGEX_STATIC_LINK -D_NO_VCL -D_RTLDLL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I\$(BCROOT)\include;../../../../ -L\$(BCROOT)\lib;\$(BCROOT)\lib\release;"
	bcb_gen_lib

	libname="liblink_test-${subdir}-d-${boost_version}"
	opts="-tWD -tWR -tWM- -DBOOST_REGEX_STATIC_LINK -D_NO_VCL -D_RTLDLL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I\$(BCROOT)\include;../../../../ -L\$(BCROOT)\lib;\$(BCROOT)\lib\release;"
	bcb_gen_lib
	
	
	cat > $out << EOF
# copyright John Maddock 2005
# Use, modification and distribution are subject to the 
# Boost Software License, Version 1.0. (See accompanying file 
# LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#
# auto generated makefile for C++ Builder
#
# usage:
# make
#   brings libraries up to date
# make install
#   brings libraries up to date and copies binaries to your C++ Builder /lib and /bin directories (recomended)
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
XLFLAGS=
#
# add additional libraries to link to here:
#
LIBS=
#
# add additional static-library creation flags here:
#
XSFLAGS=

!ifndef BCROOT
BCROOT=\$(MAKEDIR)\\..
!endif

EOF
	echo "" >> $out
	echo "ALL_HEADER=$header" >> $out
	echo "" >> $out
	echo "all : $all_dep" >> $out
	echo >> $out
	echo "clean : $all_clean" >> $out
	echo >> $out
	echo "install : all" >> $out
	cat $iout >> $out
	echo >> $out
	echo $subdir : >> $out
	echo "	-@mkdir $subdir" >> $out
	echo "" >> $out

	cat $tout >> $out
}

. common.sh

#
# generate C++ Builder 6 files:
out="borland.mak"
subdir="borland"
has_stlport="yes"
bcb_gen








