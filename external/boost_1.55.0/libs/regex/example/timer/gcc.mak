# copyright John Maddock 2003
# Distributed under the Boost Software License, Version 1.0. 
# (See accompanying file LICENSE_1_0.txt or copy at 
# http://www.boost.org/LICENSE_1_0.txt.

# very basic makefile for timer.exe
#
# GNU compiler GCC
#
CXX= $(INCLUDES) -I../../../../ -I./ $(CXXFLAGS)

timer : regex_timer.cpp
	g++ $(CXX) -O2 -o timer regex_timer.cpp -L../../build/gcc $(LDFLAGS) -lboost_regex $(LIBS)

debug : regex_timer.cpp timer.cpp 
	g++ $(CXX) -g -o timer regex_timer.cpp -L../../build/gcc $(LDFLAGS) -lboost_regex_debug $(LIBS)






















