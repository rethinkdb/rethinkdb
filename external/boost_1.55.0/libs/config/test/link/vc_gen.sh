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
# extra debug /RTc options:
debug_extra=""

function vc6_gen_lib()
{
	all_dep="$all_dep $libname""_dir ./$subdir/$libname.lib ./$subdir/$libname.exe"
	echo "	copy $subdir\\$libname.lib "'"$'"(MSVCDIR)\\lib"'"' >> $iout
	if test $debug == "yes"; then
		echo "	copy $subdir\\$libname.pdb "'"$'"(MSVCDIR)\\lib"'"' >> $iout
	fi
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
		obj="$subdir/$libname/$obj"
		all_obj="$all_obj $obj"
		echo "$obj: $file \$(ALL_HEADER)" >> $tout
		echo "	cl /c \$(INCLUDES) $opts \$(CXXFLAGS) -Y- -Fo./$subdir/$libname/ -Fd$subdir/$libname.pdb $file" >> $tout
		echo "" >> $tout
	done
#
#	 now for the directories for this library:
	echo "$libname"_dir : >> $tout
	echo "	@if not exist \"$subdir\\$libname\\\$(NULL)\" mkdir $subdir\\$libname" >> $tout
	echo "" >> $tout
#
#	 now for the clean options for this library:
	all_clean="$all_clean $libname""_clean"
	echo "$libname"_clean : >> $tout
	echo "	del $subdir\\$libname\\"'*.obj' >> $tout
	echo "	del $subdir\\$libname\\"'*.idb' >> $tout
	echo "	del $subdir\\$libname\\"'*.exp' >> $tout
	echo "	del $subdir\\$libname\\"'*.pch' >> $tout
	echo "" >> $tout
#
#	 now for the main target for this library:
	echo ./$subdir/$libname.lib : $all_obj >> $tout
	echo "	link -lib /nologo /out:$subdir/$libname.lib \$(XSFLAGS) $all_obj" >> $tout
	echo "" >> $tout
#  now the test program:
	echo ./$subdir/$libname.exe : main.cpp ./$subdir/$libname.lib >> $tout
	echo "	cl \$(INCLUDES) $opts /DBOOST_LIB_DIAGNOSTIC=1 \$(CXXFLAGS) -o ./$subdir/$libname.exe main.cpp /link /LIBPATH:./$subdir" >> $tout
	echo "   $subdir"'\'"$libname.exe" >> $tout
	echo "" >> $tout
}

function vc6_gen_dll()
{
	all_dep="$all_dep $libname""_dir ./$subdir/$libname.lib ./$subdir/$libname.exe"
	echo "	copy $subdir\\$libname.lib "'"$'"(MSVCDIR)\\lib"'"' >> $iout
	echo "	copy $subdir\\$libname.dll "'"$'"(MSVCDIR)\\bin"'"' >> $iout
	if test $debug == "yes"; then
		echo "	copy $subdir\\$libname.pdb "'"$'"(MSVCDIR)\\lib"'"' >> $iout
	fi
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
		obj="$subdir/$libname/$obj"
		all_obj="$all_obj $obj"
		echo "$obj: $file \$(ALL_HEADER)" >> $tout
		echo "	cl /c \$(INCLUDES) $opts \$(CXXFLAGS) -Y- -Fo./$subdir/$libname/ -Fd$subdir/$libname.pdb $file" >> $tout
		echo "" >> $tout
	done
#
#	 now for the directories for this library:
	echo "$libname"_dir : >> $tout
	echo "	@if not exist \"$subdir\\$libname\\\$(NULL)\" mkdir $subdir\\$libname" >> $tout
	echo "" >> $tout
#
#	 now for the clean options for this library:
	all_clean="$all_clean $libname""_clean"
	echo "$libname"_clean : >> $tout
	echo "	del $subdir\\$libname\\"'*.obj' >> $tout
	echo "	del $subdir\\$libname\\"'*.idb' >> $tout
	echo "	del $subdir\\$libname\\"'*.exp' >> $tout
	echo "	del $subdir\\$libname\\"'*.pch' >> $tout
	echo "" >> $tout
#
#	 now for the main target for this library:
	echo ./$subdir/$libname.lib : $all_obj >> $tout
	echo "	link kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /incremental:yes /pdb:\"$subdir/$libname.pdb\" /debug /machine:I386 /out:\"$subdir/$libname.dll\" /implib:\"$subdir/$libname.lib\" /LIBPATH:\$(STLPORT_PATH)\\lib \$(XLFLAGS) $all_obj" >> $tout
	echo "" >> $tout
#  now the test program:
	echo ./$subdir/$libname.exe : main.cpp ./$subdir/$libname.lib >> $tout
	echo "	cl \$(INCLUDES) $opts /DBOOST_LIB_DIAGNOSTIC=1 \$(CXXFLAGS) -o ./$subdir/$libname.exe main.cpp /link /LIBPATH:./$subdir" >> $tout
	echo "   $subdir"'\'"$libname.exe" >> $tout
	echo "" >> $tout
}

is_stlport="no"

function vc6_gen()
{
	debug="no"
	tout="temp"
	iout="temp_install"
	all_dep="main_dir"
	all_clean=""
	echo > $out
	echo > $tout
	rm -f $iout
	
	libname="liblink_test-${subdir}-s-${boost_version}"
	opts='/nologo /ML /W3 /GX /O2 /GB /GF /Gy /I..\..\..\..\ /DWIN32 /DNDEBUG /D_MBCS /D_LIB /FD'
	vc6_gen_lib
	
	libname="liblink_test-${subdir}-mt-s-${boost_version}"
	opts='/nologo /MT /W3 /GX /O2 /GB /GF /Gy /I..\..\..\..\ /D_MT /DWIN32 /DNDEBUG /D_MBCS /D_LIB /FD '
	vc6_gen_lib
	
	debug="yes"
	libname="liblink_test-${subdir}-sgd-${boost_version}"
	opts='/nologo /MLd /W3 /Gm /GX /Zi /Od /I..\..\..\..\ /DWIN32 /D_DEBUG /D_MBCS /D_LIB /FD '"$debug_extra"'  '
	vc6_gen_lib
	
	libname="liblink_test-${subdir}-mt-sgd-${boost_version}"
	opts='/nologo /MTd /W3 /Gm /GX /Zi /Od /I..\..\..\..\ /DWIN32 /D_MT /D_DEBUG /D_MBCS /D_LIB /FD '"$debug_extra"' '
	vc6_gen_lib
	
	libname="link_test-${subdir}-mt-gd-${boost_version}"
	opts='/nologo /MDd /W3 /Gm /GX /Zi /Od /I..\..\..\..\ /D_DEBUG /DBOOST_DYN_LINK /DWIN32 /D_WINDOWS /D_MBCS /DUSRDLL /FD '"$debug_extra"' '
	vc6_gen_dll
	
	debug="no"
	opts='/nologo /MD /W3 /GX /O2 /GB /GF /Gy /I..\..\..\..\ /DBOOST_DYN_LINK /DNDEBUG /DWIN32 /D_WINDOWS /D_MBCS /D_USRDLL /FD '
	libname="link_test-${subdir}-mt-${boost_version}"
	vc6_gen_dll
	
	debug="no"
	opts='/nologo /MD /W3 /GX /O2 /GB /GF /Gy /I..\..\..\..\ /DBOOST_REGEX_STATIC_LINK /DNDEBUG /DWIN32 /D_WINDOWS /D_MBCS /D_USRDLL /FD '
	libname="liblink_test-${subdir}-mt-${boost_version}"
	vc6_gen_lib
	
	debug="yes"
	libname="liblink_test-${subdir}-mt-gd-${boost_version}"
	opts='/nologo /MDd /W3 /Gm /GX /Zi /Od /I..\..\..\..\ /DBOOST_REGEX_STATIC_LINK /D_DEBUG /DWIN32 /D_WINDOWS /D_MBCS /DUSRDLL /FD '"$debug_extra"' '
	vc6_gen_lib
	
	cat > $out << EOF
# copyright John Maddock 2005
# Use, modification and distribution are subject to the 
# Boost Software License, Version 1.0. (See accompanying file 
# LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#
# auto generated makefile for VC6 compiler
#
# usage:
# make
#   brings libraries up to date
# make install
#   brings libraries up to date and copies binaries to your VC6 /lib and /bin directories (recomended)
#

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
# add additional static-library creation flags here:
#
XSFLAGS=

!IF "\$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF "\$(MSVCDIR)" == ""
!ERROR Variable MSVCDIR not set.
!ENDIF

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
	echo main_dir : >> $out
	echo "	@if not exist \"$subdir\\\$(NULL)\" mkdir $subdir" >> $out
	echo "" >> $out

	cat $tout >> $out
}

function vc6_stlp_gen()
{
	debug="no"
	tout="temp"
	iout="temp_install"
	all_dep="main_dir"
	all_clean=""
	echo > $out
	echo > $tout
	rm -f $iout
	
	libname="liblink_test-${subdir}-mt-s-${boost_version}"
	opts='/nologo /MT /W3 /GX /O2 /GB /GF /Gy /I$(STLPORT_PATH)\stlport /I..\..\..\..\ /D_MT /DWIN32 /DNDEBUG /D_MBCS /D_LIB '
	vc6_gen_lib
	
	debug="true"
	libname="liblink_test-${subdir}-mt-sgd-${boost_version}"
	opts='/nologo /MTd /W3 /Gm /GX /Zi /Od /I$(STLPORT_PATH)\stlport /I..\..\..\..\ /DWIN32 /D_MT /D_DEBUG /D_MBCS /D_LIB '"$debug_extra"' '
	vc6_gen_lib
	
	libname="link_test-${subdir}-mt-gd-${boost_version}"
	opts='/nologo /MDd /W3 /Gm /GX /Zi /Od /I$(STLPORT_PATH)\stlport /I..\..\..\..\ /DBOOST_DYN_LINK /D_DEBUG /DWIN32 /D_WINDOWS /D_MBCS /DUSRDLL '"$debug_extra"' '
	vc6_gen_dll
	
	debug="no"
	opts='/nologo /MD /W3 /GX /O2 /GB /GF /I$(STLPORT_PATH)\stlport /Gy /I..\..\..\..\ /DBOOST_DYN_LINK /DNDEBUG /DWIN32 /D_WINDOWS /D_MBCS /D_USRDLL '
	libname="link_test-${subdir}-mt-${boost_version}"
	vc6_gen_dll
	
	debug="no"
	opts='/nologo /MD /W3 /GX /O2 /GB /GF /Gy /I$(STLPORT_PATH)\stlport /I..\..\..\..\ /DBOOST_REGEX_STATIC_LINK /DNDEBUG /DWIN32 /D_WINDOWS /D_MBCS /D_USRDLL '
	libname="liblink_test-${subdir}-mt-${boost_version}"
	vc6_gen_lib
	
	debug="true"
	libname="liblink_test-${subdir}-mt-gd-${boost_version}"
	opts='/nologo /MDd /W3 /Gm /GX /Zi /Od /I$(STLPORT_PATH)\stlport /I..\..\..\..\ /DBOOST_REGEX_STATIC_LINK /D_DEBUG /DWIN32 /D_WINDOWS /D_MBCS /DUSRDLL '"$debug_extra"' '
	vc6_gen_lib

#  debug STLPort mode:
#  not yet supported by bjam?
	debug="yes"
	opts='/nologo /MDd /W3 /Gm /GX /Zi /Od /I$(STLPORT_PATH)\stlport /I..\..\..\..\ /DBOOST_DYN_LINK /D__STL_DEBUG /D_STLP_DEBUG /D_DEBUG /DWIN32 /D_WINDOWS /D_MBCS /DUSRDLL '"$debug_extra"' '
	libname="link_test-${subdir}-mt-pgd-${boost_version}"
	vc6_gen_dll
	libname="liblink_test-${subdir}-mt-spgd-${boost_version}"
	opts='/nologo /MTd /W3 /Gm /GX /Zi /Od /I$(STLPORT_PATH)\stlport /I..\..\..\..\ /D__STL_DEBUG /D_STLP_DEBUG /DWIN32 /D_MT /D_DEBUG /D_MBCS /D_LIB '"$debug_extra"' '
	vc6_gen_lib
	opts='/nologo /MDd /W3 /Gm /GX /Zi /Od /I$(STLPORT_PATH)\stlport /I..\..\..\..\ /DBOOST_REGEX_STATIC_LINK /D__STL_DEBUG /D_STLP_DEBUG /D_DEBUG /DWIN32 /D_WINDOWS /D_MBCS /DUSRDLL '"$debug_extra"' '
	libname="liblink_test-${subdir}-mt-pgd-${boost_version}"
	vc6_gen_lib
	
	cat > $out << EOF
# copyright John Maddock 2005
# Use, modification and distribution are subject to the 
# Boost Software License, Version 1.0. (See accompanying file 
# LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#
# auto generated makefile for VC6+STLPort
#
# usage:
# make
#   brings libraries up to date
# make install
#   brings libraries up to date and copies binaries to your VC6 /lib and /bin directories (recomended)
#

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
# add additional static-library creation flags here:
#
XSFLAGS=

!IF "\$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF "\$(MSVCDIR)" == ""
!ERROR Variable MSVCDIR not set.
!ENDIF

!IF "\$(STLPORT_PATH)" == ""
!ERROR Variable STLPORT_PATH not set.
!ENDIF

EOF
	echo "" >> $out
	echo "ALL_HEADER=$header" >> $out
	echo "" >> $out
	echo "all : $all_dep" >> $out
	echo >> $out
	echo "clean : $all_clean" >> $out
	echo >> $out
	echo "install : stlport_check all" >> $out
	cat $iout >> $out
	echo >> $out
	echo main_dir : >> $out
	echo "	@if not exist \"$subdir\\\$(NULL)\" mkdir $subdir" >> $out
	echo "" >> $out
	echo 'stlport_check : $(STLPORT_PATH)\stlport\string' >> $out
	echo "	echo" >> $out
	echo "" >> $out

	cat $tout >> $out
}


. common.sh

#
# generate vc6 makefile:
debug_extra="/GX"
out="vc6.mak"
subdir="vc6"
vc6_gen
#
# generate vc6-stlport makefile:
is_stlport="yes"
out="vc6-stlport.mak"
no_single="yes"
subdir="vc6-stlport"
vc6_stlp_gen
#
# generate vc7 makefile:
debug_extra="/GX /RTC1"
is_stlport="no"
out="vc7.mak"
no_single="no"
subdir="vc7"
vc6_gen
#
# generate vc7-stlport makefile:
is_stlport="yes"
out="vc7-stlport.mak"
no_single="yes"
subdir="vc7-stlport"
vc6_stlp_gen
#
# generate vc71 makefile:
is_stlport="no"
out="vc71.mak"
no_single="no"
subdir="vc71"
vc6_gen
#
# generate vc71-stlport makefile:
is_stlport="yes"
out="vc71-stlport.mak"
no_single="yes"
subdir="vc71-stlport"
vc6_stlp_gen


#
# remove tmep files;
rm -f $tout $iout











