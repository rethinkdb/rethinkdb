/*
*******************************************************************************
*
*   Copyright (C) 1997-2010, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  locdispnames.cpp
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2010feb25
*   created by: Markus W. Scherer
*
*   Code for locale display names, separated out from other .cpp files
*   that then do not depend on resource bundle code and display name data.
*/

#include "unicode/utypes.h"
#include "unicode/brkiter.h"
#include "unicode/locid.h"
#include "unicode/uloc.h"
#include "unicode/ures.h"
#include "unicode/ustring.h"
#include "cmemory.h"
#include "cstring.h"
#include "putilimp.h"
#include "ulocimp.h"
#include "uresimp.h"
#include "ureslocs.h"
#include "ustr_imp.h"

// C++ API ----------------------------------------------------------------- ***

U_NAMESPACE_BEGIN

UnicodeString&
Locale::getDisplayLanguage(UnicodeString& dispLang) const
{
    return this->getDisplayLanguage(getDefault(), dispLang);
}

/*We cannot make any assumptions on the size of the output display strings
* Yet, since we are calling through to a C API, we need to set limits on
* buffer size. For all the following getDisplay functions we first attempt
* to fill up a stack allocated buffer. If it is to small we heap allocated
* the exact buffer we need copy it to the UnicodeString and delete it*/

UnicodeString&
Locale::getDisplayLanguage(const Locale &displayLocale,
                           UnicodeString &result) const {
    UChar *buffer;
    UErrorCode errorCode=U_ZERO_ERROR;
    int32_t length;

    buffer=result.getBuffer(ULOC_FULLNAME_CAPACITY);
    if(buffer==0) {
        result.truncate(0);
        return result;
    }

    length=uloc_getDisplayLanguage(fullName, displayLocale.fullName,
                                   buffer, result.getCapacity(),
                                   &errorCode);
    result.releaseBuffer(U_SUCCESS(errorCode) ? length : 0);

    if(errorCode==U_BUFFER_OVERFLOW_ERROR) {
        buffer=result.getBuffer(length);
        if(buffer==0) {
            result.truncate(0);
            return result;
        }
        errorCode=U_ZERO_ERROR;
        length=uloc_getDisplayLanguage(fullName, displayLocale.fullName,
                                       buffer, result.getCapacity(),
                                       &errorCode);
        result.releaseBuffer(U_SUCCESS(errorCode) ? length : 0);
    }

    return result;
}

UnicodeString&
Locale::getDisplayScript(UnicodeString& dispScript) const
{
    return this->getDisplayScript(getDefault(), dispScript);
}

UnicodeString&
Locale::getDisplayScript(const Locale &displayLocale,
                          UnicodeString &result) const {
    UChar *buffer;
    UErrorCode errorCode=U_ZERO_ERROR;
    int32_t length;

    buffer=result.getBuffer(ULOC_FULLNAME_CAPACITY);
    if(buffer==0) {
        result.truncate(0);
        return result;
    }

    length=uloc_getDisplayScript(fullName, displayLocale.fullName,
                                  buffer, result.getCapacity(),
                                  &errorCode);
    result.releaseBuffer(U_SUCCESS(errorCode) ? length : 0);

    if(errorCode==U_BUFFER_OVERFLOW_ERROR) {
        buffer=result.getBuffer(length);
        if(buffer==0) {
            result.truncate(0);
            return result;
        }
        errorCode=U_ZERO_ERROR;
        length=uloc_getDisplayScript(fullName, displayLocale.fullName,
                                      buffer, result.getCapacity(),
                                      &errorCode);
        result.releaseBuffer(U_SUCCESS(errorCode) ? length : 0);
    }

    return result;
}

UnicodeString&
Locale::getDisplayCountry(UnicodeString& dispCntry) const
{
    return this->getDisplayCountry(getDefault(), dispCntry);
}

UnicodeString&
Locale::getDisplayCountry(const Locale &displayLocale,
                          UnicodeString &result) const {
    UChar *buffer;
    UErrorCode errorCode=U_ZERO_ERROR;
    int32_t length;

    buffer=result.getBuffer(ULOC_FULLNAME_CAPACITY);
    if(buffer==0) {
        result.truncate(0);
        return result;
    }

    length=uloc_getDisplayCountry(fullName, displayLocale.fullName,
                                  buffer, result.getCapacity(),
                                  &errorCode);
    result.releaseBuffer(U_SUCCESS(errorCode) ? length : 0);

    if(errorCode==U_BUFFER_OVERFLOW_ERROR) {
        buffer=result.getBuffer(length);
        if(buffer==0) {
            result.truncate(0);
            return result;
        }
        errorCode=U_ZERO_ERROR;
        length=uloc_getDisplayCountry(fullName, displayLocale.fullName,
                                      buffer, result.getCapacity(),
                                      &errorCode);
        result.releaseBuffer(U_SUCCESS(errorCode) ? length : 0);
    }

    return result;
}

UnicodeString&
Locale::getDisplayVariant(UnicodeString& dispVar) const
{
    return this->getDisplayVariant(getDefault(), dispVar);
}

UnicodeString&
Locale::getDisplayVariant(const Locale &displayLocale,
                          UnicodeString &result) const {
    UChar *buffer;
    UErrorCode errorCode=U_ZERO_ERROR;
    int32_t length;

    buffer=result.getBuffer(ULOC_FULLNAME_CAPACITY);
    if(buffer==0) {
        result.truncate(0);
        return result;
    }

    length=uloc_getDisplayVariant(fullName, displayLocale.fullName,
                                  buffer, result.getCapacity(),
                                  &errorCode);
    result.releaseBuffer(U_SUCCESS(errorCode) ? length : 0);

    if(errorCode==U_BUFFER_OVERFLOW_ERROR) {
        buffer=result.getBuffer(length);
        if(buffer==0) {
            result.truncate(0);
            return result;
        }
        errorCode=U_ZERO_ERROR;
        length=uloc_getDisplayVariant(fullName, displayLocale.fullName,
                                      buffer, result.getCapacity(),
                                      &errorCode);
        result.releaseBuffer(U_SUCCESS(errorCode) ? length : 0);
    }

    return result;
}

UnicodeString&
Locale::getDisplayName( UnicodeString& name ) const
{
    return this->getDisplayName(getDefault(), name);
}

UnicodeString&
Locale::getDisplayName(const Locale &displayLocale,
                       UnicodeString &result) const {
    UChar *buffer;
    UErrorCode errorCode=U_ZERO_ERROR;
    int32_t length;

    buffer=result.getBuffer(ULOC_FULLNAME_CAPACITY);
    if(buffer==0) {
        result.truncate(0);
        return result;
    }

    length=uloc_getDisplayName(fullName, displayLocale.fullName,
                               buffer, result.getCapacity(),
                               &errorCode);
    result.releaseBuffer(U_SUCCESS(errorCode) ? length : 0);

    if(errorCode==U_BUFFER_OVERFLOW_ERROR) {
        buffer=result.getBuffer(length);
        if(buffer==0) {
            result.truncate(0);
            return result;
        }
        errorCode=U_ZERO_ERROR;
        length=uloc_getDisplayName(fullName, displayLocale.fullName,
                                   buffer, result.getCapacity(),
                                   &errorCode);
        result.releaseBuffer(U_SUCCESS(errorCode) ? length : 0);
    }

    return result;
}

#if ! UCONFIG_NO_BREAK_ITERATION

// -------------------------------------
// Gets the objectLocale display name in the default locale language.
UnicodeString& U_EXPORT2
BreakIterator::getDisplayName(const Locale& objectLocale,
                             UnicodeString& name)
{
    return objectLocale.getDisplayName(name);
}

// -------------------------------------
// Gets the objectLocale display name in the displayLocale language.
UnicodeString& U_EXPORT2
BreakIterator::getDisplayName(const Locale& objectLocale,
                             const Locale& displayLocale,
                             UnicodeString& name)
{
    return objectLocale.getDisplayName(displayLocale, name);
}

#endif


U_NAMESPACE_END

// C API ------------------------------------------------------------------- ***

U_NAMESPACE_USE

/* ### Constants **************************************************/

/* These strings describe the resources we attempt to load from
 the locale ResourceBundle data file.*/
static const char _kLanguages[]       = "Languages";
static const char _kScripts[]         = "Scripts";
static const char _kCountries[]       = "Countries";
static const char _kVariants[]        = "Variants";
static const char _kKeys[]            = "Keys";
static const char _kTypes[]           = "Types";
static const char _kRootName[]        = "root";
static const char _kCurrency[]        = "currency";
static const char _kCurrencies[]      = "Currencies";
static const char _kLocaleDisplayPattern[] = "localeDisplayPattern";
static const char _kPattern[]         = "pattern";
static const char _kSeparator[]       = "separator";

/* ### Display name **************************************************/

static int32_t
_getStringOrCopyKey(const char *path, const char *locale,
                    const char *tableKey, 
                    const char* subTableKey,
                    const char *itemKey,
                    const char *substitute,
                    UChar *dest, int32_t destCapacity,
                    UErrorCode *pErrorCode) {
    const UChar *s = NULL;
    int32_t length = 0;

    if(itemKey==NULL) {
        /* top-level item: normal resource bundle access */
        UResourceBundle *rb;

        rb=ures_open(path, locale, pErrorCode);

        if(U_SUCCESS(*pErrorCode)) {
            s=ures_getStringByKey(rb, tableKey, &length, pErrorCode);
            /* see comment about closing rb near "return item;" in _res_getTableStringWithFallback() */
            ures_close(rb);
        }
    } else {
        /* Language code should not be a number. If it is, set the error code. */
        if (!uprv_strncmp(tableKey, "Languages", 9) && uprv_strtol(itemKey, NULL, 10)) {
            *pErrorCode = U_MISSING_RESOURCE_ERROR;
        } else {
            /* second-level item, use special fallback */
            s=uloc_getTableStringWithFallback(path, locale,
                                               tableKey, 
                                               subTableKey,
                                               itemKey,
                                               &length,
                                               pErrorCode);
        }
    }

    if(U_SUCCESS(*pErrorCode)) {
        int32_t copyLength=uprv_min(length, destCapacity);
        if(copyLength>0 && s != NULL) {
            u_memcpy(dest, s, copyLength);
        }
    } else {
        /* no string from a resource bundle: convert the substitute */
        length=(int32_t)uprv_strlen(substitute);
        u_charsToUChars(substitute, dest, uprv_min(length, destCapacity));
        *pErrorCode=U_USING_DEFAULT_WARNING;
    }

    return u_terminateUChars(dest, destCapacity, length, pErrorCode);
}

typedef  int32_t U_CALLCONV UDisplayNameGetter(const char *, char *, int32_t, UErrorCode *);

static int32_t
_getDisplayNameForComponent(const char *locale,
                            const char *displayLocale,
                            UChar *dest, int32_t destCapacity,
                            UDisplayNameGetter *getter,
                            const char *tag,
                            UErrorCode *pErrorCode) {
    char localeBuffer[ULOC_FULLNAME_CAPACITY*4];
    int32_t length;
    UErrorCode localStatus;
    const char* root = NULL;

    /* argument checking */
    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return 0;
    }

    if(destCapacity<0 || (destCapacity>0 && dest==NULL)) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    localStatus = U_ZERO_ERROR;
    length=(*getter)(locale, localeBuffer, sizeof(localeBuffer), &localStatus);
    if(U_FAILURE(localStatus) || localStatus==U_STRING_NOT_TERMINATED_WARNING) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }
    if(length==0) {
        return u_terminateUChars(dest, destCapacity, 0, pErrorCode);
    }

    root = tag == _kCountries ? U_ICUDATA_REGION : U_ICUDATA_LANG;

    return _getStringOrCopyKey(root, displayLocale,
                               tag, NULL, localeBuffer,
                               localeBuffer,
                               dest, destCapacity,
                               pErrorCode);
}

U_CAPI int32_t U_EXPORT2
uloc_getDisplayLanguage(const char *locale,
                        const char *displayLocale,
                        UChar *dest, int32_t destCapacity,
                        UErrorCode *pErrorCode) {
    return _getDisplayNameForComponent(locale, displayLocale, dest, destCapacity,
                uloc_getLanguage, _kLanguages, pErrorCode);
}

U_CAPI int32_t U_EXPORT2
uloc_getDisplayScript(const char* locale,
                      const char* displayLocale,
                      UChar *dest, int32_t destCapacity,
                      UErrorCode *pErrorCode)
{
    return _getDisplayNameForComponent(locale, displayLocale, dest, destCapacity,
                uloc_getScript, _kScripts, pErrorCode);
}

U_CAPI int32_t U_EXPORT2
uloc_getDisplayCountry(const char *locale,
                       const char *displayLocale,
                       UChar *dest, int32_t destCapacity,
                       UErrorCode *pErrorCode) {
    return _getDisplayNameForComponent(locale, displayLocale, dest, destCapacity,
                uloc_getCountry, _kCountries, pErrorCode);
}

/*
 * TODO separate variant1_variant2_variant3...
 * by getting each tag's display string and concatenating them with ", "
 * in between - similar to uloc_getDisplayName()
 */
U_CAPI int32_t U_EXPORT2
uloc_getDisplayVariant(const char *locale,
                       const char *displayLocale,
                       UChar *dest, int32_t destCapacity,
                       UErrorCode *pErrorCode) {
    return _getDisplayNameForComponent(locale, displayLocale, dest, destCapacity,
                uloc_getVariant, _kVariants, pErrorCode);
}

U_CAPI int32_t U_EXPORT2
uloc_getDisplayName(const char *locale,
                    const char *displayLocale,
                    UChar *dest, int32_t destCapacity,
                    UErrorCode *pErrorCode)
{
    int32_t length, length2, length3 = 0;
    UBool hasLanguage, hasScript, hasCountry, hasVariant, hasKeywords;
    UEnumeration* keywordEnum = NULL;
    int32_t keywordCount = 0;
    const char *keyword = NULL;
    int32_t keywordLen = 0;
    char keywordValue[256];
    int32_t keywordValueLen = 0;

    int32_t locSepLen = 0;
    int32_t locPatLen = 0;
    int32_t p0Len = 0;
    int32_t defaultPatternLen = 9;
    const UChar *dispLocSeparator;
    const UChar *dispLocPattern;
    static const UChar defaultSeparator[3] = { 0x002c, 0x0020 , 0x0000 }; /* comma + space */
    static const UChar defaultPattern[10] = { 0x007b, 0x0030, 0x007d, 0x0020, 0x0028, 0x007b, 0x0031, 0x007d, 0x0029, 0x0000 }; /* {0} ({1}) */
    static const UChar pat0[4] = { 0x007b, 0x0030, 0x007d , 0x0000 } ; /* {0} */
    static const UChar pat1[4] = { 0x007b, 0x0031, 0x007d , 0x0000 } ; /* {1} */
    
    UResourceBundle *bundle = NULL;
    UResourceBundle *locdsppat = NULL;
    
    UErrorCode status = U_ZERO_ERROR;

    /* argument checking */
    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return 0;
    }

    if(destCapacity<0 || (destCapacity>0 && dest==NULL)) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    bundle    = ures_open(U_ICUDATA_LANG, displayLocale, &status);

    locdsppat = ures_getByKeyWithFallback(bundle, _kLocaleDisplayPattern, NULL, &status);
    dispLocSeparator = ures_getStringByKeyWithFallback(locdsppat, _kSeparator, &locSepLen, &status);
    dispLocPattern = ures_getStringByKeyWithFallback(locdsppat, _kPattern, &locPatLen, &status);
        
    /*close the bundles */
    ures_close(locdsppat);
    ures_close(bundle);

    /* If we couldn't find any data, then use the defaults */
    if ( locSepLen == 0) {
       dispLocSeparator = defaultSeparator;
       locSepLen = 2;
    }

    if ( locPatLen == 0) {
       dispLocPattern = defaultPattern;
       locPatLen = 9;
    }

    /*
     * if there is a language, then write "language (country, variant)"
     * otherwise write "country, variant"
     */

    /* write the language */
    length=uloc_getDisplayLanguage(locale, displayLocale,
                                   dest, destCapacity,
                                   pErrorCode);
    hasLanguage= length>0;

    if(hasLanguage) {
        p0Len = length;

        /* append " (" */
        if(length<destCapacity) {
            dest[length]=0x20;
        }
        ++length;
        if(length<destCapacity) {
            dest[length]=0x28;
        }
        ++length;
    }

    if(*pErrorCode==U_BUFFER_OVERFLOW_ERROR) {
        /* keep preflighting */
        *pErrorCode=U_ZERO_ERROR;
    }

    /* append the script */
    if(length<destCapacity) {
        length2=uloc_getDisplayScript(locale, displayLocale,
                                       dest+length, destCapacity-length,
                                       pErrorCode);
    } else {
        length2=uloc_getDisplayScript(locale, displayLocale,
                                       NULL, 0,
                                       pErrorCode);
    }
    hasScript= length2>0;
    length+=length2;

    if(hasScript) {
        /* append separator */
        if(length+locSepLen<=destCapacity) {
            u_memcpy(dest+length,dispLocSeparator,locSepLen);
        }
        length+=locSepLen;
    }

    if(*pErrorCode==U_BUFFER_OVERFLOW_ERROR) {
        /* keep preflighting */
        *pErrorCode=U_ZERO_ERROR;
    }

    /* append the country */
    if(length<destCapacity) {
        length2=uloc_getDisplayCountry(locale, displayLocale,
                                       dest+length, destCapacity-length,
                                       pErrorCode);
    } else {
        length2=uloc_getDisplayCountry(locale, displayLocale,
                                       NULL, 0,
                                       pErrorCode);
    }
    hasCountry= length2>0;
    length+=length2;

    if(hasCountry) {
        /* append separator */
        if(length+locSepLen<=destCapacity) {
            u_memcpy(dest+length,dispLocSeparator,locSepLen);
        }
        length+=locSepLen;
    }

    if(*pErrorCode==U_BUFFER_OVERFLOW_ERROR) {
        /* keep preflighting */
        *pErrorCode=U_ZERO_ERROR;
    }

    /* append the variant */
    if(length<destCapacity) {
        length2=uloc_getDisplayVariant(locale, displayLocale,
                                       dest+length, destCapacity-length,
                                       pErrorCode);
    } else {
        length2=uloc_getDisplayVariant(locale, displayLocale,
                                       NULL, 0,
                                       pErrorCode);
    }
    hasVariant= length2>0;
    length+=length2;

    if(hasVariant) {
        /* append separator */
        if(length+locSepLen<=destCapacity) {
            u_memcpy(dest+length,dispLocSeparator,locSepLen);
        }
        length+=locSepLen;
    }

    keywordEnum = uloc_openKeywords(locale, pErrorCode);
    
    for(keywordCount = uenum_count(keywordEnum, pErrorCode); keywordCount > 0 ; keywordCount--){
          if(U_FAILURE(*pErrorCode)){
              break;
          }
          /* the uenum_next returns NUL terminated string */
          keyword = uenum_next(keywordEnum, &keywordLen, pErrorCode);
          if(length + length3 < destCapacity) {
            length3 += uloc_getDisplayKeyword(keyword, displayLocale, dest+length+length3, destCapacity-length-length3, pErrorCode);
          } else {
            length3 += uloc_getDisplayKeyword(keyword, displayLocale, NULL, 0, pErrorCode);
          }
          if(*pErrorCode==U_BUFFER_OVERFLOW_ERROR) {
              /* keep preflighting */
              *pErrorCode=U_ZERO_ERROR;
          }
          keywordValueLen = uloc_getKeywordValue(locale, keyword, keywordValue, 256, pErrorCode);
          if(keywordValueLen) {
            if(length + length3 < destCapacity) {
              dest[length + length3] = 0x3D;
            }
            length3++;
            if(length + length3 < destCapacity) {
              length3 += uloc_getDisplayKeywordValue(locale, keyword, displayLocale, dest+length+length3, destCapacity-length-length3, pErrorCode);
            } else {
              length3 += uloc_getDisplayKeywordValue(locale, keyword, displayLocale, NULL, 0, pErrorCode);
            }
            if(*pErrorCode==U_BUFFER_OVERFLOW_ERROR) {
                /* keep preflighting */
                *pErrorCode=U_ZERO_ERROR;
            }
          }
          if(keywordCount > 1) {
              if(length + length3 + locSepLen <= destCapacity && keywordCount) {
                  u_memcpy(dest+length+length3,dispLocSeparator,locSepLen);
                  length3+=locSepLen;
              }
          }
    }
    uenum_close(keywordEnum);

    hasKeywords = length3 > 0;
    length += length3;


    if ((hasScript && !hasCountry)
        || ((hasScript || hasCountry) && !hasVariant && !hasKeywords)
        || ((hasScript || hasCountry || hasVariant) && !hasKeywords)) {
        /* Remove separator  */
        length -= locSepLen;
    } else if (hasLanguage && !hasScript && !hasCountry && !hasVariant && !hasKeywords) {
        /* Remove " (" */
        length-=2;
    }

    if (hasLanguage && (hasScript || hasCountry || hasVariant || hasKeywords)) {
        /* append ")" */
        if(length<destCapacity) {
            dest[length]=0x29;
        }
        ++length;

        /* If the localized display pattern is something other than the default pattern of "{0} ({1})", then
         * then we need to do the formatting here.  It would be easier to use a messageFormat to do this, but we
         * can't since we don't have the APIs in the i18n library available to us at this point.
         */
        if (locPatLen != defaultPatternLen || u_strcmp(dispLocPattern,defaultPattern)) { /* Something other than the default pattern */
           UChar *p0 = u_strstr(dispLocPattern,pat0);
           UChar *p1 = u_strstr(dispLocPattern,pat1);
           u_terminateUChars(dest, destCapacity, length, pErrorCode);

           if ( p0 != NULL && p1 != NULL ) { /* The pattern is well formed */
              if ( dest ) {
                  int32_t destLen = 0;
                  UChar *result = (UChar *)uprv_malloc((length+1)*sizeof(UChar));
                  UChar *upos = (UChar *)dispLocPattern;
                  u_strcpy(result,dest);
                  dest[0] = 0;
                  while ( *upos ) {
                     if ( upos == p0 ) { /* Handle {0} substitution */
                         u_strncat(dest,result,p0Len);
                         destLen += p0Len;
                         dest[destLen] = 0; /* Null terminate */
                         upos += 3;
                     } else if ( upos == p1 ) { /* Handle {1} substitution */
                         UChar *p1Start = &result[p0Len+2];
                         u_strncat(dest,p1Start,length-p0Len-3);
                         destLen += (length-p0Len-3);
                         dest[destLen] = 0; /* Null terminate */
                         upos += 3;
                     } else { /* Something from the pattern not {0} or {1} */
                         u_strncat(dest,upos,1);
                         upos++;
                         destLen++;
                         dest[destLen] = 0; /* Null terminate */
                     }
                  }
                  length = destLen;
                  uprv_free(result);
              }
           }
        }
    }
    if(*pErrorCode==U_BUFFER_OVERFLOW_ERROR) {
        /* keep preflighting */
        *pErrorCode=U_ZERO_ERROR;
    }

    return u_terminateUChars(dest, destCapacity, length, pErrorCode);
}

U_CAPI int32_t U_EXPORT2
uloc_getDisplayKeyword(const char* keyword,
                       const char* displayLocale,
                       UChar* dest,
                       int32_t destCapacity,
                       UErrorCode* status){

    /* argument checking */
    if(status==NULL || U_FAILURE(*status)) {
        return 0;
    }

    if(destCapacity<0 || (destCapacity>0 && dest==NULL)) {
        *status=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }


    /* pass itemKey=NULL to look for a top-level item */
    return _getStringOrCopyKey(U_ICUDATA_LANG, displayLocale,
                               _kKeys, NULL, 
                               keyword, 
                               keyword,      
                               dest, destCapacity,
                               status);

}


#define UCURRENCY_DISPLAY_NAME_INDEX 1

U_CAPI int32_t U_EXPORT2
uloc_getDisplayKeywordValue(   const char* locale,
                               const char* keyword,
                               const char* displayLocale,
                               UChar* dest,
                               int32_t destCapacity,
                               UErrorCode* status){


    char keywordValue[ULOC_FULLNAME_CAPACITY*4];
    int32_t capacity = ULOC_FULLNAME_CAPACITY*4;
    int32_t keywordValueLen =0;

    /* argument checking */
    if(status==NULL || U_FAILURE(*status)) {
        return 0;
    }

    if(destCapacity<0 || (destCapacity>0 && dest==NULL)) {
        *status=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    /* get the keyword value */
    keywordValue[0]=0;
    keywordValueLen = uloc_getKeywordValue(locale, keyword, keywordValue, capacity, status);

    /* 
     * if the keyword is equal to currency .. then to get the display name 
     * we need to do the fallback ourselves
     */
    if(uprv_stricmp(keyword, _kCurrency)==0){

        int32_t dispNameLen = 0;
        const UChar *dispName = NULL;
        
        UResourceBundle *bundle     = ures_open(U_ICUDATA_CURR, displayLocale, status);
        UResourceBundle *currencies = ures_getByKey(bundle, _kCurrencies, NULL, status);
        UResourceBundle *currency   = ures_getByKeyWithFallback(currencies, keywordValue, NULL, status);
        
        dispName = ures_getStringByIndex(currency, UCURRENCY_DISPLAY_NAME_INDEX, &dispNameLen, status);
        
        /*close the bundles */
        ures_close(currency);
        ures_close(currencies);
        ures_close(bundle);
        
        if(U_FAILURE(*status)){
            if(*status == U_MISSING_RESOURCE_ERROR){
                /* we just want to write the value over if nothing is available */
                *status = U_USING_DEFAULT_WARNING;
            }else{
                return 0;
            }
        }

        /* now copy the dispName over if not NULL */
        if(dispName != NULL){
            if(dispNameLen <= destCapacity){
                uprv_memcpy(dest, dispName, dispNameLen * U_SIZEOF_UCHAR);
                return u_terminateUChars(dest, destCapacity, dispNameLen, status);
            }else{
                *status = U_BUFFER_OVERFLOW_ERROR;
                return dispNameLen;
            }
        }else{
            /* we have not found the display name for the value .. just copy over */
            if(keywordValueLen <= destCapacity){
                u_charsToUChars(keywordValue, dest, keywordValueLen);
                return u_terminateUChars(dest, destCapacity, keywordValueLen, status);
            }else{
                 *status = U_BUFFER_OVERFLOW_ERROR;
                return keywordValueLen;
            }
        }

        
    }else{

        return _getStringOrCopyKey(U_ICUDATA_LANG, displayLocale,
                                   _kTypes, keyword, 
                                   keywordValue,
                                   keywordValue,
                                   dest, destCapacity,
                                   status);
    }
}
