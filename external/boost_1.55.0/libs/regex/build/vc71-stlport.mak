# copyright John Maddock 2006
# Distributed under the Boost Software License, Version 1.0. 
# (See accompanying file LICENSE_1_0.txt or copy at 
# http://www.boost.org/LICENSE_1_0.txt.
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
# ICU setup:
#
ICU_PATH=
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

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF "$(MSVCDIR)" == ""
!ERROR Variable MSVCDIR not set.
!ENDIF

!IF "$(STLPORT_PATH)" == ""
!ERROR Variable STLPORT_PATH not set.
!ENDIF

!IF "$(ICU_PATH)" == ""
ICU_COMPILE_OPTS=
ICU_LINK_OPTS=
!MESSAGE Building Boost.Regex without ICU / Unicode support:
!MESSAGE Hint: set ICU_PATH on the nmake command line to point 
!MESSAGE to your ICU installation if you have one.
!ELSE
ICU_COMPILE_OPTS= -DBOOST_HAS_ICU=1 -I"$(ICU_PATH)\include"
ICU_LINK_OPTS= /LIBPATH:"$(ICU_PATH)\lib" icuin.lib icuuc.lib
!MESSAGE Building Boost.Regex with ICU in $(ICU_PATH)
!ENDIF


ALL_HEADER= ../../../boost/regex/config.hpp ../../../boost/regex/icu.hpp ../../../boost/regex/pattern_except.hpp ../../../boost/regex/regex_traits.hpp ../../../boost/regex/user.hpp ../../../boost/regex/v4/basic_regex.hpp ../../../boost/regex/v4/basic_regex_creator.hpp ../../../boost/regex/v4/basic_regex_parser.hpp ../../../boost/regex/v4/c_regex_traits.hpp ../../../boost/regex/v4/char_regex_traits.hpp ../../../boost/regex/v4/cpp_regex_traits.hpp ../../../boost/regex/v4/cregex.hpp ../../../boost/regex/v4/error_type.hpp ../../../boost/regex/v4/fileiter.hpp ../../../boost/regex/v4/instances.hpp ../../../boost/regex/v4/iterator_category.hpp ../../../boost/regex/v4/iterator_traits.hpp ../../../boost/regex/v4/match_flags.hpp ../../../boost/regex/v4/match_results.hpp ../../../boost/regex/v4/mem_block_cache.hpp ../../../boost/regex/v4/perl_matcher.hpp ../../../boost/regex/v4/perl_matcher_common.hpp ../../../boost/regex/v4/perl_matcher_non_recursive.hpp ../../../boost/regex/v4/perl_matcher_recursive.hpp ../../../boost/regex/v4/primary_transform.hpp ../../../boost/regex/v4/protected_call.hpp ../../../boost/regex/v4/regbase.hpp ../../../boost/regex/v4/regex.hpp ../../../boost/regex/v4/regex_format.hpp ../../../boost/regex/v4/regex_fwd.hpp ../../../boost/regex/v4/regex_grep.hpp ../../../boost/regex/v4/regex_iterator.hpp ../../../boost/regex/v4/regex_match.hpp ../../../boost/regex/v4/regex_merge.hpp ../../../boost/regex/v4/regex_raw_buffer.hpp ../../../boost/regex/v4/regex_replace.hpp ../../../boost/regex/v4/regex_search.hpp ../../../boost/regex/v4/regex_split.hpp ../../../boost/regex/v4/regex_token_iterator.hpp ../../../boost/regex/v4/regex_traits.hpp ../../../boost/regex/v4/regex_traits_defaults.hpp ../../../boost/regex/v4/regex_workaround.hpp ../../../boost/regex/v4/states.hpp ../../../boost/regex/v4/sub_match.hpp ../../../boost/regex/v4/syntax_type.hpp ../../../boost/regex/v4/u32regex_iterator.hpp ../../../boost/regex/v4/u32regex_token_iterator.hpp ../../../boost/regex/v4/w32_regex_traits.hpp ../../../boost/regex/config/borland.hpp ../../../boost/regex/config/cwchar.hpp

all : main_dir libboost_regex-vc71-mt-sp-1_53_dir ./vc71-stlport/libboost_regex-vc71-mt-sp-1_53.lib boost_regex-vc71-mt-p-1_53_dir ./vc71-stlport/boost_regex-vc71-mt-p-1_53.lib libboost_regex-vc71-mt-p-1_53_dir ./vc71-stlport/libboost_regex-vc71-mt-p-1_53.lib boost_regex-vc71-mt-gdp-1_53_dir ./vc71-stlport/boost_regex-vc71-mt-gdp-1_53.lib libboost_regex-vc71-mt-sgdp-1_53_dir ./vc71-stlport/libboost_regex-vc71-mt-sgdp-1_53.lib libboost_regex-vc71-mt-gdp-1_53_dir ./vc71-stlport/libboost_regex-vc71-mt-gdp-1_53.lib

clean :  libboost_regex-vc71-mt-sp-1_53_clean boost_regex-vc71-mt-p-1_53_clean libboost_regex-vc71-mt-p-1_53_clean boost_regex-vc71-mt-gdp-1_53_clean libboost_regex-vc71-mt-sgdp-1_53_clean libboost_regex-vc71-mt-gdp-1_53_clean

install : stlport_check all
	copy vc71-stlport\libboost_regex-vc71-mt-sp-1_53.lib "$(MSVCDIR)\lib"
	copy vc71-stlport\boost_regex-vc71-mt-p-1_53.lib "$(MSVCDIR)\lib"
	copy vc71-stlport\boost_regex-vc71-mt-p-1_53.dll "$(MSVCDIR)\bin"
	copy vc71-stlport\libboost_regex-vc71-mt-p-1_53.lib "$(MSVCDIR)\lib"
	copy vc71-stlport\boost_regex-vc71-mt-gdp-1_53.lib "$(MSVCDIR)\lib"
	copy vc71-stlport\boost_regex-vc71-mt-gdp-1_53.dll "$(MSVCDIR)\bin"
	copy vc71-stlport\boost_regex-vc71-mt-gdp-1_53.pdb "$(MSVCDIR)\lib"
	copy vc71-stlport\libboost_regex-vc71-mt-sgdp-1_53.lib "$(MSVCDIR)\lib"
	copy vc71-stlport\libboost_regex-vc71-mt-sgdp-1_53.pdb "$(MSVCDIR)\lib"
	copy vc71-stlport\libboost_regex-vc71-mt-gdp-1_53.lib "$(MSVCDIR)\lib"
	copy vc71-stlport\libboost_regex-vc71-mt-gdp-1_53.pdb "$(MSVCDIR)\lib"

main_dir :
	@if not exist "vc71-stlport\$(NULL)" mkdir vc71-stlport

stlport_check : "$(STLPORT_PATH)\stlport\string"
	echo


########################################################
#
# section for libboost_regex-vc71-mt-sp-1_53.lib
#
########################################################
vc71-stlport/libboost_regex-vc71-mt-sp-1_53/c_regex_traits.obj: ../src/c_regex_traits.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MT /W3 /GX /O2 /GB /GF /Gy /I"$(STLPORT_PATH)\stlport" /I..\..\..\ /D_MT /DWIN32 /DNDEBUG /D_MBCS /D_LIB /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/libboost_regex-vc71-mt-sp-1_53/ -Fdvc71-stlport/libboost_regex-vc71-mt-sp-1_53.pdb ../src/c_regex_traits.cpp

vc71-stlport/libboost_regex-vc71-mt-sp-1_53/cpp_regex_traits.obj: ../src/cpp_regex_traits.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MT /W3 /GX /O2 /GB /GF /Gy /I"$(STLPORT_PATH)\stlport" /I..\..\..\ /D_MT /DWIN32 /DNDEBUG /D_MBCS /D_LIB /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/libboost_regex-vc71-mt-sp-1_53/ -Fdvc71-stlport/libboost_regex-vc71-mt-sp-1_53.pdb ../src/cpp_regex_traits.cpp

vc71-stlport/libboost_regex-vc71-mt-sp-1_53/cregex.obj: ../src/cregex.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MT /W3 /GX /O2 /GB /GF /Gy /I"$(STLPORT_PATH)\stlport" /I..\..\..\ /D_MT /DWIN32 /DNDEBUG /D_MBCS /D_LIB /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/libboost_regex-vc71-mt-sp-1_53/ -Fdvc71-stlport/libboost_regex-vc71-mt-sp-1_53.pdb ../src/cregex.cpp

vc71-stlport/libboost_regex-vc71-mt-sp-1_53/fileiter.obj: ../src/fileiter.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MT /W3 /GX /O2 /GB /GF /Gy /I"$(STLPORT_PATH)\stlport" /I..\..\..\ /D_MT /DWIN32 /DNDEBUG /D_MBCS /D_LIB /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/libboost_regex-vc71-mt-sp-1_53/ -Fdvc71-stlport/libboost_regex-vc71-mt-sp-1_53.pdb ../src/fileiter.cpp

vc71-stlport/libboost_regex-vc71-mt-sp-1_53/icu.obj: ../src/icu.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MT /W3 /GX /O2 /GB /GF /Gy /I"$(STLPORT_PATH)\stlport" /I..\..\..\ /D_MT /DWIN32 /DNDEBUG /D_MBCS /D_LIB /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/libboost_regex-vc71-mt-sp-1_53/ -Fdvc71-stlport/libboost_regex-vc71-mt-sp-1_53.pdb ../src/icu.cpp

vc71-stlport/libboost_regex-vc71-mt-sp-1_53/instances.obj: ../src/instances.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MT /W3 /GX /O2 /GB /GF /Gy /I"$(STLPORT_PATH)\stlport" /I..\..\..\ /D_MT /DWIN32 /DNDEBUG /D_MBCS /D_LIB /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/libboost_regex-vc71-mt-sp-1_53/ -Fdvc71-stlport/libboost_regex-vc71-mt-sp-1_53.pdb ../src/instances.cpp

vc71-stlport/libboost_regex-vc71-mt-sp-1_53/posix_api.obj: ../src/posix_api.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MT /W3 /GX /O2 /GB /GF /Gy /I"$(STLPORT_PATH)\stlport" /I..\..\..\ /D_MT /DWIN32 /DNDEBUG /D_MBCS /D_LIB /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/libboost_regex-vc71-mt-sp-1_53/ -Fdvc71-stlport/libboost_regex-vc71-mt-sp-1_53.pdb ../src/posix_api.cpp

vc71-stlport/libboost_regex-vc71-mt-sp-1_53/regex.obj: ../src/regex.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MT /W3 /GX /O2 /GB /GF /Gy /I"$(STLPORT_PATH)\stlport" /I..\..\..\ /D_MT /DWIN32 /DNDEBUG /D_MBCS /D_LIB /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/libboost_regex-vc71-mt-sp-1_53/ -Fdvc71-stlport/libboost_regex-vc71-mt-sp-1_53.pdb ../src/regex.cpp

vc71-stlport/libboost_regex-vc71-mt-sp-1_53/regex_debug.obj: ../src/regex_debug.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MT /W3 /GX /O2 /GB /GF /Gy /I"$(STLPORT_PATH)\stlport" /I..\..\..\ /D_MT /DWIN32 /DNDEBUG /D_MBCS /D_LIB /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/libboost_regex-vc71-mt-sp-1_53/ -Fdvc71-stlport/libboost_regex-vc71-mt-sp-1_53.pdb ../src/regex_debug.cpp

vc71-stlport/libboost_regex-vc71-mt-sp-1_53/regex_raw_buffer.obj: ../src/regex_raw_buffer.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MT /W3 /GX /O2 /GB /GF /Gy /I"$(STLPORT_PATH)\stlport" /I..\..\..\ /D_MT /DWIN32 /DNDEBUG /D_MBCS /D_LIB /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/libboost_regex-vc71-mt-sp-1_53/ -Fdvc71-stlport/libboost_regex-vc71-mt-sp-1_53.pdb ../src/regex_raw_buffer.cpp

vc71-stlport/libboost_regex-vc71-mt-sp-1_53/regex_traits_defaults.obj: ../src/regex_traits_defaults.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MT /W3 /GX /O2 /GB /GF /Gy /I"$(STLPORT_PATH)\stlport" /I..\..\..\ /D_MT /DWIN32 /DNDEBUG /D_MBCS /D_LIB /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/libboost_regex-vc71-mt-sp-1_53/ -Fdvc71-stlport/libboost_regex-vc71-mt-sp-1_53.pdb ../src/regex_traits_defaults.cpp

vc71-stlport/libboost_regex-vc71-mt-sp-1_53/static_mutex.obj: ../src/static_mutex.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MT /W3 /GX /O2 /GB /GF /Gy /I"$(STLPORT_PATH)\stlport" /I..\..\..\ /D_MT /DWIN32 /DNDEBUG /D_MBCS /D_LIB /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/libboost_regex-vc71-mt-sp-1_53/ -Fdvc71-stlport/libboost_regex-vc71-mt-sp-1_53.pdb ../src/static_mutex.cpp

vc71-stlport/libboost_regex-vc71-mt-sp-1_53/usinstances.obj: ../src/usinstances.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MT /W3 /GX /O2 /GB /GF /Gy /I"$(STLPORT_PATH)\stlport" /I..\..\..\ /D_MT /DWIN32 /DNDEBUG /D_MBCS /D_LIB /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/libboost_regex-vc71-mt-sp-1_53/ -Fdvc71-stlport/libboost_regex-vc71-mt-sp-1_53.pdb ../src/usinstances.cpp

vc71-stlport/libboost_regex-vc71-mt-sp-1_53/w32_regex_traits.obj: ../src/w32_regex_traits.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MT /W3 /GX /O2 /GB /GF /Gy /I"$(STLPORT_PATH)\stlport" /I..\..\..\ /D_MT /DWIN32 /DNDEBUG /D_MBCS /D_LIB /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/libboost_regex-vc71-mt-sp-1_53/ -Fdvc71-stlport/libboost_regex-vc71-mt-sp-1_53.pdb ../src/w32_regex_traits.cpp

vc71-stlport/libboost_regex-vc71-mt-sp-1_53/wc_regex_traits.obj: ../src/wc_regex_traits.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MT /W3 /GX /O2 /GB /GF /Gy /I"$(STLPORT_PATH)\stlport" /I..\..\..\ /D_MT /DWIN32 /DNDEBUG /D_MBCS /D_LIB /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/libboost_regex-vc71-mt-sp-1_53/ -Fdvc71-stlport/libboost_regex-vc71-mt-sp-1_53.pdb ../src/wc_regex_traits.cpp

vc71-stlport/libboost_regex-vc71-mt-sp-1_53/wide_posix_api.obj: ../src/wide_posix_api.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MT /W3 /GX /O2 /GB /GF /Gy /I"$(STLPORT_PATH)\stlport" /I..\..\..\ /D_MT /DWIN32 /DNDEBUG /D_MBCS /D_LIB /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/libboost_regex-vc71-mt-sp-1_53/ -Fdvc71-stlport/libboost_regex-vc71-mt-sp-1_53.pdb ../src/wide_posix_api.cpp

vc71-stlport/libboost_regex-vc71-mt-sp-1_53/winstances.obj: ../src/winstances.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MT /W3 /GX /O2 /GB /GF /Gy /I"$(STLPORT_PATH)\stlport" /I..\..\..\ /D_MT /DWIN32 /DNDEBUG /D_MBCS /D_LIB /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/libboost_regex-vc71-mt-sp-1_53/ -Fdvc71-stlport/libboost_regex-vc71-mt-sp-1_53.pdb ../src/winstances.cpp

libboost_regex-vc71-mt-sp-1_53_dir :
	@if not exist "vc71-stlport\libboost_regex-vc71-mt-sp-1_53\$(NULL)" mkdir vc71-stlport\libboost_regex-vc71-mt-sp-1_53

libboost_regex-vc71-mt-sp-1_53_clean :
	del vc71-stlport\libboost_regex-vc71-mt-sp-1_53\*.obj
	del vc71-stlport\libboost_regex-vc71-mt-sp-1_53\*.idb
	del vc71-stlport\libboost_regex-vc71-mt-sp-1_53\*.exp
	del vc71-stlport\libboost_regex-vc71-mt-sp-1_53\*.pch

./vc71-stlport/libboost_regex-vc71-mt-sp-1_53.lib : vc71-stlport/libboost_regex-vc71-mt-sp-1_53/c_regex_traits.obj vc71-stlport/libboost_regex-vc71-mt-sp-1_53/cpp_regex_traits.obj vc71-stlport/libboost_regex-vc71-mt-sp-1_53/cregex.obj vc71-stlport/libboost_regex-vc71-mt-sp-1_53/fileiter.obj vc71-stlport/libboost_regex-vc71-mt-sp-1_53/icu.obj vc71-stlport/libboost_regex-vc71-mt-sp-1_53/instances.obj vc71-stlport/libboost_regex-vc71-mt-sp-1_53/posix_api.obj vc71-stlport/libboost_regex-vc71-mt-sp-1_53/regex.obj vc71-stlport/libboost_regex-vc71-mt-sp-1_53/regex_debug.obj vc71-stlport/libboost_regex-vc71-mt-sp-1_53/regex_raw_buffer.obj vc71-stlport/libboost_regex-vc71-mt-sp-1_53/regex_traits_defaults.obj vc71-stlport/libboost_regex-vc71-mt-sp-1_53/static_mutex.obj vc71-stlport/libboost_regex-vc71-mt-sp-1_53/usinstances.obj vc71-stlport/libboost_regex-vc71-mt-sp-1_53/w32_regex_traits.obj vc71-stlport/libboost_regex-vc71-mt-sp-1_53/wc_regex_traits.obj vc71-stlport/libboost_regex-vc71-mt-sp-1_53/wide_posix_api.obj vc71-stlport/libboost_regex-vc71-mt-sp-1_53/winstances.obj
	link -lib /nologo /out:vc71-stlport/libboost_regex-vc71-mt-sp-1_53.lib $(XSFLAGS)  vc71-stlport/libboost_regex-vc71-mt-sp-1_53/c_regex_traits.obj vc71-stlport/libboost_regex-vc71-mt-sp-1_53/cpp_regex_traits.obj vc71-stlport/libboost_regex-vc71-mt-sp-1_53/cregex.obj vc71-stlport/libboost_regex-vc71-mt-sp-1_53/fileiter.obj vc71-stlport/libboost_regex-vc71-mt-sp-1_53/icu.obj vc71-stlport/libboost_regex-vc71-mt-sp-1_53/instances.obj vc71-stlport/libboost_regex-vc71-mt-sp-1_53/posix_api.obj vc71-stlport/libboost_regex-vc71-mt-sp-1_53/regex.obj vc71-stlport/libboost_regex-vc71-mt-sp-1_53/regex_debug.obj vc71-stlport/libboost_regex-vc71-mt-sp-1_53/regex_raw_buffer.obj vc71-stlport/libboost_regex-vc71-mt-sp-1_53/regex_traits_defaults.obj vc71-stlport/libboost_regex-vc71-mt-sp-1_53/static_mutex.obj vc71-stlport/libboost_regex-vc71-mt-sp-1_53/usinstances.obj vc71-stlport/libboost_regex-vc71-mt-sp-1_53/w32_regex_traits.obj vc71-stlport/libboost_regex-vc71-mt-sp-1_53/wc_regex_traits.obj vc71-stlport/libboost_regex-vc71-mt-sp-1_53/wide_posix_api.obj vc71-stlport/libboost_regex-vc71-mt-sp-1_53/winstances.obj

########################################################
#
# section for boost_regex-vc71-mt-p-1_53.lib
#
########################################################
vc71-stlport/boost_regex-vc71-mt-p-1_53/c_regex_traits.obj: ../src/c_regex_traits.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MD /W3 /GX /O2 /GB /GF /I"$(STLPORT_PATH)\stlport" /Gy /I../../../ /DBOOST_REGEX_DYN_LINK /DNDEBUG /DWIN32 /D_WINDOWS /D_MBCS /D_USRDLL /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/boost_regex-vc71-mt-p-1_53/ -Fdvc71-stlport/boost_regex-vc71-mt-p-1_53.pdb ../src/c_regex_traits.cpp

vc71-stlport/boost_regex-vc71-mt-p-1_53/cpp_regex_traits.obj: ../src/cpp_regex_traits.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MD /W3 /GX /O2 /GB /GF /I"$(STLPORT_PATH)\stlport" /Gy /I../../../ /DBOOST_REGEX_DYN_LINK /DNDEBUG /DWIN32 /D_WINDOWS /D_MBCS /D_USRDLL /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/boost_regex-vc71-mt-p-1_53/ -Fdvc71-stlport/boost_regex-vc71-mt-p-1_53.pdb ../src/cpp_regex_traits.cpp

vc71-stlport/boost_regex-vc71-mt-p-1_53/cregex.obj: ../src/cregex.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MD /W3 /GX /O2 /GB /GF /I"$(STLPORT_PATH)\stlport" /Gy /I../../../ /DBOOST_REGEX_DYN_LINK /DNDEBUG /DWIN32 /D_WINDOWS /D_MBCS /D_USRDLL /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/boost_regex-vc71-mt-p-1_53/ -Fdvc71-stlport/boost_regex-vc71-mt-p-1_53.pdb ../src/cregex.cpp

vc71-stlport/boost_regex-vc71-mt-p-1_53/fileiter.obj: ../src/fileiter.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MD /W3 /GX /O2 /GB /GF /I"$(STLPORT_PATH)\stlport" /Gy /I../../../ /DBOOST_REGEX_DYN_LINK /DNDEBUG /DWIN32 /D_WINDOWS /D_MBCS /D_USRDLL /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/boost_regex-vc71-mt-p-1_53/ -Fdvc71-stlport/boost_regex-vc71-mt-p-1_53.pdb ../src/fileiter.cpp

vc71-stlport/boost_regex-vc71-mt-p-1_53/icu.obj: ../src/icu.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MD /W3 /GX /O2 /GB /GF /I"$(STLPORT_PATH)\stlport" /Gy /I../../../ /DBOOST_REGEX_DYN_LINK /DNDEBUG /DWIN32 /D_WINDOWS /D_MBCS /D_USRDLL /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/boost_regex-vc71-mt-p-1_53/ -Fdvc71-stlport/boost_regex-vc71-mt-p-1_53.pdb ../src/icu.cpp

vc71-stlport/boost_regex-vc71-mt-p-1_53/instances.obj: ../src/instances.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MD /W3 /GX /O2 /GB /GF /I"$(STLPORT_PATH)\stlport" /Gy /I../../../ /DBOOST_REGEX_DYN_LINK /DNDEBUG /DWIN32 /D_WINDOWS /D_MBCS /D_USRDLL /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/boost_regex-vc71-mt-p-1_53/ -Fdvc71-stlport/boost_regex-vc71-mt-p-1_53.pdb ../src/instances.cpp

vc71-stlport/boost_regex-vc71-mt-p-1_53/posix_api.obj: ../src/posix_api.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MD /W3 /GX /O2 /GB /GF /I"$(STLPORT_PATH)\stlport" /Gy /I../../../ /DBOOST_REGEX_DYN_LINK /DNDEBUG /DWIN32 /D_WINDOWS /D_MBCS /D_USRDLL /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/boost_regex-vc71-mt-p-1_53/ -Fdvc71-stlport/boost_regex-vc71-mt-p-1_53.pdb ../src/posix_api.cpp

vc71-stlport/boost_regex-vc71-mt-p-1_53/regex.obj: ../src/regex.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MD /W3 /GX /O2 /GB /GF /I"$(STLPORT_PATH)\stlport" /Gy /I../../../ /DBOOST_REGEX_DYN_LINK /DNDEBUG /DWIN32 /D_WINDOWS /D_MBCS /D_USRDLL /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/boost_regex-vc71-mt-p-1_53/ -Fdvc71-stlport/boost_regex-vc71-mt-p-1_53.pdb ../src/regex.cpp

vc71-stlport/boost_regex-vc71-mt-p-1_53/regex_debug.obj: ../src/regex_debug.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MD /W3 /GX /O2 /GB /GF /I"$(STLPORT_PATH)\stlport" /Gy /I../../../ /DBOOST_REGEX_DYN_LINK /DNDEBUG /DWIN32 /D_WINDOWS /D_MBCS /D_USRDLL /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/boost_regex-vc71-mt-p-1_53/ -Fdvc71-stlport/boost_regex-vc71-mt-p-1_53.pdb ../src/regex_debug.cpp

vc71-stlport/boost_regex-vc71-mt-p-1_53/regex_raw_buffer.obj: ../src/regex_raw_buffer.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MD /W3 /GX /O2 /GB /GF /I"$(STLPORT_PATH)\stlport" /Gy /I../../../ /DBOOST_REGEX_DYN_LINK /DNDEBUG /DWIN32 /D_WINDOWS /D_MBCS /D_USRDLL /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/boost_regex-vc71-mt-p-1_53/ -Fdvc71-stlport/boost_regex-vc71-mt-p-1_53.pdb ../src/regex_raw_buffer.cpp

vc71-stlport/boost_regex-vc71-mt-p-1_53/regex_traits_defaults.obj: ../src/regex_traits_defaults.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MD /W3 /GX /O2 /GB /GF /I"$(STLPORT_PATH)\stlport" /Gy /I../../../ /DBOOST_REGEX_DYN_LINK /DNDEBUG /DWIN32 /D_WINDOWS /D_MBCS /D_USRDLL /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/boost_regex-vc71-mt-p-1_53/ -Fdvc71-stlport/boost_regex-vc71-mt-p-1_53.pdb ../src/regex_traits_defaults.cpp

vc71-stlport/boost_regex-vc71-mt-p-1_53/static_mutex.obj: ../src/static_mutex.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MD /W3 /GX /O2 /GB /GF /I"$(STLPORT_PATH)\stlport" /Gy /I../../../ /DBOOST_REGEX_DYN_LINK /DNDEBUG /DWIN32 /D_WINDOWS /D_MBCS /D_USRDLL /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/boost_regex-vc71-mt-p-1_53/ -Fdvc71-stlport/boost_regex-vc71-mt-p-1_53.pdb ../src/static_mutex.cpp

vc71-stlport/boost_regex-vc71-mt-p-1_53/usinstances.obj: ../src/usinstances.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MD /W3 /GX /O2 /GB /GF /I"$(STLPORT_PATH)\stlport" /Gy /I../../../ /DBOOST_REGEX_DYN_LINK /DNDEBUG /DWIN32 /D_WINDOWS /D_MBCS /D_USRDLL /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/boost_regex-vc71-mt-p-1_53/ -Fdvc71-stlport/boost_regex-vc71-mt-p-1_53.pdb ../src/usinstances.cpp

vc71-stlport/boost_regex-vc71-mt-p-1_53/w32_regex_traits.obj: ../src/w32_regex_traits.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MD /W3 /GX /O2 /GB /GF /I"$(STLPORT_PATH)\stlport" /Gy /I../../../ /DBOOST_REGEX_DYN_LINK /DNDEBUG /DWIN32 /D_WINDOWS /D_MBCS /D_USRDLL /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/boost_regex-vc71-mt-p-1_53/ -Fdvc71-stlport/boost_regex-vc71-mt-p-1_53.pdb ../src/w32_regex_traits.cpp

vc71-stlport/boost_regex-vc71-mt-p-1_53/wc_regex_traits.obj: ../src/wc_regex_traits.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MD /W3 /GX /O2 /GB /GF /I"$(STLPORT_PATH)\stlport" /Gy /I../../../ /DBOOST_REGEX_DYN_LINK /DNDEBUG /DWIN32 /D_WINDOWS /D_MBCS /D_USRDLL /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/boost_regex-vc71-mt-p-1_53/ -Fdvc71-stlport/boost_regex-vc71-mt-p-1_53.pdb ../src/wc_regex_traits.cpp

vc71-stlport/boost_regex-vc71-mt-p-1_53/wide_posix_api.obj: ../src/wide_posix_api.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MD /W3 /GX /O2 /GB /GF /I"$(STLPORT_PATH)\stlport" /Gy /I../../../ /DBOOST_REGEX_DYN_LINK /DNDEBUG /DWIN32 /D_WINDOWS /D_MBCS /D_USRDLL /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/boost_regex-vc71-mt-p-1_53/ -Fdvc71-stlport/boost_regex-vc71-mt-p-1_53.pdb ../src/wide_posix_api.cpp

vc71-stlport/boost_regex-vc71-mt-p-1_53/winstances.obj: ../src/winstances.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MD /W3 /GX /O2 /GB /GF /I"$(STLPORT_PATH)\stlport" /Gy /I../../../ /DBOOST_REGEX_DYN_LINK /DNDEBUG /DWIN32 /D_WINDOWS /D_MBCS /D_USRDLL /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/boost_regex-vc71-mt-p-1_53/ -Fdvc71-stlport/boost_regex-vc71-mt-p-1_53.pdb ../src/winstances.cpp

boost_regex-vc71-mt-p-1_53_dir :
	@if not exist "vc71-stlport\boost_regex-vc71-mt-p-1_53\$(NULL)" mkdir vc71-stlport\boost_regex-vc71-mt-p-1_53

boost_regex-vc71-mt-p-1_53_clean :
	del vc71-stlport\boost_regex-vc71-mt-p-1_53\*.obj
	del vc71-stlport\boost_regex-vc71-mt-p-1_53\*.idb
	del vc71-stlport\boost_regex-vc71-mt-p-1_53\*.exp
	del vc71-stlport\boost_regex-vc71-mt-p-1_53\*.pch

./vc71-stlport/boost_regex-vc71-mt-p-1_53.lib : vc71-stlport/boost_regex-vc71-mt-p-1_53/c_regex_traits.obj vc71-stlport/boost_regex-vc71-mt-p-1_53/cpp_regex_traits.obj vc71-stlport/boost_regex-vc71-mt-p-1_53/cregex.obj vc71-stlport/boost_regex-vc71-mt-p-1_53/fileiter.obj vc71-stlport/boost_regex-vc71-mt-p-1_53/icu.obj vc71-stlport/boost_regex-vc71-mt-p-1_53/instances.obj vc71-stlport/boost_regex-vc71-mt-p-1_53/posix_api.obj vc71-stlport/boost_regex-vc71-mt-p-1_53/regex.obj vc71-stlport/boost_regex-vc71-mt-p-1_53/regex_debug.obj vc71-stlport/boost_regex-vc71-mt-p-1_53/regex_raw_buffer.obj vc71-stlport/boost_regex-vc71-mt-p-1_53/regex_traits_defaults.obj vc71-stlport/boost_regex-vc71-mt-p-1_53/static_mutex.obj vc71-stlport/boost_regex-vc71-mt-p-1_53/usinstances.obj vc71-stlport/boost_regex-vc71-mt-p-1_53/w32_regex_traits.obj vc71-stlport/boost_regex-vc71-mt-p-1_53/wc_regex_traits.obj vc71-stlport/boost_regex-vc71-mt-p-1_53/wide_posix_api.obj vc71-stlport/boost_regex-vc71-mt-p-1_53/winstances.obj
	link kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /dll /incremental:yes /pdb:"vc71-stlport/boost_regex-vc71-mt-p-1_53.pdb" /debug /machine:I386 /out:"vc71-stlport/boost_regex-vc71-mt-p-1_53.dll" /implib:"vc71-stlport/boost_regex-vc71-mt-p-1_53.lib" /LIBPATH:"$(STLPORT_PATH)\lib" $(XLFLAGS) $(ICU_LINK_OPTS)  vc71-stlport/boost_regex-vc71-mt-p-1_53/c_regex_traits.obj vc71-stlport/boost_regex-vc71-mt-p-1_53/cpp_regex_traits.obj vc71-stlport/boost_regex-vc71-mt-p-1_53/cregex.obj vc71-stlport/boost_regex-vc71-mt-p-1_53/fileiter.obj vc71-stlport/boost_regex-vc71-mt-p-1_53/icu.obj vc71-stlport/boost_regex-vc71-mt-p-1_53/instances.obj vc71-stlport/boost_regex-vc71-mt-p-1_53/posix_api.obj vc71-stlport/boost_regex-vc71-mt-p-1_53/regex.obj vc71-stlport/boost_regex-vc71-mt-p-1_53/regex_debug.obj vc71-stlport/boost_regex-vc71-mt-p-1_53/regex_raw_buffer.obj vc71-stlport/boost_regex-vc71-mt-p-1_53/regex_traits_defaults.obj vc71-stlport/boost_regex-vc71-mt-p-1_53/static_mutex.obj vc71-stlport/boost_regex-vc71-mt-p-1_53/usinstances.obj vc71-stlport/boost_regex-vc71-mt-p-1_53/w32_regex_traits.obj vc71-stlport/boost_regex-vc71-mt-p-1_53/wc_regex_traits.obj vc71-stlport/boost_regex-vc71-mt-p-1_53/wide_posix_api.obj vc71-stlport/boost_regex-vc71-mt-p-1_53/winstances.obj

########################################################
#
# section for libboost_regex-vc71-mt-p-1_53.lib
#
########################################################
vc71-stlport/libboost_regex-vc71-mt-p-1_53/c_regex_traits.obj: ../src/c_regex_traits.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MD /W3 /GX /O2 /GB /GF /Gy /I"$(STLPORT_PATH)\stlport" /I../../../ /DBOOST_REGEX_STATIC_LINK /DNDEBUG /DWIN32 /D_WINDOWS /D_MBCS /D_USRDLL /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/libboost_regex-vc71-mt-p-1_53/ -Fdvc71-stlport/libboost_regex-vc71-mt-p-1_53.pdb ../src/c_regex_traits.cpp

vc71-stlport/libboost_regex-vc71-mt-p-1_53/cpp_regex_traits.obj: ../src/cpp_regex_traits.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MD /W3 /GX /O2 /GB /GF /Gy /I"$(STLPORT_PATH)\stlport" /I../../../ /DBOOST_REGEX_STATIC_LINK /DNDEBUG /DWIN32 /D_WINDOWS /D_MBCS /D_USRDLL /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/libboost_regex-vc71-mt-p-1_53/ -Fdvc71-stlport/libboost_regex-vc71-mt-p-1_53.pdb ../src/cpp_regex_traits.cpp

vc71-stlport/libboost_regex-vc71-mt-p-1_53/cregex.obj: ../src/cregex.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MD /W3 /GX /O2 /GB /GF /Gy /I"$(STLPORT_PATH)\stlport" /I../../../ /DBOOST_REGEX_STATIC_LINK /DNDEBUG /DWIN32 /D_WINDOWS /D_MBCS /D_USRDLL /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/libboost_regex-vc71-mt-p-1_53/ -Fdvc71-stlport/libboost_regex-vc71-mt-p-1_53.pdb ../src/cregex.cpp

vc71-stlport/libboost_regex-vc71-mt-p-1_53/fileiter.obj: ../src/fileiter.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MD /W3 /GX /O2 /GB /GF /Gy /I"$(STLPORT_PATH)\stlport" /I../../../ /DBOOST_REGEX_STATIC_LINK /DNDEBUG /DWIN32 /D_WINDOWS /D_MBCS /D_USRDLL /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/libboost_regex-vc71-mt-p-1_53/ -Fdvc71-stlport/libboost_regex-vc71-mt-p-1_53.pdb ../src/fileiter.cpp

vc71-stlport/libboost_regex-vc71-mt-p-1_53/icu.obj: ../src/icu.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MD /W3 /GX /O2 /GB /GF /Gy /I"$(STLPORT_PATH)\stlport" /I../../../ /DBOOST_REGEX_STATIC_LINK /DNDEBUG /DWIN32 /D_WINDOWS /D_MBCS /D_USRDLL /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/libboost_regex-vc71-mt-p-1_53/ -Fdvc71-stlport/libboost_regex-vc71-mt-p-1_53.pdb ../src/icu.cpp

vc71-stlport/libboost_regex-vc71-mt-p-1_53/instances.obj: ../src/instances.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MD /W3 /GX /O2 /GB /GF /Gy /I"$(STLPORT_PATH)\stlport" /I../../../ /DBOOST_REGEX_STATIC_LINK /DNDEBUG /DWIN32 /D_WINDOWS /D_MBCS /D_USRDLL /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/libboost_regex-vc71-mt-p-1_53/ -Fdvc71-stlport/libboost_regex-vc71-mt-p-1_53.pdb ../src/instances.cpp

vc71-stlport/libboost_regex-vc71-mt-p-1_53/posix_api.obj: ../src/posix_api.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MD /W3 /GX /O2 /GB /GF /Gy /I"$(STLPORT_PATH)\stlport" /I../../../ /DBOOST_REGEX_STATIC_LINK /DNDEBUG /DWIN32 /D_WINDOWS /D_MBCS /D_USRDLL /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/libboost_regex-vc71-mt-p-1_53/ -Fdvc71-stlport/libboost_regex-vc71-mt-p-1_53.pdb ../src/posix_api.cpp

vc71-stlport/libboost_regex-vc71-mt-p-1_53/regex.obj: ../src/regex.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MD /W3 /GX /O2 /GB /GF /Gy /I"$(STLPORT_PATH)\stlport" /I../../../ /DBOOST_REGEX_STATIC_LINK /DNDEBUG /DWIN32 /D_WINDOWS /D_MBCS /D_USRDLL /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/libboost_regex-vc71-mt-p-1_53/ -Fdvc71-stlport/libboost_regex-vc71-mt-p-1_53.pdb ../src/regex.cpp

vc71-stlport/libboost_regex-vc71-mt-p-1_53/regex_debug.obj: ../src/regex_debug.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MD /W3 /GX /O2 /GB /GF /Gy /I"$(STLPORT_PATH)\stlport" /I../../../ /DBOOST_REGEX_STATIC_LINK /DNDEBUG /DWIN32 /D_WINDOWS /D_MBCS /D_USRDLL /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/libboost_regex-vc71-mt-p-1_53/ -Fdvc71-stlport/libboost_regex-vc71-mt-p-1_53.pdb ../src/regex_debug.cpp

vc71-stlport/libboost_regex-vc71-mt-p-1_53/regex_raw_buffer.obj: ../src/regex_raw_buffer.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MD /W3 /GX /O2 /GB /GF /Gy /I"$(STLPORT_PATH)\stlport" /I../../../ /DBOOST_REGEX_STATIC_LINK /DNDEBUG /DWIN32 /D_WINDOWS /D_MBCS /D_USRDLL /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/libboost_regex-vc71-mt-p-1_53/ -Fdvc71-stlport/libboost_regex-vc71-mt-p-1_53.pdb ../src/regex_raw_buffer.cpp

vc71-stlport/libboost_regex-vc71-mt-p-1_53/regex_traits_defaults.obj: ../src/regex_traits_defaults.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MD /W3 /GX /O2 /GB /GF /Gy /I"$(STLPORT_PATH)\stlport" /I../../../ /DBOOST_REGEX_STATIC_LINK /DNDEBUG /DWIN32 /D_WINDOWS /D_MBCS /D_USRDLL /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/libboost_regex-vc71-mt-p-1_53/ -Fdvc71-stlport/libboost_regex-vc71-mt-p-1_53.pdb ../src/regex_traits_defaults.cpp

vc71-stlport/libboost_regex-vc71-mt-p-1_53/static_mutex.obj: ../src/static_mutex.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MD /W3 /GX /O2 /GB /GF /Gy /I"$(STLPORT_PATH)\stlport" /I../../../ /DBOOST_REGEX_STATIC_LINK /DNDEBUG /DWIN32 /D_WINDOWS /D_MBCS /D_USRDLL /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/libboost_regex-vc71-mt-p-1_53/ -Fdvc71-stlport/libboost_regex-vc71-mt-p-1_53.pdb ../src/static_mutex.cpp

vc71-stlport/libboost_regex-vc71-mt-p-1_53/usinstances.obj: ../src/usinstances.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MD /W3 /GX /O2 /GB /GF /Gy /I"$(STLPORT_PATH)\stlport" /I../../../ /DBOOST_REGEX_STATIC_LINK /DNDEBUG /DWIN32 /D_WINDOWS /D_MBCS /D_USRDLL /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/libboost_regex-vc71-mt-p-1_53/ -Fdvc71-stlport/libboost_regex-vc71-mt-p-1_53.pdb ../src/usinstances.cpp

vc71-stlport/libboost_regex-vc71-mt-p-1_53/w32_regex_traits.obj: ../src/w32_regex_traits.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MD /W3 /GX /O2 /GB /GF /Gy /I"$(STLPORT_PATH)\stlport" /I../../../ /DBOOST_REGEX_STATIC_LINK /DNDEBUG /DWIN32 /D_WINDOWS /D_MBCS /D_USRDLL /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/libboost_regex-vc71-mt-p-1_53/ -Fdvc71-stlport/libboost_regex-vc71-mt-p-1_53.pdb ../src/w32_regex_traits.cpp

vc71-stlport/libboost_regex-vc71-mt-p-1_53/wc_regex_traits.obj: ../src/wc_regex_traits.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MD /W3 /GX /O2 /GB /GF /Gy /I"$(STLPORT_PATH)\stlport" /I../../../ /DBOOST_REGEX_STATIC_LINK /DNDEBUG /DWIN32 /D_WINDOWS /D_MBCS /D_USRDLL /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/libboost_regex-vc71-mt-p-1_53/ -Fdvc71-stlport/libboost_regex-vc71-mt-p-1_53.pdb ../src/wc_regex_traits.cpp

vc71-stlport/libboost_regex-vc71-mt-p-1_53/wide_posix_api.obj: ../src/wide_posix_api.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MD /W3 /GX /O2 /GB /GF /Gy /I"$(STLPORT_PATH)\stlport" /I../../../ /DBOOST_REGEX_STATIC_LINK /DNDEBUG /DWIN32 /D_WINDOWS /D_MBCS /D_USRDLL /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/libboost_regex-vc71-mt-p-1_53/ -Fdvc71-stlport/libboost_regex-vc71-mt-p-1_53.pdb ../src/wide_posix_api.cpp

vc71-stlport/libboost_regex-vc71-mt-p-1_53/winstances.obj: ../src/winstances.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MD /W3 /GX /O2 /GB /GF /Gy /I"$(STLPORT_PATH)\stlport" /I../../../ /DBOOST_REGEX_STATIC_LINK /DNDEBUG /DWIN32 /D_WINDOWS /D_MBCS /D_USRDLL /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/libboost_regex-vc71-mt-p-1_53/ -Fdvc71-stlport/libboost_regex-vc71-mt-p-1_53.pdb ../src/winstances.cpp

libboost_regex-vc71-mt-p-1_53_dir :
	@if not exist "vc71-stlport\libboost_regex-vc71-mt-p-1_53\$(NULL)" mkdir vc71-stlport\libboost_regex-vc71-mt-p-1_53

libboost_regex-vc71-mt-p-1_53_clean :
	del vc71-stlport\libboost_regex-vc71-mt-p-1_53\*.obj
	del vc71-stlport\libboost_regex-vc71-mt-p-1_53\*.idb
	del vc71-stlport\libboost_regex-vc71-mt-p-1_53\*.exp
	del vc71-stlport\libboost_regex-vc71-mt-p-1_53\*.pch

./vc71-stlport/libboost_regex-vc71-mt-p-1_53.lib : vc71-stlport/libboost_regex-vc71-mt-p-1_53/c_regex_traits.obj vc71-stlport/libboost_regex-vc71-mt-p-1_53/cpp_regex_traits.obj vc71-stlport/libboost_regex-vc71-mt-p-1_53/cregex.obj vc71-stlport/libboost_regex-vc71-mt-p-1_53/fileiter.obj vc71-stlport/libboost_regex-vc71-mt-p-1_53/icu.obj vc71-stlport/libboost_regex-vc71-mt-p-1_53/instances.obj vc71-stlport/libboost_regex-vc71-mt-p-1_53/posix_api.obj vc71-stlport/libboost_regex-vc71-mt-p-1_53/regex.obj vc71-stlport/libboost_regex-vc71-mt-p-1_53/regex_debug.obj vc71-stlport/libboost_regex-vc71-mt-p-1_53/regex_raw_buffer.obj vc71-stlport/libboost_regex-vc71-mt-p-1_53/regex_traits_defaults.obj vc71-stlport/libboost_regex-vc71-mt-p-1_53/static_mutex.obj vc71-stlport/libboost_regex-vc71-mt-p-1_53/usinstances.obj vc71-stlport/libboost_regex-vc71-mt-p-1_53/w32_regex_traits.obj vc71-stlport/libboost_regex-vc71-mt-p-1_53/wc_regex_traits.obj vc71-stlport/libboost_regex-vc71-mt-p-1_53/wide_posix_api.obj vc71-stlport/libboost_regex-vc71-mt-p-1_53/winstances.obj
	link -lib /nologo /out:vc71-stlport/libboost_regex-vc71-mt-p-1_53.lib $(XSFLAGS)  vc71-stlport/libboost_regex-vc71-mt-p-1_53/c_regex_traits.obj vc71-stlport/libboost_regex-vc71-mt-p-1_53/cpp_regex_traits.obj vc71-stlport/libboost_regex-vc71-mt-p-1_53/cregex.obj vc71-stlport/libboost_regex-vc71-mt-p-1_53/fileiter.obj vc71-stlport/libboost_regex-vc71-mt-p-1_53/icu.obj vc71-stlport/libboost_regex-vc71-mt-p-1_53/instances.obj vc71-stlport/libboost_regex-vc71-mt-p-1_53/posix_api.obj vc71-stlport/libboost_regex-vc71-mt-p-1_53/regex.obj vc71-stlport/libboost_regex-vc71-mt-p-1_53/regex_debug.obj vc71-stlport/libboost_regex-vc71-mt-p-1_53/regex_raw_buffer.obj vc71-stlport/libboost_regex-vc71-mt-p-1_53/regex_traits_defaults.obj vc71-stlport/libboost_regex-vc71-mt-p-1_53/static_mutex.obj vc71-stlport/libboost_regex-vc71-mt-p-1_53/usinstances.obj vc71-stlport/libboost_regex-vc71-mt-p-1_53/w32_regex_traits.obj vc71-stlport/libboost_regex-vc71-mt-p-1_53/wc_regex_traits.obj vc71-stlport/libboost_regex-vc71-mt-p-1_53/wide_posix_api.obj vc71-stlport/libboost_regex-vc71-mt-p-1_53/winstances.obj

########################################################
#
# section for boost_regex-vc71-mt-gdp-1_53.lib
#
########################################################
vc71-stlport/boost_regex-vc71-mt-gdp-1_53/c_regex_traits.obj: ../src/c_regex_traits.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MDd /W3 /Gm /GX /Zi /Od /I"$(STLPORT_PATH)\stlport" /I../../../ /DBOOST_REGEX_DYN_LINK /D__STL_DEBUG /D_STLP_DEBUG /D_DEBUG /DWIN32 /D_WINDOWS /D_MBCS /DUSRDLL /GX /RTC1 /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/boost_regex-vc71-mt-gdp-1_53/ -Fdvc71-stlport/boost_regex-vc71-mt-gdp-1_53.pdb ../src/c_regex_traits.cpp

vc71-stlport/boost_regex-vc71-mt-gdp-1_53/cpp_regex_traits.obj: ../src/cpp_regex_traits.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MDd /W3 /Gm /GX /Zi /Od /I"$(STLPORT_PATH)\stlport" /I../../../ /DBOOST_REGEX_DYN_LINK /D__STL_DEBUG /D_STLP_DEBUG /D_DEBUG /DWIN32 /D_WINDOWS /D_MBCS /DUSRDLL /GX /RTC1 /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/boost_regex-vc71-mt-gdp-1_53/ -Fdvc71-stlport/boost_regex-vc71-mt-gdp-1_53.pdb ../src/cpp_regex_traits.cpp

vc71-stlport/boost_regex-vc71-mt-gdp-1_53/cregex.obj: ../src/cregex.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MDd /W3 /Gm /GX /Zi /Od /I"$(STLPORT_PATH)\stlport" /I../../../ /DBOOST_REGEX_DYN_LINK /D__STL_DEBUG /D_STLP_DEBUG /D_DEBUG /DWIN32 /D_WINDOWS /D_MBCS /DUSRDLL /GX /RTC1 /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/boost_regex-vc71-mt-gdp-1_53/ -Fdvc71-stlport/boost_regex-vc71-mt-gdp-1_53.pdb ../src/cregex.cpp

vc71-stlport/boost_regex-vc71-mt-gdp-1_53/fileiter.obj: ../src/fileiter.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MDd /W3 /Gm /GX /Zi /Od /I"$(STLPORT_PATH)\stlport" /I../../../ /DBOOST_REGEX_DYN_LINK /D__STL_DEBUG /D_STLP_DEBUG /D_DEBUG /DWIN32 /D_WINDOWS /D_MBCS /DUSRDLL /GX /RTC1 /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/boost_regex-vc71-mt-gdp-1_53/ -Fdvc71-stlport/boost_regex-vc71-mt-gdp-1_53.pdb ../src/fileiter.cpp

vc71-stlport/boost_regex-vc71-mt-gdp-1_53/icu.obj: ../src/icu.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MDd /W3 /Gm /GX /Zi /Od /I"$(STLPORT_PATH)\stlport" /I../../../ /DBOOST_REGEX_DYN_LINK /D__STL_DEBUG /D_STLP_DEBUG /D_DEBUG /DWIN32 /D_WINDOWS /D_MBCS /DUSRDLL /GX /RTC1 /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/boost_regex-vc71-mt-gdp-1_53/ -Fdvc71-stlport/boost_regex-vc71-mt-gdp-1_53.pdb ../src/icu.cpp

vc71-stlport/boost_regex-vc71-mt-gdp-1_53/instances.obj: ../src/instances.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MDd /W3 /Gm /GX /Zi /Od /I"$(STLPORT_PATH)\stlport" /I../../../ /DBOOST_REGEX_DYN_LINK /D__STL_DEBUG /D_STLP_DEBUG /D_DEBUG /DWIN32 /D_WINDOWS /D_MBCS /DUSRDLL /GX /RTC1 /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/boost_regex-vc71-mt-gdp-1_53/ -Fdvc71-stlport/boost_regex-vc71-mt-gdp-1_53.pdb ../src/instances.cpp

vc71-stlport/boost_regex-vc71-mt-gdp-1_53/posix_api.obj: ../src/posix_api.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MDd /W3 /Gm /GX /Zi /Od /I"$(STLPORT_PATH)\stlport" /I../../../ /DBOOST_REGEX_DYN_LINK /D__STL_DEBUG /D_STLP_DEBUG /D_DEBUG /DWIN32 /D_WINDOWS /D_MBCS /DUSRDLL /GX /RTC1 /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/boost_regex-vc71-mt-gdp-1_53/ -Fdvc71-stlport/boost_regex-vc71-mt-gdp-1_53.pdb ../src/posix_api.cpp

vc71-stlport/boost_regex-vc71-mt-gdp-1_53/regex.obj: ../src/regex.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MDd /W3 /Gm /GX /Zi /Od /I"$(STLPORT_PATH)\stlport" /I../../../ /DBOOST_REGEX_DYN_LINK /D__STL_DEBUG /D_STLP_DEBUG /D_DEBUG /DWIN32 /D_WINDOWS /D_MBCS /DUSRDLL /GX /RTC1 /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/boost_regex-vc71-mt-gdp-1_53/ -Fdvc71-stlport/boost_regex-vc71-mt-gdp-1_53.pdb ../src/regex.cpp

vc71-stlport/boost_regex-vc71-mt-gdp-1_53/regex_debug.obj: ../src/regex_debug.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MDd /W3 /Gm /GX /Zi /Od /I"$(STLPORT_PATH)\stlport" /I../../../ /DBOOST_REGEX_DYN_LINK /D__STL_DEBUG /D_STLP_DEBUG /D_DEBUG /DWIN32 /D_WINDOWS /D_MBCS /DUSRDLL /GX /RTC1 /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/boost_regex-vc71-mt-gdp-1_53/ -Fdvc71-stlport/boost_regex-vc71-mt-gdp-1_53.pdb ../src/regex_debug.cpp

vc71-stlport/boost_regex-vc71-mt-gdp-1_53/regex_raw_buffer.obj: ../src/regex_raw_buffer.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MDd /W3 /Gm /GX /Zi /Od /I"$(STLPORT_PATH)\stlport" /I../../../ /DBOOST_REGEX_DYN_LINK /D__STL_DEBUG /D_STLP_DEBUG /D_DEBUG /DWIN32 /D_WINDOWS /D_MBCS /DUSRDLL /GX /RTC1 /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/boost_regex-vc71-mt-gdp-1_53/ -Fdvc71-stlport/boost_regex-vc71-mt-gdp-1_53.pdb ../src/regex_raw_buffer.cpp

vc71-stlport/boost_regex-vc71-mt-gdp-1_53/regex_traits_defaults.obj: ../src/regex_traits_defaults.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MDd /W3 /Gm /GX /Zi /Od /I"$(STLPORT_PATH)\stlport" /I../../../ /DBOOST_REGEX_DYN_LINK /D__STL_DEBUG /D_STLP_DEBUG /D_DEBUG /DWIN32 /D_WINDOWS /D_MBCS /DUSRDLL /GX /RTC1 /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/boost_regex-vc71-mt-gdp-1_53/ -Fdvc71-stlport/boost_regex-vc71-mt-gdp-1_53.pdb ../src/regex_traits_defaults.cpp

vc71-stlport/boost_regex-vc71-mt-gdp-1_53/static_mutex.obj: ../src/static_mutex.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MDd /W3 /Gm /GX /Zi /Od /I"$(STLPORT_PATH)\stlport" /I../../../ /DBOOST_REGEX_DYN_LINK /D__STL_DEBUG /D_STLP_DEBUG /D_DEBUG /DWIN32 /D_WINDOWS /D_MBCS /DUSRDLL /GX /RTC1 /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/boost_regex-vc71-mt-gdp-1_53/ -Fdvc71-stlport/boost_regex-vc71-mt-gdp-1_53.pdb ../src/static_mutex.cpp

vc71-stlport/boost_regex-vc71-mt-gdp-1_53/usinstances.obj: ../src/usinstances.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MDd /W3 /Gm /GX /Zi /Od /I"$(STLPORT_PATH)\stlport" /I../../../ /DBOOST_REGEX_DYN_LINK /D__STL_DEBUG /D_STLP_DEBUG /D_DEBUG /DWIN32 /D_WINDOWS /D_MBCS /DUSRDLL /GX /RTC1 /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/boost_regex-vc71-mt-gdp-1_53/ -Fdvc71-stlport/boost_regex-vc71-mt-gdp-1_53.pdb ../src/usinstances.cpp

vc71-stlport/boost_regex-vc71-mt-gdp-1_53/w32_regex_traits.obj: ../src/w32_regex_traits.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MDd /W3 /Gm /GX /Zi /Od /I"$(STLPORT_PATH)\stlport" /I../../../ /DBOOST_REGEX_DYN_LINK /D__STL_DEBUG /D_STLP_DEBUG /D_DEBUG /DWIN32 /D_WINDOWS /D_MBCS /DUSRDLL /GX /RTC1 /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/boost_regex-vc71-mt-gdp-1_53/ -Fdvc71-stlport/boost_regex-vc71-mt-gdp-1_53.pdb ../src/w32_regex_traits.cpp

vc71-stlport/boost_regex-vc71-mt-gdp-1_53/wc_regex_traits.obj: ../src/wc_regex_traits.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MDd /W3 /Gm /GX /Zi /Od /I"$(STLPORT_PATH)\stlport" /I../../../ /DBOOST_REGEX_DYN_LINK /D__STL_DEBUG /D_STLP_DEBUG /D_DEBUG /DWIN32 /D_WINDOWS /D_MBCS /DUSRDLL /GX /RTC1 /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/boost_regex-vc71-mt-gdp-1_53/ -Fdvc71-stlport/boost_regex-vc71-mt-gdp-1_53.pdb ../src/wc_regex_traits.cpp

vc71-stlport/boost_regex-vc71-mt-gdp-1_53/wide_posix_api.obj: ../src/wide_posix_api.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MDd /W3 /Gm /GX /Zi /Od /I"$(STLPORT_PATH)\stlport" /I../../../ /DBOOST_REGEX_DYN_LINK /D__STL_DEBUG /D_STLP_DEBUG /D_DEBUG /DWIN32 /D_WINDOWS /D_MBCS /DUSRDLL /GX /RTC1 /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/boost_regex-vc71-mt-gdp-1_53/ -Fdvc71-stlport/boost_regex-vc71-mt-gdp-1_53.pdb ../src/wide_posix_api.cpp

vc71-stlport/boost_regex-vc71-mt-gdp-1_53/winstances.obj: ../src/winstances.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MDd /W3 /Gm /GX /Zi /Od /I"$(STLPORT_PATH)\stlport" /I../../../ /DBOOST_REGEX_DYN_LINK /D__STL_DEBUG /D_STLP_DEBUG /D_DEBUG /DWIN32 /D_WINDOWS /D_MBCS /DUSRDLL /GX /RTC1 /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/boost_regex-vc71-mt-gdp-1_53/ -Fdvc71-stlport/boost_regex-vc71-mt-gdp-1_53.pdb ../src/winstances.cpp

boost_regex-vc71-mt-gdp-1_53_dir :
	@if not exist "vc71-stlport\boost_regex-vc71-mt-gdp-1_53\$(NULL)" mkdir vc71-stlport\boost_regex-vc71-mt-gdp-1_53

boost_regex-vc71-mt-gdp-1_53_clean :
	del vc71-stlport\boost_regex-vc71-mt-gdp-1_53\*.obj
	del vc71-stlport\boost_regex-vc71-mt-gdp-1_53\*.idb
	del vc71-stlport\boost_regex-vc71-mt-gdp-1_53\*.exp
	del vc71-stlport\boost_regex-vc71-mt-gdp-1_53\*.pch

./vc71-stlport/boost_regex-vc71-mt-gdp-1_53.lib : vc71-stlport/boost_regex-vc71-mt-gdp-1_53/c_regex_traits.obj vc71-stlport/boost_regex-vc71-mt-gdp-1_53/cpp_regex_traits.obj vc71-stlport/boost_regex-vc71-mt-gdp-1_53/cregex.obj vc71-stlport/boost_regex-vc71-mt-gdp-1_53/fileiter.obj vc71-stlport/boost_regex-vc71-mt-gdp-1_53/icu.obj vc71-stlport/boost_regex-vc71-mt-gdp-1_53/instances.obj vc71-stlport/boost_regex-vc71-mt-gdp-1_53/posix_api.obj vc71-stlport/boost_regex-vc71-mt-gdp-1_53/regex.obj vc71-stlport/boost_regex-vc71-mt-gdp-1_53/regex_debug.obj vc71-stlport/boost_regex-vc71-mt-gdp-1_53/regex_raw_buffer.obj vc71-stlport/boost_regex-vc71-mt-gdp-1_53/regex_traits_defaults.obj vc71-stlport/boost_regex-vc71-mt-gdp-1_53/static_mutex.obj vc71-stlport/boost_regex-vc71-mt-gdp-1_53/usinstances.obj vc71-stlport/boost_regex-vc71-mt-gdp-1_53/w32_regex_traits.obj vc71-stlport/boost_regex-vc71-mt-gdp-1_53/wc_regex_traits.obj vc71-stlport/boost_regex-vc71-mt-gdp-1_53/wide_posix_api.obj vc71-stlport/boost_regex-vc71-mt-gdp-1_53/winstances.obj
	link kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /dll /incremental:yes /pdb:"vc71-stlport/boost_regex-vc71-mt-gdp-1_53.pdb" /debug /machine:I386 /out:"vc71-stlport/boost_regex-vc71-mt-gdp-1_53.dll" /implib:"vc71-stlport/boost_regex-vc71-mt-gdp-1_53.lib" /LIBPATH:"$(STLPORT_PATH)\lib" $(XLFLAGS) $(ICU_LINK_OPTS)  vc71-stlport/boost_regex-vc71-mt-gdp-1_53/c_regex_traits.obj vc71-stlport/boost_regex-vc71-mt-gdp-1_53/cpp_regex_traits.obj vc71-stlport/boost_regex-vc71-mt-gdp-1_53/cregex.obj vc71-stlport/boost_regex-vc71-mt-gdp-1_53/fileiter.obj vc71-stlport/boost_regex-vc71-mt-gdp-1_53/icu.obj vc71-stlport/boost_regex-vc71-mt-gdp-1_53/instances.obj vc71-stlport/boost_regex-vc71-mt-gdp-1_53/posix_api.obj vc71-stlport/boost_regex-vc71-mt-gdp-1_53/regex.obj vc71-stlport/boost_regex-vc71-mt-gdp-1_53/regex_debug.obj vc71-stlport/boost_regex-vc71-mt-gdp-1_53/regex_raw_buffer.obj vc71-stlport/boost_regex-vc71-mt-gdp-1_53/regex_traits_defaults.obj vc71-stlport/boost_regex-vc71-mt-gdp-1_53/static_mutex.obj vc71-stlport/boost_regex-vc71-mt-gdp-1_53/usinstances.obj vc71-stlport/boost_regex-vc71-mt-gdp-1_53/w32_regex_traits.obj vc71-stlport/boost_regex-vc71-mt-gdp-1_53/wc_regex_traits.obj vc71-stlport/boost_regex-vc71-mt-gdp-1_53/wide_posix_api.obj vc71-stlport/boost_regex-vc71-mt-gdp-1_53/winstances.obj

########################################################
#
# section for libboost_regex-vc71-mt-sgdp-1_53.lib
#
########################################################
vc71-stlport/libboost_regex-vc71-mt-sgdp-1_53/c_regex_traits.obj: ../src/c_regex_traits.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MTd /W3 /Gm /GX /Zi /Od /I"$(STLPORT_PATH)\stlport" /I..\..\..\ /D__STL_DEBUG /D_STLP_DEBUG /DWIN32 /D_MT /D_DEBUG /D_MBCS /D_LIB /GX /RTC1 /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/libboost_regex-vc71-mt-sgdp-1_53/ -Fdvc71-stlport/libboost_regex-vc71-mt-sgdp-1_53.pdb ../src/c_regex_traits.cpp

vc71-stlport/libboost_regex-vc71-mt-sgdp-1_53/cpp_regex_traits.obj: ../src/cpp_regex_traits.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MTd /W3 /Gm /GX /Zi /Od /I"$(STLPORT_PATH)\stlport" /I..\..\..\ /D__STL_DEBUG /D_STLP_DEBUG /DWIN32 /D_MT /D_DEBUG /D_MBCS /D_LIB /GX /RTC1 /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/libboost_regex-vc71-mt-sgdp-1_53/ -Fdvc71-stlport/libboost_regex-vc71-mt-sgdp-1_53.pdb ../src/cpp_regex_traits.cpp

vc71-stlport/libboost_regex-vc71-mt-sgdp-1_53/cregex.obj: ../src/cregex.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MTd /W3 /Gm /GX /Zi /Od /I"$(STLPORT_PATH)\stlport" /I..\..\..\ /D__STL_DEBUG /D_STLP_DEBUG /DWIN32 /D_MT /D_DEBUG /D_MBCS /D_LIB /GX /RTC1 /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/libboost_regex-vc71-mt-sgdp-1_53/ -Fdvc71-stlport/libboost_regex-vc71-mt-sgdp-1_53.pdb ../src/cregex.cpp

vc71-stlport/libboost_regex-vc71-mt-sgdp-1_53/fileiter.obj: ../src/fileiter.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MTd /W3 /Gm /GX /Zi /Od /I"$(STLPORT_PATH)\stlport" /I..\..\..\ /D__STL_DEBUG /D_STLP_DEBUG /DWIN32 /D_MT /D_DEBUG /D_MBCS /D_LIB /GX /RTC1 /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/libboost_regex-vc71-mt-sgdp-1_53/ -Fdvc71-stlport/libboost_regex-vc71-mt-sgdp-1_53.pdb ../src/fileiter.cpp

vc71-stlport/libboost_regex-vc71-mt-sgdp-1_53/icu.obj: ../src/icu.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MTd /W3 /Gm /GX /Zi /Od /I"$(STLPORT_PATH)\stlport" /I..\..\..\ /D__STL_DEBUG /D_STLP_DEBUG /DWIN32 /D_MT /D_DEBUG /D_MBCS /D_LIB /GX /RTC1 /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/libboost_regex-vc71-mt-sgdp-1_53/ -Fdvc71-stlport/libboost_regex-vc71-mt-sgdp-1_53.pdb ../src/icu.cpp

vc71-stlport/libboost_regex-vc71-mt-sgdp-1_53/instances.obj: ../src/instances.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MTd /W3 /Gm /GX /Zi /Od /I"$(STLPORT_PATH)\stlport" /I..\..\..\ /D__STL_DEBUG /D_STLP_DEBUG /DWIN32 /D_MT /D_DEBUG /D_MBCS /D_LIB /GX /RTC1 /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/libboost_regex-vc71-mt-sgdp-1_53/ -Fdvc71-stlport/libboost_regex-vc71-mt-sgdp-1_53.pdb ../src/instances.cpp

vc71-stlport/libboost_regex-vc71-mt-sgdp-1_53/posix_api.obj: ../src/posix_api.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MTd /W3 /Gm /GX /Zi /Od /I"$(STLPORT_PATH)\stlport" /I..\..\..\ /D__STL_DEBUG /D_STLP_DEBUG /DWIN32 /D_MT /D_DEBUG /D_MBCS /D_LIB /GX /RTC1 /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/libboost_regex-vc71-mt-sgdp-1_53/ -Fdvc71-stlport/libboost_regex-vc71-mt-sgdp-1_53.pdb ../src/posix_api.cpp

vc71-stlport/libboost_regex-vc71-mt-sgdp-1_53/regex.obj: ../src/regex.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MTd /W3 /Gm /GX /Zi /Od /I"$(STLPORT_PATH)\stlport" /I..\..\..\ /D__STL_DEBUG /D_STLP_DEBUG /DWIN32 /D_MT /D_DEBUG /D_MBCS /D_LIB /GX /RTC1 /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/libboost_regex-vc71-mt-sgdp-1_53/ -Fdvc71-stlport/libboost_regex-vc71-mt-sgdp-1_53.pdb ../src/regex.cpp

vc71-stlport/libboost_regex-vc71-mt-sgdp-1_53/regex_debug.obj: ../src/regex_debug.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MTd /W3 /Gm /GX /Zi /Od /I"$(STLPORT_PATH)\stlport" /I..\..\..\ /D__STL_DEBUG /D_STLP_DEBUG /DWIN32 /D_MT /D_DEBUG /D_MBCS /D_LIB /GX /RTC1 /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/libboost_regex-vc71-mt-sgdp-1_53/ -Fdvc71-stlport/libboost_regex-vc71-mt-sgdp-1_53.pdb ../src/regex_debug.cpp

vc71-stlport/libboost_regex-vc71-mt-sgdp-1_53/regex_raw_buffer.obj: ../src/regex_raw_buffer.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MTd /W3 /Gm /GX /Zi /Od /I"$(STLPORT_PATH)\stlport" /I..\..\..\ /D__STL_DEBUG /D_STLP_DEBUG /DWIN32 /D_MT /D_DEBUG /D_MBCS /D_LIB /GX /RTC1 /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/libboost_regex-vc71-mt-sgdp-1_53/ -Fdvc71-stlport/libboost_regex-vc71-mt-sgdp-1_53.pdb ../src/regex_raw_buffer.cpp

vc71-stlport/libboost_regex-vc71-mt-sgdp-1_53/regex_traits_defaults.obj: ../src/regex_traits_defaults.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MTd /W3 /Gm /GX /Zi /Od /I"$(STLPORT_PATH)\stlport" /I..\..\..\ /D__STL_DEBUG /D_STLP_DEBUG /DWIN32 /D_MT /D_DEBUG /D_MBCS /D_LIB /GX /RTC1 /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/libboost_regex-vc71-mt-sgdp-1_53/ -Fdvc71-stlport/libboost_regex-vc71-mt-sgdp-1_53.pdb ../src/regex_traits_defaults.cpp

vc71-stlport/libboost_regex-vc71-mt-sgdp-1_53/static_mutex.obj: ../src/static_mutex.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MTd /W3 /Gm /GX /Zi /Od /I"$(STLPORT_PATH)\stlport" /I..\..\..\ /D__STL_DEBUG /D_STLP_DEBUG /DWIN32 /D_MT /D_DEBUG /D_MBCS /D_LIB /GX /RTC1 /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/libboost_regex-vc71-mt-sgdp-1_53/ -Fdvc71-stlport/libboost_regex-vc71-mt-sgdp-1_53.pdb ../src/static_mutex.cpp

vc71-stlport/libboost_regex-vc71-mt-sgdp-1_53/usinstances.obj: ../src/usinstances.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MTd /W3 /Gm /GX /Zi /Od /I"$(STLPORT_PATH)\stlport" /I..\..\..\ /D__STL_DEBUG /D_STLP_DEBUG /DWIN32 /D_MT /D_DEBUG /D_MBCS /D_LIB /GX /RTC1 /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/libboost_regex-vc71-mt-sgdp-1_53/ -Fdvc71-stlport/libboost_regex-vc71-mt-sgdp-1_53.pdb ../src/usinstances.cpp

vc71-stlport/libboost_regex-vc71-mt-sgdp-1_53/w32_regex_traits.obj: ../src/w32_regex_traits.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MTd /W3 /Gm /GX /Zi /Od /I"$(STLPORT_PATH)\stlport" /I..\..\..\ /D__STL_DEBUG /D_STLP_DEBUG /DWIN32 /D_MT /D_DEBUG /D_MBCS /D_LIB /GX /RTC1 /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/libboost_regex-vc71-mt-sgdp-1_53/ -Fdvc71-stlport/libboost_regex-vc71-mt-sgdp-1_53.pdb ../src/w32_regex_traits.cpp

vc71-stlport/libboost_regex-vc71-mt-sgdp-1_53/wc_regex_traits.obj: ../src/wc_regex_traits.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MTd /W3 /Gm /GX /Zi /Od /I"$(STLPORT_PATH)\stlport" /I..\..\..\ /D__STL_DEBUG /D_STLP_DEBUG /DWIN32 /D_MT /D_DEBUG /D_MBCS /D_LIB /GX /RTC1 /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/libboost_regex-vc71-mt-sgdp-1_53/ -Fdvc71-stlport/libboost_regex-vc71-mt-sgdp-1_53.pdb ../src/wc_regex_traits.cpp

vc71-stlport/libboost_regex-vc71-mt-sgdp-1_53/wide_posix_api.obj: ../src/wide_posix_api.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MTd /W3 /Gm /GX /Zi /Od /I"$(STLPORT_PATH)\stlport" /I..\..\..\ /D__STL_DEBUG /D_STLP_DEBUG /DWIN32 /D_MT /D_DEBUG /D_MBCS /D_LIB /GX /RTC1 /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/libboost_regex-vc71-mt-sgdp-1_53/ -Fdvc71-stlport/libboost_regex-vc71-mt-sgdp-1_53.pdb ../src/wide_posix_api.cpp

vc71-stlport/libboost_regex-vc71-mt-sgdp-1_53/winstances.obj: ../src/winstances.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MTd /W3 /Gm /GX /Zi /Od /I"$(STLPORT_PATH)\stlport" /I..\..\..\ /D__STL_DEBUG /D_STLP_DEBUG /DWIN32 /D_MT /D_DEBUG /D_MBCS /D_LIB /GX /RTC1 /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/libboost_regex-vc71-mt-sgdp-1_53/ -Fdvc71-stlport/libboost_regex-vc71-mt-sgdp-1_53.pdb ../src/winstances.cpp

libboost_regex-vc71-mt-sgdp-1_53_dir :
	@if not exist "vc71-stlport\libboost_regex-vc71-mt-sgdp-1_53\$(NULL)" mkdir vc71-stlport\libboost_regex-vc71-mt-sgdp-1_53

libboost_regex-vc71-mt-sgdp-1_53_clean :
	del vc71-stlport\libboost_regex-vc71-mt-sgdp-1_53\*.obj
	del vc71-stlport\libboost_regex-vc71-mt-sgdp-1_53\*.idb
	del vc71-stlport\libboost_regex-vc71-mt-sgdp-1_53\*.exp
	del vc71-stlport\libboost_regex-vc71-mt-sgdp-1_53\*.pch

./vc71-stlport/libboost_regex-vc71-mt-sgdp-1_53.lib : vc71-stlport/libboost_regex-vc71-mt-sgdp-1_53/c_regex_traits.obj vc71-stlport/libboost_regex-vc71-mt-sgdp-1_53/cpp_regex_traits.obj vc71-stlport/libboost_regex-vc71-mt-sgdp-1_53/cregex.obj vc71-stlport/libboost_regex-vc71-mt-sgdp-1_53/fileiter.obj vc71-stlport/libboost_regex-vc71-mt-sgdp-1_53/icu.obj vc71-stlport/libboost_regex-vc71-mt-sgdp-1_53/instances.obj vc71-stlport/libboost_regex-vc71-mt-sgdp-1_53/posix_api.obj vc71-stlport/libboost_regex-vc71-mt-sgdp-1_53/regex.obj vc71-stlport/libboost_regex-vc71-mt-sgdp-1_53/regex_debug.obj vc71-stlport/libboost_regex-vc71-mt-sgdp-1_53/regex_raw_buffer.obj vc71-stlport/libboost_regex-vc71-mt-sgdp-1_53/regex_traits_defaults.obj vc71-stlport/libboost_regex-vc71-mt-sgdp-1_53/static_mutex.obj vc71-stlport/libboost_regex-vc71-mt-sgdp-1_53/usinstances.obj vc71-stlport/libboost_regex-vc71-mt-sgdp-1_53/w32_regex_traits.obj vc71-stlport/libboost_regex-vc71-mt-sgdp-1_53/wc_regex_traits.obj vc71-stlport/libboost_regex-vc71-mt-sgdp-1_53/wide_posix_api.obj vc71-stlport/libboost_regex-vc71-mt-sgdp-1_53/winstances.obj
	link -lib /nologo /out:vc71-stlport/libboost_regex-vc71-mt-sgdp-1_53.lib $(XSFLAGS)  vc71-stlport/libboost_regex-vc71-mt-sgdp-1_53/c_regex_traits.obj vc71-stlport/libboost_regex-vc71-mt-sgdp-1_53/cpp_regex_traits.obj vc71-stlport/libboost_regex-vc71-mt-sgdp-1_53/cregex.obj vc71-stlport/libboost_regex-vc71-mt-sgdp-1_53/fileiter.obj vc71-stlport/libboost_regex-vc71-mt-sgdp-1_53/icu.obj vc71-stlport/libboost_regex-vc71-mt-sgdp-1_53/instances.obj vc71-stlport/libboost_regex-vc71-mt-sgdp-1_53/posix_api.obj vc71-stlport/libboost_regex-vc71-mt-sgdp-1_53/regex.obj vc71-stlport/libboost_regex-vc71-mt-sgdp-1_53/regex_debug.obj vc71-stlport/libboost_regex-vc71-mt-sgdp-1_53/regex_raw_buffer.obj vc71-stlport/libboost_regex-vc71-mt-sgdp-1_53/regex_traits_defaults.obj vc71-stlport/libboost_regex-vc71-mt-sgdp-1_53/static_mutex.obj vc71-stlport/libboost_regex-vc71-mt-sgdp-1_53/usinstances.obj vc71-stlport/libboost_regex-vc71-mt-sgdp-1_53/w32_regex_traits.obj vc71-stlport/libboost_regex-vc71-mt-sgdp-1_53/wc_regex_traits.obj vc71-stlport/libboost_regex-vc71-mt-sgdp-1_53/wide_posix_api.obj vc71-stlport/libboost_regex-vc71-mt-sgdp-1_53/winstances.obj

########################################################
#
# section for libboost_regex-vc71-mt-gdp-1_53.lib
#
########################################################
vc71-stlport/libboost_regex-vc71-mt-gdp-1_53/c_regex_traits.obj: ../src/c_regex_traits.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MDd /W3 /Gm /GX /Zi /Od /I"$(STLPORT_PATH)\stlport" /I../../../ /DBOOST_REGEX_STATIC_LINK /D__STL_DEBUG /D_STLP_DEBUG /D_DEBUG /DWIN32 /D_WINDOWS /D_MBCS /DUSRDLL /GX /RTC1 /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/libboost_regex-vc71-mt-gdp-1_53/ -Fdvc71-stlport/libboost_regex-vc71-mt-gdp-1_53.pdb ../src/c_regex_traits.cpp

vc71-stlport/libboost_regex-vc71-mt-gdp-1_53/cpp_regex_traits.obj: ../src/cpp_regex_traits.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MDd /W3 /Gm /GX /Zi /Od /I"$(STLPORT_PATH)\stlport" /I../../../ /DBOOST_REGEX_STATIC_LINK /D__STL_DEBUG /D_STLP_DEBUG /D_DEBUG /DWIN32 /D_WINDOWS /D_MBCS /DUSRDLL /GX /RTC1 /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/libboost_regex-vc71-mt-gdp-1_53/ -Fdvc71-stlport/libboost_regex-vc71-mt-gdp-1_53.pdb ../src/cpp_regex_traits.cpp

vc71-stlport/libboost_regex-vc71-mt-gdp-1_53/cregex.obj: ../src/cregex.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MDd /W3 /Gm /GX /Zi /Od /I"$(STLPORT_PATH)\stlport" /I../../../ /DBOOST_REGEX_STATIC_LINK /D__STL_DEBUG /D_STLP_DEBUG /D_DEBUG /DWIN32 /D_WINDOWS /D_MBCS /DUSRDLL /GX /RTC1 /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/libboost_regex-vc71-mt-gdp-1_53/ -Fdvc71-stlport/libboost_regex-vc71-mt-gdp-1_53.pdb ../src/cregex.cpp

vc71-stlport/libboost_regex-vc71-mt-gdp-1_53/fileiter.obj: ../src/fileiter.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MDd /W3 /Gm /GX /Zi /Od /I"$(STLPORT_PATH)\stlport" /I../../../ /DBOOST_REGEX_STATIC_LINK /D__STL_DEBUG /D_STLP_DEBUG /D_DEBUG /DWIN32 /D_WINDOWS /D_MBCS /DUSRDLL /GX /RTC1 /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/libboost_regex-vc71-mt-gdp-1_53/ -Fdvc71-stlport/libboost_regex-vc71-mt-gdp-1_53.pdb ../src/fileiter.cpp

vc71-stlport/libboost_regex-vc71-mt-gdp-1_53/icu.obj: ../src/icu.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MDd /W3 /Gm /GX /Zi /Od /I"$(STLPORT_PATH)\stlport" /I../../../ /DBOOST_REGEX_STATIC_LINK /D__STL_DEBUG /D_STLP_DEBUG /D_DEBUG /DWIN32 /D_WINDOWS /D_MBCS /DUSRDLL /GX /RTC1 /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/libboost_regex-vc71-mt-gdp-1_53/ -Fdvc71-stlport/libboost_regex-vc71-mt-gdp-1_53.pdb ../src/icu.cpp

vc71-stlport/libboost_regex-vc71-mt-gdp-1_53/instances.obj: ../src/instances.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MDd /W3 /Gm /GX /Zi /Od /I"$(STLPORT_PATH)\stlport" /I../../../ /DBOOST_REGEX_STATIC_LINK /D__STL_DEBUG /D_STLP_DEBUG /D_DEBUG /DWIN32 /D_WINDOWS /D_MBCS /DUSRDLL /GX /RTC1 /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/libboost_regex-vc71-mt-gdp-1_53/ -Fdvc71-stlport/libboost_regex-vc71-mt-gdp-1_53.pdb ../src/instances.cpp

vc71-stlport/libboost_regex-vc71-mt-gdp-1_53/posix_api.obj: ../src/posix_api.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MDd /W3 /Gm /GX /Zi /Od /I"$(STLPORT_PATH)\stlport" /I../../../ /DBOOST_REGEX_STATIC_LINK /D__STL_DEBUG /D_STLP_DEBUG /D_DEBUG /DWIN32 /D_WINDOWS /D_MBCS /DUSRDLL /GX /RTC1 /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/libboost_regex-vc71-mt-gdp-1_53/ -Fdvc71-stlport/libboost_regex-vc71-mt-gdp-1_53.pdb ../src/posix_api.cpp

vc71-stlport/libboost_regex-vc71-mt-gdp-1_53/regex.obj: ../src/regex.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MDd /W3 /Gm /GX /Zi /Od /I"$(STLPORT_PATH)\stlport" /I../../../ /DBOOST_REGEX_STATIC_LINK /D__STL_DEBUG /D_STLP_DEBUG /D_DEBUG /DWIN32 /D_WINDOWS /D_MBCS /DUSRDLL /GX /RTC1 /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/libboost_regex-vc71-mt-gdp-1_53/ -Fdvc71-stlport/libboost_regex-vc71-mt-gdp-1_53.pdb ../src/regex.cpp

vc71-stlport/libboost_regex-vc71-mt-gdp-1_53/regex_debug.obj: ../src/regex_debug.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MDd /W3 /Gm /GX /Zi /Od /I"$(STLPORT_PATH)\stlport" /I../../../ /DBOOST_REGEX_STATIC_LINK /D__STL_DEBUG /D_STLP_DEBUG /D_DEBUG /DWIN32 /D_WINDOWS /D_MBCS /DUSRDLL /GX /RTC1 /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/libboost_regex-vc71-mt-gdp-1_53/ -Fdvc71-stlport/libboost_regex-vc71-mt-gdp-1_53.pdb ../src/regex_debug.cpp

vc71-stlport/libboost_regex-vc71-mt-gdp-1_53/regex_raw_buffer.obj: ../src/regex_raw_buffer.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MDd /W3 /Gm /GX /Zi /Od /I"$(STLPORT_PATH)\stlport" /I../../../ /DBOOST_REGEX_STATIC_LINK /D__STL_DEBUG /D_STLP_DEBUG /D_DEBUG /DWIN32 /D_WINDOWS /D_MBCS /DUSRDLL /GX /RTC1 /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/libboost_regex-vc71-mt-gdp-1_53/ -Fdvc71-stlport/libboost_regex-vc71-mt-gdp-1_53.pdb ../src/regex_raw_buffer.cpp

vc71-stlport/libboost_regex-vc71-mt-gdp-1_53/regex_traits_defaults.obj: ../src/regex_traits_defaults.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MDd /W3 /Gm /GX /Zi /Od /I"$(STLPORT_PATH)\stlport" /I../../../ /DBOOST_REGEX_STATIC_LINK /D__STL_DEBUG /D_STLP_DEBUG /D_DEBUG /DWIN32 /D_WINDOWS /D_MBCS /DUSRDLL /GX /RTC1 /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/libboost_regex-vc71-mt-gdp-1_53/ -Fdvc71-stlport/libboost_regex-vc71-mt-gdp-1_53.pdb ../src/regex_traits_defaults.cpp

vc71-stlport/libboost_regex-vc71-mt-gdp-1_53/static_mutex.obj: ../src/static_mutex.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MDd /W3 /Gm /GX /Zi /Od /I"$(STLPORT_PATH)\stlport" /I../../../ /DBOOST_REGEX_STATIC_LINK /D__STL_DEBUG /D_STLP_DEBUG /D_DEBUG /DWIN32 /D_WINDOWS /D_MBCS /DUSRDLL /GX /RTC1 /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/libboost_regex-vc71-mt-gdp-1_53/ -Fdvc71-stlport/libboost_regex-vc71-mt-gdp-1_53.pdb ../src/static_mutex.cpp

vc71-stlport/libboost_regex-vc71-mt-gdp-1_53/usinstances.obj: ../src/usinstances.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MDd /W3 /Gm /GX /Zi /Od /I"$(STLPORT_PATH)\stlport" /I../../../ /DBOOST_REGEX_STATIC_LINK /D__STL_DEBUG /D_STLP_DEBUG /D_DEBUG /DWIN32 /D_WINDOWS /D_MBCS /DUSRDLL /GX /RTC1 /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/libboost_regex-vc71-mt-gdp-1_53/ -Fdvc71-stlport/libboost_regex-vc71-mt-gdp-1_53.pdb ../src/usinstances.cpp

vc71-stlport/libboost_regex-vc71-mt-gdp-1_53/w32_regex_traits.obj: ../src/w32_regex_traits.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MDd /W3 /Gm /GX /Zi /Od /I"$(STLPORT_PATH)\stlport" /I../../../ /DBOOST_REGEX_STATIC_LINK /D__STL_DEBUG /D_STLP_DEBUG /D_DEBUG /DWIN32 /D_WINDOWS /D_MBCS /DUSRDLL /GX /RTC1 /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/libboost_regex-vc71-mt-gdp-1_53/ -Fdvc71-stlport/libboost_regex-vc71-mt-gdp-1_53.pdb ../src/w32_regex_traits.cpp

vc71-stlport/libboost_regex-vc71-mt-gdp-1_53/wc_regex_traits.obj: ../src/wc_regex_traits.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MDd /W3 /Gm /GX /Zi /Od /I"$(STLPORT_PATH)\stlport" /I../../../ /DBOOST_REGEX_STATIC_LINK /D__STL_DEBUG /D_STLP_DEBUG /D_DEBUG /DWIN32 /D_WINDOWS /D_MBCS /DUSRDLL /GX /RTC1 /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/libboost_regex-vc71-mt-gdp-1_53/ -Fdvc71-stlport/libboost_regex-vc71-mt-gdp-1_53.pdb ../src/wc_regex_traits.cpp

vc71-stlport/libboost_regex-vc71-mt-gdp-1_53/wide_posix_api.obj: ../src/wide_posix_api.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MDd /W3 /Gm /GX /Zi /Od /I"$(STLPORT_PATH)\stlport" /I../../../ /DBOOST_REGEX_STATIC_LINK /D__STL_DEBUG /D_STLP_DEBUG /D_DEBUG /DWIN32 /D_WINDOWS /D_MBCS /DUSRDLL /GX /RTC1 /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/libboost_regex-vc71-mt-gdp-1_53/ -Fdvc71-stlport/libboost_regex-vc71-mt-gdp-1_53.pdb ../src/wide_posix_api.cpp

vc71-stlport/libboost_regex-vc71-mt-gdp-1_53/winstances.obj: ../src/winstances.cpp $(ALL_HEADER)
	cl $(INCLUDES) /nologo /MDd /W3 /Gm /GX /Zi /Od /I"$(STLPORT_PATH)\stlport" /I../../../ /DBOOST_REGEX_STATIC_LINK /D__STL_DEBUG /D_STLP_DEBUG /D_DEBUG /DWIN32 /D_WINDOWS /D_MBCS /DUSRDLL /GX /RTC1 /Zc:wchar_t /c $(CXXFLAGS) $(ICU_COMPILE_OPTS) -Y- -Fo./vc71-stlport/libboost_regex-vc71-mt-gdp-1_53/ -Fdvc71-stlport/libboost_regex-vc71-mt-gdp-1_53.pdb ../src/winstances.cpp

libboost_regex-vc71-mt-gdp-1_53_dir :
	@if not exist "vc71-stlport\libboost_regex-vc71-mt-gdp-1_53\$(NULL)" mkdir vc71-stlport\libboost_regex-vc71-mt-gdp-1_53

libboost_regex-vc71-mt-gdp-1_53_clean :
	del vc71-stlport\libboost_regex-vc71-mt-gdp-1_53\*.obj
	del vc71-stlport\libboost_regex-vc71-mt-gdp-1_53\*.idb
	del vc71-stlport\libboost_regex-vc71-mt-gdp-1_53\*.exp
	del vc71-stlport\libboost_regex-vc71-mt-gdp-1_53\*.pch

./vc71-stlport/libboost_regex-vc71-mt-gdp-1_53.lib : vc71-stlport/libboost_regex-vc71-mt-gdp-1_53/c_regex_traits.obj vc71-stlport/libboost_regex-vc71-mt-gdp-1_53/cpp_regex_traits.obj vc71-stlport/libboost_regex-vc71-mt-gdp-1_53/cregex.obj vc71-stlport/libboost_regex-vc71-mt-gdp-1_53/fileiter.obj vc71-stlport/libboost_regex-vc71-mt-gdp-1_53/icu.obj vc71-stlport/libboost_regex-vc71-mt-gdp-1_53/instances.obj vc71-stlport/libboost_regex-vc71-mt-gdp-1_53/posix_api.obj vc71-stlport/libboost_regex-vc71-mt-gdp-1_53/regex.obj vc71-stlport/libboost_regex-vc71-mt-gdp-1_53/regex_debug.obj vc71-stlport/libboost_regex-vc71-mt-gdp-1_53/regex_raw_buffer.obj vc71-stlport/libboost_regex-vc71-mt-gdp-1_53/regex_traits_defaults.obj vc71-stlport/libboost_regex-vc71-mt-gdp-1_53/static_mutex.obj vc71-stlport/libboost_regex-vc71-mt-gdp-1_53/usinstances.obj vc71-stlport/libboost_regex-vc71-mt-gdp-1_53/w32_regex_traits.obj vc71-stlport/libboost_regex-vc71-mt-gdp-1_53/wc_regex_traits.obj vc71-stlport/libboost_regex-vc71-mt-gdp-1_53/wide_posix_api.obj vc71-stlport/libboost_regex-vc71-mt-gdp-1_53/winstances.obj
	link -lib /nologo /out:vc71-stlport/libboost_regex-vc71-mt-gdp-1_53.lib $(XSFLAGS)  vc71-stlport/libboost_regex-vc71-mt-gdp-1_53/c_regex_traits.obj vc71-stlport/libboost_regex-vc71-mt-gdp-1_53/cpp_regex_traits.obj vc71-stlport/libboost_regex-vc71-mt-gdp-1_53/cregex.obj vc71-stlport/libboost_regex-vc71-mt-gdp-1_53/fileiter.obj vc71-stlport/libboost_regex-vc71-mt-gdp-1_53/icu.obj vc71-stlport/libboost_regex-vc71-mt-gdp-1_53/instances.obj vc71-stlport/libboost_regex-vc71-mt-gdp-1_53/posix_api.obj vc71-stlport/libboost_regex-vc71-mt-gdp-1_53/regex.obj vc71-stlport/libboost_regex-vc71-mt-gdp-1_53/regex_debug.obj vc71-stlport/libboost_regex-vc71-mt-gdp-1_53/regex_raw_buffer.obj vc71-stlport/libboost_regex-vc71-mt-gdp-1_53/regex_traits_defaults.obj vc71-stlport/libboost_regex-vc71-mt-gdp-1_53/static_mutex.obj vc71-stlport/libboost_regex-vc71-mt-gdp-1_53/usinstances.obj vc71-stlport/libboost_regex-vc71-mt-gdp-1_53/w32_regex_traits.obj vc71-stlport/libboost_regex-vc71-mt-gdp-1_53/wc_regex_traits.obj vc71-stlport/libboost_regex-vc71-mt-gdp-1_53/wide_posix_api.obj vc71-stlport/libboost_regex-vc71-mt-gdp-1_53/winstances.obj

