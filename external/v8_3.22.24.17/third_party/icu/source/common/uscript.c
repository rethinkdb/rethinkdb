/*
**********************************************************************
*   Copyright (C) 1997-2010, International Business Machines
*   Corporation and others.  All Rights Reserved.
**********************************************************************
*
* File USCRIPT.C
*
* Modification History:
*
*   Date        Name        Description
*   07/06/2001    Ram         Creation.
******************************************************************************
*/

#include "unicode/uscript.h"
#include "unicode/ures.h"
#include "unicode/uchar.h"
#include "unicode/putil.h"
#include "uprops.h"
#include "cmemory.h"
#include "cstring.h"

static const char kLocaleScript[] = "LocaleScript";

/* TODO: this is a bad API should be deprecated */
U_CAPI int32_t  U_EXPORT2
uscript_getCode(const char* nameOrAbbrOrLocale,
                UScriptCode* fillIn,
                int32_t capacity,
                UErrorCode* err){

    UScriptCode code = USCRIPT_INVALID_CODE;
    int32_t numFilled=0;
    int32_t len=0;
    /* check arguments */
    if(err==NULL ||U_FAILURE(*err)){
        return numFilled;
    }
    if(nameOrAbbrOrLocale==NULL || fillIn == NULL || capacity<0){
        *err = U_ILLEGAL_ARGUMENT_ERROR;
        return numFilled;
    }

    if(uprv_strchr(nameOrAbbrOrLocale, '-')==NULL && uprv_strchr(nameOrAbbrOrLocale, '_')==NULL ){
        /* try long and abbreviated script names first */
        code = (UScriptCode) u_getPropertyValueEnum(UCHAR_SCRIPT, nameOrAbbrOrLocale);
        
    }
    if(code==(UScriptCode)UCHAR_INVALID_CODE){
       /* Do not propagate error codes from just not finding a locale bundle. */
        UErrorCode localErrorCode = U_ZERO_ERROR;
        UResourceBundle* resB = ures_open(NULL,nameOrAbbrOrLocale,&localErrorCode);
        if(U_SUCCESS(localErrorCode)&& localErrorCode != U_USING_DEFAULT_WARNING){
            UResourceBundle* resD = ures_getByKey(resB,kLocaleScript,NULL,&localErrorCode);
            if(U_SUCCESS(localErrorCode) ){
                len =0;
                while(ures_hasNext(resD)){
                    const UChar* name = ures_getNextString(resD,&len,NULL,&localErrorCode);
                    if(U_SUCCESS(localErrorCode)){
                        char cName[50] = {'\0'};
                        u_UCharsToChars(name,cName,len);
                        code = (UScriptCode) u_getPropertyValueEnum(UCHAR_SCRIPT, cName);
                        /* got the script code now fill in the buffer */
                        if(numFilled<capacity){ 
                            *(fillIn)++=code;
                            numFilled++;
                        }else{
                            ures_close(resD);
                            ures_close(resB);
                            *err=U_BUFFER_OVERFLOW_ERROR;
                            return len;
                        }
                    }
                }
            }
            ures_close(resD); 
        }
        ures_close(resB);
        code = USCRIPT_INVALID_CODE;
    }
    if(code==(UScriptCode)UCHAR_INVALID_CODE){
       /* still not found .. try long and abbreviated script names again */
        code = (UScriptCode) u_getPropertyValueEnum(UCHAR_SCRIPT, nameOrAbbrOrLocale);
    }
    if(code!=(UScriptCode)UCHAR_INVALID_CODE){
        /* we found it */
        if(numFilled<capacity){ 
            *(fillIn)++=code;
            numFilled++;
        }else{
            *err=U_BUFFER_OVERFLOW_ERROR;
            return len;
        }
    }
    return numFilled;
}

U_CAPI const char*  U_EXPORT2
uscript_getName(UScriptCode scriptCode){
    return u_getPropertyValueName(UCHAR_SCRIPT, scriptCode,
                                  U_LONG_PROPERTY_NAME);
}

U_CAPI const char*  U_EXPORT2
uscript_getShortName(UScriptCode scriptCode){
    return u_getPropertyValueName(UCHAR_SCRIPT, scriptCode,
                                  U_SHORT_PROPERTY_NAME);
}

