# copyright John Maddock 2003
# Distributed under the Boost Software License, Version 1.0. 
# (See accompanying file LICENSE_1_0.txt or copy at 
# http://www.boost.org/LICENSE_1_0.txt.

# very basic makefile for regression tests
#
# Visual C++ 6 + full stlport 4.x
#
# we don't test single threaded builds as stlport doesn't support these...
#
#
# Add additional compiler options here:
#
CXXFLAGS=
#
# Add additional debugging options here:
#
CXXDEBUG=/D_STLP_DEBUG=1
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

!IF "$(MSVCDIR)" == ""
!ERROR Variable MSVCDIR not set.
!ENDIF

!IF "$(STLPORT_PATH)" == ""
!ERROR Variable STLPORT_PATH not set.
!ENDIF


CFLAGS= $(INCLUDES) /I$(STLPORT_PATH)\stlport /Zm400 /GF /Gy -GX -GR -I..\..\..\..\ $(CXXFLAGS) /DBOOST_LIB_DIAGNOSTIC=1

LFLAGS= -link /LIBPATH:..\..\..\..\stage\lib /LIBPATH:..\..\build\vc6-stlport /LIBPATH:$(STLPORT_PATH)\lib user32.lib $(XLFLAGS)

all :: r3-vc6-stlport.exe r4-vc6-stlport.exe r5-vc6-stlport.exe r6-vc6-stlport.exe r7-vc6-stlport.exe r8-vc6-stlport.exe
	r1-vc6-stlport
	r2-vc6-stlport
	r3-vc6-stlport
	r4-vc6-stlport
	r5-vc6-stlport
	r6-vc6-stlport
	-copy ..\..\build\vc6\boost_regex*.dll
	-copy ..\..\..\..\stage\lib\boost_regex*.dll
	r7-vc6-stlport
	r8-vc6-stlport

r3-vc6-stlport.exe : 
	cl /MT $(CFLAGS) /O2 -o r3-vc6-stlport.exe $(SOURCES) $(LFLAGS)

r4-vc6-stlport.exe : 
	cl /MTd $(CFLAGS) -o r4-vc6-stlport.exe $(SOURCES) $(LFLAGS)

r5-vc6-stlport.exe : 
	cl /MD $(CFLAGS) /O2 -o r5-vc6-stlport.exe $(SOURCES) $(LFLAGS)

r6-vc6-stlport.exe : 
	cl /MDd $(CFLAGS) -o r6-vc6-stlport.exe $(SOURCES) $(LFLAGS)

r7-vc6-stlport.exe : 
	cl /MD $(CFLAGS) /O2 /DBOOST_ALL_DYN_LINK -o r7-vc6-stlport.exe $(SOURCES) $(LFLAGS)

r8-vc6-stlport.exe : 
	cl /MDd $(CFLAGS) /DBOOST_ALL_DYN_LINK -o r8-vc6-stlport.exe $(SOURCES) $(LFLAGS)



