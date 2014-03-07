# copyright John Maddock 2003
# Distributed under the Boost Software License, Version 1.0. 
# (See accompanying file LICENSE_1_0.txt or copy at 
# http://www.boost.org/LICENSE_1_0.txt.

# very basic makefile for regression tests
#
# g++ 2.95 and greater
#
CXX= g++ $(INCLUDES) -L../../../../stage/lib -I../../../../ -I./ $(CXXFLAGS) -L../../build/gcc $(LDFLAGS)
#
# sources to compile for each test:
#
SOURCES=*.cpp 

total : gcc_regress
	export LD_LIBRARY_PATH="../../build/gcc:$LD_LIBRARY_PATH" && ./gcc_regress tests.txt

gcc_regress : $(SOURCES)
	$(CXX) -O2 -o gcc_regress $(SOURCES) ../../build/gcc/libboost_regex-gcc*.a $(LIBS)

debug : $(SOURCES)
	$(CXX) -g -o gcc_regress $(SOURCES) ../../build/gcc/libboost_regex-gcc-d*.a $(LIBS)



















































