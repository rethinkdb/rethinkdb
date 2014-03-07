# copyright John Maddock 2006
# Distributed under the Boost Software License, Version 1.0. 
# (See accompanying file LICENSE_1_0.txt or copy at 
# http://www.boost.org/LICENSE_1_0.txt.
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
BCROOT=$(MAKEDIR)\..
!endif


ALL_HEADER= ../../../boost/regex/config.hpp ../../../boost/regex/icu.hpp ../../../boost/regex/pattern_except.hpp ../../../boost/regex/regex_traits.hpp ../../../boost/regex/user.hpp ../../../boost/regex/v4/basic_regex.hpp ../../../boost/regex/v4/basic_regex_creator.hpp ../../../boost/regex/v4/basic_regex_parser.hpp ../../../boost/regex/v4/c_regex_traits.hpp ../../../boost/regex/v4/char_regex_traits.hpp ../../../boost/regex/v4/cpp_regex_traits.hpp ../../../boost/regex/v4/cregex.hpp ../../../boost/regex/v4/error_type.hpp ../../../boost/regex/v4/fileiter.hpp ../../../boost/regex/v4/instances.hpp ../../../boost/regex/v4/iterator_category.hpp ../../../boost/regex/v4/iterator_traits.hpp ../../../boost/regex/v4/match_flags.hpp ../../../boost/regex/v4/match_results.hpp ../../../boost/regex/v4/mem_block_cache.hpp ../../../boost/regex/v4/perl_matcher.hpp ../../../boost/regex/v4/perl_matcher_common.hpp ../../../boost/regex/v4/perl_matcher_non_recursive.hpp ../../../boost/regex/v4/perl_matcher_recursive.hpp ../../../boost/regex/v4/primary_transform.hpp ../../../boost/regex/v4/protected_call.hpp ../../../boost/regex/v4/regbase.hpp ../../../boost/regex/v4/regex.hpp ../../../boost/regex/v4/regex_format.hpp ../../../boost/regex/v4/regex_fwd.hpp ../../../boost/regex/v4/regex_grep.hpp ../../../boost/regex/v4/regex_iterator.hpp ../../../boost/regex/v4/regex_match.hpp ../../../boost/regex/v4/regex_merge.hpp ../../../boost/regex/v4/regex_raw_buffer.hpp ../../../boost/regex/v4/regex_replace.hpp ../../../boost/regex/v4/regex_search.hpp ../../../boost/regex/v4/regex_split.hpp ../../../boost/regex/v4/regex_token_iterator.hpp ../../../boost/regex/v4/regex_traits.hpp ../../../boost/regex/v4/regex_traits_defaults.hpp ../../../boost/regex/v4/regex_workaround.hpp ../../../boost/regex/v4/states.hpp ../../../boost/regex/v4/sub_match.hpp ../../../boost/regex/v4/syntax_type.hpp ../../../boost/regex/v4/u32regex_iterator.hpp ../../../boost/regex/v4/u32regex_token_iterator.hpp ../../../boost/regex/v4/w32_regex_traits.hpp ../../../boost/regex/config/borland.hpp ../../../boost/regex/config/cwchar.hpp

all : bcb bcb\libboost_regex-bcb-s-1_53 bcb\libboost_regex-bcb-s-1_53.lib bcb\libboost_regex-bcb-mt-s-1_53 bcb\libboost_regex-bcb-mt-s-1_53.lib bcb\boost_regex-bcb-mt-1_53 bcb\boost_regex-bcb-mt-1_53.lib bcb\boost_regex-bcb-1_53 bcb\boost_regex-bcb-1_53.lib bcb\libboost_regex-bcb-mt-1_53 bcb\libboost_regex-bcb-mt-1_53.lib bcb\libboost_regex-bcb-1_53 bcb\libboost_regex-bcb-1_53.lib bcb\libboost_regex-bcb-sd-1_53 bcb\libboost_regex-bcb-sd-1_53.lib bcb\libboost_regex-bcb-mt-sd-1_53 bcb\libboost_regex-bcb-mt-sd-1_53.lib bcb\boost_regex-bcb-mt-d-1_53 bcb\boost_regex-bcb-mt-d-1_53.lib bcb\boost_regex-bcb-d-1_53 bcb\boost_regex-bcb-d-1_53.lib bcb\libboost_regex-bcb-mt-d-1_53 bcb\libboost_regex-bcb-mt-d-1_53.lib bcb\libboost_regex-bcb-d-1_53 bcb\libboost_regex-bcb-d-1_53.lib

clean :  libboost_regex-bcb-s-1_53_clean libboost_regex-bcb-mt-s-1_53_clean boost_regex-bcb-mt-1_53_clean boost_regex-bcb-1_53_clean libboost_regex-bcb-mt-1_53_clean libboost_regex-bcb-1_53_clean libboost_regex-bcb-sd-1_53_clean libboost_regex-bcb-mt-sd-1_53_clean boost_regex-bcb-mt-d-1_53_clean boost_regex-bcb-d-1_53_clean libboost_regex-bcb-mt-d-1_53_clean libboost_regex-bcb-d-1_53_clean

install : all
	copy bcb\libboost_regex-bcb-s-1_53.lib $(BCROOT)\lib
	copy bcb\libboost_regex-bcb-mt-s-1_53.lib $(BCROOT)\lib
	copy bcb\boost_regex-bcb-mt-1_53.lib $(BCROOT)\lib
	copy bcb\boost_regex-bcb-mt-1_53.dll $(BCROOT)\bin
	copy bcb\boost_regex-bcb-mt-1_53.tds $(BCROOT)\bin
	copy bcb\boost_regex-bcb-1_53.lib $(BCROOT)\lib
	copy bcb\boost_regex-bcb-1_53.dll $(BCROOT)\bin
	copy bcb\boost_regex-bcb-1_53.tds $(BCROOT)\bin
	copy bcb\libboost_regex-bcb-mt-1_53.lib $(BCROOT)\lib
	copy bcb\libboost_regex-bcb-1_53.lib $(BCROOT)\lib
	copy bcb\libboost_regex-bcb-sd-1_53.lib $(BCROOT)\lib
	copy bcb\libboost_regex-bcb-mt-sd-1_53.lib $(BCROOT)\lib
	copy bcb\boost_regex-bcb-mt-d-1_53.lib $(BCROOT)\lib
	copy bcb\boost_regex-bcb-mt-d-1_53.dll $(BCROOT)\bin
	copy bcb\boost_regex-bcb-mt-d-1_53.tds $(BCROOT)\bin
	copy bcb\boost_regex-bcb-d-1_53.lib $(BCROOT)\lib
	copy bcb\boost_regex-bcb-d-1_53.dll $(BCROOT)\bin
	copy bcb\boost_regex-bcb-d-1_53.tds $(BCROOT)\bin
	copy bcb\libboost_regex-bcb-mt-d-1_53.lib $(BCROOT)\lib
	copy bcb\libboost_regex-bcb-d-1_53.lib $(BCROOT)\lib

bcb :
	-@mkdir bcb


########################################################
#
# section for libboost_regex-bcb-s-1_53.lib
#
########################################################
bcb\libboost_regex-bcb-s-1_53\c_regex_traits.obj: ../src/c_regex_traits.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWM- -D_NO_VCL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8037 -w-8057 -DSTRICT; -I$(BCROOT)\include;../../../ $(CXXFLAGS) -obcb\libboost_regex-bcb-s-1_53\c_regex_traits.obj ../src/c_regex_traits.cpp
|

bcb\libboost_regex-bcb-s-1_53\cpp_regex_traits.obj: ../src/cpp_regex_traits.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWM- -D_NO_VCL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8037 -w-8057 -DSTRICT; -I$(BCROOT)\include;../../../ $(CXXFLAGS) -obcb\libboost_regex-bcb-s-1_53\cpp_regex_traits.obj ../src/cpp_regex_traits.cpp
|

bcb\libboost_regex-bcb-s-1_53\cregex.obj: ../src/cregex.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWM- -D_NO_VCL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8037 -w-8057 -DSTRICT; -I$(BCROOT)\include;../../../ $(CXXFLAGS) -obcb\libboost_regex-bcb-s-1_53\cregex.obj ../src/cregex.cpp
|

bcb\libboost_regex-bcb-s-1_53\fileiter.obj: ../src/fileiter.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWM- -D_NO_VCL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8037 -w-8057 -DSTRICT; -I$(BCROOT)\include;../../../ $(CXXFLAGS) -obcb\libboost_regex-bcb-s-1_53\fileiter.obj ../src/fileiter.cpp
|

bcb\libboost_regex-bcb-s-1_53\icu.obj: ../src/icu.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWM- -D_NO_VCL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8037 -w-8057 -DSTRICT; -I$(BCROOT)\include;../../../ $(CXXFLAGS) -obcb\libboost_regex-bcb-s-1_53\icu.obj ../src/icu.cpp
|

bcb\libboost_regex-bcb-s-1_53\instances.obj: ../src/instances.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWM- -D_NO_VCL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8037 -w-8057 -DSTRICT; -I$(BCROOT)\include;../../../ $(CXXFLAGS) -obcb\libboost_regex-bcb-s-1_53\instances.obj ../src/instances.cpp
|

bcb\libboost_regex-bcb-s-1_53\posix_api.obj: ../src/posix_api.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWM- -D_NO_VCL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8037 -w-8057 -DSTRICT; -I$(BCROOT)\include;../../../ $(CXXFLAGS) -obcb\libboost_regex-bcb-s-1_53\posix_api.obj ../src/posix_api.cpp
|

bcb\libboost_regex-bcb-s-1_53\regex.obj: ../src/regex.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWM- -D_NO_VCL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8037 -w-8057 -DSTRICT; -I$(BCROOT)\include;../../../ $(CXXFLAGS) -obcb\libboost_regex-bcb-s-1_53\regex.obj ../src/regex.cpp
|

bcb\libboost_regex-bcb-s-1_53\regex_debug.obj: ../src/regex_debug.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWM- -D_NO_VCL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8037 -w-8057 -DSTRICT; -I$(BCROOT)\include;../../../ $(CXXFLAGS) -obcb\libboost_regex-bcb-s-1_53\regex_debug.obj ../src/regex_debug.cpp
|

bcb\libboost_regex-bcb-s-1_53\regex_raw_buffer.obj: ../src/regex_raw_buffer.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWM- -D_NO_VCL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8037 -w-8057 -DSTRICT; -I$(BCROOT)\include;../../../ $(CXXFLAGS) -obcb\libboost_regex-bcb-s-1_53\regex_raw_buffer.obj ../src/regex_raw_buffer.cpp
|

bcb\libboost_regex-bcb-s-1_53\regex_traits_defaults.obj: ../src/regex_traits_defaults.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWM- -D_NO_VCL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8037 -w-8057 -DSTRICT; -I$(BCROOT)\include;../../../ $(CXXFLAGS) -obcb\libboost_regex-bcb-s-1_53\regex_traits_defaults.obj ../src/regex_traits_defaults.cpp
|

bcb\libboost_regex-bcb-s-1_53\static_mutex.obj: ../src/static_mutex.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWM- -D_NO_VCL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8037 -w-8057 -DSTRICT; -I$(BCROOT)\include;../../../ $(CXXFLAGS) -obcb\libboost_regex-bcb-s-1_53\static_mutex.obj ../src/static_mutex.cpp
|

bcb\libboost_regex-bcb-s-1_53\usinstances.obj: ../src/usinstances.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWM- -D_NO_VCL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8037 -w-8057 -DSTRICT; -I$(BCROOT)\include;../../../ $(CXXFLAGS) -obcb\libboost_regex-bcb-s-1_53\usinstances.obj ../src/usinstances.cpp
|

bcb\libboost_regex-bcb-s-1_53\w32_regex_traits.obj: ../src/w32_regex_traits.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWM- -D_NO_VCL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8037 -w-8057 -DSTRICT; -I$(BCROOT)\include;../../../ $(CXXFLAGS) -obcb\libboost_regex-bcb-s-1_53\w32_regex_traits.obj ../src/w32_regex_traits.cpp
|

bcb\libboost_regex-bcb-s-1_53\wc_regex_traits.obj: ../src/wc_regex_traits.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWM- -D_NO_VCL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8037 -w-8057 -DSTRICT; -I$(BCROOT)\include;../../../ $(CXXFLAGS) -obcb\libboost_regex-bcb-s-1_53\wc_regex_traits.obj ../src/wc_regex_traits.cpp
|

bcb\libboost_regex-bcb-s-1_53\wide_posix_api.obj: ../src/wide_posix_api.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWM- -D_NO_VCL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8037 -w-8057 -DSTRICT; -I$(BCROOT)\include;../../../ $(CXXFLAGS) -obcb\libboost_regex-bcb-s-1_53\wide_posix_api.obj ../src/wide_posix_api.cpp
|

bcb\libboost_regex-bcb-s-1_53\winstances.obj: ../src/winstances.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWM- -D_NO_VCL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8037 -w-8057 -DSTRICT; -I$(BCROOT)\include;../../../ $(CXXFLAGS) -obcb\libboost_regex-bcb-s-1_53\winstances.obj ../src/winstances.cpp
|

bcb\libboost_regex-bcb-s-1_53 : 
	-@mkdir bcb\libboost_regex-bcb-s-1_53

libboost_regex-bcb-s-1_53_clean :
	del bcb\libboost_regex-bcb-s-1_53\*.obj
	del bcb\libboost_regex-bcb-s-1_53\*.il?
	del bcb\libboost_regex-bcb-s-1_53\*.csm
	del bcb\libboost_regex-bcb-s-1_53\*.tds

bcb\libboost_regex-bcb-s-1_53.lib : bcb\libboost_regex-bcb-s-1_53\c_regex_traits.obj bcb\libboost_regex-bcb-s-1_53\cpp_regex_traits.obj bcb\libboost_regex-bcb-s-1_53\cregex.obj bcb\libboost_regex-bcb-s-1_53\fileiter.obj bcb\libboost_regex-bcb-s-1_53\icu.obj bcb\libboost_regex-bcb-s-1_53\instances.obj bcb\libboost_regex-bcb-s-1_53\posix_api.obj bcb\libboost_regex-bcb-s-1_53\regex.obj bcb\libboost_regex-bcb-s-1_53\regex_debug.obj bcb\libboost_regex-bcb-s-1_53\regex_raw_buffer.obj bcb\libboost_regex-bcb-s-1_53\regex_traits_defaults.obj bcb\libboost_regex-bcb-s-1_53\static_mutex.obj bcb\libboost_regex-bcb-s-1_53\usinstances.obj bcb\libboost_regex-bcb-s-1_53\w32_regex_traits.obj bcb\libboost_regex-bcb-s-1_53\wc_regex_traits.obj bcb\libboost_regex-bcb-s-1_53\wide_posix_api.obj bcb\libboost_regex-bcb-s-1_53\winstances.obj
	if exist bcb\libboost_regex-bcb-s-1_53.lib del bcb\libboost_regex-bcb-s-1_53.lib 
	tlib @&&|
/P128 /C /u /a $(XSFLAGS) "bcb\libboost_regex-bcb-s-1_53.lib"  +"bcb\libboost_regex-bcb-s-1_53\c_regex_traits.obj" +"bcb\libboost_regex-bcb-s-1_53\cpp_regex_traits.obj" +"bcb\libboost_regex-bcb-s-1_53\cregex.obj" +"bcb\libboost_regex-bcb-s-1_53\fileiter.obj" +"bcb\libboost_regex-bcb-s-1_53\icu.obj" +"bcb\libboost_regex-bcb-s-1_53\instances.obj" +"bcb\libboost_regex-bcb-s-1_53\posix_api.obj" +"bcb\libboost_regex-bcb-s-1_53\regex.obj" +"bcb\libboost_regex-bcb-s-1_53\regex_debug.obj" +"bcb\libboost_regex-bcb-s-1_53\regex_raw_buffer.obj" +"bcb\libboost_regex-bcb-s-1_53\regex_traits_defaults.obj" +"bcb\libboost_regex-bcb-s-1_53\static_mutex.obj" +"bcb\libboost_regex-bcb-s-1_53\usinstances.obj" +"bcb\libboost_regex-bcb-s-1_53\w32_regex_traits.obj" +"bcb\libboost_regex-bcb-s-1_53\wc_regex_traits.obj" +"bcb\libboost_regex-bcb-s-1_53\wide_posix_api.obj" +"bcb\libboost_regex-bcb-s-1_53\winstances.obj"
|

########################################################
#
# section for libboost_regex-bcb-mt-s-1_53.lib
#
########################################################
bcb\libboost_regex-bcb-mt-s-1_53\c_regex_traits.obj: ../src/c_regex_traits.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWM -D_NO_VCL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ $(CXXFLAGS) -obcb\libboost_regex-bcb-mt-s-1_53\c_regex_traits.obj ../src/c_regex_traits.cpp
|

bcb\libboost_regex-bcb-mt-s-1_53\cpp_regex_traits.obj: ../src/cpp_regex_traits.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWM -D_NO_VCL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ $(CXXFLAGS) -obcb\libboost_regex-bcb-mt-s-1_53\cpp_regex_traits.obj ../src/cpp_regex_traits.cpp
|

bcb\libboost_regex-bcb-mt-s-1_53\cregex.obj: ../src/cregex.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWM -D_NO_VCL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ $(CXXFLAGS) -obcb\libboost_regex-bcb-mt-s-1_53\cregex.obj ../src/cregex.cpp
|

bcb\libboost_regex-bcb-mt-s-1_53\fileiter.obj: ../src/fileiter.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWM -D_NO_VCL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ $(CXXFLAGS) -obcb\libboost_regex-bcb-mt-s-1_53\fileiter.obj ../src/fileiter.cpp
|

bcb\libboost_regex-bcb-mt-s-1_53\icu.obj: ../src/icu.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWM -D_NO_VCL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ $(CXXFLAGS) -obcb\libboost_regex-bcb-mt-s-1_53\icu.obj ../src/icu.cpp
|

bcb\libboost_regex-bcb-mt-s-1_53\instances.obj: ../src/instances.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWM -D_NO_VCL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ $(CXXFLAGS) -obcb\libboost_regex-bcb-mt-s-1_53\instances.obj ../src/instances.cpp
|

bcb\libboost_regex-bcb-mt-s-1_53\posix_api.obj: ../src/posix_api.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWM -D_NO_VCL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ $(CXXFLAGS) -obcb\libboost_regex-bcb-mt-s-1_53\posix_api.obj ../src/posix_api.cpp
|

bcb\libboost_regex-bcb-mt-s-1_53\regex.obj: ../src/regex.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWM -D_NO_VCL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ $(CXXFLAGS) -obcb\libboost_regex-bcb-mt-s-1_53\regex.obj ../src/regex.cpp
|

bcb\libboost_regex-bcb-mt-s-1_53\regex_debug.obj: ../src/regex_debug.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWM -D_NO_VCL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ $(CXXFLAGS) -obcb\libboost_regex-bcb-mt-s-1_53\regex_debug.obj ../src/regex_debug.cpp
|

bcb\libboost_regex-bcb-mt-s-1_53\regex_raw_buffer.obj: ../src/regex_raw_buffer.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWM -D_NO_VCL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ $(CXXFLAGS) -obcb\libboost_regex-bcb-mt-s-1_53\regex_raw_buffer.obj ../src/regex_raw_buffer.cpp
|

bcb\libboost_regex-bcb-mt-s-1_53\regex_traits_defaults.obj: ../src/regex_traits_defaults.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWM -D_NO_VCL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ $(CXXFLAGS) -obcb\libboost_regex-bcb-mt-s-1_53\regex_traits_defaults.obj ../src/regex_traits_defaults.cpp
|

bcb\libboost_regex-bcb-mt-s-1_53\static_mutex.obj: ../src/static_mutex.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWM -D_NO_VCL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ $(CXXFLAGS) -obcb\libboost_regex-bcb-mt-s-1_53\static_mutex.obj ../src/static_mutex.cpp
|

bcb\libboost_regex-bcb-mt-s-1_53\usinstances.obj: ../src/usinstances.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWM -D_NO_VCL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ $(CXXFLAGS) -obcb\libboost_regex-bcb-mt-s-1_53\usinstances.obj ../src/usinstances.cpp
|

bcb\libboost_regex-bcb-mt-s-1_53\w32_regex_traits.obj: ../src/w32_regex_traits.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWM -D_NO_VCL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ $(CXXFLAGS) -obcb\libboost_regex-bcb-mt-s-1_53\w32_regex_traits.obj ../src/w32_regex_traits.cpp
|

bcb\libboost_regex-bcb-mt-s-1_53\wc_regex_traits.obj: ../src/wc_regex_traits.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWM -D_NO_VCL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ $(CXXFLAGS) -obcb\libboost_regex-bcb-mt-s-1_53\wc_regex_traits.obj ../src/wc_regex_traits.cpp
|

bcb\libboost_regex-bcb-mt-s-1_53\wide_posix_api.obj: ../src/wide_posix_api.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWM -D_NO_VCL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ $(CXXFLAGS) -obcb\libboost_regex-bcb-mt-s-1_53\wide_posix_api.obj ../src/wide_posix_api.cpp
|

bcb\libboost_regex-bcb-mt-s-1_53\winstances.obj: ../src/winstances.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWM -D_NO_VCL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ $(CXXFLAGS) -obcb\libboost_regex-bcb-mt-s-1_53\winstances.obj ../src/winstances.cpp
|

bcb\libboost_regex-bcb-mt-s-1_53 : 
	-@mkdir bcb\libboost_regex-bcb-mt-s-1_53

libboost_regex-bcb-mt-s-1_53_clean :
	del bcb\libboost_regex-bcb-mt-s-1_53\*.obj
	del bcb\libboost_regex-bcb-mt-s-1_53\*.il?
	del bcb\libboost_regex-bcb-mt-s-1_53\*.csm
	del bcb\libboost_regex-bcb-mt-s-1_53\*.tds

bcb\libboost_regex-bcb-mt-s-1_53.lib : bcb\libboost_regex-bcb-mt-s-1_53\c_regex_traits.obj bcb\libboost_regex-bcb-mt-s-1_53\cpp_regex_traits.obj bcb\libboost_regex-bcb-mt-s-1_53\cregex.obj bcb\libboost_regex-bcb-mt-s-1_53\fileiter.obj bcb\libboost_regex-bcb-mt-s-1_53\icu.obj bcb\libboost_regex-bcb-mt-s-1_53\instances.obj bcb\libboost_regex-bcb-mt-s-1_53\posix_api.obj bcb\libboost_regex-bcb-mt-s-1_53\regex.obj bcb\libboost_regex-bcb-mt-s-1_53\regex_debug.obj bcb\libboost_regex-bcb-mt-s-1_53\regex_raw_buffer.obj bcb\libboost_regex-bcb-mt-s-1_53\regex_traits_defaults.obj bcb\libboost_regex-bcb-mt-s-1_53\static_mutex.obj bcb\libboost_regex-bcb-mt-s-1_53\usinstances.obj bcb\libboost_regex-bcb-mt-s-1_53\w32_regex_traits.obj bcb\libboost_regex-bcb-mt-s-1_53\wc_regex_traits.obj bcb\libboost_regex-bcb-mt-s-1_53\wide_posix_api.obj bcb\libboost_regex-bcb-mt-s-1_53\winstances.obj
	if exist bcb\libboost_regex-bcb-mt-s-1_53.lib del bcb\libboost_regex-bcb-mt-s-1_53.lib 
	tlib @&&|
/P128 /C /u /a $(XSFLAGS) "bcb\libboost_regex-bcb-mt-s-1_53.lib"  +"bcb\libboost_regex-bcb-mt-s-1_53\c_regex_traits.obj" +"bcb\libboost_regex-bcb-mt-s-1_53\cpp_regex_traits.obj" +"bcb\libboost_regex-bcb-mt-s-1_53\cregex.obj" +"bcb\libboost_regex-bcb-mt-s-1_53\fileiter.obj" +"bcb\libboost_regex-bcb-mt-s-1_53\icu.obj" +"bcb\libboost_regex-bcb-mt-s-1_53\instances.obj" +"bcb\libboost_regex-bcb-mt-s-1_53\posix_api.obj" +"bcb\libboost_regex-bcb-mt-s-1_53\regex.obj" +"bcb\libboost_regex-bcb-mt-s-1_53\regex_debug.obj" +"bcb\libboost_regex-bcb-mt-s-1_53\regex_raw_buffer.obj" +"bcb\libboost_regex-bcb-mt-s-1_53\regex_traits_defaults.obj" +"bcb\libboost_regex-bcb-mt-s-1_53\static_mutex.obj" +"bcb\libboost_regex-bcb-mt-s-1_53\usinstances.obj" +"bcb\libboost_regex-bcb-mt-s-1_53\w32_regex_traits.obj" +"bcb\libboost_regex-bcb-mt-s-1_53\wc_regex_traits.obj" +"bcb\libboost_regex-bcb-mt-s-1_53\wide_posix_api.obj" +"bcb\libboost_regex-bcb-mt-s-1_53\winstances.obj"
|

########################################################
#
# section for boost_regex-bcb-mt-1_53.lib
#
########################################################
bcb\boost_regex-bcb-mt-1_53\c_regex_traits.obj: ../src/c_regex_traits.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWM -tWR -D_NO_VCL -D_RTLDLL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -DBOOST_REGEX_DYN_LINK -obcb\boost_regex-bcb-mt-1_53\c_regex_traits.obj ../src/c_regex_traits.cpp
|

bcb\boost_regex-bcb-mt-1_53\cpp_regex_traits.obj: ../src/cpp_regex_traits.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWM -tWR -D_NO_VCL -D_RTLDLL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -DBOOST_REGEX_DYN_LINK -obcb\boost_regex-bcb-mt-1_53\cpp_regex_traits.obj ../src/cpp_regex_traits.cpp
|

bcb\boost_regex-bcb-mt-1_53\cregex.obj: ../src/cregex.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWM -tWR -D_NO_VCL -D_RTLDLL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -DBOOST_REGEX_DYN_LINK -obcb\boost_regex-bcb-mt-1_53\cregex.obj ../src/cregex.cpp
|

bcb\boost_regex-bcb-mt-1_53\fileiter.obj: ../src/fileiter.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWM -tWR -D_NO_VCL -D_RTLDLL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -DBOOST_REGEX_DYN_LINK -obcb\boost_regex-bcb-mt-1_53\fileiter.obj ../src/fileiter.cpp
|

bcb\boost_regex-bcb-mt-1_53\icu.obj: ../src/icu.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWM -tWR -D_NO_VCL -D_RTLDLL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -DBOOST_REGEX_DYN_LINK -obcb\boost_regex-bcb-mt-1_53\icu.obj ../src/icu.cpp
|

bcb\boost_regex-bcb-mt-1_53\instances.obj: ../src/instances.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWM -tWR -D_NO_VCL -D_RTLDLL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -DBOOST_REGEX_DYN_LINK -obcb\boost_regex-bcb-mt-1_53\instances.obj ../src/instances.cpp
|

bcb\boost_regex-bcb-mt-1_53\posix_api.obj: ../src/posix_api.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWM -tWR -D_NO_VCL -D_RTLDLL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -DBOOST_REGEX_DYN_LINK -obcb\boost_regex-bcb-mt-1_53\posix_api.obj ../src/posix_api.cpp
|

bcb\boost_regex-bcb-mt-1_53\regex.obj: ../src/regex.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWM -tWR -D_NO_VCL -D_RTLDLL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -DBOOST_REGEX_DYN_LINK -obcb\boost_regex-bcb-mt-1_53\regex.obj ../src/regex.cpp
|

bcb\boost_regex-bcb-mt-1_53\regex_debug.obj: ../src/regex_debug.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWM -tWR -D_NO_VCL -D_RTLDLL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -DBOOST_REGEX_DYN_LINK -obcb\boost_regex-bcb-mt-1_53\regex_debug.obj ../src/regex_debug.cpp
|

bcb\boost_regex-bcb-mt-1_53\regex_raw_buffer.obj: ../src/regex_raw_buffer.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWM -tWR -D_NO_VCL -D_RTLDLL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -DBOOST_REGEX_DYN_LINK -obcb\boost_regex-bcb-mt-1_53\regex_raw_buffer.obj ../src/regex_raw_buffer.cpp
|

bcb\boost_regex-bcb-mt-1_53\regex_traits_defaults.obj: ../src/regex_traits_defaults.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWM -tWR -D_NO_VCL -D_RTLDLL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -DBOOST_REGEX_DYN_LINK -obcb\boost_regex-bcb-mt-1_53\regex_traits_defaults.obj ../src/regex_traits_defaults.cpp
|

bcb\boost_regex-bcb-mt-1_53\static_mutex.obj: ../src/static_mutex.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWM -tWR -D_NO_VCL -D_RTLDLL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -DBOOST_REGEX_DYN_LINK -obcb\boost_regex-bcb-mt-1_53\static_mutex.obj ../src/static_mutex.cpp
|

bcb\boost_regex-bcb-mt-1_53\usinstances.obj: ../src/usinstances.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWM -tWR -D_NO_VCL -D_RTLDLL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -DBOOST_REGEX_DYN_LINK -obcb\boost_regex-bcb-mt-1_53\usinstances.obj ../src/usinstances.cpp
|

bcb\boost_regex-bcb-mt-1_53\w32_regex_traits.obj: ../src/w32_regex_traits.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWM -tWR -D_NO_VCL -D_RTLDLL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -DBOOST_REGEX_DYN_LINK -obcb\boost_regex-bcb-mt-1_53\w32_regex_traits.obj ../src/w32_regex_traits.cpp
|

bcb\boost_regex-bcb-mt-1_53\wc_regex_traits.obj: ../src/wc_regex_traits.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWM -tWR -D_NO_VCL -D_RTLDLL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -DBOOST_REGEX_DYN_LINK -obcb\boost_regex-bcb-mt-1_53\wc_regex_traits.obj ../src/wc_regex_traits.cpp
|

bcb\boost_regex-bcb-mt-1_53\wide_posix_api.obj: ../src/wide_posix_api.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWM -tWR -D_NO_VCL -D_RTLDLL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -DBOOST_REGEX_DYN_LINK -obcb\boost_regex-bcb-mt-1_53\wide_posix_api.obj ../src/wide_posix_api.cpp
|

bcb\boost_regex-bcb-mt-1_53\winstances.obj: ../src/winstances.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWM -tWR -D_NO_VCL -D_RTLDLL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -DBOOST_REGEX_DYN_LINK -obcb\boost_regex-bcb-mt-1_53\winstances.obj ../src/winstances.cpp
|

bcb\boost_regex-bcb-mt-1_53 :
	-@mkdir bcb\boost_regex-bcb-mt-1_53

boost_regex-bcb-mt-1_53_clean :
	del bcb\boost_regex-bcb-mt-1_53\*.obj
	del bcb\boost_regex-bcb-mt-1_53\*.il?
	del bcb\boost_regex-bcb-mt-1_53\*.csm
	del bcb\boost_regex-bcb-mt-1_53\*.tds
	del bcb\*.tds

bcb\boost_regex-bcb-mt-1_53.lib : bcb\boost_regex-bcb-mt-1_53\c_regex_traits.obj bcb\boost_regex-bcb-mt-1_53\cpp_regex_traits.obj bcb\boost_regex-bcb-mt-1_53\cregex.obj bcb\boost_regex-bcb-mt-1_53\fileiter.obj bcb\boost_regex-bcb-mt-1_53\icu.obj bcb\boost_regex-bcb-mt-1_53\instances.obj bcb\boost_regex-bcb-mt-1_53\posix_api.obj bcb\boost_regex-bcb-mt-1_53\regex.obj bcb\boost_regex-bcb-mt-1_53\regex_debug.obj bcb\boost_regex-bcb-mt-1_53\regex_raw_buffer.obj bcb\boost_regex-bcb-mt-1_53\regex_traits_defaults.obj bcb\boost_regex-bcb-mt-1_53\static_mutex.obj bcb\boost_regex-bcb-mt-1_53\usinstances.obj bcb\boost_regex-bcb-mt-1_53\w32_regex_traits.obj bcb\boost_regex-bcb-mt-1_53\wc_regex_traits.obj bcb\boost_regex-bcb-mt-1_53\wide_posix_api.obj bcb\boost_regex-bcb-mt-1_53\winstances.obj
	bcc32 @&&|
-lw-dup -lw-dpl -tWD -tWM -tWR -D_NO_VCL -D_RTLDLL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; -ebcb\boost_regex-bcb-mt-1_53.dll $(XLFLAGS)  bcb\boost_regex-bcb-mt-1_53\c_regex_traits.obj bcb\boost_regex-bcb-mt-1_53\cpp_regex_traits.obj bcb\boost_regex-bcb-mt-1_53\cregex.obj bcb\boost_regex-bcb-mt-1_53\fileiter.obj bcb\boost_regex-bcb-mt-1_53\icu.obj bcb\boost_regex-bcb-mt-1_53\instances.obj bcb\boost_regex-bcb-mt-1_53\posix_api.obj bcb\boost_regex-bcb-mt-1_53\regex.obj bcb\boost_regex-bcb-mt-1_53\regex_debug.obj bcb\boost_regex-bcb-mt-1_53\regex_raw_buffer.obj bcb\boost_regex-bcb-mt-1_53\regex_traits_defaults.obj bcb\boost_regex-bcb-mt-1_53\static_mutex.obj bcb\boost_regex-bcb-mt-1_53\usinstances.obj bcb\boost_regex-bcb-mt-1_53\w32_regex_traits.obj bcb\boost_regex-bcb-mt-1_53\wc_regex_traits.obj bcb\boost_regex-bcb-mt-1_53\wide_posix_api.obj bcb\boost_regex-bcb-mt-1_53\winstances.obj $(LIBS)
|
	implib -w bcb\boost_regex-bcb-mt-1_53.lib bcb\boost_regex-bcb-mt-1_53.dll

########################################################
#
# section for boost_regex-bcb-1_53.lib
#
########################################################
bcb\boost_regex-bcb-1_53\c_regex_traits.obj: ../src/c_regex_traits.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWR -tWM- -D_NO_VCL -D_RTLDLL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -DBOOST_REGEX_DYN_LINK -obcb\boost_regex-bcb-1_53\c_regex_traits.obj ../src/c_regex_traits.cpp
|

bcb\boost_regex-bcb-1_53\cpp_regex_traits.obj: ../src/cpp_regex_traits.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWR -tWM- -D_NO_VCL -D_RTLDLL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -DBOOST_REGEX_DYN_LINK -obcb\boost_regex-bcb-1_53\cpp_regex_traits.obj ../src/cpp_regex_traits.cpp
|

bcb\boost_regex-bcb-1_53\cregex.obj: ../src/cregex.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWR -tWM- -D_NO_VCL -D_RTLDLL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -DBOOST_REGEX_DYN_LINK -obcb\boost_regex-bcb-1_53\cregex.obj ../src/cregex.cpp
|

bcb\boost_regex-bcb-1_53\fileiter.obj: ../src/fileiter.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWR -tWM- -D_NO_VCL -D_RTLDLL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -DBOOST_REGEX_DYN_LINK -obcb\boost_regex-bcb-1_53\fileiter.obj ../src/fileiter.cpp
|

bcb\boost_regex-bcb-1_53\icu.obj: ../src/icu.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWR -tWM- -D_NO_VCL -D_RTLDLL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -DBOOST_REGEX_DYN_LINK -obcb\boost_regex-bcb-1_53\icu.obj ../src/icu.cpp
|

bcb\boost_regex-bcb-1_53\instances.obj: ../src/instances.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWR -tWM- -D_NO_VCL -D_RTLDLL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -DBOOST_REGEX_DYN_LINK -obcb\boost_regex-bcb-1_53\instances.obj ../src/instances.cpp
|

bcb\boost_regex-bcb-1_53\posix_api.obj: ../src/posix_api.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWR -tWM- -D_NO_VCL -D_RTLDLL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -DBOOST_REGEX_DYN_LINK -obcb\boost_regex-bcb-1_53\posix_api.obj ../src/posix_api.cpp
|

bcb\boost_regex-bcb-1_53\regex.obj: ../src/regex.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWR -tWM- -D_NO_VCL -D_RTLDLL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -DBOOST_REGEX_DYN_LINK -obcb\boost_regex-bcb-1_53\regex.obj ../src/regex.cpp
|

bcb\boost_regex-bcb-1_53\regex_debug.obj: ../src/regex_debug.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWR -tWM- -D_NO_VCL -D_RTLDLL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -DBOOST_REGEX_DYN_LINK -obcb\boost_regex-bcb-1_53\regex_debug.obj ../src/regex_debug.cpp
|

bcb\boost_regex-bcb-1_53\regex_raw_buffer.obj: ../src/regex_raw_buffer.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWR -tWM- -D_NO_VCL -D_RTLDLL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -DBOOST_REGEX_DYN_LINK -obcb\boost_regex-bcb-1_53\regex_raw_buffer.obj ../src/regex_raw_buffer.cpp
|

bcb\boost_regex-bcb-1_53\regex_traits_defaults.obj: ../src/regex_traits_defaults.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWR -tWM- -D_NO_VCL -D_RTLDLL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -DBOOST_REGEX_DYN_LINK -obcb\boost_regex-bcb-1_53\regex_traits_defaults.obj ../src/regex_traits_defaults.cpp
|

bcb\boost_regex-bcb-1_53\static_mutex.obj: ../src/static_mutex.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWR -tWM- -D_NO_VCL -D_RTLDLL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -DBOOST_REGEX_DYN_LINK -obcb\boost_regex-bcb-1_53\static_mutex.obj ../src/static_mutex.cpp
|

bcb\boost_regex-bcb-1_53\usinstances.obj: ../src/usinstances.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWR -tWM- -D_NO_VCL -D_RTLDLL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -DBOOST_REGEX_DYN_LINK -obcb\boost_regex-bcb-1_53\usinstances.obj ../src/usinstances.cpp
|

bcb\boost_regex-bcb-1_53\w32_regex_traits.obj: ../src/w32_regex_traits.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWR -tWM- -D_NO_VCL -D_RTLDLL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -DBOOST_REGEX_DYN_LINK -obcb\boost_regex-bcb-1_53\w32_regex_traits.obj ../src/w32_regex_traits.cpp
|

bcb\boost_regex-bcb-1_53\wc_regex_traits.obj: ../src/wc_regex_traits.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWR -tWM- -D_NO_VCL -D_RTLDLL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -DBOOST_REGEX_DYN_LINK -obcb\boost_regex-bcb-1_53\wc_regex_traits.obj ../src/wc_regex_traits.cpp
|

bcb\boost_regex-bcb-1_53\wide_posix_api.obj: ../src/wide_posix_api.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWR -tWM- -D_NO_VCL -D_RTLDLL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -DBOOST_REGEX_DYN_LINK -obcb\boost_regex-bcb-1_53\wide_posix_api.obj ../src/wide_posix_api.cpp
|

bcb\boost_regex-bcb-1_53\winstances.obj: ../src/winstances.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWR -tWM- -D_NO_VCL -D_RTLDLL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -DBOOST_REGEX_DYN_LINK -obcb\boost_regex-bcb-1_53\winstances.obj ../src/winstances.cpp
|

bcb\boost_regex-bcb-1_53 :
	-@mkdir bcb\boost_regex-bcb-1_53

boost_regex-bcb-1_53_clean :
	del bcb\boost_regex-bcb-1_53\*.obj
	del bcb\boost_regex-bcb-1_53\*.il?
	del bcb\boost_regex-bcb-1_53\*.csm
	del bcb\boost_regex-bcb-1_53\*.tds
	del bcb\*.tds

bcb\boost_regex-bcb-1_53.lib : bcb\boost_regex-bcb-1_53\c_regex_traits.obj bcb\boost_regex-bcb-1_53\cpp_regex_traits.obj bcb\boost_regex-bcb-1_53\cregex.obj bcb\boost_regex-bcb-1_53\fileiter.obj bcb\boost_regex-bcb-1_53\icu.obj bcb\boost_regex-bcb-1_53\instances.obj bcb\boost_regex-bcb-1_53\posix_api.obj bcb\boost_regex-bcb-1_53\regex.obj bcb\boost_regex-bcb-1_53\regex_debug.obj bcb\boost_regex-bcb-1_53\regex_raw_buffer.obj bcb\boost_regex-bcb-1_53\regex_traits_defaults.obj bcb\boost_regex-bcb-1_53\static_mutex.obj bcb\boost_regex-bcb-1_53\usinstances.obj bcb\boost_regex-bcb-1_53\w32_regex_traits.obj bcb\boost_regex-bcb-1_53\wc_regex_traits.obj bcb\boost_regex-bcb-1_53\wide_posix_api.obj bcb\boost_regex-bcb-1_53\winstances.obj
	bcc32 @&&|
-lw-dup -lw-dpl -tWD -tWR -tWM- -D_NO_VCL -D_RTLDLL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; -ebcb\boost_regex-bcb-1_53.dll $(XLFLAGS)  bcb\boost_regex-bcb-1_53\c_regex_traits.obj bcb\boost_regex-bcb-1_53\cpp_regex_traits.obj bcb\boost_regex-bcb-1_53\cregex.obj bcb\boost_regex-bcb-1_53\fileiter.obj bcb\boost_regex-bcb-1_53\icu.obj bcb\boost_regex-bcb-1_53\instances.obj bcb\boost_regex-bcb-1_53\posix_api.obj bcb\boost_regex-bcb-1_53\regex.obj bcb\boost_regex-bcb-1_53\regex_debug.obj bcb\boost_regex-bcb-1_53\regex_raw_buffer.obj bcb\boost_regex-bcb-1_53\regex_traits_defaults.obj bcb\boost_regex-bcb-1_53\static_mutex.obj bcb\boost_regex-bcb-1_53\usinstances.obj bcb\boost_regex-bcb-1_53\w32_regex_traits.obj bcb\boost_regex-bcb-1_53\wc_regex_traits.obj bcb\boost_regex-bcb-1_53\wide_posix_api.obj bcb\boost_regex-bcb-1_53\winstances.obj $(LIBS)
|
	implib -w bcb\boost_regex-bcb-1_53.lib bcb\boost_regex-bcb-1_53.dll

########################################################
#
# section for libboost_regex-bcb-mt-1_53.lib
#
########################################################
bcb\libboost_regex-bcb-mt-1_53\c_regex_traits.obj: ../src/c_regex_traits.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWM -tWR -DBOOST_REGEX_STATIC_LINK -D_NO_VCL -D_RTLDLL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -obcb\libboost_regex-bcb-mt-1_53\c_regex_traits.obj ../src/c_regex_traits.cpp
|

bcb\libboost_regex-bcb-mt-1_53\cpp_regex_traits.obj: ../src/cpp_regex_traits.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWM -tWR -DBOOST_REGEX_STATIC_LINK -D_NO_VCL -D_RTLDLL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -obcb\libboost_regex-bcb-mt-1_53\cpp_regex_traits.obj ../src/cpp_regex_traits.cpp
|

bcb\libboost_regex-bcb-mt-1_53\cregex.obj: ../src/cregex.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWM -tWR -DBOOST_REGEX_STATIC_LINK -D_NO_VCL -D_RTLDLL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -obcb\libboost_regex-bcb-mt-1_53\cregex.obj ../src/cregex.cpp
|

bcb\libboost_regex-bcb-mt-1_53\fileiter.obj: ../src/fileiter.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWM -tWR -DBOOST_REGEX_STATIC_LINK -D_NO_VCL -D_RTLDLL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -obcb\libboost_regex-bcb-mt-1_53\fileiter.obj ../src/fileiter.cpp
|

bcb\libboost_regex-bcb-mt-1_53\icu.obj: ../src/icu.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWM -tWR -DBOOST_REGEX_STATIC_LINK -D_NO_VCL -D_RTLDLL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -obcb\libboost_regex-bcb-mt-1_53\icu.obj ../src/icu.cpp
|

bcb\libboost_regex-bcb-mt-1_53\instances.obj: ../src/instances.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWM -tWR -DBOOST_REGEX_STATIC_LINK -D_NO_VCL -D_RTLDLL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -obcb\libboost_regex-bcb-mt-1_53\instances.obj ../src/instances.cpp
|

bcb\libboost_regex-bcb-mt-1_53\posix_api.obj: ../src/posix_api.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWM -tWR -DBOOST_REGEX_STATIC_LINK -D_NO_VCL -D_RTLDLL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -obcb\libboost_regex-bcb-mt-1_53\posix_api.obj ../src/posix_api.cpp
|

bcb\libboost_regex-bcb-mt-1_53\regex.obj: ../src/regex.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWM -tWR -DBOOST_REGEX_STATIC_LINK -D_NO_VCL -D_RTLDLL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -obcb\libboost_regex-bcb-mt-1_53\regex.obj ../src/regex.cpp
|

bcb\libboost_regex-bcb-mt-1_53\regex_debug.obj: ../src/regex_debug.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWM -tWR -DBOOST_REGEX_STATIC_LINK -D_NO_VCL -D_RTLDLL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -obcb\libboost_regex-bcb-mt-1_53\regex_debug.obj ../src/regex_debug.cpp
|

bcb\libboost_regex-bcb-mt-1_53\regex_raw_buffer.obj: ../src/regex_raw_buffer.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWM -tWR -DBOOST_REGEX_STATIC_LINK -D_NO_VCL -D_RTLDLL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -obcb\libboost_regex-bcb-mt-1_53\regex_raw_buffer.obj ../src/regex_raw_buffer.cpp
|

bcb\libboost_regex-bcb-mt-1_53\regex_traits_defaults.obj: ../src/regex_traits_defaults.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWM -tWR -DBOOST_REGEX_STATIC_LINK -D_NO_VCL -D_RTLDLL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -obcb\libboost_regex-bcb-mt-1_53\regex_traits_defaults.obj ../src/regex_traits_defaults.cpp
|

bcb\libboost_regex-bcb-mt-1_53\static_mutex.obj: ../src/static_mutex.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWM -tWR -DBOOST_REGEX_STATIC_LINK -D_NO_VCL -D_RTLDLL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -obcb\libboost_regex-bcb-mt-1_53\static_mutex.obj ../src/static_mutex.cpp
|

bcb\libboost_regex-bcb-mt-1_53\usinstances.obj: ../src/usinstances.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWM -tWR -DBOOST_REGEX_STATIC_LINK -D_NO_VCL -D_RTLDLL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -obcb\libboost_regex-bcb-mt-1_53\usinstances.obj ../src/usinstances.cpp
|

bcb\libboost_regex-bcb-mt-1_53\w32_regex_traits.obj: ../src/w32_regex_traits.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWM -tWR -DBOOST_REGEX_STATIC_LINK -D_NO_VCL -D_RTLDLL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -obcb\libboost_regex-bcb-mt-1_53\w32_regex_traits.obj ../src/w32_regex_traits.cpp
|

bcb\libboost_regex-bcb-mt-1_53\wc_regex_traits.obj: ../src/wc_regex_traits.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWM -tWR -DBOOST_REGEX_STATIC_LINK -D_NO_VCL -D_RTLDLL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -obcb\libboost_regex-bcb-mt-1_53\wc_regex_traits.obj ../src/wc_regex_traits.cpp
|

bcb\libboost_regex-bcb-mt-1_53\wide_posix_api.obj: ../src/wide_posix_api.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWM -tWR -DBOOST_REGEX_STATIC_LINK -D_NO_VCL -D_RTLDLL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -obcb\libboost_regex-bcb-mt-1_53\wide_posix_api.obj ../src/wide_posix_api.cpp
|

bcb\libboost_regex-bcb-mt-1_53\winstances.obj: ../src/winstances.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWM -tWR -DBOOST_REGEX_STATIC_LINK -D_NO_VCL -D_RTLDLL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -obcb\libboost_regex-bcb-mt-1_53\winstances.obj ../src/winstances.cpp
|

bcb\libboost_regex-bcb-mt-1_53 : 
	-@mkdir bcb\libboost_regex-bcb-mt-1_53

libboost_regex-bcb-mt-1_53_clean :
	del bcb\libboost_regex-bcb-mt-1_53\*.obj
	del bcb\libboost_regex-bcb-mt-1_53\*.il?
	del bcb\libboost_regex-bcb-mt-1_53\*.csm
	del bcb\libboost_regex-bcb-mt-1_53\*.tds

bcb\libboost_regex-bcb-mt-1_53.lib : bcb\libboost_regex-bcb-mt-1_53\c_regex_traits.obj bcb\libboost_regex-bcb-mt-1_53\cpp_regex_traits.obj bcb\libboost_regex-bcb-mt-1_53\cregex.obj bcb\libboost_regex-bcb-mt-1_53\fileiter.obj bcb\libboost_regex-bcb-mt-1_53\icu.obj bcb\libboost_regex-bcb-mt-1_53\instances.obj bcb\libboost_regex-bcb-mt-1_53\posix_api.obj bcb\libboost_regex-bcb-mt-1_53\regex.obj bcb\libboost_regex-bcb-mt-1_53\regex_debug.obj bcb\libboost_regex-bcb-mt-1_53\regex_raw_buffer.obj bcb\libboost_regex-bcb-mt-1_53\regex_traits_defaults.obj bcb\libboost_regex-bcb-mt-1_53\static_mutex.obj bcb\libboost_regex-bcb-mt-1_53\usinstances.obj bcb\libboost_regex-bcb-mt-1_53\w32_regex_traits.obj bcb\libboost_regex-bcb-mt-1_53\wc_regex_traits.obj bcb\libboost_regex-bcb-mt-1_53\wide_posix_api.obj bcb\libboost_regex-bcb-mt-1_53\winstances.obj
	if exist bcb\libboost_regex-bcb-mt-1_53.lib del bcb\libboost_regex-bcb-mt-1_53.lib 
	tlib @&&|
/P128 /C /u /a $(XSFLAGS) "bcb\libboost_regex-bcb-mt-1_53.lib"  +"bcb\libboost_regex-bcb-mt-1_53\c_regex_traits.obj" +"bcb\libboost_regex-bcb-mt-1_53\cpp_regex_traits.obj" +"bcb\libboost_regex-bcb-mt-1_53\cregex.obj" +"bcb\libboost_regex-bcb-mt-1_53\fileiter.obj" +"bcb\libboost_regex-bcb-mt-1_53\icu.obj" +"bcb\libboost_regex-bcb-mt-1_53\instances.obj" +"bcb\libboost_regex-bcb-mt-1_53\posix_api.obj" +"bcb\libboost_regex-bcb-mt-1_53\regex.obj" +"bcb\libboost_regex-bcb-mt-1_53\regex_debug.obj" +"bcb\libboost_regex-bcb-mt-1_53\regex_raw_buffer.obj" +"bcb\libboost_regex-bcb-mt-1_53\regex_traits_defaults.obj" +"bcb\libboost_regex-bcb-mt-1_53\static_mutex.obj" +"bcb\libboost_regex-bcb-mt-1_53\usinstances.obj" +"bcb\libboost_regex-bcb-mt-1_53\w32_regex_traits.obj" +"bcb\libboost_regex-bcb-mt-1_53\wc_regex_traits.obj" +"bcb\libboost_regex-bcb-mt-1_53\wide_posix_api.obj" +"bcb\libboost_regex-bcb-mt-1_53\winstances.obj"
|

########################################################
#
# section for libboost_regex-bcb-1_53.lib
#
########################################################
bcb\libboost_regex-bcb-1_53\c_regex_traits.obj: ../src/c_regex_traits.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWR -tWM- -DBOOST_REGEX_STATIC_LINK -D_NO_VCL -D_RTLDLL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -obcb\libboost_regex-bcb-1_53\c_regex_traits.obj ../src/c_regex_traits.cpp
|

bcb\libboost_regex-bcb-1_53\cpp_regex_traits.obj: ../src/cpp_regex_traits.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWR -tWM- -DBOOST_REGEX_STATIC_LINK -D_NO_VCL -D_RTLDLL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -obcb\libboost_regex-bcb-1_53\cpp_regex_traits.obj ../src/cpp_regex_traits.cpp
|

bcb\libboost_regex-bcb-1_53\cregex.obj: ../src/cregex.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWR -tWM- -DBOOST_REGEX_STATIC_LINK -D_NO_VCL -D_RTLDLL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -obcb\libboost_regex-bcb-1_53\cregex.obj ../src/cregex.cpp
|

bcb\libboost_regex-bcb-1_53\fileiter.obj: ../src/fileiter.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWR -tWM- -DBOOST_REGEX_STATIC_LINK -D_NO_VCL -D_RTLDLL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -obcb\libboost_regex-bcb-1_53\fileiter.obj ../src/fileiter.cpp
|

bcb\libboost_regex-bcb-1_53\icu.obj: ../src/icu.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWR -tWM- -DBOOST_REGEX_STATIC_LINK -D_NO_VCL -D_RTLDLL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -obcb\libboost_regex-bcb-1_53\icu.obj ../src/icu.cpp
|

bcb\libboost_regex-bcb-1_53\instances.obj: ../src/instances.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWR -tWM- -DBOOST_REGEX_STATIC_LINK -D_NO_VCL -D_RTLDLL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -obcb\libboost_regex-bcb-1_53\instances.obj ../src/instances.cpp
|

bcb\libboost_regex-bcb-1_53\posix_api.obj: ../src/posix_api.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWR -tWM- -DBOOST_REGEX_STATIC_LINK -D_NO_VCL -D_RTLDLL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -obcb\libboost_regex-bcb-1_53\posix_api.obj ../src/posix_api.cpp
|

bcb\libboost_regex-bcb-1_53\regex.obj: ../src/regex.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWR -tWM- -DBOOST_REGEX_STATIC_LINK -D_NO_VCL -D_RTLDLL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -obcb\libboost_regex-bcb-1_53\regex.obj ../src/regex.cpp
|

bcb\libboost_regex-bcb-1_53\regex_debug.obj: ../src/regex_debug.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWR -tWM- -DBOOST_REGEX_STATIC_LINK -D_NO_VCL -D_RTLDLL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -obcb\libboost_regex-bcb-1_53\regex_debug.obj ../src/regex_debug.cpp
|

bcb\libboost_regex-bcb-1_53\regex_raw_buffer.obj: ../src/regex_raw_buffer.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWR -tWM- -DBOOST_REGEX_STATIC_LINK -D_NO_VCL -D_RTLDLL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -obcb\libboost_regex-bcb-1_53\regex_raw_buffer.obj ../src/regex_raw_buffer.cpp
|

bcb\libboost_regex-bcb-1_53\regex_traits_defaults.obj: ../src/regex_traits_defaults.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWR -tWM- -DBOOST_REGEX_STATIC_LINK -D_NO_VCL -D_RTLDLL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -obcb\libboost_regex-bcb-1_53\regex_traits_defaults.obj ../src/regex_traits_defaults.cpp
|

bcb\libboost_regex-bcb-1_53\static_mutex.obj: ../src/static_mutex.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWR -tWM- -DBOOST_REGEX_STATIC_LINK -D_NO_VCL -D_RTLDLL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -obcb\libboost_regex-bcb-1_53\static_mutex.obj ../src/static_mutex.cpp
|

bcb\libboost_regex-bcb-1_53\usinstances.obj: ../src/usinstances.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWR -tWM- -DBOOST_REGEX_STATIC_LINK -D_NO_VCL -D_RTLDLL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -obcb\libboost_regex-bcb-1_53\usinstances.obj ../src/usinstances.cpp
|

bcb\libboost_regex-bcb-1_53\w32_regex_traits.obj: ../src/w32_regex_traits.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWR -tWM- -DBOOST_REGEX_STATIC_LINK -D_NO_VCL -D_RTLDLL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -obcb\libboost_regex-bcb-1_53\w32_regex_traits.obj ../src/w32_regex_traits.cpp
|

bcb\libboost_regex-bcb-1_53\wc_regex_traits.obj: ../src/wc_regex_traits.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWR -tWM- -DBOOST_REGEX_STATIC_LINK -D_NO_VCL -D_RTLDLL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -obcb\libboost_regex-bcb-1_53\wc_regex_traits.obj ../src/wc_regex_traits.cpp
|

bcb\libboost_regex-bcb-1_53\wide_posix_api.obj: ../src/wide_posix_api.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWR -tWM- -DBOOST_REGEX_STATIC_LINK -D_NO_VCL -D_RTLDLL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -obcb\libboost_regex-bcb-1_53\wide_posix_api.obj ../src/wide_posix_api.cpp
|

bcb\libboost_regex-bcb-1_53\winstances.obj: ../src/winstances.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWR -tWM- -DBOOST_REGEX_STATIC_LINK -D_NO_VCL -D_RTLDLL -O2 -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -obcb\libboost_regex-bcb-1_53\winstances.obj ../src/winstances.cpp
|

bcb\libboost_regex-bcb-1_53 : 
	-@mkdir bcb\libboost_regex-bcb-1_53

libboost_regex-bcb-1_53_clean :
	del bcb\libboost_regex-bcb-1_53\*.obj
	del bcb\libboost_regex-bcb-1_53\*.il?
	del bcb\libboost_regex-bcb-1_53\*.csm
	del bcb\libboost_regex-bcb-1_53\*.tds

bcb\libboost_regex-bcb-1_53.lib : bcb\libboost_regex-bcb-1_53\c_regex_traits.obj bcb\libboost_regex-bcb-1_53\cpp_regex_traits.obj bcb\libboost_regex-bcb-1_53\cregex.obj bcb\libboost_regex-bcb-1_53\fileiter.obj bcb\libboost_regex-bcb-1_53\icu.obj bcb\libboost_regex-bcb-1_53\instances.obj bcb\libboost_regex-bcb-1_53\posix_api.obj bcb\libboost_regex-bcb-1_53\regex.obj bcb\libboost_regex-bcb-1_53\regex_debug.obj bcb\libboost_regex-bcb-1_53\regex_raw_buffer.obj bcb\libboost_regex-bcb-1_53\regex_traits_defaults.obj bcb\libboost_regex-bcb-1_53\static_mutex.obj bcb\libboost_regex-bcb-1_53\usinstances.obj bcb\libboost_regex-bcb-1_53\w32_regex_traits.obj bcb\libboost_regex-bcb-1_53\wc_regex_traits.obj bcb\libboost_regex-bcb-1_53\wide_posix_api.obj bcb\libboost_regex-bcb-1_53\winstances.obj
	if exist bcb\libboost_regex-bcb-1_53.lib del bcb\libboost_regex-bcb-1_53.lib 
	tlib @&&|
/P128 /C /u /a $(XSFLAGS) "bcb\libboost_regex-bcb-1_53.lib"  +"bcb\libboost_regex-bcb-1_53\c_regex_traits.obj" +"bcb\libboost_regex-bcb-1_53\cpp_regex_traits.obj" +"bcb\libboost_regex-bcb-1_53\cregex.obj" +"bcb\libboost_regex-bcb-1_53\fileiter.obj" +"bcb\libboost_regex-bcb-1_53\icu.obj" +"bcb\libboost_regex-bcb-1_53\instances.obj" +"bcb\libboost_regex-bcb-1_53\posix_api.obj" +"bcb\libboost_regex-bcb-1_53\regex.obj" +"bcb\libboost_regex-bcb-1_53\regex_debug.obj" +"bcb\libboost_regex-bcb-1_53\regex_raw_buffer.obj" +"bcb\libboost_regex-bcb-1_53\regex_traits_defaults.obj" +"bcb\libboost_regex-bcb-1_53\static_mutex.obj" +"bcb\libboost_regex-bcb-1_53\usinstances.obj" +"bcb\libboost_regex-bcb-1_53\w32_regex_traits.obj" +"bcb\libboost_regex-bcb-1_53\wc_regex_traits.obj" +"bcb\libboost_regex-bcb-1_53\wide_posix_api.obj" +"bcb\libboost_regex-bcb-1_53\winstances.obj"
|

########################################################
#
# section for libboost_regex-bcb-sd-1_53.lib
#
########################################################
bcb\libboost_regex-bcb-sd-1_53\c_regex_traits.obj: ../src/c_regex_traits.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWM- -D_NO_VCL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8037 -w-8057 -DSTRICT; -I$(BCROOT)\include;../../../ $(CXXFLAGS) -obcb\libboost_regex-bcb-sd-1_53\c_regex_traits.obj ../src/c_regex_traits.cpp
|

bcb\libboost_regex-bcb-sd-1_53\cpp_regex_traits.obj: ../src/cpp_regex_traits.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWM- -D_NO_VCL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8037 -w-8057 -DSTRICT; -I$(BCROOT)\include;../../../ $(CXXFLAGS) -obcb\libboost_regex-bcb-sd-1_53\cpp_regex_traits.obj ../src/cpp_regex_traits.cpp
|

bcb\libboost_regex-bcb-sd-1_53\cregex.obj: ../src/cregex.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWM- -D_NO_VCL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8037 -w-8057 -DSTRICT; -I$(BCROOT)\include;../../../ $(CXXFLAGS) -obcb\libboost_regex-bcb-sd-1_53\cregex.obj ../src/cregex.cpp
|

bcb\libboost_regex-bcb-sd-1_53\fileiter.obj: ../src/fileiter.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWM- -D_NO_VCL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8037 -w-8057 -DSTRICT; -I$(BCROOT)\include;../../../ $(CXXFLAGS) -obcb\libboost_regex-bcb-sd-1_53\fileiter.obj ../src/fileiter.cpp
|

bcb\libboost_regex-bcb-sd-1_53\icu.obj: ../src/icu.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWM- -D_NO_VCL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8037 -w-8057 -DSTRICT; -I$(BCROOT)\include;../../../ $(CXXFLAGS) -obcb\libboost_regex-bcb-sd-1_53\icu.obj ../src/icu.cpp
|

bcb\libboost_regex-bcb-sd-1_53\instances.obj: ../src/instances.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWM- -D_NO_VCL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8037 -w-8057 -DSTRICT; -I$(BCROOT)\include;../../../ $(CXXFLAGS) -obcb\libboost_regex-bcb-sd-1_53\instances.obj ../src/instances.cpp
|

bcb\libboost_regex-bcb-sd-1_53\posix_api.obj: ../src/posix_api.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWM- -D_NO_VCL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8037 -w-8057 -DSTRICT; -I$(BCROOT)\include;../../../ $(CXXFLAGS) -obcb\libboost_regex-bcb-sd-1_53\posix_api.obj ../src/posix_api.cpp
|

bcb\libboost_regex-bcb-sd-1_53\regex.obj: ../src/regex.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWM- -D_NO_VCL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8037 -w-8057 -DSTRICT; -I$(BCROOT)\include;../../../ $(CXXFLAGS) -obcb\libboost_regex-bcb-sd-1_53\regex.obj ../src/regex.cpp
|

bcb\libboost_regex-bcb-sd-1_53\regex_debug.obj: ../src/regex_debug.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWM- -D_NO_VCL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8037 -w-8057 -DSTRICT; -I$(BCROOT)\include;../../../ $(CXXFLAGS) -obcb\libboost_regex-bcb-sd-1_53\regex_debug.obj ../src/regex_debug.cpp
|

bcb\libboost_regex-bcb-sd-1_53\regex_raw_buffer.obj: ../src/regex_raw_buffer.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWM- -D_NO_VCL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8037 -w-8057 -DSTRICT; -I$(BCROOT)\include;../../../ $(CXXFLAGS) -obcb\libboost_regex-bcb-sd-1_53\regex_raw_buffer.obj ../src/regex_raw_buffer.cpp
|

bcb\libboost_regex-bcb-sd-1_53\regex_traits_defaults.obj: ../src/regex_traits_defaults.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWM- -D_NO_VCL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8037 -w-8057 -DSTRICT; -I$(BCROOT)\include;../../../ $(CXXFLAGS) -obcb\libboost_regex-bcb-sd-1_53\regex_traits_defaults.obj ../src/regex_traits_defaults.cpp
|

bcb\libboost_regex-bcb-sd-1_53\static_mutex.obj: ../src/static_mutex.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWM- -D_NO_VCL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8037 -w-8057 -DSTRICT; -I$(BCROOT)\include;../../../ $(CXXFLAGS) -obcb\libboost_regex-bcb-sd-1_53\static_mutex.obj ../src/static_mutex.cpp
|

bcb\libboost_regex-bcb-sd-1_53\usinstances.obj: ../src/usinstances.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWM- -D_NO_VCL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8037 -w-8057 -DSTRICT; -I$(BCROOT)\include;../../../ $(CXXFLAGS) -obcb\libboost_regex-bcb-sd-1_53\usinstances.obj ../src/usinstances.cpp
|

bcb\libboost_regex-bcb-sd-1_53\w32_regex_traits.obj: ../src/w32_regex_traits.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWM- -D_NO_VCL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8037 -w-8057 -DSTRICT; -I$(BCROOT)\include;../../../ $(CXXFLAGS) -obcb\libboost_regex-bcb-sd-1_53\w32_regex_traits.obj ../src/w32_regex_traits.cpp
|

bcb\libboost_regex-bcb-sd-1_53\wc_regex_traits.obj: ../src/wc_regex_traits.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWM- -D_NO_VCL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8037 -w-8057 -DSTRICT; -I$(BCROOT)\include;../../../ $(CXXFLAGS) -obcb\libboost_regex-bcb-sd-1_53\wc_regex_traits.obj ../src/wc_regex_traits.cpp
|

bcb\libboost_regex-bcb-sd-1_53\wide_posix_api.obj: ../src/wide_posix_api.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWM- -D_NO_VCL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8037 -w-8057 -DSTRICT; -I$(BCROOT)\include;../../../ $(CXXFLAGS) -obcb\libboost_regex-bcb-sd-1_53\wide_posix_api.obj ../src/wide_posix_api.cpp
|

bcb\libboost_regex-bcb-sd-1_53\winstances.obj: ../src/winstances.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWM- -D_NO_VCL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8037 -w-8057 -DSTRICT; -I$(BCROOT)\include;../../../ $(CXXFLAGS) -obcb\libboost_regex-bcb-sd-1_53\winstances.obj ../src/winstances.cpp
|

bcb\libboost_regex-bcb-sd-1_53 : 
	-@mkdir bcb\libboost_regex-bcb-sd-1_53

libboost_regex-bcb-sd-1_53_clean :
	del bcb\libboost_regex-bcb-sd-1_53\*.obj
	del bcb\libboost_regex-bcb-sd-1_53\*.il?
	del bcb\libboost_regex-bcb-sd-1_53\*.csm
	del bcb\libboost_regex-bcb-sd-1_53\*.tds

bcb\libboost_regex-bcb-sd-1_53.lib : bcb\libboost_regex-bcb-sd-1_53\c_regex_traits.obj bcb\libboost_regex-bcb-sd-1_53\cpp_regex_traits.obj bcb\libboost_regex-bcb-sd-1_53\cregex.obj bcb\libboost_regex-bcb-sd-1_53\fileiter.obj bcb\libboost_regex-bcb-sd-1_53\icu.obj bcb\libboost_regex-bcb-sd-1_53\instances.obj bcb\libboost_regex-bcb-sd-1_53\posix_api.obj bcb\libboost_regex-bcb-sd-1_53\regex.obj bcb\libboost_regex-bcb-sd-1_53\regex_debug.obj bcb\libboost_regex-bcb-sd-1_53\regex_raw_buffer.obj bcb\libboost_regex-bcb-sd-1_53\regex_traits_defaults.obj bcb\libboost_regex-bcb-sd-1_53\static_mutex.obj bcb\libboost_regex-bcb-sd-1_53\usinstances.obj bcb\libboost_regex-bcb-sd-1_53\w32_regex_traits.obj bcb\libboost_regex-bcb-sd-1_53\wc_regex_traits.obj bcb\libboost_regex-bcb-sd-1_53\wide_posix_api.obj bcb\libboost_regex-bcb-sd-1_53\winstances.obj
	if exist bcb\libboost_regex-bcb-sd-1_53.lib del bcb\libboost_regex-bcb-sd-1_53.lib 
	tlib @&&|
/P128 /C /u /a $(XSFLAGS) "bcb\libboost_regex-bcb-sd-1_53.lib"  +"bcb\libboost_regex-bcb-sd-1_53\c_regex_traits.obj" +"bcb\libboost_regex-bcb-sd-1_53\cpp_regex_traits.obj" +"bcb\libboost_regex-bcb-sd-1_53\cregex.obj" +"bcb\libboost_regex-bcb-sd-1_53\fileiter.obj" +"bcb\libboost_regex-bcb-sd-1_53\icu.obj" +"bcb\libboost_regex-bcb-sd-1_53\instances.obj" +"bcb\libboost_regex-bcb-sd-1_53\posix_api.obj" +"bcb\libboost_regex-bcb-sd-1_53\regex.obj" +"bcb\libboost_regex-bcb-sd-1_53\regex_debug.obj" +"bcb\libboost_regex-bcb-sd-1_53\regex_raw_buffer.obj" +"bcb\libboost_regex-bcb-sd-1_53\regex_traits_defaults.obj" +"bcb\libboost_regex-bcb-sd-1_53\static_mutex.obj" +"bcb\libboost_regex-bcb-sd-1_53\usinstances.obj" +"bcb\libboost_regex-bcb-sd-1_53\w32_regex_traits.obj" +"bcb\libboost_regex-bcb-sd-1_53\wc_regex_traits.obj" +"bcb\libboost_regex-bcb-sd-1_53\wide_posix_api.obj" +"bcb\libboost_regex-bcb-sd-1_53\winstances.obj"
|

########################################################
#
# section for libboost_regex-bcb-mt-sd-1_53.lib
#
########################################################
bcb\libboost_regex-bcb-mt-sd-1_53\c_regex_traits.obj: ../src/c_regex_traits.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWM -D_NO_VCL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ $(CXXFLAGS) -obcb\libboost_regex-bcb-mt-sd-1_53\c_regex_traits.obj ../src/c_regex_traits.cpp
|

bcb\libboost_regex-bcb-mt-sd-1_53\cpp_regex_traits.obj: ../src/cpp_regex_traits.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWM -D_NO_VCL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ $(CXXFLAGS) -obcb\libboost_regex-bcb-mt-sd-1_53\cpp_regex_traits.obj ../src/cpp_regex_traits.cpp
|

bcb\libboost_regex-bcb-mt-sd-1_53\cregex.obj: ../src/cregex.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWM -D_NO_VCL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ $(CXXFLAGS) -obcb\libboost_regex-bcb-mt-sd-1_53\cregex.obj ../src/cregex.cpp
|

bcb\libboost_regex-bcb-mt-sd-1_53\fileiter.obj: ../src/fileiter.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWM -D_NO_VCL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ $(CXXFLAGS) -obcb\libboost_regex-bcb-mt-sd-1_53\fileiter.obj ../src/fileiter.cpp
|

bcb\libboost_regex-bcb-mt-sd-1_53\icu.obj: ../src/icu.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWM -D_NO_VCL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ $(CXXFLAGS) -obcb\libboost_regex-bcb-mt-sd-1_53\icu.obj ../src/icu.cpp
|

bcb\libboost_regex-bcb-mt-sd-1_53\instances.obj: ../src/instances.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWM -D_NO_VCL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ $(CXXFLAGS) -obcb\libboost_regex-bcb-mt-sd-1_53\instances.obj ../src/instances.cpp
|

bcb\libboost_regex-bcb-mt-sd-1_53\posix_api.obj: ../src/posix_api.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWM -D_NO_VCL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ $(CXXFLAGS) -obcb\libboost_regex-bcb-mt-sd-1_53\posix_api.obj ../src/posix_api.cpp
|

bcb\libboost_regex-bcb-mt-sd-1_53\regex.obj: ../src/regex.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWM -D_NO_VCL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ $(CXXFLAGS) -obcb\libboost_regex-bcb-mt-sd-1_53\regex.obj ../src/regex.cpp
|

bcb\libboost_regex-bcb-mt-sd-1_53\regex_debug.obj: ../src/regex_debug.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWM -D_NO_VCL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ $(CXXFLAGS) -obcb\libboost_regex-bcb-mt-sd-1_53\regex_debug.obj ../src/regex_debug.cpp
|

bcb\libboost_regex-bcb-mt-sd-1_53\regex_raw_buffer.obj: ../src/regex_raw_buffer.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWM -D_NO_VCL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ $(CXXFLAGS) -obcb\libboost_regex-bcb-mt-sd-1_53\regex_raw_buffer.obj ../src/regex_raw_buffer.cpp
|

bcb\libboost_regex-bcb-mt-sd-1_53\regex_traits_defaults.obj: ../src/regex_traits_defaults.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWM -D_NO_VCL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ $(CXXFLAGS) -obcb\libboost_regex-bcb-mt-sd-1_53\regex_traits_defaults.obj ../src/regex_traits_defaults.cpp
|

bcb\libboost_regex-bcb-mt-sd-1_53\static_mutex.obj: ../src/static_mutex.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWM -D_NO_VCL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ $(CXXFLAGS) -obcb\libboost_regex-bcb-mt-sd-1_53\static_mutex.obj ../src/static_mutex.cpp
|

bcb\libboost_regex-bcb-mt-sd-1_53\usinstances.obj: ../src/usinstances.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWM -D_NO_VCL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ $(CXXFLAGS) -obcb\libboost_regex-bcb-mt-sd-1_53\usinstances.obj ../src/usinstances.cpp
|

bcb\libboost_regex-bcb-mt-sd-1_53\w32_regex_traits.obj: ../src/w32_regex_traits.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWM -D_NO_VCL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ $(CXXFLAGS) -obcb\libboost_regex-bcb-mt-sd-1_53\w32_regex_traits.obj ../src/w32_regex_traits.cpp
|

bcb\libboost_regex-bcb-mt-sd-1_53\wc_regex_traits.obj: ../src/wc_regex_traits.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWM -D_NO_VCL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ $(CXXFLAGS) -obcb\libboost_regex-bcb-mt-sd-1_53\wc_regex_traits.obj ../src/wc_regex_traits.cpp
|

bcb\libboost_regex-bcb-mt-sd-1_53\wide_posix_api.obj: ../src/wide_posix_api.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWM -D_NO_VCL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ $(CXXFLAGS) -obcb\libboost_regex-bcb-mt-sd-1_53\wide_posix_api.obj ../src/wide_posix_api.cpp
|

bcb\libboost_regex-bcb-mt-sd-1_53\winstances.obj: ../src/winstances.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWM -D_NO_VCL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ $(CXXFLAGS) -obcb\libboost_regex-bcb-mt-sd-1_53\winstances.obj ../src/winstances.cpp
|

bcb\libboost_regex-bcb-mt-sd-1_53 : 
	-@mkdir bcb\libboost_regex-bcb-mt-sd-1_53

libboost_regex-bcb-mt-sd-1_53_clean :
	del bcb\libboost_regex-bcb-mt-sd-1_53\*.obj
	del bcb\libboost_regex-bcb-mt-sd-1_53\*.il?
	del bcb\libboost_regex-bcb-mt-sd-1_53\*.csm
	del bcb\libboost_regex-bcb-mt-sd-1_53\*.tds

bcb\libboost_regex-bcb-mt-sd-1_53.lib : bcb\libboost_regex-bcb-mt-sd-1_53\c_regex_traits.obj bcb\libboost_regex-bcb-mt-sd-1_53\cpp_regex_traits.obj bcb\libboost_regex-bcb-mt-sd-1_53\cregex.obj bcb\libboost_regex-bcb-mt-sd-1_53\fileiter.obj bcb\libboost_regex-bcb-mt-sd-1_53\icu.obj bcb\libboost_regex-bcb-mt-sd-1_53\instances.obj bcb\libboost_regex-bcb-mt-sd-1_53\posix_api.obj bcb\libboost_regex-bcb-mt-sd-1_53\regex.obj bcb\libboost_regex-bcb-mt-sd-1_53\regex_debug.obj bcb\libboost_regex-bcb-mt-sd-1_53\regex_raw_buffer.obj bcb\libboost_regex-bcb-mt-sd-1_53\regex_traits_defaults.obj bcb\libboost_regex-bcb-mt-sd-1_53\static_mutex.obj bcb\libboost_regex-bcb-mt-sd-1_53\usinstances.obj bcb\libboost_regex-bcb-mt-sd-1_53\w32_regex_traits.obj bcb\libboost_regex-bcb-mt-sd-1_53\wc_regex_traits.obj bcb\libboost_regex-bcb-mt-sd-1_53\wide_posix_api.obj bcb\libboost_regex-bcb-mt-sd-1_53\winstances.obj
	if exist bcb\libboost_regex-bcb-mt-sd-1_53.lib del bcb\libboost_regex-bcb-mt-sd-1_53.lib 
	tlib @&&|
/P128 /C /u /a $(XSFLAGS) "bcb\libboost_regex-bcb-mt-sd-1_53.lib"  +"bcb\libboost_regex-bcb-mt-sd-1_53\c_regex_traits.obj" +"bcb\libboost_regex-bcb-mt-sd-1_53\cpp_regex_traits.obj" +"bcb\libboost_regex-bcb-mt-sd-1_53\cregex.obj" +"bcb\libboost_regex-bcb-mt-sd-1_53\fileiter.obj" +"bcb\libboost_regex-bcb-mt-sd-1_53\icu.obj" +"bcb\libboost_regex-bcb-mt-sd-1_53\instances.obj" +"bcb\libboost_regex-bcb-mt-sd-1_53\posix_api.obj" +"bcb\libboost_regex-bcb-mt-sd-1_53\regex.obj" +"bcb\libboost_regex-bcb-mt-sd-1_53\regex_debug.obj" +"bcb\libboost_regex-bcb-mt-sd-1_53\regex_raw_buffer.obj" +"bcb\libboost_regex-bcb-mt-sd-1_53\regex_traits_defaults.obj" +"bcb\libboost_regex-bcb-mt-sd-1_53\static_mutex.obj" +"bcb\libboost_regex-bcb-mt-sd-1_53\usinstances.obj" +"bcb\libboost_regex-bcb-mt-sd-1_53\w32_regex_traits.obj" +"bcb\libboost_regex-bcb-mt-sd-1_53\wc_regex_traits.obj" +"bcb\libboost_regex-bcb-mt-sd-1_53\wide_posix_api.obj" +"bcb\libboost_regex-bcb-mt-sd-1_53\winstances.obj"
|

########################################################
#
# section for boost_regex-bcb-mt-d-1_53.lib
#
########################################################
bcb\boost_regex-bcb-mt-d-1_53\c_regex_traits.obj: ../src/c_regex_traits.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWM -tWR -D_NO_VCL -D_RTLDLL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -DBOOST_REGEX_DYN_LINK -obcb\boost_regex-bcb-mt-d-1_53\c_regex_traits.obj ../src/c_regex_traits.cpp
|

bcb\boost_regex-bcb-mt-d-1_53\cpp_regex_traits.obj: ../src/cpp_regex_traits.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWM -tWR -D_NO_VCL -D_RTLDLL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -DBOOST_REGEX_DYN_LINK -obcb\boost_regex-bcb-mt-d-1_53\cpp_regex_traits.obj ../src/cpp_regex_traits.cpp
|

bcb\boost_regex-bcb-mt-d-1_53\cregex.obj: ../src/cregex.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWM -tWR -D_NO_VCL -D_RTLDLL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -DBOOST_REGEX_DYN_LINK -obcb\boost_regex-bcb-mt-d-1_53\cregex.obj ../src/cregex.cpp
|

bcb\boost_regex-bcb-mt-d-1_53\fileiter.obj: ../src/fileiter.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWM -tWR -D_NO_VCL -D_RTLDLL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -DBOOST_REGEX_DYN_LINK -obcb\boost_regex-bcb-mt-d-1_53\fileiter.obj ../src/fileiter.cpp
|

bcb\boost_regex-bcb-mt-d-1_53\icu.obj: ../src/icu.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWM -tWR -D_NO_VCL -D_RTLDLL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -DBOOST_REGEX_DYN_LINK -obcb\boost_regex-bcb-mt-d-1_53\icu.obj ../src/icu.cpp
|

bcb\boost_regex-bcb-mt-d-1_53\instances.obj: ../src/instances.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWM -tWR -D_NO_VCL -D_RTLDLL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -DBOOST_REGEX_DYN_LINK -obcb\boost_regex-bcb-mt-d-1_53\instances.obj ../src/instances.cpp
|

bcb\boost_regex-bcb-mt-d-1_53\posix_api.obj: ../src/posix_api.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWM -tWR -D_NO_VCL -D_RTLDLL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -DBOOST_REGEX_DYN_LINK -obcb\boost_regex-bcb-mt-d-1_53\posix_api.obj ../src/posix_api.cpp
|

bcb\boost_regex-bcb-mt-d-1_53\regex.obj: ../src/regex.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWM -tWR -D_NO_VCL -D_RTLDLL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -DBOOST_REGEX_DYN_LINK -obcb\boost_regex-bcb-mt-d-1_53\regex.obj ../src/regex.cpp
|

bcb\boost_regex-bcb-mt-d-1_53\regex_debug.obj: ../src/regex_debug.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWM -tWR -D_NO_VCL -D_RTLDLL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -DBOOST_REGEX_DYN_LINK -obcb\boost_regex-bcb-mt-d-1_53\regex_debug.obj ../src/regex_debug.cpp
|

bcb\boost_regex-bcb-mt-d-1_53\regex_raw_buffer.obj: ../src/regex_raw_buffer.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWM -tWR -D_NO_VCL -D_RTLDLL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -DBOOST_REGEX_DYN_LINK -obcb\boost_regex-bcb-mt-d-1_53\regex_raw_buffer.obj ../src/regex_raw_buffer.cpp
|

bcb\boost_regex-bcb-mt-d-1_53\regex_traits_defaults.obj: ../src/regex_traits_defaults.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWM -tWR -D_NO_VCL -D_RTLDLL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -DBOOST_REGEX_DYN_LINK -obcb\boost_regex-bcb-mt-d-1_53\regex_traits_defaults.obj ../src/regex_traits_defaults.cpp
|

bcb\boost_regex-bcb-mt-d-1_53\static_mutex.obj: ../src/static_mutex.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWM -tWR -D_NO_VCL -D_RTLDLL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -DBOOST_REGEX_DYN_LINK -obcb\boost_regex-bcb-mt-d-1_53\static_mutex.obj ../src/static_mutex.cpp
|

bcb\boost_regex-bcb-mt-d-1_53\usinstances.obj: ../src/usinstances.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWM -tWR -D_NO_VCL -D_RTLDLL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -DBOOST_REGEX_DYN_LINK -obcb\boost_regex-bcb-mt-d-1_53\usinstances.obj ../src/usinstances.cpp
|

bcb\boost_regex-bcb-mt-d-1_53\w32_regex_traits.obj: ../src/w32_regex_traits.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWM -tWR -D_NO_VCL -D_RTLDLL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -DBOOST_REGEX_DYN_LINK -obcb\boost_regex-bcb-mt-d-1_53\w32_regex_traits.obj ../src/w32_regex_traits.cpp
|

bcb\boost_regex-bcb-mt-d-1_53\wc_regex_traits.obj: ../src/wc_regex_traits.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWM -tWR -D_NO_VCL -D_RTLDLL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -DBOOST_REGEX_DYN_LINK -obcb\boost_regex-bcb-mt-d-1_53\wc_regex_traits.obj ../src/wc_regex_traits.cpp
|

bcb\boost_regex-bcb-mt-d-1_53\wide_posix_api.obj: ../src/wide_posix_api.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWM -tWR -D_NO_VCL -D_RTLDLL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -DBOOST_REGEX_DYN_LINK -obcb\boost_regex-bcb-mt-d-1_53\wide_posix_api.obj ../src/wide_posix_api.cpp
|

bcb\boost_regex-bcb-mt-d-1_53\winstances.obj: ../src/winstances.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWM -tWR -D_NO_VCL -D_RTLDLL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -DBOOST_REGEX_DYN_LINK -obcb\boost_regex-bcb-mt-d-1_53\winstances.obj ../src/winstances.cpp
|

bcb\boost_regex-bcb-mt-d-1_53 :
	-@mkdir bcb\boost_regex-bcb-mt-d-1_53

boost_regex-bcb-mt-d-1_53_clean :
	del bcb\boost_regex-bcb-mt-d-1_53\*.obj
	del bcb\boost_regex-bcb-mt-d-1_53\*.il?
	del bcb\boost_regex-bcb-mt-d-1_53\*.csm
	del bcb\boost_regex-bcb-mt-d-1_53\*.tds
	del bcb\*.tds

bcb\boost_regex-bcb-mt-d-1_53.lib : bcb\boost_regex-bcb-mt-d-1_53\c_regex_traits.obj bcb\boost_regex-bcb-mt-d-1_53\cpp_regex_traits.obj bcb\boost_regex-bcb-mt-d-1_53\cregex.obj bcb\boost_regex-bcb-mt-d-1_53\fileiter.obj bcb\boost_regex-bcb-mt-d-1_53\icu.obj bcb\boost_regex-bcb-mt-d-1_53\instances.obj bcb\boost_regex-bcb-mt-d-1_53\posix_api.obj bcb\boost_regex-bcb-mt-d-1_53\regex.obj bcb\boost_regex-bcb-mt-d-1_53\regex_debug.obj bcb\boost_regex-bcb-mt-d-1_53\regex_raw_buffer.obj bcb\boost_regex-bcb-mt-d-1_53\regex_traits_defaults.obj bcb\boost_regex-bcb-mt-d-1_53\static_mutex.obj bcb\boost_regex-bcb-mt-d-1_53\usinstances.obj bcb\boost_regex-bcb-mt-d-1_53\w32_regex_traits.obj bcb\boost_regex-bcb-mt-d-1_53\wc_regex_traits.obj bcb\boost_regex-bcb-mt-d-1_53\wide_posix_api.obj bcb\boost_regex-bcb-mt-d-1_53\winstances.obj
	bcc32 @&&|
-lw-dup -lw-dpl -tWD -tWM -tWR -D_NO_VCL -D_RTLDLL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; -ebcb\boost_regex-bcb-mt-d-1_53.dll $(XLFLAGS)  bcb\boost_regex-bcb-mt-d-1_53\c_regex_traits.obj bcb\boost_regex-bcb-mt-d-1_53\cpp_regex_traits.obj bcb\boost_regex-bcb-mt-d-1_53\cregex.obj bcb\boost_regex-bcb-mt-d-1_53\fileiter.obj bcb\boost_regex-bcb-mt-d-1_53\icu.obj bcb\boost_regex-bcb-mt-d-1_53\instances.obj bcb\boost_regex-bcb-mt-d-1_53\posix_api.obj bcb\boost_regex-bcb-mt-d-1_53\regex.obj bcb\boost_regex-bcb-mt-d-1_53\regex_debug.obj bcb\boost_regex-bcb-mt-d-1_53\regex_raw_buffer.obj bcb\boost_regex-bcb-mt-d-1_53\regex_traits_defaults.obj bcb\boost_regex-bcb-mt-d-1_53\static_mutex.obj bcb\boost_regex-bcb-mt-d-1_53\usinstances.obj bcb\boost_regex-bcb-mt-d-1_53\w32_regex_traits.obj bcb\boost_regex-bcb-mt-d-1_53\wc_regex_traits.obj bcb\boost_regex-bcb-mt-d-1_53\wide_posix_api.obj bcb\boost_regex-bcb-mt-d-1_53\winstances.obj $(LIBS)
|
	implib -w bcb\boost_regex-bcb-mt-d-1_53.lib bcb\boost_regex-bcb-mt-d-1_53.dll

########################################################
#
# section for boost_regex-bcb-d-1_53.lib
#
########################################################
bcb\boost_regex-bcb-d-1_53\c_regex_traits.obj: ../src/c_regex_traits.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWR -tWM- -D_NO_VCL -D_RTLDLL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -DBOOST_REGEX_DYN_LINK -obcb\boost_regex-bcb-d-1_53\c_regex_traits.obj ../src/c_regex_traits.cpp
|

bcb\boost_regex-bcb-d-1_53\cpp_regex_traits.obj: ../src/cpp_regex_traits.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWR -tWM- -D_NO_VCL -D_RTLDLL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -DBOOST_REGEX_DYN_LINK -obcb\boost_regex-bcb-d-1_53\cpp_regex_traits.obj ../src/cpp_regex_traits.cpp
|

bcb\boost_regex-bcb-d-1_53\cregex.obj: ../src/cregex.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWR -tWM- -D_NO_VCL -D_RTLDLL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -DBOOST_REGEX_DYN_LINK -obcb\boost_regex-bcb-d-1_53\cregex.obj ../src/cregex.cpp
|

bcb\boost_regex-bcb-d-1_53\fileiter.obj: ../src/fileiter.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWR -tWM- -D_NO_VCL -D_RTLDLL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -DBOOST_REGEX_DYN_LINK -obcb\boost_regex-bcb-d-1_53\fileiter.obj ../src/fileiter.cpp
|

bcb\boost_regex-bcb-d-1_53\icu.obj: ../src/icu.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWR -tWM- -D_NO_VCL -D_RTLDLL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -DBOOST_REGEX_DYN_LINK -obcb\boost_regex-bcb-d-1_53\icu.obj ../src/icu.cpp
|

bcb\boost_regex-bcb-d-1_53\instances.obj: ../src/instances.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWR -tWM- -D_NO_VCL -D_RTLDLL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -DBOOST_REGEX_DYN_LINK -obcb\boost_regex-bcb-d-1_53\instances.obj ../src/instances.cpp
|

bcb\boost_regex-bcb-d-1_53\posix_api.obj: ../src/posix_api.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWR -tWM- -D_NO_VCL -D_RTLDLL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -DBOOST_REGEX_DYN_LINK -obcb\boost_regex-bcb-d-1_53\posix_api.obj ../src/posix_api.cpp
|

bcb\boost_regex-bcb-d-1_53\regex.obj: ../src/regex.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWR -tWM- -D_NO_VCL -D_RTLDLL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -DBOOST_REGEX_DYN_LINK -obcb\boost_regex-bcb-d-1_53\regex.obj ../src/regex.cpp
|

bcb\boost_regex-bcb-d-1_53\regex_debug.obj: ../src/regex_debug.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWR -tWM- -D_NO_VCL -D_RTLDLL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -DBOOST_REGEX_DYN_LINK -obcb\boost_regex-bcb-d-1_53\regex_debug.obj ../src/regex_debug.cpp
|

bcb\boost_regex-bcb-d-1_53\regex_raw_buffer.obj: ../src/regex_raw_buffer.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWR -tWM- -D_NO_VCL -D_RTLDLL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -DBOOST_REGEX_DYN_LINK -obcb\boost_regex-bcb-d-1_53\regex_raw_buffer.obj ../src/regex_raw_buffer.cpp
|

bcb\boost_regex-bcb-d-1_53\regex_traits_defaults.obj: ../src/regex_traits_defaults.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWR -tWM- -D_NO_VCL -D_RTLDLL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -DBOOST_REGEX_DYN_LINK -obcb\boost_regex-bcb-d-1_53\regex_traits_defaults.obj ../src/regex_traits_defaults.cpp
|

bcb\boost_regex-bcb-d-1_53\static_mutex.obj: ../src/static_mutex.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWR -tWM- -D_NO_VCL -D_RTLDLL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -DBOOST_REGEX_DYN_LINK -obcb\boost_regex-bcb-d-1_53\static_mutex.obj ../src/static_mutex.cpp
|

bcb\boost_regex-bcb-d-1_53\usinstances.obj: ../src/usinstances.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWR -tWM- -D_NO_VCL -D_RTLDLL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -DBOOST_REGEX_DYN_LINK -obcb\boost_regex-bcb-d-1_53\usinstances.obj ../src/usinstances.cpp
|

bcb\boost_regex-bcb-d-1_53\w32_regex_traits.obj: ../src/w32_regex_traits.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWR -tWM- -D_NO_VCL -D_RTLDLL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -DBOOST_REGEX_DYN_LINK -obcb\boost_regex-bcb-d-1_53\w32_regex_traits.obj ../src/w32_regex_traits.cpp
|

bcb\boost_regex-bcb-d-1_53\wc_regex_traits.obj: ../src/wc_regex_traits.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWR -tWM- -D_NO_VCL -D_RTLDLL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -DBOOST_REGEX_DYN_LINK -obcb\boost_regex-bcb-d-1_53\wc_regex_traits.obj ../src/wc_regex_traits.cpp
|

bcb\boost_regex-bcb-d-1_53\wide_posix_api.obj: ../src/wide_posix_api.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWR -tWM- -D_NO_VCL -D_RTLDLL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -DBOOST_REGEX_DYN_LINK -obcb\boost_regex-bcb-d-1_53\wide_posix_api.obj ../src/wide_posix_api.cpp
|

bcb\boost_regex-bcb-d-1_53\winstances.obj: ../src/winstances.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWR -tWM- -D_NO_VCL -D_RTLDLL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -DBOOST_REGEX_DYN_LINK -obcb\boost_regex-bcb-d-1_53\winstances.obj ../src/winstances.cpp
|

bcb\boost_regex-bcb-d-1_53 :
	-@mkdir bcb\boost_regex-bcb-d-1_53

boost_regex-bcb-d-1_53_clean :
	del bcb\boost_regex-bcb-d-1_53\*.obj
	del bcb\boost_regex-bcb-d-1_53\*.il?
	del bcb\boost_regex-bcb-d-1_53\*.csm
	del bcb\boost_regex-bcb-d-1_53\*.tds
	del bcb\*.tds

bcb\boost_regex-bcb-d-1_53.lib : bcb\boost_regex-bcb-d-1_53\c_regex_traits.obj bcb\boost_regex-bcb-d-1_53\cpp_regex_traits.obj bcb\boost_regex-bcb-d-1_53\cregex.obj bcb\boost_regex-bcb-d-1_53\fileiter.obj bcb\boost_regex-bcb-d-1_53\icu.obj bcb\boost_regex-bcb-d-1_53\instances.obj bcb\boost_regex-bcb-d-1_53\posix_api.obj bcb\boost_regex-bcb-d-1_53\regex.obj bcb\boost_regex-bcb-d-1_53\regex_debug.obj bcb\boost_regex-bcb-d-1_53\regex_raw_buffer.obj bcb\boost_regex-bcb-d-1_53\regex_traits_defaults.obj bcb\boost_regex-bcb-d-1_53\static_mutex.obj bcb\boost_regex-bcb-d-1_53\usinstances.obj bcb\boost_regex-bcb-d-1_53\w32_regex_traits.obj bcb\boost_regex-bcb-d-1_53\wc_regex_traits.obj bcb\boost_regex-bcb-d-1_53\wide_posix_api.obj bcb\boost_regex-bcb-d-1_53\winstances.obj
	bcc32 @&&|
-lw-dup -lw-dpl -tWD -tWR -tWM- -D_NO_VCL -D_RTLDLL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; -ebcb\boost_regex-bcb-d-1_53.dll $(XLFLAGS)  bcb\boost_regex-bcb-d-1_53\c_regex_traits.obj bcb\boost_regex-bcb-d-1_53\cpp_regex_traits.obj bcb\boost_regex-bcb-d-1_53\cregex.obj bcb\boost_regex-bcb-d-1_53\fileiter.obj bcb\boost_regex-bcb-d-1_53\icu.obj bcb\boost_regex-bcb-d-1_53\instances.obj bcb\boost_regex-bcb-d-1_53\posix_api.obj bcb\boost_regex-bcb-d-1_53\regex.obj bcb\boost_regex-bcb-d-1_53\regex_debug.obj bcb\boost_regex-bcb-d-1_53\regex_raw_buffer.obj bcb\boost_regex-bcb-d-1_53\regex_traits_defaults.obj bcb\boost_regex-bcb-d-1_53\static_mutex.obj bcb\boost_regex-bcb-d-1_53\usinstances.obj bcb\boost_regex-bcb-d-1_53\w32_regex_traits.obj bcb\boost_regex-bcb-d-1_53\wc_regex_traits.obj bcb\boost_regex-bcb-d-1_53\wide_posix_api.obj bcb\boost_regex-bcb-d-1_53\winstances.obj $(LIBS)
|
	implib -w bcb\boost_regex-bcb-d-1_53.lib bcb\boost_regex-bcb-d-1_53.dll

########################################################
#
# section for libboost_regex-bcb-mt-d-1_53.lib
#
########################################################
bcb\libboost_regex-bcb-mt-d-1_53\c_regex_traits.obj: ../src/c_regex_traits.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWM -tWR -DBOOST_REGEX_STATIC_LINK -D_NO_VCL -D_RTLDLL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -obcb\libboost_regex-bcb-mt-d-1_53\c_regex_traits.obj ../src/c_regex_traits.cpp
|

bcb\libboost_regex-bcb-mt-d-1_53\cpp_regex_traits.obj: ../src/cpp_regex_traits.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWM -tWR -DBOOST_REGEX_STATIC_LINK -D_NO_VCL -D_RTLDLL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -obcb\libboost_regex-bcb-mt-d-1_53\cpp_regex_traits.obj ../src/cpp_regex_traits.cpp
|

bcb\libboost_regex-bcb-mt-d-1_53\cregex.obj: ../src/cregex.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWM -tWR -DBOOST_REGEX_STATIC_LINK -D_NO_VCL -D_RTLDLL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -obcb\libboost_regex-bcb-mt-d-1_53\cregex.obj ../src/cregex.cpp
|

bcb\libboost_regex-bcb-mt-d-1_53\fileiter.obj: ../src/fileiter.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWM -tWR -DBOOST_REGEX_STATIC_LINK -D_NO_VCL -D_RTLDLL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -obcb\libboost_regex-bcb-mt-d-1_53\fileiter.obj ../src/fileiter.cpp
|

bcb\libboost_regex-bcb-mt-d-1_53\icu.obj: ../src/icu.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWM -tWR -DBOOST_REGEX_STATIC_LINK -D_NO_VCL -D_RTLDLL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -obcb\libboost_regex-bcb-mt-d-1_53\icu.obj ../src/icu.cpp
|

bcb\libboost_regex-bcb-mt-d-1_53\instances.obj: ../src/instances.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWM -tWR -DBOOST_REGEX_STATIC_LINK -D_NO_VCL -D_RTLDLL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -obcb\libboost_regex-bcb-mt-d-1_53\instances.obj ../src/instances.cpp
|

bcb\libboost_regex-bcb-mt-d-1_53\posix_api.obj: ../src/posix_api.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWM -tWR -DBOOST_REGEX_STATIC_LINK -D_NO_VCL -D_RTLDLL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -obcb\libboost_regex-bcb-mt-d-1_53\posix_api.obj ../src/posix_api.cpp
|

bcb\libboost_regex-bcb-mt-d-1_53\regex.obj: ../src/regex.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWM -tWR -DBOOST_REGEX_STATIC_LINK -D_NO_VCL -D_RTLDLL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -obcb\libboost_regex-bcb-mt-d-1_53\regex.obj ../src/regex.cpp
|

bcb\libboost_regex-bcb-mt-d-1_53\regex_debug.obj: ../src/regex_debug.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWM -tWR -DBOOST_REGEX_STATIC_LINK -D_NO_VCL -D_RTLDLL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -obcb\libboost_regex-bcb-mt-d-1_53\regex_debug.obj ../src/regex_debug.cpp
|

bcb\libboost_regex-bcb-mt-d-1_53\regex_raw_buffer.obj: ../src/regex_raw_buffer.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWM -tWR -DBOOST_REGEX_STATIC_LINK -D_NO_VCL -D_RTLDLL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -obcb\libboost_regex-bcb-mt-d-1_53\regex_raw_buffer.obj ../src/regex_raw_buffer.cpp
|

bcb\libboost_regex-bcb-mt-d-1_53\regex_traits_defaults.obj: ../src/regex_traits_defaults.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWM -tWR -DBOOST_REGEX_STATIC_LINK -D_NO_VCL -D_RTLDLL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -obcb\libboost_regex-bcb-mt-d-1_53\regex_traits_defaults.obj ../src/regex_traits_defaults.cpp
|

bcb\libboost_regex-bcb-mt-d-1_53\static_mutex.obj: ../src/static_mutex.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWM -tWR -DBOOST_REGEX_STATIC_LINK -D_NO_VCL -D_RTLDLL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -obcb\libboost_regex-bcb-mt-d-1_53\static_mutex.obj ../src/static_mutex.cpp
|

bcb\libboost_regex-bcb-mt-d-1_53\usinstances.obj: ../src/usinstances.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWM -tWR -DBOOST_REGEX_STATIC_LINK -D_NO_VCL -D_RTLDLL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -obcb\libboost_regex-bcb-mt-d-1_53\usinstances.obj ../src/usinstances.cpp
|

bcb\libboost_regex-bcb-mt-d-1_53\w32_regex_traits.obj: ../src/w32_regex_traits.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWM -tWR -DBOOST_REGEX_STATIC_LINK -D_NO_VCL -D_RTLDLL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -obcb\libboost_regex-bcb-mt-d-1_53\w32_regex_traits.obj ../src/w32_regex_traits.cpp
|

bcb\libboost_regex-bcb-mt-d-1_53\wc_regex_traits.obj: ../src/wc_regex_traits.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWM -tWR -DBOOST_REGEX_STATIC_LINK -D_NO_VCL -D_RTLDLL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -obcb\libboost_regex-bcb-mt-d-1_53\wc_regex_traits.obj ../src/wc_regex_traits.cpp
|

bcb\libboost_regex-bcb-mt-d-1_53\wide_posix_api.obj: ../src/wide_posix_api.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWM -tWR -DBOOST_REGEX_STATIC_LINK -D_NO_VCL -D_RTLDLL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -obcb\libboost_regex-bcb-mt-d-1_53\wide_posix_api.obj ../src/wide_posix_api.cpp
|

bcb\libboost_regex-bcb-mt-d-1_53\winstances.obj: ../src/winstances.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWM -tWR -DBOOST_REGEX_STATIC_LINK -D_NO_VCL -D_RTLDLL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -obcb\libboost_regex-bcb-mt-d-1_53\winstances.obj ../src/winstances.cpp
|

bcb\libboost_regex-bcb-mt-d-1_53 : 
	-@mkdir bcb\libboost_regex-bcb-mt-d-1_53

libboost_regex-bcb-mt-d-1_53_clean :
	del bcb\libboost_regex-bcb-mt-d-1_53\*.obj
	del bcb\libboost_regex-bcb-mt-d-1_53\*.il?
	del bcb\libboost_regex-bcb-mt-d-1_53\*.csm
	del bcb\libboost_regex-bcb-mt-d-1_53\*.tds

bcb\libboost_regex-bcb-mt-d-1_53.lib : bcb\libboost_regex-bcb-mt-d-1_53\c_regex_traits.obj bcb\libboost_regex-bcb-mt-d-1_53\cpp_regex_traits.obj bcb\libboost_regex-bcb-mt-d-1_53\cregex.obj bcb\libboost_regex-bcb-mt-d-1_53\fileiter.obj bcb\libboost_regex-bcb-mt-d-1_53\icu.obj bcb\libboost_regex-bcb-mt-d-1_53\instances.obj bcb\libboost_regex-bcb-mt-d-1_53\posix_api.obj bcb\libboost_regex-bcb-mt-d-1_53\regex.obj bcb\libboost_regex-bcb-mt-d-1_53\regex_debug.obj bcb\libboost_regex-bcb-mt-d-1_53\regex_raw_buffer.obj bcb\libboost_regex-bcb-mt-d-1_53\regex_traits_defaults.obj bcb\libboost_regex-bcb-mt-d-1_53\static_mutex.obj bcb\libboost_regex-bcb-mt-d-1_53\usinstances.obj bcb\libboost_regex-bcb-mt-d-1_53\w32_regex_traits.obj bcb\libboost_regex-bcb-mt-d-1_53\wc_regex_traits.obj bcb\libboost_regex-bcb-mt-d-1_53\wide_posix_api.obj bcb\libboost_regex-bcb-mt-d-1_53\winstances.obj
	if exist bcb\libboost_regex-bcb-mt-d-1_53.lib del bcb\libboost_regex-bcb-mt-d-1_53.lib 
	tlib @&&|
/P128 /C /u /a $(XSFLAGS) "bcb\libboost_regex-bcb-mt-d-1_53.lib"  +"bcb\libboost_regex-bcb-mt-d-1_53\c_regex_traits.obj" +"bcb\libboost_regex-bcb-mt-d-1_53\cpp_regex_traits.obj" +"bcb\libboost_regex-bcb-mt-d-1_53\cregex.obj" +"bcb\libboost_regex-bcb-mt-d-1_53\fileiter.obj" +"bcb\libboost_regex-bcb-mt-d-1_53\icu.obj" +"bcb\libboost_regex-bcb-mt-d-1_53\instances.obj" +"bcb\libboost_regex-bcb-mt-d-1_53\posix_api.obj" +"bcb\libboost_regex-bcb-mt-d-1_53\regex.obj" +"bcb\libboost_regex-bcb-mt-d-1_53\regex_debug.obj" +"bcb\libboost_regex-bcb-mt-d-1_53\regex_raw_buffer.obj" +"bcb\libboost_regex-bcb-mt-d-1_53\regex_traits_defaults.obj" +"bcb\libboost_regex-bcb-mt-d-1_53\static_mutex.obj" +"bcb\libboost_regex-bcb-mt-d-1_53\usinstances.obj" +"bcb\libboost_regex-bcb-mt-d-1_53\w32_regex_traits.obj" +"bcb\libboost_regex-bcb-mt-d-1_53\wc_regex_traits.obj" +"bcb\libboost_regex-bcb-mt-d-1_53\wide_posix_api.obj" +"bcb\libboost_regex-bcb-mt-d-1_53\winstances.obj"
|

########################################################
#
# section for libboost_regex-bcb-d-1_53.lib
#
########################################################
bcb\libboost_regex-bcb-d-1_53\c_regex_traits.obj: ../src/c_regex_traits.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWR -tWM- -DBOOST_REGEX_STATIC_LINK -D_NO_VCL -D_RTLDLL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -obcb\libboost_regex-bcb-d-1_53\c_regex_traits.obj ../src/c_regex_traits.cpp
|

bcb\libboost_regex-bcb-d-1_53\cpp_regex_traits.obj: ../src/cpp_regex_traits.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWR -tWM- -DBOOST_REGEX_STATIC_LINK -D_NO_VCL -D_RTLDLL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -obcb\libboost_regex-bcb-d-1_53\cpp_regex_traits.obj ../src/cpp_regex_traits.cpp
|

bcb\libboost_regex-bcb-d-1_53\cregex.obj: ../src/cregex.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWR -tWM- -DBOOST_REGEX_STATIC_LINK -D_NO_VCL -D_RTLDLL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -obcb\libboost_regex-bcb-d-1_53\cregex.obj ../src/cregex.cpp
|

bcb\libboost_regex-bcb-d-1_53\fileiter.obj: ../src/fileiter.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWR -tWM- -DBOOST_REGEX_STATIC_LINK -D_NO_VCL -D_RTLDLL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -obcb\libboost_regex-bcb-d-1_53\fileiter.obj ../src/fileiter.cpp
|

bcb\libboost_regex-bcb-d-1_53\icu.obj: ../src/icu.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWR -tWM- -DBOOST_REGEX_STATIC_LINK -D_NO_VCL -D_RTLDLL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -obcb\libboost_regex-bcb-d-1_53\icu.obj ../src/icu.cpp
|

bcb\libboost_regex-bcb-d-1_53\instances.obj: ../src/instances.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWR -tWM- -DBOOST_REGEX_STATIC_LINK -D_NO_VCL -D_RTLDLL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -obcb\libboost_regex-bcb-d-1_53\instances.obj ../src/instances.cpp
|

bcb\libboost_regex-bcb-d-1_53\posix_api.obj: ../src/posix_api.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWR -tWM- -DBOOST_REGEX_STATIC_LINK -D_NO_VCL -D_RTLDLL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -obcb\libboost_regex-bcb-d-1_53\posix_api.obj ../src/posix_api.cpp
|

bcb\libboost_regex-bcb-d-1_53\regex.obj: ../src/regex.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWR -tWM- -DBOOST_REGEX_STATIC_LINK -D_NO_VCL -D_RTLDLL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -obcb\libboost_regex-bcb-d-1_53\regex.obj ../src/regex.cpp
|

bcb\libboost_regex-bcb-d-1_53\regex_debug.obj: ../src/regex_debug.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWR -tWM- -DBOOST_REGEX_STATIC_LINK -D_NO_VCL -D_RTLDLL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -obcb\libboost_regex-bcb-d-1_53\regex_debug.obj ../src/regex_debug.cpp
|

bcb\libboost_regex-bcb-d-1_53\regex_raw_buffer.obj: ../src/regex_raw_buffer.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWR -tWM- -DBOOST_REGEX_STATIC_LINK -D_NO_VCL -D_RTLDLL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -obcb\libboost_regex-bcb-d-1_53\regex_raw_buffer.obj ../src/regex_raw_buffer.cpp
|

bcb\libboost_regex-bcb-d-1_53\regex_traits_defaults.obj: ../src/regex_traits_defaults.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWR -tWM- -DBOOST_REGEX_STATIC_LINK -D_NO_VCL -D_RTLDLL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -obcb\libboost_regex-bcb-d-1_53\regex_traits_defaults.obj ../src/regex_traits_defaults.cpp
|

bcb\libboost_regex-bcb-d-1_53\static_mutex.obj: ../src/static_mutex.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWR -tWM- -DBOOST_REGEX_STATIC_LINK -D_NO_VCL -D_RTLDLL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -obcb\libboost_regex-bcb-d-1_53\static_mutex.obj ../src/static_mutex.cpp
|

bcb\libboost_regex-bcb-d-1_53\usinstances.obj: ../src/usinstances.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWR -tWM- -DBOOST_REGEX_STATIC_LINK -D_NO_VCL -D_RTLDLL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -obcb\libboost_regex-bcb-d-1_53\usinstances.obj ../src/usinstances.cpp
|

bcb\libboost_regex-bcb-d-1_53\w32_regex_traits.obj: ../src/w32_regex_traits.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWR -tWM- -DBOOST_REGEX_STATIC_LINK -D_NO_VCL -D_RTLDLL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -obcb\libboost_regex-bcb-d-1_53\w32_regex_traits.obj ../src/w32_regex_traits.cpp
|

bcb\libboost_regex-bcb-d-1_53\wc_regex_traits.obj: ../src/wc_regex_traits.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWR -tWM- -DBOOST_REGEX_STATIC_LINK -D_NO_VCL -D_RTLDLL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -obcb\libboost_regex-bcb-d-1_53\wc_regex_traits.obj ../src/wc_regex_traits.cpp
|

bcb\libboost_regex-bcb-d-1_53\wide_posix_api.obj: ../src/wide_posix_api.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWR -tWM- -DBOOST_REGEX_STATIC_LINK -D_NO_VCL -D_RTLDLL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -obcb\libboost_regex-bcb-d-1_53\wide_posix_api.obj ../src/wide_posix_api.cpp
|

bcb\libboost_regex-bcb-d-1_53\winstances.obj: ../src/winstances.cpp $(ALL_HEADER)
	bcc32 @&&|
-c $(INCLUDES) -tWD -tWR -tWM- -DBOOST_REGEX_STATIC_LINK -D_NO_VCL -D_RTLDLL -v -Ve -Vx -w-inl -w-aus -w-rch -w-8012 -w-8057 -w-8037 -DSTRICT; -I$(BCROOT)\include;../../../ -L$(BCROOT)\lib;$(BCROOT)\lib\release; $(CXXFLAGS) -obcb\libboost_regex-bcb-d-1_53\winstances.obj ../src/winstances.cpp
|

bcb\libboost_regex-bcb-d-1_53 : 
	-@mkdir bcb\libboost_regex-bcb-d-1_53

libboost_regex-bcb-d-1_53_clean :
	del bcb\libboost_regex-bcb-d-1_53\*.obj
	del bcb\libboost_regex-bcb-d-1_53\*.il?
	del bcb\libboost_regex-bcb-d-1_53\*.csm
	del bcb\libboost_regex-bcb-d-1_53\*.tds

bcb\libboost_regex-bcb-d-1_53.lib : bcb\libboost_regex-bcb-d-1_53\c_regex_traits.obj bcb\libboost_regex-bcb-d-1_53\cpp_regex_traits.obj bcb\libboost_regex-bcb-d-1_53\cregex.obj bcb\libboost_regex-bcb-d-1_53\fileiter.obj bcb\libboost_regex-bcb-d-1_53\icu.obj bcb\libboost_regex-bcb-d-1_53\instances.obj bcb\libboost_regex-bcb-d-1_53\posix_api.obj bcb\libboost_regex-bcb-d-1_53\regex.obj bcb\libboost_regex-bcb-d-1_53\regex_debug.obj bcb\libboost_regex-bcb-d-1_53\regex_raw_buffer.obj bcb\libboost_regex-bcb-d-1_53\regex_traits_defaults.obj bcb\libboost_regex-bcb-d-1_53\static_mutex.obj bcb\libboost_regex-bcb-d-1_53\usinstances.obj bcb\libboost_regex-bcb-d-1_53\w32_regex_traits.obj bcb\libboost_regex-bcb-d-1_53\wc_regex_traits.obj bcb\libboost_regex-bcb-d-1_53\wide_posix_api.obj bcb\libboost_regex-bcb-d-1_53\winstances.obj
	if exist bcb\libboost_regex-bcb-d-1_53.lib del bcb\libboost_regex-bcb-d-1_53.lib 
	tlib @&&|
/P128 /C /u /a $(XSFLAGS) "bcb\libboost_regex-bcb-d-1_53.lib"  +"bcb\libboost_regex-bcb-d-1_53\c_regex_traits.obj" +"bcb\libboost_regex-bcb-d-1_53\cpp_regex_traits.obj" +"bcb\libboost_regex-bcb-d-1_53\cregex.obj" +"bcb\libboost_regex-bcb-d-1_53\fileiter.obj" +"bcb\libboost_regex-bcb-d-1_53\icu.obj" +"bcb\libboost_regex-bcb-d-1_53\instances.obj" +"bcb\libboost_regex-bcb-d-1_53\posix_api.obj" +"bcb\libboost_regex-bcb-d-1_53\regex.obj" +"bcb\libboost_regex-bcb-d-1_53\regex_debug.obj" +"bcb\libboost_regex-bcb-d-1_53\regex_raw_buffer.obj" +"bcb\libboost_regex-bcb-d-1_53\regex_traits_defaults.obj" +"bcb\libboost_regex-bcb-d-1_53\static_mutex.obj" +"bcb\libboost_regex-bcb-d-1_53\usinstances.obj" +"bcb\libboost_regex-bcb-d-1_53\w32_regex_traits.obj" +"bcb\libboost_regex-bcb-d-1_53\wc_regex_traits.obj" +"bcb\libboost_regex-bcb-d-1_53\wide_posix_api.obj" +"bcb\libboost_regex-bcb-d-1_53\winstances.obj"
|

