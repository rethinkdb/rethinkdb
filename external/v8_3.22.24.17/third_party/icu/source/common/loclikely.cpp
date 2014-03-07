/*
*******************************************************************************
*
*   Copyright (C) 1997-2010, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  loclikely.cpp
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2010feb25
*   created by: Markus W. Scherer
*
*   Code for likely and minimized locale subtags, separated out from other .cpp files
*   that then do not depend on resource bundle code and likely-subtags data.
*/

#include "unicode/utypes.h"
#include "unicode/putil.h"
#include "unicode/uloc.h"
#include "unicode/ures.h"
#include "cmemory.h"
#include "cstring.h"
#include "ulocimp.h"
#include "ustr_imp.h"

/**
 * This function looks for the localeID in the likelySubtags resource.
 *
 * @param localeID The tag to find.
 * @param buffer A buffer to hold the matching entry
 * @param bufferLength The length of the output buffer
 * @return A pointer to "buffer" if found, or a null pointer if not.
 */
static const char*  U_CALLCONV
findLikelySubtags(const char* localeID,
                  char* buffer,
                  int32_t bufferLength,
                  UErrorCode* err) {
    const char* result = NULL;

    if (!U_FAILURE(*err)) {
        int32_t resLen = 0;
        const UChar* s = NULL;
        UErrorCode tmpErr = U_ZERO_ERROR;
        UResourceBundle* subtags = ures_openDirect(NULL, "likelySubtags", &tmpErr);
        if (U_SUCCESS(tmpErr)) {
            s = ures_getStringByKey(subtags, localeID, &resLen, &tmpErr);

            if (U_FAILURE(tmpErr)) {
                /*
                 * If a resource is missing, it's not really an error, it's
                 * just that we don't have any data for that particular locale ID.
                 */
                if (tmpErr != U_MISSING_RESOURCE_ERROR) {
                    *err = tmpErr;
                }
            }
            else if (resLen >= bufferLength) {
                /* The buffer should never overflow. */
                *err = U_INTERNAL_PROGRAM_ERROR;
            }
            else {
                u_UCharsToChars(s, buffer, resLen + 1);
                result = buffer;
            }

            ures_close(subtags);
        } else {
            *err = tmpErr;
        }
    }

    return result;
}

/**
 * Append a tag to a buffer, adding the separator if necessary.  The buffer
 * must be large enough to contain the resulting tag plus any separator
 * necessary. The tag must not be a zero-length string.
 *
 * @param tag The tag to add.
 * @param tagLength The length of the tag.
 * @param buffer The output buffer.
 * @param bufferLength The length of the output buffer.  This is an input/ouput parameter.
 **/
static void U_CALLCONV
appendTag(
    const char* tag,
    int32_t tagLength,
    char* buffer,
    int32_t* bufferLength) {

    if (*bufferLength > 0) {
        buffer[*bufferLength] = '_';
        ++(*bufferLength);
    }

    uprv_memmove(
        &buffer[*bufferLength],
        tag,
        tagLength);

    *bufferLength += tagLength;
}

/**
 * These are the canonical strings for unknown languages, scripts and regions.
 **/
static const char* const unknownLanguage = "und";
static const char* const unknownScript = "Zzzz";
static const char* const unknownRegion = "ZZ";

/**
 * Create a tag string from the supplied parameters.  The lang, script and region
 * parameters may be NULL pointers. If they are, their corresponding length parameters
 * must be less than or equal to 0.
 *
 * If any of the language, script or region parameters are empty, and the alternateTags
 * parameter is not NULL, it will be parsed for potential language, script and region tags
 * to be used when constructing the new tag.  If the alternateTags parameter is NULL, or
 * it contains no language tag, the default tag for the unknown language is used.
 *
 * If the length of the new string exceeds the capacity of the output buffer, 
 * the function copies as many bytes to the output buffer as it can, and returns
 * the error U_BUFFER_OVERFLOW_ERROR.
 *
 * If an illegal argument is provided, the function returns the error
 * U_ILLEGAL_ARGUMENT_ERROR.
 *
 * Note that this function can return the warning U_STRING_NOT_TERMINATED_WARNING if
 * the tag string fits in the output buffer, but the null terminator doesn't.
 *
 * @param lang The language tag to use.
 * @param langLength The length of the language tag.
 * @param script The script tag to use.
 * @param scriptLength The length of the script tag.
 * @param region The region tag to use.
 * @param regionLength The length of the region tag.
 * @param trailing Any trailing data to append to the new tag.
 * @param trailingLength The length of the trailing data.
 * @param alternateTags A string containing any alternate tags.
 * @param tag The output buffer.
 * @param tagCapacity The capacity of the output buffer.
 * @param err A pointer to a UErrorCode for error reporting.
 * @return The length of the tag string, which may be greater than tagCapacity, or -1 on error.
 **/
static int32_t U_CALLCONV
createTagStringWithAlternates(
    const char* lang,
    int32_t langLength,
    const char* script,
    int32_t scriptLength,
    const char* region,
    int32_t regionLength,
    const char* trailing,
    int32_t trailingLength,
    const char* alternateTags,
    char* tag,
    int32_t tagCapacity,
    UErrorCode* err) {

    if (U_FAILURE(*err)) {
        goto error;
    }
    else if (tag == NULL ||
             tagCapacity <= 0 ||
             langLength >= ULOC_LANG_CAPACITY ||
             scriptLength >= ULOC_SCRIPT_CAPACITY ||
             regionLength >= ULOC_COUNTRY_CAPACITY) {
        goto error;
    }
    else {
        /**
         * ULOC_FULLNAME_CAPACITY will provide enough capacity
         * that we can build a string that contains the language,
         * script and region code without worrying about overrunning
         * the user-supplied buffer.
         **/
        char tagBuffer[ULOC_FULLNAME_CAPACITY];
        int32_t tagLength = 0;
        int32_t capacityRemaining = tagCapacity;
        UBool regionAppended = FALSE;

        if (langLength > 0) {
            appendTag(
                lang,
                langLength,
                tagBuffer,
                &tagLength);
        }
        else if (alternateTags == NULL) {
            /*
             * Append the value for an unknown language, if
             * we found no language.
             */
            appendTag(
                unknownLanguage,
                (int32_t)uprv_strlen(unknownLanguage),
                tagBuffer,
                &tagLength);
        }
        else {
            /*
             * Parse the alternateTags string for the language.
             */
            char alternateLang[ULOC_LANG_CAPACITY];
            int32_t alternateLangLength = sizeof(alternateLang);

            alternateLangLength =
                uloc_getLanguage(
                    alternateTags,
                    alternateLang,
                    alternateLangLength,
                    err);
            if(U_FAILURE(*err) ||
                alternateLangLength >= ULOC_LANG_CAPACITY) {
                goto error;
            }
            else if (alternateLangLength == 0) {
                /*
                 * Append the value for an unknown language, if
                 * we found no language.
                 */
                appendTag(
                    unknownLanguage,
                    (int32_t)uprv_strlen(unknownLanguage),
                    tagBuffer,
                    &tagLength);
            }
            else {
                appendTag(
                    alternateLang,
                    alternateLangLength,
                    tagBuffer,
                    &tagLength);
            }
        }

        if (scriptLength > 0) {
            appendTag(
                script,
                scriptLength,
                tagBuffer,
                &tagLength);
        }
        else if (alternateTags != NULL) {
            /*
             * Parse the alternateTags string for the script.
             */
            char alternateScript[ULOC_SCRIPT_CAPACITY];

            const int32_t alternateScriptLength =
                uloc_getScript(
                    alternateTags,
                    alternateScript,
                    sizeof(alternateScript),
                    err);

            if (U_FAILURE(*err) ||
                alternateScriptLength >= ULOC_SCRIPT_CAPACITY) {
                goto error;
            }
            else if (alternateScriptLength > 0) {
                appendTag(
                    alternateScript,
                    alternateScriptLength,
                    tagBuffer,
                    &tagLength);
            }
        }

        if (regionLength > 0) {
            appendTag(
                region,
                regionLength,
                tagBuffer,
                &tagLength);

            regionAppended = TRUE;
        }
        else if (alternateTags != NULL) {
            /*
             * Parse the alternateTags string for the region.
             */
            char alternateRegion[ULOC_COUNTRY_CAPACITY];

            const int32_t alternateRegionLength =
                uloc_getCountry(
                    alternateTags,
                    alternateRegion,
                    sizeof(alternateRegion),
                    err);
            if (U_FAILURE(*err) ||
                alternateRegionLength >= ULOC_COUNTRY_CAPACITY) {
                goto error;
            }
            else if (alternateRegionLength > 0) {
                appendTag(
                    alternateRegion,
                    alternateRegionLength,
                    tagBuffer,
                    &tagLength);

                regionAppended = TRUE;
            }
        }

        {
            const int32_t toCopy =
                tagLength >= tagCapacity ? tagCapacity : tagLength;

            /**
             * Copy the partial tag from our internal buffer to the supplied
             * target.
             **/
            uprv_memcpy(
                tag,
                tagBuffer,
                toCopy);

            capacityRemaining -= toCopy;
        }

        if (trailingLength > 0) {
            if (capacityRemaining > 0 && !regionAppended) {
                tag[tagLength++] = '_';
                --capacityRemaining;
            }

            if (capacityRemaining > 0) {
                /*
                 * Copy the trailing data into the supplied buffer.  Use uprv_memmove, since we
                 * don't know if the user-supplied buffers overlap.
                 */
                const int32_t toCopy =
                    trailingLength >= capacityRemaining ? capacityRemaining : trailingLength;

                uprv_memmove(
                    &tag[tagLength],
                    trailing,
                    toCopy);
            }
        }

        tagLength += trailingLength;

        return u_terminateChars(
                    tag,
                    tagCapacity,
                    tagLength,
                    err);
    }

error:

    /**
     * An overflow indicates the locale ID passed in
     * is ill-formed.  If we got here, and there was
     * no previous error, it's an implicit overflow.
     **/
    if (*err ==  U_BUFFER_OVERFLOW_ERROR ||
        U_SUCCESS(*err)) {
        *err = U_ILLEGAL_ARGUMENT_ERROR;
    }

    return -1;
}

/**
 * Create a tag string from the supplied parameters.  The lang, script and region
 * parameters may be NULL pointers. If they are, their corresponding length parameters
 * must be less than or equal to 0.  If the lang parameter is an empty string, the
 * default value for an unknown language is written to the output buffer.
 *
 * If the length of the new string exceeds the capacity of the output buffer, 
 * the function copies as many bytes to the output buffer as it can, and returns
 * the error U_BUFFER_OVERFLOW_ERROR.
 *
 * If an illegal argument is provided, the function returns the error
 * U_ILLEGAL_ARGUMENT_ERROR.
 *
 * @param lang The language tag to use.
 * @param langLength The length of the language tag.
 * @param script The script tag to use.
 * @param scriptLength The length of the script tag.
 * @param region The region tag to use.
 * @param regionLength The length of the region tag.
 * @param trailing Any trailing data to append to the new tag.
 * @param trailingLength The length of the trailing data.
 * @param tag The output buffer.
 * @param tagCapacity The capacity of the output buffer.
 * @param err A pointer to a UErrorCode for error reporting.
 * @return The length of the tag string, which may be greater than tagCapacity.
 **/
static int32_t U_CALLCONV
createTagString(
    const char* lang,
    int32_t langLength,
    const char* script,
    int32_t scriptLength,
    const char* region,
    int32_t regionLength,
    const char* trailing,
    int32_t trailingLength,
    char* tag,
    int32_t tagCapacity,
    UErrorCode* err)
{
    return createTagStringWithAlternates(
                lang,
                langLength,
                script,
                scriptLength,
                region,
                regionLength,
                trailing,
                trailingLength,
                NULL,
                tag,
                tagCapacity,
                err);
}

/**
 * Parse the language, script, and region subtags from a tag string, and copy the
 * results into the corresponding output parameters. The buffers are null-terminated,
 * unless overflow occurs.
 *
 * The langLength, scriptLength, and regionLength parameters are input/output
 * parameters, and must contain the capacity of their corresponding buffers on
 * input.  On output, they will contain the actual length of the buffers, not
 * including the null terminator.
 *
 * If the length of any of the output subtags exceeds the capacity of the corresponding
 * buffer, the function copies as many bytes to the output buffer as it can, and returns
 * the error U_BUFFER_OVERFLOW_ERROR.  It will not parse any more subtags once overflow
 * occurs.
 *
 * If an illegal argument is provided, the function returns the error
 * U_ILLEGAL_ARGUMENT_ERROR.
 *
 * @param localeID The locale ID to parse.
 * @param lang The language tag buffer.
 * @param langLength The length of the language tag.
 * @param script The script tag buffer.
 * @param scriptLength The length of the script tag.
 * @param region The region tag buffer.
 * @param regionLength The length of the region tag.
 * @param err A pointer to a UErrorCode for error reporting.
 * @return The number of chars of the localeID parameter consumed.
 **/
static int32_t U_CALLCONV
parseTagString(
    const char* localeID,
    char* lang,
    int32_t* langLength,
    char* script,
    int32_t* scriptLength,
    char* region,
    int32_t* regionLength,
    UErrorCode* err)
{
    const char* position = localeID;
    int32_t subtagLength = 0;

    if(U_FAILURE(*err) ||
       localeID == NULL ||
       lang == NULL ||
       langLength == NULL ||
       script == NULL ||
       scriptLength == NULL ||
       region == NULL ||
       regionLength == NULL) {
        goto error;
    }

    subtagLength = ulocimp_getLanguage(position, lang, *langLength, &position);
    u_terminateChars(lang, *langLength, subtagLength, err);

    /*
     * Note that we explicit consider U_STRING_NOT_TERMINATED_WARNING
     * to be an error, because it indicates the user-supplied tag is
     * not well-formed.
     */
    if(U_FAILURE(*err)) {
        goto error;
    }

    *langLength = subtagLength;

    /*
     * If no language was present, use the value of unknownLanguage
     * instead.  Otherwise, move past any separator.
     */
    if (*langLength == 0) {
        uprv_strcpy(
            lang,
            unknownLanguage);
        *langLength = (int32_t)uprv_strlen(lang);
    }
    else if (_isIDSeparator(*position)) {
        ++position;
    }

    subtagLength = ulocimp_getScript(position, script, *scriptLength, &position);
    u_terminateChars(script, *scriptLength, subtagLength, err);

    if(U_FAILURE(*err)) {
        goto error;
    }

    *scriptLength = subtagLength;

    if (*scriptLength > 0) {
        if (uprv_strnicmp(script, unknownScript, *scriptLength) == 0) {
            /**
             * If the script part is the "unknown" script, then don't return it.
             **/
            *scriptLength = 0;
        }

        /*
         * Move past any separator.
         */
        if (_isIDSeparator(*position)) {
            ++position;
        }    
    }

    subtagLength = ulocimp_getCountry(position, region, *regionLength, &position);
    u_terminateChars(region, *regionLength, subtagLength, err);

    if(U_FAILURE(*err)) {
        goto error;
    }

    *regionLength = subtagLength;

    if (*regionLength > 0) {
        if (uprv_strnicmp(region, unknownRegion, *regionLength) == 0) {
            /**
             * If the region part is the "unknown" region, then don't return it.
             **/
            *regionLength = 0;
        }
    }

exit:

    return (int32_t)(position - localeID);

error:

    /**
     * If we get here, we have no explicit error, it's the result of an
     * illegal argument.
     **/
    if (!U_FAILURE(*err)) {
        *err = U_ILLEGAL_ARGUMENT_ERROR;
    }

    goto exit;
}

static int32_t U_CALLCONV
createLikelySubtagsString(
    const char* lang,
    int32_t langLength,
    const char* script,
    int32_t scriptLength,
    const char* region,
    int32_t regionLength,
    const char* variants,
    int32_t variantsLength,
    char* tag,
    int32_t tagCapacity,
    UErrorCode* err)
{
    /**
     * ULOC_FULLNAME_CAPACITY will provide enough capacity
     * that we can build a string that contains the language,
     * script and region code without worrying about overrunning
     * the user-supplied buffer.
     **/
    char tagBuffer[ULOC_FULLNAME_CAPACITY];
    char likelySubtagsBuffer[ULOC_FULLNAME_CAPACITY];
    int32_t tagBufferLength = 0;

    if(U_FAILURE(*err)) {
        goto error;
    }

    /**
     * Try the language with the script and region first.
     **/
    if (scriptLength > 0 && regionLength > 0) {

        const char* likelySubtags = NULL;

        tagBufferLength = createTagString(
            lang,
            langLength,
            script,
            scriptLength,
            region,
            regionLength,
            NULL,
            0,
            tagBuffer,
            sizeof(tagBuffer),
            err);
        if(U_FAILURE(*err)) {
            goto error;
        }

        likelySubtags =
            findLikelySubtags(
                tagBuffer,
                likelySubtagsBuffer,
                sizeof(likelySubtagsBuffer),
                err);
        if(U_FAILURE(*err)) {
            goto error;
        }

        if (likelySubtags != NULL) {
            /* Always use the language tag from the
               maximal string, since it may be more
               specific than the one provided. */
            return createTagStringWithAlternates(
                        NULL,
                        0,
                        NULL,
                        0,
                        NULL,
                        0,
                        variants,
                        variantsLength,
                        likelySubtags,
                        tag,
                        tagCapacity,
                        err);
        }
    }

    /**
     * Try the language with just the script.
     **/
    if (scriptLength > 0) {

        const char* likelySubtags = NULL;

        tagBufferLength = createTagString(
            lang,
            langLength,
            script,
            scriptLength,
            NULL,
            0,
            NULL,
            0,
            tagBuffer,
            sizeof(tagBuffer),
            err);
        if(U_FAILURE(*err)) {
            goto error;
        }

        likelySubtags =
            findLikelySubtags(
                tagBuffer,
                likelySubtagsBuffer,
                sizeof(likelySubtagsBuffer),
                err);
        if(U_FAILURE(*err)) {
            goto error;
        }

        if (likelySubtags != NULL) {
            /* Always use the language tag from the
               maximal string, since it may be more
               specific than the one provided. */
            return createTagStringWithAlternates(
                        NULL,
                        0,
                        NULL,
                        0,
                        region,
                        regionLength,
                        variants,
                        variantsLength,
                        likelySubtags,
                        tag,
                        tagCapacity,
                        err);
        }
    }

    /**
     * Try the language with just the region.
     **/
    if (regionLength > 0) {

        const char* likelySubtags = NULL;

        createTagString(
            lang,
            langLength,
            NULL,
            0,
            region,
            regionLength,
            NULL,
            0,
            tagBuffer,
            sizeof(tagBuffer),
            err);
        if(U_FAILURE(*err)) {
            goto error;
        }

        likelySubtags =
            findLikelySubtags(
                tagBuffer,
                likelySubtagsBuffer,
                sizeof(likelySubtagsBuffer),
                err);
        if(U_FAILURE(*err)) {
            goto error;
        }

        if (likelySubtags != NULL) {
            /* Always use the language tag from the
               maximal string, since it may be more
               specific than the one provided. */
            return createTagStringWithAlternates(
                        NULL,
                        0,
                        script,
                        scriptLength,
                        NULL,
                        0,
                        variants,
                        variantsLength,
                        likelySubtags,
                        tag,
                        tagCapacity,
                        err);
        }
    }

    /**
     * Finally, try just the language.
     **/
    {
        const char* likelySubtags = NULL;

        createTagString(
            lang,
            langLength,
            NULL,
            0,
            NULL,
            0,
            NULL,
            0,
            tagBuffer,
            sizeof(tagBuffer),
            err);
        if(U_FAILURE(*err)) {
            goto error;
        }

        likelySubtags =
            findLikelySubtags(
                tagBuffer,
                likelySubtagsBuffer,
                sizeof(likelySubtagsBuffer),
                err);
        if(U_FAILURE(*err)) {
            goto error;
        }

        if (likelySubtags != NULL) {
            /* Always use the language tag from the
               maximal string, since it may be more
               specific than the one provided. */
            return createTagStringWithAlternates(
                        NULL,
                        0,
                        script,
                        scriptLength,
                        region,
                        regionLength,
                        variants,
                        variantsLength,
                        likelySubtags,
                        tag,
                        tagCapacity,
                        err);
        }
    }

    return u_terminateChars(
                tag,
                tagCapacity,
                0,
                err);

error:

    if (!U_FAILURE(*err)) {
        *err = U_ILLEGAL_ARGUMENT_ERROR;
    }

    return -1;
}

#define CHECK_TRAILING_VARIANT_SIZE(trailing, trailingLength) \
    {   int32_t count = 0; \
        int32_t i; \
        for (i = 0; i < trailingLength; i++) { \
            if (trailing[i] == '-' || trailing[i] == '_') { \
                count = 0; \
                if (count > 8) { \
                    goto error; \
                } \
            } else if (trailing[i] == '@') { \
                break; \
            } else if (count > 8) { \
                goto error; \
            } else { \
                count++; \
            } \
        } \
    }

static int32_t
_uloc_addLikelySubtags(const char*    localeID,
         char* maximizedLocaleID,
         int32_t maximizedLocaleIDCapacity,
         UErrorCode* err)
{
    char lang[ULOC_LANG_CAPACITY];
    int32_t langLength = sizeof(lang);
    char script[ULOC_SCRIPT_CAPACITY];
    int32_t scriptLength = sizeof(script);
    char region[ULOC_COUNTRY_CAPACITY];
    int32_t regionLength = sizeof(region);
    const char* trailing = "";
    int32_t trailingLength = 0;
    int32_t trailingIndex = 0;
    int32_t resultLength = 0;

    if(U_FAILURE(*err)) {
        goto error;
    }
    else if (localeID == NULL ||
             maximizedLocaleID == NULL ||
             maximizedLocaleIDCapacity <= 0) {
        goto error;
    }

    trailingIndex = parseTagString(
        localeID,
        lang,
        &langLength,
        script,
        &scriptLength,
        region,
        &regionLength,
        err);
    if(U_FAILURE(*err)) {
        /* Overflow indicates an illegal argument error */
        if (*err == U_BUFFER_OVERFLOW_ERROR) {
            *err = U_ILLEGAL_ARGUMENT_ERROR;
        }

        goto error;
    }

    /* Find the length of the trailing portion. */
    trailing = &localeID[trailingIndex];
    trailingLength = (int32_t)uprv_strlen(trailing);

    CHECK_TRAILING_VARIANT_SIZE(trailing, trailingLength);

    resultLength =
        createLikelySubtagsString(
            lang,
            langLength,
            script,
            scriptLength,
            region,
            regionLength,
            trailing,
            trailingLength,
            maximizedLocaleID,
            maximizedLocaleIDCapacity,
            err);

    if (resultLength == 0) {
        const int32_t localIDLength = (int32_t)uprv_strlen(localeID);

        /*
         * If we get here, we need to return localeID.
         */
        uprv_memcpy(
            maximizedLocaleID,
            localeID,
            localIDLength <= maximizedLocaleIDCapacity ? 
                localIDLength : maximizedLocaleIDCapacity);

        resultLength =
            u_terminateChars(
                maximizedLocaleID,
                maximizedLocaleIDCapacity,
                localIDLength,
                err);
    }

    return resultLength;

error:

    if (!U_FAILURE(*err)) {
        *err = U_ILLEGAL_ARGUMENT_ERROR;
    }

    return -1;
}

static int32_t
_uloc_minimizeSubtags(const char*    localeID,
         char* minimizedLocaleID,
         int32_t minimizedLocaleIDCapacity,
         UErrorCode* err)
{
    /**
     * ULOC_FULLNAME_CAPACITY will provide enough capacity
     * that we can build a string that contains the language,
     * script and region code without worrying about overrunning
     * the user-supplied buffer.
     **/
    char maximizedTagBuffer[ULOC_FULLNAME_CAPACITY];
    int32_t maximizedTagBufferLength = sizeof(maximizedTagBuffer);

    char lang[ULOC_LANG_CAPACITY];
    int32_t langLength = sizeof(lang);
    char script[ULOC_SCRIPT_CAPACITY];
    int32_t scriptLength = sizeof(script);
    char region[ULOC_COUNTRY_CAPACITY];
    int32_t regionLength = sizeof(region);
    const char* trailing = "";
    int32_t trailingLength = 0;
    int32_t trailingIndex = 0;

    if(U_FAILURE(*err)) {
        goto error;
    }
    else if (localeID == NULL ||
             minimizedLocaleID == NULL ||
             minimizedLocaleIDCapacity <= 0) {
        goto error;
    }

    trailingIndex =
        parseTagString(
            localeID,
            lang,
            &langLength,
            script,
            &scriptLength,
            region,
            &regionLength,
            err);
    if(U_FAILURE(*err)) {

        /* Overflow indicates an illegal argument error */
        if (*err == U_BUFFER_OVERFLOW_ERROR) {
            *err = U_ILLEGAL_ARGUMENT_ERROR;
        }

        goto error;
    }

    /* Find the spot where the variants begin, if any. */
    trailing = &localeID[trailingIndex];
    trailingLength = (int32_t)uprv_strlen(trailing);

    CHECK_TRAILING_VARIANT_SIZE(trailing, trailingLength);

    createTagString(
        lang,
        langLength,
        script,
        scriptLength,
        region,
        regionLength,
        NULL,
        0,
        maximizedTagBuffer,
        maximizedTagBufferLength,
        err);
    if(U_FAILURE(*err)) {
        goto error;
    }

    /**
     * First, we need to first get the maximization
     * from AddLikelySubtags.
     **/
    maximizedTagBufferLength =
        uloc_addLikelySubtags(
            maximizedTagBuffer,
            maximizedTagBuffer,
            maximizedTagBufferLength,
            err);

    if(U_FAILURE(*err)) {
        goto error;
    }

    /**
     * Start first with just the language.
     **/
    {
        char tagBuffer[ULOC_FULLNAME_CAPACITY];

        const int32_t tagBufferLength =
            createLikelySubtagsString(
                lang,
                langLength,
                NULL,
                0,
                NULL,
                0,
                NULL,
                0,
                tagBuffer,
                sizeof(tagBuffer),
                err);

        if(U_FAILURE(*err)) {
            goto error;
        }
        else if (uprv_strnicmp(
                    maximizedTagBuffer,
                    tagBuffer,
                    tagBufferLength) == 0) {

            return createTagString(
                        lang,
                        langLength,
                        NULL,
                        0,
                        NULL,
                        0,
                        trailing,
                        trailingLength,
                        minimizedLocaleID,
                        minimizedLocaleIDCapacity,
                        err);
        }
    }

    /**
     * Next, try the language and region.
     **/
    if (regionLength > 0) {

        char tagBuffer[ULOC_FULLNAME_CAPACITY];

        const int32_t tagBufferLength =
            createLikelySubtagsString(
                lang,
                langLength,
                NULL,
                0,
                region,
                regionLength,
                NULL,
                0,
                tagBuffer,
                sizeof(tagBuffer),
                err);

        if(U_FAILURE(*err)) {
            goto error;
        }
        else if (uprv_strnicmp(
                    maximizedTagBuffer,
                    tagBuffer,
                    tagBufferLength) == 0) {

            return createTagString(
                        lang,
                        langLength,
                        NULL,
                        0,
                        region,
                        regionLength,
                        trailing,
                        trailingLength,
                        minimizedLocaleID,
                        minimizedLocaleIDCapacity,
                        err);
        }
    }

    /**
     * Finally, try the language and script.  This is our last chance,
     * since trying with all three subtags would only yield the
     * maximal version that we already have.
     **/
    if (scriptLength > 0 && regionLength > 0) {
        char tagBuffer[ULOC_FULLNAME_CAPACITY];

        const int32_t tagBufferLength =
            createLikelySubtagsString(
                lang,
                langLength,
                script,
                scriptLength,
                NULL,
                0,
                NULL,
                0,
                tagBuffer,
                sizeof(tagBuffer),
                err);

        if(U_FAILURE(*err)) {
            goto error;
        }
        else if (uprv_strnicmp(
                    maximizedTagBuffer,
                    tagBuffer,
                    tagBufferLength) == 0) {

            return createTagString(
                        lang,
                        langLength,
                        script,
                        scriptLength,
                        NULL,
                        0,
                        trailing,
                        trailingLength,
                        minimizedLocaleID,
                        minimizedLocaleIDCapacity,
                        err);
        }
    }

    {
        /**
         * If we got here, return the locale ID parameter.
         **/
        const int32_t localeIDLength = (int32_t)uprv_strlen(localeID);

        uprv_memcpy(
            minimizedLocaleID,
            localeID,
            localeIDLength <= minimizedLocaleIDCapacity ? 
                localeIDLength : minimizedLocaleIDCapacity);

        return u_terminateChars(
                    minimizedLocaleID,
                    minimizedLocaleIDCapacity,
                    localeIDLength,
                    err);
    }

error:

    if (!U_FAILURE(*err)) {
        *err = U_ILLEGAL_ARGUMENT_ERROR;
    }

    return -1;


}

static UBool
do_canonicalize(const char*    localeID,
         char* buffer,
         int32_t bufferCapacity,
         UErrorCode* err)
{
    uloc_canonicalize(
        localeID,
        buffer,
        bufferCapacity,
        err);

    if (*err == U_STRING_NOT_TERMINATED_WARNING ||
        *err == U_BUFFER_OVERFLOW_ERROR) {
        *err = U_ILLEGAL_ARGUMENT_ERROR;

        return FALSE;
    }
    else if (U_FAILURE(*err)) {

        return FALSE;
    }
    else {
        return TRUE;
    }
}

U_DRAFT int32_t U_EXPORT2
uloc_addLikelySubtags(const char*    localeID,
         char* maximizedLocaleID,
         int32_t maximizedLocaleIDCapacity,
         UErrorCode* err)
{
    char localeBuffer[ULOC_FULLNAME_CAPACITY];

    if (!do_canonicalize(
        localeID,
        localeBuffer,
        sizeof(localeBuffer),
        err)) {
        return -1;
    }
    else {
        return _uloc_addLikelySubtags(
                    localeBuffer,
                    maximizedLocaleID,
                    maximizedLocaleIDCapacity,
                    err);
    }    
}

U_DRAFT int32_t U_EXPORT2
uloc_minimizeSubtags(const char*    localeID,
         char* minimizedLocaleID,
         int32_t minimizedLocaleIDCapacity,
         UErrorCode* err)
{
    char localeBuffer[ULOC_FULLNAME_CAPACITY];

    if (!do_canonicalize(
        localeID,
        localeBuffer,
        sizeof(localeBuffer),
        err)) {
        return -1;
    }
    else {
        return _uloc_minimizeSubtags(
                    localeBuffer,
                    minimizedLocaleID,
                    minimizedLocaleIDCapacity,
                    err);
    }    
}
