# Microsoft Developer Studio Project File - Name="ubrkperf20" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=ubrkperf20 - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ubrkperf20.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ubrkperf20.mak" CFG="ubrkperf20 - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ubrkperf20 - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "ubrkperf20 - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE "ubrkperf20 - Win64 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "ubrkperf20 - Win64 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ubrkperf20 - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
MTL=midl.exe
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /FD /c
# ADD CPP /nologo /G6 /MD /W3 /GX /O2 /Ob2 /I "c:\src\oldICU\icu-2-0\include" /I "..\..\..\tools\toolutil" /I "c:\src\oldICU\icu-2-0\source\common" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 c:\src\oldICU\icu-2-0\lib\icuuc.lib c:\src\oldICU\icu-2-0\lib\icuin.lib ..\..\..\..\lib\icutu.lib /nologo /subsystem:console /machine:I386 /out:"..\..\..\..\bin\ubrkperf20.exe" /libpath:"..\..\..\..\lib\\"
# Begin Custom Build
InputPath=\dev\0_icu\bin\ubrkperf20.exe
SOURCE="$(InputPath)"

"c:\dev\0_icu\bin\ubrkperf20.exe" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy $(InputPath) c:\dev\0_icu\bin\ubrkperf20.exe

# End Custom Build

!ELSEIF  "$(CFG)" == "ubrkperf20 - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
MTL=midl.exe
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /FD /GZ /c
# ADD CPP /nologo /G6 /MDd /W3 /Gm /GX /ZI /Od /I "c:\src\oldICU\icu-2-0\include" /I "..\..\..\tools\toolutil" /I "c:\src\oldICU\icu-2-0\source\common" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /FR /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 c:\src\oldICU\icu-2-0\lib\icuucd.lib c:\src\oldICU\icu-2-0\lib\icuind.lib ..\..\..\..\lib\icutud.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept

!ELSEIF  "$(CFG)" == "ubrkperf20 - Win64 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
MTL=midl.exe
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN64" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /FD /c
# ADD CPP /nologo /MD /W3 /GX /Zi /O2 /Op /I "c:\src\oldICU\icu-2-0\include" /I "..\..\..\tools\toolutil" /I "c:\src\oldICU\icu-2-0\source\common" /D "WIN64" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D "_IA64_" /D "WIN32" /D "_AFX_NO_DAO_SUPPORT" /FD /Wp64 /Zm600 /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:IX86 /machine:IA64
# ADD LINK32 c:\src\oldICU\icu-2-0\lib\icuuc.lib c:\src\oldICU\icu-2-0\lib\icuin.lib ..\..\..\..\lib\icutu.lib /nologo /subsystem:console /machine:IX86 /machine:IA64

!ELSEIF  "$(CFG)" == "ubrkperf20 - Win64 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
MTL=midl.exe
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN64" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /Op /I "c:\src\oldICU\icu-2-0\include" /I "..\..\..\tools\toolutil" /I "c:\src\oldICU\icu-2-0\source\common" /D "WIN64" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D "_IA64_" /D "WIN32" /D "_AFX_NO_DAO_SUPPORT" /FR /FD /GZ /Wp64 /Zm600 /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:IX86 /pdbtype:sept /machine:IA64
# ADD LINK32 c:\src\oldICU\icu-2-0\lib\icuucd.lib c:\src\oldICU\icu-2-0\lib\icuind.lib ..\..\..\..\lib\icutud.lib /nologo /subsystem:console /incremental:no /debug /machine:IX86 /pdbtype:sept /machine:IA64

!ENDIF 

# Begin Target

# Name "ubrkperf20 - Win32 Release"
# Name "ubrkperf20 - Win32 Debug"
# Name "ubrkperf20 - Win64 Release"
# Name "ubrkperf20 - Win64 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\..\..\..\..\dev\0_icu\source\test\perf\ubrkperf\ubrkperf.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\ubrkperf.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
