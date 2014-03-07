#**********************************************************************
#* Copyright (C) 1999-2010, International Business Machines Corporation
#* and others.  All Rights Reserved.
#**********************************************************************
# nmake file for creating data files on win32
# invoke with
# nmake /f makedata.mak icumake=$(ProjectDir)
#
#	12/10/1999	weiv	Created

##############################################################################
# Keep the following in sync with the version - see common/unicode/uversion.h
U_ICUDATA_NAME=icudt46
##############################################################################
U_ICUDATA_ENDIAN_SUFFIX=l
UNICODE_VERSION=6.0
ICU_LIB_TARGET=$(DLL_OUTPUT)\$(U_ICUDATA_NAME).dll

#  ICUMAKE
#     Must be provided by whoever runs this makefile.
#     Is the directory containing this file (makedata.mak)
#     Is the directory into which most data is built (prior to packaging)
#     Is icu\source\data\
#
!IF "$(ICUMAKE)"==""
!ERROR Can't find ICUMAKE (ICU Data Make dir, should point to icu\source\data\ )!
!ENDIF
!MESSAGE ICU data make path is $(ICUMAKE)

# Suffixes for data files
.SUFFIXES : .nrm .icu .ucm .cnv .dll .dat .res .txt .c

ICUOUT=$(ICUMAKE)\out

#  the prefix "icudt21_" for use in filenames
ICUPKG=$(U_ICUDATA_NAME)$(U_ICUDATA_ENDIAN_SUFFIX)

# need to nuke \\ for .NET...
ICUOUT=$(ICUOUT:\\=\)

ICUBLD=$(ICUOUT)\build
ICUBLD_PKG=$(ICUBLD)\$(ICUPKG)
ICUTMP=$(ICUOUT)\tmp

#  ICUP
#     The root of the ICU source directory tree
#
ICUP=$(ICUMAKE)\..\..
ICUP=$(ICUP:\source\data\..\..=)
# In case the first one didn't do it, try this one.  .NET would do the second one.
ICUP=$(ICUP:\source\data\\..\..=)
!MESSAGE ICU root path is $(ICUP)


#  ICUSRCDATA
#       The data directory in source
#
ICUSRCDATA=$(ICUP)\source\data
ICUSRCDATA_RELATIVE_PATH=..\..\..

#  ICUUCM
#       The directory that contains ucmcore.mk files along with *.ucm files
#
ICUUCM=mappings

#  ICULOC
#       The directory that contains resfiles.mk files along with *.txt locale data files
#
ICULOC=locales

#  ICUCOL
#       The directory that contains colfiles.mk files along with *.txt collation data files
#
ICUCOL=coll

#  ICURBNF
#       The directory that contains rbnffiles.mk files along with *.txt RBNF data files
#
ICURBNF=rbnf

#  ICUTRNS
#       The directory that contains trfiles.mk files along with *.txt transliterator files
#
ICUTRNS=translit

#  ICUBRK
#       The directory that contains resfiles.mk files along with *.txt break iterator files
#
ICUBRK=brkitr

#  ICUUNIDATA
#       The directory that contains Unicode data files
#
ICUUNIDATA=$(ICUP)\source\data\unidata


#  ICUMISC
#       The directory that contains miscfiles.mk along with files that are miscelleneous data
#
ICUMISC=$(ICUP)\source\data\misc
ICUMISC2=misc

#  ICUSPREP
#       The directory that contains sprepfiles.mk files along with *.txt stringprep files
#
ICUSPREP=sprep

#
#  ICUDATA
#     The source directory.  Contains the source files for the common data to be built.
#     WARNING:  NOT THE SAME AS ICU_DATA environment variable.  Confusing.
ICUDATA=$(ICUP)\source\data

#
#  DLL_OUTPUT
#      Destination directory for the common data DLL file.
#      This is the same place that all of the other ICU DLLs go (the code-containing DLLs)
#      The lib file for the data DLL goes in $(DLL_OUTPUT)/../lib/
#
!IF "$(CFG)" == "x64\Release" || "$(CFG)" == "x64\Debug"
DLL_OUTPUT=$(ICUP)\bin64
!ELSE
DLL_OUTPUT=$(ICUP)\bin
!ENDIF

#
#  TESTDATA
#     The source directory for data needed for test programs.
TESTDATA=$(ICUP)\source\test\testdata

#
#   TESTDATAOUT
#      The destination directory for the built test data .dat file
TESTDATAOUT=$(ICUP)\source\test\testdata\out

#
#   TESTDATABLD
#		The build directory for test data intermidiate files
#		(Tests are NOT run from this makefile,
#         only the data is put in place.)
TESTDATABLD=$(ICUP)\source\test\testdata\out\build

#
#   ICUTOOLS
#       Directory under which all of the ICU data building tools live.
#
ICUTOOLS=$(ICUP)\source\tools

# The current ICU tools need to be in the path first.
!IF "$(CFG)" == "x64\Release" || "$(CFG)" == "x64\Debug"
PATH = $(ICUP)\bin64;$(PATH)
ICUPBIN=$(ICUP)\bin64
!ELSE
PATH = $(ICUP)\bin;$(PATH)
ICUPBIN=$(ICUP)\bin
!ENDIF


# This variable can be overridden to "-m static" by the project settings,
# if you want a static data library.
!IF "$(ICU_PACKAGE_MODE)"==""
ICU_PACKAGE_MODE=-m dll
!ENDIF

# If this archive exists, build from that
# instead of building everything from scratch.
ICUDATA_SOURCE_ARCHIVE=$(ICUSRCDATA)\in\$(ICUPKG).dat
!IF !EXISTS("$(ICUDATA_SOURCE_ARCHIVE)")
# Does a big endian version exist either?
ICUDATA_ARCHIVE=$(ICUSRCDATA)\in\$(U_ICUDATA_NAME)b.dat
!IF EXISTS("$(ICUDATA_ARCHIVE)")
ICUDATA_SOURCE_ARCHIVE=$(ICUTMP)\$(ICUPKG).dat
!ELSE
# Nothing was usable for input
!UNDEF ICUDATA_SOURCE_ARCHIVE
!ENDIF
!ENDIF

!IFDEF ICUDATA_SOURCE_ARCHIVE
!MESSAGE ICU data source archive is $(ICUDATA_SOURCE_ARCHIVE)
!ELSE
# We're including a list of .ucm files.
# There are several lists, they are all optional.

# Always build the mapping files for the EBCDIC fallback codepages
# They are necessary on EBCDIC machines, and
# the following logic is much easier if UCM_SOURCE is never empty.
# (They are small.)
UCM_SOURCE=ibm-37_P100-1995.ucm ibm-1047_P100-1995.ucm

!IF EXISTS("$(ICUSRCDATA)\$(ICUUCM)\ucmcore.mk")
!INCLUDE "$(ICUSRCDATA)\$(ICUUCM)\ucmcore.mk"
UCM_SOURCE=$(UCM_SOURCE) $(UCM_SOURCE_CORE)
!ELSE
!MESSAGE Warning: cannot find "ucmcore.mk". Not building core MIME/Unix/Windows converter files.
!ENDIF

!IF EXISTS("$(ICUSRCDATA)\$(ICUUCM)\ucmfiles.mk")
!INCLUDE "$(ICUSRCDATA)\$(ICUUCM)\ucmfiles.mk"
UCM_SOURCE=$(UCM_SOURCE) $(UCM_SOURCE_FILES)
!ELSE
!MESSAGE Warning: cannot find "ucmfiles.mk". Not building many converter files.
!ENDIF

!IF EXISTS("$(ICUSRCDATA)\$(ICUUCM)\ucmebcdic.mk")
!INCLUDE "$(ICUSRCDATA)\$(ICUUCM)\ucmebcdic.mk"
UCM_SOURCE=$(UCM_SOURCE) $(UCM_SOURCE_EBCDIC)
!IFDEF UCM_SOURCE_EBCDIC_IGNORE_SISO
BUILD_SPECIAL_CNV_FILES=YES
UCM_SOURCE_SPECIAL=$(UCM_SOURCE_EBCDIC_IGNORE_SISO)
!ELSE
!UNDEF BUILD_SPECIAL_CNV_FILES
!ENDIF
!ELSE
!MESSAGE Warning: cannot find "ucmebcdic.mk". Not building EBCDIC converter files.
!ENDIF

!IF EXISTS("$(ICUSRCDATA)\$(ICUUCM)\ucmlocal.mk")
!INCLUDE "$(ICUSRCDATA)\$(ICUUCM)\ucmlocal.mk"
UCM_SOURCE=$(UCM_SOURCE) $(UCM_SOURCE_LOCAL)
!IFDEF UCM_SOURCE_EBCDIC_IGNORE_SISO_LOCAL
UCM_SOURCE_SPECIAL=$(UCM_SOURCE_SPECIAL) $(UCM_SOURCE_EBCDIC_IGNORE_SISO_LOCAL)
BUILD_SPECIAL_CNV_FILES=YES
!ENDIF
!ELSE
!MESSAGE Information: cannot find "ucmlocal.mk". Not building user-additional converter files.
!ENDIF

CNV_FILES=$(UCM_SOURCE:.ucm=.cnv)
!IFDEF BUILD_SPECIAL_CNV_FILES
CNV_FILES_SPECIAL=$(UCM_SOURCE_SPECIAL:.ucm=.cnv)
!ENDIF

!IF EXISTS("$(ICUSRCDATA)\$(ICUBRK)\brkfiles.mk")
!INCLUDE "$(ICUSRCDATA)\$(ICUBRK)\brkfiles.mk"
!IF EXISTS("$(ICUSRCDATA)\$(ICUBRK)\brklocal.mk")
!INCLUDE "$(ICUSRCDATA)\$(ICUBRK)\brklocal.mk"
BRK_SOURCE=$(BRK_SOURCE) $(BRK_SOURCE_LOCAL)
BRK_CTD_SOURCE=$(BRK_CTD_SOURCE) $(BRK_CTD_SOURCE_LOCAL)
BRK_RES_SOURCE=$(BRK_RES_SOURCE) $(BRK_RES_SOURCE_LOCAL)
!ELSE
!MESSAGE Information: cannot find "brklocal.mk". Not building user-additional break iterator files.
!ENDIF
!ELSE
!MESSAGE Warning: cannot find "brkfiles.mk"
!ENDIF

#
#  Break iterator data files.
#
BRK_FILES=$(ICUBRK)\$(BRK_SOURCE:.txt =.brk brkitr\)
BRK_FILES=$(BRK_FILES:.txt=.brk)
BRK_FILES=$(BRK_FILES:brkitr\ =brkitr\)

!IFDEF BRK_CTD_SOURCE
BRK_CTD_FILES = $(ICUBRK)\$(BRK_CTD_SOURCE:.txt =.ctd brkitr\)
BRK_CTD_FILES = $(BRK_CTD_FILES:.txt=.ctd)
BRK_CTD_FILES = $(BRK_CTD_FILES:brkitr\ =)
!ENDIF

!IFDEF BRK_RES_SOURCE
BRK_RES_FILES = $(BRK_RES_SOURCE:.txt =.res brkitr\)
BRK_RES_FILES = $(BRK_RES_FILES:.txt=.res)
BRK_RES_FILES = $(ICUBRK)\root.res $(ICUBRK)\$(BRK_RES_FILES:brkitr\ =)
ALL_RES = $(ALL_RES) $(ICUBRK)\res_index.res
!ENDIF

# Read list of locale resource bundle files
!IF EXISTS("$(ICUSRCDATA)\$(ICULOC)\resfiles.mk")
!INCLUDE "$(ICUSRCDATA)\$(ICULOC)\resfiles.mk"
!IF EXISTS("$(ICUSRCDATA)\$(ICULOC)\reslocal.mk")
!INCLUDE "$(ICUSRCDATA)\$(ICULOC)\reslocal.mk"
GENRB_SOURCE=$(GENRB_SOURCE) $(GENRB_SOURCE_LOCAL)
!ELSE
!MESSAGE Information: cannot find "reslocal.mk". Not building user-additional resource bundle files.
!ENDIF
!ELSE
!MESSAGE Warning: cannot find "resfiles.mk"
!ENDIF

!IFDEF GENRB_SOURCE
RB_FILES = root.res pool.res $(GENRB_ALIAS_SOURCE:.txt=.res) $(GENRB_ALIAS_SOURCE_LOCAL:.txt=.res) $(GENRB_SOURCE:.txt=.res)
ALL_RES = $(ALL_RES) res_index.res
!ENDIF


# Read the list of currency display name resource bundle files
!IF EXISTS("$(ICUSRCDATA)\curr\resfiles.mk")
!INCLUDE "$(ICUSRCDATA)\curr\resfiles.mk"
!IF EXISTS("$(ICUSRCDATA)\curr\reslocal.mk")
!INCLUDE "$(ICUSRCDATA)\curr\reslocal.mk"
CURR_SOURCE=$(CURR_SOURCE) $(CURR_SOURCE_LOCAL)
!ELSE
!MESSAGE Information: cannot find "curr\reslocal.mk". Not building user-additional resource bundle files.
!ENDIF
!ELSE
!MESSAGE Warning: cannot find "curr\resfiles.mk"
!ENDIF

!IFDEF CURR_SOURCE
CURR_FILES = curr\root.txt supplementalData.txt $(CURR_ALIAS_SOURCE) $(CURR_SOURCE)
CURR_RES_FILES = $(CURR_FILES:.txt =.res curr\)
CURR_RES_FILES = $(CURR_RES_FILES:.txt=.res)
CURR_RES_FILES = curr\pool.res $(CURR_RES_FILES:curr\ =curr\)
ALL_RES = $(ALL_RES) curr\res_index.res
!ENDIF

# Read the list of language/script display name resource bundle files
!IF EXISTS("$(ICUSRCDATA)\lang\resfiles.mk")
!INCLUDE "$(ICUSRCDATA)\lang\resfiles.mk"
!IF EXISTS("$(ICUSRCDATA)\lang\reslocal.mk")
!INCLUDE "$(ICUSRCDATA)\lang\reslocal.mk"
LANG_SOURCE=$(LANG_SOURCE) $(LANG_SOURCE_LOCAL)
!ELSE
!MESSAGE Information: cannot find "lang\reslocal.mk". Not building user-additional resource bundle files.
!ENDIF
!ELSE
!MESSAGE Warning: cannot find "lang\resfiles.mk"
!ENDIF

!IFDEF LANG_SOURCE
LANG_FILES = lang\root.txt $(LANG_ALIAS_SOURCE) $(LANG_SOURCE)
LANG_RES_FILES = $(LANG_FILES:.txt =.res lang\)
LANG_RES_FILES = $(LANG_RES_FILES:.txt=.res)
LANG_RES_FILES = lang\pool.res $(LANG_RES_FILES:lang\ =lang\)
ALL_RES = $(ALL_RES) lang\res_index.res
!ENDIF

# Read the list of region display name resource bundle files
!IF EXISTS("$(ICUSRCDATA)\region\resfiles.mk")
!INCLUDE "$(ICUSRCDATA)\region\resfiles.mk"
!IF EXISTS("$(ICUSRCDATA)\region\reslocal.mk")
!INCLUDE "$(ICUSRCDATA)\region\reslocal.mk"
REGION_SOURCE=$(REGION_SOURCE) $(REGION_SOURCE_LOCAL)
!ELSE
!MESSAGE Information: cannot find "region\reslocal.mk". Not building user-additional resource bundle files.
!ENDIF
!ELSE
!MESSAGE Warning: cannot find "region\resfiles.mk"
!ENDIF

!IFDEF REGION_SOURCE
REGION_FILES = region\root.txt $(REGION_ALIAS_SOURCE) $(REGION_SOURCE)
REGION_RES_FILES = $(REGION_FILES:.txt =.res region\)
REGION_RES_FILES = $(REGION_RES_FILES:.txt=.res)
REGION_RES_FILES = region\pool.res $(REGION_RES_FILES:region\ =region\)
ALL_RES = $(ALL_RES) region\res_index.res
!ENDIF

# Read the list of time zone display name resource bundle files
!IF EXISTS("$(ICUSRCDATA)\zone\resfiles.mk")
!INCLUDE "$(ICUSRCDATA)\zone\resfiles.mk"
!IF EXISTS("$(ICUSRCDATA)\zone\reslocal.mk")
!INCLUDE "$(ICUSRCDATA)\zone\reslocal.mk"
ZONE_SOURCE=$(ZONE_SOURCE) $(ZONE_SOURCE_LOCAL)
!ELSE
!MESSAGE Information: cannot find "zone\reslocal.mk". Not building user-additional resource bundle files.
!ENDIF
!ELSE
!MESSAGE Warning: cannot find "zone\resfiles.mk"
!ENDIF

!IFDEF ZONE_SOURCE
ZONE_FILES = zone\root.txt $(ZONE_ALIAS_SOURCE) $(ZONE_SOURCE)
ZONE_RES_FILES = $(ZONE_FILES:.txt =.res zone\)
ZONE_RES_FILES = $(ZONE_RES_FILES:.txt=.res)
ZONE_RES_FILES = zone\pool.res $(ZONE_RES_FILES:zone\ =zone\)
ALL_RES = $(ALL_RES) zone\res_index.res
!ENDIF

# Read the list of collation resource bundle files
!IF EXISTS("$(ICUSRCDATA)\$(ICUCOL)\colfiles.mk")
!INCLUDE "$(ICUSRCDATA)\$(ICUCOL)\colfiles.mk"
!IF EXISTS("$(ICUSRCDATA)\$(ICUCOL)\collocal.mk")
!INCLUDE "$(ICUSRCDATA)\$(ICUCOL)\collocal.mk"
COLLATION_SOURCE=$(COLLATION_SOURCE) $(COLLATION_SOURCE_LOCAL)
!ELSE
!MESSAGE Information: cannot find "collocal.mk". Not building user-additional resource bundle files.
!ENDIF
!ELSE
!MESSAGE Warning: cannot find "colfiles.mk"
!ENDIF

!IFDEF COLLATION_SOURCE
COL_FILES = $(ICUCOL)\root.txt $(COLLATION_ALIAS_SOURCE) $(COLLATION_SOURCE)
COL_COL_FILES = $(COL_FILES:.txt =.res coll\)
COL_COL_FILES = $(COL_COL_FILES:.txt=.res)
COL_COL_FILES = $(COL_COL_FILES:coll\ =)
ALL_RES = $(ALL_RES) $(ICUCOL)\res_index.res
!ENDIF

# Read the list of RBNF resource bundle files
!IF EXISTS("$(ICUSRCDATA)\$(ICURBNF)\rbnffiles.mk")
!INCLUDE "$(ICUSRCDATA)\$(ICURBNF)\rbnffiles.mk"
!IF EXISTS("$(ICUSRCDATA)\$(ICURBNF)\rbnflocal.mk")
!INCLUDE "$(ICUSRCDATA)\$(ICURBNF)\rbnflocal.mk"
RBNF_SOURCE=$(RBNF_SOURCE) $(RBNF_SOURCE_LOCAL)
!ELSE
!MESSAGE Information: cannot find "rbnflocal.mk". Not building user-additional resource bundle files.
!ENDIF
!ELSE
!MESSAGE Warning: cannot find "rbnffiles.mk"
!ENDIF

!IFDEF RBNF_SOURCE
RBNF_FILES = $(ICURBNF)\root.txt $(RBNF_ALIAS_SOURCE) $(RBNF_SOURCE)
RBNF_RES_FILES = $(RBNF_FILES:.txt =.res rbnf\)
RBNF_RES_FILES = $(RBNF_RES_FILES:.txt=.res)
RBNF_RES_FILES = $(RBNF_RES_FILES:rbnf\ =rbnf\)
ALL_RES = $(ALL_RES) $(ICURBNF)\res_index.res
!ENDIF

# Read the list of transliterator resource bundle files
!IF EXISTS("$(ICUSRCDATA)\$(ICUTRNS)\trnsfiles.mk")
!INCLUDE "$(ICUSRCDATA)\$(ICUTRNS)\trnsfiles.mk"
!IF EXISTS("$(ICUSRCDATA)\$(ICUTRNS)\trnslocal.mk")
!INCLUDE "$(ICUSRCDATA)\$(ICUTRNS)\trnslocal.mk"
TRANSLIT_SOURCE=$(TRANSLIT_SOURCE) $(TRANSLIT_SOURCE_LOCAL)
!ELSE
!MESSAGE Information: cannot find "trnslocal.mk". Not building user-additional transliterator files.
!ENDIF
!ELSE
!MESSAGE Warning: cannot find "trnsfiles.mk"
!ENDIF

!IFDEF TRANSLIT_SOURCE
TRANSLIT_FILES = $(ICUTRNS)\$(TRANSLIT_ALIAS_SOURCE) $(TRANSLIT_SOURCE)
TRANSLIT_RES_FILES = $(TRANSLIT_FILES:.txt =.res translit\)
TRANSLIT_RES_FILES = $(TRANSLIT_RES_FILES:.txt=.res)
TRANSLIT_RES_FILES = $(TRANSLIT_RES_FILES:translit\ =translit\)
#ALL_RES = $(ALL_RES) $(ICUTRNS)\res_index.res
!ENDIF

# Read the list of miscellaneous resource bundle files
!IF EXISTS("$(ICUSRCDATA)\$(ICUMISC2)\miscfiles.mk")
!INCLUDE "$(ICUSRCDATA)\$(ICUMISC2)\miscfiles.mk"
!IF EXISTS("$(ICUSRCDATA)\$(ICUMISC2)\misclocal.mk")
!INCLUDE "$(ICUSRCDATA)\$(ICUMISC2)\misclocal.mk"
MISC_SOURCE=$(MISC_SOURCE) $(MISC_SOURCE_LOCAL)
!ELSE
!MESSAGE Information: cannot find "misclocal.mk". Not building user-additional miscellaenous files.
!ENDIF
!ELSE
!MESSAGE Warning: cannot find "miscfiles.mk"
!ENDIF

MISC_FILES = $(MISC_SOURCE:.txt=.res)

# don't include COL_FILES
ALL_RES = $(ALL_RES) $(RB_FILES) $(MISC_FILES)
!ENDIF

# Read the list of stringprep profile files
!IF EXISTS("$(ICUSRCDATA)\$(ICUSPREP)\sprepfiles.mk")
!INCLUDE "$(ICUSRCDATA)\$(ICUSPREP)\sprepfiles.mk"
!IF EXISTS("$(ICUSRCDATA)\$(ICUSPREP)\spreplocal.mk")
!INCLUDE "$(ICUSRCDATA)\$(ICUSPREP)\spreplocal.mk"
SPREP_SOURCE=$(SPREP_SOURCE) $(SPREP_SOURCE_LOCAL)
!ELSE
!MESSAGE Information: cannot find "spreplocal.mk". Not building user-additional stringprep files.
!ENDIF
!ELSE
!MESSAGE Warning: cannot find "sprepfiles.mk"
!ENDIF

SPREP_FILES = $(SPREP_SOURCE:.txt=.spp)

# Common defines for both ways of building ICU's data library.
COMMON_ICUDATA_DEPENDENCIES="$(ICUPBIN)\pkgdata.exe" "$(ICUTMP)\icudata.res" "$(ICUP)\source\stubdata\stubdatabuilt.txt"
COMMON_ICUDATA_ARGUMENTS=-f -e $(U_ICUDATA_NAME) -v $(ICU_PACKAGE_MODE) -c -p $(ICUPKG) -T "$(ICUTMP)" -L $(U_ICUDATA_NAME) -d "$(ICUBLD_PKG)" -s .

#############################################################################
#
# ALL
#     This target builds all the data files.  The world starts here.
#			Note: we really want the common data dll to go to $(DLL_OUTPUT), not $(ICUBLD_PKG).  But specifying
#				that here seems to cause confusion with the building of the stub library of the same name.
#				Building the common dll in $(ICUBLD_PKG) unconditionally copies it to $(DLL_OUTPUT) too.
#
#############################################################################
ALL : GODATA "$(ICU_LIB_TARGET)" "$(TESTDATAOUT)\testdata.dat"
	@echo All targets are up to date
#############################################################################
#
# DATALIB
#     This target builds the data library. It's identical to 
#     ALL except that testdata.dat is not built.
#
#############################################################################
DATALIB : GODATA "$(ICU_LIB_TARGET)"
	@echo Data library is built



# The core Unicode properties files (uprops.icu, ucase.icu, ubidi.icu)
# are hardcoded in the common DLL and therefore not included in the data package any more.
# They are not built by default but need to be built for ICU4J data and for getting the .c source files
# when updating the Unicode data.
# Changed in makedata.mak revision 1.117. See Jitterbug 4497.
# Command line:
#   C:\svn\icuproj\icu\trunk\source\data>nmake -f makedata.mak ICUMAKE=C:\svn\icuproj\icu\trunk\source\data\ CFG=x86\Debug uni-core-data
uni-core-data: GODATA "$(ICUBLD_PKG)\uprops.icu" "$(ICUBLD_PKG)\ucase.icu" "$(ICUBLD_PKG)\ubidi.icu"
	@echo Unicode .icu files built to "$(ICUBLD_PKG)"

# Build the ICU4J icudata.jar and testdata.jar.
# see icu4j-readme.txt

# Build icudata.jar:
# - add the uni-core-data to the ICU package
# - swap the ICU data
# - extract all data items
# - package them into the .jar file
"$(ICUOUT)\icu4j\icudata.jar": GODATA "$(ICUOUT)\$(ICUPKG).dat" uni-core-data
	if not exist "$(ICUOUT)\icu4j\com\ibm\icu\impl\data\$(U_ICUDATA_NAME)b" mkdir "$(ICUOUT)\icu4j\com\ibm\icu\impl\data\$(U_ICUDATA_NAME)b"
	echo ubidi.icu ucase.icu uprops.icu > "$(ICUOUT)\icu4j\add.txt"
	"$(ICUPBIN)\icupkg" "$(ICUOUT)\$(ICUPKG).dat" "$(ICUOUT)\icu4j\$(U_ICUDATA_NAME)b.dat" -a "$(ICUOUT)\icu4j\add.txt" -s "$(ICUBLD_PKG)" -x * -tb -d "$(ICUOUT)\icu4j\com\ibm\icu\impl\data\$(U_ICUDATA_NAME)b"
	"$(JAR)" cf "$(ICUOUT)\icu4j\icudata.jar" -C "$(ICUOUT)\icu4j" com\ibm\icu\impl\data\$(U_ICUDATA_NAME)b

# Build testdata.jar:
# - swap the test data
# - extract all data items
# - package them into the .jar file
"$(ICUOUT)\icu4j\testdata.jar": GODATA "$(TESTDATAOUT)\testdata.dat"
	if not exist "$(ICUOUT)\icu4j\com\ibm\icu\dev\data\testdata" mkdir "$(ICUOUT)\icu4j\com\ibm\icu\dev\data\testdata"
	"$(ICUPBIN)\icupkg" "$(TESTDATAOUT)\testdata.dat" -r test.icu -x * -tb -d "$(ICUOUT)\icu4j\com\ibm\icu\dev\data\testdata"
	"$(JAR)" cf "$(ICUOUT)\icu4j\testdata.jar" -C "$(ICUOUT)\icu4j" com\ibm\icu\dev\data\testdata

## Compare to:  source\data\Makefile.in and source\test\testdata\Makefile.in

DEBUGUTILITIESDATA_DIR=main\tests\core\src\com\ibm\icu\dev\test\util
DEBUGUTILITIESDATA_SRC=DebugUtilitiesData.java

# Build DebugUtilitiesData.java
"$(ICUOUT)\icu4j\src\$(DEBUGUTILITIESDATA_DIR)\$(DEBUGUTILITIESDATA_SRC)" : {"$(ICUTOOLS)\gentest\$(CFG)"}gentest.exe
	if not exist "$(ICUOUT)\icu4j\src\$(DEBUGUTILITIESDATA_DIR)" mkdir "$(ICUOUT)\icu4j\src\$(DEBUGUTILITIESDATA_DIR)"
	"$(ICUTOOLS)\gentest\$(CFG)\gentest" -j -d"$(ICUOUT)\icu4j\src\$(DEBUGUTILITIESDATA_DIR)"

ICU4J_DATA="$(ICUOUT)\icu4j\icudata.jar" "$(ICUOUT)\icu4j\testdata.jar"  "$(ICUOUT)\icu4j\src\$(DEBUGUTILITIESDATA_DIR)\$(DEBUGUTILITIESDATA_SRC)"

icu4j-data: GODATA $(ICU4J_DATA)

!IFDEF ICU4J_ROOT

"$(ICU4J_ROOT)\main\shared\data\icudata.jar": "$(ICUOUT)\icu4j\icudata.jar"
	if not exist "$(ICU4J_ROOT)\main\shared\data" mkdir "$(ICU4J_ROOT)\main\shared\data"
	copy "$(ICUOUT)\icu4j\icudata.jar" "$(ICU4J_ROOT)\main\shared\data"

"$(ICU4J_ROOT)\main\shared\data\testdata.jar": "$(ICUOUT)\icu4j\testdata.jar"
	if not exist "$(ICU4J_ROOT)\main\shared\data" mkdir "$(ICU4J_ROOT)\main\shared\data"
	copy "$(ICUOUT)\icu4j\testdata.jar" "$(ICU4J_ROOT)\main\shared\data"

# "$(DEBUGUTILTIESDATA_OUT)"

"$(ICU4J_ROOT)\$(DEBUGUTILITIESDATA_DIR)\$(DEBUGUTILITIESDATA_SRC)": "$(ICUOUT)\icu4j\src\$(DEBUGUTILITIESDATA_DIR)\$(DEBUGUTILITIESDATA_SRC)"
	if not exist "$(ICU4J_ROOT)\$(DEBUGUTILITIESDATA_DIR)" mkdir "$(ICU4J_ROOT)\$(DEBUGUTILITIESDATA_DIR)"
	copy "$(ICUOUT)\icu4j\src\$(DEBUGUTILITIESDATA_DIR)\$(DEBUGUTILITIESDATA_SRC)" "$(ICU4J_ROOT)\$(DEBUGUTILITIESDATA_DIR)\$(DEBUGUTILITIESDATA_SRC)"

ICU4J_DATA_INSTALLED="$(ICU4J_ROOT)\main\shared\data\icudata.jar" "$(ICU4J_ROOT)\main\shared\data\testdata.jar" "$(ICU4J_ROOT)\$(DEBUGUTILITIESDATA_DIR)\$(DEBUGUTILITIESDATA_SRC)"

icu4j-data-install : GODATA $(ICU4J_DATA) $(ICU4J_DATA_INSTALLED)
	@echo ICU4J  data output to "$(ICU4J_ROOT)"

!ELSE

icu4j-data-install : 
	@echo ERROR ICU4J_ROOT not set
	@exit 1

!ENDIF



#
# testdata - nmake will invoke pkgdata, which will create testdata.dat
#
"$(TESTDATAOUT)\testdata.dat": "$(TESTDATA)\*" "$(ICUBLD_PKG)\$(ICUCOL)\ucadata.icu" $(TRANSLIT_RES_FILES) $(MISC_FILES) $(RB_FILES) {"$(ICUTOOLS)\genrb\$(CFG)"}genrb.exe
	@cd "$(TESTDATA)"
	@echo building testdata...
	nmake /nologo /f "$(TESTDATA)\testdata.mak" TESTDATA=. ICUTOOLS="$(ICUTOOLS)" ICUPBIN="$(ICUPBIN)" ICUP="$(ICUP)" CFG=$(CFG) TESTDATAOUT="$(TESTDATAOUT)" TESTDATABLD="$(TESTDATABLD)"

#invoke pkgdata for ICU common data
#  pkgdata will drop all output files (.dat, .dll, .lib) into the target (ICUBLD_PKG) directory.
#  move the .dll and .lib files to their final destination afterwards.
#  The $(U_ICUDATA_NAME).lib and $(U_ICUDATA_NAME).exp should already be in the right place due to stubdata.
#
#  2005-may-05 Removed Unicode properties files (unorm.icu, uprops.icu, ucase.icu, ubidi.icu)
#  from data build. See Jitterbug 4497. (makedata.mak revision 1.117)
#
!IFDEF ICUDATA_SOURCE_ARCHIVE
"$(ICU_LIB_TARGET)" : $(COMMON_ICUDATA_DEPENDENCIES) "$(ICUDATA_SOURCE_ARCHIVE)"
	@echo Building icu data from $(ICUDATA_SOURCE_ARCHIVE)
	cd "$(ICUBLD_PKG)"
	"$(ICUPBIN)\icupkg" -x * --list "$(ICUDATA_SOURCE_ARCHIVE)" > "$(ICUTMP)\icudata.lst"
	"$(ICUPBIN)\pkgdata" $(COMMON_ICUDATA_ARGUMENTS) "$(ICUTMP)\icudata.lst"
	editbin /NXCOMPAT /DYNAMICBASE "$(U_ICUDATA_NAME).dll"
	copy "$(U_ICUDATA_NAME).dll" "$(DLL_OUTPUT)"
	-@erase "$(U_ICUDATA_NAME).dll"
	copy "$(ICUTMP)\$(ICUPKG).dat" "$(ICUOUT)\$(U_ICUDATA_NAME)$(U_ICUDATA_ENDIAN_SUFFIX).dat"
	-@erase "$(ICUTMP)\$(ICUPKG).dat"
!ELSE
"$(ICU_LIB_TARGET)" : $(COMMON_ICUDATA_DEPENDENCIES) $(CNV_FILES) $(CNV_FILES_SPECIAL) "$(ICUBLD_PKG)\unames.icu" "$(ICUBLD_PKG)\pnames.icu" "$(ICUBLD_PKG)\cnvalias.icu" "$(ICUBLD_PKG)\nfc.nrm" "$(ICUBLD_PKG)\nfkc.nrm" "$(ICUBLD_PKG)\nfkc_cf.nrm" "$(ICUBLD_PKG)\uts46.nrm" "$(ICUBLD_PKG)\$(ICUCOL)\ucadata.icu" "$(ICUBLD_PKG)\$(ICUCOL)\invuca.icu" $(CURR_RES_FILES) $(LANG_RES_FILES) $(REGION_RES_FILES) $(ZONE_RES_FILES) $(BRK_FILES) $(BRK_CTD_FILES) $(BRK_RES_FILES) $(COL_COL_FILES) $(RBNF_RES_FILES) $(TRANSLIT_RES_FILES) $(ALL_RES) $(SPREP_FILES) "$(ICUBLD_PKG)\confusables.cfu"
	@echo Building icu data
	cd "$(ICUBLD_PKG)"
	"$(ICUPBIN)\pkgdata" $(COMMON_ICUDATA_ARGUMENTS) <<"$(ICUTMP)\icudata.lst"
pnames.icu
unames.icu
confusables.cfu
$(ICUCOL)\ucadata.icu
$(ICUCOL)\invuca.icu
cnvalias.icu
nfc.nrm
nfkc.nrm
nfkc_cf.nrm
uts46.nrm
$(CNV_FILES:.cnv =.cnv
)
$(CNV_FILES_SPECIAL:.cnv =.cnv
)
$(ALL_RES:.res =.res
)
$(CURR_RES_FILES:.res =.res
)
$(LANG_RES_FILES:.res =.res
)
$(REGION_RES_FILES:.res =.res
)
$(ZONE_RES_FILES:.res =.res
)
$(COL_COL_FILES:.res =.res
)
$(RBNF_RES_FILES:.res =.res
)
$(TRANSLIT_RES_FILES:.res =.res
)
$(BRK_FILES:.brk =.brk
)
$(BRK_CTD_FILES:.ctd =.ctd
)
$(BRK_RES_FILES:.res =.res
)
$(SPREP_FILES:.spp=.spp
)
<<KEEP
	-@erase "$(ICU_LIB_TARGET)"
	copy "$(U_ICUDATA_NAME).dll" "$(ICU_LIB_TARGET)"
	-@erase "$(U_ICUDATA_NAME).dll"
	copy "$(ICUTMP)\$(ICUPKG).dat" "$(ICUOUT)\$(U_ICUDATA_NAME)$(U_ICUDATA_ENDIAN_SUFFIX).dat"
	-@erase "$(ICUTMP)\$(ICUPKG).dat"
!ENDIF

# utility target to create missing directories
CREATE_DIRS :
	@if not exist "$(ICUOUT)\$(NULL)" mkdir "$(ICUOUT)"
	@if not exist "$(ICUTMP)\$(NULL)" mkdir "$(ICUTMP)"
	@if not exist "$(ICUOUT)\build\$(NULL)" mkdir "$(ICUOUT)\build"
	@if not exist "$(ICUBLD_PKG)\$(NULL)" mkdir "$(ICUBLD_PKG)"
	@if not exist "$(ICUBLD_PKG)\curr\$(NULL)" mkdir "$(ICUBLD_PKG)\curr"
	@if not exist "$(ICUBLD_PKG)\lang\$(NULL)" mkdir "$(ICUBLD_PKG)\lang"
	@if not exist "$(ICUBLD_PKG)\region\$(NULL)" mkdir "$(ICUBLD_PKG)\region"
	@if not exist "$(ICUBLD_PKG)\zone\$(NULL)" mkdir "$(ICUBLD_PKG)\zone"
	@if not exist "$(ICUBLD_PKG)\$(ICUBRK)\$(NULL)" mkdir "$(ICUBLD_PKG)\$(ICUBRK)"
	@if not exist "$(ICUBLD_PKG)\$(ICUCOL)\$(NULL)" mkdir "$(ICUBLD_PKG)\$(ICUCOL)"
	@if not exist "$(ICUBLD_PKG)\$(ICURBNF)\$(NULL)" mkdir "$(ICUBLD_PKG)\$(ICURBNF)"
	@if not exist "$(ICUBLD_PKG)\$(ICUTRNS)\$(NULL)" mkdir "$(ICUBLD_PKG)\$(ICUTRNS)"
	@if not exist "$(TESTDATAOUT)\$(NULL)" mkdir "$(TESTDATAOUT)"
	@if not exist "$(TESTDATABLD)\$(NULL)" mkdir "$(TESTDATABLD)"
	@if not exist "$(TESTDATAOUT)\testdata\$(NULL)" mkdir "$(TESTDATAOUT)\testdata"

# utility target to send us to the right dir
GODATA : CREATE_DIRS
	@cd "$(ICUBLD_PKG)"

# This is to remove all the data files
CLEAN : GODATA
	@echo Cleaning up the data files.
	@cd "$(ICUBLD_PKG)"
	-@erase "*.cnv"
	-@erase "*.exp"
	-@erase "*.icu"
	-@erase "*.lib"
	-@erase "*.nrm"
	-@erase "*.res"
	-@erase "*.spp"
	-@erase "*.txt"
	-@erase "*.cfu"
	-@erase "curr\*.res"
	-@erase "curr\*.txt"
	-@erase "lang\*.res"
	-@erase "lang\*.txt"
	-@erase "region\*.res"
	-@erase "region\*.txt"
	-@erase "zone\*.res"
	-@erase "zone\*.txt"
	@cd "$(ICUBLD_PKG)\$(ICUBRK)"
	-@erase "*.brk"
	-@erase "*.ctd"
	-@erase "*.res"
	-@erase "*.txt"
	@cd "$(ICUBLD_PKG)\$(ICUCOL)"
	-@erase "*.res"
	-@erase "*.txt"
	@cd "$(ICUBLD_PKG)\$(ICURBNF)"
	-@erase "*.res"
	-@erase "*.txt"
	@cd "$(ICUBLD_PKG)\$(ICUTRNS)"
	-@erase "*.res"
	@cd "$(ICUOUT)"
	-@erase "*.dat"
	@cd "$(ICUTMP)"
	-@erase "*.html"
	-@erase "*.lst"
	-@erase "*.mak"
	-@erase "*.obj"
	-@erase "*.res"
	@cd "$(TESTDATABLD)"
	-@erase "*.cnv"
	-@erase "*.icu"
	-@erase "*.mak"
	-@erase "*.res"
	-@erase "*.spp"
	-@erase "*.txt"
	@cd "$(TESTDATAOUT)"
	-@erase "*.dat"
	@cd "$(TESTDATAOUT)\testdata"
	-@erase "*.typ"
	@cd "$(ICUBLD_PKG)"


# RBBI .brk file generation.
{$(ICUSRCDATA_RELATIVE_PATH)\$(ICUBRK)}.txt.brk:
	@echo Creating $@
	@"$(ICUTOOLS)\genbrk\$(CFG)\genbrk" -c -r $< -o $@ -d"$(ICUBLD_PKG)" -i "$(ICUBLD_PKG)"

# RBBI .ctd file generation.
{$(ICUSRCDATA_RELATIVE_PATH)\$(ICUBRK)}.txt.ctd:
	@echo Creating $@
	@"$(ICUTOOLS)\genctd\$(CFG)\genctd" -c -o $@ -d"$(ICUBLD_PKG)" -i "$(ICUBLD_PKG)" $<

!IFNDEF ICUDATA_SOURCE_ARCHIVE
# Rule for creating converters
$(CNV_FILES): $(UCM_SOURCE)
	@echo Making Charset Conversion tables
	@"$(ICUTOOLS)\makeconv\$(CFG)\makeconv" -c -d"$(ICUBLD_PKG)" $(ICUSRCDATA_RELATIVE_PATH)\$(ICUUCM)\$(@B).ucm
!ENDIF

!IFDEF BUILD_SPECIAL_CNV_FILES
$(CNV_FILES_SPECIAL): $(UCM_SOURCE_SPECIAL)
	@echo Making Special Charset Conversion tables
	@"$(ICUTOOLS)\makeconv\$(CFG)\makeconv" -c --ignore-siso-check -d"$(ICUBLD_PKG)" $(ICUSRCDATA_RELATIVE_PATH)\$(ICUUCM)\$(@B).ucm
!ENDIF

# Batch inference rule for creating miscellaneous resource files
# TODO: -q option is specified to squelch the 120+ warnings about
#       empty intvectors and binary elements.  Unfortunately, this may
#       squelch other legitimate warnings.  When there is a better
#       way, remove the -q.
{$(ICUSRCDATA_RELATIVE_PATH)\$(ICUMISC2)}.txt.res::
	@echo Making Miscellaneous Resource Bundle files
	@"$(ICUTOOLS)\genrb\$(CFG)\genrb" -k -q -d"$(ICUBLD_PKG)" $<

# Inference rule for creating resource bundle files
{$(ICUSRCDATA_RELATIVE_PATH)\$(ICULOC)}.txt.res::
	@echo Making Locale Resource Bundle files
	@"$(ICUTOOLS)\genrb\$(CFG)\genrb" --usePoolBundle $(ICUSRCDATA_RELATIVE_PATH)\$(ICULOC) -k -d"$(ICUBLD_PKG)" $<

# copy the locales/pool.res file from the source folder to the build output folder
# and swap it to native endianness
pool.res: $(ICUSRCDATA_RELATIVE_PATH)\$(ICULOC)\pool.res
	"$(ICUPBIN)\icupkg" -tl "$(ICUSRCDATA_RELATIVE_PATH)\$(ICULOC)\pool.res" pool.res

res_index.res:
	@echo Generating <<res_index.txt
// Warning this file is automatically generated
res_index:table(nofallback) {
    InstalledLocales {
        $(GENRB_SOURCE:.txt= {""}
       )
    }
}
<<KEEP
	@"$(ICUTOOLS)\genrb\$(CFG)\genrb" -k -d"$(ICUBLD_PKG)" .\res_index.txt
	

{$(ICUSRCDATA_RELATIVE_PATH)\curr}.txt{curr}.res::
	@echo Making currency display name files
	@"$(ICUTOOLS)\genrb\$(CFG)\genrb" --usePoolBundle $(ICUSRCDATA_RELATIVE_PATH)\curr -k -i "$(ICUBLD_PKG)" -d"$(ICUBLD_PKG)\curr" $<

# copy the curr/pool.res file from the source folder to the build output folder
# and swap it to native endianness
curr\pool.res: $(ICUSRCDATA_RELATIVE_PATH)\curr\pool.res
	"$(ICUPBIN)\icupkg" -tl "$(ICUSRCDATA_RELATIVE_PATH)\curr\pool.res" curr\pool.res

curr\res_index.res:
	@echo Generating <<curr\res_index.txt
// Warning this file is automatically generated
res_index:table(nofallback) {
    InstalledLocales {
        $(CURR_SOURCE:.txt= {""}
       )
    }
}
<<KEEP
	@"$(ICUTOOLS)\genrb\$(CFG)\genrb" -k -d"$(ICUBLD_PKG)\curr" .\curr\res_index.txt


{$(ICUSRCDATA_RELATIVE_PATH)\lang}.txt{lang}.res::
	@echo Making language/script display name files
	@"$(ICUTOOLS)\genrb\$(CFG)\genrb" --usePoolBundle $(ICUSRCDATA_RELATIVE_PATH)\lang -k -i "$(ICUBLD_PKG)" -d"$(ICUBLD_PKG)\lang" $<

# copy the lang/pool.res file from the source folder to the build output folder
# and swap it to native endianness
lang\pool.res: $(ICUSRCDATA_RELATIVE_PATH)\lang\pool.res
	"$(ICUPBIN)\icupkg" -tl "$(ICUSRCDATA_RELATIVE_PATH)\lang\pool.res" lang\pool.res

lang\res_index.res:
	@echo Generating <<lang\res_index.txt
// Warning this file is automatically generated
res_index:table(nofallback) {
    InstalledLocales {
        $(LANG_SOURCE:.txt= {""}
       )
    }
}
<<KEEP
	@"$(ICUTOOLS)\genrb\$(CFG)\genrb" -k -d"$(ICUBLD_PKG)\lang" .\lang\res_index.txt


{$(ICUSRCDATA_RELATIVE_PATH)\region}.txt{region}.res::
	@echo Making region display name files
	@"$(ICUTOOLS)\genrb\$(CFG)\genrb" --usePoolBundle $(ICUSRCDATA_RELATIVE_PATH)\region -k -i "$(ICUBLD_PKG)" -d"$(ICUBLD_PKG)\region" $<

# copy the region/pool.res file from the source folder to the build output folder
# and swap it to native endianness
region\pool.res: $(ICUSRCDATA_RELATIVE_PATH)\region\pool.res
	"$(ICUPBIN)\icupkg" -tl "$(ICUSRCDATA_RELATIVE_PATH)\region\pool.res" region\pool.res

region\res_index.res:
	@echo Generating <<region\res_index.txt
// Warning this file is automatically generated
res_index:table(nofallback) {
    InstalledLocales {
        $(REGION_SOURCE:.txt= {""}
       )
    }
}
<<KEEP
	@"$(ICUTOOLS)\genrb\$(CFG)\genrb" -k -d"$(ICUBLD_PKG)\region" .\region\res_index.txt


{$(ICUSRCDATA_RELATIVE_PATH)\zone}.txt{zone}.res::
	@echo Making time zone display name files
	@"$(ICUTOOLS)\genrb\$(CFG)\genrb" --usePoolBundle $(ICUSRCDATA_RELATIVE_PATH)\zone -k -i "$(ICUBLD_PKG)" -d"$(ICUBLD_PKG)\zone" $<

# copy the zone/pool.res file from the source folder to the build output folder
# and swap it to native endianness
zone\pool.res: $(ICUSRCDATA_RELATIVE_PATH)\zone\pool.res
	"$(ICUPBIN)\icupkg" -tl "$(ICUSRCDATA_RELATIVE_PATH)\zone\pool.res" zone\pool.res

zone\res_index.res:
	@echo Generating <<zone\res_index.txt
// Warning this file is automatically generated
res_index:table(nofallback) {
    InstalledLocales {
        $(ZONE_SOURCE:.txt= {""}
       )
    }
}
<<KEEP
	@"$(ICUTOOLS)\genrb\$(CFG)\genrb" -k -d"$(ICUBLD_PKG)\zone" .\zone\res_index.txt


{$(ICUSRCDATA_RELATIVE_PATH)\$(ICUCOL)}.txt{$(ICUCOL)}.res::
	@echo Making Collation files
	@"$(ICUTOOLS)\genrb\$(CFG)\genrb" -k -i "$(ICUBLD_PKG)" -d"$(ICUBLD_PKG)\$(ICUCOL)" $<

$(ICUCOL)\res_index.res:
	@echo Generating <<$(ICUCOL)\res_index.txt
// Warning this file is automatically generated
res_index:table(nofallback) {
    InstalledLocales {
        $(COLLATION_SOURCE:.txt= {""}
       )
    }
}
<<KEEP
	@"$(ICUTOOLS)\genrb\$(CFG)\genrb" -k -d"$(ICUBLD_PKG)\$(ICUCOL)" .\$(ICUCOL)\res_index.txt

{$(ICUSRCDATA_RELATIVE_PATH)\$(ICURBNF)}.txt{$(ICURBNF)}.res::
	@echo Making RBNF files
	@"$(ICUTOOLS)\genrb\$(CFG)\genrb" -k -i "$(ICUBLD_PKG)" -d"$(ICUBLD_PKG)\$(ICURBNF)" $<

$(ICURBNF)\res_index.res:
	@echo Generating <<$(ICURBNF)\res_index.txt
// Warning this file is automatically generated
res_index:table(nofallback) {
    InstalledLocales {
        $(RBNF_SOURCE:.txt= {""}
       )
    }
}
<<KEEP
	@"$(ICUTOOLS)\genrb\$(CFG)\genrb" -k -d"$(ICUBLD_PKG)\$(ICURBNF)" .\$(ICURBNF)\res_index.txt

$(ICUBRK)\res_index.res:
	@echo Generating <<$(ICUBRK)\res_index.txt
// Warning this file is automatically generated
res_index:table(nofallback) {
    InstalledLocales {
        $(BRK_RES_SOURCE:.txt= {""}
       )
    }
}
<<KEEP
	@"$(ICUTOOLS)\genrb\$(CFG)\genrb" -k -d"$(ICUBLD_PKG)\$(ICUBRK)" .\$(ICUBRK)\res_index.txt

{$(ICUSRCDATA_RELATIVE_PATH)\$(ICUBRK)}.txt{$(ICUBRK)}.res::
	@echo Making Break Iterator Resource files
	@"$(ICUTOOLS)\genrb\$(CFG)\genrb" -k -i "$(ICUBLD_PKG)" -d"$(ICUBLD_PKG)\$(ICUBRK)" $<

{$(ICUSRCDATA_RELATIVE_PATH)\$(ICUTRNS)}.txt{$(ICUTRNS)}.res::
	@echo Making Transliterator files
	@"$(ICUTOOLS)\genrb\$(CFG)\genrb" -k -i "$(ICUBLD_PKG)" -d"$(ICUBLD_PKG)\$(ICUTRNS)" $<


# DLL version information
# If you modify this, modify winmode.c in pkgdata.
"$(ICUTMP)\icudata.res": "$(ICUMISC)\icudata.rc"
	@echo Creating data DLL version information from $**
	@rc.exe /i "..\..\..\..\common;..\..\..\..\..\public\common" /r /fo $@ $**

# Targets for converters
"$(ICUBLD_PKG)\cnvalias.icu" : {"$(ICUSRCDATA)\$(ICUUCM)"}\convrtrs.txt "$(ICUTOOLS)\gencnval\$(CFG)\gencnval.exe"
	@echo Creating data file for Converter Aliases
	@"$(ICUTOOLS)\gencnval\$(CFG)\gencnval" -d "$(ICUBLD_PKG)" "$(ICUSRCDATA)\$(ICUUCM)\convrtrs.txt"

# Targets for prebuilt Unicode data
"$(ICUBLD_PKG)\pnames.icu": $(ICUSRCDATA_RELATIVE_PATH)\in\pnames.icu
	"$(ICUPBIN)\icupkg" -tl $? $@

"$(ICUBLD_PKG)\ubidi.icu": $(ICUSRCDATA_RELATIVE_PATH)\in\ubidi.icu
	"$(ICUPBIN)\icupkg" -tl $? $@

"$(ICUBLD_PKG)\ucase.icu": $(ICUSRCDATA_RELATIVE_PATH)\in\ucase.icu
	"$(ICUPBIN)\icupkg" -tl $? $@

"$(ICUBLD_PKG)\uprops.icu": $(ICUSRCDATA_RELATIVE_PATH)\in\uprops.icu
	"$(ICUPBIN)\icupkg" -tl $? $@

"$(ICUBLD_PKG)\unames.icu": $(ICUSRCDATA_RELATIVE_PATH)\in\unames.icu
	"$(ICUPBIN)\icupkg" -tl $? $@

"$(ICUBLD_PKG)\nfc.nrm": $(ICUSRCDATA_RELATIVE_PATH)\in\nfc.nrm
	"$(ICUPBIN)\icupkg" -tl $? $@

"$(ICUBLD_PKG)\nfkc.nrm": $(ICUSRCDATA_RELATIVE_PATH)\in\nfkc.nrm
	"$(ICUPBIN)\icupkg" -tl $? $@

"$(ICUBLD_PKG)\nfkc_cf.nrm": $(ICUSRCDATA_RELATIVE_PATH)\in\nfkc_cf.nrm
	"$(ICUPBIN)\icupkg" -tl $? $@

"$(ICUBLD_PKG)\uts46.nrm": $(ICUSRCDATA_RELATIVE_PATH)\in\uts46.nrm
	"$(ICUPBIN)\icupkg" -tl $? $@

"$(ICUBLD_PKG)\coll\invuca.icu": $(ICUSRCDATA_RELATIVE_PATH)\in\coll\invuca.icu
	"$(ICUPBIN)\icupkg" -tl $? $@

"$(ICUBLD_PKG)\coll\ucadata.icu": $(ICUSRCDATA_RELATIVE_PATH)\in\coll\ucadata.icu
	"$(ICUPBIN)\icupkg" -tl $? $@

# Stringprep .spp file generation.
{$(ICUSRCDATA_RELATIVE_PATH)\$(ICUSPREP)}.txt.spp:
	@echo Creating $@
	@"$(ICUTOOLS)\gensprep\$(CFG)\gensprep" -s $(<D) -d "$(ICUBLD_PKG)" -b $(@B) -m "$(ICUUNIDATA)" -u 3.2.0 $(<F)

# Confusables .cfu file generation
#     Can't use an inference rule because two .txt source files combine to produce a single .cfu output file
"$(ICUBLD_PKG)\confusables.cfu": "$(ICUUNIDATA)\confusables.txt" "$(ICUUNIDATA)\confusablesWholeScript.txt" "$(ICUTOOLS)\gencfu\$(CFG)\gencfu.exe"
	@echo Creating $@
	@"$(ICUTOOLS)\gencfu\$(CFG)\gencfu" -c -r "$(ICUUNIDATA)\confusables.txt" -w "$(ICUUNIDATA)\confusablesWholeScript.txt" -o $@ -i "$(ICUBLD_PKG)"

!IFDEF ICUDATA_ARCHIVE
"$(ICUDATA_SOURCE_ARCHIVE)": CREATE_DIRS $(ICUDATA_ARCHIVE) "$(ICUTOOLS)\icupkg\$(CFG)\icupkg.exe"
	"$(ICUTOOLS)\icupkg\$(CFG)\icupkg" -t$(U_ICUDATA_ENDIAN_SUFFIX) "$(ICUDATA_ARCHIVE)" "$(ICUDATA_SOURCE_ARCHIVE)"
!ENDIF

# Dependencies on the tools for the batch inference rules

!IFNDEF ICUDATA_SOURCE_ARCHIVE
$(UCM_SOURCE) : {"$(ICUTOOLS)\makeconv\$(CFG)"}makeconv.exe

!IFDEF BUILD_SPECIAL_CNV_FILES
$(UCM_SOURCE_SPECIAL): {"$(ICUTOOLS)\makeconv\$(CFG)"}makeconv.exe
!ENDIF

# This used to depend on "$(ICUBLD_PKG)\uprops.icu" "$(ICUBLD_PKG)\ucase.icu" "$(ICUBLD_PKG)\ubidi.icu"
# This data is now hard coded as a part of the library.
# See Jitterbug 4497 for details.
$(MISC_SOURCE) $(RB_FILES) $(CURR_FILES) $(LANG_FILES) $(REGION_FILES) $(ZONE_FILES) $(COL_COL_FILES) $(RBNF_RES_FILES) $(BRK_RES_FILES) $(TRANSLIT_RES_FILES): {"$(ICUTOOLS)\genrb\$(CFG)"}genrb.exe "$(ICUBLD_PKG)\nfc.nrm" "$(ICUBLD_PKG)\$(ICUCOL)\ucadata.icu"

# This used to depend on "$(ICUBLD_PKG)\uprops.icu" "$(ICUBLD_PKG)\ucase.icu" "$(ICUBLD_PKG)\ubidi.icu"
# This data is now hard coded as a part of the library.
# See Jitterbug 4497 for details.
$(BRK_SOURCE) : "$(ICUBLD_PKG)\unames.icu" "$(ICUBLD_PKG)\pnames.icu" "$(ICUBLD_PKG)\nfc.nrm"
!ENDIF
