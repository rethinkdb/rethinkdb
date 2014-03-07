/******************************************************************************
 *   Copyright (C) 2000-2010, International Business Machines
 *   Corporation and others.  All Rights Reserved.
 *******************************************************************************
 *   file name:  pkgdata.c
 *   encoding:   ANSI X3.4 (1968)
 *   tab size:   8 (not used)
 *   indentation:4
 *
 *   created on: 2000may15
 *   created by: Steven \u24C7 Loomis
 *
 *   This program packages the ICU data into different forms
 *   (DLL, common data, etc.)
 */

/*
 * We define _XOPEN_SOURCE so that we can get popen and pclose.
 */
#if !defined(_XOPEN_SOURCE)
#if __STDC_VERSION__ >= 199901L
/* It is invalid to compile an XPG3, XPG4, XPG4v2 or XPG5 application using c99 on Solaris */
#define _XOPEN_SOURCE 600
#else
#define _XOPEN_SOURCE 4
#endif
#endif


#include "unicode/utypes.h"

#if U_HAVE_POPEN
#if defined(U_CYGWIN) && defined(__STRICT_ANSI__)
/* popen/pclose aren't defined in strict ANSI on Cygwin */
#undef __STRICT_ANSI__
#endif
#endif

#include "unicode/putil.h"
#include "cmemory.h"
#include "cstring.h"
#include "filestrm.h"
#include "toolutil.h"
#include "unicode/uclean.h"
#include "unewdata.h"
#include "uoptions.h"
#include "putilimp.h"
#include "package.h"
#include "pkg_icu.h"
#include "pkg_genc.h"
#include "pkg_gencmn.h"
#include "flagparser.h"
#include "filetools.h"


#if U_HAVE_POPEN
# include <unistd.h>
#endif
#include <stdio.h>
#include <stdlib.h>

U_CDECL_BEGIN
#include "pkgtypes.h"
U_CDECL_END

#ifdef U_WINDOWS
#ifdef __GNUC__
#define WINDOWS_WITH_GNUC
#else
#define WINDOWS_WITH_MSVC
#endif
#endif
#if !defined(WINDOWS_WITH_MSVC) && !defined(U_LINUX)
#define BUILD_DATA_WITHOUT_ASSEMBLY
#endif
#if defined(WINDOWS_WITH_MSVC) || defined(U_LINUX)
#define CAN_WRITE_OBJ_CODE
#endif
#if defined(U_CYGWIN) || defined(CYGWINMSVC)
#define USING_CYGWIN
#endif

/*
 * When building the data library without assembly,
 * some platforms use a single c code file for all of
 * the data to generate the final data library. This can
 * increase the performance of the pkdata tool.
 */
#if defined(OS400)
#define USE_SINGLE_CCODE_FILE
#endif

/* Need to fix the file seperator character when using MinGW. */
#if defined(WINDOWS_WITH_GNUC) || defined(USING_CYGWIN)
#define PKGDATA_FILE_SEP_STRING "/"
#else
#define PKGDATA_FILE_SEP_STRING U_FILE_SEP_STRING
#endif

#define LARGE_BUFFER_MAX_SIZE 2048
#define SMALL_BUFFER_MAX_SIZE 512

static void loadLists(UPKGOptions *o, UErrorCode *status);

static int32_t pkg_executeOptions(UPKGOptions *o);

#ifdef WINDOWS_WITH_MSVC
static int32_t pkg_createWindowsDLL(const char mode, const char *gencFilePath, UPKGOptions *o);
#endif
static int32_t pkg_createSymLinks(const char *targetDir, UBool specialHandling=FALSE);
static int32_t pkg_installLibrary(const char *installDir, const char *dir);
static int32_t pkg_installFileMode(const char *installDir, const char *srcDir, const char *fileListName);
static int32_t pkg_installCommonMode(const char *installDir, const char *fileName);

#ifdef BUILD_DATA_WITHOUT_ASSEMBLY
static int32_t pkg_createWithoutAssemblyCode(UPKGOptions *o, const char *targetDir, const char mode);
#endif

static int32_t pkg_createWithAssemblyCode(const char *targetDir, const char mode, const char *gencFilePath);
static int32_t pkg_generateLibraryFile(const char *targetDir, const char mode, const char *objectFile, char *command = NULL);
static int32_t pkg_archiveLibrary(const char *targetDir, const char *version, UBool reverseExt);
static void createFileNames(UPKGOptions *o, const char mode, const char *version_major, const char *version, const char *libName, const UBool reverseExt);
static int32_t initializePkgDataFlags(UPKGOptions *o);

static int32_t pkg_getOptionsFromICUConfig(UBool verbose, UOption *option);
static int runCommand(const char* command, UBool specialHandling=FALSE);

enum {
    NAME,
    BLDOPT,
    MODE,
    HELP,
    HELP_QUESTION_MARK,
    VERBOSE,
    COPYRIGHT,
    COMMENT,
    DESTDIR,
    REBUILD,
    TEMPDIR,
    INSTALL,
    SOURCEDIR,
    ENTRYPOINT,
    REVISION,
    FORCE_PREFIX,
    LIBNAME,
    QUIET
};

/* This sets the modes that are available */
static struct {
    const char *name, *alt_name;
    const char *desc;
} modes[] = {
        { "files", 0,           "Uses raw data files (no effect). Installation copies all files to the target location." },
#ifdef U_WINDOWS
        { "dll",    "library",  "Generates one common data file and one shared library, <package>.dll"},
        { "common", "archive",  "Generates just the common file, <package>.dat"},
        { "static", "static",   "Generates one statically linked library, " LIB_PREFIX "<package>" UDATA_LIB_SUFFIX }
#else
#ifdef UDATA_SO_SUFFIX
        { "dll",    "library",  "Generates one shared library, <package>" UDATA_SO_SUFFIX },
#endif
        { "common", "archive",  "Generates one common data file, <package>.dat" },
        { "static", "static",   "Generates one statically linked library, " LIB_PREFIX "<package>" UDATA_LIB_SUFFIX }
#endif
};

static UOption options[]={
    /*00*/    UOPTION_DEF( "name",    'p', UOPT_REQUIRES_ARG),
    /*01*/    UOPTION_DEF( "bldopt",  'O', UOPT_REQUIRES_ARG), /* on Win32 it is release or debug */
    /*02*/    UOPTION_DEF( "mode",    'm', UOPT_REQUIRES_ARG),
    /*03*/    UOPTION_HELP_H,                                   /* -h */
    /*04*/    UOPTION_HELP_QUESTION_MARK,                       /* -? */
    /*05*/    UOPTION_VERBOSE,                                  /* -v */
    /*06*/    UOPTION_COPYRIGHT,                                /* -c */
    /*07*/    UOPTION_DEF( "comment", 'C', UOPT_REQUIRES_ARG),
    /*08*/    UOPTION_DESTDIR,                                  /* -d */
    /*11*/    UOPTION_DEF( "rebuild", 'F', UOPT_NO_ARG),
    /*12*/    UOPTION_DEF( "tempdir", 'T', UOPT_REQUIRES_ARG),
    /*13*/    UOPTION_DEF( "install", 'I', UOPT_REQUIRES_ARG),
    /*14*/    UOPTION_SOURCEDIR ,
    /*15*/    UOPTION_DEF( "entrypoint", 'e', UOPT_REQUIRES_ARG),
    /*16*/    UOPTION_DEF( "revision", 'r', UOPT_REQUIRES_ARG),
    /*17*/    UOPTION_DEF( "force-prefix", 'f', UOPT_NO_ARG),
    /*18*/    UOPTION_DEF( "libname", 'L', UOPT_REQUIRES_ARG),
    /*19*/    UOPTION_DEF( "quiet", 'q', UOPT_NO_ARG)
};

enum {
    GENCCODE_ASSEMBLY_TYPE,
    SO_EXT,
    SOBJ_EXT,
    A_EXT,
    LIBPREFIX,
    LIB_EXT_ORDER,
    COMPILER,
    LIBFLAGS,
    GENLIB,
    LDICUDTFLAGS,
    LD_SONAME,
    RPATH_FLAGS,
    BIR_FLAGS,
    AR,
    ARFLAGS,
    RANLIB,
    INSTALL_CMD,
    PKGDATA_FLAGS_SIZE
};
static char **pkgDataFlags = NULL;

enum {
    LIB_FILE,
    LIB_FILE_VERSION_MAJOR,
    LIB_FILE_VERSION,
    LIB_FILE_VERSION_TMP,
#ifdef U_CYGWIN
    LIB_FILE_CYGWIN,
	LIB_FILE_CYGWIN_VERSION,
#endif
    LIB_FILENAMES_SIZE
};
static char libFileNames[LIB_FILENAMES_SIZE][256];

static UPKGOptions  *pkg_checkFlag(UPKGOptions *o);

const char options_help[][320]={
    "Set the data name",
#ifdef U_MAKE_IS_NMAKE
    "The directory where the ICU is located (e.g. <ICUROOT> which contains the bin directory)",
#else
    "Specify options for the builder.",
#endif
    "Specify the mode of building (see below; default: common)",
    "This usage text",
    "This usage text",
    "Make the output verbose",
    "Use the standard ICU copyright",
    "Use a custom comment (instead of the copyright)",
    "Specify the destination directory for files",
    "Force rebuilding of all data",
    "Specify temporary dir (default: output dir)",
    "Install the data (specify target)",
    "Specify a custom source directory",
    "Specify a custom entrypoint name (default: short name)",
    "Specify a version when packaging in dll or static mode",
    "Add package to all file names if not present",
    "Library name to build (if different than package name)",
    "Quite mode. (e.g. Do not output a readme file for static libraries)"
};

const char  *progname = "PKGDATA";

int
main(int argc, char* argv[]) {
    int result = 0;
    /* FileStream  *out; */
    UPKGOptions  o;
    CharList    *tail;
    UBool        needsHelp = FALSE;
    UErrorCode   status = U_ZERO_ERROR;
    /* char         tmp[1024]; */
    uint32_t i;
    int32_t n;

    U_MAIN_INIT_ARGS(argc, argv);

    progname = argv[0];

    options[MODE].value = "common";

    /* read command line options */
    argc=u_parseArgs(argc, argv, sizeof(options)/sizeof(options[0]), options);

    /* error handling, printing usage message */
    /* I've decided to simply print an error and quit. This tool has too
    many options to just display them all of the time. */

    if(options[HELP].doesOccur || options[HELP_QUESTION_MARK].doesOccur) {
        needsHelp = TRUE;
    }
    else {
        if(!needsHelp && argc<0) {
            fprintf(stderr,
                "%s: error in command line argument \"%s\"\n",
                progname,
                argv[-argc]);
            fprintf(stderr, "Run '%s --help' for help.\n", progname);
            return 1;
        }


#if !defined(WINDOWS_WITH_MSVC) || defined(USING_CYGWIN)
        if(!options[BLDOPT].doesOccur && uprv_strcmp(options[MODE].value, "common") != 0) {
          if (pkg_getOptionsFromICUConfig(options[VERBOSE].doesOccur, &options[BLDOPT]) != 0) {
                fprintf(stderr, " required parameter is missing: -O is required for static and shared builds.\n");
                fprintf(stderr, "Run '%s --help' for help.\n", progname);
                return 1;
            }
        }
#else
        if(options[BLDOPT].doesOccur) {
            fprintf(stdout, "Warning: You are using the -O option which is not needed for MSVC build on Windows.\n");
        }
#endif

        if(!options[NAME].doesOccur) /* -O we already have - don't report it. */
        {
            fprintf(stderr, " required parameter -p is missing \n");
            fprintf(stderr, "Run '%s --help' for help.\n", progname);
            return 1;
        }

        if(argc == 1) {
            fprintf(stderr,
                "No input files specified.\n"
                "Run '%s --help' for help.\n", progname);
            return 1;
        }
    }   /* end !needsHelp */

    if(argc<0 || needsHelp  ) {
        fprintf(stderr,
            "usage: %s [-options] [-] [packageFile] \n"
            "\tProduce packaged ICU data from the given list(s) of files.\n"
            "\t'-' by itself means to read from stdin.\n"
            "\tpackageFile is a text file containing the list of files to package.\n",
            progname);

        fprintf(stderr, "\n options:\n");
        for(i=0;i<(sizeof(options)/sizeof(options[0]));i++) {
            fprintf(stderr, "%-5s -%c %s%-10s  %s\n",
                (i<1?"[REQ]":""),
                options[i].shortName,
                options[i].longName ? "or --" : "     ",
                options[i].longName ? options[i].longName : "",
                options_help[i]);
        }

        fprintf(stderr, "modes: (-m option)\n");
        for(i=0;i<(sizeof(modes)/sizeof(modes[0]));i++) {
            fprintf(stderr, "   %-9s ", modes[i].name);
            if (modes[i].alt_name) {
                fprintf(stderr, "/ %-9s", modes[i].alt_name);
            } else {
                fprintf(stderr, "           ");
            }
            fprintf(stderr, "  %s\n", modes[i].desc);
        }
        return 1;
    }

    /* OK, fill in the options struct */
    uprv_memset(&o, 0, sizeof(o));

    o.mode      = options[MODE].value;
    o.version   = options[REVISION].doesOccur ? options[REVISION].value : 0;

    o.shortName = options[NAME].value;
    {
        int32_t len = (int32_t)uprv_strlen(o.shortName);
        char *csname, *cp;
        const char *sp;

        cp = csname = (char *) uprv_malloc((len + 1 + 1) * sizeof(*o.cShortName));
        if (*(sp = o.shortName)) {
            *cp++ = isalpha(*sp) ? * sp : '_';
            for (++sp; *sp; ++sp) {
                *cp++ = isalnum(*sp) ? *sp : '_';
            }
        }
        *cp = 0;

        o.cShortName = csname;
    }

    if(options[LIBNAME].doesOccur) { /* get libname from shortname, or explicit -L parameter */
      o.libName = options[LIBNAME].value;
    } else {
      o.libName = o.shortName;
    }

    if(options[QUIET].doesOccur) {
      o.quiet = TRUE;
    } else {
      o.quiet = FALSE;
    }

    o.verbose   = options[VERBOSE].doesOccur;


#if !defined(WINDOWS_WITH_MSVC) || defined(USING_CYGWIN) /* on UNIX, we'll just include the file... */
    if (options[BLDOPT].doesOccur) {
        o.options   = options[BLDOPT].value;
    } else {
        o.options = NULL;
    }
#endif
    if(options[COPYRIGHT].doesOccur) {
        o.comment = U_COPYRIGHT_STRING;
    } else if (options[COMMENT].doesOccur) {
        o.comment = options[COMMENT].value;
    }

    if( options[DESTDIR].doesOccur ) {
        o.targetDir = options[DESTDIR].value;
    } else {
        o.targetDir = ".";  /* cwd */
    }

    o.rebuild   = options[REBUILD].doesOccur;

    if( options[TEMPDIR].doesOccur ) {
        o.tmpDir    = options[TEMPDIR].value;
    } else {
        o.tmpDir    = o.targetDir;
    }

    if( options[INSTALL].doesOccur ) {
        o.install  = options[INSTALL].value;
    } else {
        o.install = NULL;
    }

    if( options[SOURCEDIR].doesOccur ) {
        o.srcDir   = options[SOURCEDIR].value;
    } else {
        o.srcDir   = ".";
    }

    if( options[ENTRYPOINT].doesOccur ) {
        o.entryName = options[ENTRYPOINT].value;
    } else {
        o.entryName = o.cShortName;
    }

    /* OK options are set up. Now the file lists. */
    tail = NULL;
    for( n=1; n<argc; n++) {
        o.fileListFiles = pkg_appendToList(o.fileListFiles, &tail, uprv_strdup(argv[n]));
    }

    /* load the files */
    loadLists(&o, &status);
    if( U_FAILURE(status) ) {
        fprintf(stderr, "error loading input file lists: %s\n", u_errorName(status));
        return 2;
    }

    result = pkg_executeOptions(&o);

    if (pkgDataFlags != NULL) {
        for (n = 0; n < PKGDATA_FLAGS_SIZE; n++) {
            if (pkgDataFlags[n] != NULL) {
                uprv_free(pkgDataFlags[n]);
            }
        }
        uprv_free(pkgDataFlags);
    }

    if (o.cShortName != NULL) {
        uprv_free((char *)o.cShortName);
    }
    if (o.fileListFiles != NULL) {
        pkg_deleteList(o.fileListFiles);
    }
    if (o.filePaths != NULL) {
        pkg_deleteList(o.filePaths);
    }
    if (o.files != NULL) {
        pkg_deleteList(o.files);
    }

    return result;
}

static int runCommand(const char* command, UBool specialHandling) {
    char *cmd = NULL;
    char cmdBuffer[SMALL_BUFFER_MAX_SIZE];
    int32_t len = strlen(command);

    if (len == 0) {
        return 0;
    }

    if (!specialHandling) {
#if defined(USING_CYGWIN) || defined(OS400)
#define CMD_PADDING_SIZE 20
        if ((len + CMD_PADDING_SIZE) >= SMALL_BUFFER_MAX_SIZE) {
            cmd = (char *)uprv_malloc(len + CMD_PADDING_SIZE);
        } else {
            cmd = cmdBuffer;
        }
#ifdef USING_CYGWIN
        sprintf(cmd, "bash -c \"%s\"", command);

#elif defined(OS400)
        sprintf(cmd, "QSH CMD('%s')", command);
#endif
#else
        goto normal_command_mode;
#endif
    } else {
normal_command_mode:
        cmd = (char *)command;
    }

    printf("pkgdata: %s\n", cmd);
    int result = system(cmd);
    if (result != 0) {
        printf("-- return status = %d\n", result);
    }

    if (cmd != cmdBuffer && cmd != command) {
        uprv_free(cmd);
    }

    return result;
}

#define LN_CMD "ln -s"
#define RM_CMD "rm -f"

#define MODE_COMMON 'c'
#define MODE_STATIC 's'
#define MODE_DLL    'd'
#define MODE_FILES  'f'

static int32_t pkg_executeOptions(UPKGOptions *o) {
    int32_t result = 0;

    const char mode = o->mode[0];
    char targetDir[SMALL_BUFFER_MAX_SIZE] = "";
    char tmpDir[SMALL_BUFFER_MAX_SIZE] = "";
    char datFileName[SMALL_BUFFER_MAX_SIZE] = "";
    char datFileNamePath[LARGE_BUFFER_MAX_SIZE] = "";
    char checkLibFile[LARGE_BUFFER_MAX_SIZE] = "";

    initializePkgDataFlags(o);

    if (mode == MODE_FILES) {
        /* Copy the raw data to the installation directory. */
        if (o->install != NULL) {
            uprv_strcpy(targetDir, o->install);
            if (o->shortName != NULL) {
                uprv_strcat(targetDir, PKGDATA_FILE_SEP_STRING);
                uprv_strcat(targetDir, o->shortName);
            }
            
            if(o->verbose) {
              fprintf(stdout, "# Install: Files mode, copying files to %s..\n", targetDir);
            }
            result = pkg_installFileMode(targetDir, o->srcDir, o->fileListFiles->str);
        }
        return result;
    } else /* if (mode == MODE_COMMON || mode == MODE_STATIC || mode == MODE_DLL) */ {
        uprv_strcpy(targetDir, o->targetDir);
        uprv_strcat(targetDir, PKGDATA_FILE_SEP_STRING);

        uprv_strcpy(tmpDir, o->tmpDir);
        uprv_strcat(tmpDir, PKGDATA_FILE_SEP_STRING);

        uprv_strcpy(datFileNamePath, tmpDir);

        uprv_strcpy(datFileName, o->shortName);
        uprv_strcat(datFileName, UDATA_CMN_SUFFIX);

        uprv_strcat(datFileNamePath, datFileName);

        if(o->verbose) {
          fprintf(stdout, "# Writing package file %s ..\n", datFileNamePath);
        }
        result = writePackageDatFile(datFileNamePath, o->comment, o->srcDir, o->fileListFiles->str, NULL, U_CHARSET_FAMILY ? 'e' :  U_IS_BIG_ENDIAN ? 'b' : 'l');
        if (result != 0) {
            fprintf(stderr,"Error writing package dat file.\n");
            return result;
        }

        if (mode == MODE_COMMON) {
            char targetFileNamePath[LARGE_BUFFER_MAX_SIZE] = "";

            uprv_strcpy(targetFileNamePath, targetDir);
            uprv_strcat(targetFileNamePath, datFileName);

            if (T_FileStream_file_exists(targetFileNamePath)) {
                if ((result = remove(targetFileNamePath)) != 0) {
                    fprintf(stderr, "Unable to remove old dat file: %s\n", targetFileNamePath);
                    return result;
                }
            }

            /* Move the dat file created to the target directory. */
            result = rename(datFileNamePath, targetFileNamePath);

            if(o->verbose) {
              fprintf(stdout, "# Moving package file to %s ..\n", targetFileNamePath);
            }
            if (result != 0) {
                fprintf(stderr, "Unable to move dat file (%s) to target location (%s).\n", datFileNamePath, targetFileNamePath);
            }

            if (o->install != NULL) {
                result = pkg_installCommonMode(o->install, targetFileNamePath);
            }

            return result;
        } else /* if (mode[0] == MODE_STATIC || mode[0] == MODE_DLL) */ {
            char gencFilePath[SMALL_BUFFER_MAX_SIZE] = "";
            char version_major[10] = "";
            UBool reverseExt = FALSE;

#if !defined(WINDOWS_WITH_MSVC) || defined(USING_CYGWIN)
            /* Get the version major number. */
            if (o->version != NULL) {
                for (uint32_t i = 0;i < sizeof(version_major);i++) {
                    if (o->version[i] == '.') {
                        version_major[i] = 0;
                        break;
                    }
                    version_major[i] = o->version[i];
                }
            }

#ifndef OS400
            /* Certain platforms have different library extension ordering. (e.g. libicudata.##.so vs libicudata.so.##)
             * reverseExt is FALSE if the suffix should be the version number.
             */
            if (pkgDataFlags[LIB_EXT_ORDER][uprv_strlen(pkgDataFlags[LIB_EXT_ORDER])-1] == pkgDataFlags[SO_EXT][uprv_strlen(pkgDataFlags[SO_EXT])-1]) {
                reverseExt = TRUE;
            }
#endif
            /* Using the base libName and version number, generate the library file names. */
            createFileNames(o, mode, version_major, o->version, o->libName, reverseExt);

            if ((o->version!=NULL || (mode==MODE_STATIC)) && o->rebuild == FALSE) {
                /* Check to see if a previous built data library file exists and check if it is the latest. */
                sprintf(checkLibFile, "%s%s", targetDir, libFileNames[LIB_FILE_VERSION]);
                if (T_FileStream_file_exists(checkLibFile)) {
                    if (isFileModTimeLater(checkLibFile, o->srcDir, TRUE) && isFileModTimeLater(checkLibFile, o->options)) {
                        if (o->install != NULL) {
                          if(o->verbose) {
                            fprintf(stdout, "# Installing already-built library into %s\n", o->install);
                          }
                          result = pkg_installLibrary(o->install, targetDir);
                        } else {
                          if(o->verbose) {
                            printf("# Not rebuilding %s - up to date.\n", checkLibFile);
                          }
                        }
                        return result;
                    } else if (o->verbose && (o->install!=NULL)) {
                      fprintf(stdout, "# Not installing up-to-date library %s into %s\n", checkLibFile, o->install);
                    }
                } else if(o->verbose && (o->install!=NULL)) {
                  fprintf(stdout, "# Not installing missing %s into %s\n", checkLibFile, o->install);
                }
            }

            pkg_checkFlag(o);
#endif

            if (pkgDataFlags[GENCCODE_ASSEMBLY_TYPE][0] != 0) {
                const char* genccodeAssembly = pkgDataFlags[GENCCODE_ASSEMBLY_TYPE];

                if(o->verbose) {
                  fprintf(stdout, "# Generating assembly code %s of type %s ..\n", gencFilePath, genccodeAssembly);
                }
                
                /* Offset genccodeAssembly by 3 because "-a " */
                if (genccodeAssembly &&
                    (uprv_strlen(genccodeAssembly)>3) &&
                    checkAssemblyHeaderName(genccodeAssembly+3)) {
                    writeAssemblyCode(datFileNamePath, o->tmpDir, o->entryName, NULL, gencFilePath);

                    result = pkg_createWithAssemblyCode(targetDir, mode, gencFilePath);
                    if (result != 0) {
                        fprintf(stderr, "Error generating assembly code for data.\n");
                        return result;
                    } else if (mode == MODE_STATIC) {
                      if(o->install != NULL) {
                        if(o->verbose) {
                          fprintf(stdout, "# Installing static library into %s\n", o->install);
                        }
                        result = pkg_installLibrary(o->install, targetDir);
                      }
                      return result;
                    }
                } else {
                    fprintf(stderr,"Assembly type \"%s\" is unknown.\n", genccodeAssembly);
                    return -1;
                }
            } else {
                if(o->verbose) {
                  fprintf(stdout, "# Writing object code to %s ..\n", gencFilePath);
                }
#ifdef CAN_WRITE_OBJ_CODE
                writeObjectCode(datFileNamePath, o->tmpDir, o->entryName, NULL, NULL, gencFilePath);
#ifdef U_LINUX
                result = pkg_generateLibraryFile(targetDir, mode, gencFilePath);
#elif defined(WINDOWS_WITH_MSVC)
                result = pkg_createWindowsDLL(mode, gencFilePath, o);
#endif
#elif defined(BUILD_DATA_WITHOUT_ASSEMBLY)
                result = pkg_createWithoutAssemblyCode(o, targetDir, mode);
#endif
                if (result != 0) {
                    fprintf(stderr, "Error generating package data.\n");
                    return result;
                }
            }
#ifndef U_WINDOWS
            if(mode != MODE_STATIC) {
                /* Certain platforms uses archive library. (e.g. AIX) */
                if(o->verbose) {
                  fprintf(stdout, "# Creating data archive library file ..\n");
                }
                result = pkg_archiveLibrary(targetDir, o->version, reverseExt);
                if (result != 0) {
                    fprintf(stderr, "Error creating data archive library file.\n");
                   return result;
                }
#ifndef OS400
                /* Create symbolic links for the final library file. */
                result = pkg_createSymLinks(targetDir);
                if (result != 0) {
                    fprintf(stderr, "Error creating symbolic links of the data library file.\n");
                    return result;
                }
#endif
            } /* !MODE_STATIC */
#endif

#if !defined(U_WINDOWS) || defined(USING_CYGWIN)
            /* Install the libraries if option was set. */
            if (o->install != NULL) {
                if(o->verbose) {
                  fprintf(stdout, "# Installing library file to %s ..\n", o->install);
                }
                result = pkg_installLibrary(o->install, targetDir);
                if (result != 0) {
                    fprintf(stderr, "Error installing the data library.\n");
                    return result;
                }
            }
#endif
        }
    }
    return result;
}

/* Initialize the pkgDataFlags with the option file given. */
static int32_t initializePkgDataFlags(UPKGOptions *o) {
    UErrorCode status = U_ZERO_ERROR;
    int32_t result = 0;
    int32_t currentBufferSize = SMALL_BUFFER_MAX_SIZE;
    int32_t tmpResult = 0;

    /* Initialize pkgdataFlags */
    pkgDataFlags = (char**)uprv_malloc(sizeof(char*) * PKGDATA_FLAGS_SIZE);

    /* If we run out of space, allocate more */
#if !defined(WINDOWS_WITH_MSVC) || defined(USING_CYGWIN)
    do {
#endif
        if (pkgDataFlags != NULL) {
            for (int32_t i = 0; i < PKGDATA_FLAGS_SIZE; i++) {
                pkgDataFlags[i] = (char*)uprv_malloc(sizeof(char) * currentBufferSize);
                if (pkgDataFlags[i] != NULL) {
                    pkgDataFlags[i][0] = 0;
                } else {
                    fprintf(stderr,"Error allocating memory for pkgDataFlags.\n");
                    return -1;
                }
            }
        } else {
            fprintf(stderr,"Error allocating memory for pkgDataFlags.\n");
            return -1;
        }

        if (o->options == NULL) {
            return result;
        }

#if !defined(WINDOWS_WITH_MSVC) || defined(USING_CYGWIN)
        /* Read in options file. */
        if(o->verbose) {
          fprintf(stdout, "# Reading options file %s\n", o->options);
        }
        status = U_ZERO_ERROR;
        tmpResult = parseFlagsFile(o->options, pkgDataFlags, currentBufferSize, (int32_t)PKGDATA_FLAGS_SIZE, &status);
        if (status == U_BUFFER_OVERFLOW_ERROR) {
            for (int32_t i = 0; i < PKGDATA_FLAGS_SIZE; i++) {
                uprv_free(pkgDataFlags[i]);
            }
            currentBufferSize = tmpResult;
        } else if (U_FAILURE(status)) {
            fprintf(stderr,"Unable to open or read \"%s\" option file. status = %s\n", o->options, u_errorName(status));
            return -1;
        }
#endif
        if(o->verbose) {
            fprintf(stdout, "# pkgDataFlags=");
            for(int32_t i=0;i<PKGDATA_FLAGS_SIZE && pkgDataFlags[i][0];i++) {
                fprintf(stdout, "%c \"%s\"", (i>0)?',':' ',pkgDataFlags[i]);
            }
            fprintf(stdout, "\n");
        }
#if !defined(WINDOWS_WITH_MSVC) || defined(USING_CYGWIN)
    } while (status == U_BUFFER_OVERFLOW_ERROR);
#endif

    return result;
}


/*
 * Given the base libName and version numbers, generate the libary file names and store it in libFileNames.
 * Depending on the configuration, the library name may either end with version number or shared object suffix.
 */
static void createFileNames(UPKGOptions *o, const char mode, const char *version_major, const char *version, const char *libName, UBool reverseExt) {
        sprintf(libFileNames[LIB_FILE], "%s%s",
                pkgDataFlags[LIBPREFIX],
                libName);

        if(o->verbose) {
          fprintf(stdout, "# libFileName[LIB_FILE] = %s\n", libFileNames[LIB_FILE]);
        }

        if (version != NULL) {
#ifdef U_CYGWIN
            sprintf(libFileNames[LIB_FILE_CYGWIN], "cyg%s.%s",
                    libName,
                    pkgDataFlags[SO_EXT]);
            sprintf(libFileNames[LIB_FILE_CYGWIN_VERSION], "cyg%s%s.%s",
                    libName,
                    version_major,
                    pkgDataFlags[SO_EXT]);

            uprv_strcat(pkgDataFlags[SO_EXT], ".");
            uprv_strcat(pkgDataFlags[SO_EXT], pkgDataFlags[A_EXT]);

#elif defined(OS400) || defined(_AIX)
            sprintf(libFileNames[LIB_FILE_VERSION_TMP], "%s.%s",
                    libFileNames[LIB_FILE],
                    pkgDataFlags[SOBJ_EXT]);
#else
            sprintf(libFileNames[LIB_FILE_VERSION_TMP], "%s%s%s.%s",
                    libFileNames[LIB_FILE],
                    pkgDataFlags[LIB_EXT_ORDER][0] == '.' ? "." : "",
                    reverseExt ? version : pkgDataFlags[SOBJ_EXT],
                    reverseExt ? pkgDataFlags[SOBJ_EXT] : version);
#endif
            sprintf(libFileNames[LIB_FILE_VERSION_MAJOR], "%s%s%s.%s",
                    libFileNames[LIB_FILE],
                    pkgDataFlags[LIB_EXT_ORDER][0] == '.' ? "." : "",
                    reverseExt ? version_major : pkgDataFlags[SO_EXT],
                    reverseExt ? pkgDataFlags[SO_EXT] : version_major);

            sprintf(libFileNames[LIB_FILE_VERSION], "%s%s%s.%s",
                    libFileNames[LIB_FILE],
                    pkgDataFlags[LIB_EXT_ORDER][0] == '.' ? "." : "",
                    reverseExt ? version : pkgDataFlags[SO_EXT],
                    reverseExt ? pkgDataFlags[SO_EXT] : version);

            if(o->verbose) {
              fprintf(stdout, "# libFileName[LIB_FILE_VERSION] = %s\n", libFileNames[LIB_FILE_VERSION]);
            }

#ifdef U_CYGWIN
            /* Cygwin only deals with the version major number. */
            uprv_strcpy(libFileNames[LIB_FILE_VERSION_TMP], libFileNames[LIB_FILE_VERSION_MAJOR]);
#endif
        }
        if(mode == MODE_STATIC) {
            sprintf(libFileNames[LIB_FILE_VERSION], "%s.%s", libFileNames[LIB_FILE], pkgDataFlags[A_EXT]);
            libFileNames[LIB_FILE_VERSION_MAJOR][0]=0;
            if(o->verbose) {
              fprintf(stdout, "# libFileName[LIB_FILE_VERSION] = %s  (static)\n", libFileNames[LIB_FILE_VERSION]);
            }
        }
}

/* Create the symbolic links for the final library file. */
static int32_t pkg_createSymLinks(const char *targetDir, UBool specialHandling) {
    int32_t result = 0;
    char cmd[LARGE_BUFFER_MAX_SIZE];
    char name1[SMALL_BUFFER_MAX_SIZE]; /* symlink file name */
    char name2[SMALL_BUFFER_MAX_SIZE]; /* file name to symlink */

#ifndef USING_CYGWIN
    /* No symbolic link to make. */
    if (uprv_strlen(libFileNames[LIB_FILE_VERSION]) == 0 || uprv_strlen(libFileNames[LIB_FILE_VERSION_MAJOR]) == 0 ||
        uprv_strcmp(libFileNames[LIB_FILE_VERSION], libFileNames[LIB_FILE_VERSION_MAJOR]) == 0) {
        return result;
    }
    
    sprintf(cmd, "cd %s && %s %s && %s %s %s",
            targetDir,
            RM_CMD,
            libFileNames[LIB_FILE_VERSION_MAJOR],
            LN_CMD,
            libFileNames[LIB_FILE_VERSION],
            libFileNames[LIB_FILE_VERSION_MAJOR]);
    result = runCommand(cmd);
    if (result != 0) {
        return result;
    }
#endif

    if (specialHandling) {
#ifdef U_CYGWIN
        sprintf(name1, "%s", libFileNames[LIB_FILE_CYGWIN]);
        sprintf(name2, "%s", libFileNames[LIB_FILE_CYGWIN_VERSION]);
#else
        goto normal_symlink_mode;
#endif
    } else {
normal_symlink_mode:
        sprintf(name1, "%s.%s", libFileNames[LIB_FILE], pkgDataFlags[SO_EXT]);
        sprintf(name2, "%s", libFileNames[LIB_FILE_VERSION]);
    }

    sprintf(cmd, "cd %s && %s %s && %s %s %s",
            targetDir,
            RM_CMD,
            name1,
            LN_CMD,
            name2,
            name1);

     result = runCommand(cmd);

    return result;
}

static int32_t pkg_installLibrary(const char *installDir, const char *targetDir) {
    int32_t result = 0;
    char cmd[SMALL_BUFFER_MAX_SIZE];

    sprintf(cmd, "cd %s && %s %s %s%s%s",
            targetDir,
            pkgDataFlags[INSTALL_CMD],
            libFileNames[LIB_FILE_VERSION],
            installDir, PKGDATA_FILE_SEP_STRING, libFileNames[LIB_FILE_VERSION]
            );

    result = runCommand(cmd);

    if (result != 0) {
        return result;
    }

#ifdef CYGWINMSVC
    sprintf(cmd, "cd %s && %s %s.lib %s",
            targetDir,
            pkgDataFlags[INSTALL_CMD],
            libFileNames[LIB_FILE],
            installDir
            );
    result = runCommand(cmd);

    if (result != 0) {
        return result;
    }
#elif defined (U_CYGWIN)
    sprintf(cmd, "cd %s && %s %s %s",
            targetDir,
            pkgDataFlags[INSTALL_CMD],
            libFileNames[LIB_FILE_CYGWIN_VERSION],
            installDir
            );
    result = runCommand(cmd);

    if (result != 0) {
        return result;
    }
#endif

    return pkg_createSymLinks(installDir, TRUE);
}

static int32_t pkg_installCommonMode(const char *installDir, const char *fileName) {
    int32_t result = 0;
    char cmd[SMALL_BUFFER_MAX_SIZE] = "";

    if (!T_FileStream_file_exists(installDir)) {
        UErrorCode status = U_ZERO_ERROR;

        uprv_mkdir(installDir, &status);
        if (U_FAILURE(status)) {
            fprintf(stderr, "Error creating installation directory: %s\n", installDir);
            return -1;
        }
    }
#ifndef U_WINDOWS_WITH_MSVC
    sprintf(cmd, "%s %s %s", pkgDataFlags[INSTALL_CMD], fileName, installDir);
#else
    sprintf(cmd, "%s %s %s %s", WIN_INSTALL_CMD, fileName, installDir, WIN_INSTALL_CMD_FLAGS);
#endif

    result = runCommand(cmd);
    if (result != 0) {
        fprintf(stderr, "Failed to install data file with command: %s\n", cmd);
    }

    return result;
}

#ifdef U_WINDOWS_MSVC
/* Copy commands for installing the raw data files on Windows. */
#define WIN_INSTALL_CMD "xcopy"
#define WIN_INSTALL_CMD_FLAGS "/E /Y /K"
#endif
static int32_t pkg_installFileMode(const char *installDir, const char *srcDir, const char *fileListName) {
    int32_t result = 0;
    char cmd[SMALL_BUFFER_MAX_SIZE] = "";

    if (!T_FileStream_file_exists(installDir)) {
        UErrorCode status = U_ZERO_ERROR;

        uprv_mkdir(installDir, &status);
        if (U_FAILURE(status)) {
            fprintf(stderr, "Error creating installation directory: %s\n", installDir);
            return -1;
        }
    }
#ifndef U_WINDOWS_WITH_MSVC
    char buffer[SMALL_BUFFER_MAX_SIZE] = "";

    FileStream *f = T_FileStream_open(fileListName, "r");
    if (f != NULL) {
        for(;;) {
            if (T_FileStream_readLine(f, buffer, SMALL_BUFFER_MAX_SIZE) != NULL) {
                /* Remove new line character. */
                buffer[uprv_strlen(buffer)-1] = 0;

                sprintf(cmd, "%s %s%s%s %s%s%s",
                        pkgDataFlags[INSTALL_CMD],
                        srcDir, PKGDATA_FILE_SEP_STRING, buffer,
                        installDir, PKGDATA_FILE_SEP_STRING, buffer);

                result = runCommand(cmd);
                if (result != 0) {
                    fprintf(stderr, "Failed to install data file with command: %s\n", cmd);
                    break;
                }
            } else {
                if (!T_FileStream_eof(f)) {
                    fprintf(stderr, "Failed to read line from file: %s\n", fileListName);
                    result = -1;
                }
                break;
            }
        }
        T_FileStream_close(f);
    } else {
        result = -1;
        fprintf(stderr, "Unable to open list file: %s\n", fileListName);
    }
#else
    sprintf(cmd, "%s %s %s %s", WIN_INSTALL_CMD, srcDir, installDir, WIN_INSTALL_CMD_FLAGS);
    result = runCommand(cmd);
    if (result != 0) {
        fprintf(stderr, "Failed to install data file with command: %s\n", cmd);
    }
#endif

    return result;
}

/* Archiving of the library file may be needed depending on the platform and options given.
 * If archiving is not needed, copy over the library file name.
 */
static int32_t pkg_archiveLibrary(const char *targetDir, const char *version, UBool reverseExt) {
    int32_t result = 0;
    char cmd[LARGE_BUFFER_MAX_SIZE];

    /* If the shared object suffix and the final object suffix is different and the final object suffix and the
     * archive file suffix is the same, then the final library needs to be archived.
     */
    if (uprv_strcmp(pkgDataFlags[SOBJ_EXT], pkgDataFlags[SO_EXT]) != 0 && uprv_strcmp(pkgDataFlags[A_EXT], pkgDataFlags[SO_EXT]) == 0) {
        sprintf(libFileNames[LIB_FILE_VERSION], "%s%s%s.%s",
                libFileNames[LIB_FILE],
                pkgDataFlags[LIB_EXT_ORDER][0] == '.' ? "." : "",
                reverseExt ? version : pkgDataFlags[SO_EXT],
                reverseExt ? pkgDataFlags[SO_EXT] : version);

        sprintf(cmd, "%s %s %s%s %s%s",
                pkgDataFlags[AR],
                pkgDataFlags[ARFLAGS],
                targetDir,
                libFileNames[LIB_FILE_VERSION],
                targetDir,
                libFileNames[LIB_FILE_VERSION_TMP]);

        result = runCommand(cmd); 
        if (result != 0) { 
            return result; 
        } 
        
        sprintf(cmd, "%s %s%s", 
            pkgDataFlags[RANLIB], 
            targetDir, 
            libFileNames[LIB_FILE_VERSION]);
        
        result = runCommand(cmd); 
        if (result != 0) {
            return result;
        }

        /* Remove unneeded library file. */
        sprintf(cmd, "%s %s%s",
                RM_CMD,
                targetDir,
                libFileNames[LIB_FILE_VERSION_TMP]);

        result = runCommand(cmd);
        if (result != 0) {
            return result;
        }

    } else {
        uprv_strcpy(libFileNames[LIB_FILE_VERSION], libFileNames[LIB_FILE_VERSION_TMP]);
    }

    return result;
}

/*
 * Using the compiler information from the configuration file set by -O option, generate the library file.
 * command may be given to allow for a larger buffer for cmd.
 */
static int32_t pkg_generateLibraryFile(const char *targetDir, const char mode, const char *objectFile, char *command) {
    int32_t result = 0;
    char *cmd = NULL;
    UBool freeCmd = FALSE;

    /* This is necessary because if packaging is done without assembly code, objectFile might be extremely large
     * containing many object files and so the calling function should supply a command buffer that is large
     * enough to handle this. Otherwise, use the default size.
     */
    if (command != NULL) {
        cmd = command;
    } else {
        if ((cmd = (char *)uprv_malloc(sizeof(char) * LARGE_BUFFER_MAX_SIZE)) == NULL) {
            fprintf(stderr, "Unable to allocate memory for command.\n");
            return -1;
        }
        freeCmd = TRUE;
    }

    if (mode == MODE_STATIC) {
        sprintf(cmd, "%s %s %s%s %s",
                pkgDataFlags[AR],
                pkgDataFlags[ARFLAGS],
                targetDir,
                libFileNames[LIB_FILE_VERSION],
                objectFile);

        result = runCommand(cmd);
        if (result == 0) {
            sprintf(cmd, "%s %s%s", 
                    pkgDataFlags[RANLIB], 
                    targetDir, 
                    libFileNames[LIB_FILE_VERSION]); 
        
            result = runCommand(cmd);
        }
    } else /* if (mode == MODE_DLL) */ {
#ifdef U_CYGWIN
        sprintf(cmd, "%s%s%s %s -o %s%s %s %s%s %s %s",
                pkgDataFlags[GENLIB],
                targetDir,
                libFileNames[LIB_FILE_VERSION_TMP],
                pkgDataFlags[LDICUDTFLAGS],
                targetDir, libFileNames[LIB_FILE_CYGWIN_VERSION],
#else
        sprintf(cmd, "%s %s -o %s%s %s %s%s %s %s",
                pkgDataFlags[GENLIB],
                pkgDataFlags[LDICUDTFLAGS],
                targetDir,
                libFileNames[LIB_FILE_VERSION_TMP],
#endif
                objectFile,
                pkgDataFlags[LD_SONAME],
                pkgDataFlags[LD_SONAME][0] == 0 ? "" : libFileNames[LIB_FILE_VERSION_MAJOR],
                pkgDataFlags[RPATH_FLAGS],
                pkgDataFlags[BIR_FLAGS]);

        /* Generate the library file. */
        result = runCommand(cmd);
    }

    if (freeCmd) {
        uprv_free(cmd);
    }

    return result;
}

static int32_t pkg_createWithAssemblyCode(const char *targetDir, const char mode, const char *gencFilePath) {
    char tempObjectFile[SMALL_BUFFER_MAX_SIZE] = "";
    char *cmd;
    int32_t result = 0;

    int32_t length = 0;

    /* Remove the ending .s and replace it with .o for the new object file. */
    uprv_strcpy(tempObjectFile, gencFilePath);
    tempObjectFile[uprv_strlen(tempObjectFile)-1] = 'o';

    length = uprv_strlen(pkgDataFlags[COMPILER]) + uprv_strlen(pkgDataFlags[LIBFLAGS])
                    + uprv_strlen(tempObjectFile) + uprv_strlen(gencFilePath) + 10;

    cmd = (char *)uprv_malloc(sizeof(char) * length);
    if (cmd == NULL) {
        return -1;
    }

    /* Generate the object file. */
    sprintf(cmd, "%s %s -o %s %s",
            pkgDataFlags[COMPILER],
            pkgDataFlags[LIBFLAGS],
            tempObjectFile,
            gencFilePath);

    result = runCommand(cmd);
    uprv_free(cmd);
    if (result != 0) {
        return result;
    }

    return pkg_generateLibraryFile(targetDir, mode, tempObjectFile);
}

#ifdef BUILD_DATA_WITHOUT_ASSEMBLY
/*
 * Generation of the data library without assembly code needs to compile each data file
 * individually and then link it all together.
 * Note: Any update to the directory structure of the data needs to be reflected here.
 */
enum {
    DATA_PREFIX_BRKITR,
    DATA_PREFIX_COLL,
    DATA_PREFIX_CURR,
    DATA_PREFIX_LANG,
    DATA_PREFIX_RBNF,
    DATA_PREFIX_REGION,
    DATA_PREFIX_TRANSLIT,
    DATA_PREFIX_ZONE,
    DATA_PREFIX_LENGTH
};

const static char DATA_PREFIX[DATA_PREFIX_LENGTH][10] = {
        "brkitr",
        "coll",
        "curr",
        "lang",
        "rbnf",
        "region",
        "translit",
        "zone"
};

static int32_t pkg_createWithoutAssemblyCode(UPKGOptions *o, const char *targetDir, const char mode) {
    int32_t result = 0;
    CharList *list = o->filePaths;
    CharList *listNames = o->files;
    int32_t listSize = pkg_countCharList(list);
    char *buffer;
    char *cmd;
    char gencmnFile[SMALL_BUFFER_MAX_SIZE] = "";
    char tempObjectFile[SMALL_BUFFER_MAX_SIZE] = "";
#ifdef USE_SINGLE_CCODE_FILE
    char icudtAll[SMALL_BUFFER_MAX_SIZE] = "";
    
    sprintf(icudtAll, "%s%s%sall.c",
            o->tmpDir,
            PKGDATA_FILE_SEP_STRING, 
            libFileNames[LIB_FILE]);
    /* Remove previous icudtall.c file. */
    if (T_FileStream_file_exists(icudtAll) && (result = remove(icudtAll)) != 0) {
        fprintf(stderr, "Unable to remove old icudtall file: %s\n", icudtAll);
        return result;
    }
#endif

    if (list == NULL || listNames == NULL) {
        /* list and listNames should never be NULL since we are looping through the CharList with
         * the given size.
         */
        return -1;
    }

    if ((cmd = (char *)uprv_malloc((listSize + 2) * SMALL_BUFFER_MAX_SIZE)) == NULL) {
        fprintf(stderr, "Unable to allocate memory for cmd.\n");
        return -1;
    } else if ((buffer = (char *)uprv_malloc((listSize + 1) * SMALL_BUFFER_MAX_SIZE)) == NULL) {
        fprintf(stderr, "Unable to allocate memory for buffer.\n");
        uprv_free(cmd);
        return -1;
    }

    for (int32_t i = 0; i < (listSize + 1); i++) {
        const char *file ;
        const char *name;

        if (i == 0) {
            /* The first iteration calls the gencmn function and initailizes the buffer. */
            createCommonDataFile(o->tmpDir, o->shortName, o->entryName, NULL, o->srcDir, o->comment, o->fileListFiles->str, 0, TRUE, o->verbose, gencmnFile);
            buffer[0] = 0;
#ifdef USE_SINGLE_CCODE_FILE
            uprv_strcpy(tempObjectFile, gencmnFile);
            tempObjectFile[uprv_strlen(tempObjectFile) - 1] = 'o';
            
            sprintf(cmd, "%s %s -o %s %s"
                        pkgDataFlags[COMPILER],
                        pkgDataFlags[LIBFLAGS],
                        tempObjectFile,
                        gencmnFile);
            
            result = runCommand(cmd);
            if (result != 0) {
                break;
            }
            
            sprintf(buffer, "%s",tempObjectFile);
#endif
        } else {
            char newName[SMALL_BUFFER_MAX_SIZE];
            char dataName[SMALL_BUFFER_MAX_SIZE];
            char dataDirName[SMALL_BUFFER_MAX_SIZE];
            const char *pSubstring;
            file = list->str;
            name = listNames->str;

            newName[0] = dataName[0] = 0;
            for (int32_t n = 0; n < DATA_PREFIX_LENGTH; n++) {
                dataDirName[0] = 0;
                sprintf(dataDirName, "%s%s", DATA_PREFIX[n], PKGDATA_FILE_SEP_STRING);
                /* If the name contains a prefix (indicating directory), alter the new name accordingly. */
                pSubstring = uprv_strstr(name, dataDirName);
                if (pSubstring != NULL) {
                    char newNameTmp[SMALL_BUFFER_MAX_SIZE] = "";
                    const char *p = name + uprv_strlen(dataDirName);
                    for (int32_t i = 0;;i++) {
                        if (p[i] == '.') {
                            newNameTmp[i] = '_';
                            continue;
                        }
                        newNameTmp[i] = p[i];
                        if (p[i] == 0) {
                            break;
                        }
                    }
                    sprintf(newName, "%s_%s",
                            DATA_PREFIX[n],
                            newNameTmp);
                    sprintf(dataName, "%s_%s",
                            o->shortName,
                            DATA_PREFIX[n]);
                }
                if (newName[0] != 0) {
                    break;
                }
            }

            writeCCode(file, o->tmpDir, dataName[0] != 0 ? dataName : o->shortName, newName[0] != 0 ? newName : NULL, gencmnFile);
#ifdef USE_SINGLE_CCODE_FILE
            sprintf(cmd, "cat %s >> %s", gencmnFile, icudtAll);
            
            result = runCommand(cmd);
            if (result != 0) {
                break;
            } else {
                /* Remove the c code file after concatenating it to icudtall.c file. */
                if ((result = remove(gencmnFile)) != 0) {
                    fprintf(stderr, "Unable to remove c code file: %s\n", gencmnFile);
                    return result;
                }
            }
#endif
        }

#ifndef USE_SINGLE_CCODE_FILE
        uprv_strcpy(tempObjectFile, gencmnFile);
        tempObjectFile[uprv_strlen(tempObjectFile) - 1] = 'o';
        
        sprintf(cmd, "%s %s -o %s %s",
                    pkgDataFlags[COMPILER],
                    pkgDataFlags[LIBFLAGS],
                    tempObjectFile,
                    gencmnFile);
        result = runCommand(cmd);
        if (result != 0) {
            break;
        }

        uprv_strcat(buffer, " ");
        uprv_strcat(buffer, tempObjectFile);

#endif
        
        if (i > 0) {
            list = list->next;
            listNames = listNames->next;
        }
    }

#ifdef USE_SINGLE_CCODE_FILE
    uprv_strcpy(tempObjectFile, icudtAll);
    tempObjectFile[uprv_strlen(tempObjectFile) - 1] = 'o';

    sprintf(cmd, "%s %s -o %s %s",
        pkgDataFlags[COMPILER],
        pkgDataFlags[LIBFLAGS],
        tempObjectFile,
        icudtAll);
    
    result = runCommand(cmd);
    if (result == 0) {
        uprv_strcat(buffer, " ");
        uprv_strcat(buffer, tempObjectFile);
    }
#endif

    if (result == 0) {
        /* Generate the library file. */
        result = pkg_generateLibraryFile(targetDir, mode, buffer, cmd);
    }

    uprv_free(buffer);
    uprv_free(cmd);

    return result;
}
#endif

#ifdef WINDOWS_WITH_MSVC
#define LINK_CMD "link.exe /nologo /release /out:"
#define LINK_FLAGS "/DLL /NOENTRY /MANIFEST:NO  /base:0x4ad00000 /implib:"
#define LIB_CMD "LIB.exe /nologo /out:"
#define LIB_FILE "icudt.lib"
#define LIB_EXT UDATA_LIB_SUFFIX
#define DLL_EXT UDATA_SO_SUFFIX

static int32_t pkg_createWindowsDLL(const char mode, const char *gencFilePath, UPKGOptions *o) {
    char cmd[LARGE_BUFFER_MAX_SIZE];
    if (mode == MODE_STATIC) {
        char staticLibFilePath[SMALL_BUFFER_MAX_SIZE] = "";

        uprv_strcpy(staticLibFilePath, o->tmpDir);
        uprv_strcat(staticLibFilePath, PKGDATA_FILE_SEP_STRING);

        uprv_strcat(staticLibFilePath, o->entryName);
        uprv_strcat(staticLibFilePath, LIB_EXT);

        sprintf(cmd, "%s\"%s\" \"%s\"",
                LIB_CMD,
                staticLibFilePath,
                gencFilePath);
    } else if (mode == MODE_DLL) {
        char dllFilePath[SMALL_BUFFER_MAX_SIZE] = "";
        char libFilePath[SMALL_BUFFER_MAX_SIZE] = "";
        char resFilePath[SMALL_BUFFER_MAX_SIZE] = "";
        char tmpResFilePath[SMALL_BUFFER_MAX_SIZE] = "";

#ifdef CYGWINMSVC
        uprv_strcpy(dllFilePath, o->targetDir);
#else
        uprv_strcpy(dllFilePath, o->srcDir);
#endif
        uprv_strcat(dllFilePath, PKGDATA_FILE_SEP_STRING);
        uprv_strcpy(libFilePath, dllFilePath);

#ifdef CYGWINMSVC
        uprv_strcat(libFilePath, o->libName);
        uprv_strcat(libFilePath, ".lib");
        
        uprv_strcat(dllFilePath, o->libName);
        uprv_strcat(dllFilePath, o->version);
#else
        if (strstr(o->libName, "icudt")) {
            uprv_strcat(libFilePath, LIB_FILE);
        } else {
            uprv_strcat(libFilePath, o->libName);
            uprv_strcat(libFilePath, ".lib");
        }
        uprv_strcat(dllFilePath, o->entryName);
#endif
        uprv_strcat(dllFilePath, DLL_EXT);
        
        uprv_strcpy(tmpResFilePath, o->tmpDir);
        uprv_strcat(tmpResFilePath, PKGDATA_FILE_SEP_STRING);
        uprv_strcat(tmpResFilePath, ICUDATA_RES_FILE);

        if (T_FileStream_file_exists(tmpResFilePath)) {
            sprintf(resFilePath, "\"%s\"", tmpResFilePath);
        }

        /* Check if dll file and lib file exists and that it is not newer than genc file. */
        if (!o->rebuild && (T_FileStream_file_exists(dllFilePath) && isFileModTimeLater(dllFilePath, gencFilePath)) &&
            (T_FileStream_file_exists(libFilePath) && isFileModTimeLater(libFilePath, gencFilePath))) {
          if(o->verbose) {
            printf("# Not rebuilding %s - up to date.\n", gencFilePath);
          }
          return 0;
        }

        sprintf(cmd, "%s\"%s\" %s\"%s\" \"%s\" %s",
                LINK_CMD,
                dllFilePath,
                LINK_FLAGS,
                libFilePath,
                gencFilePath,
                resFilePath
                );
    }

    return runCommand(cmd, TRUE);
}
#endif

static UPKGOptions *pkg_checkFlag(UPKGOptions *o) {
#ifdef U_AIX
    /* AIX needs a map file. */
    char *flag = NULL;
    int32_t length = 0;
    char tmpbuffer[SMALL_BUFFER_MAX_SIZE];
    const char MAP_FILE_EXT[] = ".map";
    FileStream *f = NULL;
    char mapFile[SMALL_BUFFER_MAX_SIZE] = "";
    int32_t start = -1;
    int32_t count = 0;

    flag = pkgDataFlags[BIR_FLAGS];
    length = uprv_strlen(pkgDataFlags[BIR_FLAGS]);

    for (int32_t i = 0; i < length; i++) {
        if (flag[i] == MAP_FILE_EXT[count]) {
            if (count == 0) {
                start = i;
            }
            count++;
        } else {
            count = 0;
        }

        if (count == uprv_strlen(MAP_FILE_EXT)) {
            break;
        }
    }

    if (start >= 0) {
        int32_t index = 0;
        for (int32_t i = 0;;i++) {
            if (i == start) {
                for (int32_t n = 0;;n++) {
                    if (o->shortName[n] == 0) {
                        break;
                    }
                    tmpbuffer[index++] = o->shortName[n];
                }
            }

            tmpbuffer[index++] = flag[i];

            if (flag[i] == 0) {
                break;
            }
        }

        uprv_memset(flag, 0, length);
        uprv_strcpy(flag, tmpbuffer);

        uprv_strcpy(mapFile, o->shortName);
        uprv_strcat(mapFile, MAP_FILE_EXT);

        f = T_FileStream_open(mapFile, "w");
        if (f == NULL) {
            fprintf(stderr,"Unable to create map file: %s.\n", mapFile);
        } else {
            sprintf(tmpbuffer, "%s%s ", o->entryName, UDATA_CMN_INTERMEDIATE_SUFFIX);
    
            T_FileStream_writeLine(f, tmpbuffer);
    
            T_FileStream_close(f);
        }
    }
#elif defined(U_CYGWIN)
    /* Cygwin needs to change flag options. */
    char *flag = NULL;
    int32_t length = 0;

    flag = pkgDataFlags[GENLIB];
    length = uprv_strlen(pkgDataFlags[GENLIB]);

    int32_t position = length - 1;

    for(;position >= 0;position--) {
        if (flag[position] == '=') {
            position++;
            break;
        }
    }

    uprv_memset(flag + position, 0, length - position);
#elif defined(OS400)
    /* OS400 needs to fix the ld options (swap single quote with double quote) */
    char *flag = NULL;
    int32_t length = 0;

    flag = pkgDataFlags[GENLIB];
    length = uprv_strlen(pkgDataFlags[GENLIB]);

    int32_t position = length - 1;

    for(int32_t i = 0; i < length; i++) {
        if (flag[i] == '\'') {
            flag[i] = '\"';
        }
    }
#endif
    // Don't really need a return value, just need to stop compiler warnings about
    // the unused parameter 'o' on platforms where it is not otherwise used.
    return o;   
}

static void loadLists(UPKGOptions *o, UErrorCode *status)
{
    CharList   *l, *tail = NULL, *tail2 = NULL;
    FileStream *in;
    char        line[16384];
    char       *linePtr, *lineNext;
    const uint32_t   lineMax = 16300;
    char        tmp[1024];
    char       *s;
    int32_t     ln=0; /* line number */

    for(l = o->fileListFiles; l; l = l->next) {
        if(o->verbose) {
            fprintf(stdout, "# pkgdata: Reading %s..\n", l->str);
        }
        /* TODO: stdin */
        in = T_FileStream_open(l->str, "r"); /* open files list */

        if(!in) {
            fprintf(stderr, "Error opening <%s>.\n", l->str);
            *status = U_FILE_ACCESS_ERROR;
            return;
        }

        while(T_FileStream_readLine(in, line, sizeof(line))!=NULL) { /* for each line */
            ln++;
            if(uprv_strlen(line)>lineMax) {
                fprintf(stderr, "%s:%d - line too long (over %d chars)\n", l->str, (int)ln, (int)lineMax);
                exit(1);
            }
            /* remove spaces at the beginning */
            linePtr = line;
            while(isspace(*linePtr)) {
                linePtr++;
            }
            s=linePtr;
            /* remove trailing newline characters */
            while(*s!=0) {
                if(*s=='\r' || *s=='\n') {
                    *s=0;
                    break;
                }
                ++s;
            }
            if((*linePtr == 0) || (*linePtr == '#')) {
                continue; /* comment or empty line */
            }

            /* Now, process the line */
            lineNext = NULL;

            while(linePtr && *linePtr) { /* process space-separated items */
                while(*linePtr == ' ') {
                    linePtr++;
                }
                /* Find the next quote */
                if(linePtr[0] == '"')
                {
                    lineNext = uprv_strchr(linePtr+1, '"');
                    if(lineNext == NULL) {
                        fprintf(stderr, "%s:%d - missing trailing double quote (\")\n",
                            l->str, (int)ln);
                        exit(1);
                    } else {
                        lineNext++;
                        if(*lineNext) {
                            if(*lineNext != ' ') {
                                fprintf(stderr, "%s:%d - malformed quoted line at position %d, expected ' ' got '%c'\n",
                                    l->str, (int)ln, (int)(lineNext-line), (*lineNext)?*lineNext:'0');
                                exit(1);
                            }
                            *lineNext = 0;
                            lineNext++;
                        }
                    }
                } else {
                    lineNext = uprv_strchr(linePtr, ' ');
                    if(lineNext) {
                        *lineNext = 0; /* terminate at space */
                        lineNext++;
                    }
                }

                /* add the file */
                s = (char*)getLongPathname(linePtr);

                /* normal mode.. o->files is just the bare list without package names */
                o->files = pkg_appendToList(o->files, &tail, uprv_strdup(linePtr));
                if(uprv_pathIsAbsolute(s)) {
                    fprintf(stderr, "pkgdata: Error: absolute path encountered. Old style paths are not supported. Use relative paths such as 'fur.res' or 'translit%cfur.res'.\n\tBad path: '%s'\n", U_FILE_SEP_CHAR, s);
                    exit(U_ILLEGAL_ARGUMENT_ERROR);
                }
                uprv_strcpy(tmp, o->srcDir);
                uprv_strcat(tmp, o->srcDir[uprv_strlen(o->srcDir)-1] == U_FILE_SEP_CHAR ? "" :PKGDATA_FILE_SEP_STRING);
                uprv_strcat(tmp, s);
                o->filePaths = pkg_appendToList(o->filePaths, &tail2, uprv_strdup(tmp));
                linePtr = lineNext;
            } /* for each entry on line */
        } /* for each line */
        T_FileStream_close(in);
    } /* for each file list file */
}

/* Try calling icu-config directly to get the option file. */
 static int32_t pkg_getOptionsFromICUConfig(UBool verbose, UOption *option) {
#if U_HAVE_POPEN
    FILE *p = NULL;
    size_t n;
    static char buf[512] = "";
    char cmdBuf[1024];
    UErrorCode status = U_ZERO_ERROR;
    const char cmd[] = "icu-config --incpkgdatafile";

    /* #1 try the same path where pkgdata was called from. */
    findDirname(progname, cmdBuf, 1024, &status);
    if(U_SUCCESS(status)) {
      uprv_strncat(cmdBuf, U_FILE_SEP_STRING, 1024);
      uprv_strncat(cmdBuf, cmd, 1024);
      
      if(verbose) {
        fprintf(stdout, "# Calling icu-config: %s\n", cmdBuf);
      }
      p = popen(cmdBuf, "r");
    }

    if(p == NULL) {
      if(verbose) {
        fprintf(stdout, "# Calling icu-config: %s\n", cmd);
      }
      p = popen(cmd, "r");      
    }

    if(p == NULL)
    {
        fprintf(stderr, "%s: icu-config: No icu-config found. (fix PATH or use -O option)\n", progname);
        return -1;
    }

    n = fread(buf, 1, 511, p);

    pclose(p);

    if(n<=0)
    {
        fprintf(stderr,"%s: icu-config: Could not read from icu-config. (fix PATH or use -O option)\n", progname);
        return -1;
    }

    for (int32_t length = strlen(buf) - 1; length >= 0; length--) {
        if (buf[length] == '\n' || buf[length] == ' ') {
            buf[length] = 0;
        } else {
            break;
        }
    }

    if(buf[strlen(buf)-1]=='\n')
    {
        buf[strlen(buf)-1]=0;
    }

    if(buf[0] == 0)
    {
        fprintf(stderr, "%s: icu-config: invalid response from icu-config (fix PATH or use -O option)\n", progname);
        return -1;
    }

    if(verbose) {
      fprintf(stdout, "# icu-config said: %s\n", buf);
    }

    option->value = buf;
    option->doesOccur = TRUE;

    return 0;
#endif
    return -1;
}
