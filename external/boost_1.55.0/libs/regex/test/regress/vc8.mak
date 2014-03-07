# copyright John Maddock 2003
# Distributed under the Boost Software License, Version 1.0. 
# (See accompanying file LICENSE_1_0.txt or copy at 
# http://www.boost.org/LICENSE_1_0.txt.

# very basic makefile for regression tests
#
# Visual C++ 8.0
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

CFLAGS= $(INCLUDES) /Zm400 /GB /GF /Gy -GX -GR -I..\..\..\..\ $(CXXFLAGS) /DBOOST_LIB_DIAGNOSTIC=1 /Zc:wchar_t

LFLAGS= -link /LIBPATH:..\..\..\..\stage\lib /LIBPATH:..\..\build\vc80 $(XLFLAGS)

all :: r1-vc8.exe r2-vc8.exe r3-vc8.exe r4-vc8.exe r5-vc8.exe r6-vc8.exe r7-vc8.exe r8-vc8.exe
	r1-vc8
	r2-vc8
	r3-vc8
	r4-vc8
	r5-vc8
	r6-vc8
	-copy ..\..\build\vc80\boost_regex*.dll
	-copy ..\..\..\..\stage\lib\boost_regex*.dll
	r7-vc8
	r8-vc8

r1-vc8.exe : 
	cl /ML $(CFLAGS) /O2 -o r1-vc8.exe $(SOURCES) $(LFLAGS)

r2-vc8.exe : 
	cl /MLd $(CFLAGS) -o r2-vc8.exe $(SOURCES) $(LFLAGS)

r3-vc8.exe : 
	cl /MT $(CFLAGS) /O2 -o r3-vc8.exe $(SOURCES) $(LFLAGS)

r4-vc8.exe : 
	cl /MTd $(CFLAGS) -o r4-vc8.exe $(SOURCES) $(LFLAGS)

r5-vc8.exe : 
	cl /MD $(CFLAGS) /O2 -o r5-vc8.exe $(SOURCES) $(LFLAGS)

r6-vc8.exe : 
	cl /MDd $(CFLAGS) -o r6-vc8.exe $(SOURCES) $(LFLAGS)

r7-vc8.exe : 
	cl /MD $(CFLAGS) /O2 /DBOOST_ALL_DYN_LINK -o r7-vc8.exe $(SOURCES) $(LFLAGS)

r8-vc8.exe : 
	cl /MDd $(CFLAGS) /DBOOST_ALL_DYN_LINK -o r8-vc8.exe $(SOURCES) $(LFLAGS)



