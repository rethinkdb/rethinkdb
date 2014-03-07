/*
*******************************************************************************
*
*   Copyright (C) 1999-2010, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  icuinfo.cpp
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2009-2010
*   created by: Steven R. Loomis
*
*   This program shows some basic info about the current ICU.
*/

#include <stdio.h>
#include <stdlib.h>
#include "unicode/utypes.h"
#include "unicode/putil.h"
#include "unicode/uclean.h"
#include "udbgutil.h"
#include "unewdata.h"
#include "cmemory.h"
#include "cstring.h"
#include "uoptions.h"
#include "toolutil.h"
#include "icuplugimp.h"
#include <unicode/uloc.h>
#include <unicode/ucnv.h>
#include "unicode/ucal.h"
#include <unicode/ulocdata.h>
#include "putilimp.h"
#include "unicode/uchar.h"

static UOption options[]={
  /*0*/ UOPTION_HELP_H,
  /*1*/ UOPTION_HELP_QUESTION_MARK,
  /*2*/ UOPTION_ICUDATADIR,
  /*3*/ UOPTION_VERBOSE,
  /*4*/ UOPTION_DEF("list-plugins", 'L', UOPT_NO_ARG),
  /*5*/ UOPTION_DEF("milisecond-time", 'm', UOPT_NO_ARG),
  /*6*/ UOPTION_DEF("cleanup", 'K', UOPT_NO_ARG),
};

static UErrorCode initStatus = U_ZERO_ERROR;
static UBool icuInitted = FALSE;

static void do_init() {
    if(!icuInitted) {
      u_init(&initStatus);
      icuInitted = TRUE;
    }
}

/** 
 * Print the current platform 
 */
static const char *getPlatform()
{
#if defined(U_PLATFORM)
	return U_PLATFORM;
#elif defined(U_WINDOWS)
	return "Windows";
#elif defined(U_PALMOS)
	return "PalmOS";
#elif defined(_PLATFORM_H)
	return "Other (POSIX-like)";
#else
	return "unknown"
#endif
}

void cmd_millis()
{
  printf("Milliseconds since Epoch: %.0f\n", uprv_getUTCtime());
}

void cmd_version(UBool noLoad)
{
    UVersionInfo icu;
    char str[200];
    printf("<ICUINFO>\n");
    printf("International Components for Unicode for C/C++\n");
    printf("%s\n", U_COPYRIGHT_STRING);
    printf("Compiled-Version: %s\n", U_ICU_VERSION);
    u_getVersion(icu);
    u_versionToString(icu, str);
    printf("Runtime-Version: %s\n", str);
    printf("Compiled-Unicode-Version: %s\n", U_UNICODE_VERSION);
    u_getUnicodeVersion(icu);
    u_versionToString(icu, str);
    printf("Runtime-Unicode-Version: %s\n", U_UNICODE_VERSION);
    printf("Platform: %s\n", getPlatform());
#if defined(U_BUILD)
    printf("Build: %s\n", U_BUILD);
#if defined(U_HOST)
    if(strcmp(U_BUILD,U_HOST)) {
      printf("Host: %s\n", U_HOST);
    }
#endif
#endif
#if defined(U_CC)
    printf("C compiler: %s\n", U_CC);
#endif
#if defined(U_CXX)
    printf("C++ compiler: %s\n", U_CXX);
#endif
#if defined(CYGWINMSVC)
    printf("Cygwin: CYGWINMSVC\n");
#endif
    printf("ICUDATA: %s\n", U_ICUDATA_NAME);
    do_init();
    printf("Data Directory: %s\n", u_getDataDirectory());
    printf("ICU Initialization returned: %s\n", u_errorName(initStatus));
    printf( "Default locale: %s\n", uloc_getDefault());
    {
      UErrorCode subStatus = U_ZERO_ERROR;
      ulocdata_getCLDRVersion(icu, &subStatus);
      if(U_SUCCESS(subStatus)) {
	u_versionToString(icu, str);
	printf("CLDR-Version: %s\n", str);
      } else {
	printf("CLDR-Version: %s\n", u_errorName(subStatus));
      }
    }
    
#if !UCONFIG_NO_CONVERSION
    if(noLoad == FALSE)
    {
      printf("Default converter: %s\n", ucnv_getDefaultName());
    }
#endif
#if !UCONFIG_NO_FORMATTING
    {
      UChar buf[100];
      char buf2[100];
      UErrorCode subsubStatus= U_ZERO_ERROR;
      int32_t len;

      len = ucal_getDefaultTimeZone(buf, 100, &subsubStatus);
      if(U_SUCCESS(subsubStatus)&&len>0) {
	u_UCharsToChars(buf, buf2, len+1);
	printf("Default TZ: %s\n", buf2);
      } else {
	printf("Default TZ: %s\n", u_errorName(subsubStatus));
      }
    }
    {
      UErrorCode subStatus = U_ZERO_ERROR;
      const char *tzVer = ucal_getTZDataVersion(&subStatus);
      if(U_FAILURE(subStatus)) {
	tzVer = u_errorName(subStatus);
      }
      printf("TZ data version: %s\n", tzVer);
    }
#endif
    
#if U_ENABLE_DYLOAD
    const char *pluginFile = uplug_getPluginFile();
    printf("Plugin file is: %s\n", (pluginFile&&*pluginFile)?pluginFile:"(not set. try setting ICU_PLUGINS to a directory.)");
#else
    fprintf(stderr, "Dynamic Loading: is disabled. No plugins will be loaded at start-up.\n");
#endif
    printf("</ICUINFO>\n\n");
}

void cmd_cleanup()
{
    u_cleanup();
    fprintf(stderr,"ICU u_cleanup() called.\n");
}


void cmd_listplugins() {
    int32_t i;
    UPlugData *plug;

    do_init();
    printf("ICU Initialized: u_init() returned %s\n", u_errorName(initStatus));
    
    printf("Plugins: \n");
    printf(    "# %6s   %s \n",
                       "Level",
                       "Name" );
    printf(    "    %10s:%-10s\n",
                       "Library",
                       "Symbol"
            );

                       
    printf(    "       config| (configuration string)\n");
    printf(    " >>>   Error          | Explanation \n");
    printf(    "-----------------------------------\n");
        
    for(i=0;(plug=uplug_getPlugInternal(i))!=NULL;i++) {
        UErrorCode libStatus = U_ZERO_ERROR;
        const char *name = uplug_getPlugName(plug);
        const char *sym = uplug_getSymbolName(plug);
        const char *lib = uplug_getLibraryName(plug, &libStatus);
        const char *config = uplug_getConfiguration(plug);
        UErrorCode loadStatus = uplug_getPlugLoadStatus(plug);
        const char *message = NULL;
        
        printf("\n#%d  %-6s %s \n",
            i+1,
            udbg_enumName(UDBG_UPlugLevel,(int32_t)uplug_getPlugLevel(plug)),
            name!=NULL?(*name?name:"this plugin did not call uplug_setPlugName()"):"(null)"
        );
        printf("    plugin| %10s:%-10s\n",
            (U_SUCCESS(libStatus)?(lib!=NULL?lib:"(null)"):u_errorName(libStatus)),
            sym!=NULL?sym:"(null)"
        );
        
        if(config!=NULL&&*config) {
            printf("    config| %s\n", config);
        }
        
        switch(loadStatus) {
            case U_PLUGIN_CHANGED_LEVEL_WARNING:
                message = "Note: This plugin changed the system level (by allocating memory or calling something which does). Later plugins may not load.";
                break;
                
            case U_PLUGIN_DIDNT_SET_LEVEL:
                message = "Error: This plugin did not call uplug_setPlugLevel during QUERY.";
                break;
            
            case U_PLUGIN_TOO_HIGH:
                message = "Error: This plugin couldn't load because the system level was too high. Try loading this plugin earlier.";
                break;
                
            case U_ZERO_ERROR: 
                message = NULL; /* no message */
                break;
            default:
                if(U_FAILURE(loadStatus)) {
                    message = "error loading:";
                } else {
                    message = "warning during load:";
                }            
        }
        
        if(message!=NULL) {
            printf("\\\\\\ status| %s\n"
                   "/// %s\n", u_errorName(loadStatus), message);
        }
        
    }
	if(i==0) {
		printf("No plugins loaded.\n");
	}

}



extern int
main(int argc, char* argv[]) {
    UErrorCode errorCode = U_ZERO_ERROR;
    UBool didSomething = FALSE;
    
    /* preset then read command line options */
    argc=u_parseArgs(argc, argv, sizeof(options)/sizeof(options[0]), options);

    /* error handling, printing usage message */
    if(argc<0) {
        fprintf(stderr,
            "error in command line argument \"%s\"\n",
            argv[-argc]);
    }
    if( options[0].doesOccur || options[1].doesOccur) {
      fprintf(stderr, "%s: Output information about the current ICU\n", argv[0]);
      fprintf(stderr, "Options:\n"
              " -h     or  --help                 - Print this help message.\n"
              " -m     or  --millisecond-time     - Print the current UTC time in milliseconds.\n"
              " -d <dir>   or  --icudatadir <dir> - Set the ICU Data Directory\n"
              " -v                                - Print version and configuration information about ICU\n"
              " -L         or  --list-plugins     - List and diagnose issues with ICU Plugins\n"
              " -K         or  --cleanup          - Call u_cleanup() before exitting (will attempt to unload plugins)\n"
              "\n"
              "If no arguments are given, the tool will print ICU version and configuration information.\n"
              );
      fprintf(stderr, "International Components for Unicode %s\n%s\n", U_ICU_VERSION, U_COPYRIGHT_STRING );
      return argc<0 ? U_ILLEGAL_ARGUMENT_ERROR : U_ZERO_ERROR;
    }
    
    if(options[2].doesOccur) {
      u_setDataDirectory(options[2].value);
    }

    if(options[5].doesOccur) {
      cmd_millis();
      didSomething=TRUE;
    } 
    if(options[4].doesOccur) {
      cmd_listplugins();
      didSomething = TRUE;
    }
    
    if(options[3].doesOccur) {
      cmd_version(FALSE);
      didSomething = TRUE;
    }
        
    if(options[6].doesOccur) {  /* 2nd part of version: cleanup */
      cmd_cleanup();
      didSomething = TRUE;
    }
        
    if(!didSomething) {
      cmd_version(FALSE);  /* at least print the version # */
    }

    return U_FAILURE(errorCode);
}
