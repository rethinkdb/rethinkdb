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
LINKER=$(CXX) -shared

#
# compiler options for release build:
#
C1=-c -O2 -I../../.. -fPIC
#
# compiler options for debug build:
#
C2=-c -g -I../../.. -fPIC

ifeq "$(ICU_PATH)" ""
$(warning "Building Boost.Regex without ICU / Unicode support:")
$(warning "Hint: set ICU_PATH on the nmake command line to point ")
$(warning "to your ICU installation if you have one.")
else
ICU_CXXFLAGS= -DBOOST_HAS_ICU=1 -I$(ICU_PATH)/include
ICU_LDFLAGS= -L$(ICU_PATH)/lib
ICU_LIBS= -licui18n -licuuc
$(warning "Building Boost.Regex with ICU in $(ICU_PATH)")
endif


ALL_HEADER= ../../../boost/regex/config.hpp ../../../boost/regex/icu.hpp ../../../boost/regex/pattern_except.hpp ../../../boost/regex/regex_traits.hpp ../../../boost/regex/user.hpp ../../../boost/regex/v4/basic_regex.hpp ../../../boost/regex/v4/basic_regex_creator.hpp ../../../boost/regex/v4/basic_regex_parser.hpp ../../../boost/regex/v4/c_regex_traits.hpp ../../../boost/regex/v4/char_regex_traits.hpp ../../../boost/regex/v4/cpp_regex_traits.hpp ../../../boost/regex/v4/cregex.hpp ../../../boost/regex/v4/error_type.hpp ../../../boost/regex/v4/fileiter.hpp ../../../boost/regex/v4/instances.hpp ../../../boost/regex/v4/iterator_category.hpp ../../../boost/regex/v4/iterator_traits.hpp ../../../boost/regex/v4/match_flags.hpp ../../../boost/regex/v4/match_results.hpp ../../../boost/regex/v4/mem_block_cache.hpp ../../../boost/regex/v4/perl_matcher.hpp ../../../boost/regex/v4/perl_matcher_common.hpp ../../../boost/regex/v4/perl_matcher_non_recursive.hpp ../../../boost/regex/v4/perl_matcher_recursive.hpp ../../../boost/regex/v4/primary_transform.hpp ../../../boost/regex/v4/protected_call.hpp ../../../boost/regex/v4/regbase.hpp ../../../boost/regex/v4/regex.hpp ../../../boost/regex/v4/regex_format.hpp ../../../boost/regex/v4/regex_fwd.hpp ../../../boost/regex/v4/regex_grep.hpp ../../../boost/regex/v4/regex_iterator.hpp ../../../boost/regex/v4/regex_match.hpp ../../../boost/regex/v4/regex_merge.hpp ../../../boost/regex/v4/regex_raw_buffer.hpp ../../../boost/regex/v4/regex_replace.hpp ../../../boost/regex/v4/regex_search.hpp ../../../boost/regex/v4/regex_split.hpp ../../../boost/regex/v4/regex_token_iterator.hpp ../../../boost/regex/v4/regex_traits.hpp ../../../boost/regex/v4/regex_traits_defaults.hpp ../../../boost/regex/v4/regex_workaround.hpp ../../../boost/regex/v4/states.hpp ../../../boost/regex/v4/sub_match.hpp ../../../boost/regex/v4/syntax_type.hpp ../../../boost/regex/v4/u32regex_iterator.hpp ../../../boost/regex/v4/u32regex_token_iterator.hpp ../../../boost/regex/v4/w32_regex_traits.hpp ../../../boost/regex/config/borland.hpp ../../../boost/regex/config/cwchar.hpp

all : gcc  gcc gcc/boost_regex-gcc-1_53_shared ./gcc/libboost_regex-gcc-1_53.so gcc gcc/boost_regex-gcc-d-1_53_shared ./gcc/libboost_regex-gcc-d-1_53.so

gcc :
	mkdir -p gcc

clean :  boost_regex-gcc-1_53_clean boost_regex-gcc-d-1_53_clean

install : all



########################################################
#
# section for libboost_regex-gcc-1_53.a
#
########################################################
gcc/boost_regex-gcc-1_53_shared/c_regex_traits.o: ../src/c_regex_traits.cpp $(ALL_HEADER)
	$(CXX) $(INCLUDES) -o gcc/boost_regex-gcc-1_53_shared/c_regex_traits.o $(C1) $(CXXFLAGS) $(ICU_CXXFLAGS) ../src/c_regex_traits.cpp

gcc/boost_regex-gcc-1_53_shared/cpp_regex_traits.o: ../src/cpp_regex_traits.cpp $(ALL_HEADER)
	$(CXX) $(INCLUDES) -o gcc/boost_regex-gcc-1_53_shared/cpp_regex_traits.o $(C1) $(CXXFLAGS) $(ICU_CXXFLAGS) ../src/cpp_regex_traits.cpp

gcc/boost_regex-gcc-1_53_shared/cregex.o: ../src/cregex.cpp $(ALL_HEADER)
	$(CXX) $(INCLUDES) -o gcc/boost_regex-gcc-1_53_shared/cregex.o $(C1) $(CXXFLAGS) $(ICU_CXXFLAGS) ../src/cregex.cpp

gcc/boost_regex-gcc-1_53_shared/fileiter.o: ../src/fileiter.cpp $(ALL_HEADER)
	$(CXX) $(INCLUDES) -o gcc/boost_regex-gcc-1_53_shared/fileiter.o $(C1) $(CXXFLAGS) $(ICU_CXXFLAGS) ../src/fileiter.cpp

gcc/boost_regex-gcc-1_53_shared/icu.o: ../src/icu.cpp $(ALL_HEADER)
	$(CXX) $(INCLUDES) -o gcc/boost_regex-gcc-1_53_shared/icu.o $(C1) $(CXXFLAGS) $(ICU_CXXFLAGS) ../src/icu.cpp

gcc/boost_regex-gcc-1_53_shared/instances.o: ../src/instances.cpp $(ALL_HEADER)
	$(CXX) $(INCLUDES) -o gcc/boost_regex-gcc-1_53_shared/instances.o $(C1) $(CXXFLAGS) $(ICU_CXXFLAGS) ../src/instances.cpp

gcc/boost_regex-gcc-1_53_shared/posix_api.o: ../src/posix_api.cpp $(ALL_HEADER)
	$(CXX) $(INCLUDES) -o gcc/boost_regex-gcc-1_53_shared/posix_api.o $(C1) $(CXXFLAGS) $(ICU_CXXFLAGS) ../src/posix_api.cpp

gcc/boost_regex-gcc-1_53_shared/regex.o: ../src/regex.cpp $(ALL_HEADER)
	$(CXX) $(INCLUDES) -o gcc/boost_regex-gcc-1_53_shared/regex.o $(C1) $(CXXFLAGS) $(ICU_CXXFLAGS) ../src/regex.cpp

gcc/boost_regex-gcc-1_53_shared/regex_debug.o: ../src/regex_debug.cpp $(ALL_HEADER)
	$(CXX) $(INCLUDES) -o gcc/boost_regex-gcc-1_53_shared/regex_debug.o $(C1) $(CXXFLAGS) $(ICU_CXXFLAGS) ../src/regex_debug.cpp

gcc/boost_regex-gcc-1_53_shared/regex_raw_buffer.o: ../src/regex_raw_buffer.cpp $(ALL_HEADER)
	$(CXX) $(INCLUDES) -o gcc/boost_regex-gcc-1_53_shared/regex_raw_buffer.o $(C1) $(CXXFLAGS) $(ICU_CXXFLAGS) ../src/regex_raw_buffer.cpp

gcc/boost_regex-gcc-1_53_shared/regex_traits_defaults.o: ../src/regex_traits_defaults.cpp $(ALL_HEADER)
	$(CXX) $(INCLUDES) -o gcc/boost_regex-gcc-1_53_shared/regex_traits_defaults.o $(C1) $(CXXFLAGS) $(ICU_CXXFLAGS) ../src/regex_traits_defaults.cpp

gcc/boost_regex-gcc-1_53_shared/static_mutex.o: ../src/static_mutex.cpp $(ALL_HEADER)
	$(CXX) $(INCLUDES) -o gcc/boost_regex-gcc-1_53_shared/static_mutex.o $(C1) $(CXXFLAGS) $(ICU_CXXFLAGS) ../src/static_mutex.cpp

gcc/boost_regex-gcc-1_53_shared/usinstances.o: ../src/usinstances.cpp $(ALL_HEADER)
	$(CXX) $(INCLUDES) -o gcc/boost_regex-gcc-1_53_shared/usinstances.o $(C1) $(CXXFLAGS) $(ICU_CXXFLAGS) ../src/usinstances.cpp

gcc/boost_regex-gcc-1_53_shared/w32_regex_traits.o: ../src/w32_regex_traits.cpp $(ALL_HEADER)
	$(CXX) $(INCLUDES) -o gcc/boost_regex-gcc-1_53_shared/w32_regex_traits.o $(C1) $(CXXFLAGS) $(ICU_CXXFLAGS) ../src/w32_regex_traits.cpp

gcc/boost_regex-gcc-1_53_shared/wc_regex_traits.o: ../src/wc_regex_traits.cpp $(ALL_HEADER)
	$(CXX) $(INCLUDES) -o gcc/boost_regex-gcc-1_53_shared/wc_regex_traits.o $(C1) $(CXXFLAGS) $(ICU_CXXFLAGS) ../src/wc_regex_traits.cpp

gcc/boost_regex-gcc-1_53_shared/wide_posix_api.o: ../src/wide_posix_api.cpp $(ALL_HEADER)
	$(CXX) $(INCLUDES) -o gcc/boost_regex-gcc-1_53_shared/wide_posix_api.o $(C1) $(CXXFLAGS) $(ICU_CXXFLAGS) ../src/wide_posix_api.cpp

gcc/boost_regex-gcc-1_53_shared/winstances.o: ../src/winstances.cpp $(ALL_HEADER)
	$(CXX) $(INCLUDES) -o gcc/boost_regex-gcc-1_53_shared/winstances.o $(C1) $(CXXFLAGS) $(ICU_CXXFLAGS) ../src/winstances.cpp

gcc/boost_regex-gcc-1_53_shared : 
	mkdir -p gcc/boost_regex-gcc-1_53_shared

boost_regex-gcc-1_53_clean :
	rm -f gcc/boost_regex-gcc-1_53_shared/*.o

./gcc/libboost_regex-gcc-1_53.so : gcc/boost_regex-gcc-1_53_shared/c_regex_traits.o gcc/boost_regex-gcc-1_53_shared/cpp_regex_traits.o gcc/boost_regex-gcc-1_53_shared/cregex.o gcc/boost_regex-gcc-1_53_shared/fileiter.o gcc/boost_regex-gcc-1_53_shared/icu.o gcc/boost_regex-gcc-1_53_shared/instances.o gcc/boost_regex-gcc-1_53_shared/posix_api.o gcc/boost_regex-gcc-1_53_shared/regex.o gcc/boost_regex-gcc-1_53_shared/regex_debug.o gcc/boost_regex-gcc-1_53_shared/regex_raw_buffer.o gcc/boost_regex-gcc-1_53_shared/regex_traits_defaults.o gcc/boost_regex-gcc-1_53_shared/static_mutex.o gcc/boost_regex-gcc-1_53_shared/usinstances.o gcc/boost_regex-gcc-1_53_shared/w32_regex_traits.o gcc/boost_regex-gcc-1_53_shared/wc_regex_traits.o gcc/boost_regex-gcc-1_53_shared/wide_posix_api.o gcc/boost_regex-gcc-1_53_shared/winstances.o
	$(LINKER) -o gcc/libboost_regex-gcc-1_53.so $(LDFLAGS)  $(ICU_LDFLAGS)  gcc/boost_regex-gcc-1_53_shared/c_regex_traits.o gcc/boost_regex-gcc-1_53_shared/cpp_regex_traits.o gcc/boost_regex-gcc-1_53_shared/cregex.o gcc/boost_regex-gcc-1_53_shared/fileiter.o gcc/boost_regex-gcc-1_53_shared/icu.o gcc/boost_regex-gcc-1_53_shared/instances.o gcc/boost_regex-gcc-1_53_shared/posix_api.o gcc/boost_regex-gcc-1_53_shared/regex.o gcc/boost_regex-gcc-1_53_shared/regex_debug.o gcc/boost_regex-gcc-1_53_shared/regex_raw_buffer.o gcc/boost_regex-gcc-1_53_shared/regex_traits_defaults.o gcc/boost_regex-gcc-1_53_shared/static_mutex.o gcc/boost_regex-gcc-1_53_shared/usinstances.o gcc/boost_regex-gcc-1_53_shared/w32_regex_traits.o gcc/boost_regex-gcc-1_53_shared/wc_regex_traits.o gcc/boost_regex-gcc-1_53_shared/wide_posix_api.o gcc/boost_regex-gcc-1_53_shared/winstances.o $(ICU_LIBS) $(LIBS)

########################################################
#
# section for libboost_regex-gcc-d-1_53.a
#
########################################################
gcc/boost_regex-gcc-d-1_53_shared/c_regex_traits.o: ../src/c_regex_traits.cpp $(ALL_HEADER)
	$(CXX) $(INCLUDES) -o gcc/boost_regex-gcc-d-1_53_shared/c_regex_traits.o $(C2) $(CXXFLAGS) $(ICU_CXXFLAGS) ../src/c_regex_traits.cpp

gcc/boost_regex-gcc-d-1_53_shared/cpp_regex_traits.o: ../src/cpp_regex_traits.cpp $(ALL_HEADER)
	$(CXX) $(INCLUDES) -o gcc/boost_regex-gcc-d-1_53_shared/cpp_regex_traits.o $(C2) $(CXXFLAGS) $(ICU_CXXFLAGS) ../src/cpp_regex_traits.cpp

gcc/boost_regex-gcc-d-1_53_shared/cregex.o: ../src/cregex.cpp $(ALL_HEADER)
	$(CXX) $(INCLUDES) -o gcc/boost_regex-gcc-d-1_53_shared/cregex.o $(C2) $(CXXFLAGS) $(ICU_CXXFLAGS) ../src/cregex.cpp

gcc/boost_regex-gcc-d-1_53_shared/fileiter.o: ../src/fileiter.cpp $(ALL_HEADER)
	$(CXX) $(INCLUDES) -o gcc/boost_regex-gcc-d-1_53_shared/fileiter.o $(C2) $(CXXFLAGS) $(ICU_CXXFLAGS) ../src/fileiter.cpp

gcc/boost_regex-gcc-d-1_53_shared/icu.o: ../src/icu.cpp $(ALL_HEADER)
	$(CXX) $(INCLUDES) -o gcc/boost_regex-gcc-d-1_53_shared/icu.o $(C2) $(CXXFLAGS) $(ICU_CXXFLAGS) ../src/icu.cpp

gcc/boost_regex-gcc-d-1_53_shared/instances.o: ../src/instances.cpp $(ALL_HEADER)
	$(CXX) $(INCLUDES) -o gcc/boost_regex-gcc-d-1_53_shared/instances.o $(C2) $(CXXFLAGS) $(ICU_CXXFLAGS) ../src/instances.cpp

gcc/boost_regex-gcc-d-1_53_shared/posix_api.o: ../src/posix_api.cpp $(ALL_HEADER)
	$(CXX) $(INCLUDES) -o gcc/boost_regex-gcc-d-1_53_shared/posix_api.o $(C2) $(CXXFLAGS) $(ICU_CXXFLAGS) ../src/posix_api.cpp

gcc/boost_regex-gcc-d-1_53_shared/regex.o: ../src/regex.cpp $(ALL_HEADER)
	$(CXX) $(INCLUDES) -o gcc/boost_regex-gcc-d-1_53_shared/regex.o $(C2) $(CXXFLAGS) $(ICU_CXXFLAGS) ../src/regex.cpp

gcc/boost_regex-gcc-d-1_53_shared/regex_debug.o: ../src/regex_debug.cpp $(ALL_HEADER)
	$(CXX) $(INCLUDES) -o gcc/boost_regex-gcc-d-1_53_shared/regex_debug.o $(C2) $(CXXFLAGS) $(ICU_CXXFLAGS) ../src/regex_debug.cpp

gcc/boost_regex-gcc-d-1_53_shared/regex_raw_buffer.o: ../src/regex_raw_buffer.cpp $(ALL_HEADER)
	$(CXX) $(INCLUDES) -o gcc/boost_regex-gcc-d-1_53_shared/regex_raw_buffer.o $(C2) $(CXXFLAGS) $(ICU_CXXFLAGS) ../src/regex_raw_buffer.cpp

gcc/boost_regex-gcc-d-1_53_shared/regex_traits_defaults.o: ../src/regex_traits_defaults.cpp $(ALL_HEADER)
	$(CXX) $(INCLUDES) -o gcc/boost_regex-gcc-d-1_53_shared/regex_traits_defaults.o $(C2) $(CXXFLAGS) $(ICU_CXXFLAGS) ../src/regex_traits_defaults.cpp

gcc/boost_regex-gcc-d-1_53_shared/static_mutex.o: ../src/static_mutex.cpp $(ALL_HEADER)
	$(CXX) $(INCLUDES) -o gcc/boost_regex-gcc-d-1_53_shared/static_mutex.o $(C2) $(CXXFLAGS) $(ICU_CXXFLAGS) ../src/static_mutex.cpp

gcc/boost_regex-gcc-d-1_53_shared/usinstances.o: ../src/usinstances.cpp $(ALL_HEADER)
	$(CXX) $(INCLUDES) -o gcc/boost_regex-gcc-d-1_53_shared/usinstances.o $(C2) $(CXXFLAGS) $(ICU_CXXFLAGS) ../src/usinstances.cpp

gcc/boost_regex-gcc-d-1_53_shared/w32_regex_traits.o: ../src/w32_regex_traits.cpp $(ALL_HEADER)
	$(CXX) $(INCLUDES) -o gcc/boost_regex-gcc-d-1_53_shared/w32_regex_traits.o $(C2) $(CXXFLAGS) $(ICU_CXXFLAGS) ../src/w32_regex_traits.cpp

gcc/boost_regex-gcc-d-1_53_shared/wc_regex_traits.o: ../src/wc_regex_traits.cpp $(ALL_HEADER)
	$(CXX) $(INCLUDES) -o gcc/boost_regex-gcc-d-1_53_shared/wc_regex_traits.o $(C2) $(CXXFLAGS) $(ICU_CXXFLAGS) ../src/wc_regex_traits.cpp

gcc/boost_regex-gcc-d-1_53_shared/wide_posix_api.o: ../src/wide_posix_api.cpp $(ALL_HEADER)
	$(CXX) $(INCLUDES) -o gcc/boost_regex-gcc-d-1_53_shared/wide_posix_api.o $(C2) $(CXXFLAGS) $(ICU_CXXFLAGS) ../src/wide_posix_api.cpp

gcc/boost_regex-gcc-d-1_53_shared/winstances.o: ../src/winstances.cpp $(ALL_HEADER)
	$(CXX) $(INCLUDES) -o gcc/boost_regex-gcc-d-1_53_shared/winstances.o $(C2) $(CXXFLAGS) $(ICU_CXXFLAGS) ../src/winstances.cpp

gcc/boost_regex-gcc-d-1_53_shared : 
	mkdir -p gcc/boost_regex-gcc-d-1_53_shared

boost_regex-gcc-d-1_53_clean :
	rm -f gcc/boost_regex-gcc-d-1_53_shared/*.o

./gcc/libboost_regex-gcc-d-1_53.so : gcc/boost_regex-gcc-d-1_53_shared/c_regex_traits.o gcc/boost_regex-gcc-d-1_53_shared/cpp_regex_traits.o gcc/boost_regex-gcc-d-1_53_shared/cregex.o gcc/boost_regex-gcc-d-1_53_shared/fileiter.o gcc/boost_regex-gcc-d-1_53_shared/icu.o gcc/boost_regex-gcc-d-1_53_shared/instances.o gcc/boost_regex-gcc-d-1_53_shared/posix_api.o gcc/boost_regex-gcc-d-1_53_shared/regex.o gcc/boost_regex-gcc-d-1_53_shared/regex_debug.o gcc/boost_regex-gcc-d-1_53_shared/regex_raw_buffer.o gcc/boost_regex-gcc-d-1_53_shared/regex_traits_defaults.o gcc/boost_regex-gcc-d-1_53_shared/static_mutex.o gcc/boost_regex-gcc-d-1_53_shared/usinstances.o gcc/boost_regex-gcc-d-1_53_shared/w32_regex_traits.o gcc/boost_regex-gcc-d-1_53_shared/wc_regex_traits.o gcc/boost_regex-gcc-d-1_53_shared/wide_posix_api.o gcc/boost_regex-gcc-d-1_53_shared/winstances.o
	$(LINKER) -o gcc/libboost_regex-gcc-d-1_53.so $(LDFLAGS)  $(ICU_LDFLAGS)  gcc/boost_regex-gcc-d-1_53_shared/c_regex_traits.o gcc/boost_regex-gcc-d-1_53_shared/cpp_regex_traits.o gcc/boost_regex-gcc-d-1_53_shared/cregex.o gcc/boost_regex-gcc-d-1_53_shared/fileiter.o gcc/boost_regex-gcc-d-1_53_shared/icu.o gcc/boost_regex-gcc-d-1_53_shared/instances.o gcc/boost_regex-gcc-d-1_53_shared/posix_api.o gcc/boost_regex-gcc-d-1_53_shared/regex.o gcc/boost_regex-gcc-d-1_53_shared/regex_debug.o gcc/boost_regex-gcc-d-1_53_shared/regex_raw_buffer.o gcc/boost_regex-gcc-d-1_53_shared/regex_traits_defaults.o gcc/boost_regex-gcc-d-1_53_shared/static_mutex.o gcc/boost_regex-gcc-d-1_53_shared/usinstances.o gcc/boost_regex-gcc-d-1_53_shared/w32_regex_traits.o gcc/boost_regex-gcc-d-1_53_shared/wc_regex_traits.o gcc/boost_regex-gcc-d-1_53_shared/wide_posix_api.o gcc/boost_regex-gcc-d-1_53_shared/winstances.o $(ICU_LIBS) $(LIBS)

