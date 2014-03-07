# copyright John Maddock 2003
# Distributed under the Boost Software License, Version 1.0. 
# (See accompanying file LICENSE_1_0.txt or copy at 
# http://www.boost.org/LICENSE_1_0.txt.

# very basic makefile for regress
#
# Borland C++ tools
#
# BCROOT defines the root directory of your bcb install
#
!ifndef BCROOT
BCROOT=$(MAKEDIR)\..
!endif
#
# sources to compile for each test:
#
SOURCES=*.cpp 

BCC32   = $(BCROOT)\bin\Bcc32.exe 
TLINK32 = $(BCROOT)\bin\ILink32.exe

IDE_LinkFLAGS32 =  -L$(BCROOT)\LIB
LINKOPTS= -ap -Tpe -x
CFLAGS= -tWC -DSTRICT; -Vx -Ve -w-inl -w-aus -w-csu -w-eff -w-rch -I$(BCROOT)\include;..\..\..\..\; -L..\..\..\..\stage\lib -L$(BCROOT)\lib\obj -L$(BCROOT)\lib\release -L..\..\build\bcb $(CXXFLAGS)

BPI= vcl.bpi rtl.bpi vclx.bpi vcle.lib

BPL= vcl.lib rtl.lib vcle.lib 
      

all :: r1.exe r2.exe r3.exe r4.exe r5.exe r6.exe r1m.exe r2m.exe r3m.exe r4m.exe r5m.exe r6m.exe r1v.exe r2v.exe r3v.exe r4v.exe r5v.exe r6v.exe r1l.exe r2l.exe r3l.exe r4l.exe r5l.exe r6l.exe r1lm.exe r2lm.exe r3lm.exe r4lm.exe r5lm.exe r6lm.exe r1lv.exe r2lv.exe r3lv.exe r4lv.exe r5lv.exe r6lv.exe
	-copy ..\..\build\bcb6\*.dll
	-copy ..\..\..\..\stage\lib\*bcb*.dll
	echo testing static single threaded version....
	r1 tests.txt test1252.txt
	r2 tests.txt
	r3 tests.txt
	r4 tests.txt test1252.txt
	r5 tests.txt
	r6 tests.txt
	echo testing static multi-threaded version....
	r1m tests.txt test1252.txt
	r2m tests.txt
	r3m tests.txt
	r4m tests.txt test1252.txt
	r5m tests.txt
	r6m tests.txt
	echo testing static VCL version....
	r1v tests.txt test1252.txt
	r2v tests.txt
	r3v tests.txt
	r4v tests.txt test1252.txt
	r5v tests.txt
	r6v tests.txt
	echo testing dll single threaded version....
	r1l tests.txt test1252.txt
	r2l tests.txt
	r3l tests.txt
	r4l tests.txt test1252.txt
	r5l tests.txt
	r6l tests.txt
	echo testing dll multi-threaded version....
	r1lm tests.txt test1252.txt
	r2lm tests.txt
	r3lm tests.txt
	r4lm tests.txt test1252.txt
	r5lm tests.txt
	r6lm tests.txt
	echo testing dll VCL version....
	r1lv tests.txt test1252.txt
	r2lv tests.txt
	r3lv tests.txt
	r4lv tests.txt test1252.txt
	r5lv tests.txt
	r6lv tests.txt


r1.exe : $(SOURCES)
	$(BCC32) -tWM- -D_NO_VCL $(CFLAGS) -er1.exe -DBOOST_RE_TEST_LOCALE_W32 $(SOURCES)

r2.exe : $(SOURCES)
	$(BCC32) -tWM- -D_NO_VCL $(CFLAGS) -er2.exe -DBOOST_RE_TEST_LOCALE_C $(SOURCES)

r3.exe : $(SOURCES)
	$(BCC32) -tWM- -D_NO_VCL $(CFLAGS) -er3.exe -DBOOST_RE_TEST_LOCALE_CPP $(SOURCES)

r4.exe : $(SOURCES)
	$(BCC32) -tWM- -D_NO_VCL $(CFLAGS) -er4.exe -DBOOST_RE_TEST_LOCALE_W32 -DTEST_UNICODE $(SOURCES)

r5.exe : $(SOURCES)
	$(BCC32) -tWM- -D_NO_VCL $(CFLAGS) -er5.exe -DBOOST_RE_TEST_LOCALE_C -DTEST_UNICODE $(SOURCES)

r6.exe : $(SOURCES)
	$(BCC32) -tWM- -D_NO_VCL $(CFLAGS) -er6.exe -DBOOST_RE_TEST_LOCALE_CPP -DTEST_UNICODE $(SOURCES)
	

r1m.exe : $(SOURCES)
	$(BCC32) -tWM -D_NO_VCL $(CFLAGS) -er1m.exe -DBOOST_RE_TEST_LOCALE_W32 $(SOURCES)

r2m.exe : $(SOURCES)
	$(BCC32) -tWM -D_NO_VCL $(CFLAGS) -er2m.exe -DBOOST_RE_TEST_LOCALE_C $(SOURCES)

r3m.exe : $(SOURCES)
	$(BCC32) -tWM -D_NO_VCL $(CFLAGS) -er3m.exe -DBOOST_RE_TEST_LOCALE_CPP $(SOURCES)

r4m.exe : $(SOURCES)
	$(BCC32) -tWM -D_NO_VCL $(CFLAGS) -er4m.exe -DBOOST_RE_TEST_LOCALE_W32 -DTEST_UNICODE $(SOURCES)

r5m.exe : $(SOURCES)
	$(BCC32) -tWM -D_NO_VCL $(CFLAGS) -er5m.exe -DBOOST_RE_TEST_LOCALE_C -DTEST_UNICODE $(SOURCES)

r6m.exe : $(SOURCES)
	$(BCC32) -tWM -D_NO_VCL $(CFLAGS) -er6m.exe -DBOOST_RE_TEST_LOCALE_CPP -DTEST_UNICODE $(SOURCES)


r1v.exe : $(SOURCES)
	$(BCC32) -tWM -tWV $(CFLAGS) -er1v.exe -DBOOST_RE_TEST_LOCALE_W32 $(SOURCES) $(BPL)

r2v.exe : $(SOURCES)
	$(BCC32) -tWM -tWV $(CFLAGS) -er2v.exe -DBOOST_RE_TEST_LOCALE_C $(SOURCES) $(BPL)

r3v.exe : $(SOURCES)
	$(BCC32) -tWM -tWV $(CFLAGS) -er3v.exe -DBOOST_RE_TEST_LOCALE_CPP $(SOURCES) $(BPL)

r4v.exe : $(SOURCES)
	$(BCC32) -tWM -tWV $(CFLAGS) -er4v.exe -DBOOST_RE_TEST_LOCALE_W32 -DTEST_UNICODE $(SOURCES) $(BPL)

r5v.exe : $(SOURCES)
	$(BCC32) -tWM -tWV $(CFLAGS) -er5v.exe -DBOOST_RE_TEST_LOCALE_C -DTEST_UNICODE $(SOURCES) $(BPL)

r6v.exe : $(SOURCES)
	$(BCC32) -tWM -tWV $(CFLAGS) -er6v.exe -DBOOST_RE_TEST_LOCALE_CPP -DTEST_UNICODE $(SOURCES) $(BPL)


r1l.exe : $(SOURCES)
	$(BCC32) -tWM- -tWR -D_NO_VCL $(CFLAGS) -er1l.exe -DBOOST_REGEX_DYN_LINK -DBOOST_RE_TEST_LOCALE_W32 $(SOURCES)

r2l.exe : $(SOURCES)
	$(BCC32) -tWM- -tWR -D_NO_VCL $(CFLAGS) -er2l.exe -DBOOST_REGEX_DYN_LINK -DBOOST_RE_TEST_LOCALE_C $(SOURCES)

r3l.exe : $(SOURCES)
	$(BCC32) -tWM- -tWR -D_NO_VCL $(CFLAGS) -er3l.exe -DBOOST_REGEX_DYN_LINK -DBOOST_RE_TEST_LOCALE_CPP $(SOURCES)

r4l.exe : $(SOURCES)
	$(BCC32) -tWM- -tWR -D_NO_VCL $(CFLAGS) -er4l.exe -DBOOST_REGEX_DYN_LINK -DBOOST_RE_TEST_LOCALE_W32 -DTEST_UNICODE $(SOURCES)

r5l.exe : $(SOURCES)
	$(BCC32) -tWM- -tWR -D_NO_VCL $(CFLAGS) -er5l.exe -DBOOST_REGEX_DYN_LINK -DBOOST_RE_TEST_LOCALE_C -DTEST_UNICODE $(SOURCES)

r6l.exe : $(SOURCES)
	$(BCC32) -tWM- -tWR -D_NO_VCL $(CFLAGS) -er6l.exe -DBOOST_REGEX_DYN_LINK -DBOOST_RE_TEST_LOCALE_CPP -DTEST_UNICODE $(SOURCES)


r1lm.exe : $(SOURCES)
	$(BCC32) -tWM -tWR -D_NO_VCL $(CFLAGS) -er1lm.exe -DBOOST_REGEX_DYN_LINK -DBOOST_RE_TEST_LOCALE_W32 $(SOURCES)

r2lm.exe : $(SOURCES)
	$(BCC32) -tWM -tWR -D_NO_VCL $(CFLAGS) -er2lm.exe -DBOOST_REGEX_DYN_LINK -DBOOST_RE_TEST_LOCALE_C $(SOURCES)

r3lm.exe : $(SOURCES)
	$(BCC32) -tWM -tWR -D_NO_VCL $(CFLAGS) -er3lm.exe -DBOOST_REGEX_DYN_LINK -DBOOST_RE_TEST_LOCALE_CPP $(SOURCES)

r4lm.exe : $(SOURCES)
	$(BCC32) -tWM -tWR -D_NO_VCL $(CFLAGS) -er4lm.exe -DBOOST_REGEX_DYN_LINK -DBOOST_RE_TEST_LOCALE_W32 -DTEST_UNICODE $(SOURCES)

r5lm.exe : $(SOURCES)
	$(BCC32) -tWM -tWR -D_NO_VCL $(CFLAGS) -er5lm.exe -DBOOST_REGEX_DYN_LINK -DBOOST_RE_TEST_LOCALE_C -DTEST_UNICODE $(SOURCES)

r6lm.exe : $(SOURCES)
	$(BCC32) -tWM -tWR -D_NO_VCL $(CFLAGS) -er6lm.exe -DBOOST_REGEX_DYN_LINK -DBOOST_RE_TEST_LOCALE_CPP -DTEST_UNICODE $(SOURCES)


r1lv.exe : $(SOURCES)
	$(BCC32) -tWM -tWR -tWV -tWC $(CFLAGS) -er1lv.exe -DBOOST_REGEX_DYN_LINK -DBOOST_RE_TEST_LOCALE_W32 $(SOURCES) $(BPI)

r2lv.exe : $(SOURCES)
	$(BCC32) -tWM -tWR -tWV -tWC $(CFLAGS) -er2lv.exe -DBOOST_REGEX_DYN_LINK -DBOOST_RE_TEST_LOCALE_C $(SOURCES) $(BPI)

r3lv.exe : $(SOURCES)
	$(BCC32) -tWM -tWR -tWV -tWC $(CFLAGS) -er3lv.exe -DBOOST_REGEX_DYN_LINK -DBOOST_RE_TEST_LOCALE_CPP $(SOURCES) $(BPI)

r4lv.exe : $(SOURCES)
	$(BCC32) -tWM -tWR -tWV -tWC $(CFLAGS) -er4lv.exe -DBOOST_REGEX_DYN_LINK -DBOOST_RE_TEST_LOCALE_W32 -DTEST_UNICODE $(SOURCES) $(BPI)

r5lv.exe : $(SOURCES)
	$(BCC32) -tWM -tWR -tWV -tWC $(CFLAGS) -er5lv.exe -DBOOST_REGEX_DYN_LINK -DBOOST_RE_TEST_LOCALE_C -DTEST_UNICODE $(SOURCES) $(BPI)

r6lv.exe : $(SOURCES)
	$(BCC32) -tWM -tWR -tWV -tWC $(CFLAGS) -er6lv.exe -DBOOST_REGEX_DYN_LINK -DBOOST_RE_TEST_LOCALE_CPP -DTEST_UNICODE $(SOURCES) $(BPI)

























