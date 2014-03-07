# copyright John Maddock 2003
# Distributed under the Boost Software License, Version 1.0. 
# (See accompanying file LICENSE_1_0.txt or copy at 
# http://www.boost.org/LICENSE_1_0.txt.
#
# very basic VC6 makefile for timer
#
CXX=cl
CXXFLAGS=/Oityb1 /GF /Gy -GX -DSTRICT -I../../../../ -I./ 
LIBS=/link /LIBPATH:..\..\build\vc6
EXE=.exe
OBJ=.obj

LIBDEP= ../../../../boost/regex/detail/regex_options.hpp ../../../../boost/regex/detail/regex_config.hpp

regex_timer$(EXE) : regex_timer$(OBJ)
	$(CXX) -o timer$(EXE) regex_timer$(OBJ) $(LIBS)

regex_timer$(OBJ) : regex_timer.cpp $(LIBDEP)
	$(CXX) -c $(CXXFLAGS) regex_timer.cpp

timer$(OBJ) : ../../../timer/timer.cpp $(LIBDEP)
	$(CXX) -c $(CXXFLAGS) ../../../timer/timer.cpp
















