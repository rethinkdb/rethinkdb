$!
$!  MAKEENGINES.COM
$!  Written By:  Richard Levitte
$!               richard@levitte.org
$!
$!  This command file compiles and creates the various engines in form
$!  of shared images.  They are placed in [.xxx.EXE.ENGINES], where "xxx"
$!  is ALPHA, IA64 or VAX, depending on your hardware.
$!
$!  P1	if this is ENGINES or ALL, the engines will build, otherwise not.
$!
$!  P2	DEBUG or NODEBUG to compile with or without debugger information.
$!
$!  P3  VAXC		for VAX C
$!	DECC		for DEC C
$!	GNUC		for GNU C (untested)
$!
$!  P4	if defined, sets the TCP/IP libraries to use.  UCX or TCPIP is
$!	used by default since most other implementations come with a
$!	compatibility library.  The value must be one of the following:
$!
$!	UCX		for UCX
$!	SOCKETSHR	for SOCKETSHR+NETLIB
$!	TCPIP		for TCPIP (post UCX)
$!
$!  P5	if defined, tells the compiler not to use special threads.
$!
$!  P6	if defined, denotes which engines to build.  If not defined,
$!	all available engines are built.
$!
$!  P7, if defined, specifies the C pointer size.  Ignored on VAX.
$!      ("64=ARGV" gives more efficient code with HP C V7.3 or newer.)
$!      Supported values are:
$!
$!	""	Compile with default (/NOPOINTER_SIZE)
$!	32	Compile with /POINTER_SIZE=32 (SHORT)
$!	64	Compile with /POINTER_SIZE=64[=ARGV] (LONG[=ARGV])
$!               (Automatically select ARGV if compiler supports it.)
$!      64=      Compile with /POINTER_SIZE=64 (LONG).
$!      64=ARGV  Compile with /POINTER_SIZE=64=ARGV (LONG=ARGV).
$!
$!  P8, if defined, specifies a directory where ZLIB files (zlib.h,
$!  libz.olb) may be found.  Optionally, a non-default object library
$!  name may be included ("dev:[dir]libz_64.olb", for example).
$!
$!-----------------------------------------------------------------------------
$!
$! Announce/identify.
$!
$ proc = f$environment( "procedure")
$ write sys$output "@@@ "+ -
   f$parse( proc, , , "name")+ f$parse( proc, , , "type")
$!
$ on control_c then goto exit
$!
$! Set the default TCP/IP library to link against if needed
$!
$ TCPIP_LIB = ""
$ ZLIB_LIB = ""
$!
$! Check What Architecture We Are Using.
$!
$ IF (F$GETSYI("CPU").LT.128)
$ THEN
$!
$!  The Architecture Is VAX.
$!
$   ARCH = "VAX"
$!
$! Else...
$!
$ ELSE
$!
$!  The Architecture Is Alpha, IA64 or whatever comes in the future.
$!
$   ARCH = F$EDIT( F$GETSYI( "ARCH_NAME"), "UPCASE")
$   IF (ARCH .EQS. "") THEN ARCH = "UNK"
$!
$! End The Architecture Check.
$!
$ ENDIF
$!
$ ARCHD = ARCH
$ LIB32 = "32"
$ OPT_FILE = ""
$ POINTER_SIZE = ""
$!
$! Set the names of the engines we want to build
$! NOTE: Some might think this list ugly.  However, it's made this way to
$! reflect the LIBNAMES variable in Makefile as closely as possible,
$! thereby making it fairly easy to verify that the lists are the same.
$! NOTE: gmp isn't built, as it's mostly a test engine and brings in another
$! library that isn't necessarely ported to VMS.
$!
$ ENGINES = "," + P6
$ IF ENGINES .EQS. "," THEN -
	ENGINES = ",4758cca,aep,atalla,cswift,chil,nuron,sureware,ubsec,padlock,"
$!
$! GOST requires a 64-bit integer type, unavailable on VAX.
$!
$ IF (ARCH .NES. "VAX") THEN -
       ENGINES = ENGINES+ ",ccgost"
$!
$! Check options.
$!
$ OPT_PHASE = P1
$ ACCEPT_PHASE = "ALL,ENGINES"
$ OPT_DEBUG = P2
$ OPT_COMPILER = P3
$ OPT_TCPIP_LIB = P4
$ OPT_SPECIAL_THREADS = P5
$ OPT_POINTER_SIZE = P7
$ ZLIB = P8
$
$ GOSUB CHECK_OPTIONS
$!
$! Set the goal directories, and create them if necessary
$!
$ OBJ_DIR := SYS$DISK:[-.'ARCHD'.OBJ.ENGINES]
$ EXE_DIR := SYS$DISK:[-.'ARCHD'.EXE.ENGINES]
$ IF F$PARSE(OBJ_DIR) .EQS. "" THEN CREATE/DIRECTORY 'OBJ_DIR'
$ IF F$PARSE(EXE_DIR) .EQS. "" THEN CREATE/DIRECTORY 'EXE_DIR'
$!
$! Set the goal files, and create them if necessary
$!
$ CRYPTO_LIB := SYS$DISK:[-.'ARCHD'.EXE.CRYPTO]SSL_LIBCRYPTO'LIB32'.OLB
$ IF F$SEARCH(CRYPTO_LIB) .EQS. "" THEN LIBRARY/CREATE/OBJECT 'CRYPTO_LIB'
$!
$! Specify the destination directory in any /MAP option.
$!
$ if (LINKMAP .eqs. "MAP")
$ then
$   LINKMAP = LINKMAP+ "=''EXE_DIR'"
$ endif
$!
$! Add the location prefix to the linker options file name.
$!
$ if (OPT_FILE .nes. "")
$ then
$   OPT_FILE = EXE_DIR+ OPT_FILE
$ endif
$!
$! Initialise.
$!
$ GOSUB INITIALISE
$ GOSUB CHECK_OPT_FILE
$!
$! Define what goes into each engine.  VAX includes a transfer vector.
$!
$ ENGINE_ = ""
$ TV_OBJ = ""
$ IF ARCH .EQS. "VAX"
$ THEN
$   ENGINE_ = "engine_vector.mar"
$   TV_OBJ_NAME = OBJ_DIR + F$PARSE(ENGINE_,,,"NAME","SYNTAX_ONLY") + ".OBJ"
$   TV_OBJ = ",''TV_OBJ_NAME'"
$ ENDIF
$ ENGINE_4758CCA = "e_4758cca"
$ ENGINE_aep = "e_aep"
$ ENGINE_atalla = "e_atalla"
$ ENGINE_cswift = "e_cswift"
$ ENGINE_chil = "e_chil"
$ ENGINE_nuron = "e_nuron"
$ ENGINE_sureware = "e_sureware"
$ ENGINE_ubsec = "e_ubsec"
$ ENGINE_padlock = "e_padlock"
$
$ ENGINE_ccgost_SUBDIR = "ccgost"
$ ENGINE_ccgost = "e_gost_err,gost2001_keyx,gost2001,gost89,gost94_keyx,"+ -
		  "gost_ameth,gost_asn1,gost_crypt,gost_ctl,gost_eng,"+ -
		  "gosthash,gost_keywrap,gost_md,gost_params,gost_pmeth,"+ -
		  "gost_sign"
$!
$! Define which programs need to be linked with a TCP/IP library
$!
$ TCPIP_ENGINES = ",,"
$ IF COMPILER .EQS. "VAXC" THEN -
     TCPIP_ENGINES = ",,"
$!
$! Set up two loops, one that keeps track of the engines,
$! and one that keeps track of all the files going into
$! the current engine.
$!
$! Here's the start of the engine loop.
$!
$ ENGINE_COUNTER = 0
$ ENGINE_NEXT:
$!
$! Extract the current engine name, and if we've reached the end, stop
$!
$ ENGINE_NAME = F$ELEMENT(ENGINE_COUNTER,",",ENGINES)
$ IF (ENGINE_NAME.EQS.",") THEN GOTO ENGINE_DONE
$!
$ ENGINE_COUNTER = ENGINE_COUNTER + 1
$!
$! Set up the engine library names.
$!
$ LIB_ENGINE = "ENGINE_" + ENGINE_NAME
$!
$! Check if the library module name actually is defined
$!
$ IF F$TYPE('LIB_ENGINE') .EQS. ""
$ THEN
$   WRITE SYS$ERROR ""
$   WRITE SYS$ERROR "The module ",ENGINE_NAME," does not exist.  Continuing..."
$   WRITE SYS$ERROR ""
$   GOTO ENGINE_NEXT
$ ENDIF
$!
$! Talk to the user
$!
$ IF ENGINE_NAME .NES. ""
$ THEN
$   WRITE SYS$OUTPUT "Compiling The ",ENGINE_NAME," Library Files. (",BUILDALL,")"
$ ELSE
$   WRITE SYS$OUTPUT "Compiling Support Files. (",BUILDALL,")"
$ ENDIF
$!
$! Create a .OPT file for the object files (for a real engine name).
$!
$ IF ENGINE_NAME .NES. ""
$ THEN
$   OPEN /WRITE OBJECTS 'EXE_DIR''ENGINE_NAME'.OPT
$ ENDIF
$!
$! Here's the start of per-engine module loop.
$!
$ FILE_COUNTER = 0
$ FILE_NEXT:
$!
$! Extract the file name from the file list, and if we've reached the end, stop
$!
$ FILE_NAME = F$ELEMENT(FILE_COUNTER,",",'LIB_ENGINE')
$ IF (FILE_NAME.EQS.",") THEN GOTO FILE_DONE
$!
$ FILE_COUNTER = FILE_COUNTER + 1
$!
$ IF FILE_NAME .EQS. "" THEN GOTO FILE_NEXT
$!
$! Set up the source and object reference
$!
$ IF F$TYPE('LIB_ENGINE'_SUBDIR) .EQS. ""
$ THEN
$     SOURCE_FILE = F$PARSE(FILE_NAME,"SYS$DISK:[].C",,,"SYNTAX_ONLY")
$ ELSE
$     SOURCE_FILE = F$PARSE(FILE_NAME,"SYS$DISK:[."+'LIB_ENGINE'_SUBDIR+"].C",,,"SYNTAX_ONLY")
$ ENDIF
$ OBJECT_FILE = OBJ_DIR + F$PARSE(FILE_NAME,,,"NAME","SYNTAX_ONLY") + ".OBJ"
$!
$! If we get some problem, we just go on trying to build the next module.
$ ON WARNING THEN GOTO FILE_NEXT
$!
$! Check if the module we want to compile is actually there.
$!
$ IF F$SEARCH(SOURCE_FILE) .EQS. ""
$ THEN
$   WRITE SYS$OUTPUT ""
$   WRITE SYS$OUTPUT "The File ",SOURCE_FILE," Doesn't Exist."
$   WRITE SYS$OUTPUT ""
$   GOTO EXIT
$ ENDIF
$!
$! Talk to the user.
$!
$ WRITE SYS$OUTPUT "	",FILE_NAME,""
$!
$! Do the dirty work.
$!
$ ON ERROR THEN GOTO FILE_NEXT
$ IF F$EDIT(F$PARSE(SOURCE_FILE,,,"TYPE","SYNTAX_ONLY"),"UPCASE") .EQS. ".MAR"
$ THEN
$   MACRO/OBJECT='OBJECT_FILE' 'SOURCE_FILE'
$ ELSE
$   CC/OBJECT='OBJECT_FILE' 'SOURCE_FILE'
$ ENDIF
$!
$! Write the entry to the .OPT file (for a real engine name).
$!
$ IF ENGINE_NAME .NES. ""
$ THEN
$   WRITE OBJECTS OBJECT_FILE
$ ENDIF
$!
$! Next file
$!
$ GOTO FILE_NEXT
$!
$ FILE_DONE:
$!
$! Do not link the support files.
$!
$ IF ENGINE_NAME .EQS. "" THEN GOTO ENGINE_NEXT
$!
$! Close the linker options file (for a real engine name).
$!
$ CLOSE OBJECTS
$!
$! Now, there are two ways to handle this.  We can either build 
$! shareable images or stick the engine object file into libcrypto.
$! For now, the latter is NOT supported.
$!
$!!!!! LIBRARY/REPLACE 'CRYPTO_LIB' 'OBJECT_FILE'
$!
$! For shareable libraries, we need to do things a little differently
$! depending on if we link with a TCP/IP library or not.
$!
$ ENGINE_OPT := SYS$DISK:[]'ARCH'.OPT
$ LINK /'DEBUGGER' /'LINKMAP' /'TRACEBACK' /SHARE='EXE_DIR''ENGINE_NAME'.EXE -
   'EXE_DIR''ENGINE_NAME'.OPT /OPTIONS -
   'TV_OBJ', -
   'CRYPTO_LIB' /LIBRARY, -
   'ENGINE_OPT' /OPTIONS -
   'TCPIP_LIB' -
   'ZLIB_LIB' -
   ,'OPT_FILE' /OPTIONS
$!
$! Next engine
$!
$ GOTO ENGINE_NEXT
$!
$ ENGINE_DONE:
$!
$! Talk to the user
$!
$ WRITE SYS$OUTPUT "All Done..."
$ EXIT:
$ GOSUB CLEANUP
$ EXIT
$!
$! Check For The Link Option FIle.
$!
$ CHECK_OPT_FILE:
$!
$! Check To See If We Need To Make A VAX C Option File.
$!
$ IF (COMPILER.EQS."VAXC")
$ THEN
$!
$!  Check To See If We Already Have A VAX C Linker Option File.
$!
$   IF (F$SEARCH(OPT_FILE).EQS."")
$   THEN
$!
$!    We Need A VAX C Linker Option File.
$!
$     CREATE 'OPT_FILE'
$DECK
!
! Default System Options File To Link Against 
! The Sharable VAX C Runtime Library.
!
SYS$SHARE:VAXCRTL.EXE/SHARE
$EOD
$!
$!  End The Option File Check.
$!
$   ENDIF
$!
$! End The VAXC Check.
$!
$ ENDIF
$!
$! Check To See If We Need A GNU C Option File.
$!
$ IF (COMPILER.EQS."GNUC")
$ THEN
$!
$!  Check To See If We Already Have A GNU C Linker Option File.
$!
$   IF (F$SEARCH(OPT_FILE).EQS."")
$   THEN
$!
$!    We Need A GNU C Linker Option File.
$!
$     CREATE 'OPT_FILE'
$DECK
!
! Default System Options File To Link Against 
! The Sharable C Runtime Library.
!
GNU_CC:[000000]GCCLIB/LIBRARY
SYS$SHARE:VAXCRTL/SHARE
$EOD
$!
$!  End The Option File Check.
$!
$   ENDIF
$!
$! End The GNU C Check.
$!
$ ENDIF
$!
$! Check To See If We Need A DEC C Option File.
$!
$ IF (COMPILER.EQS."DECC")
$ THEN
$!
$!  Check To See If We Already Have A DEC C Linker Option File.
$!
$   IF (F$SEARCH(OPT_FILE).EQS."")
$   THEN
$!
$!    Figure Out If We Need A non-VAX Or A VAX Linker Option File.
$!
$     IF ARCH .EQS. "VAX"
$     THEN
$!
$!      We Need A DEC C Linker Option File For VAX.
$!
$       CREATE 'OPT_FILE'
$DECK
!
! Default System Options File To Link Against 
! The Sharable DEC C Runtime Library.
!
SYS$SHARE:DECC$SHR.EXE/SHARE
$EOD
$!
$!    Else...
$!
$     ELSE
$!
$!      Create The non-VAX Linker Option File.
$!
$       CREATE 'OPT_FILE'
$DECK
!
! Default System Options File For non-VAX To Link Against 
! The Sharable C Runtime Library.
!
SYS$SHARE:CMA$OPEN_LIB_SHR/SHARE
SYS$SHARE:CMA$OPEN_RTL/SHARE
$EOD
$!
$!    End The DEC C Option File Check.
$!
$     ENDIF
$!
$!  End The Option File Search.
$!
$   ENDIF
$!
$! End The DEC C Check.
$!
$ ENDIF
$!
$!  Tell The User What Linker Option File We Are Using.
$!
$ WRITE SYS$OUTPUT "Using Linker Option File ",OPT_FILE,"."	
$!
$! Time To RETURN.
$!
$ RETURN
$!
$! Check The User's Options.
$!
$ CHECK_OPTIONS:
$!
$! Check To See If OPT_PHASE Is Blank.
$!
$ IF (OPT_PHASE.EQS."ALL")
$ THEN
$!
$!   OPT_PHASE Is Blank, So Build Everything.
$!
$    BUILDALL = "ALL"
$!
$! Else...
$!
$ ELSE
$!
$!  Else, Check To See If OPT_PHASE Has A Valid Argument.
$!
$   IF ("," + ACCEPT_PHASE + ",") - ("," + OPT_PHASE + ",") -
       .NES. ("," + ACCEPT_PHASE + ",")
$   THEN
$!
$!    A Valid Argument.
$!
$     BUILDALL = OPT_PHASE
$!
$!  Else...
$!
$   ELSE
$!
$!    Tell The User We Don't Know What They Want.
$!
$     WRITE SYS$OUTPUT ""
$     WRITE SYS$OUTPUT "The option ",OPT_PHASE," is invalid.  The valid options are:"
$     WRITE SYS$OUTPUT ""
$     IF ("," + ACCEPT_PHASE + ",") - ",ALL," -
	.NES. ("," + ACCEPT_PHASE + ",") THEN -
	WRITE SYS$OUTPUT "    ALL      :  just build everything."
$     IF ("," + ACCEPT_PHASE + ",") - ",ENGINES," -
	.NES. ("," + ACCEPT_PHASE + ",") THEN -
	WRITE SYS$OUTPUT "    ENGINES  :  to compile just the [.xxx.EXE.ENGINES]*.EXE hareable images."
$     WRITE SYS$OUTPUT ""
$     WRITE SYS$OUTPUT " where 'xxx' stands for:"
$     WRITE SYS$OUTPUT ""
$     WRITE SYS$OUTPUT "    ALPHA[64]:  Alpha architecture."
$     WRITE SYS$OUTPUT "    IA64[64] :  IA64 architecture."
$     WRITE SYS$OUTPUT "    VAX      :  VAX architecture."
$     WRITE SYS$OUTPUT ""
$!
$!    Time To EXIT.
$!
$     EXIT
$!
$!  End The Valid Argument Check.
$!
$   ENDIF
$!
$! End The OPT_PHASE Check.
$!
$ ENDIF
$!
$! Check To See If OPT_DEBUG Is Blank.
$!
$ IF (OPT_DEBUG.EQS."NODEBUG")
$ THEN
$!
$!  OPT_DEBUG Is NODEBUG, So Compile Without The Debugger Information.
$!
$   DEBUGGER = "NODEBUG"
$   LINKMAP = "NOMAP"
$   TRACEBACK = "NOTRACEBACK" 
$   GCC_OPTIMIZE = "OPTIMIZE"
$   CC_OPTIMIZE = "OPTIMIZE"
$   MACRO_OPTIMIZE = "OPTIMIZE"
$   WRITE SYS$OUTPUT "No Debugger Information Will Be Produced During Compile."
$   WRITE SYS$OUTPUT "Compiling With Compiler Optimization."
$ ELSE
$!
$!  Check To See If We Are To Compile With Debugger Information.
$!
$   IF (OPT_DEBUG.EQS."DEBUG")
$   THEN
$!
$!    Compile With Debugger Information.
$!
$     DEBUGGER = "DEBUG"
$     LINKMAP = "MAP"
$     TRACEBACK = "TRACEBACK"
$     GCC_OPTIMIZE = "NOOPTIMIZE"
$     CC_OPTIMIZE = "NOOPTIMIZE"
$     MACRO_OPTIMIZE = "NOOPTIMIZE"
$     WRITE SYS$OUTPUT "Debugger Information Will Be Produced During Compile."
$     WRITE SYS$OUTPUT "Compiling Without Compiler Optimization."
$   ELSE 
$!
$!    They Entered An Invalid Option.
$!
$     WRITE SYS$OUTPUT ""
$     WRITE SYS$OUTPUT "The Option ",OPT_DEBUG," Is Invalid.  The Valid Options Are:"
$     WRITE SYS$OUTPUT ""
$     WRITE SYS$OUTPUT "     DEBUG   :  Compile With The Debugger Information."
$     WRITE SYS$OUTPUT "     NODEBUG :  Compile Without The Debugger Information."
$     WRITE SYS$OUTPUT ""
$!
$!    Time To EXIT.
$!
$     EXIT
$!
$!  End The Valid Argument Check.
$!
$   ENDIF
$!
$! End The OPT_DEBUG Check.
$!
$ ENDIF
$!
$! Special Threads For OpenVMS v7.1 Or Later
$!
$! Written By:  Richard Levitte
$!              richard@levitte.org
$!
$!
$! Check To See If We Have A Option For OPT_SPECIAL_THREADS.
$!
$ IF (OPT_SPECIAL_THREADS.EQS."")
$ THEN
$!
$!  Get The Version Of VMS We Are Using.
$!
$   ISSEVEN :=
$   TMP = F$ELEMENT(0,"-",F$EXTRACT(1,4,F$GETSYI("VERSION")))
$   TMP = F$INTEGER(F$ELEMENT(0,".",TMP)+F$ELEMENT(1,".",TMP))
$!
$!  Check To See If The VMS Version Is v7.1 Or Later.
$!
$   IF (TMP.GE.71)
$   THEN
$!
$!    We Have OpenVMS v7.1 Or Later, So Use The Special Threads.
$!
$     ISSEVEN := ,PTHREAD_USE_D4
$!
$!  End The VMS Version Check.
$!
$   ENDIF
$!
$! End The OPT_SPECIAL_THREADS Check.
$!
$ ENDIF
$!
$! Check OPT_POINTER_SIZE (P7).
$!
$ IF (OPT_POINTER_SIZE .NES. "") .AND. (ARCH .NES. "VAX")
$ THEN
$!
$   IF (OPT_POINTER_SIZE .EQS. "32")
$   THEN
$     POINTER_SIZE = " /POINTER_SIZE=32"
$   ELSE
$     POINTER_SIZE = F$EDIT( OPT_POINTER_SIZE, "COLLAPSE, UPCASE")
$     IF ((POINTER_SIZE .EQS. "64") .OR. -
       (POINTER_SIZE .EQS. "64=") .OR. -
       (POINTER_SIZE .EQS. "64=ARGV"))
$     THEN
$       ARCHD = ARCH+ "_64"
$       LIB32 = ""
$       POINTER_SIZE = " /POINTER_SIZE=64"
$     ELSE
$!
$!      Tell The User Entered An Invalid Option.
$!
$       WRITE SYS$OUTPUT ""
$       WRITE SYS$OUTPUT "The Option ", OPT_POINTER_SIZE, -
         " Is Invalid.  The Valid Options Are:"
$       WRITE SYS$OUTPUT ""
$       WRITE SYS$OUTPUT -
         "    """"       :  Compile with default (short) pointers."
$       WRITE SYS$OUTPUT -
         "    32       :  Compile with 32-bit (short) pointers."
$       WRITE SYS$OUTPUT -
         "    64       :  Compile with 64-bit (long) pointers (auto ARGV)."
$       WRITE SYS$OUTPUT -
         "    64=      :  Compile with 64-bit (long) pointers (no ARGV)."
$       WRITE SYS$OUTPUT -
         "    64=ARGV  :  Compile with 64-bit (long) pointers (ARGV)."
$       WRITE SYS$OUTPUT ""
$! 
$!      Time To EXIT.
$!
$       EXIT
$!
$     ENDIF
$!
$   ENDIF
$!
$! End The OPT_POINTER_SIZE Check.
$!
$ ENDIF
$!
$! Set basic C compiler /INCLUDE directories.
$!
$ CC_INCLUDES = "SYS$DISK:[],SYS$DISK:[.VENDOR_DEFNS]"
$!
$! Check To See If OPT_COMPILER Is Blank.
$!
$ IF (OPT_COMPILER.EQS."")
$ THEN
$!
$!  O.K., The User Didn't Specify A Compiler, Let's Try To
$!  Find Out Which One To Use.
$!
$!  Check To See If We Have GNU C.
$!
$   IF (F$TRNLNM("GNU_CC").NES."")
$   THEN
$!
$!    Looks Like GNUC, Set To Use GNUC.
$!
$     OPT_COMPILER = "GNUC"
$!
$!  Else...
$!
$   ELSE
$!
$!    Check To See If We Have VAXC Or DECC.
$!
$     IF (ARCH.NES."VAX").OR.(F$TRNLNM("DECC$CC_DEFAULT").NES."")
$     THEN 
$!
$!      Looks Like DECC, Set To Use DECC.
$!
$       OPT_COMPILER = "DECC"
$!
$!    Else...
$!
$     ELSE
$!
$!      Looks Like VAXC, Set To Use VAXC.
$!
$       OPT_COMPILER = "VAXC"
$!
$!    End The VAXC Compiler Check.
$!
$     ENDIF
$!
$!  End The DECC & VAXC Compiler Check.
$!
$   ENDIF
$!
$!  End The Compiler Check.
$!
$ ENDIF
$!
$! Check To See If We Have A Option For OPT_TCPIP_LIB.
$!
$ IF (OPT_TCPIP_LIB.EQS."")
$ THEN
$!
$!  Find out what socket library we have available
$!
$   IF F$PARSE("SOCKETSHR:") .NES. ""
$   THEN
$!
$!    We have SOCKETSHR, and it is my opinion that it's the best to use.
$!
$     OPT_TCPIP_LIB = "SOCKETSHR"
$!
$!    Tell the user
$!
$     WRITE SYS$OUTPUT "Using SOCKETSHR for TCP/IP"
$!
$!    Else, let's look for something else
$!
$   ELSE
$!
$!    Like UCX (the reason to do this before Multinet is that the UCX
$!    emulation is easier to use...)
$!
$     IF F$TRNLNM("UCX$IPC_SHR") .NES. "" -
	 .OR. F$PARSE("SYS$SHARE:UCX$IPC_SHR.EXE") .NES. "" -
	 .OR. F$PARSE("SYS$LIBRARY:UCX$IPC.OLB") .NES. ""
$     THEN
$!
$!	Last resort: a UCX or UCX-compatible library
$!
$	OPT_TCPIP_LIB = "UCX"
$!
$!      Tell the user
$!
$       WRITE SYS$OUTPUT "Using UCX or an emulation thereof for TCP/IP"
$!
$!	That was all...
$!
$     ENDIF
$   ENDIF
$ ENDIF
$!
$! Set Up Initial CC Definitions, Possibly With User Ones
$!
$ CCDEFS = "TCPIP_TYPE_''OPT_TCPIP_LIB',DSO_VMS"
$ IF F$TYPE(USER_CCDEFS) .NES. "" THEN CCDEFS = CCDEFS + "," + USER_CCDEFS
$ CCEXTRAFLAGS = ""
$ IF F$TYPE(USER_CCFLAGS) .NES. "" THEN CCEXTRAFLAGS = USER_CCFLAGS
$ CCDISABLEWARNINGS = "" !!! "LONGLONGTYPE,LONGLONGSUFX"
$ IF F$TYPE(USER_CCDISABLEWARNINGS) .NES. "" THEN -
	CCDISABLEWARNINGS = CCDISABLEWARNINGS + "," + USER_CCDISABLEWARNINGS
$!
$! Check To See If We Have A ZLIB Option.
$!
$ IF (ZLIB .NES. "")
$ THEN
$!
$!  Check for expected ZLIB files.
$!
$   err = 0
$   file1 = f$parse( "zlib.h", ZLIB, , , "SYNTAX_ONLY")
$   if (f$search( file1) .eqs. "")
$   then
$     WRITE SYS$OUTPUT ""
$     WRITE SYS$OUTPUT "The Option ", ZLIB, " Is Invalid."
$     WRITE SYS$OUTPUT "    Can't find header: ''file1'"
$     err = 1
$   endif
$   file1 = f$parse( "A.;", ZLIB)- "A.;"
$!
$   file2 = f$parse( ZLIB, "libz.olb", , , "SYNTAX_ONLY")
$   if (f$search( file2) .eqs. "")
$   then
$     if (err .eq. 0)
$     then
$       WRITE SYS$OUTPUT ""
$       WRITE SYS$OUTPUT "The Option ", ZLIB, " Is Invalid."
$     endif
$     WRITE SYS$OUTPUT "    Can't find library: ''file2'"
$     WRITE SYS$OUTPUT ""
$     err = err+ 2
$   endif
$   if (err .eq. 1)
$   then
$     WRITE SYS$OUTPUT ""
$   endif
$!
$   if (err .ne. 0)
$   then
$     EXIT
$   endif
$!
$   CCDEFS = """ZLIB=1"", "+ CCDEFS
$   CC_INCLUDES = CC_INCLUDES+ ", "+ file1
$   ZLIB_LIB = ", ''file2' /library"
$!
$!  Print info
$!
$   WRITE SYS$OUTPUT "ZLIB library spec: ", file2
$!
$! End The ZLIB Check.
$!
$ ENDIF
$!
$!  Check To See If The User Entered A Valid Parameter.
$!
$ IF (OPT_COMPILER.EQS."VAXC").OR.(OPT_COMPILER.EQS."DECC").OR.(OPT_COMPILER.EQS."GNUC")
$ THEN
$!
$!    Check To See If The User Wanted DECC.
$!
$   IF (OPT_COMPILER.EQS."DECC")
$   THEN
$!
$!    Looks Like DECC, Set To Use DECC.
$!
$     COMPILER = "DECC"
$!
$!    Tell The User We Are Using DECC.
$!
$     WRITE SYS$OUTPUT "Using DECC 'C' Compiler."
$!
$!    Use DECC...
$!
$     CC = "CC"
$     IF ARCH.EQS."VAX" .AND. F$TRNLNM("DECC$CC_DEFAULT").NES."/DECC" -
	 THEN CC = "CC/DECC"
$     CC = CC + " /''CC_OPTIMIZE' /''DEBUGGER' /STANDARD=RELAXED"+ -
       "''POINTER_SIZE' /NOLIST /PREFIX=ALL" + -
       " /INCLUDE=(''CC_INCLUDES') " + -
       CCEXTRAFLAGS
$!
$!    Define The Linker Options File Name.
$!
$     OPT_FILE = "VAX_DECC_OPTIONS.OPT"
$!
$!  End DECC Check.
$!
$   ENDIF
$!
$!  Check To See If We Are To Use VAXC.
$!
$   IF (OPT_COMPILER.EQS."VAXC")
$   THEN
$!
$!    Looks Like VAXC, Set To Use VAXC.
$!
$     COMPILER = "VAXC"
$!
$!    Tell The User We Are Using VAX C.
$!
$     WRITE SYS$OUTPUT "Using VAXC 'C' Compiler."
$!
$!    Compile Using VAXC.
$!
$     CC = "CC"
$     IF ARCH.NES."VAX"
$     THEN
$	WRITE SYS$OUTPUT "There is no VAX C on Alpha!"
$	EXIT
$     ENDIF
$     IF F$TRNLNM("DECC$CC_DEFAULT").EQS."/DECC" THEN CC = "CC/VAXC"
$     CC = CC + "/''CC_OPTIMIZE'/''DEBUGGER'/NOLIST" + -
	   "/INCLUDE=(''CC_INCLUDES')" + -
	   CCEXTRAFLAGS
$     CCDEFS = """VAXC""," + CCDEFS
$!
$!    Define <sys> As SYS$COMMON:[SYSLIB]
$!
$     DEFINE/NOLOG SYS SYS$COMMON:[SYSLIB]
$!
$!    Define The Linker Options File Name.
$!
$     OPT_FILE = "VAX_VAXC_OPTIONS.OPT"
$!
$!  End VAXC Check
$!
$   ENDIF
$!
$!  Check To See If We Are To Use GNU C.
$!
$   IF (OPT_COMPILER.EQS."GNUC")
$   THEN
$!
$!    Looks Like GNUC, Set To Use GNUC.
$!
$     COMPILER = "GNUC"
$!
$!    Tell The User We Are Using GNUC.
$!
$     WRITE SYS$OUTPUT "Using GNU 'C' Compiler."
$!
$!    Use GNU C...
$!
$     CC = "GCC/NOCASE_HACK/''GCC_OPTIMIZE'/''DEBUGGER'/NOLIST" + -
	   "/INCLUDE=(''CC_INCLUDES')" + -
	   CCEXTRAFLAGS
$!
$!    Define The Linker Options File Name.
$!
$     OPT_FILE = "VAX_GNUC_OPTIONS.OPT"
$!
$!  End The GNU C Check.
$!
$   ENDIF
$!
$!  Set up default defines
$!
$   CCDEFS = """FLAT_INC=1""," + CCDEFS
$!
$!  Finish up the definition of CC.
$!
$   IF COMPILER .EQS. "DECC"
$   THEN
$     IF CCDISABLEWARNINGS .NES. ""
$     THEN
$       CCDISABLEWARNINGS = " /WARNING=(DISABLE=(" + CCDISABLEWARNINGS + "))"
$     ENDIF
$   ELSE
$     CCDISABLEWARNINGS = ""
$   ENDIF
$   CC = CC + " /DEFINE=(" + CCDEFS + ")" + CCDISABLEWARNINGS
$!
$!  Show user the result
$!
$   WRITE/SYMBOL SYS$OUTPUT "Main C Compiling Command: ",CC
$!
$!  Else The User Entered An Invalid Argument.
$!
$ ELSE
$!
$!  Tell The User We Don't Know What They Want.
$!
$   WRITE SYS$OUTPUT ""
$   WRITE SYS$OUTPUT "The Option ",OPT_COMPILER," Is Invalid.  The Valid Options Are:"
$   WRITE SYS$OUTPUT ""
$   WRITE SYS$OUTPUT "    VAXC  :  To Compile With VAX C."
$   WRITE SYS$OUTPUT "    DECC  :  To Compile With DEC C."
$   WRITE SYS$OUTPUT "    GNUC  :  To Compile With GNU C."
$   WRITE SYS$OUTPUT ""
$!
$!  Time To EXIT.
$!
$   EXIT
$!
$! End The Valid Argument Check.
$!
$ ENDIF
$!
$! Build a MACRO command for the architecture at hand
$!
$ IF ARCH .EQS. "VAX"
$ THEN
$   MACRO = "MACRO/''DEBUGGER'"
$ ELSE
$   MACRO = "MACRO/MIGRATION/''DEBUGGER'/''MACRO_OPTIMIZE'"
$ ENDIF
$!
$!  Show user the result
$!
$ WRITE/SYMBOL SYS$OUTPUT "Main MACRO Compiling Command: ",MACRO
$!
$! Time to check the contents, and to make sure we get the correct library.
$!
$ IF OPT_TCPIP_LIB.EQS."SOCKETSHR" .OR. OPT_TCPIP_LIB.EQS."MULTINET" -
     .OR. OPT_TCPIP_LIB.EQS."UCX" .OR. OPT_TCPIP_LIB.EQS."TCPIP" -
     .OR. OPT_TCPIP_LIB.EQS."NONE"
$ THEN
$!
$!  Check to see if SOCKETSHR was chosen
$!
$   IF OPT_TCPIP_LIB.EQS."SOCKETSHR"
$   THEN
$!
$!    Set the library to use SOCKETSHR
$!
$     TCPIP_LIB = ",SYS$DISK:[-.VMS]SOCKETSHR_SHR.OPT /OPTIONS"
$!
$!    Done with SOCKETSHR
$!
$   ENDIF
$!
$!  Check to see if MULTINET was chosen
$!
$   IF OPT_TCPIP_LIB.EQS."MULTINET"
$   THEN
$!
$!    Set the library to use UCX emulation.
$!
$     OPT_TCPIP_LIB = "UCX"
$!
$!    Done with MULTINET
$!
$   ENDIF
$!
$!  Check to see if UCX was chosen
$!
$   IF OPT_TCPIP_LIB.EQS."UCX"
$   THEN
$!
$!    Set the library to use UCX.
$!
$     TCPIP_LIB = "SYS$DISK:[-.VMS]UCX_SHR_DECC.OPT /OPTIONS"
$     IF F$TRNLNM("UCX$IPC_SHR") .NES. ""
$     THEN
$       TCPIP_LIB = ",SYS$DISK:[-.VMS]UCX_SHR_DECC_LOG.OPT /OPTIONS"
$     ELSE
$       IF COMPILER .NES. "DECC" .AND. ARCH .EQS. "VAX" THEN -
	  TCPIP_LIB = ",SYS$DISK:[-.VMS]UCX_SHR_VAXC.OPT /OPTIONS"
$     ENDIF
$!
$!    Done with UCX
$!
$   ENDIF
$!
$!  Check to see if TCPIP was chosen
$!
$   IF OPT_TCPIP_LIB.EQS."TCPIP"
$   THEN
$!
$!    Set the library to use TCPIP (post UCX).
$!
$     TCPIP_LIB = ",SYS$DISK:[-.VMS]TCPIP_SHR_DECC.OPT /OPTIONS"
$!
$!    Done with TCPIP
$!
$   ENDIF
$!
$!  Check to see if NONE was chosen
$!
$   IF OPT_TCPIP_LIB.EQS."NONE"
$   THEN
$!
$!    Do not use a TCPIP library.
$!
$     TCPIP_LIB = ""
$!
$!    Done with TCPIP
$!
$   ENDIF
$!
$!  Print info
$!
$   WRITE SYS$OUTPUT "TCP/IP library spec: ", TCPIP_LIB- ","
$!
$!  Else The User Entered An Invalid Argument.
$!
$ ELSE
$!
$!  Tell The User We Don't Know What They Want.
$!
$   WRITE SYS$OUTPUT ""
$   WRITE SYS$OUTPUT "The Option ",OPT_TCPIP_LIB," Is Invalid.  The Valid Options Are:"
$   WRITE SYS$OUTPUT ""
$   WRITE SYS$OUTPUT "    SOCKETSHR  :  To link with SOCKETSHR TCP/IP library."
$   WRITE SYS$OUTPUT "    UCX        :  To link with UCX TCP/IP library."
$   WRITE SYS$OUTPUT "    TCPIP      :  To link with TCPIP (post UCX) TCP/IP library."
$   WRITE SYS$OUTPUT ""
$!
$!  Time To EXIT.
$!
$   EXIT
$!
$!  Done with TCP/IP libraries
$!
$ ENDIF
$!
$!  Time To RETURN...
$!
$ RETURN
$!
$ INITIALISE:
$!
$! Save old value of the logical name OPENSSL
$!
$ __SAVE_OPENSSL = F$TRNLNM("OPENSSL","LNM$PROCESS_TABLE")
$!
$! Save directory information
$!
$ __HERE = F$PARSE(F$PARSE("A.;",F$ENVIRONMENT("PROCEDURE"))-"A.;","[]A.;") - "A.;"
$ __HERE = F$EDIT(__HERE,"UPCASE")
$ __TOP = __HERE - "ENGINES]"
$ __INCLUDE = __TOP + "INCLUDE.OPENSSL]"
$!
$! Set up the logical name OPENSSL to point at the include directory
$!
$ DEFINE OPENSSL /NOLOG '__INCLUDE'
$!
$! Done
$!
$ RETURN
$!
$ CLEANUP:
$!
$! Restore the saved logical name OPENSSL, if it had a value.
$!
$ if (f$type( __SAVE_OPENSSL) .nes. "")
$ then
$   IF __SAVE_OPENSSL .EQS. ""
$   THEN
$     DEASSIGN OPENSSL
$   ELSE
$     DEFINE /NOLOG OPENSSL '__SAVE_OPENSSL'
$   ENDIF
$ endif
$!
$! Close any open files.
$!
$ if (f$trnlnm( "objects", "LNM$PROCESS", 0, "SUPERVISOR") .nes. "") then -
   close objects
$!
$! Done
$!
$ RETURN
$!
