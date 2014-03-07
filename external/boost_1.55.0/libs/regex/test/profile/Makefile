# copyright John Maddock 2003
# Distributed under the Boost Software License, Version 1.0. 
# (See accompanying file LICENSE_1_0.txt or copy at 
# http://www.boost.org/LICENSE_1_0.txt.


regress :
	g++ -fprofile-arcs -ftest-coverage -DBOOST_REGEX_RECURSIVE -DBOOST_REGEX_BLOCKSIZE=512 -DBOOST_REGEX_MAX_CACHE_BLOCKS=0 -DBOOST_REGEX_MATCH_EXTRA -g -I../../../../ -o regress ../regress/*.cpp ../../src/*.cpp ../../../test/src/ex*.cpp ../../../test/src/cpp_main.cpp
	./regress
	gcov basic_tests.cpp






