/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2010, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

#include "unicode/ucnv.h"
#include "unicode/ucnv_err.h"

#include "cintltst.h"
#include "ustr_cnv.h"
#include <string.h>
void TestDefaultConverterError(void); /* keep gcc happy */
void TestDefaultConverterSet(void); /* keep gcc happy */


/* This makes sure that a converter isn't leaked when an error is passed to 
    u_getDefaultConverter */
void TestDefaultConverterError(void) {
    UErrorCode          err                 =   U_ZERO_ERROR;

    /* Remove the default converter */
    ucnv_close(u_getDefaultConverter(&err));

    if (U_FAILURE(err)) {
        log_err("Didn't expect a failure yet %s\n", myErrorName(err));
        return;
    }

    /* Set to any radom error state */
    err = U_FILE_ACCESS_ERROR;
    if (u_getDefaultConverter(&err) != NULL) {
        log_err("Didn't expect to get a converter on a failure\n");
    }
}

/* Get the default converter. Copy its name. Put it back. */
static void copyDefaultConverterName(char *out, UErrorCode *status) {
    UConverter *defConv;
    const char *itsName;
    out[0]=0;
    if(U_FAILURE(*status)) return;
    defConv = u_getDefaultConverter(status);
    /* get its name */
    itsName = ucnv_getName(defConv, status);
    if(U_FAILURE(*status)) return;
    strcpy(out, itsName); 
    /* put it back. */
    u_releaseDefaultConverter(defConv); 
}

/* 
  Changing the default name may not affect the actual name from u_getDefaultConverter
   ( for example, if UTF-8 is the fixed converter ).
  But, if it does cause a change, that change should be reflected when the converter is
  set back.
*/
void TestDefaultConverterSet(void) {
    UErrorCode status = U_ZERO_ERROR;
    static char defaultName[UCNV_MAX_CONVERTER_NAME_LENGTH + 1];
    static char nameBeforeSet[UCNV_MAX_CONVERTER_NAME_LENGTH + 1];
    static char nameAfterSet[UCNV_MAX_CONVERTER_NAME_LENGTH + 1];
    static char nameAfterRestore[UCNV_MAX_CONVERTER_NAME_LENGTH + 1];
    static const char SET_TO[]="iso-8859-3";
    strcpy(defaultName, ucnv_getDefaultName());

    log_verbose("getDefaultName returned %s\n", defaultName);

   /* first, flush any extant converter */
   u_flushDefaultConverter();
   copyDefaultConverterName(nameBeforeSet, &status);
   log_verbose("name from u_getDefaultConverter() = %s\n", nameBeforeSet);
   u_flushDefaultConverter();
   ucnv_setDefaultName(SET_TO);
   copyDefaultConverterName(nameAfterSet, &status);
   log_verbose("name from u_getDefaultConverter() after set to %s (%s) = %s\n", SET_TO, ucnv_getDefaultName(), nameAfterSet);
   ucnv_setDefaultName(defaultName);
   copyDefaultConverterName(nameAfterRestore, &status);
   log_verbose("name from u_getDefaultConverter() after restore = %s\n", nameAfterRestore);
   u_flushDefaultConverter();

   if(U_FAILURE(status)) {
      log_err("Error in test: %s\n", u_errorName(status));
   } else {
      if(!strcmp(nameBeforeSet, nameAfterSet)) { /* changing the default didn't affect. */
          log_info("Skipping test: ucnv_setDefaultName() did not affect actual name of %s\n", nameBeforeSet);
      } else {
          if(strcmp(nameBeforeSet, nameAfterRestore)) {
               log_err("Error: u_getDefaultConverter() is still returning %s (expected %s) even though default converter was set back to %s (was %s)\n", nameAfterRestore, nameBeforeSet, defaultName , SET_TO);
          } else {
              log_verbose("Test passed. \n");
          }
      }
   }
}


