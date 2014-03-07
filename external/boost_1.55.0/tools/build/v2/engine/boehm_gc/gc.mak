# Microsoft Developer Studio Generated NMAKE File, Format Version 4.10
# This has been hand-edited way too many times.
# A clean, manually generated makefile would be an improvement.

# TARGTYPE "Win32 (x86) Application" 0x0101
# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

AO_VERSION=1.2
!IF "$(CFG)" == ""
CFG=gctest - Win32 Release
!MESSAGE No configuration specified.  Defaulting to cord - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "gc - Win32 Release" && "$(CFG)" != "gc - Win32 Debug" &&\
 "$(CFG)" != "gctest - Win32 Release" && "$(CFG)" != "gctest - Win32 Debug" &&\
 "$(CFG)" != "cord - Win32 Release" && "$(CFG)" != "cord - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "gc.mak" CFG="cord - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "gc - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "gc - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "gctest - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "gctest - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "cord - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "cord - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 
################################################################################
# Begin Project
# PROP Target_Last_Scanned "gctest - Win32 Debug"

!IF  "$(CFG)" == "gc - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
OUTDIR=.\Release
INTDIR=.\Release

ALL : ".\Release\gc.dll" ".\Release\gc.bsc"

CLEAN : 
	-@erase ".\Release\allchblk.obj"
	-@erase ".\Release\allchblk.sbr"
	-@erase ".\Release\alloc.obj"
	-@erase ".\Release\alloc.sbr"
	-@erase ".\Release\blacklst.obj"
	-@erase ".\Release\blacklst.sbr"
	-@erase ".\Release\checksums.obj"
	-@erase ".\Release\checksums.sbr"
	-@erase ".\Release\dbg_mlc.obj"
	-@erase ".\Release\dbg_mlc.sbr"
	-@erase ".\Release\dyn_load.obj"
	-@erase ".\Release\dyn_load.sbr"
	-@erase ".\Release\finalize.obj"
	-@erase ".\Release\finalize.sbr"
	-@erase ".\Release\gc.bsc"
	-@erase ".\Release\gc_cpp.obj"
	-@erase ".\Release\gc_cpp.sbr"
	-@erase ".\Release\gc.dll"
	-@erase ".\Release\gc.exp"
	-@erase ".\Release\gc.lib"
	-@erase ".\Release\headers.obj"
	-@erase ".\Release\headers.sbr"
	-@erase ".\Release\mach_dep.obj"
	-@erase ".\Release\mach_dep.sbr"
	-@erase ".\Release\malloc.obj"
	-@erase ".\Release\malloc.sbr"
	-@erase ".\Release\mallocx.obj"
	-@erase ".\Release\mallocx.sbr"
	-@erase ".\Release\mark.obj"
	-@erase ".\Release\mark.sbr"
	-@erase ".\Release\mark_rts.obj"
	-@erase ".\Release\mark_rts.sbr"
	-@erase ".\Release\misc.obj"
	-@erase ".\Release\misc.sbr"
	-@erase ".\Release\new_hblk.obj"
	-@erase ".\Release\new_hblk.sbr"
	-@erase ".\Release\obj_map.obj"
	-@erase ".\Release\obj_map.sbr"
	-@erase ".\Release\os_dep.obj"
	-@erase ".\Release\os_dep.sbr"
	-@erase ".\Release\ptr_chck.obj"
	-@erase ".\Release\ptr_chck.sbr"
	-@erase ".\Release\reclaim.obj"
	-@erase ".\Release\reclaim.sbr"
	-@erase ".\Release\stubborn.obj"
	-@erase ".\Release\stubborn.sbr"
	-@erase ".\Release\typd_mlc.obj"
	-@erase ".\Release\typd_mlc.sbr"
	-@erase ".\Release\win32_threads.obj"
	-@erase ".\Release\win32_threads.sbr"
	-@erase ".\Release\msvc_dbg.obj"
	-@erase ".\Release\msvc_dbg.sbr"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I include /D "NDEBUG" /D "GC_BUILD" /D "WIN32" /D "_WINDOWS" /D "ALL_INTERIOR_POINTERS" /D "__STDC__" /D "GC_WIN32_THREADS" /FR /YX /c
CPP_PROJ=/nologo /MD /W3 /GX /O2 /I include /D "NDEBUG" /D "GC_BUILD" /D\
 "WIN32" /D "_WINDOWS" /D "ALL_INTERIOR_POINTERS" /D "__STDC__" /D\
 "GC_WIN32_THREADS" /FR"$(INTDIR)/" /Fp"$(INTDIR)/gc.pch" \
 /Ilibatomic_ops-$(AO_VERSION)/src /YX /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\Release/
CPP_SBRS=.\Release/

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.c{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

MTL=mktyplib.exe
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
RSC=rc.exe
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/gc.bsc" 
BSC32_SBRS= \
	".\Release\allchblk.sbr" \
	".\Release\alloc.sbr" \
	".\Release\blacklst.sbr" \
	".\Release\checksums.sbr" \
	".\Release\dbg_mlc.sbr" \
	".\Release\dyn_load.sbr" \
	".\Release\finalize.sbr" \
	".\Release\gc_cpp.sbr" \
	".\Release\headers.sbr" \
	".\Release\mach_dep.sbr" \
	".\Release\malloc.sbr" \
	".\Release\mallocx.sbr" \
	".\Release\mark.sbr" \
	".\Release\mark_rts.sbr" \
	".\Release\misc.sbr" \
	".\Release\new_hblk.sbr" \
	".\Release\obj_map.sbr" \
	".\Release\os_dep.sbr" \
	".\Release\ptr_chck.sbr" \
	".\Release\reclaim.sbr" \
	".\Release\stubborn.sbr" \
	".\Release\typd_mlc.sbr" \
	".\Release\msvc_dbg.sbr" \
	".\Release\win32_threads.sbr"

".\Release\gc.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:windows /dll /incremental:no\
 /pdb:"$(OUTDIR)/gc.pdb" /machine:I386 /out:"$(OUTDIR)/gc.dll"\
 /implib:"$(OUTDIR)/gc.lib" 
LINK32_OBJS= \
	".\Release\allchblk.obj" \
	".\Release\alloc.obj" \
	".\Release\blacklst.obj" \
	".\Release\checksums.obj" \
	".\Release\dbg_mlc.obj" \
	".\Release\dyn_load.obj" \
	".\Release\finalize.obj" \
	".\Release\gc_cpp.obj" \
	".\Release\headers.obj" \
	".\Release\mach_dep.obj" \
	".\Release\malloc.obj" \
	".\Release\mallocx.obj" \
	".\Release\mark.obj" \
	".\Release\mark_rts.obj" \
	".\Release\misc.obj" \
	".\Release\new_hblk.obj" \
	".\Release\obj_map.obj" \
	".\Release\os_dep.obj" \
	".\Release\ptr_chck.obj" \
	".\Release\reclaim.obj" \
	".\Release\stubborn.obj" \
	".\Release\typd_mlc.obj" \
	".\Release\msvc_dbg.obj" \
	".\Release\win32_threads.obj"

".\Release\gc.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "gc - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
OUTDIR=.\Debug
INTDIR=.\Debug

ALL : ".\Debug\gc.dll" ".\Debug\gc.bsc"

CLEAN : 
	-@erase ".\Debug\allchblk.obj"
	-@erase ".\Debug\allchblk.sbr"
	-@erase ".\Debug\alloc.obj"
	-@erase ".\Debug\alloc.sbr"
	-@erase ".\Debug\blacklst.obj"
	-@erase ".\Debug\blacklst.sbr"
	-@erase ".\Debug\checksums.obj"
	-@erase ".\Debug\checksums.sbr"
	-@erase ".\Debug\dbg_mlc.obj"
	-@erase ".\Debug\dbg_mlc.sbr"
	-@erase ".\Debug\dyn_load.obj"
	-@erase ".\Debug\dyn_load.sbr"
	-@erase ".\Debug\finalize.obj"
	-@erase ".\Debug\finalize.sbr"
	-@erase ".\Debug\gc_cpp.obj"
	-@erase ".\Debug\gc_cpp.sbr"
	-@erase ".\Debug\gc.bsc"
	-@erase ".\Debug\gc.dll"
	-@erase ".\Debug\gc.exp"
	-@erase ".\Debug\gc.lib"
	-@erase ".\Debug\gc.map"
	-@erase ".\Debug\gc.pdb"
	-@erase ".\Debug\headers.obj"
	-@erase ".\Debug\headers.sbr"
	-@erase ".\Debug\mach_dep.obj"
	-@erase ".\Debug\mach_dep.sbr"
	-@erase ".\Debug\malloc.obj"
	-@erase ".\Debug\malloc.sbr"
	-@erase ".\Debug\mallocx.obj"
	-@erase ".\Debug\mallocx.sbr"
	-@erase ".\Debug\mark.obj"
	-@erase ".\Debug\mark.sbr"
	-@erase ".\Debug\mark_rts.obj"
	-@erase ".\Debug\mark_rts.sbr"
	-@erase ".\Debug\misc.obj"
	-@erase ".\Debug\misc.sbr"
	-@erase ".\Debug\new_hblk.obj"
	-@erase ".\Debug\new_hblk.sbr"
	-@erase ".\Debug\obj_map.obj"
	-@erase ".\Debug\obj_map.sbr"
	-@erase ".\Debug\os_dep.obj"
	-@erase ".\Debug\os_dep.sbr"
	-@erase ".\Debug\ptr_chck.obj"
	-@erase ".\Debug\ptr_chck.sbr"
	-@erase ".\Debug\reclaim.obj"
	-@erase ".\Debug\reclaim.sbr"
	-@erase ".\Debug\stubborn.obj"
	-@erase ".\Debug\stubborn.sbr"
	-@erase ".\Debug\typd_mlc.obj"
	-@erase ".\Debug\typd_mlc.sbr"
	-@erase ".\Debug\vc40.idb"
	-@erase ".\Debug\vc40.pdb"
	-@erase ".\Debug\win32_threads.obj"
	-@erase ".\Debug\win32_threads.sbr"
	-@erase ".\Debug\msvc_dbg.obj"
	-@erase ".\Debug\msvc_dbg.sbr"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /I include /D "_DEBUG" /D "GC_BUILD" /D "WIN32" /D "_WINDOWS" /D "ALL_INTERIOR_POINTERS" /D "__STDC__" /D "GC_WIN32_THREADS" /FR /YX /c
CPP_PROJ=/nologo /MDd /W3 /Gm /GX /Zi /Od /I include /D "_DEBUG" /D "GC_BUILD"\
 /D "WIN32" /D "_WINDOWS" /D "ALL_INTERIOR_POINTERS" \
 /D "GC_ASSERTIONS" /D "__STDC__" /D\
 "GC_WIN32_THREADS" /FR"$(INTDIR)/" /Fp"$(INTDIR)/gc.pch" /YX /Fo"$(INTDIR)/"\
 /Ilibatomic_ops-$(AO_VERSION)/src /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.\Debug/

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.c{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

MTL=mktyplib.exe
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
RSC=rc.exe
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/gc.bsc" 
BSC32_SBRS= \
	".\Debug\allchblk.sbr" \
	".\Debug\alloc.sbr" \
	".\Debug\blacklst.sbr" \
	".\Debug\checksums.sbr" \
	".\Debug\dbg_mlc.sbr" \
	".\Debug\dyn_load.sbr" \
	".\Debug\finalize.sbr" \
	".\Debug\gc_cpp.sbr" \
	".\Debug\headers.sbr" \
	".\Debug\mach_dep.sbr" \
	".\Debug\malloc.sbr" \
	".\Debug\mallocx.sbr" \
	".\Debug\mark.sbr" \
	".\Debug\mark_rts.sbr" \
	".\Debug\misc.sbr" \
	".\Debug\new_hblk.sbr" \
	".\Debug\obj_map.sbr" \
	".\Debug\os_dep.sbr" \
	".\Debug\ptr_chck.sbr" \
	".\Debug\reclaim.sbr" \
	".\Debug\stubborn.sbr" \
	".\Debug\typd_mlc.sbr" \
	".\Debug\msvc_dbg.sbr" \
	".\Debug\win32_threads.sbr"

".\Debug\gc.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /incremental:no /map /debug /machine:I386
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:windows /dll /incremental:no\
 /pdb:"$(OUTDIR)/gc.pdb" /map:"$(INTDIR)/gc.map" /debug /machine:I386\
 /out:"$(OUTDIR)/gc.dll" /implib:"$(OUTDIR)/gc.lib" 
LINK32_OBJS= \
	".\Debug\allchblk.obj" \
	".\Debug\alloc.obj" \
	".\Debug\blacklst.obj" \
	".\Debug\checksums.obj" \
	".\Debug\dbg_mlc.obj" \
	".\Debug\dyn_load.obj" \
	".\Debug\finalize.obj" \
	".\Debug\gc_cpp.obj" \
	".\Debug\headers.obj" \
	".\Debug\mach_dep.obj" \
	".\Debug\malloc.obj" \
	".\Debug\mallocx.obj" \
	".\Debug\mark.obj" \
	".\Debug\mark_rts.obj" \
	".\Debug\misc.obj" \
	".\Debug\new_hblk.obj" \
	".\Debug\obj_map.obj" \
	".\Debug\os_dep.obj" \
	".\Debug\ptr_chck.obj" \
	".\Debug\reclaim.obj" \
	".\Debug\stubborn.obj" \
	".\Debug\typd_mlc.obj" \
	".\Debug\msvc_dbg.obj" \
	".\Debug\win32_threads.obj"

".\Debug\gc.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "gctest - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "gctest\Release"
# PROP BASE Intermediate_Dir "gctest\Release"
# PROP BASE Target_Dir "gctest"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "gctest\Release"
# PROP Intermediate_Dir "gctest\Release"
# PROP Target_Dir "gctest"
OUTDIR=.\gctest\Release
INTDIR=.\gctest\Release

ALL : "gc - Win32 Release" ".\Release\gctest.exe"

CLEAN : 
	-@erase ".\gctest\Release\test.obj"
	-@erase ".\Release\gctest.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

test.c : tests\test.c
	copy tests\test.c test.c

CPP=cl.exe
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I include /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "ALL_INTERIOR_POINTERS" /D "__STDC__" /D "GC_WIN32_THREADS" /YX /c
CPP_PROJ=/nologo /MD /W3 /GX /O2 /I include /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D\
 "ALL_INTERIOR_POINTERS" /D "__STDC__" /D "GC_WIN32_THREADS"\
 /Ilibatomic_ops-$(AO_VERSION)/src /Fp"$(INTDIR)/gctest.pch" \
 /YX /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\gctest\Release/
CPP_SBRS=.\.

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.c{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

MTL=mktyplib.exe
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
RSC=rc.exe
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/gctest.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386 /out:"Release/gctest.exe"
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:windows /incremental:no\
 /pdb:"$(OUTDIR)/gctest.pdb" /machine:I386 /out:"Release/gctest.exe" 
LINK32_OBJS= \
	".\gctest\Release\test.obj" \
	".\Release\gc.lib"

".\Release\gctest.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "gctest - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "gctest\Debug"
# PROP BASE Intermediate_Dir "gctest\Debug"
# PROP BASE Target_Dir "gctest"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "gctest\Debug"
# PROP Intermediate_Dir "gctest\Debug"
# PROP Target_Dir "gctest"
OUTDIR=.\gctest\Debug
INTDIR=.\gctest\Debug

ALL : "gc - Win32 Debug" ".\Debug\gctest.exe" ".\gctest\Debug\gctest.bsc"

CLEAN : 
	-@erase ".\Debug\gctest.exe"
	-@erase ".\gctest\Debug\gctest.bsc"
	-@erase ".\gctest\Debug\gctest.map"
	-@erase ".\gctest\Debug\gctest.pdb"
	-@erase ".\gctest\Debug\test.obj"
	-@erase ".\gctest\Debug\test.sbr"
	-@erase ".\gctest\Debug\vc40.idb"
	-@erase ".\gctest\Debug\vc40.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "ALL_INTERIOR_POINTERS" /D "__STDC__" /D "GC_WIN32_THREADS" /FR /YX /c
CPP_PROJ=/nologo /MDd /W3 /Gm /GX /Zi /Od /I include /D "_DEBUG" /D "WIN32" /D "_WINDOWS"\
 /D "ALL_INTERIOR_POINTERS" /D "__STDC__" /D "GC_WIN32_THREADS" /FR"$(INTDIR)/"\
 /Ilibatomic_ops-$(AO_VERSION)/src /Fp"$(INTDIR)/gctest.pch" /YX /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\gctest\Debug/
CPP_SBRS=.\gctest\Debug/

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.c{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

MTL=mktyplib.exe
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
RSC=rc.exe
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/gctest.bsc" 
BSC32_SBRS= \
	".\gctest\Debug\test.sbr"

".\gctest\Debug\gctest.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /incremental:no /map /debug /machine:I386 /out:"Debug/gctest.exe"
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:windows /incremental:no\
 /pdb:"$(OUTDIR)/gctest.pdb" /map:"$(INTDIR)/gctest.map" /debug /machine:I386\
 /out:"Debug/gctest.exe" 
LINK32_OBJS= \
	".\Debug\gc.lib" \
	".\gctest\Debug\test.obj"

".\Debug\gctest.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "cord - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "cord\Release"
# PROP BASE Intermediate_Dir "cord\Release"
# PROP BASE Target_Dir "cord"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "cord\Release"
# PROP Intermediate_Dir "cord\Release"
# PROP Target_Dir "cord"
OUTDIR=.\cord\Release
INTDIR=.\cord\Release

ALL : "gc - Win32 Release" ".\Release\de.exe"

CLEAN : 
	-@erase ".\cord\Release\cordbscs.obj"
	-@erase ".\cord\Release\cordxtra.obj"
	-@erase ".\cord\Release\de.obj"
	-@erase ".\cord\Release\de_win.obj"
	-@erase ".\cord\Release\de_win.res"
	-@erase ".\Release\de.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "." /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "ALL_INTERIOR_POINTERS" /YX /c
CPP_PROJ=/nologo /MD /W3 /GX /O2 /I "." /I include /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D\
 /Ilibatomic_ops-$(AO_VERSION)/src "ALL_INTERIOR_POINTERS" /Fp"$(INTDIR)/cord.pch" /YX /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\cord\Release/
CPP_SBRS=.\.

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.c{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

MTL=mktyplib.exe
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
RSC=rc.exe
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
RSC_PROJ=/l 0x809 /fo"$(INTDIR)/de_win.res" /d "NDEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/cord.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386 /out:"Release/de.exe"
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:windows /incremental:no /pdb:"$(OUTDIR)/de.pdb"\
 /machine:I386 /out:"Release/de.exe" 
LINK32_OBJS= \
	".\cord\Release\cordbscs.obj" \
	".\cord\Release\cordxtra.obj" \
	".\cord\Release\de.obj" \
	".\cord\Release\de_win.obj" \
	".\cord\Release\de_win.res" \
	".\Release\gc.lib"

".\Release\de.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "cord - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "cord\Debug"
# PROP BASE Intermediate_Dir "cord\Debug"
# PROP BASE Target_Dir "cord"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "cord\Debug"
# PROP Intermediate_Dir "cord\Debug"
# PROP Target_Dir "cord"
OUTDIR=.\cord\Debug
INTDIR=.\cord\Debug

ALL : "gc - Win32 Debug" ".\Debug\de.exe"

CLEAN : 
	-@erase ".\cord\Debug\cordbscs.obj"
	-@erase ".\cord\Debug\cordxtra.obj"
	-@erase ".\cord\Debug\de.obj"
	-@erase ".\cord\Debug\de.pdb"
	-@erase ".\cord\Debug\de_win.obj"
	-@erase ".\cord\Debug\de_win.res"
	-@erase ".\cord\Debug\vc40.idb"
	-@erase ".\cord\Debug\vc40.pdb"
	-@erase ".\Debug\de.exe"
	-@erase ".\Debug\de.ilk"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /I "." /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "ALL_INTERIOR_POINTERS" /YX /c
CPP_PROJ=/nologo /MDd /W3 /Gm /GX /Zi /Od /I "." /I include /D "_DEBUG" /D "WIN32" /D\
 "_WINDOWS" /D "ALL_INTERIOR_POINTERS" /Fp"$(INTDIR)/cord.pch" /YX\
 /Ilibatomic_ops-$(AO_VERSION)/src /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\cord\Debug/
CPP_SBRS=.\.

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.c{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

MTL=mktyplib.exe
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
RSC=rc.exe
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
RSC_PROJ=/l 0x809 /fo"$(INTDIR)/de_win.res" /d "_DEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/cord.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /out:"Debug/de.exe"
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:windows /incremental:yes\
 /pdb:"$(OUTDIR)/de.pdb" /debug /machine:I386 /out:"Debug/de.exe" 
LINK32_OBJS= \
	".\cord\Debug\cordbscs.obj" \
	".\cord\Debug\cordxtra.obj" \
	".\cord\Debug\de.obj" \
	".\cord\Debug\de_win.obj" \
	".\cord\Debug\de_win.res" \
	".\Debug\gc.lib"

".\Debug\de.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 

################################################################################
# Begin Target

# Name "gc - Win32 Release"
# Name "gc - Win32 Debug"

!IF  "$(CFG)" == "gc - Win32 Release"

!ELSEIF  "$(CFG)" == "gc - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\gc_cpp.cpp

!IF  "$(CFG)" == "gc - Win32 Release"

DEP_CPP_RECLA=\
	".\include\private\gcconfig.h"\
	".\include\gc.h"\
	".\include\private\gc_hdrs.h"\
	".\include\private\gc_priv.h"\
	".\include\gc_cpp.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_RECLA=\
	".\th\PCR_Th.h"\
	".\th\PCR_ThCrSec.h"\
	".\th\PCR_ThCtl.h"\
	

".\Release\gc_cpp.obj" : $(SOURCE) $(DEP_CPP_RECLA) "$(INTDIR)"

".\Release\gc_cpp.sbr" : $(SOURCE) $(DEP_CPP_RECLA) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "gc - Win32 Debug"

DEP_CPP_RECLA=\
	".\include\private\gcconfig.h"\
	".\include\gc.h"\
	".\include\private\gc_hdrs.h"\
	".\include\private\gc_priv.h"\
	".\include\gc_cpp.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_RECLA=\
	".\th\PCR_Th.h"\
	".\th\PCR_ThCrSec.h"\
	".\th\PCR_ThCtl.h"\
	

".\Debug\gc_cpp.obj" : $(SOURCE) $(DEP_CPP_RECLA) "$(INTDIR)"

".\Debug\gc_cpp.sbr" : $(SOURCE) $(DEP_CPP_RECLA) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\reclaim.c

!IF  "$(CFG)" == "gc - Win32 Release"

DEP_CPP_RECLA=\
	".\include\private\gcconfig.h"\
	".\include\gc.h"\
	".\include\private\gc_hdrs.h"\
	".\include\private\gc_priv.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_RECLA=\
	".\th\PCR_Th.h"\
	".\th\PCR_ThCrSec.h"\
	".\th\PCR_ThCtl.h"\
	

".\Release\reclaim.obj" : $(SOURCE) $(DEP_CPP_RECLA) "$(INTDIR)"

".\Release\reclaim.sbr" : $(SOURCE) $(DEP_CPP_RECLA) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "gc - Win32 Debug"

DEP_CPP_RECLA=\
	".\include\private\gcconfig.h"\
	".\include\gc.h"\
	".\include\private\gc_hdrs.h"\
	".\include\private\gc_priv.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_RECLA=\
	".\th\PCR_Th.h"\
	".\th\PCR_ThCrSec.h"\
	".\th\PCR_ThCtl.h"\
	

".\Debug\reclaim.obj" : $(SOURCE) $(DEP_CPP_RECLA) "$(INTDIR)"

".\Debug\reclaim.sbr" : $(SOURCE) $(DEP_CPP_RECLA) "$(INTDIR)"


!ENDIF 

# End Source File

################################################################################
# Begin Source File

SOURCE=.\os_dep.c

!IF  "$(CFG)" == "gc - Win32 Release"

DEP_CPP_OS_DE=\
	".\include\private\gcconfig.h"\
	".\include\gc.h"\
	".\include\private\gc_hdrs.h"\
	".\include\private\gc_priv.h"\
	{$(INCLUDE)}"\sys\STAT.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_OS_DE=\
	".\il\PCR_IL.h"\
	".\mm\PCR_MM.h"\
	".\th\PCR_Th.h"\
	".\th\PCR_ThCrSec.h"\
	".\th\PCR_ThCtl.h"\
	".\vd\PCR_VD.h"\
	

".\Release\os_dep.obj" : $(SOURCE) $(DEP_CPP_OS_DE) "$(INTDIR)"

".\Release\os_dep.sbr" : $(SOURCE) $(DEP_CPP_OS_DE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "gc - Win32 Debug"

DEP_CPP_OS_DE=\
	".\include\private\gcconfig.h"\
	".\include\gc.h"\
	".\include\private\gc_hdrs.h"\
	".\include\private\gc_priv.h"\
	{$(INCLUDE)}"\sys\STAT.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_OS_DE=\
	".\il\PCR_IL.h"\
	".\mm\PCR_MM.h"\
	".\th\PCR_Th.h"\
	".\th\PCR_ThCrSec.h"\
	".\th\PCR_ThCtl.h"\
	".\vd\PCR_VD.h"\
	

".\Debug\os_dep.obj" : $(SOURCE) $(DEP_CPP_OS_DE) "$(INTDIR)"

".\Debug\os_dep.sbr" : $(SOURCE) $(DEP_CPP_OS_DE) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\misc.c

!IF  "$(CFG)" == "gc - Win32 Release"

DEP_CPP_MISC_=\
	".\include\private\gcconfig.h"\
	".\include\gc.h"\
	".\include\private\gc_hdrs.h"\
	".\include\private\gc_priv.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_MISC_=\
	".\il\PCR_IL.h"\
	".\th\PCR_Th.h"\
	".\th\PCR_ThCrSec.h"\
	".\th\PCR_ThCtl.h"\
	

".\Release\misc.obj" : $(SOURCE) $(DEP_CPP_MISC_) "$(INTDIR)"

".\Release\misc.sbr" : $(SOURCE) $(DEP_CPP_MISC_) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "gc - Win32 Debug"

DEP_CPP_MISC_=\
	".\include\private\gcconfig.h"\
	".\include\gc.h"\
	".\include\private\gc_hdrs.h"\
	".\include\private\gc_priv.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_MISC_=\
	".\il\PCR_IL.h"\
	".\th\PCR_Th.h"\
	".\th\PCR_ThCrSec.h"\
	".\th\PCR_ThCtl.h"\
	

".\Debug\misc.obj" : $(SOURCE) $(DEP_CPP_MISC_) "$(INTDIR)"

".\Debug\misc.sbr" : $(SOURCE) $(DEP_CPP_MISC_) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\mark_rts.c

!IF  "$(CFG)" == "gc - Win32 Release"

DEP_CPP_MARK_=\
	".\include\private\gcconfig.h"\
	".\include\gc.h"\
	".\include\private\gc_hdrs.h"\
	".\include\private\gc_priv.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_MARK_=\
	".\th\PCR_Th.h"\
	".\th\PCR_ThCrSec.h"\
	".\th\PCR_ThCtl.h"\
	

".\Release\mark_rts.obj" : $(SOURCE) $(DEP_CPP_MARK_) "$(INTDIR)"

".\Release\mark_rts.sbr" : $(SOURCE) $(DEP_CPP_MARK_) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "gc - Win32 Debug"

DEP_CPP_MARK_=\
	".\include\private\gcconfig.h"\
	".\include\gc.h"\
	".\include\private\gc_hdrs.h"\
	".\include\private\gc_priv.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_MARK_=\
	".\th\PCR_Th.h"\
	".\th\PCR_ThCrSec.h"\
	".\th\PCR_ThCtl.h"\
	

".\Debug\mark_rts.obj" : $(SOURCE) $(DEP_CPP_MARK_) "$(INTDIR)"

".\Debug\mark_rts.sbr" : $(SOURCE) $(DEP_CPP_MARK_) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\mach_dep.c

!IF  "$(CFG)" == "gc - Win32 Release"

DEP_CPP_MACH_=\
	".\include\private\gcconfig.h"\
	".\include\gc.h"\
	".\include\private\gc_hdrs.h"\
	".\include\private\gc_priv.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_MACH_=\
	".\th\PCR_Th.h"\
	".\th\PCR_ThCrSec.h"\
	".\th\PCR_ThCtl.h"\
	

".\Release\mach_dep.obj" : $(SOURCE) $(DEP_CPP_MACH_) "$(INTDIR)"

".\Release\mach_dep.sbr" : $(SOURCE) $(DEP_CPP_MACH_) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "gc - Win32 Debug"

DEP_CPP_MACH_=\
	".\include\private\gcconfig.h"\
	".\include\gc.h"\
	".\include\private\gc_hdrs.h"\
	".\include\private\gc_priv.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_MACH_=\
	".\th\PCR_Th.h"\
	".\th\PCR_ThCrSec.h"\
	".\th\PCR_ThCtl.h"\
	

".\Debug\mach_dep.obj" : $(SOURCE) $(DEP_CPP_MACH_) "$(INTDIR)"

".\Debug\mach_dep.sbr" : $(SOURCE) $(DEP_CPP_MACH_) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\headers.c

!IF  "$(CFG)" == "gc - Win32 Release"

DEP_CPP_HEADE=\
	".\include\private\gcconfig.h"\
	".\include\gc.h"\
	".\include\private\gc_hdrs.h"\
	".\include\private\gc_priv.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_HEADE=\
	".\th\PCR_Th.h"\
	".\th\PCR_ThCrSec.h"\
	".\th\PCR_ThCtl.h"\
	

".\Release\headers.obj" : $(SOURCE) $(DEP_CPP_HEADE) "$(INTDIR)"

".\Release\headers.sbr" : $(SOURCE) $(DEP_CPP_HEADE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "gc - Win32 Debug"

DEP_CPP_HEADE=\
	".\include\private\gcconfig.h"\
	".\include\gc.h"\
	".\include\private\gc_hdrs.h"\
	".\include\private\gc_priv.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_HEADE=\
	".\th\PCR_Th.h"\
	".\th\PCR_ThCrSec.h"\
	".\th\PCR_ThCtl.h"\
	

".\Debug\headers.obj" : $(SOURCE) $(DEP_CPP_HEADE) "$(INTDIR)"

".\Debug\headers.sbr" : $(SOURCE) $(DEP_CPP_HEADE) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\alloc.c

!IF  "$(CFG)" == "gc - Win32 Release"

DEP_CPP_ALLOC=\
	".\include\private\gcconfig.h"\
	".\include\gc.h"\
	".\include\private\gc_hdrs.h"\
	".\include\private\gc_priv.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_ALLOC=\
	".\th\PCR_Th.h"\
	".\th\PCR_ThCrSec.h"\
	".\th\PCR_ThCtl.h"\
	

".\Release\alloc.obj" : $(SOURCE) $(DEP_CPP_ALLOC) "$(INTDIR)"

".\Release\alloc.sbr" : $(SOURCE) $(DEP_CPP_ALLOC) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "gc - Win32 Debug"

DEP_CPP_ALLOC=\
	".\include\private\gcconfig.h"\
	".\include\gc.h"\
	".\include\private\gc_hdrs.h"\
	".\include\private\gc_priv.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_ALLOC=\
	".\th\PCR_Th.h"\
	".\th\PCR_ThCrSec.h"\
	".\th\PCR_ThCtl.h"\
	

".\Debug\alloc.obj" : $(SOURCE) $(DEP_CPP_ALLOC) "$(INTDIR)"

".\Debug\alloc.sbr" : $(SOURCE) $(DEP_CPP_ALLOC) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\allchblk.c

!IF  "$(CFG)" == "gc - Win32 Release"

DEP_CPP_ALLCH=\
	".\include\private\gcconfig.h"\
	".\include\gc.h"\
	".\include\private\gc_hdrs.h"\
	".\include\private\gc_priv.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_ALLCH=\
	".\th\PCR_Th.h"\
	".\th\PCR_ThCrSec.h"\
	".\th\PCR_ThCtl.h"\
	

".\Release\allchblk.obj" : $(SOURCE) $(DEP_CPP_ALLCH) "$(INTDIR)"

".\Release\allchblk.sbr" : $(SOURCE) $(DEP_CPP_ALLCH) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "gc - Win32 Debug"

DEP_CPP_ALLCH=\
	".\include\private\gcconfig.h"\
	".\include\gc.h"\
	".\include\private\gc_hdrs.h"\
	".\include\private\gc_priv.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_ALLCH=\
	".\th\PCR_Th.h"\
	".\th\PCR_ThCrSec.h"\
	".\th\PCR_ThCtl.h"\
	

".\Debug\allchblk.obj" : $(SOURCE) $(DEP_CPP_ALLCH) "$(INTDIR)"

".\Debug\allchblk.sbr" : $(SOURCE) $(DEP_CPP_ALLCH) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\stubborn.c

!IF  "$(CFG)" == "gc - Win32 Release"

DEP_CPP_STUBB=\
	".\include\private\gcconfig.h"\
	".\include\gc.h"\
	".\include\private\gc_hdrs.h"\
	".\include\private\gc_priv.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_STUBB=\
	".\th\PCR_Th.h"\
	".\th\PCR_ThCrSec.h"\
	".\th\PCR_ThCtl.h"\
	

".\Release\stubborn.obj" : $(SOURCE) $(DEP_CPP_STUBB) "$(INTDIR)"

".\Release\stubborn.sbr" : $(SOURCE) $(DEP_CPP_STUBB) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "gc - Win32 Debug"

DEP_CPP_STUBB=\
	".\include\private\gcconfig.h"\
	".\include\gc.h"\
	".\include\private\gc_hdrs.h"\
	".\include\private\gc_priv.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_STUBB=\
	".\th\PCR_Th.h"\
	".\th\PCR_ThCrSec.h"\
	".\th\PCR_ThCtl.h"\
	

".\Debug\stubborn.obj" : $(SOURCE) $(DEP_CPP_STUBB) "$(INTDIR)"

".\Debug\stubborn.sbr" : $(SOURCE) $(DEP_CPP_STUBB) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\obj_map.c

!IF  "$(CFG)" == "gc - Win32 Release"

DEP_CPP_OBJ_M=\
	".\include\private\gcconfig.h"\
	".\include\gc.h"\
	".\include\private\gc_hdrs.h"\
	".\include\private\gc_priv.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_OBJ_M=\
	".\th\PCR_Th.h"\
	".\th\PCR_ThCrSec.h"\
	".\th\PCR_ThCtl.h"\
	

".\Release\obj_map.obj" : $(SOURCE) $(DEP_CPP_OBJ_M) "$(INTDIR)"

".\Release\obj_map.sbr" : $(SOURCE) $(DEP_CPP_OBJ_M) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "gc - Win32 Debug"

DEP_CPP_OBJ_M=\
	".\include\private\gcconfig.h"\
	".\include\gc.h"\
	".\include\private\gc_hdrs.h"\
	".\include\private\gc_priv.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_OBJ_M=\
	".\th\PCR_Th.h"\
	".\th\PCR_ThCrSec.h"\
	".\th\PCR_ThCtl.h"\
	

".\Debug\obj_map.obj" : $(SOURCE) $(DEP_CPP_OBJ_M) "$(INTDIR)"

".\Debug\obj_map.sbr" : $(SOURCE) $(DEP_CPP_OBJ_M) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\new_hblk.c

!IF  "$(CFG)" == "gc - Win32 Release"

DEP_CPP_NEW_H=\
	".\include\private\gcconfig.h"\
	".\include\gc.h"\
	".\include\private\gc_hdrs.h"\
	".\include\private\gc_priv.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_NEW_H=\
	".\th\PCR_Th.h"\
	".\th\PCR_ThCrSec.h"\
	".\th\PCR_ThCtl.h"\
	

".\Release\new_hblk.obj" : $(SOURCE) $(DEP_CPP_NEW_H) "$(INTDIR)"

".\Release\new_hblk.sbr" : $(SOURCE) $(DEP_CPP_NEW_H) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "gc - Win32 Debug"

DEP_CPP_NEW_H=\
	".\include\private\gcconfig.h"\
	".\include\gc.h"\
	".\include\private\gc_hdrs.h"\
	".\include\private\gc_priv.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_NEW_H=\
	".\th\PCR_Th.h"\
	".\th\PCR_ThCrSec.h"\
	".\th\PCR_ThCtl.h"\
	

".\Debug\new_hblk.obj" : $(SOURCE) $(DEP_CPP_NEW_H) "$(INTDIR)"

".\Debug\new_hblk.sbr" : $(SOURCE) $(DEP_CPP_NEW_H) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\mark.c

!IF  "$(CFG)" == "gc - Win32 Release"

DEP_CPP_MARK_C=\
	".\include\private\gcconfig.h"\
	".\include\gc.h"\
	".\include\private\gc_hdrs.h"\
	".\include\private\gc_pmark.h"\
	".\include\gc_mark.h"\
	".\include\private\gc_priv.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_MARK_C=\
	".\th\PCR_Th.h"\
	".\th\PCR_ThCrSec.h"\
	".\th\PCR_ThCtl.h"\
	

".\Release\mark.obj" : $(SOURCE) $(DEP_CPP_MARK_C) "$(INTDIR)"

".\Release\mark.sbr" : $(SOURCE) $(DEP_CPP_MARK_C) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "gc - Win32 Debug"

DEP_CPP_MARK_C=\
	".\include\private\gcconfig.h"\
	".\include\gc.h"\
	".\include\private\gc_hdrs.h"\
	".\include\private\gc_pmark.h"\
	".\include\gc_mark.h"\
	".\include\private\gc_priv.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_MARK_C=\
	".\th\PCR_Th.h"\
	".\th\PCR_ThCrSec.h"\
	".\th\PCR_ThCtl.h"\
	

".\Debug\mark.obj" : $(SOURCE) $(DEP_CPP_MARK_C) "$(INTDIR)"

".\Debug\mark.sbr" : $(SOURCE) $(DEP_CPP_MARK_C) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\malloc.c

!IF  "$(CFG)" == "gc - Win32 Release"

DEP_CPP_MALLO=\
	".\include\private\gcconfig.h"\
	".\include\gc.h"\
	".\include\private\gc_hdrs.h"\
	".\include\private\gc_priv.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_MALLO=\
	".\th\PCR_Th.h"\
	".\th\PCR_ThCrSec.h"\
	".\th\PCR_ThCtl.h"\
	

".\Release\malloc.obj" : $(SOURCE) $(DEP_CPP_MALLO) "$(INTDIR)"

".\Release\malloc.sbr" : $(SOURCE) $(DEP_CPP_MALLO) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "gc - Win32 Debug"

DEP_CPP_MALLO=\
	".\include\private\gcconfig.h"\
	".\include\gc.h"\
	".\include\private\gc_hdrs.h"\
	".\include\private\gc_priv.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_MALLO=\
	".\th\PCR_Th.h"\
	".\th\PCR_ThCrSec.h"\
	".\th\PCR_ThCtl.h"\
	

".\Debug\malloc.obj" : $(SOURCE) $(DEP_CPP_MALLO) "$(INTDIR)"

".\Debug\malloc.sbr" : $(SOURCE) $(DEP_CPP_MALLO) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\mallocx.c

!IF  "$(CFG)" == "gc - Win32 Release"

DEP_CPP_MALLX=\
	".\include\private\gcconfig.h"\
	".\include\gc.h"\
	".\include\private\gc_hdrs.h"\
	".\include\private\gc_priv.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_MALLX=\
	".\th\PCR_Th.h"\
	".\th\PCR_ThCrSec.h"\
	".\th\PCR_ThCtl.h"\
	

".\Release\mallocx.obj" : $(SOURCE) $(DEP_CPP_MALLX) "$(INTDIR)"

".\Release\mallocx.sbr" : $(SOURCE) $(DEP_CPP_MALLX) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "gc - Win32 Debug"

DEP_CPP_MALLX=\
	".\include\private\gcconfig.h"\
	".\include\gc.h"\
	".\include\private\gc_hdrs.h"\
	".\include\private\gc_priv.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_MALLX=\
	".\th\PCR_Th.h"\
	".\th\PCR_ThCrSec.h"\
	".\th\PCR_ThCtl.h"\
	

".\Debug\mallocx.obj" : $(SOURCE) $(DEP_CPP_MALLX) "$(INTDIR)"

".\Debug\mallocx.sbr" : $(SOURCE) $(DEP_CPP_MALLX) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\finalize.c

!IF  "$(CFG)" == "gc - Win32 Release"

DEP_CPP_FINAL=\
	".\include\private\gcconfig.h"\
	".\include\gc.h"\
	".\include\private\gc_hdrs.h"\
	".\include\private\gc_pmark.h"\
	".\include\gc_mark.h"\
	".\include\private\gc_priv.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_FINAL=\
	".\th\PCR_Th.h"\
	".\th\PCR_ThCrSec.h"\
	".\th\PCR_ThCtl.h"\
	

".\Release\finalize.obj" : $(SOURCE) $(DEP_CPP_FINAL) "$(INTDIR)"

".\Release\finalize.sbr" : $(SOURCE) $(DEP_CPP_FINAL) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "gc - Win32 Debug"

DEP_CPP_FINAL=\
	".\include\private\gcconfig.h"\
	".\include\gc.h"\
	".\include\private\gc_hdrs.h"\
	".\include\private\gc_pmark.h"\
	".\include\gc_mark.h"\
	".\include\private\gc_priv.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_FINAL=\
	".\th\PCR_Th.h"\
	".\th\PCR_ThCrSec.h"\
	".\th\PCR_ThCtl.h"\
	

".\Debug\finalize.obj" : $(SOURCE) $(DEP_CPP_FINAL) "$(INTDIR)"

".\Debug\finalize.sbr" : $(SOURCE) $(DEP_CPP_FINAL) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\dbg_mlc.c

!IF  "$(CFG)" == "gc - Win32 Release"

DEP_CPP_DBG_M=\
	".\include\private\gcconfig.h"\
	".\include\gc.h"\
	".\include\private\gc_hdrs.h"\
	".\include\private\gc_priv.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_DBG_M=\
	".\th\PCR_Th.h"\
	".\th\PCR_ThCrSec.h"\
	".\th\PCR_ThCtl.h"\
	

".\Release\dbg_mlc.obj" : $(SOURCE) $(DEP_CPP_DBG_M) "$(INTDIR)"

".\Release\dbg_mlc.sbr" : $(SOURCE) $(DEP_CPP_DBG_M) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "gc - Win32 Debug"

DEP_CPP_DBG_M=\
	".\include\private\gcconfig.h"\
	".\include\gc.h"\
	".\include\private\gc_hdrs.h"\
	".\include\private\gc_priv.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_DBG_M=\
	".\th\PCR_Th.h"\
	".\th\PCR_ThCrSec.h"\
	".\th\PCR_ThCtl.h"\
	

".\Debug\dbg_mlc.obj" : $(SOURCE) $(DEP_CPP_DBG_M) "$(INTDIR)"

".\Debug\dbg_mlc.sbr" : $(SOURCE) $(DEP_CPP_DBG_M) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\blacklst.c

!IF  "$(CFG)" == "gc - Win32 Release"

DEP_CPP_BLACK=\
	".\include\private\gcconfig.h"\
	".\include\gc.h"\
	".\include\private\gc_hdrs.h"\
	".\include\private\gc_priv.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_BLACK=\
	".\th\PCR_Th.h"\
	".\th\PCR_ThCrSec.h"\
	".\th\PCR_ThCtl.h"\
	

".\Release\blacklst.obj" : $(SOURCE) $(DEP_CPP_BLACK) "$(INTDIR)"

".\Release\blacklst.sbr" : $(SOURCE) $(DEP_CPP_BLACK) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "gc - Win32 Debug"

DEP_CPP_BLACK=\
	".\include\private\gcconfig.h"\
	".\include\gc.h"\
	".\include\private\gc_hdrs.h"\
	".\include\private\gc_priv.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_BLACK=\
	".\th\PCR_Th.h"\
	".\th\PCR_ThCrSec.h"\
	".\th\PCR_ThCtl.h"\
	

".\Debug\blacklst.obj" : $(SOURCE) $(DEP_CPP_BLACK) "$(INTDIR)"

".\Debug\blacklst.sbr" : $(SOURCE) $(DEP_CPP_BLACK) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\typd_mlc.c

!IF  "$(CFG)" == "gc - Win32 Release"

DEP_CPP_TYPD_=\
	".\include\private\gcconfig.h"\
	".\include\gc.h"\
	".\include\private\gc_hdrs.h"\
	".\include\private\gc_pmark.h"\
	".\include\gc_mark.h"\
	".\include\private\gc_priv.h"\
	".\include\gc_typed.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_TYPD_=\
	".\th\PCR_Th.h"\
	".\th\PCR_ThCrSec.h"\
	".\th\PCR_ThCtl.h"\
	

".\Release\typd_mlc.obj" : $(SOURCE) $(DEP_CPP_TYPD_) "$(INTDIR)"

".\Release\typd_mlc.sbr" : $(SOURCE) $(DEP_CPP_TYPD_) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "gc - Win32 Debug"

DEP_CPP_TYPD_=\
	".\include\private\gcconfig.h"\
	".\include\gc.h"\
	".\include\private\gc_hdrs.h"\
	".\include\private\gc_pmark.h"\
	".\include\gc_mark.h"\
	".\include\private\gc_priv.h"\
	".\include\gc_typed.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_TYPD_=\
	".\th\PCR_Th.h"\
	".\th\PCR_ThCrSec.h"\
	".\th\PCR_ThCtl.h"\
	

".\Debug\typd_mlc.obj" : $(SOURCE) $(DEP_CPP_TYPD_) "$(INTDIR)"

".\Debug\typd_mlc.sbr" : $(SOURCE) $(DEP_CPP_TYPD_) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\ptr_chck.c

!IF  "$(CFG)" == "gc - Win32 Release"

DEP_CPP_PTR_C=\
	".\include\private\gcconfig.h"\
	".\include\gc.h"\
	".\include\private\gc_hdrs.h"\
	".\include\private\gc_pmark.h"\
	".\include\gc_mark.h"\
	".\include\private\gc_priv.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_PTR_C=\
	".\th\PCR_Th.h"\
	".\th\PCR_ThCrSec.h"\
	".\th\PCR_ThCtl.h"\
	

".\Release\ptr_chck.obj" : $(SOURCE) $(DEP_CPP_PTR_C) "$(INTDIR)"

".\Release\ptr_chck.sbr" : $(SOURCE) $(DEP_CPP_PTR_C) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "gc - Win32 Debug"

DEP_CPP_PTR_C=\
	".\include\private\gcconfig.h"\
	".\include\gc.h"\
	".\include\private\gc_hdrs.h"\
	".\include\private\gc_pmark.h"\
	".\include\gc_mark.h"\
	".\include\private\gc_priv.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_PTR_C=\
	".\th\PCR_Th.h"\
	".\th\PCR_ThCrSec.h"\
	".\th\PCR_ThCtl.h"\
	

".\Debug\ptr_chck.obj" : $(SOURCE) $(DEP_CPP_PTR_C) "$(INTDIR)"

".\Debug\ptr_chck.sbr" : $(SOURCE) $(DEP_CPP_PTR_C) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\dyn_load.c

!IF  "$(CFG)" == "gc - Win32 Release"

DEP_CPP_DYN_L=\
	".\include\private\gcconfig.h"\
	".\include\gc.h"\
	".\include\private\gc_hdrs.h"\
	".\include\private\gc_priv.h"\
	{$(INCLUDE)}"\sys\STAT.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_DYN_L=\
	".\il\PCR_IL.h"\
	".\mm\PCR_MM.h"\
	".\th\PCR_Th.h"\
	".\th\PCR_ThCrSec.h"\
	".\th\PCR_ThCtl.h"\
	

".\Release\dyn_load.obj" : $(SOURCE) $(DEP_CPP_DYN_L) "$(INTDIR)"

".\Release\dyn_load.sbr" : $(SOURCE) $(DEP_CPP_DYN_L) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "gc - Win32 Debug"

DEP_CPP_DYN_L=\
	".\include\private\gcconfig.h"\
	".\include\gc.h"\
	".\include\private\gc_hdrs.h"\
	".\include\private\gc_priv.h"\
	{$(INCLUDE)}"\sys\STAT.H"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_DYN_L=\
	".\il\PCR_IL.h"\
	".\mm\PCR_MM.h"\
	".\th\PCR_Th.h"\
	".\th\PCR_ThCrSec.h"\
	".\th\PCR_ThCtl.h"\
	

".\Debug\dyn_load.obj" : $(SOURCE) $(DEP_CPP_DYN_L) "$(INTDIR)"

".\Debug\dyn_load.sbr" : $(SOURCE) $(DEP_CPP_DYN_L) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\win32_threads.c

!IF  "$(CFG)" == "gc - Win32 Release"

DEP_CPP_WIN32=\
	".\include\private\gcconfig.h"\
	".\include\gc.h"\
	".\include\private\gc_hdrs.h"\
	".\include\private\gc_priv.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_WIN32=\
	".\th\PCR_Th.h"\
	".\th\PCR_ThCrSec.h"\
	".\th\PCR_ThCtl.h"\
	

".\Release\win32_threads.obj" : $(SOURCE) $(DEP_CPP_WIN32) "$(INTDIR)"

".\Release\win32_threads.sbr" : $(SOURCE) $(DEP_CPP_WIN32) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "gc - Win32 Debug"

DEP_CPP_WIN32=\
	".\include\private\gcconfig.h"\
	".\include\gc.h"\
	".\include\private\gc_hdrs.h"\
	".\include\private\gc_priv.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_WIN32=\
	".\th\PCR_Th.h"\
	".\th\PCR_ThCrSec.h"\
	".\th\PCR_ThCtl.h"\
	

".\Debug\win32_threads.obj" : $(SOURCE) $(DEP_CPP_WIN32) "$(INTDIR)"

".\Debug\win32_threads.sbr" : $(SOURCE) $(DEP_CPP_WIN32) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\msvc_dbg.c

!IF  "$(CFG)" == "gc - Win32 Release"

DEP_CPP_WIN32=\
	".\include\private\gcconfig.h"\
	".\include\gc.h"\
	".\include\private\gc_hdrs.h"\
	".\include\private\gc_priv.h"\
	".\include\private\msvc_dbg.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_WIN32=\
	".\th\PCR_Th.h"\
	".\th\PCR_ThCrSec.h"\
	".\th\PCR_ThCtl.h"\
	

".\Release\msvc_dbg.obj" : $(SOURCE) $(DEP_CPP_WIN32) "$(INTDIR)"

".\Release\msvc_dbg.sbr" : $(SOURCE) $(DEP_CPP_WIN32) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "gc - Win32 Debug"

DEP_CPP_WIN32=\
	".\include\private\gcconfig.h"\
	".\include\gc.h"\
	".\include\private\gc_hdrs.h"\
	".\include\private\gc_priv.h"\
	".\include\private\msvc_dbg.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_WIN32=\
	".\th\PCR_Th.h"\
	".\th\PCR_ThCrSec.h"\
	".\th\PCR_ThCtl.h"\
	

".\Debug\msvc_dbg.obj" : $(SOURCE) $(DEP_CPP_WIN32) "$(INTDIR)"

".\Debug\msvc_dbg.sbr" : $(SOURCE) $(DEP_CPP_WIN32) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\checksums.c

!IF  "$(CFG)" == "gc - Win32 Release"

DEP_CPP_CHECK=\
	".\include\private\gcconfig.h"\
	".\include\gc.h"\
	".\include\private\gc_hdrs.h"\
	".\include\private\gc_priv.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_CHECK=\
	".\th\PCR_Th.h"\
	".\th\PCR_ThCrSec.h"\
	".\th\PCR_ThCtl.h"\
	

".\Release\checksums.obj" : $(SOURCE) $(DEP_CPP_CHECK) "$(INTDIR)"

".\Release\checksums.sbr" : $(SOURCE) $(DEP_CPP_CHECK) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "gc - Win32 Debug"

DEP_CPP_CHECK=\
	".\include\private\gcconfig.h"\
	".\include\gc.h"\
	".\include\private\gc_hdrs.h"\
	".\include\private\gc_priv.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_CHECK=\
	".\th\PCR_Th.h"\
	".\th\PCR_ThCrSec.h"\
	".\th\PCR_ThCtl.h"\
	

".\Debug\checksums.obj" : $(SOURCE) $(DEP_CPP_CHECK) "$(INTDIR)"

".\Debug\checksums.sbr" : $(SOURCE) $(DEP_CPP_CHECK) "$(INTDIR)"


!ENDIF 

# End Source File
# End Target
################################################################################
# Begin Target

# Name "gctest - Win32 Release"
# Name "gctest - Win32 Debug"

!IF  "$(CFG)" == "gctest - Win32 Release"

!ELSEIF  "$(CFG)" == "gctest - Win32 Debug"

!ENDIF 

################################################################################
# Begin Project Dependency

# Project_Dep_Name "gc"

!IF  "$(CFG)" == "gctest - Win32 Release"

"gc - Win32 Release" : 
   $(MAKE) /$(MAKEFLAGS) /F ".\gc.mak" CFG="gc - Win32 Release" 

!ELSEIF  "$(CFG)" == "gctest - Win32 Debug"

"gc - Win32 Debug" : 
   $(MAKE) /$(MAKEFLAGS) /F ".\gc.mak" CFG="gc - Win32 Debug" 

!ENDIF 

# End Project Dependency
################################################################################
# Begin Source File

SOURCE=.\tests\test.c
DEP_CPP_TEST_=\
	".\include\private\gcconfig.h"\
	".\include\gc.h"\
	".\include\private\gc_hdrs.h"\
	".\include\private\gc_priv.h"\
	".\include\gc_typed.h"\
	{$(INCLUDE)}"\sys\TYPES.H"\
	
NODEP_CPP_TEST_=\
	".\th\PCR_Th.h"\
	".\th\PCR_ThCrSec.h"\
	".\th\PCR_ThCtl.h"\
	

!IF  "$(CFG)" == "gctest - Win32 Release"


".\gctest\Release\test.obj" : $(SOURCE) $(DEP_CPP_TEST_) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "gctest - Win32 Debug"


".\gctest\Debug\test.obj" : $(SOURCE) $(DEP_CPP_TEST_) "$(INTDIR)"

".\gctest\Debug\test.sbr" : $(SOURCE) $(DEP_CPP_TEST_) "$(INTDIR)"


!ENDIF 

# End Source File
# End Target
################################################################################
# Begin Target

# Name "cord - Win32 Release"
# Name "cord - Win32 Debug"

!IF  "$(CFG)" == "cord - Win32 Release"

!ELSEIF  "$(CFG)" == "cord - Win32 Debug"

!ENDIF 

################################################################################
# Begin Project Dependency

# Project_Dep_Name "gc"

!IF  "$(CFG)" == "cord - Win32 Release"

"gc - Win32 Release" : 
   $(MAKE) /$(MAKEFLAGS) /F ".\gc.mak" CFG="gc - Win32 Release" 

!ELSEIF  "$(CFG)" == "cord - Win32 Debug"

"gc - Win32 Debug" : 
   $(MAKE) /$(MAKEFLAGS) /F ".\gc.mak" CFG="gc - Win32 Debug" 

!ENDIF 

# End Project Dependency
################################################################################
# Begin Source File

SOURCE=.\cord\de_win.c
DEP_CPP_DE_WI=\
	".\include\cord.h"\
	".\cord\de_cmds.h"\
	".\cord\de_win.h"\
	".\include\private\cord_pos.h"\
	
NODEP_CPP_DE_WI=\
	".\include\gc.h"\
	

!IF  "$(CFG)" == "cord - Win32 Release"


".\cord\Release\de_win.obj" : $(SOURCE) $(DEP_CPP_DE_WI) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "cord - Win32 Debug"


".\cord\Debug\de_win.obj" : $(SOURCE) $(DEP_CPP_DE_WI) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\cord\de.c
DEP_CPP_DE_C2e=\
	".\include\cord.h"\
	".\cord\de_cmds.h"\
	".\cord\de_win.h"\
	".\include\private\cord_pos.h"\
	
NODEP_CPP_DE_C2e=\
	".\include\gc.h"\
	

!IF  "$(CFG)" == "cord - Win32 Release"


".\cord\Release\de.obj" : $(SOURCE) $(DEP_CPP_DE_C2e) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "cord - Win32 Debug"


".\cord\Debug\de.obj" : $(SOURCE) $(DEP_CPP_DE_C2e) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\cord\cordxtra.c
DEP_CPP_CORDX=\
	".\include\cord.h"\
	".\include\ec.h"\
	".\include\private\cord_pos.h"\
	
NODEP_CPP_CORDX=\
	".\include\gc.h"\
	

!IF  "$(CFG)" == "cord - Win32 Release"


".\cord\Release\cordxtra.obj" : $(SOURCE) $(DEP_CPP_CORDX) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "cord - Win32 Debug"


".\cord\Debug\cordxtra.obj" : $(SOURCE) $(DEP_CPP_CORDX) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\cord\cordbscs.c
DEP_CPP_CORDB=\
	".\include\cord.h"\
	".\include\private\cord_pos.h"\
	
NODEP_CPP_CORDB=\
	".\include\gc.h"\
	

!IF  "$(CFG)" == "cord - Win32 Release"


".\cord\Release\cordbscs.obj" : $(SOURCE) $(DEP_CPP_CORDB) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "cord - Win32 Debug"


".\cord\Debug\cordbscs.obj" : $(SOURCE) $(DEP_CPP_CORDB) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\cord\de_win.RC

!IF  "$(CFG)" == "cord - Win32 Release"


".\cord\Release\de_win.res" : $(SOURCE) "$(INTDIR)"
   $(RSC) /l 0x809 /fo"$(INTDIR)/de_win.res" /i "cord" /d "NDEBUG" $(SOURCE)


!ELSEIF  "$(CFG)" == "cord - Win32 Debug"


".\cord\Debug\de_win.res" : $(SOURCE) "$(INTDIR)"
   $(RSC) /l 0x809 /fo"$(INTDIR)/de_win.res" /i "cord" /d "_DEBUG" $(SOURCE)


!ENDIF 

# End Source File
# End Target
# End Project
################################################################################
