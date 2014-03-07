/*
*******************************************************************************
*   Copyright (C) 2004-2010, International Business Machines
*   Corporation and others.  All Rights Reserved.
*******************************************************************************
*   file name:  ucol_sit.cpp
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
* Modification history
* Date        Name      Comments
* 03/12/2004  weiv      Creation
*/

#include "unicode/ustring.h"
#include "unicode/udata.h"

#include "utracimp.h"
#include "ucol_imp.h"
#include "ucol_tok.h"
#include "cmemory.h"
#include "cstring.h"
#include "uresimp.h"

#if !UCONFIG_NO_COLLATION

enum OptionsList {
    UCOL_SIT_LANGUAGE = 0,
    UCOL_SIT_SCRIPT,
    UCOL_SIT_REGION,
    UCOL_SIT_VARIANT,
    UCOL_SIT_KEYWORD,
    UCOL_SIT_BCP47,
    UCOL_SIT_STRENGTH,
    UCOL_SIT_CASE_LEVEL,
    UCOL_SIT_CASE_FIRST,
    UCOL_SIT_NUMERIC_COLLATION,
    UCOL_SIT_ALTERNATE_HANDLING,
    UCOL_SIT_NORMALIZATION_MODE,
    UCOL_SIT_FRENCH_COLLATION,
    UCOL_SIT_HIRAGANA_QUATERNARY,
    UCOL_SIT_VARIABLE_TOP,
    UCOL_SIT_VARIABLE_TOP_VALUE,
    UCOL_SIT_ITEMS_COUNT
};

/* option starters chars. */
static const char alternateHArg     = 'A';
static const char variableTopValArg = 'B';
static const char caseFirstArg      = 'C';
static const char numericCollArg    = 'D';
static const char caseLevelArg      = 'E';
static const char frenchCollArg     = 'F';
static const char hiraganaQArg      = 'H';
static const char keywordArg        = 'K';
static const char languageArg       = 'L';
static const char normArg           = 'N';
static const char regionArg         = 'R';
static const char strengthArg       = 'S';
static const char variableTopArg    = 'T';
static const char variantArg        = 'V';
static const char RFC3066Arg        = 'X';
static const char scriptArg         = 'Z';

static const char collationKeyword[]  = "@collation=";

static const int32_t locElementCount = 5;
static const int32_t locElementCapacity = 32;
static const int32_t loc3066Capacity = 256;
static const int32_t internalBufferSize = 512;

/* structure containing specification of a collator. Initialized
 * from a short string. Also used to construct a short string from a
 * collator instance
 */
struct CollatorSpec {
    char locElements[locElementCount][locElementCapacity];
    char locale[loc3066Capacity];
    UColAttributeValue options[UCOL_ATTRIBUTE_COUNT];
    uint32_t variableTopValue;
    UChar variableTopString[locElementCapacity];
    int32_t variableTopStringLen;
    UBool variableTopSet;
    struct {
        const char *start;
        int32_t len;
    } entries[UCOL_SIT_ITEMS_COUNT];
};


/* structure for converting between character attribute
 * representation and real collation attribute value.
 */
struct AttributeConversion {
    char letter;
    UColAttributeValue value;
};

static const AttributeConversion conversions[12] = {
    { '1', UCOL_PRIMARY },
    { '2', UCOL_SECONDARY },
    { '3', UCOL_TERTIARY },
    { '4', UCOL_QUATERNARY },
    { 'D', UCOL_DEFAULT },
    { 'I', UCOL_IDENTICAL },
    { 'L', UCOL_LOWER_FIRST },
    { 'N', UCOL_NON_IGNORABLE },
    { 'O', UCOL_ON },
    { 'S', UCOL_SHIFTED },
    { 'U', UCOL_UPPER_FIRST },
    { 'X', UCOL_OFF }
};


static char
ucol_sit_attributeValueToLetter(UColAttributeValue value, UErrorCode *status) {
    uint32_t i = 0;
    for(i = 0; i < sizeof(conversions)/sizeof(conversions[0]); i++) {
        if(conversions[i].value == value) {
            return conversions[i].letter;
        }
    }
    *status = U_ILLEGAL_ARGUMENT_ERROR;
    return 0;
}

static UColAttributeValue
ucol_sit_letterToAttributeValue(char letter, UErrorCode *status) {
    uint32_t i = 0;
    for(i = 0; i < sizeof(conversions)/sizeof(conversions[0]); i++) {
        if(conversions[i].letter == letter) {
            return conversions[i].value;
        }
    }
    *status = U_ILLEGAL_ARGUMENT_ERROR;
    return UCOL_DEFAULT;
}

/* function prototype for functions used to parse a short string */
U_CDECL_BEGIN
typedef const char* U_CALLCONV
ActionFunction(CollatorSpec *spec, uint32_t value1, const char* string,
               UErrorCode *status);
U_CDECL_END

U_CDECL_BEGIN
static const char* U_CALLCONV
_processLocaleElement(CollatorSpec *spec, uint32_t value, const char* string,
                      UErrorCode *status)
{
    int32_t len = 0;
    do {
        if(value == 0 || value == 4) {
            spec->locElements[value][len++] = uprv_tolower(*string);
        } else {
            spec->locElements[value][len++] = *string;
        }
    } while(*(++string) != '_' && *string && len < locElementCapacity);
    if(len >= locElementCapacity) {
        *status = U_BUFFER_OVERFLOW_ERROR;
        return string;
    }
    // don't skip the underscore at the end
    return string;
}
U_CDECL_END

U_CDECL_BEGIN
static const char* U_CALLCONV
_processRFC3066Locale(CollatorSpec *spec, uint32_t, const char* string,
                      UErrorCode *status)
{
    char terminator = *string;
    string++;
    const char *end = uprv_strchr(string+1, terminator);
    if(end == NULL || end - string >= loc3066Capacity) {
        *status = U_BUFFER_OVERFLOW_ERROR;
        return string;
    } else {
        uprv_strncpy(spec->locale, string, end-string);
        return end+1;
    }
}

U_CDECL_END

U_CDECL_BEGIN
static const char* U_CALLCONV
_processCollatorOption(CollatorSpec *spec, uint32_t option, const char* string,
                       UErrorCode *status)
{
    spec->options[option] = ucol_sit_letterToAttributeValue(*string, status);
    if((*(++string) != '_' && *string) || U_FAILURE(*status)) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
    }
    return string;
}
U_CDECL_END


static UChar
readHexCodeUnit(const char **string, UErrorCode *status)
{
    UChar result = 0;
    int32_t value = 0;
    char c;
    int32_t noDigits = 0;
    while((c = **string) != 0 && noDigits < 4) {
        if( c >= '0' && c <= '9') {
            value = c - '0';
        } else if ( c >= 'a' && c <= 'f') {
            value = c - 'a' + 10;
        } else if ( c >= 'A' && c <= 'F') {
            value = c - 'A' + 10;
        } else {
            *status = U_ILLEGAL_ARGUMENT_ERROR;
            return 0;
        }
        result = (result << 4) | (UChar)value;
        noDigits++;
        (*string)++;
    }
    // if the string was terminated before we read 4 digits, set an error
    if(noDigits < 4) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
    }
    return result;
}

U_CDECL_BEGIN
static const char* U_CALLCONV
_processVariableTop(CollatorSpec *spec, uint32_t value1, const char* string, UErrorCode *status)
{
    // get four digits
    int32_t i = 0;
    if(!value1) {
        while(U_SUCCESS(*status) && i < locElementCapacity && *string != 0 && *string != '_') {
            spec->variableTopString[i++] = readHexCodeUnit(&string, status);
        }
        spec->variableTopStringLen = i;
        if(i == locElementCapacity && *string != 0 && *string != '_') {
            *status = U_BUFFER_OVERFLOW_ERROR;
        }
    } else {
        spec->variableTopValue = readHexCodeUnit(&string, status);
    }
    if(U_SUCCESS(*status)) {
        spec->variableTopSet = TRUE;
    }
    return string;
}
U_CDECL_END


/* Table for parsing short strings */
struct ShortStringOptions {
    char optionStart;
    ActionFunction *action;
    uint32_t attr;
};

static const ShortStringOptions options[UCOL_SIT_ITEMS_COUNT] =
{
/* 10 ALTERNATE_HANDLING */   {alternateHArg,     _processCollatorOption, UCOL_ALTERNATE_HANDLING }, // alternate  N, S, D
/* 15 VARIABLE_TOP_VALUE */   {variableTopValArg, _processVariableTop,    1 },
/* 08 CASE_FIRST */           {caseFirstArg,      _processCollatorOption, UCOL_CASE_FIRST }, // case first L, U, X, D
/* 09 NUMERIC_COLLATION */    {numericCollArg,    _processCollatorOption, UCOL_NUMERIC_COLLATION }, // codan      O, X, D
/* 07 CASE_LEVEL */           {caseLevelArg,      _processCollatorOption, UCOL_CASE_LEVEL }, // case level O, X, D
/* 12 FRENCH_COLLATION */     {frenchCollArg,     _processCollatorOption, UCOL_FRENCH_COLLATION }, // french     O, X, D
/* 13 HIRAGANA_QUATERNARY] */ {hiraganaQArg,      _processCollatorOption, UCOL_HIRAGANA_QUATERNARY_MODE }, // hiragana   O, X, D
/* 04 KEYWORD */              {keywordArg,        _processLocaleElement,  4 }, // keyword
/* 00 LANGUAGE */             {languageArg,       _processLocaleElement,  0 }, // language
/* 11 NORMALIZATION_MODE */   {normArg,           _processCollatorOption, UCOL_NORMALIZATION_MODE }, // norm       O, X, D
/* 02 REGION */               {regionArg,         _processLocaleElement,  2 }, // region
/* 06 STRENGTH */             {strengthArg,       _processCollatorOption, UCOL_STRENGTH }, // strength   1, 2, 3, 4, I, D
/* 14 VARIABLE_TOP */         {variableTopArg,    _processVariableTop,    0 },
/* 03 VARIANT */              {variantArg,        _processLocaleElement,  3 }, // variant
/* 05 RFC3066BIS */           {RFC3066Arg,        _processRFC3066Locale,  0 }, // rfc3066bis locale name
/* 01 SCRIPT */               {scriptArg,         _processLocaleElement,  1 }  // script
};


static
const char* ucol_sit_readOption(const char *start, CollatorSpec *spec,
                            UErrorCode *status)
{
  int32_t i = 0;

  for(i = 0; i < UCOL_SIT_ITEMS_COUNT; i++) {
      if(*start == options[i].optionStart) {
          spec->entries[i].start = start;
          const char* end = options[i].action(spec, options[i].attr, start+1, status);
          spec->entries[i].len = (int32_t)(end - start);
          return end;
      }
  }
  *status = U_ILLEGAL_ARGUMENT_ERROR;
  return start;
}

static
void ucol_sit_initCollatorSpecs(CollatorSpec *spec)
{
    // reset everything
    uprv_memset(spec, 0, sizeof(CollatorSpec));
    // set collation options to default
    int32_t i = 0;
    for(i = 0; i < UCOL_ATTRIBUTE_COUNT; i++) {
        spec->options[i] = UCOL_DEFAULT;
    }
}

static const char*
ucol_sit_readSpecs(CollatorSpec *s, const char *string,
                        UParseError *parseError, UErrorCode *status)
{
    const char *definition = string;
    while(U_SUCCESS(*status) && *string) {
        string = ucol_sit_readOption(string, s, status);
        // advance over '_'
        while(*string && *string == '_') {
            string++;
        }
    }
    if(U_FAILURE(*status)) {
        parseError->offset = (int32_t)(string - definition);
    }
    return string;
}

static
int32_t ucol_sit_dumpSpecs(CollatorSpec *s, char *destination, int32_t capacity, UErrorCode *status)
{
    int32_t i = 0, j = 0;
    int32_t len = 0;
    char optName;
    if(U_SUCCESS(*status)) {
        for(i = 0; i < UCOL_SIT_ITEMS_COUNT; i++) {
            if(s->entries[i].start) {
                if(len) {
                    if(len < capacity) {
                        uprv_strcat(destination, "_");
                    }
                    len++;
                }
                optName = *(s->entries[i].start);
                if(optName == languageArg || optName == regionArg || optName == variantArg || optName == keywordArg) {
                    for(j = 0; j < s->entries[i].len; j++) {
                        if(len + j < capacity) {
                            destination[len+j] = uprv_toupper(*(s->entries[i].start+j));
                        }
                    }
                    len += s->entries[i].len;
                } else {
                    len += s->entries[i].len;
                    if(len < capacity) {
                        uprv_strncat(destination,s->entries[i].start, s->entries[i].len);
                    }
                }
            }
        }
        return len;
    } else {
        return 0;
    }
}

static void
ucol_sit_calculateWholeLocale(CollatorSpec *s) {
    // put the locale together, unless we have a done
    // locale
    if(s->locale[0] == 0) {
        // first the language
        uprv_strcat(s->locale, s->locElements[0]);
        // then the script, if present
        if(*(s->locElements[1])) {
            uprv_strcat(s->locale, "_");
            uprv_strcat(s->locale, s->locElements[1]);
        }
        // then the region, if present
        if(*(s->locElements[2])) {
            uprv_strcat(s->locale, "_");
            uprv_strcat(s->locale, s->locElements[2]);
        } else if(*(s->locElements[3])) { // if there is a variant, we need an underscore
            uprv_strcat(s->locale, "_");
        }
        // add variant, if there
        if(*(s->locElements[3])) {
            uprv_strcat(s->locale, "_");
            uprv_strcat(s->locale, s->locElements[3]);
        }

        // if there is a collation keyword, add that too
        if(*(s->locElements[4])) {
            uprv_strcat(s->locale, collationKeyword);
            uprv_strcat(s->locale, s->locElements[4]);
        }
    }
}


U_CAPI void U_EXPORT2
ucol_prepareShortStringOpen( const char *definition,
                          UBool,
                          UParseError *parseError,
                          UErrorCode *status)
{
    if(U_FAILURE(*status)) return;

    UParseError internalParseError;

    if(!parseError) {
        parseError = &internalParseError;
    }
    parseError->line = 0;
    parseError->offset = 0;
    parseError->preContext[0] = 0;
    parseError->postContext[0] = 0;


    // first we want to pick stuff out of short string.
    // we'll end up with an UCA version, locale and a bunch of
    // settings

    // analyse the string in order to get everything we need.
    CollatorSpec s;
    ucol_sit_initCollatorSpecs(&s);
    ucol_sit_readSpecs(&s, definition, parseError, status);
    ucol_sit_calculateWholeLocale(&s);

    char buffer[internalBufferSize];
    uprv_memset(buffer, 0, internalBufferSize);
    uloc_canonicalize(s.locale, buffer, internalBufferSize, status);

    UResourceBundle *b = ures_open(U_ICUDATA_COLL, buffer, status);
    /* we try to find stuff from keyword */
    UResourceBundle *collations = ures_getByKey(b, "collations", NULL, status);
    UResourceBundle *collElem = NULL;
    char keyBuffer[256];
    // if there is a keyword, we pick it up and try to get elements
    if(!uloc_getKeywordValue(buffer, "collation", keyBuffer, 256, status)) {
      // no keyword. we try to find the default setting, which will give us the keyword value
      UResourceBundle *defaultColl = ures_getByKeyWithFallback(collations, "default", NULL, status);
      if(U_SUCCESS(*status)) {
        int32_t defaultKeyLen = 0;
        const UChar *defaultKey = ures_getString(defaultColl, &defaultKeyLen, status);
        u_UCharsToChars(defaultKey, keyBuffer, defaultKeyLen);
        keyBuffer[defaultKeyLen] = 0;
      } else {
        *status = U_INTERNAL_PROGRAM_ERROR;
        return;
      }
      ures_close(defaultColl);
    }
    collElem = ures_getByKeyWithFallback(collations, keyBuffer, collElem, status);
    ures_close(collElem);
    ures_close(collations);
    ures_close(b);
}


U_CAPI UCollator* U_EXPORT2
ucol_openFromShortString( const char *definition,
                          UBool forceDefaults,
                          UParseError *parseError,
                          UErrorCode *status)
{
    UTRACE_ENTRY_OC(UTRACE_UCOL_OPEN_FROM_SHORT_STRING);
    UTRACE_DATA1(UTRACE_INFO, "short string = \"%s\"", definition);

    if(U_FAILURE(*status)) return 0;

    UParseError internalParseError;

    if(!parseError) {
        parseError = &internalParseError;
    }
    parseError->line = 0;
    parseError->offset = 0;
    parseError->preContext[0] = 0;
    parseError->postContext[0] = 0;


    // first we want to pick stuff out of short string.
    // we'll end up with an UCA version, locale and a bunch of
    // settings

    // analyse the string in order to get everything we need.
    const char *string = definition;
    CollatorSpec s;
    ucol_sit_initCollatorSpecs(&s);
    string = ucol_sit_readSpecs(&s, definition, parseError, status);
    ucol_sit_calculateWholeLocale(&s);

    char buffer[internalBufferSize];
    uprv_memset(buffer, 0, internalBufferSize);
    uloc_canonicalize(s.locale, buffer, internalBufferSize, status);

    UCollator *result = ucol_open(buffer, status);
    int32_t i = 0;

    for(i = 0; i < UCOL_ATTRIBUTE_COUNT; i++) {
        if(s.options[i] != UCOL_DEFAULT) {
            if(forceDefaults || ucol_getAttribute(result, (UColAttribute)i, status) != s.options[i]) {
                ucol_setAttribute(result, (UColAttribute)i, s.options[i], status);
            }

            if(U_FAILURE(*status)) {
                parseError->offset = (int32_t)(string - definition);
                ucol_close(result);
                return NULL;
            }

        }
    }
    if(s.variableTopSet) {
        if(s.variableTopString[0]) {
            ucol_setVariableTop(result, s.variableTopString, s.variableTopStringLen, status);
        } else { // we set by value, using 'B'
            ucol_restoreVariableTop(result, s.variableTopValue, status);
        }
    }


    if(U_FAILURE(*status)) { // here it can only be a bogus value
        ucol_close(result);
        result = NULL;
    }

    UTRACE_EXIT_PTR_STATUS(result, *status);
    return result;
}


static void appendShortStringElement(const char *src, int32_t len, char *result, int32_t *resultSize, int32_t capacity, char arg)
{
    if(len) {
        if(*resultSize) {
            if(*resultSize < capacity) {
                uprv_strcat(result, "_");
            }
            (*resultSize)++;
        }
        *resultSize += len + 1;
        if(*resultSize < capacity) {
            uprv_strncat(result, &arg, 1);
            uprv_strncat(result, src, len);
        }
    }
}

U_CAPI int32_t U_EXPORT2
ucol_getShortDefinitionString(const UCollator *coll,
                              const char *locale,
                              char *dst,
                              int32_t capacity,
                              UErrorCode *status)
{
    if(U_FAILURE(*status)) return 0;
    char buffer[internalBufferSize];
    uprv_memset(buffer, 0, internalBufferSize*sizeof(char));
    int32_t resultSize = 0;
    char tempbuff[internalBufferSize];
    char locBuff[internalBufferSize];
    uprv_memset(buffer, 0, internalBufferSize*sizeof(char));
    int32_t elementSize = 0;
    UBool isAvailable = 0;
    CollatorSpec s;
    ucol_sit_initCollatorSpecs(&s);

    if(!locale) {
        locale = ucol_getLocaleByType(coll, ULOC_VALID_LOCALE, status);
    }
    elementSize = ucol_getFunctionalEquivalent(locBuff, internalBufferSize, "collation", locale, &isAvailable, status);

    if(elementSize) {
        // we should probably canonicalize here...
        elementSize = uloc_getLanguage(locBuff, tempbuff, internalBufferSize, status);
        appendShortStringElement(tempbuff, elementSize, buffer, &resultSize, /*capacity*/internalBufferSize, languageArg);
        elementSize = uloc_getCountry(locBuff, tempbuff, internalBufferSize, status);
        appendShortStringElement(tempbuff, elementSize, buffer, &resultSize, /*capacity*/internalBufferSize, regionArg);
        elementSize = uloc_getScript(locBuff, tempbuff, internalBufferSize, status);
        appendShortStringElement(tempbuff, elementSize, buffer, &resultSize, /*capacity*/internalBufferSize, scriptArg);
        elementSize = uloc_getVariant(locBuff, tempbuff, internalBufferSize, status);
        appendShortStringElement(tempbuff, elementSize, buffer, &resultSize, /*capacity*/internalBufferSize, variantArg);
        elementSize = uloc_getKeywordValue(locBuff, "collation", tempbuff, internalBufferSize, status);
        appendShortStringElement(tempbuff, elementSize, buffer, &resultSize, /*capacity*/internalBufferSize, keywordArg);
    }

    int32_t i = 0;
    UColAttributeValue attribute = UCOL_DEFAULT;
    for(i = 0; i < UCOL_SIT_ITEMS_COUNT; i++) {
        if(options[i].action == _processCollatorOption) {
            attribute = ucol_getAttributeOrDefault(coll, (UColAttribute)options[i].attr, status);
            if(attribute != UCOL_DEFAULT) {
                char letter = ucol_sit_attributeValueToLetter(attribute, status);
                appendShortStringElement(&letter, 1,
                    buffer, &resultSize, /*capacity*/internalBufferSize, options[i].optionStart);
            }
        }
    }
    if(coll->variableTopValueisDefault == FALSE) {
        //s.variableTopValue = ucol_getVariableTop(coll, status);
        elementSize = T_CString_integerToString(tempbuff, coll->variableTopValue, 16);
        appendShortStringElement(tempbuff, elementSize, buffer, &resultSize, capacity, variableTopValArg);
    }

    UParseError parseError;
    return ucol_normalizeShortDefinitionString(buffer, dst, capacity, &parseError, status);
}

U_CAPI int32_t U_EXPORT2
ucol_normalizeShortDefinitionString(const char *definition,
                                    char *destination,
                                    int32_t capacity,
                                    UParseError *parseError,
                                    UErrorCode *status)
{

    if(U_FAILURE(*status)) {
        return 0;
    }

    if(destination) {
        uprv_memset(destination, 0, capacity*sizeof(char));
    }

    UParseError pe;
    if(!parseError) {
        parseError = &pe;
    }

    // validate
    CollatorSpec s;
    ucol_sit_initCollatorSpecs(&s);
    ucol_sit_readSpecs(&s, definition, parseError, status);
    return ucol_sit_dumpSpecs(&s, destination, capacity, status);
}

U_CAPI UColAttributeValue  U_EXPORT2
ucol_getAttributeOrDefault(const UCollator *coll, UColAttribute attr, UErrorCode *status)
{
    if(U_FAILURE(*status) || coll == NULL) {
      return UCOL_DEFAULT;
    }
    switch(attr) {
    case UCOL_NUMERIC_COLLATION:
        return coll->numericCollationisDefault?UCOL_DEFAULT:coll->numericCollation;
    case UCOL_HIRAGANA_QUATERNARY_MODE:
        return coll->hiraganaQisDefault?UCOL_DEFAULT:coll->hiraganaQ;
    case UCOL_FRENCH_COLLATION: /* attribute for direction of secondary weights*/
        return coll->frenchCollationisDefault?UCOL_DEFAULT:coll->frenchCollation;
    case UCOL_ALTERNATE_HANDLING: /* attribute for handling variable elements*/
        return coll->alternateHandlingisDefault?UCOL_DEFAULT:coll->alternateHandling;
    case UCOL_CASE_FIRST: /* who goes first, lower case or uppercase */
        return coll->caseFirstisDefault?UCOL_DEFAULT:coll->caseFirst;
    case UCOL_CASE_LEVEL: /* do we have an extra case level */
        return coll->caseLevelisDefault?UCOL_DEFAULT:coll->caseLevel;
    case UCOL_NORMALIZATION_MODE: /* attribute for normalization */
        return coll->normalizationModeisDefault?UCOL_DEFAULT:coll->normalizationMode;
    case UCOL_STRENGTH:         /* attribute for strength */
        return coll->strengthisDefault?UCOL_DEFAULT:coll->strength;
    case UCOL_ATTRIBUTE_COUNT:
    default:
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        break;
    }
    return UCOL_DEFAULT;
}


struct contContext {
    const UCollator *coll;
    USet            *conts;
    USet            *expansions;
    USet            *removedContractions;
    UBool           addPrefixes;
    UErrorCode      *status;
};



static void
addSpecial(contContext *context, UChar *buffer, int32_t bufLen,
               uint32_t CE, int32_t leftIndex, int32_t rightIndex, UErrorCode *status)
{
  const UCollator *coll = context->coll;
  USet *contractions = context->conts;
  USet *expansions = context->expansions;
  UBool addPrefixes = context->addPrefixes;

    const UChar *UCharOffset = (UChar *)coll->image+getContractOffset(CE);
    uint32_t newCE = *(coll->contractionCEs + (UCharOffset - coll->contractionIndex));
    // we might have a contraction that ends from previous level
    if(newCE != UCOL_NOT_FOUND) {
      if(isSpecial(CE) && getCETag(CE) == CONTRACTION_TAG && isSpecial(newCE) && getCETag(newCE) == SPEC_PROC_TAG && addPrefixes) {
        addSpecial(context, buffer, bufLen, newCE, leftIndex, rightIndex, status);
      }
      if(contractions && rightIndex-leftIndex > 1) {
            uset_addString(contractions, buffer+leftIndex, rightIndex-leftIndex);
            if(expansions && isSpecial(CE) && getCETag(CE) == EXPANSION_TAG) {
              uset_addString(expansions, buffer+leftIndex, rightIndex-leftIndex);
            }
      }
    }

    UCharOffset++;
    // check whether we're doing contraction or prefix
    if(getCETag(CE) == SPEC_PROC_TAG && addPrefixes) {
      if(leftIndex == 0) {
          *status = U_INTERNAL_PROGRAM_ERROR;
          return;
      }
      --leftIndex;
      while(*UCharOffset != 0xFFFF) {
          newCE = *(coll->contractionCEs + (UCharOffset - coll->contractionIndex));
          buffer[leftIndex] = *UCharOffset;
          if(isSpecial(newCE) && (getCETag(newCE) == CONTRACTION_TAG || getCETag(newCE) == SPEC_PROC_TAG)) {
              addSpecial(context, buffer, bufLen, newCE, leftIndex, rightIndex, status);
          } else {
            if(contractions) {
                uset_addString(contractions, buffer+leftIndex, rightIndex-leftIndex);
            }
            if(expansions && isSpecial(newCE) && getCETag(newCE) == EXPANSION_TAG) {
              uset_addString(expansions, buffer+leftIndex, rightIndex-leftIndex);
            }
          }
          UCharOffset++;
      }
    } else if(getCETag(CE) == CONTRACTION_TAG) {
      if(rightIndex == bufLen-1) {
          *status = U_INTERNAL_PROGRAM_ERROR;
          return;
      }
      while(*UCharOffset != 0xFFFF) {
          newCE = *(coll->contractionCEs + (UCharOffset - coll->contractionIndex));
          buffer[rightIndex] = *UCharOffset;
          if(isSpecial(newCE) && (getCETag(newCE) == CONTRACTION_TAG || getCETag(newCE) == SPEC_PROC_TAG)) {
              addSpecial(context, buffer, bufLen, newCE, leftIndex, rightIndex+1, status);
          } else {
            if(contractions) {
              uset_addString(contractions, buffer+leftIndex, rightIndex+1-leftIndex);
            }
            if(expansions && isSpecial(newCE) && getCETag(newCE) == EXPANSION_TAG) {
              uset_addString(expansions, buffer+leftIndex, rightIndex+1-leftIndex);
            }
          }
          UCharOffset++;
      }
    }

}

U_CDECL_BEGIN
static UBool U_CALLCONV
_processSpecials(const void *context, UChar32 start, UChar32 limit, uint32_t CE)
{
    UErrorCode *status = ((contContext *)context)->status;
    USet *expansions = ((contContext *)context)->expansions;
    USet *removed = ((contContext *)context)->removedContractions;
    UBool addPrefixes = ((contContext *)context)->addPrefixes;
    UChar contraction[internalBufferSize];
    if(isSpecial(CE)) {
      if(((getCETag(CE) == SPEC_PROC_TAG && addPrefixes) || getCETag(CE) == CONTRACTION_TAG)) {
        while(start < limit && U_SUCCESS(*status)) {
            // if there are suppressed contractions, we don't
            // want to add them.
            if(removed && uset_contains(removed, start)) {
                start++;
                continue;
            }
            // we start our contraction from middle, since we don't know if it
            // will grow toward right or left
            contraction[internalBufferSize/2] = (UChar)start;
            addSpecial(((contContext *)context), contraction, internalBufferSize, CE, internalBufferSize/2, internalBufferSize/2+1, status);
            start++;
        }
      } else if(expansions && getCETag(CE) == EXPANSION_TAG) {
        while(start < limit && U_SUCCESS(*status)) {
          uset_add(expansions, start++);
        }
      }
    }
    if(U_FAILURE(*status)) {
        return FALSE;
    } else {
        return TRUE;
    }
}

U_CDECL_END



/**
 * Get a set containing the contractions defined by the collator. The set includes
 * both the UCA contractions and the contractions defined by the collator
 * @param coll collator
 * @param conts the set to hold the result
 * @param status to hold the error code
 * @return the size of the contraction set
 */
U_CAPI int32_t U_EXPORT2
ucol_getContractions( const UCollator *coll,
                  USet *contractions,
                  UErrorCode *status)
{
  ucol_getContractionsAndExpansions(coll, contractions, NULL, FALSE, status);
  return uset_getItemCount(contractions);
}

/**
 * Get a set containing the expansions defined by the collator. The set includes
 * both the UCA expansions and the expansions defined by the tailoring
 * @param coll collator
 * @param conts the set to hold the result
 * @param addPrefixes add the prefix contextual elements to contractions
 * @param status to hold the error code
 *
 * @draft ICU 3.4
 */
U_CAPI void U_EXPORT2
ucol_getContractionsAndExpansions( const UCollator *coll,
                  USet *contractions,
                  USet *expansions,
                  UBool addPrefixes,
                  UErrorCode *status)
{
    if(U_FAILURE(*status)) {
        return;
    }
    if(coll == NULL) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }

    if(contractions) {
      uset_clear(contractions);
    }
    if(expansions) {
      uset_clear(expansions);
    }
    int32_t rulesLen = 0;
    const UChar* rules = ucol_getRules(coll, &rulesLen);
    UColTokenParser src;
    ucol_tok_initTokenList(&src, rules, rulesLen, coll->UCA,
                           ucol_tok_getRulesFromBundle, NULL, status);

    contContext c = { NULL, contractions, expansions, src.removeSet, addPrefixes, status };

    // Add the UCA contractions
    c.coll = coll->UCA;
    utrie_enum(&coll->UCA->mapping, NULL, _processSpecials, &c);

    // This is collator specific. Add contractions from a collator
    c.coll = coll;
    c.removedContractions =  NULL;
    utrie_enum(&coll->mapping, NULL, _processSpecials, &c);
    ucol_tok_closeTokenList(&src);
}

U_CAPI int32_t U_EXPORT2
ucol_getUnsafeSet( const UCollator *coll,
                  USet *unsafe,
                  UErrorCode *status)
{
    UChar buffer[internalBufferSize];
    int32_t len = 0;

    uset_clear(unsafe);

    // cccpattern = "[[:^tccc=0:][:^lccc=0:]]", unfortunately variant
    static const UChar cccpattern[25] = { 0x5b, 0x5b, 0x3a, 0x5e, 0x74, 0x63, 0x63, 0x63, 0x3d, 0x30, 0x3a, 0x5d,
                                    0x5b, 0x3a, 0x5e, 0x6c, 0x63, 0x63, 0x63, 0x3d, 0x30, 0x3a, 0x5d, 0x5d, 0x00 };

    // add chars that fail the fcd check
    uset_applyPattern(unsafe, cccpattern, 24, USET_IGNORE_SPACE, status);

    // add Thai/Lao prevowels
    uset_addRange(unsafe, 0xe40, 0xe44);
    uset_addRange(unsafe, 0xec0, 0xec4);
    // add lead/trail surrogates
    uset_addRange(unsafe, 0xd800, 0xdfff);

    USet *contractions = uset_open(0,0);

    int32_t i = 0, j = 0;
    int32_t contsSize = ucol_getContractions(coll, contractions, status);
    UChar32 c = 0;
    // Contraction set consists only of strings
    // to get unsafe code points, we need to
    // break the strings apart and add them to the unsafe set
    for(i = 0; i < contsSize; i++) {
        len = uset_getItem(contractions, i, NULL, NULL, buffer, internalBufferSize, status);
        if(len > 0) {
            j = 0;
            while(j < len) {
                U16_NEXT(buffer, j, len, c);
                if(j < len) {
                    uset_add(unsafe, c);
                }
            }
        }
    }

    uset_close(contractions);

    return uset_size(unsafe);
}
#endif
