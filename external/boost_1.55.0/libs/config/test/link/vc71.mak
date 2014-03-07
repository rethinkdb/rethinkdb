# copyright John Maddock 2005
# Use, modification and distribution are subject to the 
# Boost Software License, Version 1.0. (See accompanying file 
# LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#
# auto generated makefile for VC6 compiler
#
# usage:
# make
#   brings libraries up to date
# make install
#   brings libraries up to date and copies binaries to your VC6 /lib and /bin directories (recomended)
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
# add additional static-library creation flags here:
#
XSFLAGS=

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF "$(MSVCDIR)" == ""
!ERROR Variable MSVCDIR not set.
!ENDIF


ALL_HEADER=

all : main_dir liblink_test-vc71-s-1_35_dir ./vc71/liblink_test-vc71-s-1_35.lib ./vc71/liblink_test-vc71-s-1_35.exe liblink_test-vc71-mt-s-1_35_dir ./vc71/liblink_test-vc71-mt-s-1_35.lib ./vc71/liblink_test-vc71-mt-s-1_35.exe liblink_test-vc71-sgd-1_35_dir ./vc71/liblink_test-vc71-sgd-1_35.lib ./vc71/liblink_test-vc71-sgd-1_35.exe liblink_test-vc71-mt-sgd-1_35_dir ./vc71/liblink_test-vc71-mt-sgd-1_35.lib ./vc71/liblink_test-vc71-mt-sgd-1_35.exe link_test-vc71-mt-gd-1_35_dir ./vc71/link_test-vc71-mt-gd-1_35.lib ./vc71/link_test-vc71-mt-gd-1_35.exe link_test-vc71-mt-1_35_dir ./vc71/link_test-vc71-mt-1_35.lib ./vc71/link_test-vc71-mt-1_35.exe liblink_test-vc71-mt-1_35_dir ./vc71/liblink_test-vc71-mt-1_35.lib ./vc71/liblink_test-vc71-mt-1_35.exe liblink_test-vc71-mt-gd-1_35_dir ./vc71/liblink_test-vc71-mt-gd-1_35.lib ./vc71/liblink_test-vc71-mt-gd-1_35.exe

clean :  liblink_test-vc71-s-1_35_clean liblink_test-vc71-mt-s-1_35_clean liblink_test-vc71-sgd-1_35_clean liblink_test-vc71-mt-sgd-1_35_clean link_test-vc71-mt-gd-1_35_clean link_test-vc71-mt-1_35_clean liblink_test-vc71-mt-1_35_clean liblink_test-vc71-mt-gd-1_35_clean

install : all
	copy vc71\liblink_test-vc71-s-1_35.lib "$(MSVCDIR)\lib"
	copy vc71\liblink_test-vc71-mt-s-1_35.lib "$(MSVCDIR)\lib"
	copy vc71\liblink_test-vc71-sgd-1_35.lib "$(MSVCDIR)\lib"
	copy vc71\liblink_test-vc71-sgd-1_35.pdb "$(MSVCDIR)\lib"
	copy vc71\liblink_test-vc71-mt-sgd-1_35.lib "$(MSVCDIR)\lib"
	copy vc71\liblink_test-vc71-mt-sgd-1_35.pdb "$(MSVCDIR)\lib"
	copy vc71\link_test-vc71-mt-gd-1_35.lib "$(MSVCDIR)\lib"
	copy vc71\link_test-vc71-mt-gd-1_35.dll "$(MSVCDIR)\bin"
	copy vc71\link_test-vc71-mt-gd-1_35.pdb "$(MSVCDIR)\lib"
	copy vc71\link_test-vc71-mt-1_35.lib "$(MSVCDIR)\lib"
	copy vc71\link_test-vc71-mt-1_35.dll "$(MSVCDIR)\bin"
	copy vc71\liblink_test-vc71-mt-1_35.lib "$(MSVCDIR)\lib"
	copy vc71\liblink_test-vc71-mt-gd-1_35.lib "$(MSVCDIR)\lib"
	copy vc71\liblink_test-vc71-mt-gd-1_35.pdb "$(MSVCDIR)\lib"

main_dir :
	@if not exist "vc71\$(NULL)" mkdir vc71


########################################################
#
# section for liblink_test-vc71-s-1_35.lib
#
########################################################
vc71/liblink_test-vc71-s-1_35/link_test.obj: link_test.cpp $(ALL_HEADER)
	cl /c $(INCLUDES) /nologo /ML /W3 /GX /O2 /GB /GF /Gy /I..\..\..\..\ /DWIN32 /DNDEBUG /D_MBCS /D_LIB /FD $(CXXFLAGS) -Y- -Fo./vc71/liblink_test-vc71-s-1_35/ -Fdvc71/liblink_test-vc71-s-1_35.pdb link_test.cpp

liblink_test-vc71-s-1_35_dir :
	@if not exist "vc71\liblink_test-vc71-s-1_35\$(NULL)" mkdir vc71\liblink_test-vc71-s-1_35

liblink_test-vc71-s-1_35_clean :
	del vc71\liblink_test-vc71-s-1_35\*.obj
	del vc71\liblink_test-vc71-s-1_35\*.idb
	del vc71\liblink_test-vc71-s-1_35\*.exp
	del vc71\liblink_test-vc71-s-1_35\*.pch

./vc71/liblink_test-vc71-s-1_35.lib : vc71/liblink_test-vc71-s-1_35/link_test.obj
	link -lib /nologo /out:vc71/liblink_test-vc71-s-1_35.lib $(XSFLAGS)  vc71/liblink_test-vc71-s-1_35/link_test.obj

./vc71/liblink_test-vc71-s-1_35.exe : main.cpp ./vc71/liblink_test-vc71-s-1_35.lib
	cl $(INCLUDES) /nologo /ML /W3 /GX /O2 /GB /GF /Gy /I..\..\..\..\ /DWIN32 /DNDEBUG /D_MBCS /D_LIB /FD /DBOOST_LIB_DIAGNOSTIC=1 $(CXXFLAGS) -o ./vc71/liblink_test-vc71-s-1_35.exe main.cpp /link /LIBPATH:./vc71
   vc71\liblink_test-vc71-s-1_35.exe

########################################################
#
# section for liblink_test-vc71-mt-s-1_35.lib
#
########################################################
vc71/liblink_test-vc71-mt-s-1_35/link_test.obj: link_test.cpp $(ALL_HEADER)
	cl /c $(INCLUDES) /nologo /MT /W3 /GX /O2 /GB /GF /Gy /I..\..\..\..\ /D_MT /DWIN32 /DNDEBUG /D_MBCS /D_LIB /FD  $(CXXFLAGS) -Y- -Fo./vc71/liblink_test-vc71-mt-s-1_35/ -Fdvc71/liblink_test-vc71-mt-s-1_35.pdb link_test.cpp

liblink_test-vc71-mt-s-1_35_dir :
	@if not exist "vc71\liblink_test-vc71-mt-s-1_35\$(NULL)" mkdir vc71\liblink_test-vc71-mt-s-1_35

liblink_test-vc71-mt-s-1_35_clean :
	del vc71\liblink_test-vc71-mt-s-1_35\*.obj
	del vc71\liblink_test-vc71-mt-s-1_35\*.idb
	del vc71\liblink_test-vc71-mt-s-1_35\*.exp
	del vc71\liblink_test-vc71-mt-s-1_35\*.pch

./vc71/liblink_test-vc71-mt-s-1_35.lib : vc71/liblink_test-vc71-mt-s-1_35/link_test.obj
	link -lib /nologo /out:vc71/liblink_test-vc71-mt-s-1_35.lib $(XSFLAGS)  vc71/liblink_test-vc71-mt-s-1_35/link_test.obj

./vc71/liblink_test-vc71-mt-s-1_35.exe : main.cpp ./vc71/liblink_test-vc71-mt-s-1_35.lib
	cl $(INCLUDES) /nologo /MT /W3 /GX /O2 /GB /GF /Gy /I..\..\..\..\ /D_MT /DWIN32 /DNDEBUG /D_MBCS /D_LIB /FD  /DBOOST_LIB_DIAGNOSTIC=1 $(CXXFLAGS) -o ./vc71/liblink_test-vc71-mt-s-1_35.exe main.cpp /link /LIBPATH:./vc71
   vc71\liblink_test-vc71-mt-s-1_35.exe

########################################################
#
# section for liblink_test-vc71-sgd-1_35.lib
#
########################################################
vc71/liblink_test-vc71-sgd-1_35/link_test.obj: link_test.cpp $(ALL_HEADER)
	cl /c $(INCLUDES) /nologo /MLd /W3 /Gm /GX /Zi /Od /I..\..\..\..\ /DWIN32 /D_DEBUG /D_MBCS /D_LIB /FD /GX /RTC1   $(CXXFLAGS) -Y- -Fo./vc71/liblink_test-vc71-sgd-1_35/ -Fdvc71/liblink_test-vc71-sgd-1_35.pdb link_test.cpp

liblink_test-vc71-sgd-1_35_dir :
	@if not exist "vc71\liblink_test-vc71-sgd-1_35\$(NULL)" mkdir vc71\liblink_test-vc71-sgd-1_35

liblink_test-vc71-sgd-1_35_clean :
	del vc71\liblink_test-vc71-sgd-1_35\*.obj
	del vc71\liblink_test-vc71-sgd-1_35\*.idb
	del vc71\liblink_test-vc71-sgd-1_35\*.exp
	del vc71\liblink_test-vc71-sgd-1_35\*.pch

./vc71/liblink_test-vc71-sgd-1_35.lib : vc71/liblink_test-vc71-sgd-1_35/link_test.obj
	link -lib /nologo /out:vc71/liblink_test-vc71-sgd-1_35.lib $(XSFLAGS)  vc71/liblink_test-vc71-sgd-1_35/link_test.obj

./vc71/liblink_test-vc71-sgd-1_35.exe : main.cpp ./vc71/liblink_test-vc71-sgd-1_35.lib
	cl $(INCLUDES) /nologo /MLd /W3 /Gm /GX /Zi /Od /I..\..\..\..\ /DWIN32 /D_DEBUG /D_MBCS /D_LIB /FD /GX /RTC1   /DBOOST_LIB_DIAGNOSTIC=1 $(CXXFLAGS) -o ./vc71/liblink_test-vc71-sgd-1_35.exe main.cpp /link /LIBPATH:./vc71
   vc71\liblink_test-vc71-sgd-1_35.exe

########################################################
#
# section for liblink_test-vc71-mt-sgd-1_35.lib
#
########################################################
vc71/liblink_test-vc71-mt-sgd-1_35/link_test.obj: link_test.cpp $(ALL_HEADER)
	cl /c $(INCLUDES) /nologo /MTd /W3 /Gm /GX /Zi /Od /I..\..\..\..\ /DWIN32 /D_MT /D_DEBUG /D_MBCS /D_LIB /FD /GX /RTC1  $(CXXFLAGS) -Y- -Fo./vc71/liblink_test-vc71-mt-sgd-1_35/ -Fdvc71/liblink_test-vc71-mt-sgd-1_35.pdb link_test.cpp

liblink_test-vc71-mt-sgd-1_35_dir :
	@if not exist "vc71\liblink_test-vc71-mt-sgd-1_35\$(NULL)" mkdir vc71\liblink_test-vc71-mt-sgd-1_35

liblink_test-vc71-mt-sgd-1_35_clean :
	del vc71\liblink_test-vc71-mt-sgd-1_35\*.obj
	del vc71\liblink_test-vc71-mt-sgd-1_35\*.idb
	del vc71\liblink_test-vc71-mt-sgd-1_35\*.exp
	del vc71\liblink_test-vc71-mt-sgd-1_35\*.pch

./vc71/liblink_test-vc71-mt-sgd-1_35.lib : vc71/liblink_test-vc71-mt-sgd-1_35/link_test.obj
	link -lib /nologo /out:vc71/liblink_test-vc71-mt-sgd-1_35.lib $(XSFLAGS)  vc71/liblink_test-vc71-mt-sgd-1_35/link_test.obj

./vc71/liblink_test-vc71-mt-sgd-1_35.exe : main.cpp ./vc71/liblink_test-vc71-mt-sgd-1_35.lib
	cl $(INCLUDES) /nologo /MTd /W3 /Gm /GX /Zi /Od /I..\..\..\..\ /DWIN32 /D_MT /D_DEBUG /D_MBCS /D_LIB /FD /GX /RTC1  /DBOOST_LIB_DIAGNOSTIC=1 $(CXXFLAGS) -o ./vc71/liblink_test-vc71-mt-sgd-1_35.exe main.cpp /link /LIBPATH:./vc71
   vc71\liblink_test-vc71-mt-sgd-1_35.exe

########################################################
#
# section for link_test-vc71-mt-gd-1_35.lib
#
########################################################
vc71/link_test-vc71-mt-gd-1_35/link_test.obj: link_test.cpp $(ALL_HEADER)
	cl /c $(INCLUDES) /nologo /MDd /W3 /Gm /GX /Zi /Od /I..\..\..\..\ /D_DEBUG /DBOOST_DYN_LINK /DWIN32 /D_WINDOWS /D_MBCS /DUSRDLL /FD /GX /RTC1  $(CXXFLAGS) -Y- -Fo./vc71/link_test-vc71-mt-gd-1_35/ -Fdvc71/link_test-vc71-mt-gd-1_35.pdb link_test.cpp

link_test-vc71-mt-gd-1_35_dir :
	@if not exist "vc71\link_test-vc71-mt-gd-1_35\$(NULL)" mkdir vc71\link_test-vc71-mt-gd-1_35

link_test-vc71-mt-gd-1_35_clean :
	del vc71\link_test-vc71-mt-gd-1_35\*.obj
	del vc71\link_test-vc71-mt-gd-1_35\*.idb
	del vc71\link_test-vc71-mt-gd-1_35\*.exp
	del vc71\link_test-vc71-mt-gd-1_35\*.pch

./vc71/link_test-vc71-mt-gd-1_35.lib : vc71/link_test-vc71-mt-gd-1_35/link_test.obj
	link kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /incremental:yes /pdb:"vc71/link_test-vc71-mt-gd-1_35.pdb" /debug /machine:I386 /out:"vc71/link_test-vc71-mt-gd-1_35.dll" /implib:"vc71/link_test-vc71-mt-gd-1_35.lib" /LIBPATH:$(STLPORT_PATH)\lib $(XLFLAGS)  vc71/link_test-vc71-mt-gd-1_35/link_test.obj

./vc71/link_test-vc71-mt-gd-1_35.exe : main.cpp ./vc71/link_test-vc71-mt-gd-1_35.lib
	cl $(INCLUDES) /nologo /MDd /W3 /Gm /GX /Zi /Od /I..\..\..\..\ /D_DEBUG /DBOOST_DYN_LINK /DWIN32 /D_WINDOWS /D_MBCS /DUSRDLL /FD /GX /RTC1  /DBOOST_LIB_DIAGNOSTIC=1 $(CXXFLAGS) -o ./vc71/link_test-vc71-mt-gd-1_35.exe main.cpp /link /LIBPATH:./vc71
   vc71\link_test-vc71-mt-gd-1_35.exe

########################################################
#
# section for link_test-vc71-mt-1_35.lib
#
########################################################
vc71/link_test-vc71-mt-1_35/link_test.obj: link_test.cpp $(ALL_HEADER)
	cl /c $(INCLUDES) /nologo /MD /W3 /GX /O2 /GB /GF /Gy /I..\..\..\..\ /DBOOST_DYN_LINK /DNDEBUG /DWIN32 /D_WINDOWS /D_MBCS /D_USRDLL /FD  $(CXXFLAGS) -Y- -Fo./vc71/link_test-vc71-mt-1_35/ -Fdvc71/link_test-vc71-mt-1_35.pdb link_test.cpp

link_test-vc71-mt-1_35_dir :
	@if not exist "vc71\link_test-vc71-mt-1_35\$(NULL)" mkdir vc71\link_test-vc71-mt-1_35

link_test-vc71-mt-1_35_clean :
	del vc71\link_test-vc71-mt-1_35\*.obj
	del vc71\link_test-vc71-mt-1_35\*.idb
	del vc71\link_test-vc71-mt-1_35\*.exp
	del vc71\link_test-vc71-mt-1_35\*.pch

./vc71/link_test-vc71-mt-1_35.lib : vc71/link_test-vc71-mt-1_35/link_test.obj
	link kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /incremental:yes /pdb:"vc71/link_test-vc71-mt-1_35.pdb" /debug /machine:I386 /out:"vc71/link_test-vc71-mt-1_35.dll" /implib:"vc71/link_test-vc71-mt-1_35.lib" /LIBPATH:$(STLPORT_PATH)\lib $(XLFLAGS)  vc71/link_test-vc71-mt-1_35/link_test.obj

./vc71/link_test-vc71-mt-1_35.exe : main.cpp ./vc71/link_test-vc71-mt-1_35.lib
	cl $(INCLUDES) /nologo /MD /W3 /GX /O2 /GB /GF /Gy /I..\..\..\..\ /DBOOST_DYN_LINK /DNDEBUG /DWIN32 /D_WINDOWS /D_MBCS /D_USRDLL /FD  /DBOOST_LIB_DIAGNOSTIC=1 $(CXXFLAGS) -o ./vc71/link_test-vc71-mt-1_35.exe main.cpp /link /LIBPATH:./vc71
   vc71\link_test-vc71-mt-1_35.exe

########################################################
#
# section for liblink_test-vc71-mt-1_35.lib
#
########################################################
vc71/liblink_test-vc71-mt-1_35/link_test.obj: link_test.cpp $(ALL_HEADER)
	cl /c $(INCLUDES) /nologo /MD /W3 /GX /O2 /GB /GF /Gy /I..\..\..\..\ /DBOOST_REGEX_STATIC_LINK /DNDEBUG /DWIN32 /D_WINDOWS /D_MBCS /D_USRDLL /FD  $(CXXFLAGS) -Y- -Fo./vc71/liblink_test-vc71-mt-1_35/ -Fdvc71/liblink_test-vc71-mt-1_35.pdb link_test.cpp

liblink_test-vc71-mt-1_35_dir :
	@if not exist "vc71\liblink_test-vc71-mt-1_35\$(NULL)" mkdir vc71\liblink_test-vc71-mt-1_35

liblink_test-vc71-mt-1_35_clean :
	del vc71\liblink_test-vc71-mt-1_35\*.obj
	del vc71\liblink_test-vc71-mt-1_35\*.idb
	del vc71\liblink_test-vc71-mt-1_35\*.exp
	del vc71\liblink_test-vc71-mt-1_35\*.pch

./vc71/liblink_test-vc71-mt-1_35.lib : vc71/liblink_test-vc71-mt-1_35/link_test.obj
	link -lib /nologo /out:vc71/liblink_test-vc71-mt-1_35.lib $(XSFLAGS)  vc71/liblink_test-vc71-mt-1_35/link_test.obj

./vc71/liblink_test-vc71-mt-1_35.exe : main.cpp ./vc71/liblink_test-vc71-mt-1_35.lib
	cl $(INCLUDES) /nologo /MD /W3 /GX /O2 /GB /GF /Gy /I..\..\..\..\ /DBOOST_REGEX_STATIC_LINK /DNDEBUG /DWIN32 /D_WINDOWS /D_MBCS /D_USRDLL /FD  /DBOOST_LIB_DIAGNOSTIC=1 $(CXXFLAGS) -o ./vc71/liblink_test-vc71-mt-1_35.exe main.cpp /link /LIBPATH:./vc71
   vc71\liblink_test-vc71-mt-1_35.exe

########################################################
#
# section for liblink_test-vc71-mt-gd-1_35.lib
#
########################################################
vc71/liblink_test-vc71-mt-gd-1_35/link_test.obj: link_test.cpp $(ALL_HEADER)
	cl /c $(INCLUDES) /nologo /MDd /W3 /Gm /GX /Zi /Od /I..\..\..\..\ /DBOOST_REGEX_STATIC_LINK /D_DEBUG /DWIN32 /D_WINDOWS /D_MBCS /DUSRDLL /FD /GX /RTC1  $(CXXFLAGS) -Y- -Fo./vc71/liblink_test-vc71-mt-gd-1_35/ -Fdvc71/liblink_test-vc71-mt-gd-1_35.pdb link_test.cpp

liblink_test-vc71-mt-gd-1_35_dir :
	@if not exist "vc71\liblink_test-vc71-mt-gd-1_35\$(NULL)" mkdir vc71\liblink_test-vc71-mt-gd-1_35

liblink_test-vc71-mt-gd-1_35_clean :
	del vc71\liblink_test-vc71-mt-gd-1_35\*.obj
	del vc71\liblink_test-vc71-mt-gd-1_35\*.idb
	del vc71\liblink_test-vc71-mt-gd-1_35\*.exp
	del vc71\liblink_test-vc71-mt-gd-1_35\*.pch

./vc71/liblink_test-vc71-mt-gd-1_35.lib : vc71/liblink_test-vc71-mt-gd-1_35/link_test.obj
	link -lib /nologo /out:vc71/liblink_test-vc71-mt-gd-1_35.lib $(XSFLAGS)  vc71/liblink_test-vc71-mt-gd-1_35/link_test.obj

./vc71/liblink_test-vc71-mt-gd-1_35.exe : main.cpp ./vc71/liblink_test-vc71-mt-gd-1_35.lib
	cl $(INCLUDES) /nologo /MDd /W3 /Gm /GX /Zi /Od /I..\..\..\..\ /DBOOST_REGEX_STATIC_LINK /D_DEBUG /DWIN32 /D_WINDOWS /D_MBCS /DUSRDLL /FD /GX /RTC1  /DBOOST_LIB_DIAGNOSTIC=1 $(CXXFLAGS) -o ./vc71/liblink_test-vc71-mt-gd-1_35.exe main.cpp /link /LIBPATH:./vc71
   vc71\liblink_test-vc71-mt-gd-1_35.exe

