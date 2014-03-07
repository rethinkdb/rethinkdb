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



ALL_HEADER= ../../../boost/regex/config.hpp ../../../boost/regex/icu.hpp ../../../boost/regex/pattern_except.hpp ../../../boost/regex/regex_traits.hpp ../../../boost/regex/user.hpp ../../../boost/regex/v4/basic_regex.hpp ../../../boost/regex/v4/basic_regex_creator.hpp ../../../boost/regex/v4/basic_regex_parser.hpp ../../../boost/regex/v4/c_regex_traits.hpp ../../../boost/regex/v4/char_regex_traits.hpp ../../../boost/regex/v4/cpp_regex_traits.hpp ../../../boost/regex/v4/cregex.hpp ../../../boost/regex/v4/error_type.hpp ../../../boost/regex/v4/fileiter.hpp ../../../boost/regex/v4/instances.hpp ../../../boost/regex/v4/iterator_category.hpp ../../../boost/regex/v4/iterator_traits.hpp ../../../boost/regex/v4/match_flags.hpp ../../../boost/regex/v4/match_results.hpp ../../../boost/regex/v4/mem_block_cache.hpp ../../../boost/regex/v4/perl_matcher.hpp ../../../boost/regex/v4/perl_matcher_common.hpp ../../../boost/regex/v4/perl_matcher_non_recursive.hpp ../../../boost/regex/v4/perl_matcher_recursive.hpp ../../../boost/regex/v4/primary_transform.hpp ../../../boost/regex/v4/protected_call.hpp ../../../boost/regex/v4/regbase.hpp ../../../boost/regex/v4/regex.hpp ../../../boost/regex/v4/regex_format.hpp ../../../boost/regex/v4/regex_fwd.hpp ../../../boost/regex/v4/regex_grep.hpp ../../../boost/regex/v4/regex_iterator.hpp ../../../boost/regex/v4/regex_match.hpp ../../../boost/regex/v4/regex_merge.hpp ../../../boost/regex/v4/regex_raw_buffer.hpp ../../../boost/regex/v4/regex_replace.hpp ../../../boost/regex/v4/regex_search.hpp ../../../boost/regex/v4/regex_split.hpp ../../../boost/regex/v4/regex_token_iterator.hpp ../../../boost/regex/v4/regex_traits.hpp ../../../boost/regex/v4/regex_traits_defaults.hpp ../../../boost/regex/v4/regex_workaround.hpp ../../../boost/regex/v4/states.hpp ../../../boost/regex/v4/sub_match.hpp ../../../boost/regex/v4/syntax_type.hpp ../../../boost/regex/v4/u32regex_iterator.hpp ../../../boost/regex/v4/u32regex_token_iterator.hpp ../../../boost/regex/v4/w32_regex_traits.hpp ../../../boost/regex/config/borland.hpp ../../../boost/regex/config/cwchar.hpp

all : $(DIRNAME)  $(DIRNAME) $(DIRNAME)/boost_regex ./$(DIRNAME)/libboost_regex.so

$(DIRNAME) :
	mkdir -p $(DIRNAME)

clean :  boost_regex_clean

install : all



########################################################
#
# section for libboost_regex.so
#
########################################################
$(DIRNAME)/boost_regex/c_regex_traits.o: ../src/c_regex_traits.cpp $(ALL_HEADER)
	$(CXX) $(INCLUDES) -o $(DIRNAME)/boost_regex/c_regex_traits.o $(C1) $(CXXFLAGS) ../src/c_regex_traits.cpp

$(DIRNAME)/boost_regex/cpp_regex_traits.o: ../src/cpp_regex_traits.cpp $(ALL_HEADER)
	$(CXX) $(INCLUDES) -o $(DIRNAME)/boost_regex/cpp_regex_traits.o $(C1) $(CXXFLAGS) ../src/cpp_regex_traits.cpp

$(DIRNAME)/boost_regex/cregex.o: ../src/cregex.cpp $(ALL_HEADER)
	$(CXX) $(INCLUDES) -o $(DIRNAME)/boost_regex/cregex.o $(C1) $(CXXFLAGS) ../src/cregex.cpp

$(DIRNAME)/boost_regex/fileiter.o: ../src/fileiter.cpp $(ALL_HEADER)
	$(CXX) $(INCLUDES) -o $(DIRNAME)/boost_regex/fileiter.o $(C1) $(CXXFLAGS) ../src/fileiter.cpp

$(DIRNAME)/boost_regex/icu.o: ../src/icu.cpp $(ALL_HEADER)
	$(CXX) $(INCLUDES) -o $(DIRNAME)/boost_regex/icu.o $(C1) $(CXXFLAGS) ../src/icu.cpp

$(DIRNAME)/boost_regex/instances.o: ../src/instances.cpp $(ALL_HEADER)
	$(CXX) $(INCLUDES) -o $(DIRNAME)/boost_regex/instances.o $(C1) $(CXXFLAGS) ../src/instances.cpp

$(DIRNAME)/boost_regex/posix_api.o: ../src/posix_api.cpp $(ALL_HEADER)
	$(CXX) $(INCLUDES) -o $(DIRNAME)/boost_regex/posix_api.o $(C1) $(CXXFLAGS) ../src/posix_api.cpp

$(DIRNAME)/boost_regex/regex.o: ../src/regex.cpp $(ALL_HEADER)
	$(CXX) $(INCLUDES) -o $(DIRNAME)/boost_regex/regex.o $(C1) $(CXXFLAGS) ../src/regex.cpp

$(DIRNAME)/boost_regex/regex_debug.o: ../src/regex_debug.cpp $(ALL_HEADER)
	$(CXX) $(INCLUDES) -o $(DIRNAME)/boost_regex/regex_debug.o $(C1) $(CXXFLAGS) ../src/regex_debug.cpp

$(DIRNAME)/boost_regex/regex_raw_buffer.o: ../src/regex_raw_buffer.cpp $(ALL_HEADER)
	$(CXX) $(INCLUDES) -o $(DIRNAME)/boost_regex/regex_raw_buffer.o $(C1) $(CXXFLAGS) ../src/regex_raw_buffer.cpp

$(DIRNAME)/boost_regex/regex_traits_defaults.o: ../src/regex_traits_defaults.cpp $(ALL_HEADER)
	$(CXX) $(INCLUDES) -o $(DIRNAME)/boost_regex/regex_traits_defaults.o $(C1) $(CXXFLAGS) ../src/regex_traits_defaults.cpp

$(DIRNAME)/boost_regex/static_mutex.o: ../src/static_mutex.cpp $(ALL_HEADER)
	$(CXX) $(INCLUDES) -o $(DIRNAME)/boost_regex/static_mutex.o $(C1) $(CXXFLAGS) ../src/static_mutex.cpp

$(DIRNAME)/boost_regex/usinstances.o: ../src/usinstances.cpp $(ALL_HEADER)
	$(CXX) $(INCLUDES) -o $(DIRNAME)/boost_regex/usinstances.o $(C1) $(CXXFLAGS) ../src/usinstances.cpp

$(DIRNAME)/boost_regex/w32_regex_traits.o: ../src/w32_regex_traits.cpp $(ALL_HEADER)
	$(CXX) $(INCLUDES) -o $(DIRNAME)/boost_regex/w32_regex_traits.o $(C1) $(CXXFLAGS) ../src/w32_regex_traits.cpp

$(DIRNAME)/boost_regex/wc_regex_traits.o: ../src/wc_regex_traits.cpp $(ALL_HEADER)
	$(CXX) $(INCLUDES) -o $(DIRNAME)/boost_regex/wc_regex_traits.o $(C1) $(CXXFLAGS) ../src/wc_regex_traits.cpp

$(DIRNAME)/boost_regex/wide_posix_api.o: ../src/wide_posix_api.cpp $(ALL_HEADER)
	$(CXX) $(INCLUDES) -o $(DIRNAME)/boost_regex/wide_posix_api.o $(C1) $(CXXFLAGS) ../src/wide_posix_api.cpp

$(DIRNAME)/boost_regex/winstances.o: ../src/winstances.cpp $(ALL_HEADER)
	$(CXX) $(INCLUDES) -o $(DIRNAME)/boost_regex/winstances.o $(C1) $(CXXFLAGS) ../src/winstances.cpp

$(DIRNAME)/boost_regex : 
	mkdir -p $(DIRNAME)/boost_regex

boost_regex_clean :
	rm -f $(DIRNAME)/boost_regex/*.o

./$(DIRNAME)/libboost_regex.so : $(DIRNAME)/boost_regex/c_regex_traits.o $(DIRNAME)/boost_regex/cpp_regex_traits.o $(DIRNAME)/boost_regex/cregex.o $(DIRNAME)/boost_regex/fileiter.o $(DIRNAME)/boost_regex/icu.o $(DIRNAME)/boost_regex/instances.o $(DIRNAME)/boost_regex/posix_api.o $(DIRNAME)/boost_regex/regex.o $(DIRNAME)/boost_regex/regex_debug.o $(DIRNAME)/boost_regex/regex_raw_buffer.o $(DIRNAME)/boost_regex/regex_traits_defaults.o $(DIRNAME)/boost_regex/static_mutex.o $(DIRNAME)/boost_regex/usinstances.o $(DIRNAME)/boost_regex/w32_regex_traits.o $(DIRNAME)/boost_regex/wc_regex_traits.o $(DIRNAME)/boost_regex/wide_posix_api.o $(DIRNAME)/boost_regex/winstances.o
	$(LINKER) $(LDFLAGS) -o $(DIRNAME)/libboost_regex.so  $(DIRNAME)/boost_regex/c_regex_traits.o $(DIRNAME)/boost_regex/cpp_regex_traits.o $(DIRNAME)/boost_regex/cregex.o $(DIRNAME)/boost_regex/fileiter.o $(DIRNAME)/boost_regex/icu.o $(DIRNAME)/boost_regex/instances.o $(DIRNAME)/boost_regex/posix_api.o $(DIRNAME)/boost_regex/regex.o $(DIRNAME)/boost_regex/regex_debug.o $(DIRNAME)/boost_regex/regex_raw_buffer.o $(DIRNAME)/boost_regex/regex_traits_defaults.o $(DIRNAME)/boost_regex/static_mutex.o $(DIRNAME)/boost_regex/usinstances.o $(DIRNAME)/boost_regex/w32_regex_traits.o $(DIRNAME)/boost_regex/wc_regex_traits.o $(DIRNAME)/boost_regex/wide_posix_api.o $(DIRNAME)/boost_regex/winstances.o $(LIBS)

