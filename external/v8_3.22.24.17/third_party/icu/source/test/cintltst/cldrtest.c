/********************************************************************
 * COPYRIGHT:
 * Copyright (c) 1997-2010, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

#include "cintltst.h"
#include "unicode/ures.h"
#include "unicode/ucurr.h"
#include "unicode/ustring.h"
#include "unicode/uset.h"
#include "unicode/udat.h"
#include "unicode/uscript.h"
#include "unicode/ulocdata.h"
#include "cstring.h"
#include "locmap.h"
#include "uresimp.h"

/*
returns a new UnicodeSet that is a flattened form of the original
UnicodeSet.
*/
static USet*
createFlattenSet(USet *origSet, UErrorCode *status) {


    USet *newSet = NULL;
    int32_t origItemCount = 0;
    int32_t idx, graphmeSize;
    UChar32 start, end;
    UChar graphme[64];
    if (U_FAILURE(*status)) {
        log_err("createFlattenSet called with %s\n", u_errorName(*status));
        return NULL;
    }
    newSet = uset_open(1, 0);
    origItemCount = uset_getItemCount(origSet);
    for (idx = 0; idx < origItemCount; idx++) {
        graphmeSize = uset_getItem(origSet, idx,
            &start, &end,
            graphme, (int32_t)(sizeof(graphme)/sizeof(graphme[0])),
            status);
        if (U_FAILURE(*status)) {
            log_err("ERROR: uset_getItem returned %s\n", u_errorName(*status));
            *status = U_ZERO_ERROR;
        }
        if (graphmeSize) {
            uset_addAllCodePoints(newSet, graphme, graphmeSize);
        }
        else {
            uset_addRange(newSet, start, end);
        }
    }
    return newSet;
}
static UBool
isCurrencyPreEuro(const char* currencyKey){
    if( strcmp(currencyKey, "PTE") == 0 ||
        strcmp(currencyKey, "ESP") == 0 ||
        strcmp(currencyKey, "LUF") == 0 ||
        strcmp(currencyKey, "GRD") == 0 ||
        strcmp(currencyKey, "BEF") == 0 ||
        strcmp(currencyKey, "ITL") == 0 ||
        strcmp(currencyKey, "EEK") == 0){
            return TRUE;
    }
    return FALSE;
}
static void
TestKeyInRootRecursive(UResourceBundle *root, const char *rootName,
                       UResourceBundle *currentBundle, const char *locale) {
    UErrorCode errorCode = U_ZERO_ERROR;
    UResourceBundle *subRootBundle = NULL, *subBundle = NULL, *arr = NULL;

    ures_resetIterator(root);
    ures_resetIterator(currentBundle);
    while (ures_hasNext(currentBundle)) {
        const char *subBundleKey = NULL;
        const char *currentBundleKey = NULL;

        errorCode = U_ZERO_ERROR;
        currentBundleKey = ures_getKey(currentBundle);
        subBundle = ures_getNextResource(currentBundle, NULL, &errorCode);
        if (U_FAILURE(errorCode)) {
            log_err("Can't open a resource for lnocale %s. Error: %s\n", locale, u_errorName(errorCode));
            continue;
        }
        subBundleKey = ures_getKey(subBundle);


        subRootBundle = ures_getByKey(root, subBundleKey, NULL, &errorCode);
        if (U_FAILURE(errorCode)) {
            log_err("Can't open a resource with key \"%s\" in \"%s\" from %s for locale \"%s\"\n",
                    subBundleKey,
                    ures_getKey(currentBundle),
                    rootName,
                    locale);
            ures_close(subBundle);
            continue;
        }
        if (ures_getType(subRootBundle) != ures_getType(subBundle)) {
            log_err("key \"%s\" in \"%s\" has a different type from root for locale \"%s\"\n"
                    "\troot=%d, locale=%d\n",
                    subBundleKey,
                    ures_getKey(currentBundle),
                    locale,
                    ures_getType(subRootBundle),
                    ures_getType(subBundle));
            ures_close(subBundle);
            continue;
        }
        else if (ures_getType(subBundle) == URES_INT_VECTOR) {
            int32_t minSize;
            int32_t subBundleSize;
            int32_t idx;
            UBool sameArray = TRUE;
            const int32_t *subRootBundleArr = ures_getIntVector(subRootBundle, &minSize, &errorCode);
            const int32_t *subBundleArr = ures_getIntVector(subBundle, &subBundleSize, &errorCode);

            if (minSize > subBundleSize) {
                minSize = subBundleSize;
                log_err("Arrays are different size with key \"%s\" in \"%s\" from root for locale \"%s\"\n",
                        subBundleKey,
                        ures_getKey(currentBundle),
                        locale);
            }

            for (idx = 0; idx < minSize && sameArray; idx++) {
                if (subRootBundleArr[idx] != subBundleArr[idx]) {
                    sameArray = FALSE;
                }
                if (strcmp(subBundleKey, "DateTimeElements") == 0
                    && (subBundleArr[idx] < 1 || 7 < subBundleArr[idx]))
                {
                    log_err("Value out of range with key \"%s\" at index %d in \"%s\" for locale \"%s\"\n",
                            subBundleKey,
                            idx,
                            ures_getKey(currentBundle),
                            locale);
                }
            }
            /* Special exception es_US and DateTimeElements */
            if (sameArray
                && !(strcmp(locale, "es_US") == 0 && strcmp(subBundleKey, "DateTimeElements") == 0))
            {
                log_err("Integer vectors are the same with key \"%s\" in \"%s\" from root for locale \"%s\"\n",
                        subBundleKey,
                        ures_getKey(currentBundle),
                        locale);
            }
        }
        else if (ures_getType(subBundle) == URES_ARRAY) {
            UResourceBundle *subSubBundle = ures_getByIndex(subBundle, 0, NULL, &errorCode);
            UResourceBundle *subSubRootBundle = ures_getByIndex(subRootBundle, 0, NULL, &errorCode);

            if (U_SUCCESS(errorCode)
                && (ures_getType(subSubBundle) == URES_ARRAY || ures_getType(subSubRootBundle) == URES_ARRAY))
            {
                /* Here is one of the recursive parts */
                TestKeyInRootRecursive(subRootBundle, rootName, subBundle, locale);
            }
            else {
                int32_t minSize = ures_getSize(subRootBundle);
                int32_t idx;
                UBool sameArray = TRUE;

                if (minSize > ures_getSize(subBundle)) {
                    minSize = ures_getSize(subBundle);
                }

                if ((subBundleKey == NULL
                    || (subBundleKey != NULL &&  strcmp(subBundleKey, "LocaleScript") != 0 && !isCurrencyPreEuro(subBundleKey)))
                    && ures_getSize(subRootBundle) != ures_getSize(subBundle))
                {
                    log_err("Different size array with key \"%s\" in \"%s\" from root for locale \"%s\"\n"
                            "\troot array size=%d, locale array size=%d\n",
                            subBundleKey,
                            ures_getKey(currentBundle),
                            locale,
                            ures_getSize(subRootBundle),
                            ures_getSize(subBundle));
                }
                /*
                if(isCurrencyPreEuro(subBundleKey) && ures_getSize(subBundle)!=3){
                    log_err("Different size array with key \"%s\" in \"%s\" for locale \"%s\" the expected size is 3 got size=%d\n",
                            subBundleKey,
                            ures_getKey(currentBundle),
                            locale,
                            ures_getSize(subBundle));
                }
                */
                for (idx = 0; idx < minSize; idx++) {
                    int32_t rootStrLen, localeStrLen;
                    const UChar *rootStr = ures_getStringByIndex(subRootBundle,idx,&rootStrLen,&errorCode);
                    const UChar *localeStr = ures_getStringByIndex(subBundle,idx,&localeStrLen,&errorCode);
                    if (rootStr && localeStr && U_SUCCESS(errorCode)) {
                        if (u_strcmp(rootStr, localeStr) != 0) {
                            sameArray = FALSE;
                        }
                    }
                    else {
                        if ( rootStrLen > 1 && rootStr[0] == 0x41 && rootStr[1] >= 0x30 && rootStr[1] <= 0x39 ) {
                           /* A2 or A4 in the root string indicates that the resource can optionally be an array instead of a */
                           /* string.  Attempt to read it as an array. */
                          errorCode = U_ZERO_ERROR;
                          arr = ures_getByIndex(subBundle,idx,NULL,&errorCode);
                          if (U_FAILURE(errorCode)) {
                              log_err("Got a NULL string with key \"%s\" in \"%s\" at index %d for root or locale \"%s\"\n",
                                      subBundleKey,
                                      ures_getKey(currentBundle),
                                      idx,
                                      locale);
                              continue;
                          }
                          if (ures_getType(arr) != URES_ARRAY || ures_getSize(arr) != (int32_t)rootStr[1] - 0x30) {
                              log_err("Got something other than a string or array of size %d for key \"%s\" in \"%s\" at index %d for root or locale \"%s\"\n",
                                      rootStr[1] - 0x30,
                                      subBundleKey,
                                      ures_getKey(currentBundle),
                                      idx,
                                      locale);
                              ures_close(arr);
                              continue;
                          }
                          localeStr = ures_getStringByIndex(arr,0,&localeStrLen,&errorCode);
                          ures_close(arr);
                          if (U_FAILURE(errorCode)) {
                              log_err("Got something other than a string or array for key \"%s\" in \"%s\" at index %d for root or locale \"%s\"\n",
                                      subBundleKey,
                                      ures_getKey(currentBundle),
                                      idx,
                                      locale);
                              continue;
                          }
                        } else {
                            log_err("Got a NULL string with key \"%s\" in \"%s\" at index %d for root or locale \"%s\"\n",
                                subBundleKey,
                                ures_getKey(currentBundle),
                                idx,
                                locale);
                            continue;
                        }
                    }
                    if (localeStr[0] == (UChar)0x20) {
                        log_err("key \"%s\" at index %d in \"%s\" starts with a space in locale \"%s\"\n",
                                subBundleKey,
                                idx,
                                ures_getKey(currentBundle),
                                locale);
                    }
                    else if ((localeStr[localeStrLen - 1] == (UChar)0x20) && (strcmp(subBundleKey,"separator") != 0)) {
                        log_err("key \"%s\" at index %d in \"%s\" ends with a space in locale \"%s\"\n",
                                subBundleKey,
                                idx,
                                ures_getKey(currentBundle),
                                locale);
                    }
                    else if (subBundleKey != NULL
                        && strcmp(subBundleKey, "DateTimePatterns") == 0)
                    {
                        int32_t quoted = 0;
                        const UChar *localeStrItr = localeStr;
                        while (*localeStrItr) {
                            if (*localeStrItr == (UChar)0x27 /* ' */) {
                                quoted++;
                            }
                            else if ((quoted % 2) == 0) {
                                /* Search for unquoted characters */
                                if (4 <= idx && idx <= 7
                                    && (*localeStrItr == (UChar)0x6B /* k */
                                    || *localeStrItr == (UChar)0x48 /* H */
                                    || *localeStrItr == (UChar)0x6D /* m */
                                    || *localeStrItr == (UChar)0x73 /* s */
                                    || *localeStrItr == (UChar)0x53 /* S */
                                    || *localeStrItr == (UChar)0x61 /* a */
                                    || *localeStrItr == (UChar)0x68 /* h */
                                    || *localeStrItr == (UChar)0x7A /* z */))
                                {
                                    log_err("key \"%s\" at index %d has time pattern chars in date for locale \"%s\"\n",
                                            subBundleKey,
                                            idx,
                                            locale);
                                }
                                else if (0 <= idx && idx <= 3
                                    && (*localeStrItr == (UChar)0x47 /* G */
                                    || *localeStrItr == (UChar)0x79 /* y */
                                    || *localeStrItr == (UChar)0x4D /* M */
                                    || *localeStrItr == (UChar)0x64 /* d */
                                    || *localeStrItr == (UChar)0x45 /* E */
                                    || *localeStrItr == (UChar)0x44 /* D */
                                    || *localeStrItr == (UChar)0x46 /* F */
                                    || *localeStrItr == (UChar)0x77 /* w */
                                    || *localeStrItr == (UChar)0x57 /* W */))
                                {
                                    log_err("key \"%s\" at index %d has date pattern chars in time for locale \"%s\"\n",
                                            subBundleKey,
                                            idx,
                                            locale);
                                }
                            }
                            localeStrItr++;
                        }
                    }
                    else if (idx == 4 && subBundleKey != NULL
                        && strcmp(subBundleKey, "NumberElements") == 0
                        && u_charDigitValue(localeStr[0]) != 0)
                    {
                        log_err("key \"%s\" at index %d has a non-zero based number for locale \"%s\"\n",
                                subBundleKey,
                                idx,
                                locale);
                    }
                }
/*                if (sameArray && strcmp(rootName, "root") == 0) {
                    log_err("Arrays are the same with key \"%s\" in \"%s\" from root for locale \"%s\"\n",
                            subBundleKey,
                            ures_getKey(currentBundle),
                            locale);
                }*/
            }
            ures_close(subSubBundle);
            ures_close(subSubRootBundle);
        }
        else if (ures_getType(subBundle) == URES_STRING) {
            int32_t len = 0;
            const UChar *string = ures_getString(subBundle, &len, &errorCode);
            if (U_FAILURE(errorCode) || string == NULL) {
                log_err("Can't open a string with key \"%s\" in \"%s\" for locale \"%s\"\n",
                        subBundleKey,
                        ures_getKey(currentBundle),
                        locale);
            } else if (string[0] == (UChar)0x20) {
                log_err("key \"%s\" in \"%s\" starts with a space in locale \"%s\"\n",
                        subBundleKey,
                        ures_getKey(currentBundle),
                        locale);
            /* localeDisplayPattern/separator can end with a space */
            } else if (string[len - 1] == (UChar)0x20 && (strcmp(subBundleKey,"separator"))) {
                log_err("key \"%s\" in \"%s\" ends with a space in locale \"%s\"\n",
                        subBundleKey,
                        ures_getKey(currentBundle),
                        locale);
            } else if (strcmp(subBundleKey, "localPatternChars") == 0) {
                /* Note: We no longer import localPatternChars data starting
                 * ICU 3.8.  So it never comes into this else if block. (ticket#5597)
                 */

                /* Check well-formedness of localPatternChars.  First, the
                 * length must match the number of fields defined by
                 * DateFormat.  Second, each character in the string must
                 * be in the set [A-Za-z].  Finally, each character must be
                 * unique.
                 */
                int32_t i,j;
#if !UCONFIG_NO_FORMATTING
                if (len != UDAT_FIELD_COUNT) {
                    log_err("key \"%s\" has the wrong number of characters in locale \"%s\"\n",
                            subBundleKey,
                            locale);
                }
#endif
                /* Check char validity. */
                for (i=0; i<len; ++i) {
                    if (!((string[i] >= 65/*'A'*/ && string[i] <= 90/*'Z'*/) ||
                          (string[i] >= 97/*'a'*/ && string[i] <= 122/*'z'*/))) {
                        log_err("key \"%s\" has illegal character '%c' in locale \"%s\"\n",
                                subBundleKey,
                                (char) string[i],
                                locale);
                    }
                    /* Do O(n^2) check for duplicate chars. */
                    for (j=0; j<i; ++j) {
                        if (string[j] == string[i]) {
                            log_err("key \"%s\" has duplicate character '%c' in locale \"%s\"\n",
                                    subBundleKey,
                                    (char) string[i],
                                    locale);
                        }
                    }
                }
            }
            /* No fallback was done. Check for duplicate data */
            /* The ures_* API does not do fallback of sub-resource bundles,
               So we can't do this now. */
#if 0
            else if (strcmp(locale, "root") != 0 && errorCode == U_ZERO_ERROR) {

                const UChar *rootString = ures_getString(subRootBundle, &len, &errorCode);
                if (U_FAILURE(errorCode) || rootString == NULL) {
                    log_err("Can't open a string with key \"%s\" in \"%s\" in root\n",
                            ures_getKey(subRootBundle),
                            ures_getKey(currentBundle));
                    continue;
                } else if (u_strcmp(string, rootString) == 0) {
                    if (strcmp(locale, "de_CH") != 0 && strcmp(subBundleKey, "Countries") != 0 &&
                        strcmp(subBundleKey, "Version") != 0) {
                        log_err("Found duplicate data with key \"%s\" in \"%s\" in locale \"%s\"\n",
                                ures_getKey(subRootBundle),
                                ures_getKey(currentBundle),
                                locale);
                    }
                    else {
                        /* Ignore for now. */
                        /* Can be fixed if fallback through de locale was done. */
                        log_verbose("Skipping key %s in %s\n", subBundleKey, locale);
                    }
                }
            }
#endif
        }
        else if (ures_getType(subBundle) == URES_TABLE) {
            if (strcmp(subBundleKey, "availableFormats")!=0) {
                /* Here is one of the recursive parts */
                TestKeyInRootRecursive(subRootBundle, rootName, subBundle, locale);
            }
            else {
                log_verbose("Skipping key %s in %s\n", subBundleKey, locale);
            }
        }
        else if (ures_getType(subBundle) == URES_BINARY || ures_getType(subBundle) == URES_INT) {
            /* Can't do anything to check it */
            /* We'll assume it's all correct */
            if (strcmp(subBundleKey, "MeasurementSystem") != 0) {
                log_verbose("Skipping key \"%s\" in \"%s\" for locale \"%s\"\n",
                        subBundleKey,
                        ures_getKey(currentBundle),
                        locale);
            }
            /* Testing for MeasurementSystem is done in VerifyTranslation */
        }
        else {
            log_err("Type %d for key \"%s\" in \"%s\" is unknown for locale \"%s\"\n",
                    ures_getType(subBundle),
                    subBundleKey,
                    ures_getKey(currentBundle),
                    locale);
        }
        ures_close(subRootBundle);
        ures_close(subBundle);
    }
}


static void
testLCID(UResourceBundle *currentBundle,
         const char *localeName)
{
    UErrorCode status = U_ZERO_ERROR;
    uint32_t expectedLCID;
    char lcidStringC[64] = {0};

    expectedLCID = uloc_getLCID(localeName);
    if (expectedLCID == 0) {
        log_verbose("INFO:    %-5s does not have any LCID mapping\n",
            localeName);
        return;
    }

    status = U_ZERO_ERROR;
    uprv_strcpy(lcidStringC, uprv_convertToPosix(expectedLCID, &status));
    if (U_FAILURE(status)) {
        log_err("ERROR:   %.4x does not have a POSIX mapping due to %s\n",
            expectedLCID, u_errorName(status));
    }

    if(strcmp(localeName, lcidStringC) != 0) {
        char langName[1024];
        char langLCID[1024];
        uloc_getLanguage(localeName, langName, sizeof(langName), &status);
        uloc_getLanguage(lcidStringC, langLCID, sizeof(langLCID), &status);

        if (strcmp(langName, langLCID) == 0) {
            log_verbose("WARNING: %-5s resolves to %s (0x%.4x)\n",
                localeName, lcidStringC, expectedLCID);
        }
        else {
            log_err("ERROR:   %-5s has 0x%.4x and the number resolves wrongfully to %s\n",
                localeName, expectedLCID, lcidStringC);
        }
    }
}

static void
TestLocaleStructure(void) {
    UResourceBundle *root, *currentLocale;
    int32_t locCount = uloc_countAvailable();
    int32_t locIndex;
    UErrorCode errorCode = U_ZERO_ERROR;
    const char *currLoc, *resolvedLoc;

    /* TODO: Compare against parent's data too. This code can't handle fallbacks that some tools do already. */
/*    char locName[ULOC_FULLNAME_CAPACITY];
    char *locNamePtr;

    for (locIndex = 0; locIndex < locCount; locIndex++) {
        errorCode=U_ZERO_ERROR;
        strcpy(locName, uloc_getAvailable(locIndex));
        locNamePtr = strrchr(locName, '_');
        if (locNamePtr) {
            *locNamePtr = 0;
        }
        else {
            strcpy(locName, "root");
        }

        root = ures_openDirect(NULL, locName, &errorCode);
        if(U_FAILURE(errorCode)) {
            log_err("Can't open %s\n", locName);
            continue;
        }
*/
    if (locCount <= 1) {
        log_data_err("At least root needs to be installed\n");
    }

    root = ures_openDirect(loadTestData(&errorCode), "structLocale", &errorCode);
    if(U_FAILURE(errorCode)) {
        log_data_err("Can't open structLocale\n");
        return;
    }
    for (locIndex = 0; locIndex < locCount; locIndex++) {
        errorCode=U_ZERO_ERROR;
        currLoc = uloc_getAvailable(locIndex);
        currentLocale = ures_open(NULL, currLoc, &errorCode);
        if(errorCode != U_ZERO_ERROR) {
            if(U_SUCCESS(errorCode)) {
                /* It's installed, but there is no data.
                   It's installed for the g18n white paper [grhoten] */
                log_err("ERROR: Locale %-5s not installed, and it should be, err %s\n",
                    uloc_getAvailable(locIndex), u_errorName(errorCode));
            } else {
                log_err("%%%%%%% Unexpected error %d in %s %%%%%%%",
                    u_errorName(errorCode),
                    uloc_getAvailable(locIndex));
            }
            ures_close(currentLocale);
            continue;
        }
        ures_getStringByKey(currentLocale, "Version", NULL, &errorCode);
        if(errorCode != U_ZERO_ERROR) {
            log_err("No version information is available for locale %s, and it should be!\n",
                currLoc);
        }
        else if (ures_getStringByKey(currentLocale, "Version", NULL, &errorCode)[0] == (UChar)(0x78)) {
            log_verbose("WARNING: The locale %s is experimental! It shouldn't be listed as an installed locale.\n",
                currLoc);
        }
        resolvedLoc = ures_getLocaleByType(currentLocale, ULOC_ACTUAL_LOCALE, &errorCode);
        if (strcmp(resolvedLoc, currLoc) != 0) {
            /* All locales have at least a Version resource.
               If it's absolutely empty, then the previous test will fail too.*/
            log_err("Locale resolves to different locale. Is %s an alias of %s?\n",
                currLoc, resolvedLoc);
        }
        TestKeyInRootRecursive(root, "root", currentLocale, currLoc);

        testLCID(currentLocale, currLoc);

        ures_close(currentLocale);
    }

    ures_close(root);
}

static void
compareArrays(const char *keyName,
              UResourceBundle *fromArray, const char *fromLocale,
              UResourceBundle *toArray, const char *toLocale,
              int32_t start, int32_t end)
{
    int32_t fromSize = ures_getSize(fromArray);
    int32_t toSize = ures_getSize(fromArray);
    int32_t idx;
    UErrorCode errorCode = U_ZERO_ERROR;

    if (fromSize > toSize) {
        fromSize = toSize;
        log_err("Arrays are different size from \"%s\" to \"%s\"\n",
                fromLocale,
                toLocale);
    }

    for (idx = start; idx <= end; idx++) {
        const UChar *fromBundleStr = ures_getStringByIndex(fromArray, idx, NULL, &errorCode);
        const UChar *toBundleStr = ures_getStringByIndex(toArray, idx, NULL, &errorCode);
        if (fromBundleStr && toBundleStr && u_strcmp(fromBundleStr, toBundleStr) != 0)
        {
            log_err("Difference for %s at index %d from %s= \"%s\" to %s= \"%s\"\n",
                    keyName,
                    idx,
                    fromLocale,
                    austrdup(fromBundleStr),
                    toLocale,
                    austrdup(toBundleStr));
        }
    }
}

static void
compareConsistentCountryInfo(const char *fromLocale, const char *toLocale) {
    UErrorCode errorCode = U_ZERO_ERROR;
    UResourceBundle *fromDateTimeElements, *toDateTimeElements, *fromWeekendData = NULL, *toWeekendData = NULL;
    UResourceBundle *fromArray, *toArray;
    UResourceBundle *fromLocaleBund = ures_open(NULL, fromLocale, &errorCode);
    UResourceBundle *toLocaleBund = ures_open(NULL, toLocale, &errorCode);
    UResourceBundle *toCalendar, *fromCalendar, *toGregorian, *fromGregorian;

    if(U_FAILURE(errorCode)) {
        log_err("Can't open resource bundle %s or %s - %s\n", fromLocale, toLocale, u_errorName(errorCode));
        return;
    }
    fromCalendar = ures_getByKey(fromLocaleBund, "calendar", NULL, &errorCode);
    fromGregorian = ures_getByKeyWithFallback(fromCalendar, "gregorian", NULL, &errorCode);
    fromDateTimeElements = ures_getByKeyWithFallback(fromGregorian, "DateTimeElements", NULL, &errorCode);

    toCalendar = ures_getByKey(toLocaleBund, "calendar", NULL, &errorCode);
    toGregorian = ures_getByKeyWithFallback(toCalendar, "gregorian", NULL, &errorCode);
    toDateTimeElements = ures_getByKeyWithFallback(toGregorian, "DateTimeElements", NULL, &errorCode);

    if(U_FAILURE(errorCode)){
        log_err("Did not get DateTimeElements from the bundle %s or %s\n", fromLocale, toLocale);
        goto cleanup;
    }

    fromWeekendData = ures_getByKeyWithFallback(fromGregorian, "weekend", NULL, &errorCode);
    if(U_FAILURE(errorCode)){
        log_err("Did not get weekend data from the bundle %s to compare against %s\n", fromLocale, toLocale);
        goto cleanup;
    }
    toWeekendData = ures_getByKeyWithFallback(toGregorian, "weekend", NULL, &errorCode);
    if(U_FAILURE(errorCode)){
        log_err("Did not get weekend data from the bundle %s to compare against %s\n", toLocale, fromLocale);
        goto cleanup;
    }

    if (strcmp(fromLocale, "ar_IN") != 0)
    {
        int32_t fromSize;
        int32_t toSize;
        int32_t idx;
        const int32_t *fromBundleArr = ures_getIntVector(fromDateTimeElements, &fromSize, &errorCode);
        const int32_t *toBundleArr = ures_getIntVector(toDateTimeElements, &toSize, &errorCode);

        if (fromSize > toSize) {
            fromSize = toSize;
            log_err("Arrays are different size with key \"DateTimeElements\" from \"%s\" to \"%s\"\n",
                    fromLocale,
                    toLocale);
        }

        for (idx = 0; idx < fromSize; idx++) {
            if (fromBundleArr[idx] != toBundleArr[idx]) {
                log_err("Difference with key \"DateTimeElements\" at index %d from \"%s\" to \"%s\"\n",
                        idx,
                        fromLocale,
                        toLocale);
            }
        }
    }

    /* test for weekend data */
    {
        int32_t fromSize;
        int32_t toSize;
        int32_t idx;
        const int32_t *fromBundleArr = ures_getIntVector(fromWeekendData, &fromSize, &errorCode);
        const int32_t *toBundleArr = ures_getIntVector(toWeekendData, &toSize, &errorCode);

        if (fromSize > toSize) {
            fromSize = toSize;
            log_err("Arrays are different size with key \"weekend\" data from \"%s\" to \"%s\"\n",
                    fromLocale,
                    toLocale);
        }

        for (idx = 0; idx < fromSize; idx++) {
            if (fromBundleArr[idx] != toBundleArr[idx]) {
                log_err("Difference with key \"weekend\" data at index %d from \"%s\" to \"%s\"\n",
                        idx,
                        fromLocale,
                        toLocale);
            }
        }
    }

    fromArray = ures_getByKey(fromLocaleBund, "CurrencyElements", NULL, &errorCode);
    toArray = ures_getByKey(toLocaleBund, "CurrencyElements", NULL, &errorCode);
    if (strcmp(fromLocale, "en_CA") != 0)
    {
        /* The first one is probably localized. */
        compareArrays("CurrencyElements", fromArray, fromLocale, toArray, toLocale, 1, 2);
    }
    ures_close(fromArray);
    ures_close(toArray);

    fromArray = ures_getByKey(fromLocaleBund, "NumberPatterns", NULL, &errorCode);
    toArray = ures_getByKey(toLocaleBund, "NumberPatterns", NULL, &errorCode);
    if (strcmp(fromLocale, "en_CA") != 0)
    {
        compareArrays("NumberPatterns", fromArray, fromLocale, toArray, toLocale, 0, 3);
    }
    ures_close(fromArray);
    ures_close(toArray);

    /* Difficult to test properly */
/*
    fromArray = ures_getByKey(fromLocaleBund, "DateTimePatterns", NULL, &errorCode);
    toArray = ures_getByKey(toLocaleBund, "DateTimePatterns", NULL, &errorCode);
    {
        compareArrays("DateTimePatterns", fromArray, fromLocale, toArray, toLocale);
    }
    ures_close(fromArray);
    ures_close(toArray);*/

    fromArray = ures_getByKey(fromLocaleBund, "NumberElements", NULL, &errorCode);
    toArray = ures_getByKey(toLocaleBund, "NumberElements", NULL, &errorCode);
    if (strcmp(fromLocale, "en_CA") != 0)
    {
        compareArrays("NumberElements", fromArray, fromLocale, toArray, toLocale, 0, 3);
        /* Index 4 is a script based 0 */
        compareArrays("NumberElements", fromArray, fromLocale, toArray, toLocale, 5, 10);
    }
    ures_close(fromArray);
    ures_close(toArray);

cleanup:
    ures_close(fromDateTimeElements);
    ures_close(toDateTimeElements);
    ures_close(fromWeekendData);
    ures_close(toWeekendData);

    ures_close(fromCalendar);
    ures_close(toCalendar);
    ures_close(fromGregorian);
    ures_close(toGregorian);

    ures_close(fromLocaleBund);
    ures_close(toLocaleBund);
}

static void
TestConsistentCountryInfo(void) {
/*    UResourceBundle *fromLocale, *toLocale;*/
    int32_t locCount = uloc_countAvailable();
    int32_t fromLocIndex, toLocIndex;

    int32_t fromCountryLen, toCountryLen;
    char fromCountry[ULOC_FULLNAME_CAPACITY], toCountry[ULOC_FULLNAME_CAPACITY];

    int32_t fromVariantLen, toVariantLen;
    char fromVariant[ULOC_FULLNAME_CAPACITY], toVariant[ULOC_FULLNAME_CAPACITY];

    UErrorCode errorCode = U_ZERO_ERROR;

    for (fromLocIndex = 0; fromLocIndex < locCount; fromLocIndex++) {
        const char *fromLocale = uloc_getAvailable(fromLocIndex);

        errorCode=U_ZERO_ERROR;
        fromCountryLen = uloc_getCountry(fromLocale, fromCountry, ULOC_FULLNAME_CAPACITY, &errorCode);
        if (fromCountryLen <= 0) {
            /* Ignore countryless locales */
            continue;
        }
        fromVariantLen = uloc_getVariant(fromLocale, fromVariant, ULOC_FULLNAME_CAPACITY, &errorCode);
        if (fromVariantLen > 0) {
            /* Most variants are ignorable like PREEURO, or collation variants. */
            continue;
        }
        /* Start comparing only after the current index.
           Previous loop should have already compared fromLocIndex.
        */
        for (toLocIndex = fromLocIndex + 1; toLocIndex < locCount; toLocIndex++) {
            const char *toLocale = uloc_getAvailable(toLocIndex);

            toCountryLen = uloc_getCountry(toLocale, toCountry, ULOC_FULLNAME_CAPACITY, &errorCode);
            if(U_FAILURE(errorCode)) {
                log_err("Unknown failure fromLocale=%s toLocale=%s errorCode=%s\n",
                    fromLocale, toLocale, u_errorName(errorCode));
                continue;
            }

            if (toCountryLen <= 0) {
                /* Ignore countryless locales */
                continue;
            }
            toVariantLen = uloc_getVariant(toLocale, toVariant, ULOC_FULLNAME_CAPACITY, &errorCode);
            if (toVariantLen > 0) {
                /* Most variants are ignorable like PREEURO, or collation variants. */
                /* They're a variant for a reason. */
                continue;
            }
            if (strcmp(fromCountry, toCountry) == 0) {
                log_verbose("comparing fromLocale=%s toLocale=%s\n",
                    fromLocale, toLocale);
                compareConsistentCountryInfo(fromLocale, toLocale);
            }
        }
    }
}

static int32_t
findStringSetMismatch(const char *currLoc, const UChar *string, int32_t langSize,
                      const UChar *exemplarCharacters, int32_t exemplarLen,
                      UBool ignoreNumbers, UChar* badCharPtr) {
    UErrorCode errorCode = U_ZERO_ERROR;
    USet *origSet = uset_openPatternOptions(exemplarCharacters, exemplarLen, USET_CASE_INSENSITIVE, &errorCode);
    USet *exemplarSet = createFlattenSet(origSet, &errorCode);
    int32_t strIdx;
    uset_close(origSet);
    if (U_FAILURE(errorCode)) {
        log_err("%s: error uset_openPattern returned %s\n", currLoc, u_errorName(errorCode));
        return -1;
    }

    for (strIdx = 0; strIdx < langSize; strIdx++) {
        if (!uset_contains(exemplarSet, string[strIdx])
            && string[strIdx] != 0x0020 && string[strIdx] != 0x00A0 && string[strIdx] != 0x002e && string[strIdx] != 0x002c && string[strIdx] != 0x002d && string[strIdx] != 0x0027 && string[strIdx] != 0x2019 && string[strIdx] != 0x0f0b
            && string[strIdx] != 0x200C && string[strIdx] != 0x200D) {
            if (!ignoreNumbers || (ignoreNumbers && (string[strIdx] < 0x30 || string[strIdx] > 0x39))) {
                uset_close(exemplarSet);
                if (badCharPtr) {
                    *badCharPtr = string[strIdx];
                }
                return strIdx;
            }
        }
    }
    uset_close(exemplarSet);
    if (badCharPtr) {
        *badCharPtr = 0;
    }
    return -1;
}
/* include non-invariant chars */
static int32_t
myUCharsToChars(const UChar* us, char* cs, int32_t len){
    int32_t i=0;
    for(; i< len; i++){
        if(us[i] < 0x7f){
            cs[i] = (char)us[i];
        }else{
            return -1;
        }
    }
    return i;
}
static void
findSetMatch( UScriptCode *scriptCodes, int32_t scriptsLen,
              USet *exemplarSet,
              const char  *locale){
    USet *scripts[10]= {0};
    char pattern[256] = { '[', ':', 0x000 };
    int32_t patternLen;
    UChar uPattern[256] = {0};
    UErrorCode status = U_ZERO_ERROR;
    int32_t i;

    /* create the sets with script codes */
    for(i = 0; i<scriptsLen; i++){
        strcat(pattern, uscript_getShortName(scriptCodes[i]));
        strcat(pattern, ":]");
        patternLen = (int32_t)strlen(pattern);
        u_charsToUChars(pattern, uPattern, patternLen);
        scripts[i] = uset_openPattern(uPattern, patternLen, &status);
        if(U_FAILURE(status)){
            log_err("Could not create set for pattern %s. Error: %s\n", pattern, u_errorName(status));
            return;
        }
        pattern[2] = 0;
    }
    if (strcmp(locale, "uk") == 0 || strcmp(locale, "uk_UA") == 0) {
        /* Special addition. Add the modifying apostrophe, which isn't in Cyrillic. */
        uset_add(scripts[0], 0x2bc);
    }
    if(U_SUCCESS(status)){
        UBool existsInScript = FALSE;
        /* iterate over the exemplarSet and ascertain if all
         * UChars in exemplarSet belong to the scripts returned
         * by getScript
         */
        int32_t count = uset_getItemCount(exemplarSet);

        for( i=0; i < count; i++){
            UChar32 start = 0;
            UChar32 end = 0;
            UChar *str = NULL;
            int32_t strCapacity = 0;

            strCapacity = uset_getItem(exemplarSet, i, &start, &end, str, strCapacity, &status);
            if(U_SUCCESS(status)){
                int32_t j;
                if(strCapacity == 0){
                    /* ok the item is a range */
                     for( j = 0; j < scriptsLen; j++){
                        if(uset_containsRange(scripts[j], start, end) == TRUE){
                            existsInScript = TRUE;
                        }
                    }
                    if(existsInScript == FALSE){
                        for( j = 0; j < scriptsLen; j++){
                            UChar toPattern[500]={'\0'};
                            char pat[500]={'\0'};
                            int32_t len = uset_toPattern(scripts[j], toPattern, 500, TRUE, &status);
                            len = myUCharsToChars(toPattern, pat, len);
                            log_err("uset_indexOf(\\u%04X)=%i uset_indexOf(\\u%04X)=%i\n", start, uset_indexOf(scripts[0], start), end, uset_indexOf(scripts[0], end));
                            if(len!=-1){
                                log_err("Pattern: %s\n",pat);
                            }
                        }
                        log_err("ExemplarCharacters and LocaleScript containment test failed for locale %s. \n", locale);
                    }
                }else{
                    strCapacity++; /* increment for NUL termination */
                    /* allocate the str and call the api again */
                    str = (UChar*) malloc(U_SIZEOF_UCHAR * strCapacity);
                    strCapacity =  uset_getItem(exemplarSet, i, &start, &end, str, strCapacity, &status);
                    /* iterate over the scripts and figure out if the string contained is actually
                     * in the script set
                     */
                    for( j = 0; j < scriptsLen; j++){
                        if(uset_containsString(scripts[j],str, strCapacity) == TRUE){
                            existsInScript = TRUE;
                        }
                    }
                    if(existsInScript == FALSE){
                        log_err("ExemplarCharacters and LocaleScript containment test failed for locale %s. \n", locale);
                    }
                }
            }
        }

    }

    /* close the sets */
    for(i = 0; i<scriptsLen; i++){
        uset_close(scripts[i]);
    }
}

static void VerifyTranslation(void) {
    static const UVersionInfo icu47 = { 4, 7, 0, 0 };
    UResourceBundle *root, *currentLocale;
    int32_t locCount = uloc_countAvailable();
    int32_t locIndex;
    UErrorCode errorCode = U_ZERO_ERROR;
    int32_t exemplarLen;
    const UChar *exemplarCharacters;
    const char *currLoc;
    UScriptCode scripts[USCRIPT_CODE_LIMIT];
    int32_t numScripts;
    int32_t idx;
    int32_t end;
    UResourceBundle *resArray;

    if (locCount <= 1) {
        log_data_err("At least root needs to be installed\n");
    }

    root = ures_openDirect(NULL, "root", &errorCode);
    if(U_FAILURE(errorCode)) {
        log_data_err("Can't open root\n");
        return;
    }
    for (locIndex = 0; locIndex < locCount; locIndex++) {
        errorCode=U_ZERO_ERROR;
        currLoc = uloc_getAvailable(locIndex);
        currentLocale = ures_open(NULL, currLoc, &errorCode);
        if(errorCode != U_ZERO_ERROR) {
            if(U_SUCCESS(errorCode)) {
                /* It's installed, but there is no data.
                   It's installed for the g18n white paper [grhoten] */
                log_err("ERROR: Locale %-5s not installed, and it should be!\n",
                    uloc_getAvailable(locIndex));
            } else {
                log_err("%%%%%%% Unexpected error %d in %s %%%%%%%",
                    u_errorName(errorCode),
                    uloc_getAvailable(locIndex));
            }
            ures_close(currentLocale);
            continue;
        }
        exemplarCharacters = ures_getStringByKey(currentLocale, "ExemplarCharacters", &exemplarLen, &errorCode);
        if (U_FAILURE(errorCode)) {
            log_err("error ures_getStringByKey returned %s\n", u_errorName(errorCode));
        }
        else if (getTestOption(QUICK_OPTION) && exemplarLen > 2048) {
            log_verbose("skipping test for %s\n", currLoc);
        }
        else if (uprv_strncmp(currLoc,"bem",3) == 0) {
            log_verbose("skipping test for %s, some month and country names known to use aux exemplars\n", currLoc);
        }
        else {
            UChar langBuffer[128];
            int32_t langSize;
            int32_t strIdx;
            UChar badChar;
            langSize = uloc_getDisplayLanguage(currLoc, currLoc, langBuffer, sizeof(langBuffer)/sizeof(langBuffer[0]), &errorCode);
            if (U_FAILURE(errorCode)) {
                log_err("error uloc_getDisplayLanguage returned %s\n", u_errorName(errorCode));
            }
            else {
                strIdx = findStringSetMismatch(currLoc, langBuffer, langSize, exemplarCharacters, exemplarLen, FALSE, &badChar);
                if (strIdx >= 0) {
                    log_err("getDisplayLanguage(%s) at index %d returned characters not in the exemplar characters: %04X.\n",
                        currLoc, strIdx, badChar);
                }
            }
            langSize = uloc_getDisplayCountry(currLoc, currLoc, langBuffer, sizeof(langBuffer)/sizeof(langBuffer[0]), &errorCode);
            if (U_FAILURE(errorCode)) {
                log_err("error uloc_getDisplayCountry returned %s\n", u_errorName(errorCode));
            }
            else if (uprv_strstr(currLoc, "ti_") != currLoc || isICUVersionAtLeast(icu47)) { /* TODO: restore DisplayCountry test for ti_* when cldrbug 3058 is fixed) */
              strIdx = findStringSetMismatch(currLoc, langBuffer, langSize, exemplarCharacters, exemplarLen, FALSE, &badChar);
                if (strIdx >= 0) {
                    log_err("getDisplayCountry(%s) at index %d returned characters not in the exemplar characters: %04X.\n",
                        currLoc, strIdx, badChar);
                }
            }
            {
                UResourceBundle* cal = ures_getByKey(currentLocale, "calendar", NULL, &errorCode);
                UResourceBundle* greg = ures_getByKeyWithFallback(cal, "gregorian", NULL, &errorCode);
                UResourceBundle* names = ures_getByKeyWithFallback(greg,  "dayNames", NULL, &errorCode);
                UResourceBundle* format = ures_getByKeyWithFallback(names,  "format", NULL, &errorCode);
                resArray = ures_getByKeyWithFallback(format,  "wide", NULL, &errorCode);

                if (U_FAILURE(errorCode)) {
                    log_err("error ures_getByKey returned %s\n", u_errorName(errorCode));
                }
                if (getTestOption(QUICK_OPTION)) {
                    end = 1;
                }
                else {
                    end = ures_getSize(resArray);
                }


                for (idx = 0; idx < end; idx++) {
                    const UChar *fromBundleStr = ures_getStringByIndex(resArray, idx, &langSize, &errorCode);
                    if (U_FAILURE(errorCode)) {
                        log_err("error ures_getStringByIndex(%d) returned %s\n", idx, u_errorName(errorCode));
                        continue;
                    }
                    strIdx = findStringSetMismatch(currLoc, fromBundleStr, langSize, exemplarCharacters, exemplarLen, TRUE, &badChar);
                    if (strIdx >= 0) {
                        log_err("getDayNames(%s, %d) at index %d returned characters not in the exemplar characters: %04X.\n",
                            currLoc, idx, strIdx, badChar);
                    }
                }
                ures_close(resArray);
                ures_close(format);
                ures_close(names);

                names = ures_getByKeyWithFallback(greg, "monthNames", NULL, &errorCode);
                format = ures_getByKeyWithFallback(names,"format", NULL, &errorCode);
                resArray = ures_getByKeyWithFallback(format, "wide", NULL, &errorCode);
                if (U_FAILURE(errorCode)) {
                    log_err("error ures_getByKey returned %s\n", u_errorName(errorCode));
                }
                if (getTestOption(QUICK_OPTION)) {
                    end = 1;
                }
                else {
                    end = ures_getSize(resArray);
                }

                for (idx = 0; idx < end; idx++) {
                    const UChar *fromBundleStr = ures_getStringByIndex(resArray, idx, &langSize, &errorCode);
                    if (U_FAILURE(errorCode)) {
                        log_err("error ures_getStringByIndex(%d) returned %s\n", idx, u_errorName(errorCode));
                        continue;
                    }
                    strIdx = findStringSetMismatch(currLoc, fromBundleStr, langSize, exemplarCharacters, exemplarLen, TRUE, &badChar);
                    if (strIdx >= 0) {
                        log_err("getMonthNames(%s, %d) at index %d returned characters not in the exemplar characters: %04X.\n",
                            currLoc, idx, strIdx, badChar);
                    }
                }
                ures_close(resArray);
                ures_close(format);
                ures_close(names);
                ures_close(greg);
                ures_close(cal);
            }
            errorCode = U_ZERO_ERROR;
            numScripts = uscript_getCode(currLoc, scripts, sizeof(scripts)/sizeof(scripts[0]), &errorCode);
            if (numScripts == 0) {
                log_err("uscript_getCode(%s) doesn't work.\n", currLoc);
            }else if(scripts[0] == USCRIPT_COMMON){
                log_err("uscript_getCode(%s) returned USCRIPT_COMMON.\n", currLoc);
            }

            /* test that the scripts are a superset of exemplar characters. */
           {
                ULocaleData *uld = ulocdata_open(currLoc,&errorCode);
                USet *exemplarSet =  ulocdata_getExemplarSet(uld, NULL, 0, ULOCDATA_ES_STANDARD, &errorCode);
                /* test if exemplar characters are part of script code */
                findSetMatch(scripts, numScripts, exemplarSet, currLoc);
                uset_close(exemplarSet);
                ulocdata_close(uld);
            }

           /* test that the paperSize API works */
           {
               int32_t height=0, width=0;
               ulocdata_getPaperSize(currLoc, &height, &width, &errorCode);
               if(U_FAILURE(errorCode)){
                   log_err("ulocdata_getPaperSize failed for locale %s with error: %s \n", currLoc, u_errorName(errorCode));
               }
               if(strstr(currLoc, "_US")!=NULL && height != 279 && width != 216 ){
                   log_err("ulocdata_getPaperSize did not return expected data for locale %s \n", currLoc);
               }
           }
            /* test that the MeasurementSystem works API works */
           {
               UMeasurementSystem measurementSystem = ulocdata_getMeasurementSystem(currLoc, &errorCode);
               if(U_FAILURE(errorCode)){
                   log_err("ulocdata_getMeasurementSystem failed for locale %s with error: %s \n", currLoc, u_errorName(errorCode));
               }
               if(strstr(currLoc, "_US")!=NULL || strstr(currLoc, "_MM")!=NULL || strstr(currLoc, "_LR")!=NULL){
                   if(measurementSystem != UMS_US){
                        log_err("ulocdata_getMeasurementSystem did not return expected data for locale %s \n", currLoc);
                   }
               }else if(measurementSystem != UMS_SI){
                   log_err("ulocdata_getMeasurementSystem did not return expected data for locale %s \n", currLoc);
               }
           }
        }
        ures_close(currentLocale);
    }

    ures_close(root);
}

/* adjust this limit as appropriate */
#define MAX_SCRIPTS_PER_LOCALE 8

static void TestExemplarSet(void){
    int32_t i, j, k, m, n;
    int32_t equalCount = 0;
    UErrorCode ec = U_ZERO_ERROR;
    UEnumeration* avail;
    USet* exemplarSets[2];
    USet* unassignedSet;
    UScriptCode code[MAX_SCRIPTS_PER_LOCALE];
    USet* codeSets[MAX_SCRIPTS_PER_LOCALE];
    int32_t codeLen;
    char cbuf[32]; /* 9 should be enough */
    UChar ubuf[64]; /* adjust as needed */
    UBool existsInScript;
    int32_t itemCount;
    int32_t strLen;
    UChar32 start, end;

    unassignedSet = NULL;
    exemplarSets[0] = NULL;
    exemplarSets[1] = NULL;
    for (i=0; i<MAX_SCRIPTS_PER_LOCALE; ++i) {
        codeSets[i] = NULL;
    }

    avail = ures_openAvailableLocales(NULL, &ec);
    if (!assertSuccess("ures_openAvailableLocales", &ec)) goto END;
    n = uenum_count(avail, &ec);
    if (!assertSuccess("uenum_count", &ec)) goto END;

    u_uastrcpy(ubuf, "[:unassigned:]");
    unassignedSet = uset_openPattern(ubuf, -1, &ec);
    if (!assertSuccess("uset_openPattern", &ec)) goto END;

    for(i=0; i<n; i++){
        const char* locale = uenum_next(avail, NULL, &ec);
        if (!assertSuccess("uenum_next", &ec)) goto END;
        log_verbose("%s\n", locale);
        for (k=0; k<2; ++k) {
            uint32_t option = (k==0) ? 0 : USET_CASE_INSENSITIVE;
            ULocaleData *uld = ulocdata_open(locale,&ec);
            USet* exemplarSet = ulocdata_getExemplarSet(uld,NULL, option, ULOCDATA_ES_STANDARD, &ec);
            uset_close(exemplarSets[k]);
            ulocdata_close(uld);
            exemplarSets[k] = exemplarSet;
            if (!assertSuccess("ulocaledata_getExemplarSet", &ec)) goto END;

            if (uset_containsSome(exemplarSet, unassignedSet)) {
                log_err("ExemplarSet contains unassigned characters for locale : %s\n", locale);
            }
            codeLen = uscript_getCode(locale, code, 8, &ec);
            if (!assertSuccess("uscript_getCode", &ec)) goto END;

            for (j=0; j<MAX_SCRIPTS_PER_LOCALE; ++j) {
                uset_close(codeSets[j]);
                codeSets[j] = NULL;
            }
            for (j=0; j<codeLen; ++j) {
                uprv_strcpy(cbuf, "[:");
                if(code[j]==-1){
                    log_err("USCRIPT_INVALID_CODE returned for locale: %s\n", locale);
                    continue;
                }
                uprv_strcat(cbuf, uscript_getShortName(code[j]));
                uprv_strcat(cbuf, ":]");
                u_uastrcpy(ubuf, cbuf);
                codeSets[j] = uset_openPattern(ubuf, -1, &ec);
            }
            if (!assertSuccess("uset_openPattern", &ec)) goto END;

            existsInScript = FALSE;
            itemCount = uset_getItemCount(exemplarSet);
            for (m=0; m<itemCount && !existsInScript; ++m) {
                strLen = uset_getItem(exemplarSet, m, &start, &end, ubuf,
                                      sizeof(ubuf)/sizeof(ubuf[0]), &ec);
                /* failure here might mean str[] needs to be larger */
                if (!assertSuccess("uset_getItem", &ec)) goto END;
                if (strLen == 0) {
                    for (j=0; j<codeLen; ++j) {
                        if (codeSets[j]!=NULL && uset_containsRange(codeSets[j], start, end)) {
                            existsInScript = TRUE;
                            break;
                        }
                    }
                } else {
                    for (j=0; j<codeLen; ++j) {
                        if (codeSets[j]!=NULL && uset_containsString(codeSets[j], ubuf, strLen)) {
                            existsInScript = TRUE;
                            break;
                        }
                    }
                }
            }

            if (existsInScript == FALSE){
                log_err("ExemplarSet containment failed for locale : %s\n", locale);
            }
        }
        assertTrue("case-folded is a superset",
                   uset_containsAll(exemplarSets[1], exemplarSets[0]));
        if (uset_equals(exemplarSets[1], exemplarSets[0])) {
            ++equalCount;
        }
    }
    /* Note: The case-folded set should sometimes be a strict superset
       and sometimes be equal. */
    assertTrue("case-folded is sometimes a strict superset, and sometimes equal",
               equalCount > 0 && equalCount < n);

 END:
    uenum_close(avail);
    uset_close(exemplarSets[0]);
    uset_close(exemplarSets[1]);
    uset_close(unassignedSet);
    for (i=0; i<MAX_SCRIPTS_PER_LOCALE; ++i) {
        uset_close(codeSets[i]);
    }
}

static void TestLocaleDisplayPattern(void){
    UErrorCode status = U_ZERO_ERROR;
    UChar pattern[32] = {0,};
    UChar separator[32] = {0,};
    ULocaleData *uld = ulocdata_open(uloc_getDefault(), &status);

    if(U_FAILURE(status)){
        log_data_err("ulocdata_open error");
        return;
    }
    ulocdata_getLocaleDisplayPattern(uld, pattern, 32, &status);
    if (U_FAILURE(status)){
        log_err("ulocdata_getLocaleDisplayPattern error!");
    }
    status = U_ZERO_ERROR;
    ulocdata_getLocaleSeparator(uld, separator, 32, &status);
    if (U_FAILURE(status)){
        log_err("ulocdata_getLocaleSeparator error!");
    }
    ulocdata_close(uld);
}

static void TestCoverage(void){
    ULocaleDataDelimiterType types[] = {
     ULOCDATA_QUOTATION_START,     /* Quotation start */
     ULOCDATA_QUOTATION_END,       /* Quotation end */
     ULOCDATA_ALT_QUOTATION_START, /* Alternate quotation start */
     ULOCDATA_ALT_QUOTATION_END,   /* Alternate quotation end */
     ULOCDATA_DELIMITER_COUNT
    };
    int i;
    UBool sub;
    UErrorCode status = U_ZERO_ERROR;
    ULocaleData *uld = ulocdata_open(uloc_getDefault(), &status);

    if(U_FAILURE(status)){
        log_data_err("ulocdata_open error");
        return;
    }


    for(i = 0; i < ULOCDATA_DELIMITER_COUNT; i++){
        UChar result[32] = {0,};
        status = U_ZERO_ERROR;
        ulocdata_getDelimiter(uld, types[i], result, 32, &status);
        if (U_FAILURE(status)){
            log_err("ulocdata_getgetDelimiter error with type %d", types[i]);
        }
    }

    sub = ulocdata_getNoSubstitute(uld);
    ulocdata_setNoSubstitute(uld,sub);
    ulocdata_close(uld);
}

static void TestCurrencyList(void){
#if !UCONFIG_NO_FORMATTING
    UErrorCode errorCode = U_ZERO_ERROR;
    int32_t structLocaleCount, currencyCount;
    UEnumeration *en = ucurr_openISOCurrencies(UCURR_ALL, &errorCode);
    const char *isoCode, *structISOCode;
    UResourceBundle *subBundle;
    UResourceBundle *currencies = ures_openDirect(loadTestData(&errorCode), "structLocale", &errorCode);
    if(U_FAILURE(errorCode)) {
        log_data_err("Can't open structLocale\n");
        return;
    }
    currencies = ures_getByKey(currencies, "Currencies", currencies, &errorCode);
    currencyCount = uenum_count(en, &errorCode);
    structLocaleCount = ures_getSize(currencies);
    if (currencyCount != structLocaleCount) {
        log_err("structLocale(%d) and ISO4217(%d) currency list are out of sync.\n", structLocaleCount, currencyCount);
#if U_CHARSET_FAMILY == U_ASCII_FAMILY
        ures_resetIterator(currencies);
        while ((isoCode = uenum_next(en, NULL, &errorCode)) != NULL && ures_hasNext(currencies)) {
            subBundle = ures_getNextResource(currencies, NULL, &errorCode);
            structISOCode = ures_getKey(subBundle);
            ures_close(subBundle);
            if (strcmp(structISOCode, isoCode) != 0) {
                log_err("First difference found at structLocale(%s) and ISO4217(%s).\n", structISOCode, isoCode);
                break;
            }
        }
#endif
    }
    ures_close(currencies);
    uenum_close(en);
#endif
}

#define TESTCASE(name) addTest(root, &name, "tsutil/cldrtest/" #name)

void addCLDRTest(TestNode** root);

void addCLDRTest(TestNode** root)
{
#if !UCONFIG_NO_FILE_IO && !UCONFIG_NO_LEGACY_CONVERSION
    TESTCASE(TestLocaleStructure);
    TESTCASE(TestCurrencyList);
#endif
    TESTCASE(TestConsistentCountryInfo);
    TESTCASE(VerifyTranslation);
    TESTCASE(TestExemplarSet);
    TESTCASE(TestLocaleDisplayPattern);
    TESTCASE(TestCoverage);
}
