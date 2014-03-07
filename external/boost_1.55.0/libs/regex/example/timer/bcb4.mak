# copyright John Maddock 2003
# Distributed under the Boost Software License, Version 1.0. 
# (See accompanying file LICENSE_1_0.txt or copy at 
# http://www.boost.org/LICENSE_1_0.txt.

# very basic makefile for timer.exe
#
# Borland C++ tools
#
# BCROOT defines the root directory of your bc builder install
#

!ifndef BCROOT
BCROOT=$(MAKEDIR)\..
!endif

BCC32   = $(BCROOT)\bin\Bcc32.exe 

IDE_LinkFLAGS32 =  -L$(BCROOT)\LIB
COMPOPTS= -O2 -tWC -tWM- -Vx -Ve -D_NO_VCL; -I../../../../; -L..\..\build\bcb4


timer.exe : regex_timer.cpp
  $(BCC32) @&&|
 $(COMPOPTS) -e$@ regex_timer.cpp
|

























