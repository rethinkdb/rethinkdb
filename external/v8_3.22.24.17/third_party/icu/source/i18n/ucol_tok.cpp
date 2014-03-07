/*
*******************************************************************************
*
*   Copyright (C) 2001-2010, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  ucol_tok.cpp
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created 02/22/2001
*   created by: Vladimir Weinstein
*
* This module reads a tailoring rule string and produces a list of
* tokens that will be turned into collation elements
*
*/

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/uscript.h"
#include "unicode/ustring.h"
#include "unicode/uchar.h"
#include "unicode/uniset.h"

#include "cmemory.h"
#include "cstring.h"
#include "ucol_bld.h"
#include "ucol_tok.h"
#include "ulocimp.h"
#include "uresimp.h"
#include "util.h"

// Define this only for debugging.
// #define DEBUG_FOR_COLL_RULES 1

#ifdef DEBUG_FOR_COLL_RULES
#include <iostream>
#endif

U_NAMESPACE_USE

U_CDECL_BEGIN
static int32_t U_CALLCONV
uhash_hashTokens(const UHashTok k)
{
    int32_t hash = 0;
    //uint32_t key = (uint32_t)k.integer;
    UColToken *key = (UColToken *)k.pointer;
    if (key != 0) {
        int32_t len = (key->source & 0xFF000000)>>24;
        int32_t inc = ((len - 32) / 32) + 1;

        const UChar *p = (key->source & 0x00FFFFFF) + *(key->rulesToParseHdl);
        const UChar *limit = p + len;

        while (p<limit) {
            hash = (hash * 37) + *p;
            p += inc;
        }
    }
    return hash;
}

static UBool U_CALLCONV
uhash_compareTokens(const UHashTok key1, const UHashTok key2)
{
    //uint32_t p1 = (uint32_t) key1.integer;
    //uint32_t p2 = (uint32_t) key2.integer;
    UColToken *p1 = (UColToken *)key1.pointer;
    UColToken *p2 = (UColToken *)key2.pointer;
    const UChar *s1 = (p1->source & 0x00FFFFFF) + *(p1->rulesToParseHdl);
    const UChar *s2 = (p2->source & 0x00FFFFFF) + *(p2->rulesToParseHdl);
    uint32_t s1L = ((p1->source & 0xFF000000) >> 24);
    uint32_t s2L = ((p2->source & 0xFF000000) >> 24);
    const UChar *end = s1+s1L-1;

    if (p1 == p2) {
        return TRUE;
    }
    if (p1->source == 0 || p2->source == 0) {
        return FALSE;
    }
    if(s1L != s2L) {
        return FALSE;
    }
    if(p1->source == p2->source) {
        return TRUE;
    }
    while((s1 < end) && *s1 == *s2) {
        ++s1;
        ++s2;
    }
    if(*s1 == *s2) {
        return TRUE;
    } else {
        return FALSE;
    }
}
U_CDECL_END

/*
 * Debug messages used to pinpoint where a format error occurred.
 * A better way is to include context-sensitive information in syntaxError() function.
 *
 * To turn this debugging on, either uncomment the following line, or define use -DDEBUG_FOR_FORMAT_ERROR
 * in the compile line.
 */
/* #define DEBUG_FOR_FORMAT_ERROR 1 */

#ifdef DEBUG_FOR_FORMAT_ERROR
#define DBG_FORMAT_ERROR { printf("U_INVALID_FORMAT_ERROR at line %d", __LINE__);}
#else
#define DBG_FORMAT_ERROR
#endif


/*
 * Controls debug messages so that the output can be compared before and after a
 * big change.  Prints the information of every code point that comes out of the
 * collation parser and its strength into a file.  When a big change in format
 * happens, the files before and after the change should be identical.
 *
 * To turn this debugging on, either uncomment the following line, or define use -DDEBUG_FOR_CODE_POINTS
 * in the compile line.
 */
// #define DEBUG_FOR_CODE_POINTS 1

#ifdef DEBUG_FOR_CODE_POINTS
    FILE* dfcp_fp = NULL;
#endif


/*static inline void U_CALLCONV
uhash_freeBlockWrapper(void *obj) {
    uhash_freeBlock(obj);
}*/


typedef struct {
    uint32_t startCE;
    uint32_t startContCE;
    uint32_t limitCE;
    uint32_t limitContCE;
} indirectBoundaries;

/* these values are used for finding CE values for indirect positioning. */
/* Indirect positioning is a mechanism for allowing resets on symbolic   */
/* values. It only works for resets and you cannot tailor indirect names */
/* An indirect name can define either an anchor point or a range. An     */
/* anchor point behaves in exactly the same way as a code point in reset */
/* would, except that it cannot be tailored. A range (we currently only  */
/* know for the [top] range will explicitly set the upper bound for      */
/* generated CEs, thus allowing for better control over how many CEs can */
/* be squeezed between in the range without performance penalty.         */
/* In that respect, we use [top] for tailoring of locales that use CJK   */
/* characters. Other indirect values are currently a pure convenience,   */
/* they can be used to assure that the CEs will be always positioned in  */
/* the same place relative to a point with known properties (e.g. first  */
/* primary ignorable). */
static indirectBoundaries ucolIndirectBoundaries[15];
/*
static indirectBoundaries ucolIndirectBoundaries[11] = {
{ UCOL_RESET_TOP_VALUE,               0,
UCOL_NEXT_TOP_VALUE,                0 },
{ UCOL_FIRST_PRIMARY_IGNORABLE,       0,
0,                                  0 },
{ UCOL_LAST_PRIMARY_IGNORABLE,        UCOL_LAST_PRIMARY_IGNORABLE_CONT,
0,                                  0 },
{ UCOL_FIRST_SECONDARY_IGNORABLE,     0,
0,                                  0 },
{ UCOL_LAST_SECONDARY_IGNORABLE,      0,
0,                                  0 },
{ UCOL_FIRST_TERTIARY_IGNORABLE,      0,
0,                                  0 },
{ UCOL_LAST_TERTIARY_IGNORABLE,       0,
0,                                  0 },
{ UCOL_FIRST_VARIABLE,                0,
0,                                  0 },
{ UCOL_LAST_VARIABLE,                 0,
0,                                  0 },
{ UCOL_FIRST_NON_VARIABLE,            0,
0,                                  0 },
{ UCOL_LAST_NON_VARIABLE,             0,
0,                                  0 },
};
*/

static void setIndirectBoundaries(uint32_t indexR, uint32_t *start, uint32_t *end) {

    // Set values for the top - TODO: once we have values for all the indirects, we are going
    // to initalize here.
    ucolIndirectBoundaries[indexR].startCE = start[0];
    ucolIndirectBoundaries[indexR].startContCE = start[1];
    if(end) {
        ucolIndirectBoundaries[indexR].limitCE = end[0];
        ucolIndirectBoundaries[indexR].limitContCE = end[1];
    } else {
        ucolIndirectBoundaries[indexR].limitCE = 0;
        ucolIndirectBoundaries[indexR].limitContCE = 0;
    }
}


static inline
void syntaxError(const UChar* rules,
                 int32_t pos,
                 int32_t rulesLen,
                 UParseError* parseError)
{
    parseError->offset = pos;
    parseError->line = 0 ; /* we are not using line numbers */

    // for pre-context
    int32_t start = (pos < U_PARSE_CONTEXT_LEN)? 0 : (pos - (U_PARSE_CONTEXT_LEN-1));
    int32_t stop  = pos;

    u_memcpy(parseError->preContext,rules+start,stop-start);
    //null terminate the buffer
    parseError->preContext[stop-start] = 0;

    //for post-context
    start = pos+1;
    stop  = ((pos+U_PARSE_CONTEXT_LEN)<= rulesLen )? (pos+(U_PARSE_CONTEXT_LEN-1)) :
    rulesLen;

    if(start < stop) {
        u_memcpy(parseError->postContext,rules+start,stop-start);
        //null terminate the buffer
        parseError->postContext[stop-start]= 0;
    } else {
        parseError->postContext[0] = 0;
    }
}

static
void ucol_uprv_tok_setOptionInImage(UColOptionSet *opts, UColAttribute attrib, UColAttributeValue value) {
    switch(attrib) {
    case UCOL_HIRAGANA_QUATERNARY_MODE:
        opts->hiraganaQ = value;
        break;
    case UCOL_FRENCH_COLLATION:
        opts->frenchCollation = value;
        break;
    case UCOL_ALTERNATE_HANDLING:
        opts->alternateHandling = value;
        break;
    case UCOL_CASE_FIRST:
        opts->caseFirst = value;
        break;
    case UCOL_CASE_LEVEL:
        opts->caseLevel = value;
        break;
    case UCOL_NORMALIZATION_MODE:
        opts->normalizationMode = value;
        break;
    case UCOL_STRENGTH:
        opts->strength = value;
        break;
    case UCOL_NUMERIC_COLLATION:
        opts->numericCollation = value;
        break;
    case UCOL_ATTRIBUTE_COUNT:
    default:
        break;
    }
}

#define UTOK_OPTION_COUNT 22

static UBool didInit = FALSE;
/* we can be strict, or we can be lenient */
/* I'd surely be lenient with the option arguments */
/* maybe even with options */
U_STRING_DECL(suboption_00, "non-ignorable", 13);
U_STRING_DECL(suboption_01, "shifted",        7);

U_STRING_DECL(suboption_02, "lower",          5);
U_STRING_DECL(suboption_03, "upper",          5);
U_STRING_DECL(suboption_04, "off",            3);
U_STRING_DECL(suboption_05, "on",             2);
U_STRING_DECL(suboption_06, "1",              1);
U_STRING_DECL(suboption_07, "2",              1);
U_STRING_DECL(suboption_08, "3",              1);
U_STRING_DECL(suboption_09, "4",              1);
U_STRING_DECL(suboption_10, "I",              1);

U_STRING_DECL(suboption_11, "primary",        7);
U_STRING_DECL(suboption_12, "secondary",      9);
U_STRING_DECL(suboption_13, "tertiary",       8);
U_STRING_DECL(suboption_14, "variable",       8);
U_STRING_DECL(suboption_15, "regular",        7);
U_STRING_DECL(suboption_16, "implicit",       8);
U_STRING_DECL(suboption_17, "trailing",       8);


U_STRING_DECL(option_00,    "undefined",      9);
U_STRING_DECL(option_01,    "rearrange",      9);
U_STRING_DECL(option_02,    "alternate",      9);
U_STRING_DECL(option_03,    "backwards",      9);
U_STRING_DECL(option_04,    "variable top",  12);
U_STRING_DECL(option_05,    "top",            3);
U_STRING_DECL(option_06,    "normalization", 13);
U_STRING_DECL(option_07,    "caseLevel",      9);
U_STRING_DECL(option_08,    "caseFirst",      9);
U_STRING_DECL(option_09,    "scriptOrder",   11);
U_STRING_DECL(option_10,    "charsetname",   11);
U_STRING_DECL(option_11,    "charset",        7);
U_STRING_DECL(option_12,    "before",         6);
U_STRING_DECL(option_13,    "hiraganaQ",      9);
U_STRING_DECL(option_14,    "strength",       8);
U_STRING_DECL(option_15,    "first",          5);
U_STRING_DECL(option_16,    "last",           4);
U_STRING_DECL(option_17,    "optimize",       8);
U_STRING_DECL(option_18,    "suppressContractions",         20);
U_STRING_DECL(option_19,    "numericOrdering",              15);
U_STRING_DECL(option_20,    "import",         6);
U_STRING_DECL(option_21,    "reorder",         7);

/*
[last variable] last variable value
[last primary ignorable] largest CE for primary ignorable
[last secondary ignorable] largest CE for secondary ignorable
[last tertiary ignorable] largest CE for tertiary ignorable
[top] guaranteed to be above all implicit CEs, for now and in the future (in 1.8)
*/


static const ucolTokSuboption alternateSub[2] = {
    {suboption_00, 13, UCOL_NON_IGNORABLE},
    {suboption_01,  7, UCOL_SHIFTED}
};

static const ucolTokSuboption caseFirstSub[3] = {
    {suboption_02, 5, UCOL_LOWER_FIRST},
    {suboption_03,  5, UCOL_UPPER_FIRST},
    {suboption_04,  3, UCOL_OFF},
};

static const ucolTokSuboption onOffSub[2] = {
    {suboption_04, 3, UCOL_OFF},
    {suboption_05, 2, UCOL_ON}
};

static const ucolTokSuboption frenchSub[1] = {
    {suboption_07, 1, UCOL_ON}
};

static const ucolTokSuboption beforeSub[3] = {
    {suboption_06, 1, UCOL_PRIMARY},
    {suboption_07, 1, UCOL_SECONDARY},
    {suboption_08, 1, UCOL_TERTIARY}
};

static const ucolTokSuboption strengthSub[5] = {
    {suboption_06, 1, UCOL_PRIMARY},
    {suboption_07, 1, UCOL_SECONDARY},
    {suboption_08, 1, UCOL_TERTIARY},
    {suboption_09, 1, UCOL_QUATERNARY},
    {suboption_10, 1, UCOL_IDENTICAL},
};

static const ucolTokSuboption firstLastSub[7] = {
    {suboption_11, 7, UCOL_PRIMARY},
    {suboption_12, 9, UCOL_PRIMARY},
    {suboption_13, 8, UCOL_PRIMARY},
    {suboption_14, 8, UCOL_PRIMARY},
    {suboption_15, 7, UCOL_PRIMARY},
    {suboption_16, 8, UCOL_PRIMARY},
    {suboption_17, 8, UCOL_PRIMARY},
};

enum OptionNumber {
    OPTION_ALTERNATE_HANDLING = 0,
    OPTION_FRENCH_COLLATION,
    OPTION_CASE_LEVEL,
    OPTION_CASE_FIRST,
    OPTION_NORMALIZATION_MODE,
    OPTION_HIRAGANA_QUATERNARY,
    OPTION_STRENGTH,
    OPTION_NUMERIC_COLLATION,
    OPTION_NORMAL_OPTIONS_LIMIT = OPTION_NUMERIC_COLLATION,
    OPTION_VARIABLE_TOP,
    OPTION_REARRANGE,
    OPTION_BEFORE,
    OPTION_TOP,
    OPTION_FIRST,
    OPTION_LAST,
    OPTION_OPTIMIZE,
    OPTION_SUPPRESS_CONTRACTIONS,
    OPTION_UNDEFINED,
    OPTION_SCRIPT_ORDER,
    OPTION_CHARSET_NAME,
    OPTION_CHARSET,
    OPTION_IMPORT,
    OPTION_SCRIPTREORDER
} ;

static const ucolTokOption rulesOptions[UTOK_OPTION_COUNT] = {
    /*00*/ {option_02,  9, alternateSub, 2, UCOL_ALTERNATE_HANDLING}, /*"alternate" */
    /*01*/ {option_03,  9, frenchSub, 1, UCOL_FRENCH_COLLATION}, /*"backwards"      */
    /*02*/ {option_07,  9, onOffSub, 2, UCOL_CASE_LEVEL},  /*"caseLevel"      */
    /*03*/ {option_08,  9, caseFirstSub, 3, UCOL_CASE_FIRST}, /*"caseFirst"   */
    /*04*/ {option_06, 13, onOffSub, 2, UCOL_NORMALIZATION_MODE}, /*"normalization" */
    /*05*/ {option_13, 9, onOffSub, 2, UCOL_HIRAGANA_QUATERNARY_MODE}, /*"hiraganaQ" */
    /*06*/ {option_14, 8, strengthSub, 5, UCOL_STRENGTH}, /*"strength" */
    /*07*/ {option_19, 15, onOffSub, 2, UCOL_NUMERIC_COLLATION},  /*"numericOrdering"*/
    /*08*/ {option_04, 12, NULL, 0, UCOL_ATTRIBUTE_COUNT}, /*"variable top"   */
    /*09*/ {option_01,  9, NULL, 0, UCOL_ATTRIBUTE_COUNT}, /*"rearrange"      */
    /*10*/ {option_12,  6, beforeSub, 3, UCOL_ATTRIBUTE_COUNT}, /*"before"    */
    /*11*/ {option_05,  3, NULL, 0, UCOL_ATTRIBUTE_COUNT}, /*"top"            */
    /*12*/ {option_15,  5, firstLastSub, 7, UCOL_ATTRIBUTE_COUNT}, /*"first" */
    /*13*/ {option_16,  4, firstLastSub, 7, UCOL_ATTRIBUTE_COUNT}, /*"last" */
    /*14*/ {option_17,  8, NULL, 0, UCOL_ATTRIBUTE_COUNT}, /*"optimize"      */
    /*15*/ {option_18, 20, NULL, 0, UCOL_ATTRIBUTE_COUNT}, /*"suppressContractions"      */
    /*16*/ {option_00,  9, NULL, 0, UCOL_ATTRIBUTE_COUNT}, /*"undefined"      */
    /*17*/ {option_09, 11, NULL, 0, UCOL_ATTRIBUTE_COUNT}, /*"scriptOrder"    */
    /*18*/ {option_10, 11, NULL, 0, UCOL_ATTRIBUTE_COUNT}, /*"charsetname"    */
    /*19*/ {option_11,  7, NULL, 0, UCOL_ATTRIBUTE_COUNT},  /*"charset"        */
    /*20*/ {option_20,  6, NULL, 0, UCOL_ATTRIBUTE_COUNT},  /*"import"        */
    /*21*/ {option_21,  7, NULL, 0, UCOL_ATTRIBUTE_COUNT}  /*"reorder"        */
};

static
int32_t u_strncmpNoCase(const UChar     *s1,
                        const UChar     *s2,
                        int32_t     n)
{
    if(n > 0) {
        int32_t rc;
        for(;;) {
            rc = (int32_t)u_tolower(*s1) - (int32_t)u_tolower(*s2);
            if(rc != 0 || *s1 == 0 || --n == 0) {
                return rc;
            }
            ++s1;
            ++s2;
        }
    }
    return 0;
}

static
void ucol_uprv_tok_initData() {
    if(!didInit) {
        U_STRING_INIT(suboption_00, "non-ignorable", 13);
        U_STRING_INIT(suboption_01, "shifted",        7);

        U_STRING_INIT(suboption_02, "lower",          5);
        U_STRING_INIT(suboption_03, "upper",          5);
        U_STRING_INIT(suboption_04, "off",            3);
        U_STRING_INIT(suboption_05, "on",             2);

        U_STRING_INIT(suboption_06, "1",              1);
        U_STRING_INIT(suboption_07, "2",              1);
        U_STRING_INIT(suboption_08, "3",              1);
        U_STRING_INIT(suboption_09, "4",              1);
        U_STRING_INIT(suboption_10, "I",              1);

        U_STRING_INIT(suboption_11, "primary",        7);
        U_STRING_INIT(suboption_12, "secondary",      9);
        U_STRING_INIT(suboption_13, "tertiary",       8);
        U_STRING_INIT(suboption_14, "variable",       8);
        U_STRING_INIT(suboption_15, "regular",        7);
        U_STRING_INIT(suboption_16, "implicit",       8);
        U_STRING_INIT(suboption_17, "trailing",       8);


        U_STRING_INIT(option_00, "undefined",      9);
        U_STRING_INIT(option_01, "rearrange",      9);
        U_STRING_INIT(option_02, "alternate",      9);
        U_STRING_INIT(option_03, "backwards",      9);
        U_STRING_INIT(option_04, "variable top",  12);
        U_STRING_INIT(option_05, "top",            3);
        U_STRING_INIT(option_06, "normalization", 13);
        U_STRING_INIT(option_07, "caseLevel",      9);
        U_STRING_INIT(option_08, "caseFirst",      9);
        U_STRING_INIT(option_09, "scriptOrder",   11);
        U_STRING_INIT(option_10, "charsetname",   11);
        U_STRING_INIT(option_11, "charset",        7);
        U_STRING_INIT(option_12, "before",         6);
        U_STRING_INIT(option_13, "hiraganaQ",      9);
        U_STRING_INIT(option_14, "strength",       8);
        U_STRING_INIT(option_15, "first",          5);
        U_STRING_INIT(option_16, "last",           4);
        U_STRING_INIT(option_17, "optimize",       8);
        U_STRING_INIT(option_18, "suppressContractions",         20);
        U_STRING_INIT(option_19, "numericOrdering",      15);
        U_STRING_INIT(option_20, "import ",        6);
        U_STRING_INIT(option_21, "reorder",        7);
        didInit = TRUE;
    }
}


// This function reads basic options to set in the runtime collator
// used by data driven tests. Should not support build time options
U_CAPI const UChar * U_EXPORT2
ucol_tok_getNextArgument(const UChar *start, const UChar *end,
                         UColAttribute *attrib, UColAttributeValue *value,
                         UErrorCode *status)
{
    uint32_t i = 0;
    int32_t j=0;
    UBool foundOption = FALSE;
    const UChar *optionArg = NULL;

    ucol_uprv_tok_initData();

    while(start < end && (u_isWhitespace(*start) || uprv_isRuleWhiteSpace(*start))) { /* eat whitespace */
        start++;
    }
    if(start >= end) {
        return NULL;
    }
    /* skip opening '[' */
    if(*start == 0x005b) {
        start++;
    } else {
        *status = U_ILLEGAL_ARGUMENT_ERROR; // no opening '['
        return NULL;
    }

    while(i < UTOK_OPTION_COUNT) {
        if(u_strncmpNoCase(start, rulesOptions[i].optionName, rulesOptions[i].optionLen) == 0) {
            foundOption = TRUE;
            if(end - start > rulesOptions[i].optionLen) {
                optionArg = start+rulesOptions[i].optionLen+1; /* start of the options, skip space */
                while(u_isWhitespace(*optionArg) || uprv_isRuleWhiteSpace(*optionArg)) { /* eat whitespace */
                    optionArg++;
                }
            }
            break;
        }
        i++;
    }

    if(!foundOption) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    }

    if(optionArg) {
        for(j = 0; j<rulesOptions[i].subSize; j++) {
            if(u_strncmpNoCase(optionArg, rulesOptions[i].subopts[j].subName, rulesOptions[i].subopts[j].subLen) == 0) {
                //ucol_uprv_tok_setOptionInImage(src->opts, rulesOptions[i].attr, rulesOptions[i].subopts[j].attrVal);
                *attrib = rulesOptions[i].attr;
                *value = rulesOptions[i].subopts[j].attrVal;
                optionArg += rulesOptions[i].subopts[j].subLen;
                while(u_isWhitespace(*optionArg) || uprv_isRuleWhiteSpace(*optionArg)) { /* eat whitespace */
                    optionArg++;
                }
                if(*optionArg == 0x005d) {
                    optionArg++;
                    return optionArg;
                } else {
                    *status = U_ILLEGAL_ARGUMENT_ERROR;
                    return NULL;
                }
            }
        }
    }
    *status = U_ILLEGAL_ARGUMENT_ERROR;
    return NULL;
}

static
USet *ucol_uprv_tok_readAndSetUnicodeSet(const UChar *start, const UChar *end, UErrorCode *status) {
    while(*start != 0x005b) { /* advance while we find the first '[' */
        start++;
    }
    // now we need to get a balanced set of '[]'. The problem is that a set can have
    // many, and *end point to the first closing '['
    int32_t noOpenBraces = 1;
    int32_t current = 1; // skip the opening brace
    while(start+current < end && noOpenBraces != 0) {
        if(start[current] == 0x005b) {
            noOpenBraces++;
        } else if(start[current] == 0x005D) { // closing brace
            noOpenBraces--;
        }
        current++;
    }

    if(noOpenBraces != 0 || u_strchr(start+current, 0x005d /*']'*/) == NULL) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    }
    return uset_openPattern(start, current, status);
}

/**
 * Reads an option and matches the option name with the predefined options. (Case-insensitive.)
 * @param start Pointer to the start UChar.
 * @param end Pointer to the last valid pointer beyond which the option will not extend.
 * @param optionArg Address of the pointer at which the options start (after the option name)
 * @return The index of the option, or -1 if the option is not valid.
 */
static
int32_t ucol_uprv_tok_readOption(const UChar *start, const UChar *end, const UChar **optionArg) {
    int32_t i = 0;
    ucol_uprv_tok_initData();

    while(u_isWhitespace(*start) || uprv_isRuleWhiteSpace(*start)) { /* eat whitespace */
        start++;
    }
    while(i < UTOK_OPTION_COUNT) {
        if(u_strncmpNoCase(start, rulesOptions[i].optionName, rulesOptions[i].optionLen) == 0) {
            if(end - start > rulesOptions[i].optionLen) {
                *optionArg = start+rulesOptions[i].optionLen; /* End of option name; start of the options */
                while(u_isWhitespace(**optionArg) || uprv_isRuleWhiteSpace(**optionArg)) { /* eat whitespace */
                    (*optionArg)++;
                }
            }
            break;
        }
        i++;
    }
    if(i == UTOK_OPTION_COUNT) {
        i = -1; // didn't find an option
    }
    return i;
}


static
void ucol_tok_parseScriptReorder(UColTokenParser *src, UErrorCode *status) {
    int32_t codeCount = 0;
    int32_t codeIndex = 0;
    char conversion[64];
    int32_t tokenLength = 0;
    const UChar* space;
    
    const UChar* current = src->current;
    const UChar* end = u_memchr(src->current, 0x005d, src->end - src->current);

    // eat leading whitespace
    while(current < end && u_isWhitespace(*current)) {
        current++;
    }

    while(current < end) {    
        space = u_memchr(current, 0x0020, end - current);
        space = space == 0 ? end : space;
        tokenLength = space - current;
        if (tokenLength < 4) {
            *status = U_INVALID_FORMAT_ERROR;
            return;
        }
        codeCount++;
        current += tokenLength;
        while(current < end && u_isWhitespace(*current)) { /* eat whitespace */
            ++current;
        }
    }

    if (codeCount == 0) {
        *status = U_INVALID_FORMAT_ERROR;
    }
    
    src->reorderCodesLength = codeCount;
    src->reorderCodes = (int32_t*)uprv_malloc(codeCount * sizeof(int32_t));
    current = src->current;
    
    // eat leading whitespace
    while(current < end && u_isWhitespace(*current)) {
        current++;
    }

    while(current < end) {    
        space = u_memchr(current, 0x0020, end - current);
        space = space == 0 ? end : space;
        tokenLength = space - current;
        if (tokenLength < 4) {
            *status = U_ILLEGAL_ARGUMENT_ERROR;
            return;
        } else {
            u_UCharsToChars(current, conversion, tokenLength);
            conversion[tokenLength] = '\0';
            src->reorderCodes[codeIndex] = ucol_findReorderingEntry(conversion);
            if (src->reorderCodes[codeIndex] == USCRIPT_INVALID_CODE) {
                src->reorderCodes[codeIndex] = u_getPropertyValueEnum(UCHAR_SCRIPT, conversion);
            }
            if (src->reorderCodes[codeIndex] == USCRIPT_INVALID_CODE) {
                *status = U_ILLEGAL_ARGUMENT_ERROR;
            }
        }
        codeIndex++;
        current += tokenLength;
        while(current < end && u_isWhitespace(*current)) { /* eat whitespace */
            ++current;
        }
    }
}

// reads and conforms to various options in rules
// end is the position of the first closing ']'
// However, some of the options take an UnicodeSet definition
// which needs to duplicate the closing ']'
// for example: '[copy [\uAC00-\uD7FF]]'
// These options will move end to the second ']' and the
// caller will set the current to it.
static
uint8_t ucol_uprv_tok_readAndSetOption(UColTokenParser *src, UErrorCode *status) {
    const UChar* start = src->current;
    int32_t i = 0;
    int32_t j=0;
    const UChar *optionArg = NULL;

    uint8_t result = 0;

    start++; /*skip opening '['*/
    i = ucol_uprv_tok_readOption(start, src->end, &optionArg);
    if(optionArg) {
        src->current = optionArg;
    }

    if(i < 0) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
    } else {
        int32_t noOpenBraces = 1;
        switch(i) {
    case OPTION_ALTERNATE_HANDLING:
    case OPTION_FRENCH_COLLATION:
    case OPTION_CASE_LEVEL:
    case OPTION_CASE_FIRST:
    case OPTION_NORMALIZATION_MODE:
    case OPTION_HIRAGANA_QUATERNARY:
    case OPTION_STRENGTH:
    case OPTION_NUMERIC_COLLATION:
        if(optionArg) {
            for(j = 0; j<rulesOptions[i].subSize; j++) {
                if(u_strncmpNoCase(optionArg, rulesOptions[i].subopts[j].subName, rulesOptions[i].subopts[j].subLen) == 0) {
                    ucol_uprv_tok_setOptionInImage(src->opts, rulesOptions[i].attr, rulesOptions[i].subopts[j].attrVal);
                    result =  UCOL_TOK_SUCCESS;
                }
            }
        }
        if(result == 0) {
            *status = U_ILLEGAL_ARGUMENT_ERROR;
        }
        break;
    case OPTION_VARIABLE_TOP:
        result = UCOL_TOK_SUCCESS | UCOL_TOK_VARIABLE_TOP;
        break;
    case OPTION_REARRANGE:
        result = UCOL_TOK_SUCCESS;
        break;
    case OPTION_BEFORE:
        if(optionArg) {
            for(j = 0; j<rulesOptions[i].subSize; j++) {
                if(u_strncmpNoCase(optionArg, rulesOptions[i].subopts[j].subName, rulesOptions[i].subopts[j].subLen) == 0) {
                    result = UCOL_TOK_SUCCESS | (rulesOptions[i].subopts[j].attrVal + 1);
                }
            }
        }
        if(result == 0) {
            *status = U_ILLEGAL_ARGUMENT_ERROR;
        }
        break;
    case OPTION_TOP: /* we are going to have an array with structures of limit CEs */
        /* index to this array will be src->parsedToken.indirectIndex*/
        src->parsedToken.indirectIndex = 0;
        result = UCOL_TOK_SUCCESS | UCOL_TOK_TOP;
        break;
    case OPTION_FIRST:
    case OPTION_LAST: /* first, last */
        for(j = 0; j<rulesOptions[i].subSize; j++) {
            if(u_strncmpNoCase(optionArg, rulesOptions[i].subopts[j].subName, rulesOptions[i].subopts[j].subLen) == 0) {
                // the calculation below assumes that OPTION_FIRST and OPTION_LAST are at i and i+1 and that the first
                // element of indirect boundaries is reserved for top.
                src->parsedToken.indirectIndex = (uint16_t)(i-OPTION_FIRST+1+j*2);
                result =  UCOL_TOK_SUCCESS | UCOL_TOK_TOP;;
            }
        }
        if(result == 0) {
            *status = U_ILLEGAL_ARGUMENT_ERROR;
        }
        break;
    case OPTION_OPTIMIZE:
    case OPTION_SUPPRESS_CONTRACTIONS:  // copy and remove are handled before normalization
        // we need to move end here
        src->current++; // skip opening brace
        while(src->current < src->end && noOpenBraces != 0) {
            if(*src->current == 0x005b) {
                noOpenBraces++;
            } else if(*src->current == 0x005D) { // closing brace
                noOpenBraces--;
            }
            src->current++;
        }
        result = UCOL_TOK_SUCCESS;
        break;
    case OPTION_SCRIPTREORDER:
        ucol_tok_parseScriptReorder(src, status);
        break;
    default:
        *status = U_UNSUPPORTED_ERROR;
        break;
        }
    }
    src->current = u_memchr(src->current, 0x005d, (int32_t)(src->end-src->current));
    return result;
}


inline void ucol_tok_addToExtraCurrent(UColTokenParser *src, const UChar *stuff, int32_t len, UErrorCode *status) {
    if (stuff == NULL || len <= 0) {
        return;
    }
    UnicodeString tempStuff(FALSE, stuff, len);
    if(src->extraCurrent+len >= src->extraEnd) {
        /* reallocate */
        if (stuff >= src->source && stuff <= src->end) {
            // Copy the "stuff" contents into tempStuff's own buffer.
            // UnicodeString is copy-on-write.
            if (len > 0) {
                tempStuff.setCharAt(0, tempStuff[0]);
            } else {
                tempStuff.remove();
            }
        }
        UChar *newSrc = (UChar *)uprv_realloc(src->source, (src->extraEnd-src->source)*2*sizeof(UChar));
        if(newSrc != NULL) {
            src->current = newSrc + (src->current - src->source);
            src->extraCurrent = newSrc + (src->extraCurrent - src->source);
            src->end = newSrc + (src->end - src->source);
            src->extraEnd = newSrc + (src->extraEnd-src->source)*2;
            src->sourceCurrent = newSrc + (src->sourceCurrent-src->source);
            src->source = newSrc;
        } else {
            *status = U_MEMORY_ALLOCATION_ERROR;
            return;
        }
    }
    if(len == 1) {
        *src->extraCurrent++ = tempStuff[0];
    } else {
        u_memcpy(src->extraCurrent, tempStuff.getBuffer(), len);
        src->extraCurrent += len;
    }
}

inline UBool ucol_tok_doSetTop(UColTokenParser *src, UErrorCode *status) {
    /*
    top = TRUE;
    */
    UChar buff[5];
    src->parsedToken.charsOffset = (uint32_t)(src->extraCurrent - src->source);
    buff[0] = 0xFFFE;
    buff[1] = (UChar)(ucolIndirectBoundaries[src->parsedToken.indirectIndex].startCE >> 16);
    buff[2] = (UChar)(ucolIndirectBoundaries[src->parsedToken.indirectIndex].startCE & 0xFFFF);
    if(ucolIndirectBoundaries[src->parsedToken.indirectIndex].startContCE == 0) {
        src->parsedToken.charsLen = 3;
        ucol_tok_addToExtraCurrent(src, buff, 3, status);
    } else {
        buff[3] = (UChar)(ucolIndirectBoundaries[src->parsedToken.indirectIndex].startContCE >> 16);
        buff[4] = (UChar)(ucolIndirectBoundaries[src->parsedToken.indirectIndex].startContCE & 0xFFFF);
        src->parsedToken.charsLen = 5;
        ucol_tok_addToExtraCurrent(src, buff, 5, status);
    }
    return TRUE;
}

static UBool isCharNewLine(UChar c){
    switch(c){
    case 0x000A: /* LF  */
    case 0x000D: /* CR  */
    case 0x000C: /* FF  */
    case 0x0085: /* NEL */
    case 0x2028: /* LS  */
    case 0x2029: /* PS  */
        return TRUE;
    default:
        return FALSE;
    }
}

/*
 * This function is called several times when a range is processed.  Each time, the next code point
 * is processed.
 * The following variables must be set before calling this function:
 *   src->currentRangeCp:  The current code point to process.
 *   src->lastRangeCp: The last code point in the range.
 * Pre-requisite: src->currentRangeCp <= src->lastRangeCp.
 */
static const UChar*
ucol_tok_processNextCodePointInRange(UColTokenParser *src,
                                     UErrorCode *status)
{
  // Append current code point to source
  UChar buff[U16_MAX_LENGTH];
  uint32_t i = 0;

  uint32_t nChars = U16_LENGTH(src->currentRangeCp);
  src->parsedToken.charsOffset = (uint32_t)(src->extraCurrent - src->source);
  src->parsedToken.charsLen = nChars;

  U16_APPEND_UNSAFE(buff, i, src->currentRangeCp);
  ucol_tok_addToExtraCurrent(src, buff, nChars, status);

  ++src->currentRangeCp;
  if (src->currentRangeCp > src->lastRangeCp) {
    src->inRange = FALSE;

    if (src->currentStarredCharIndex > src->lastStarredCharIndex) {
      src->isStarred = FALSE;
    }
  } else {
    src->previousCp = src->currentRangeCp;
  }
  return src->current;
}

/*
 * This function is called several times when a starred list is processed.  Each time, the next code point
 * in the list is processed.
 * The following variables must be set before calling this function:
 *   src->currentStarredCharIndex:  Index (in src->source) of the first char of the current code point.
 *   src->lastStarredCharIndex: Index to the last character in the list.
 * Pre-requisite: src->currentStarredCharIndex <= src->lastStarredCharIndex.
 */
static const UChar*
ucol_tok_processNextTokenInStarredList(UColTokenParser *src)
{
  // Extract the characters corresponding to the next code point.
  UChar32 cp;
  src->parsedToken.charsOffset = src->currentStarredCharIndex;
  int32_t prev = src->currentStarredCharIndex;
  U16_NEXT(src->source, src->currentStarredCharIndex, (uint32_t)(src->end - src->source), cp);
  src->parsedToken.charsLen = src->currentStarredCharIndex - prev;

  // When we are done parsing the starred string, turn the flag off so that
  // the normal processing is restored.
  if (src->currentStarredCharIndex > src->lastStarredCharIndex) {
    src->isStarred = FALSE;
  }
  src->previousCp = cp;
  return src->current;
}

/*
 * Partially parses the next token, keeps the indices in src->parsedToken, and updates the counters.
 *
 * This routine parses and separates almost all tokens. The following are the syntax characters recognized.
 *  # : Comment character
 *  & : Reset operator
 *  = : Equality
 *  < : Primary collation
 *  << : Secondary collation
 *  <<< : Tertiary collation
 *  ; : Secondary collation
 *  , : Tertiary collation
 *  / : Expansions
 *  | : Prefix
 *  - : Range

 *  ! : Java Thai modifier, ignored
 *  @ : French only

 * [] : Options
 * '' : Quotes
 *
 *  Along with operators =, <, <<, <<<, the operator * is supported to indicate a list.  For example, &a<*bcdexyz
 *  is equivalent to &a<b<c<d<e<x<y<z.  In lists, ranges also can be given, so &a*b-ex-z is equivalent to the above.
 *  This function do not separate the tokens in a list.  Instead, &a<*b-ex-z is parsed as three tokens - "&a",
 *  "<*b", "-ex", "-z".  The strength (< in this case), whether in a list, whether in a range and the previous
 *  character returned as cached so that the calling program can do further splitting.
 */
static const UChar*
ucol_tok_parseNextTokenInternal(UColTokenParser *src,
                                UBool startOfRules,
                                UParseError *parseError,
                                UErrorCode *status)
{
    UBool variableTop = FALSE;
    UBool top = FALSE;
    UBool inChars = TRUE;
    UBool inQuote = FALSE;
    UBool wasInQuote = FALSE;
    uint8_t before = 0;
    UBool isEscaped = FALSE;

    // TODO: replace these variables with src->parsedToken counterparts
    // no need to use them anymore since we have src->parsedToken.
    // Ideally, token parser would be a nice class... Once, when I have
    // more time (around 2020 probably).
    uint32_t newExtensionLen = 0;
    uint32_t extensionOffset = 0;
    uint32_t newStrength = UCOL_TOK_UNSET;
    UChar buff[10];

    src->parsedToken.charsOffset = 0;  src->parsedToken.charsLen = 0;
    src->parsedToken.prefixOffset = 0; src->parsedToken.prefixLen = 0;
    src->parsedToken.indirectIndex = 0;

    while (src->current < src->end) {
        UChar ch = *(src->current);

        if (inQuote) {
            if (ch == 0x0027/*'\''*/) {
                inQuote = FALSE;
            } else {
                if ((src->parsedToken.charsLen == 0) || inChars) {
                    if(src->parsedToken.charsLen == 0) {
                        src->parsedToken.charsOffset = (uint32_t)(src->extraCurrent - src->source);
                    }
                    src->parsedToken.charsLen++;
                } else {
                    if(newExtensionLen == 0) {
                        extensionOffset = (uint32_t)(src->extraCurrent - src->source);
                    }
                    newExtensionLen++;
                }
            }
        }else if(isEscaped){
            isEscaped =FALSE;
            if (newStrength == UCOL_TOK_UNSET) {
                *status = U_INVALID_FORMAT_ERROR;
                syntaxError(src->source,(int32_t)(src->current-src->source),(int32_t)(src->end-src->source),parseError);
                DBG_FORMAT_ERROR
                return NULL;
                // enabling rules to start with non-tokens a < b
                // newStrength = UCOL_TOK_RESET;
            }
            if(ch != 0x0000  && src->current != src->end) {
                if (inChars) {
                    if(src->parsedToken.charsLen == 0) {
                        src->parsedToken.charsOffset = (uint32_t)(src->current - src->source);
                    }
                    src->parsedToken.charsLen++;
                } else {
                    if(newExtensionLen == 0) {
                        extensionOffset = (uint32_t)(src->current - src->source);
                    }
                    newExtensionLen++;
                }
            }
        }else {
            if(!uprv_isRuleWhiteSpace(ch)) {
                /* Sets the strength for this entry */
                switch (ch) {
                case 0x003D/*'='*/ :
                    if (newStrength != UCOL_TOK_UNSET) {
                        goto EndOfLoop;
                    }

                    /* if we start with strength, we'll reset to top */
                    if(startOfRules == TRUE) {
                        src->parsedToken.indirectIndex = 5;
                        top = ucol_tok_doSetTop(src, status);
                        newStrength = UCOL_TOK_RESET;
                        goto EndOfLoop;
                    }
                    newStrength = UCOL_IDENTICAL;
                    if(*(src->current+1) == 0x002A) {/*'*'*/
                        src->current++;
                        src->isStarred = TRUE;
                    }
                    break;

                case 0x002C/*','*/:
                    if (newStrength != UCOL_TOK_UNSET) {
                        goto EndOfLoop;
                    }

                    /* if we start with strength, we'll reset to top */
                    if(startOfRules == TRUE) {
                        src->parsedToken.indirectIndex = 5;
                        top = ucol_tok_doSetTop(src, status);
                        newStrength = UCOL_TOK_RESET;
                        goto EndOfLoop;
                    }
                    newStrength = UCOL_TERTIARY;
                    break;

                case  0x003B/*';'*/:
                    if (newStrength != UCOL_TOK_UNSET) {
                        goto EndOfLoop;
                    }

                    /* if we start with strength, we'll reset to top */
                    if(startOfRules == TRUE) {
                        src->parsedToken.indirectIndex = 5;
                        top = ucol_tok_doSetTop(src, status);
                        newStrength = UCOL_TOK_RESET;
                        goto EndOfLoop;
                    }
                    newStrength = UCOL_SECONDARY;
                    break;

                case 0x003C/*'<'*/:
                    if (newStrength != UCOL_TOK_UNSET) {
                        goto EndOfLoop;
                    }

                    /* if we start with strength, we'll reset to top */
                    if(startOfRules == TRUE) {
                        src->parsedToken.indirectIndex = 5;
                        top = ucol_tok_doSetTop(src, status);
                        newStrength = UCOL_TOK_RESET;
                        goto EndOfLoop;
                    }
                    /* before this, do a scan to verify whether this is */
                    /* another strength */
                    if(*(src->current+1) == 0x003C) {
                        src->current++;
                        if(*(src->current+1) == 0x003C) {
                            src->current++; /* three in a row! */
                            newStrength = UCOL_TERTIARY;
                        } else { /* two in a row */
                            newStrength = UCOL_SECONDARY;
                        }
                    } else { /* just one */
                        newStrength = UCOL_PRIMARY;
                    }
                    if(*(src->current+1) == 0x002A) {/*'*'*/
                        src->current++;
                        src->isStarred = TRUE;
                    }
                    break;

                case 0x0026/*'&'*/:
                    if (newStrength != UCOL_TOK_UNSET) {
                        /**/
                        goto EndOfLoop;
                    }

                    newStrength = UCOL_TOK_RESET; /* PatternEntry::RESET = 0 */
                    break;

                case 0x005b/*'['*/:
                    /* options - read an option, analyze it */
                    if(u_strchr(src->current, 0x005d /*']'*/) != NULL) {
                        uint8_t result = ucol_uprv_tok_readAndSetOption(src, status);
                        if(U_SUCCESS(*status)) {
                            if(result & UCOL_TOK_TOP) {
                                if(newStrength == UCOL_TOK_RESET) {
                                    top = ucol_tok_doSetTop(src, status);
                                    if(before) { // This is a combination of before and indirection like '&[before 2][first regular]<b'
                                        src->parsedToken.charsLen+=2;
                                        buff[0] = 0x002d;
                                        buff[1] = before;
                                        ucol_tok_addToExtraCurrent(src, buff, 2, status);
                                    }

                                    src->current++;
                                    goto EndOfLoop;
                                } else {
                                    *status = U_INVALID_FORMAT_ERROR;
                                    syntaxError(src->source,(int32_t)(src->current-src->source),(int32_t)(src->end-src->source),parseError);
                                    DBG_FORMAT_ERROR
                                }
                            } else if(result & UCOL_TOK_VARIABLE_TOP) {
                                if(newStrength != UCOL_TOK_RESET && newStrength != UCOL_TOK_UNSET) {
                                    variableTop = TRUE;
                                    src->parsedToken.charsOffset = (uint32_t)(src->extraCurrent - src->source);
                                    src->parsedToken.charsLen = 1;
                                    buff[0] = 0xFFFF;
                                    ucol_tok_addToExtraCurrent(src, buff, 1, status);
                                    src->current++;
                                    goto EndOfLoop;
                                } else {
                                    *status = U_INVALID_FORMAT_ERROR;
                                    syntaxError(src->source,(int32_t)(src->current-src->source),(int32_t)(src->end-src->source),parseError);
                                    DBG_FORMAT_ERROR
                                }
                            } else if (result & UCOL_TOK_BEFORE){
                                if(newStrength == UCOL_TOK_RESET) {
                                    before = result & UCOL_TOK_BEFORE;
                                } else {
                                    *status = U_INVALID_FORMAT_ERROR;
                                    syntaxError(src->source,(int32_t)(src->current-src->source),(int32_t)(src->end-src->source),parseError);
                                    DBG_FORMAT_ERROR
                                }
                            }
                        } else {
                            *status = U_INVALID_FORMAT_ERROR;
                            syntaxError(src->source,(int32_t)(src->current-src->source),(int32_t)(src->end-src->source),parseError);
                            DBG_FORMAT_ERROR
                            return NULL;
                        }
                    }
                    break;
                case 0x0021/*! skip java thai modifier reordering*/:
                    break;
                case 0x002F/*'/'*/:
                    wasInQuote = FALSE; /* if we were copying source characters, we want to stop now */
                    inChars = FALSE; /* we're now processing expansion */
                    break;
                case 0x005C /* back slash for escaped chars */:
                    isEscaped = TRUE;
                    break;
                    /* found a quote, we're gonna start copying */
                case 0x0027/*'\''*/:
                    if (newStrength == UCOL_TOK_UNSET) { /* quote is illegal until we have a strength */
                      *status = U_INVALID_FORMAT_ERROR;
                      syntaxError(src->source,(int32_t)(src->current-src->source),(int32_t)(src->end-src->source),parseError);
                      DBG_FORMAT_ERROR
                      return NULL;
                      // enabling rules to start with a non-token character a < b
                      // newStrength = UCOL_TOK_RESET;
                    }

                    inQuote = TRUE;

                    if(inChars) { /* we're doing characters */
                        if(wasInQuote == FALSE) {
                            src->parsedToken.charsOffset = (uint32_t)(src->extraCurrent - src->source);
                        }
                        if (src->parsedToken.charsLen != 0) {
                            ucol_tok_addToExtraCurrent(src, src->current - src->parsedToken.charsLen, src->parsedToken.charsLen, status);
                        }
                        src->parsedToken.charsLen++;
                    } else { /* we're doing an expansion */
                        if(wasInQuote == FALSE) {
                            extensionOffset = (uint32_t)(src->extraCurrent - src->source);
                        }
                        if (newExtensionLen != 0) {
                            ucol_tok_addToExtraCurrent(src, src->current - newExtensionLen, newExtensionLen, status);
                        }
                        newExtensionLen++;
                    }

                    wasInQuote = TRUE;

                    ch = *(++(src->current));
                    if(ch == 0x0027) { /* copy the double quote */
                        ucol_tok_addToExtraCurrent(src, &ch, 1, status);
                        inQuote = FALSE;
                    }
                    break;

                    /* '@' is french only if the strength is not currently set */
                    /* if it is, it's just a regular character in collation rules */
                case 0x0040/*'@'*/:
                    if (newStrength == UCOL_TOK_UNSET) {
                        src->opts->frenchCollation = UCOL_ON;
                        break;
                    }

                case 0x007C /*|*/: /* this means we have actually been reading prefix part */
                    // we want to store read characters to the prefix part and continue reading
                    // the characters (proper way would be to restart reading the chars, but in
                    // that case we would have to complicate the token hasher, which I do not
                    // intend to play with. Instead, we will do prefixes when prefixes are due
                    // (before adding the elements).
                    src->parsedToken.prefixOffset = src->parsedToken.charsOffset;
                    src->parsedToken.prefixLen = src->parsedToken.charsLen;

                    if(inChars) { /* we're doing characters */
                        if(wasInQuote == FALSE) {
                            src->parsedToken.charsOffset = (uint32_t)(src->extraCurrent - src->source);
                        }
                        if (src->parsedToken.charsLen != 0) {
                            ucol_tok_addToExtraCurrent(src, src->current - src->parsedToken.charsLen, src->parsedToken.charsLen, status);
                        }
                        src->parsedToken.charsLen++;
                    }

                    wasInQuote = TRUE;

                    do {
                        ch = *(++(src->current));
                        // skip whitespace between '|' and the character
                    } while (uprv_isRuleWhiteSpace(ch));
                    break;

                    //charsOffset = 0;
                    //newCharsLen = 0;
                    //break; // We want to store the whole prefix/character sequence. If we break
                    // the '|' is going to get lost.

                case 0x002D /*-*/: /* A range. */
                    if (newStrength != UCOL_TOK_UNSET) {
                      // While processing the pending token, the isStarred field
                      // is reset, so it needs to be saved for the next
                      // invocation.
                      src->savedIsStarred = src->isStarred;
                      goto EndOfLoop;
                   }
                   src->isStarred = src->savedIsStarred;

                   // Ranges are valid only in starred tokens.
                   if (!src->isStarred) {
                     *status = U_INVALID_FORMAT_ERROR;
                     syntaxError(src->source,(int32_t)(src->current-src->source),(int32_t)(src->end-src->source),parseError);
                     DBG_FORMAT_ERROR
                     return NULL;
                   }
                   newStrength = src->parsedToken.strength;
                   src->inRange = TRUE;
                   break;

                case 0x0023 /*#*/: /* this is a comment, skip everything through the end of line */
                    do {
                        ch = *(++(src->current));
                    } while (!isCharNewLine(ch));

                    break;
                default:
                    if (newStrength == UCOL_TOK_UNSET) {
                      *status = U_INVALID_FORMAT_ERROR;
                      syntaxError(src->source,(int32_t)(src->current-src->source),(int32_t)(src->end-src->source),parseError);
                      DBG_FORMAT_ERROR
                      return NULL;
                    }

                    if (ucol_tok_isSpecialChar(ch) && (inQuote == FALSE)) {
                        *status = U_INVALID_FORMAT_ERROR;
                        syntaxError(src->source,(int32_t)(src->current-src->source),(int32_t)(src->end-src->source),parseError);
                        DBG_FORMAT_ERROR
                        return NULL;
                    }

                    if(ch == 0x0000 && src->current+1 == src->end) {
                        break;
                    }

                    if (inChars) {
                        if(src->parsedToken.charsLen == 0) {
                            src->parsedToken.charsOffset = (uint32_t)(src->current - src->source);
                        }
                        src->parsedToken.charsLen++;
                    } else {
                        if(newExtensionLen == 0) {
                            extensionOffset = (uint32_t)(src->current - src->source);
                        }
                        newExtensionLen++;
                    }

                    break;
                }
            }
        }

        if(wasInQuote) {
            if(ch != 0x27) {
                if(inQuote || !uprv_isRuleWhiteSpace(ch)) {
                    ucol_tok_addToExtraCurrent(src, &ch, 1, status);
                }
            }
        }

        src->current++;
    }

EndOfLoop:
    wasInQuote = FALSE;
    if (newStrength == UCOL_TOK_UNSET) {
        return NULL;
    }

    if (src->parsedToken.charsLen == 0 && top == FALSE) {
        syntaxError(src->source,(int32_t)(src->current-src->source),(int32_t)(src->end-src->source),parseError);
        *status = U_INVALID_FORMAT_ERROR;
        DBG_FORMAT_ERROR
        return NULL;
    }

    src->parsedToken.strength = newStrength;
    src->parsedToken.extensionOffset = extensionOffset;
    src->parsedToken.extensionLen = newExtensionLen;
    src->parsedToken.flags = (UCOL_TOK_VARIABLE_TOP * (variableTop?1:0)) | (UCOL_TOK_TOP * (top?1:0)) | before;

    return src->current;
}

/*
 * Parses the next token, keeps the indices in src->parsedToken, and updates the counters.
 * @see ucol_tok_parseNextTokenInternal() for the description of what operators are supported.
 *
 * In addition to what ucol_tok_parseNextTokenInternal() does, this function does the following:
 *  1) ucol_tok_parseNextTokenInternal() returns a range as a single token.  This function separates
 *     it to separate tokens and returns one by one.  In order to do that, the necessary states are
 *     cached as member variables of the token parser.
 *  2) When encountering a range, ucol_tok_parseNextTokenInternal() processes characters up to the
 *     starting character as a single list token (which is separated into individual characters here)
 *     and as another list token starting with the last character in the range.  Before expanding it
 *     as a list of tokens, this function expands the range by filling the intermediate characters and
 *     returns them one by one as separate tokens.
 * Necessary checks are done for invalid combinations.
 */
U_CAPI const UChar* U_EXPORT2
ucol_tok_parseNextToken(UColTokenParser *src,
                        UBool startOfRules,
                        UParseError *parseError,
                        UErrorCode *status)
{
  const UChar *nextToken;

  if (src->inRange) {
    // We are not done processing a range.  Continue it.
    return ucol_tok_processNextCodePointInRange(src, status);
  } else if (src->isStarred) {
    // We are not done processing a starred token.  Continue it.
    return ucol_tok_processNextTokenInStarredList(src);
  }

  // Get the next token.
  nextToken = ucol_tok_parseNextTokenInternal(src, startOfRules, parseError, status);

  if (nextToken == NULL) {
    return NULL;
  }

  if (src->inRange) {
    // A new range has started.
    // Check whether it is a chain of ranges with more than one hyphen.
    if (src->lastRangeCp > 0 && src->lastRangeCp == src->previousCp) {
        *status = U_INVALID_FORMAT_ERROR;
        syntaxError(src->source,src->parsedToken.charsOffset-1,
                    src->parsedToken.charsOffset+src->parsedToken.charsLen, parseError);
        DBG_FORMAT_ERROR
        return NULL;
    }

    // The current token indicates the second code point of the range.
    // Process just that, and then proceed with the star.
    src->currentStarredCharIndex = src->parsedToken.charsOffset;
    U16_NEXT(src->source, src->currentStarredCharIndex, 
             (uint32_t)(src->end - src->source), src->lastRangeCp);
    if (src->lastRangeCp <= src->previousCp) {
        *status = U_INVALID_FORMAT_ERROR;
        syntaxError(src->source,src->parsedToken.charsOffset-1,
                    src->parsedToken.charsOffset+src->parsedToken.charsLen,parseError);
        DBG_FORMAT_ERROR
        return NULL;
    }

    // Set current range code point to process the range loop
    src->currentRangeCp = src->previousCp + 1;

    src->lastStarredCharIndex = src->parsedToken.charsOffset + src->parsedToken.charsLen - 1;

    return ucol_tok_processNextCodePointInRange(src, status);
 } else if (src->isStarred) {
    // We define two indices m_currentStarredCharIndex_ and m_lastStarredCharIndex_ so that
    // [m_currentStarredCharIndex_ .. m_lastStarredCharIndex_], both inclusive, need to be
    // separated into several tokens and returned.
    src->currentStarredCharIndex = src->parsedToken.charsOffset;
    src->lastStarredCharIndex =  src->parsedToken.charsOffset + src->parsedToken.charsLen - 1;

    return ucol_tok_processNextTokenInStarredList(src);
  } else {
    // Set previous codepoint
    U16_GET(src->source, 0, src->parsedToken.charsOffset, (uint32_t)(src->end - src->source), src->previousCp);
  }
  return nextToken;
}


/*
Processing Description
1 Build a ListList. Each list has a header, which contains two lists (positive
and negative), a reset token, a baseCE, nextCE, and previousCE. The lists and
reset may be null.
2 As you process, you keep a LAST pointer that points to the last token you
handled.

*/

static UColToken *ucol_tok_initAReset(UColTokenParser *src, const UChar *expand, uint32_t *expandNext,
                                      UParseError *parseError, UErrorCode *status)
{
    if(src->resultLen == src->listCapacity) {
        // Unfortunately, this won't work, as we store addresses of lhs in token
        src->listCapacity *= 2;
        src->lh = (UColTokListHeader *)uprv_realloc(src->lh, src->listCapacity*sizeof(UColTokListHeader));
        if(src->lh == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            return NULL;
        }
    }
    /* do the reset thing */
    UColToken *sourceToken = (UColToken *)uprv_malloc(sizeof(UColToken));
    /* test for NULL */
    if (sourceToken == NULL) {
        *status = U_MEMORY_ALLOCATION_ERROR;
        return NULL;
    }
    sourceToken->rulesToParseHdl = &(src->source);
    sourceToken->source = src->parsedToken.charsLen << 24 | src->parsedToken.charsOffset;
    sourceToken->expansion = src->parsedToken.extensionLen << 24 | src->parsedToken.extensionOffset;

    sourceToken->debugSource = *(src->source + src->parsedToken.charsOffset);
    sourceToken->debugExpansion = *(src->source + src->parsedToken.extensionOffset);

    // keep the flags around so that we know about before
    sourceToken->flags = src->parsedToken.flags;

    if(src->parsedToken.prefixOffset != 0) {
        // this is a syntax error
        *status = U_INVALID_FORMAT_ERROR;
        syntaxError(src->source,src->parsedToken.charsOffset-1,src->parsedToken.charsOffset+src->parsedToken.charsLen,parseError);
        DBG_FORMAT_ERROR
        uprv_free(sourceToken);
        return 0;
    } else {
        sourceToken->prefix = 0;
    }

    sourceToken->polarity = UCOL_TOK_POLARITY_POSITIVE; /* TODO: this should also handle reverse */
    sourceToken->strength = UCOL_TOK_RESET;
    sourceToken->next = NULL;
    sourceToken->previous = NULL;
    sourceToken->noOfCEs = 0;
    sourceToken->noOfExpCEs = 0;
    sourceToken->listHeader = &src->lh[src->resultLen];

    src->lh[src->resultLen].first = NULL;
    src->lh[src->resultLen].last = NULL;
    src->lh[src->resultLen].first = NULL;
    src->lh[src->resultLen].last = NULL;

    src->lh[src->resultLen].reset = sourceToken;

    /*
    3 Consider each item: relation, source, and expansion: e.g. ...< x / y ...
    First convert all expansions into normal form. Examples:
    If "xy" doesn't occur earlier in the list or in the UCA, convert &xy * c *
    d * ... into &x * c/y * d * ...
    Note: reset values can never have expansions, although they can cause the
    very next item to have one. They may be contractions, if they are found
    earlier in the list.
    */
    *expandNext = 0;
    if(expand != NULL) {
        /* check to see if there is an expansion */
        if(src->parsedToken.charsLen > 1) {
            uint32_t resetCharsOffset;
            resetCharsOffset = (uint32_t)(expand - src->source);
            sourceToken->source = ((resetCharsOffset - src->parsedToken.charsOffset ) << 24) | src->parsedToken.charsOffset;
            *expandNext = ((src->parsedToken.charsLen + src->parsedToken.charsOffset - resetCharsOffset)<<24) | (resetCharsOffset);
        }
    }

    src->resultLen++;

    uhash_put(src->tailored, sourceToken, sourceToken, status);

    return sourceToken;
}

static
inline UColToken *getVirginBefore(UColTokenParser *src, UColToken *sourceToken, uint8_t strength, UParseError *parseError, UErrorCode *status) {
    if(U_FAILURE(*status)) {
        return NULL;
    }
    /* this is a virgin before - we need to fish the anchor from the UCA */
    collIterate s;
    uint32_t baseCE = UCOL_NOT_FOUND, baseContCE = UCOL_NOT_FOUND;
    uint32_t CE, SecondCE;
    uint32_t invPos;
    if(sourceToken != NULL) {
        uprv_init_collIterate(src->UCA, src->source+((sourceToken->source)&0xFFFFFF), 1, &s, status);
    } else {
        uprv_init_collIterate(src->UCA, src->source+src->parsedToken.charsOffset /**charsOffset*/, 1, &s, status);
    }
    if(U_FAILURE(*status)) {
        return NULL;
    }

    baseCE = ucol_getNextCE(src->UCA, &s, status) & 0xFFFFFF3F;
    baseContCE = ucol_getNextCE(src->UCA, &s, status);
    if(baseContCE == UCOL_NO_MORE_CES) {
        baseContCE = 0;
    }


    UCAConstants *consts = (UCAConstants *)((uint8_t *)src->UCA->image + src->UCA->image->UCAConsts);
    uint32_t ch = 0;
    uint32_t expandNext = 0;
    UColToken key;

    if((baseCE & 0xFF000000) >= (consts->UCA_PRIMARY_IMPLICIT_MIN<<24) && (baseCE & 0xFF000000) <= (consts->UCA_PRIMARY_IMPLICIT_MAX<<24) ) { /* implicits - */
        uint32_t primary = (baseCE & UCOL_PRIMARYMASK) | ((baseContCE & UCOL_PRIMARYMASK) >> 16);
        uint32_t raw = uprv_uca_getRawFromImplicit(primary);
        ch = uprv_uca_getCodePointFromRaw(raw-1);
        uint32_t primaryCE = uprv_uca_getImplicitFromRaw(raw-1);
        CE = (primaryCE & UCOL_PRIMARYMASK) | 0x0505;
        SecondCE = ((primaryCE << 16) & UCOL_PRIMARYMASK) | UCOL_CONTINUATION_MARKER;

        src->parsedToken.charsOffset = (uint32_t)(src->extraCurrent - src->source);
        *src->extraCurrent++ = 0xFFFE;
        *src->extraCurrent++ = (UChar)ch;
        src->parsedToken.charsLen++;

        key.source = (src->parsedToken.charsLen/**newCharsLen*/ << 24) | src->parsedToken.charsOffset/**charsOffset*/;
        key.rulesToParseHdl = &(src->source);

        //sourceToken = (UColToken *)uhash_iget(src->tailored, (int32_t)key);
        sourceToken = (UColToken *)uhash_get(src->tailored, &key);

        if(sourceToken == NULL) {
            src->lh[src->resultLen].baseCE = CE & 0xFFFFFF3F;
            if(isContinuation(SecondCE)) {
                src->lh[src->resultLen].baseContCE = SecondCE;
            } else {
                src->lh[src->resultLen].baseContCE = 0;
            }
            src->lh[src->resultLen].nextCE = 0;
            src->lh[src->resultLen].nextContCE = 0;
            src->lh[src->resultLen].previousCE = 0;
            src->lh[src->resultLen].previousContCE = 0;

            src->lh[src->resultLen].indirect = FALSE;

            sourceToken = ucol_tok_initAReset(src, 0, &expandNext, parseError, status);
        }

    } else {
        invPos = ucol_inv_getPrevCE(src, baseCE, baseContCE, &CE, &SecondCE, strength);

        // we got the previous CE. Now we need to see if the difference between
        // the two CEs is really of the requested strength.
        // if it's a bigger difference (we asked for secondary and got primary), we
        // need to modify the CE.
        if(ucol_getCEStrengthDifference(baseCE, baseContCE, CE, SecondCE) < strength) {
            // adjust the strength
            // now we are in the situation where our baseCE should actually be modified in
            // order to get the CE in the right position.
            if(strength == UCOL_SECONDARY) {
                CE = baseCE - 0x0200;
            } else { // strength == UCOL_TERTIARY
                CE = baseCE - 0x02;
            }
            if(baseContCE) {
                if(strength == UCOL_SECONDARY) {
                    SecondCE = baseContCE - 0x0200;
                } else { // strength == UCOL_TERTIARY
                    SecondCE = baseContCE - 0x02;
                }
            }
        }

#if 0
        // the code below relies on getting a code point from the inverse table, in order to be
        // able to merge the situations like &x < 9 &[before 1]a < d. This won't work:
        // 1. There are many code points that have the same CE
        // 2. The CE to codepoint table (things pointed to by CETable[3*invPos+2] are broken.
        // Also, in case when there is no equivalent strength before an element, we have to actually
        // construct one. For example, &[before 2]a << x won't result in x << a, because the element
        // before a is a primary difference.

        //uint32_t *CETable = (uint32_t *)((uint8_t *)src->invUCA+src->invUCA->table);


        ch = CETable[3*invPos+2];

        if((ch &  UCOL_INV_SIZEMASK) != 0) {
            uint16_t *conts = (uint16_t *)((uint8_t *)src->invUCA+src->invUCA->conts);
            uint32_t offset = (ch & UCOL_INV_OFFSETMASK);
            ch = conts[offset];
        }

        *src->extraCurrent++ = (UChar)ch;
        src->parsedToken.charsOffset = (uint32_t)(src->extraCurrent - src->source - 1);
        src->parsedToken.charsLen = 1;

        // We got an UCA before. However, this might have been tailored.
        // example:
        // &\u30ca = \u306a
        // &[before 3]\u306a<<<\u306a|\u309d


        // uint32_t key = (*newCharsLen << 24) | *charsOffset;
        key.source = (src->parsedToken.charsLen/**newCharsLen*/ << 24) | src->parsedToken.charsOffset/**charsOffset*/;
        key.rulesToParseHdl = &(src->source);

        //sourceToken = (UColToken *)uhash_iget(src->tailored, (int32_t)key);
        sourceToken = (UColToken *)uhash_get(src->tailored, &key);
#endif

        // here is how it should be. The situation such as &[before 1]a < x, should be
        // resolved exactly as if we wrote &a > x.
        // therefore, I don't really care if the UCA value before a has been changed.
        // However, I do care if the strength between my element and the previous element
        // is bigger then I wanted. So, if CE < baseCE and I wanted &[before 2], then i'll
        // have to construct the base CE.



        // if we found a tailored thing, we have to use the UCA value and construct
        // a new reset token with constructed name
        //if(sourceToken != NULL && sourceToken->strength != UCOL_TOK_RESET) {
        // character to which we want to anchor is already tailored.
        // We need to construct a new token which will be the anchor
        // point
        //*(src->extraCurrent-1) = 0xFFFE;
        //*src->extraCurrent++ = (UChar)ch;
        // grab before
        src->parsedToken.charsOffset -= 10;
        src->parsedToken.charsLen += 10;
        src->lh[src->resultLen].baseCE = CE & 0xFFFFFF3F;
        if(isContinuation(SecondCE)) {
            src->lh[src->resultLen].baseContCE = SecondCE;
        } else {
            src->lh[src->resultLen].baseContCE = 0;
        }
        src->lh[src->resultLen].nextCE = 0;
        src->lh[src->resultLen].nextContCE = 0;
        src->lh[src->resultLen].previousCE = 0;
        src->lh[src->resultLen].previousContCE = 0;

        src->lh[src->resultLen].indirect = FALSE;

        sourceToken = ucol_tok_initAReset(src, 0, &expandNext, parseError, status);
        //}
    }

    return sourceToken;

}

uint32_t ucol_tok_assembleTokenList(UColTokenParser *src, UParseError *parseError, UErrorCode *status) {
    UColToken *lastToken = NULL;
    const UChar *parseEnd = NULL;
    uint32_t expandNext = 0;
    UBool variableTop = FALSE;
    UBool top = FALSE;
    uint16_t specs = 0;
    UColTokListHeader *ListList = NULL;

    src->parsedToken.strength = UCOL_TOK_UNSET;

    ListList = src->lh;

    if(U_FAILURE(*status)) {
        return 0;
    }
#ifdef DEBUG_FOR_CODE_POINTS
    char filename[35];
    sprintf(filename, "/tmp/debug_for_cp_%09d.txt", getpid());
    dfcp_fp = fopen(filename, "a");
    fprintf(stdout, "Output is in the file %s.\n", filename);
#endif

#ifdef DEBUG_FOR_COLL_RULES
    std::string s3;
    UnicodeString(src->source).toUTF8String(s3);
    std::cout << "src->source = " << s3 << std::endl;
#endif

    while(src->current < src->end || src->isStarred) {
        src->parsedToken.prefixOffset = 0;

        parseEnd = ucol_tok_parseNextToken(src,
            (UBool)(lastToken == NULL),
            parseError,
            status);

        specs = src->parsedToken.flags;


        variableTop = ((specs & UCOL_TOK_VARIABLE_TOP) != 0);
        top = ((specs & UCOL_TOK_TOP) != 0);

        if(U_SUCCESS(*status) && parseEnd != NULL) {
            UColToken *sourceToken = NULL;
            //uint32_t key = 0;
            uint32_t lastStrength = UCOL_TOK_UNSET;

            if(lastToken != NULL ) {
                lastStrength = lastToken->strength;
            }

#ifdef DEBUG_FOR_CODE_POINTS
            UChar32 cp;
            U16_GET(src->source, 0, src->parsedToken.charsOffset, (uint32_t)(src->extraEnd - src->source), cp);
            fprintf(dfcp_fp, "Code point = %x, Strength = %x\n", cp, src->parsedToken.strength);
#endif
            //key = newCharsLen << 24 | charsOffset;
            UColToken key;
            key.source = src->parsedToken.charsLen << 24 | src->parsedToken.charsOffset;
            key.rulesToParseHdl = &(src->source);

            /*  4 Lookup each source in the CharsToToken map, and find a sourceToken */
            sourceToken = (UColToken *)uhash_get(src->tailored, &key);

            if(src->parsedToken.strength != UCOL_TOK_RESET) {
                if(lastToken == NULL) { /* this means that rules haven't started properly */
                    *status = U_INVALID_FORMAT_ERROR;
                    syntaxError(src->source,0,(int32_t)(src->end-src->source),parseError);
                    DBG_FORMAT_ERROR
                    return 0;
                }
                /*  6 Otherwise (when relation != reset) */
                if(sourceToken == NULL) {
                    /* If sourceToken is null, create new one, */
                    sourceToken = (UColToken *)uprv_malloc(sizeof(UColToken));
                    /* test for NULL */
                    if (sourceToken == NULL) {
                        *status = U_MEMORY_ALLOCATION_ERROR;
                        return 0;
                    }
                    sourceToken->rulesToParseHdl = &(src->source);
                    sourceToken->source = src->parsedToken.charsLen << 24 | src->parsedToken.charsOffset;

                    sourceToken->debugSource = *(src->source + src->parsedToken.charsOffset);

                    sourceToken->prefix = src->parsedToken.prefixLen << 24 | src->parsedToken.prefixOffset;
                    sourceToken->debugPrefix = *(src->source + src->parsedToken.prefixOffset);

                    sourceToken->polarity = UCOL_TOK_POLARITY_POSITIVE; /* TODO: this should also handle reverse */
                    sourceToken->next = NULL;
                    sourceToken->previous = NULL;
                    sourceToken->noOfCEs = 0;
                    sourceToken->noOfExpCEs = 0;
                    // keep the flags around so that we know about before
                    sourceToken->flags = src->parsedToken.flags;
                    uhash_put(src->tailored, sourceToken, sourceToken, status);
                    if(U_FAILURE(*status)) {
                        return 0;
                    }
                } else {
                    /* we could have fished out a reset here */
                    if(sourceToken->strength != UCOL_TOK_RESET && lastToken != sourceToken) {
                        /* otherwise remove sourceToken from where it was. */
                        if(sourceToken->next != NULL) {
                            if(sourceToken->next->strength > sourceToken->strength) {
                                sourceToken->next->strength = sourceToken->strength;
                            }
                            sourceToken->next->previous = sourceToken->previous;
                        } else {
                            sourceToken->listHeader->last = sourceToken->previous;
                        }

                        if(sourceToken->previous != NULL) {
                            sourceToken->previous->next = sourceToken->next;
                        } else {
                            sourceToken->listHeader->first = sourceToken->next;
                        }
                        sourceToken->next = NULL;
                        sourceToken->previous = NULL;
                    }
                }

                sourceToken->strength = src->parsedToken.strength;
                sourceToken->listHeader = lastToken->listHeader;

                /*
                1.  Find the strongest strength in each list, and set strongestP and strongestN
                accordingly in the headers.
                */
                if(lastStrength == UCOL_TOK_RESET
                    || sourceToken->listHeader->first == 0) {
                        /* If LAST is a reset
                        insert sourceToken in the list. */
                        if(sourceToken->listHeader->first == 0) {
                            sourceToken->listHeader->first = sourceToken;
                            sourceToken->listHeader->last = sourceToken;
                        } else { /* we need to find a place for us */
                            /* and we'll get in front of the same strength */
                            if(sourceToken->listHeader->first->strength <= sourceToken->strength) {
                                sourceToken->next = sourceToken->listHeader->first;
                                sourceToken->next->previous = sourceToken;
                                sourceToken->listHeader->first = sourceToken;
                                sourceToken->previous = NULL;
                            } else {
                                lastToken = sourceToken->listHeader->first;
                                while(lastToken->next != NULL && lastToken->next->strength > sourceToken->strength) {
                                    lastToken = lastToken->next;
                                }
                                if(lastToken->next != NULL) {
                                    lastToken->next->previous = sourceToken;
                                } else {
                                    sourceToken->listHeader->last = sourceToken;
                                }
                                sourceToken->previous = lastToken;
                                sourceToken->next = lastToken->next;
                                lastToken->next = sourceToken;
                            }
                        }
                    } else {
                        /* Otherwise (when LAST is not a reset)
                        if polarity (LAST) == polarity(relation), insert sourceToken after LAST,
                        otherwise insert before.
                        when inserting after or before, search to the next position with the same
                        strength in that direction. (This is called postpone insertion).         */
                        if(sourceToken != lastToken) {
                            if(lastToken->polarity == sourceToken->polarity) {
                                while(lastToken->next != NULL && lastToken->next->strength > sourceToken->strength) {
                                    lastToken = lastToken->next;
                                }
                                sourceToken->previous = lastToken;
                                if(lastToken->next != NULL) {
                                    lastToken->next->previous = sourceToken;
                                } else {
                                    sourceToken->listHeader->last = sourceToken;
                                }

                                sourceToken->next = lastToken->next;
                                lastToken->next = sourceToken;
                            } else {
                                while(lastToken->previous != NULL && lastToken->previous->strength > sourceToken->strength) {
                                    lastToken = lastToken->previous;
                                }
                                sourceToken->next = lastToken;
                                if(lastToken->previous != NULL) {
                                    lastToken->previous->next = sourceToken;
                                } else {
                                    sourceToken->listHeader->first = sourceToken;
                                }
                                sourceToken->previous = lastToken->previous;
                                lastToken->previous = sourceToken;
                            }
                        } else { /* repeated one thing twice in rules, stay with the stronger strength */
                            if(lastStrength < sourceToken->strength) {
                                sourceToken->strength = lastStrength;
                            }
                        }
                    }

                    /* if the token was a variable top, we're gonna put it in */
                    if(variableTop == TRUE && src->varTop == NULL) {
                        variableTop = FALSE;
                        src->varTop = sourceToken;
                    }

                    // Treat the expansions.
                    // There are two types of expansions: explicit (x / y) and reset based propagating expansions
                    // (&abc * d * e <=> &ab * d / c * e / c)
                    // if both of them are in effect for a token, they are combined.

                    sourceToken->expansion = src->parsedToken.extensionLen << 24 | src->parsedToken.extensionOffset;

                    if(expandNext != 0) {
                        if(sourceToken->strength == UCOL_PRIMARY) { /* primary strength kills off the implicit expansion */
                            expandNext = 0;
                        } else if(sourceToken->expansion == 0) { /* if there is no expansion, implicit is just added to the token */
                            sourceToken->expansion = expandNext;
                        } else { /* there is both explicit and implicit expansion. We need to make a combination */
                            uprv_memcpy(src->extraCurrent, src->source + (expandNext & 0xFFFFFF), (expandNext >> 24)*sizeof(UChar));
                            uprv_memcpy(src->extraCurrent+(expandNext >> 24), src->source + src->parsedToken.extensionOffset, src->parsedToken.extensionLen*sizeof(UChar));
                            sourceToken->expansion = (uint32_t)(((expandNext >> 24) + src->parsedToken.extensionLen)<<24 | (uint32_t)(src->extraCurrent - src->source));
                            src->extraCurrent += (expandNext >> 24) + src->parsedToken.extensionLen;
                        }
                    }

                    // This is just for debugging purposes
                    if(sourceToken->expansion != 0) {
                        sourceToken->debugExpansion = *(src->source + src->parsedToken.extensionOffset);
                    } else {
                        sourceToken->debugExpansion = 0;
                    }
                    // if the previous token was a reset before, the strength of this
                    // token must match the strength of before. Otherwise we have an
                    // undefined situation.
                    // In other words, we currently have a cludge which we use to
                    // represent &a >> x. This is written as &[before 2]a << x.
                    if((lastToken->flags & UCOL_TOK_BEFORE) != 0) {
                        uint8_t beforeStrength = (lastToken->flags & UCOL_TOK_BEFORE) - 1;
                        if(beforeStrength != sourceToken->strength) {
                            *status = U_INVALID_FORMAT_ERROR;
                            syntaxError(src->source,0,(int32_t)(src->end-src->source),parseError);
                            DBG_FORMAT_ERROR
                            return 0;
                        }
                    }
            } else {
                if(lastToken != NULL && lastStrength == UCOL_TOK_RESET) {
                    /* if the previous token was also a reset, */
                    /*this means that we have two consecutive resets */
                    /* and we want to remove the previous one if empty*/
                    if(src->resultLen > 0 && ListList[src->resultLen-1].first == NULL) {
                        src->resultLen--;
                    }
                }

                if(sourceToken == NULL) { /* this is a reset, but it might still be somewhere in the tailoring, in shorter form */
                    uint32_t searchCharsLen = src->parsedToken.charsLen;
                    while(searchCharsLen > 1 && sourceToken == NULL) {
                        searchCharsLen--;
                        //key = searchCharsLen << 24 | charsOffset;
                        UColToken key;
                        key.source = searchCharsLen << 24 | src->parsedToken.charsOffset;
                        key.rulesToParseHdl = &(src->source);
                        sourceToken = (UColToken *)uhash_get(src->tailored, &key);
                    }
                    if(sourceToken != NULL) {
                        expandNext = (src->parsedToken.charsLen - searchCharsLen) << 24 | (src->parsedToken.charsOffset + searchCharsLen);
                    }
                }

                if((specs & UCOL_TOK_BEFORE) != 0) { /* we're doing before */
                    if(top == FALSE) { /* there is no indirection */
                        uint8_t strength = (specs & UCOL_TOK_BEFORE) - 1;
                        if(sourceToken != NULL && sourceToken->strength != UCOL_TOK_RESET) {
                            /* this is a before that is already ordered in the UCA - so we need to get the previous with good strength */
                            while(sourceToken->strength > strength && sourceToken->previous != NULL) {
                                sourceToken = sourceToken->previous;
                            }
                            /* here, either we hit the strength or NULL */
                            if(sourceToken->strength == strength) {
                                if(sourceToken->previous != NULL) {
                                    sourceToken = sourceToken->previous;
                                } else { /* start of list */
                                    sourceToken = sourceToken->listHeader->reset;
                                }
                            } else { /* we hit NULL */
                                /* we should be doing the else part */
                                sourceToken = sourceToken->listHeader->reset;
                                sourceToken = getVirginBefore(src, sourceToken, strength, parseError, status);
                            }
                        } else {
                            sourceToken = getVirginBefore(src, sourceToken, strength, parseError, status);
                        }
                    } else { /* this is both before and indirection */
                        top = FALSE;
                        ListList[src->resultLen].previousCE = 0;
                        ListList[src->resultLen].previousContCE = 0;
                        ListList[src->resultLen].indirect = TRUE;
                        /* we need to do slightly more work. we need to get the baseCE using the */
                        /* inverse UCA & getPrevious. The next bound is not set, and will be decided */
                        /* in ucol_bld */
                        uint8_t strength = (specs & UCOL_TOK_BEFORE) - 1;
                        uint32_t baseCE = ucolIndirectBoundaries[src->parsedToken.indirectIndex].startCE;
                        uint32_t baseContCE = ucolIndirectBoundaries[src->parsedToken.indirectIndex].startContCE;//&0xFFFFFF3F;
                        uint32_t CE = UCOL_NOT_FOUND, SecondCE = UCOL_NOT_FOUND;

                        UCAConstants *consts = (UCAConstants *)((uint8_t *)src->UCA->image + src->UCA->image->UCAConsts);
                        if((baseCE & 0xFF000000) >= (consts->UCA_PRIMARY_IMPLICIT_MIN<<24) && 
                           (baseCE & 0xFF000000) <= (consts->UCA_PRIMARY_IMPLICIT_MAX<<24) ) { /* implicits - */
                            uint32_t primary = (baseCE & UCOL_PRIMARYMASK) | ((baseContCE & UCOL_PRIMARYMASK) >> 16);
                            uint32_t raw = uprv_uca_getRawFromImplicit(primary);
                            uint32_t primaryCE = uprv_uca_getImplicitFromRaw(raw-1);
                            CE = (primaryCE & UCOL_PRIMARYMASK) | 0x0505;
                            SecondCE = ((primaryCE << 16) & UCOL_PRIMARYMASK) | UCOL_CONTINUATION_MARKER;
                        } else {
                            /*int32_t invPos = ucol_inv_getPrevCE(baseCE, baseContCE, &CE, &SecondCE, strength);*/
                            ucol_inv_getPrevCE(src, baseCE, baseContCE, &CE, &SecondCE, strength);
                        }

                        ListList[src->resultLen].baseCE = CE;
                        ListList[src->resultLen].baseContCE = SecondCE;
                        ListList[src->resultLen].nextCE = 0;
                        ListList[src->resultLen].nextContCE = 0;

                        sourceToken = ucol_tok_initAReset(src, 0, &expandNext, parseError, status);
                    }
                }


                /*  5 If the relation is a reset:
                If sourceToken is null
                Create new list, create new sourceToken, make the baseCE from source, put
                the sourceToken in ListHeader of the new list */
                if(sourceToken == NULL) {
                    /*
                    3 Consider each item: relation, source, and expansion: e.g. ...< x / y ...
                    First convert all expansions into normal form. Examples:
                    If "xy" doesn't occur earlier in the list or in the UCA, convert &xy * c *
                    d * ... into &x * c/y * d * ...
                    Note: reset values can never have expansions, although they can cause the
                    very next item to have one. They may be contractions, if they are found
                    earlier in the list.
                    */
                    if(top == FALSE) {
                        collIterate s;
                        uint32_t CE = UCOL_NOT_FOUND, SecondCE = UCOL_NOT_FOUND;

                        uprv_init_collIterate(src->UCA, src->source+src->parsedToken.charsOffset, src->parsedToken.charsLen, &s, status);

                        CE = ucol_getNextCE(src->UCA, &s, status);
                        const UChar *expand = s.pos;
                        SecondCE = ucol_getNextCE(src->UCA, &s, status);

                        ListList[src->resultLen].baseCE = CE & 0xFFFFFF3F;
                        if(isContinuation(SecondCE)) {
                            ListList[src->resultLen].baseContCE = SecondCE;
                        } else {
                            ListList[src->resultLen].baseContCE = 0;
                        }
                        ListList[src->resultLen].nextCE = 0;
                        ListList[src->resultLen].nextContCE = 0;
                        ListList[src->resultLen].previousCE = 0;
                        ListList[src->resultLen].previousContCE = 0;
                        ListList[src->resultLen].indirect = FALSE;
                        sourceToken = ucol_tok_initAReset(src, expand, &expandNext, parseError, status);
                    } else { /* top == TRUE */
                        /* just use the supplied values */
                        top = FALSE;
                        ListList[src->resultLen].previousCE = 0;
                        ListList[src->resultLen].previousContCE = 0;
                        ListList[src->resultLen].indirect = TRUE;
                        ListList[src->resultLen].baseCE = ucolIndirectBoundaries[src->parsedToken.indirectIndex].startCE;
                        ListList[src->resultLen].baseContCE = ucolIndirectBoundaries[src->parsedToken.indirectIndex].startContCE;
                        ListList[src->resultLen].nextCE = ucolIndirectBoundaries[src->parsedToken.indirectIndex].limitCE;
                        ListList[src->resultLen].nextContCE = ucolIndirectBoundaries[src->parsedToken.indirectIndex].limitContCE;

                        sourceToken = ucol_tok_initAReset(src, 0, &expandNext, parseError, status);

                    }
                } else { /* reset to something already in rules */
                    top = FALSE;
                }
            }
            /*  7 After all this, set LAST to point to sourceToken, and goto step 3. */
            lastToken = sourceToken;
        } else {
            if(U_FAILURE(*status)) {
                return 0;
            }
        }
    }
#ifdef DEBUG_FOR_CODE_POINTS
    fclose(dfcp_fp);
#endif


    if(src->resultLen > 0 && ListList[src->resultLen-1].first == NULL) {
        src->resultLen--;
    }
    return src->resultLen;
}

const UChar* ucol_tok_getRulesFromBundle(
    void* /*context*/,
    const char* locale,
    const char* type,
    int32_t* pLength,
    UErrorCode* status)
{
    const UChar* rules = NULL;
    UResourceBundle* bundle;
    UResourceBundle* collations;
    UResourceBundle* collation;

    *pLength = 0;

    bundle = ures_open(U_ICUDATA_COLL, locale, status);
    if(U_SUCCESS(*status)){
        collations = ures_getByKey(bundle, "collations", NULL, status);
        if(U_SUCCESS(*status)){
            collation = ures_getByKey(collations, type, NULL, status);
            if(U_SUCCESS(*status)){
                rules = ures_getStringByKey(collation, "Sequence", pLength, status);
                if(U_FAILURE(*status)){
                    *pLength = 0;
                    rules = NULL;
                }
                ures_close(collation);
            }
            ures_close(collations);
        }
    }

    ures_close(bundle);

    return rules;
}

void ucol_tok_initTokenList(
    UColTokenParser *src,
    const UChar *rules,
    uint32_t rulesLength,
    const UCollator *UCA,
    GetCollationRulesFunction importFunc,
    void* context, 
    UErrorCode *status) {
    U_NAMESPACE_USE

    uint32_t nSize = 0;
    uint32_t estimatedSize = (2*rulesLength+UCOL_TOK_EXTRA_RULE_SPACE_SIZE);

    bool needToDeallocRules = false;

    if(U_FAILURE(*status)) {
        return;
    }

    // set everything to zero, so that we can clean up gracefully
    uprv_memset(src, 0, sizeof(UColTokenParser));

    // first we need to find options that don't like to be normalized,
    // like copy and remove...
    //const UChar *openBrace = rules;
    int32_t optionNumber = -1;
    const UChar *setStart = NULL;
    uint32_t i = 0;
    while(i < rulesLength) {
        if(rules[i] == 0x005B) {    // '[': start of an option
            /* Gets the following:
               optionNumber: The index of the option.
               setStart: The pointer at which the option arguments start.
             */
            optionNumber = ucol_uprv_tok_readOption(rules+i+1, rules+rulesLength, &setStart);

            if(optionNumber == OPTION_OPTIMIZE) { /* copy - parts of UCA to tailoring */
                // [optimize]
                USet *newSet = ucol_uprv_tok_readAndSetUnicodeSet(setStart, rules+rulesLength, status);
                if(U_SUCCESS(*status)) {
                    if(src->copySet == NULL) {
                        src->copySet = newSet;
                    } else {
                        uset_addAll(src->copySet, newSet);
                        uset_close(newSet);
                    }
                } else {
                    return;
                }
            } else if(optionNumber == OPTION_SUPPRESS_CONTRACTIONS) {
                USet *newSet = ucol_uprv_tok_readAndSetUnicodeSet(setStart, rules+rulesLength, status);
                if(U_SUCCESS(*status)) {
                    if(src->removeSet == NULL) {
                        src->removeSet = newSet;
                    } else {
                        uset_addAll(src->removeSet, newSet);
                        uset_close(newSet);
                    }
                } else {
                    return;
                }
            } else if(optionNumber == OPTION_IMPORT){
                // [import <collation-name>]

                // Find the address of the closing ].
                UChar* import_end = u_strchr(setStart, 0x005D);
                int32_t optionEndOffset = (int32_t)(import_end + 1 - rules);
                // Ignore trailing whitespace.
                while(uprv_isRuleWhiteSpace(*(import_end-1))) {
                    --import_end;
                }

                int32_t optionLength = (int32_t)(import_end - setStart);
                char option[50];
                if(optionLength >= (int32_t)sizeof(option)) {
                    *status = U_ILLEGAL_ARGUMENT_ERROR;
                    return;
                }
                u_UCharsToChars(setStart, option, optionLength);
                option[optionLength] = 0;

                *status = U_ZERO_ERROR;
                char locale[50];
                int32_t templ;
                uloc_forLanguageTag(option, locale, (int32_t)sizeof(locale), &templ, status);
                if(U_FAILURE(*status)) {
                    *status = U_ILLEGAL_ARGUMENT_ERROR;
                    return;
                }

                char type[50];
                if (uloc_getKeywordValue(locale, "collation", type, (int32_t)sizeof(type), status) <= 0 ||
                    U_FAILURE(*status)
                ) {
                    *status = U_ZERO_ERROR;
                    uprv_strcpy(type, "standard");
                }

                // TODO: Use public functions when available, see ticket #8134.
                char *keywords = (char *)locale_getKeywordsStart(locale);
                if(keywords != NULL) {
                    *keywords = 0;
                }

                int32_t importRulesLength = 0;
                const UChar* importRules = importFunc(context, locale, type, &importRulesLength, status);

#ifdef DEBUG_FOR_COLL_RULES
                std::string s;
                UnicodeString(importRules).toUTF8String(s);
                std::cout << "Import rules = " << s << std::endl;
#endif

                // Add the length of the imported rules to length of the original rules,
                // and subtract the length of the import option.
                uint32_t newRulesLength = rulesLength + importRulesLength - (optionEndOffset - i);

                UChar* newRules = (UChar*)uprv_malloc(newRulesLength*sizeof(UChar));

#ifdef DEBUG_FOR_COLL_RULES
                std::string s1;
                UnicodeString(rules).toUTF8String(s1);
                std::cout << "Original rules = " << s1 << std::endl;
#endif


                // Copy the section of the original rules leading up to the import
                uprv_memcpy(newRules, rules, i*sizeof(UChar));
                // Copy the imported rules
                uprv_memcpy(newRules+i, importRules, importRulesLength*sizeof(UChar));
                // Copy the rest of the original rules (minus the import option itself)
                uprv_memcpy(newRules+i+importRulesLength,
                            rules+optionEndOffset,
                            (rulesLength-optionEndOffset)*sizeof(UChar));

#ifdef DEBUG_FOR_COLL_RULES
                std::string s2;
                UnicodeString(newRules).toUTF8String(s2);
                std::cout << "Resulting rules = " << s2 << std::endl;
#endif

                if(needToDeallocRules){
                    // if needToDeallocRules is set, then we allocated rules, so it's safe to cast and free
                    uprv_free((void*)rules);
                }
                needToDeallocRules = true;
                rules = newRules;
                rulesLength = newRulesLength;

                estimatedSize += importRulesLength*2;

                // First character of the new rules needs to be processed
                i--;
            }
        }
        //openBrace++;
        i++;
    }

    src->source = (UChar *)uprv_malloc(estimatedSize*sizeof(UChar));
    /* test for NULL */
    if (src->source == NULL) {
        *status = U_MEMORY_ALLOCATION_ERROR;
        return;
    }
    uprv_memset(src->source, 0, estimatedSize*sizeof(UChar));
    nSize = unorm_normalize(rules, rulesLength, UNORM_NFD, 0, src->source, estimatedSize, status);
    if(nSize > estimatedSize || *status == U_BUFFER_OVERFLOW_ERROR) {
        *status = U_ZERO_ERROR;
        src->source = (UChar *)uprv_realloc(src->source, (nSize+UCOL_TOK_EXTRA_RULE_SPACE_SIZE)*sizeof(UChar));
        /* test for NULL */
        if (src->source == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            return;
        }
        nSize = unorm_normalize(rules, rulesLength, UNORM_NFD, 0, src->source, nSize+UCOL_TOK_EXTRA_RULE_SPACE_SIZE, status);
    }
    if(needToDeallocRules){
        // if needToDeallocRules is set, then we allocated rules, so it's safe to cast and free
        uprv_free((void*)rules);
    }


    src->current = src->source;
    src->end = src->source+nSize;
    src->sourceCurrent = src->source;
    src->extraCurrent = src->end+1; // Preserve terminating zero in the rule string so that option scanning works correctly
    src->extraEnd = src->source+estimatedSize; //src->end+UCOL_TOK_EXTRA_RULE_SPACE_SIZE;
    src->varTop = NULL;
    src->UCA = UCA;
    src->invUCA = ucol_initInverseUCA(status);
    src->parsedToken.charsLen = 0;
    src->parsedToken.charsOffset = 0;
    src->parsedToken.extensionLen = 0;
    src->parsedToken.extensionOffset = 0;
    src->parsedToken.prefixLen = 0;
    src->parsedToken.prefixOffset = 0;
    src->parsedToken.flags = 0;
    src->parsedToken.strength = UCOL_TOK_UNSET;
    src->buildCCTabFlag = FALSE;
    src->isStarred = FALSE;
    src->inRange = FALSE;
    src->lastRangeCp = 0;
    src->previousCp = 0;

    if(U_FAILURE(*status)) {
        return;
    }
    src->tailored = uhash_open(uhash_hashTokens, uhash_compareTokens, NULL, status);
    if(U_FAILURE(*status)) {
        return;
    }
    uhash_setValueDeleter(src->tailored, uhash_freeBlock);

    src->opts = (UColOptionSet *)uprv_malloc(sizeof(UColOptionSet));
    /* test for NULL */
    if (src->opts == NULL) {
        *status = U_MEMORY_ALLOCATION_ERROR;
        return;
    }

    uprv_memcpy(src->opts, UCA->options, sizeof(UColOptionSet));

    src->lh = 0;
    src->listCapacity = 1024;
    src->lh = (UColTokListHeader *)uprv_malloc(src->listCapacity*sizeof(UColTokListHeader));
    //Test for NULL
    if (src->lh == NULL) {
        *status = U_MEMORY_ALLOCATION_ERROR;
        return;
    }
    uprv_memset(src->lh, 0, src->listCapacity*sizeof(UColTokListHeader));
    src->resultLen = 0;

    UCAConstants *consts = (UCAConstants *)((uint8_t *)src->UCA->image + src->UCA->image->UCAConsts);

    // UCOL_RESET_TOP_VALUE
    setIndirectBoundaries(0, consts->UCA_LAST_NON_VARIABLE, consts->UCA_FIRST_IMPLICIT);
    // UCOL_FIRST_PRIMARY_IGNORABLE
    setIndirectBoundaries(1, consts->UCA_FIRST_PRIMARY_IGNORABLE, 0);
    // UCOL_LAST_PRIMARY_IGNORABLE
    setIndirectBoundaries(2, consts->UCA_LAST_PRIMARY_IGNORABLE, 0);
    // UCOL_FIRST_SECONDARY_IGNORABLE
    setIndirectBoundaries(3, consts->UCA_FIRST_SECONDARY_IGNORABLE, 0);
    // UCOL_LAST_SECONDARY_IGNORABLE
    setIndirectBoundaries(4, consts->UCA_LAST_SECONDARY_IGNORABLE, 0);
    // UCOL_FIRST_TERTIARY_IGNORABLE
    setIndirectBoundaries(5, consts->UCA_FIRST_TERTIARY_IGNORABLE, 0);
    // UCOL_LAST_TERTIARY_IGNORABLE
    setIndirectBoundaries(6, consts->UCA_LAST_TERTIARY_IGNORABLE, 0);
    // UCOL_FIRST_VARIABLE
    setIndirectBoundaries(7, consts->UCA_FIRST_VARIABLE, 0);
    // UCOL_LAST_VARIABLE
    setIndirectBoundaries(8, consts->UCA_LAST_VARIABLE, 0);
    // UCOL_FIRST_NON_VARIABLE
    setIndirectBoundaries(9, consts->UCA_FIRST_NON_VARIABLE, 0);
    // UCOL_LAST_NON_VARIABLE
    setIndirectBoundaries(10, consts->UCA_LAST_NON_VARIABLE, consts->UCA_FIRST_IMPLICIT);
    // UCOL_FIRST_IMPLICIT
    setIndirectBoundaries(11, consts->UCA_FIRST_IMPLICIT, 0);
    // UCOL_LAST_IMPLICIT
    setIndirectBoundaries(12, consts->UCA_LAST_IMPLICIT, consts->UCA_FIRST_TRAILING);
    // UCOL_FIRST_TRAILING
    setIndirectBoundaries(13, consts->UCA_FIRST_TRAILING, 0);
    // UCOL_LAST_TRAILING
    setIndirectBoundaries(14, consts->UCA_LAST_TRAILING, 0);
    ucolIndirectBoundaries[14].limitCE = (consts->UCA_PRIMARY_SPECIAL_MIN<<24);
}


void ucol_tok_closeTokenList(UColTokenParser *src) {
    if(src->copySet != NULL) {
        uset_close(src->copySet);
    }
    if(src->removeSet != NULL) {
        uset_close(src->removeSet);
    }
    if(src->tailored != NULL) {
        uhash_close(src->tailored);
    }
    if(src->lh != NULL) {
        uprv_free(src->lh);
    }
    if(src->source != NULL) {
        uprv_free(src->source);
    }
    if(src->opts != NULL) {
        uprv_free(src->opts);
    }
    if (src->reorderCodes != NULL) {
        uprv_free(src->reorderCodes);
    }
}

#endif /* #if !UCONFIG_NO_COLLATION */
