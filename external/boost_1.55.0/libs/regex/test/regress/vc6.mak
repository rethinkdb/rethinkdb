# copyright John Maddock 2003
# Distributed under the Boost Software License, Version 1.0. 
# (See accompanying file LICENSE_1_0.txt or copy at 
# http://www.boost.org/LICENSE_1_0.txt.

# very basic makefile for regression tests
#
# Visual C++ 6
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
# sources to compile for each test:
#
SOURCES=*.cpp 

CFLAGS= $(INCLUDES) /Zm400 /GF /Gy -GX -GR -I..\..\..\..\  /DBOOST_LIB_DIAGNOSTIC=1 $(CXXFLAGS)

LFLAGS= -link /LIBPATH:..\..\..\..\stage\lib /LIBPATH:..\..\build\vc6 user32.lib $(XLFLAGS)

all :: r1-vc6.exe r2-vc6.exe r3-vc6.exe r4-vc6.exe r5-vc6.exe r6-vc6.exe r7-vc6.exe r8-vc6.exe
	r1-vc6
	r2-vc6
	r3-vc6
	r4-vc6
	r5-vc6
	r6-vc6
	-copy ..\..\build\vc6\boost_regex*.dll
	-copy ..\..\..\..\stage\lib\boost_regex*.dll
	r7-vc6
	r8-vc6

r1-vc6.exe : 
	cl /ML $(CFLAGS) /O2 -o r1-vc6.exe $(SOURCES) $(LFLAGS)

r2-vc6.exe : 
	cl /MLd $(CFLAGS) -o r2-vc6.exe $(SOURCES) $(LFLAGS)

r3-vc6.exe : 
	cl /MT $(CFLAGS) /O2 -o r3-vc6.exe $(SOURCES) $(LFLAGS)

r4-vc6.exe : 
	cl /MTd $(CFLAGS) -o r4-vc6.exe $(SOURCES) $(LFLAGS)

r5-vc6.exe : 
	cl /MD $(CFLAGS) /O2 -o r5-vc6.exe $(SOURCES) $(LFLAGS)

r6-vc6.exe : 
	cl /MDd $(CFLAGS) -o r6-vc6.exe $(SOURCES) $(LFLAGS)

r7-vc6.exe : 
	cl /MD $(CFLAGS) /O2 /DBOOST_ALL_DYN_LINK -o r7-vc6.exe $(SOURCES) $(LFLAGS)

r8-vc6.exe : 
	cl /MDd $(CFLAGS) /DBOOST_ALL_DYN_LINK -o r8-vc6.exe $(SOURCES) $(LFLAGS)



