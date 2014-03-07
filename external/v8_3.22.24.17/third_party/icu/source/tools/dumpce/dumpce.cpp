/********************************************************************
 * COPYRIGHT:
 * Copyright (C) 2001-2005 IBM, Inc.   All Rights Reserved.
 *
 ********************************************************************/
/********************************************************************************
*
* File dumpce.cpp
*
* Modification History:
* Name          Date           Description
* synwee        May 31 2001    Creation
*
*********************************************************************************
*/

/**
* This program outputs the collation elements used for a requested tailoring.
*
* Usage:
*     dumpce options... please check main function.
*/
#include <unicode/utypes.h>
#include <unicode/ucol.h>
#include <unicode/uloc.h>
#include <unicode/ucoleitr.h>
#include <unicode/uchar.h>
#include <unicode/uscript.h>
#include <unicode/utf16.h>
#include <unicode/putil.h>
#include <unicode/ustring.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "ucol_tok.h"
#include "cstring.h"
#include "uoptions.h"
#include "ucol_imp.h"
#include <unicode/ures.h>
#include <unicode/uniset.h>
#include <unicode/usetiter.h>

/**
* Command line option variables. 
* These global variables are set according to the options specified on the 
* command line by the user.
*/
static UOption options[]={
    /* 00 */ UOPTION_HELP_H, 
    /* 01 */ UOPTION_HELP_QUESTION_MARK,
    /* 02 */ {"locale",        NULL, NULL, NULL, 'l', UOPT_REQUIRES_ARG, 0},
    /* 03 */ {"serialize",     NULL, NULL, NULL, 'z', UOPT_NO_ARG, 0},
	/* 04 */ UOPTION_DESTDIR,
    /* 05 */ UOPTION_SOURCEDIR,
    /* 06 */ {"attribute",     NULL, NULL, NULL, 'a', UOPT_REQUIRES_ARG, 0},
    /* 07 */ {"rule",          NULL, NULL, NULL, 'r', UOPT_REQUIRES_ARG, 0},
    /* 08 */ {"normalization", NULL, NULL, NULL, 'n', UOPT_REQUIRES_ARG, 0},
    /* 09 */ {"scripts",       NULL, NULL, NULL, 't', UOPT_NO_ARG, 0},
    /* 10 */ {"reducehan",     NULL, NULL, NULL, 'e', UOPT_NO_ARG, 0},
	/* 11 */ UOPTION_VERBOSE,
    /* 12 */ {"wholescripts",      NULL, NULL, NULL, 'W', UOPT_NO_ARG, 0}
};

/**
* Collator used in this program
*/
static UCollator *COLLATOR_;
/**
* Output strea, used in this program
*/
static FILE *OUTPUT_;

static UColAttributeValue ATTRIBUTE_[UCOL_ATTRIBUTE_COUNT] = {
    UCOL_DEFAULT, UCOL_DEFAULT, UCOL_DEFAULT, UCOL_DEFAULT, UCOL_DEFAULT, 
    UCOL_DEFAULT, UCOL_DEFAULT, UCOL_DEFAULT,
};

typedef struct {
    int   value;
    char *name;
} EnumNameValuePair;

static const EnumNameValuePair ATTRIBUTE_NAME_[] = {
    {UCOL_FRENCH_COLLATION, "UCOL_FRENCH_COLLATION"},
    {UCOL_ALTERNATE_HANDLING, "UCOL_ALTERNATE_HANDLING"}, 
    {UCOL_CASE_FIRST, "UCOL_CASE_FIRST"}, 
    {UCOL_CASE_LEVEL, "UCOL_CASE_LEVEL"}, 
    {UCOL_NORMALIZATION_MODE, 
        "UCOL_NORMALIZATION_MODE|UCOL_DECOMPOSITION_MODE"},
    {UCOL_STRENGTH, "UCOL_STRENGTH"},
	{UCOL_HIRAGANA_QUATERNARY_MODE, "UCOL_HIRAGANA_QUATERNARY_MODE"},
    {UCOL_NUMERIC_COLLATION, "UCOL_NUMERIC_COLLATION"},
    NULL
};
     
static const EnumNameValuePair ATTRIBUTE_VALUE_[] = {
    {UCOL_PRIMARY, "UCOL_PRIMARY"},
    {UCOL_SECONDARY, "UCOL_SECONDARY"},
    {UCOL_TERTIARY, "UCOL_TERTIARY|UCOL_DEFAULT_STRENGTH"},
    {UCOL_QUATERNARY, "UCOL_QUATERNARY"},
    {UCOL_IDENTICAL, "UCOL_IDENTICAL"},
    {UCOL_OFF, "UCOL_OFF"},
    {UCOL_ON, "UCOL_ON"},
    {UCOL_SHIFTED, "UCOL_SHIFTED"},
    {UCOL_NON_IGNORABLE, "UCOL_NON_IGNORABLE"},
    {UCOL_LOWER_FIRST, "UCOL_LOWER_FIRST"},
    {UCOL_UPPER_FIRST, "UCOL_UPPER_FIRST"},
    NULL
};

typedef struct {
    UChar ch[32];
    int   count; // number of codepoint
    UBool tailored;
} ScriptElement;

/**
* Writes the hexadecimal of a null-terminated array of codepoints into a 
* file
* @param f UFILE instance to store
* @param c codepoints array
*/
void serialize(FILE *f, const UChar *c) 
{
    UChar cp = *(c ++);
    
    fprintf(f, " %04x", cp);
   
    while (*c != 0) {
        cp = *(c ++);
        fprintf(f, " %04x", cp);
    }
}

/**
* Writes the hexadecimal of a non-null-terminated array of codepoints into a 
* file
* @param f UFILE instance to store
* @param c codepoints array
* @param l codepoints array length
*/
void serialize(FILE *f, const UChar *c, int l) 
{
    int   count = 1;
    UChar cp    = *(c ++);
    
    fprintf(f, " %04x", cp);
   
    while (count < l) {
        cp = *(c ++);
        fprintf(f, " %04x", cp);
        count ++;
    }
}

/**
* Sets the iterator to the argument string and outputs the collation elements.
* @param f file output stream
* @param iter collation element iterator
*/
void serialize(FILE *f, UCollationElements *iter) {
    UChar   *codepoint = iter->iteratordata_.string;
    // unlikely that sortkeys will be over this size 
    uint8_t  sortkey[64];
    uint8_t *psortkey = sortkey;
    int      sortkeylength = 0;

    if (iter->iteratordata_.flags & UCOL_ITER_HASLEN) {
        serialize(f, codepoint, iter->iteratordata_.endp - codepoint);
        sortkeylength = ucol_getSortKey(iter->iteratordata_.coll, codepoint, 
                        iter->iteratordata_.endp - codepoint, sortkey, 64);
    }
    else {
        serialize(f, codepoint);
        sortkeylength = ucol_getSortKey(iter->iteratordata_.coll, codepoint, 
                                        -1, sortkey, 64);
    }
    if (options[11].doesOccur) {
        serialize(stdout, codepoint);
        fprintf(stdout, "\n");
    }

    fprintf(f, "; ");

    UErrorCode error = U_ZERO_ERROR;
    uint32_t ce = ucol_next(iter, &error);
    if (U_FAILURE(error)) {
        fprintf(f, "Error retrieving collation elements\n");
        return;
    }
    
    while (TRUE) {
        fprintf(f, "[");
        if (UCOL_PRIMARYORDER(ce) != 0) {
            fprintf(f, "%04x", UCOL_PRIMARYORDER(ce));
        }
        fprintf(f, ",");
        if (UCOL_SECONDARYORDER(ce) != 0) {
            fprintf(f, " %02x", UCOL_SECONDARYORDER(ce));
        }
        fprintf(f, ",");
        if (UCOL_TERTIARYORDER(ce) != 0) {
            fprintf(f, " %02x", UCOL_TERTIARYORDER(ce));
        }
        fprintf(f, "] ");

        ce = ucol_next(iter, &error);
        if (ce == UCOL_NULLORDER) {
            break;
        }
        if (U_FAILURE(error)) {
            fprintf(stdout, "Error retrieving collation elements");
            return;
        }
    }
    
    if (sortkeylength > 64) {
        fprintf(f, "Sortkey exceeds pre-allocated size");
    }

    fprintf(f, "[");
    while (TRUE) {
        fprintf(f, "%02x", *psortkey);
        psortkey ++;
        if ((*psortkey) == 0) {
            break;
        }
        fprintf(f, " ");
    }
    fprintf(f, "]\n");
}

/**
* Serializes the contraction within the given argument rule
* @param f file output stream
* @param r rule
* @param rlen rule length
* @param contractionsonly flag to indicate if only contractions are to be 
*                         output or all collation elements
* @param iter iterator to iterate over collation elements
*/
void serialize(FILE *f, UChar *rule, int rlen, UBool contractiononly, 
               UCollationElements *iter) {
    const UChar           *current  = NULL;
          uint32_t         strength = 0;
          uint32_t         chOffset = 0; 
          uint32_t         chLen    = 0;
          uint32_t         exOffset = 0; 
          uint32_t         exLen    = 0;
          uint32_t         prefixOffset = 0; 
          uint32_t         prefixLen    = 0;
          uint8_t          specs    = 0;
          UBool            rstart   = TRUE;
          UColTokenParser  src;
          UColOptionSet    opts;
          UParseError      parseError;
          UErrorCode       error    = U_ZERO_ERROR;
    
    src.opts = &opts;
      
    src.source       = rule; 
	src.current = rule;
    src.end          = rule + rlen;
    src.extraCurrent = src.end;
    src.extraEnd     = src.end + UCOL_TOK_EXTRA_RULE_SPACE_SIZE;

        
    while ((current = ucol_tok_parseNextToken(&src, rstart, &parseError,
                                              &error)) != NULL) {
      chOffset = src.parsedToken.charsOffset;
      chLen = src.parsedToken.charsLen;
        // contractions handled here
        if (!contractiononly || chLen > 1) {
            ucol_setText(iter, rule + chOffset, chLen, &error);
            if (U_FAILURE(error)) {
                fprintf(stdout, "Error setting text in iterator\n");
                return;
            }
            serialize(f, iter);
        }
        rstart = FALSE;
    }
}

/**
* Prints the attribute values in the argument collator into the output stream
* @param collator
*/
void outputAttribute(UCollator *collator, UErrorCode *error) 
{
    UColAttribute attribute = UCOL_FRENCH_COLLATION;
    while (attribute < UCOL_ATTRIBUTE_COUNT) {
        int count = 0;
        while (TRUE) {
            // getting attribute name
            if (ATTRIBUTE_NAME_[count].value == attribute) {
                fprintf(OUTPUT_, "%s = ", ATTRIBUTE_NAME_[count].name);
                break;
            }
            count ++;
        }
        count = 0;
        int attributeval = ucol_getAttribute(collator, attribute, error);
        if (U_FAILURE(*error)) {
            fprintf(stdout, "Failure in reading collator attribute\n");
            return;
        }
        while (TRUE) {
            // getting attribute value
            if (ATTRIBUTE_VALUE_[count].value == attributeval) {
                fprintf(OUTPUT_, "%s\n", ATTRIBUTE_VALUE_[count].name);
                break;
            }
            count ++;
        }
        attribute = (UColAttribute)(attribute + 1);
    }
}

/**
* Prints the normalization mode in the argument collator into the output stream
* @param collator
*/
void outputNormalization(UCollator *collator) 
{
	UErrorCode status = U_ZERO_ERROR;
    int normmode = ucol_getAttribute(collator, UCOL_NORMALIZATION_MODE, &status);
    int count = 0;
    while (TRUE) {
        // getting attribute name
        if (ATTRIBUTE_VALUE_[count].value == normmode) {
            break;
        }
        count ++;
    }
    fprintf(OUTPUT_, "NORMALIZATION MODE = %s\n", 
            ATTRIBUTE_VALUE_[count].name);
}

/**
* Output the collation element belonging to the locale into a file
* @param locale string
* @param fullrules flag to indicate if only tailored collation elements are to
*        be output or all collation elements
*/
void serialize(const char *locale, UBool tailoredonly) {
    UErrorCode  error              = U_ZERO_ERROR;
    UChar       str[128];
    int         strlen = 0;

    fprintf(OUTPUT_, "# This file contains the serialized collation elements\n");
    fprintf(OUTPUT_, "# as of the collation version indicated below.\n");
    fprintf(OUTPUT_, "# Data format: xxxx xxxx..; [yyyy, yy, yy] [yyyy, yy, yy] ... [yyyy, yy, yy] [zz zz..\n");
    fprintf(OUTPUT_, "#              where xxxx are codepoints in hexadecimals,\n");
    fprintf(OUTPUT_, "#              yyyyyyyy are the corresponding\n");
    fprintf(OUTPUT_, "#              collation elements in hexadecimals\n");
    fprintf(OUTPUT_, "#              and zz are the sortkey values in hexadecimals\n");

    fprintf(OUTPUT_, "\n# Collator information\n");

    fprintf(OUTPUT_, "\nLocale: %s\n", locale);
    fprintf(stdout, "Locale: %s\n", locale);
    UVersionInfo version;
    ucol_getVersion(COLLATOR_, version);
    fprintf(OUTPUT_, "Version number: %d.%d.%d.%d\n", 
                      version[0], version[1], version[2], version[3]);
    outputAttribute(COLLATOR_, &error);
    outputNormalization(COLLATOR_);
    
    UCollationElements *iter = ucol_openElements(COLLATOR_, str, strlen, 
                                                 &error);
    if (U_FAILURE(error)) {
        fprintf(stdout, "Error creating iterator\n");
        return;
    }

    if (!tailoredonly) {
        fprintf(OUTPUT_, "\n# Range of unicode characters\n\n");
        UChar32     codepoint          = 0;
        while (codepoint <= UCHAR_MAX_VALUE) { 
            if (u_isdefined(codepoint)) {
                strlen = 0;
                UTF16_APPEND_CHAR_UNSAFE(str, strlen, codepoint);
                str[strlen] = 0;
                ucol_setText(iter, str, strlen, &error);
                if (U_FAILURE(error)) {
                    fprintf(stdout, "Error setting text in iterator\n");
                    return;
                }
                serialize(OUTPUT_, iter);
            }
            codepoint ++;
        }
    }

    UChar    ucarules[0x10000];
    UChar   *rules;
    int32_t  rulelength = 0;
    rules      = ucarules;
    
    if (tailoredonly) {
              int32_t  rulelength = 0;
        const UChar   *temp = ucol_getRules(COLLATOR_, &rulelength);
        if (rulelength + UCOL_TOK_EXTRA_RULE_SPACE_SIZE > 0x10000) {
            rules = (UChar *)malloc(sizeof(UChar) * 
                                (rulelength + UCOL_TOK_EXTRA_RULE_SPACE_SIZE));
        }
        memcpy(rules, temp, rulelength * sizeof(UChar));
        rules[rulelength] = 0;
        fprintf(OUTPUT_, "\n# Tailorings\n\n");
        serialize(OUTPUT_, rules, rulelength, FALSE, iter);
        if (rules != ucarules) {
            free(rules);
        }
    }
    else {        
        rulelength = ucol_getRulesEx(COLLATOR_, UCOL_FULL_RULES, ucarules, 
                                     0x10000);
        if (rulelength + UCOL_TOK_EXTRA_RULE_SPACE_SIZE > 0x10000) {
            rules = (UChar *)malloc(sizeof(UChar) * 
                                (rulelength + UCOL_TOK_EXTRA_RULE_SPACE_SIZE));
            rulelength = ucol_getRulesEx(COLLATOR_, UCOL_FULL_RULES, rules, 
                                         rulelength);
        }
        fprintf(OUTPUT_, "\n# Contractions\n\n");
        serialize(OUTPUT_, rules, rulelength, TRUE, iter);
        if (rules != ucarules) {
            free(rules);
        }
    }
        
    ucol_closeElements(iter);
}

/**
* Sets the collator with the attribute values
* @param collator
* @param error status
*/
void setAttributes(UCollator *collator, UErrorCode *error) 
{
    int count = 0;
    while (count < UCOL_ATTRIBUTE_COUNT) {
        if (ATTRIBUTE_[count] != UCOL_DEFAULT) {
            ucol_setAttribute(collator, (UColAttribute)count, 
                              ATTRIBUTE_[count], error);
            if (U_FAILURE(*error)) {
                return;
            }
        }
        count ++;
    }
}

/**
* Appends directory path with an ending seperator if necessary.
* @param path with enough space to append one seperator
* @return new directory path length
*/
int appendDirSeparator(char *dir) 
{
    int dirlength = strlen(dir);
    char dirending = dir[dirlength - 1];
    if (dirending != U_FILE_SEP_CHAR) {
        dir[dirlength] = U_FILE_SEP_CHAR;
        dir[dirlength + 1] = 0;
        return dirlength + 1;
    }
    return dirlength;
}

/**
* Output the collation element into a file
*/
void serialize() {
    char filename[128];
    int  dirlength = 0;

    if (options[4].doesOccur) {
        strcpy(filename, options[4].value);
        dirlength = appendDirSeparator(filename);
    }

    if (options[2].doesOccur) {
        const char    *locale      = (char *)options[2].value;
              int32_t  localeindex = 0;
        
        if (strcmp(locale, "all") == 0) {
            if (options[4].doesOccur) {
                strcat(filename, "UCA.txt");
                OUTPUT_ = fopen(filename, "w");
                if (OUTPUT_ == NULL) {
                    fprintf(stdout, "Cannot open file:%s\n", filename);
                    return;
                }
            }
            fprintf(stdout, "UCA\n");
            UErrorCode error = U_ZERO_ERROR;
            COLLATOR_ = ucol_open("en_US", &error);
            if (U_FAILURE(error)) {
                fprintf(stdout, "Collator creation failed:");
                fprintf(stdout, u_errorName(error));
                goto CLOSEUCA;
                return;
            }
            setAttributes(COLLATOR_, &error);
            if (U_FAILURE(error)) {
                fprintf(stdout, "Collator attribute setting failed:");
                fprintf(stdout, u_errorName(error));
                goto CLOSEUCA;
                return;
            }
        
            serialize("UCA", FALSE);
CLOSEUCA :  
            if (options[4].doesOccur) {
                filename[dirlength] = 0;
                fclose(OUTPUT_);
            }
            ucol_close(COLLATOR_);
            localeindex = ucol_countAvailable() - 1;
            fprintf(stdout, "Number of locales: %d\n", localeindex + 1);
            locale      = ucol_getAvailable(localeindex);
        }

        while (TRUE) {
            UErrorCode error = U_ZERO_ERROR;
            COLLATOR_ = ucol_open(locale, &error);
            if (U_FAILURE(error)) {
                fprintf(stdout, "Collator creation failed:");
                fprintf(stdout, u_errorName(error));
                goto CLOSETAILOR;
                return;
            }
            setAttributes(COLLATOR_, &error);
            if (U_FAILURE(error)) {
                fprintf(stdout, "Collator attribute setting failed:");
                fprintf(stdout, u_errorName(error));
                goto CLOSETAILOR;
                return;
            }

            if (options[4].doesOccur) {
                strcat(filename, locale);
                strcat(filename, ".txt");
                OUTPUT_ = fopen(filename, "w");
                if (OUTPUT_ == NULL) {
                    fprintf(stdout, "Cannot open file:%s\n", filename);
                    return;
                }
            }

            if (options[3].doesOccur) {
                serialize(locale, TRUE);
            }

            ucol_close(COLLATOR_);

CLOSETAILOR : 
            if (options[4].doesOccur) {
                filename[dirlength] = 0;
                fclose(OUTPUT_);
            }
    
            localeindex --;
            if (localeindex < 0) {
                break;
            }
            locale = ucol_getAvailable(localeindex);
        }
    }

    if (options[7].doesOccur) {
        char inputfilename[128];
        // rules are to be used
        if (options[5].doesOccur) {
            strcpy(inputfilename, options[5].value);
            appendDirSeparator(inputfilename);
        }
        strcat(inputfilename, options[7].value);
        FILE *input = fopen(inputfilename, "r");
        if (input == NULL) {
            fprintf(stdout, "Cannot open file:%s\n", filename);
            return;
        }
        
        char   s[1024];
        UChar  rule[1024];
        UChar *prule = rule;
        int    size = 1024;
        // synwee TODO: make this part dynamic
        while (fscanf(input, "%[^\n]s", s) != EOF) {
            size -= u_unescape(s, prule, size);
            prule = prule + u_strlen(prule);
        }
        fclose(input);

        if (options[4].doesOccur) {
            strcat(filename, "Rules.txt");
            OUTPUT_ = fopen(filename, "w");
            if (OUTPUT_ == NULL) {
                fprintf(stdout, "Cannot open file:%s\n", filename);
                return;
            }
        }

        fprintf(stdout, "Rules\n");
        UErrorCode  error = U_ZERO_ERROR;
        UParseError parseError;
        COLLATOR_ = ucol_openRules(rule, u_strlen(rule), UCOL_DEFAULT, 
                                   UCOL_DEFAULT_STRENGTH, &parseError, &error);
        if (U_FAILURE(error)) {
            fprintf(stdout, "Collator creation failed:");
            fprintf(stdout, u_errorName(error));
            goto CLOSERULES;
            return;
        }
        setAttributes(COLLATOR_, &error);
        if (U_FAILURE(error)) {
            fprintf(stdout, "Collator attribute setting failed:");
            fprintf(stdout, u_errorName(error));
            goto CLOSERULES;
            return;
        }
        
        serialize("Rule-based", TRUE);
        ucol_close(COLLATOR_);

CLOSERULES :
        if (options[4].doesOccur) {
            filename[dirlength] = 0;
            fclose(OUTPUT_);
        }
    }
}

/**
* Parse for enum values.
* Note this only works for positive enum values.
* @param enumarray array containing names of the enum values in string and 
*        their corresponding value.
*        declared enum value.
* @param str string to be parsed
* @return corresponding integer enum value or -1 if value is not found.
*/
int parseEnums(const EnumNameValuePair enumarray[], const char *str) 
{
    const char *enumname = enumarray[0].name;
    int result = atoi(str);
    if (result == 0 && str[0] != '0') {
        while (strcmp(enumname, str) != 0) {
            // checking for multiple enum names sharing the same values
            enumname = strstr(enumname, str);
            if (enumname != NULL) {
                int size = strchr(enumname, '|') - enumname;
                if (size < 0) {
                    size = strlen(enumname);
                }
                if (size == (int)strlen(str)) {
                    return enumarray[result].value;
                }
            }
            result ++;
            if (&(enumarray[result]) == NULL) {
                return -1;
            }
            enumname = enumarray[result].name;
        }
    }
    return -1;
}

/**
* Parser for attribute name value pair
*/
void parseAttributes() {
    char str[32];
    const char *pname = options[6].value;
    const char *pend  = options[6].value + strlen(options[6].value);
    const char *pvalue;
    
    while (pname < pend) {
        pvalue = strchr(pname, '=');
        if (pvalue == NULL) {
            fprintf(stdout, 
                    "No matching value found for attribute argument %s\n", 
                    pname);        
            return;
        }
        int count = pvalue - pname;
        strncpy(str, pname, count);
        str[count] = 0;

        int name = parseEnums(ATTRIBUTE_NAME_, str);
        if (name == -1) {
            fprintf(stdout, "Attribute name not found: %s\n", str);
            return;
        }
        
        pvalue ++;
        // getting corresponding enum value
        pname = strchr(pvalue, ',');
        if (pname == NULL) {
            pname = pend;
        }
        count = pname - pvalue;
        strncpy(str, pvalue, count);
        str[count] = 0;
        int value = parseEnums(ATTRIBUTE_VALUE_, str);
        if (value == -1) {
            fprintf(stdout, "Attribute value not found: %s\n", str);
            return;
        }
        ATTRIBUTE_[name] = (UColAttributeValue)value;
        pname ++;
    }
}

/**
* Checks if the locale argument is a base language 
* @param locale to be checked
* @return TRUE if it is a base language
*/
inline UBool checkLocaleForLanguage(const char *locale)
{
    return strlen(locale) <= 2;
}

/**
* Converts a UChar array into its string form "xxxx xxxx"
* @param ch array of UChar characters
* @param count number of UChar characters
*/
void outputUChar(UChar ch[], int count)
{
    for (int i = 0; i < count; i ++) {
        fprintf(OUTPUT_, "%04X ", ch[i]);
    }
}

/**
* If it is a primary difference returns -1 or 1.
* If it is a secondary difference returns -2 or 2.
* If it is a tertiary difference returns -3 or 3.
* If equals returns 0.
*/
int compareSortKey(const void *elem1, const void *elem2)
{
    // compare the 2 script element sort key
    UChar     *ch1   = ((ScriptElement *)elem1)->ch;
    UChar     *ch2   = ((ScriptElement *)elem2)->ch;
    int        size1 = ((ScriptElement *)elem1)->count;
    int        size2 = ((ScriptElement *)elem2)->count;
    UErrorCode error = U_ZERO_ERROR;
    
    ucol_setStrength(COLLATOR_, UCOL_PRIMARY);
    int result = ucol_strcoll(COLLATOR_, ch1, size1, ch2, size2);
    if (result == 0) {
        ucol_setStrength(COLLATOR_, UCOL_SECONDARY);
        result = ucol_strcoll(COLLATOR_, ch1, size1, ch2, size2);
        if (result == 0) {
            ucol_setStrength(COLLATOR_, UCOL_TERTIARY);
            result = ucol_strcoll(COLLATOR_, ch1, size1, ch2, size2);
            if (result < 0) {
                return -3;
            }
            if (result > 0) {
                return 3;
            }    
        }
        if (result < 0) {
            return -2;
        }
        if (result > 0) {
            return 2;
        }
    }
    return result;
}

/**
* Output serialized script elements
* @param element the element to output
* @param compare the comparison with the previous element
* @param expansion flags TRUE if element has an expansion
*/
void outputScriptElem(ScriptElement &element, int compare, UBool expansion)
{
    switch (compare) {
    case 0: 
        if (expansion) {
            fprintf(OUTPUT_, "<tr><td class='eq' title='["); 
        }
        else {
            fprintf(OUTPUT_, "<tr><td class='q' title='["); 
        }
        break;  
    case -1: 
        if (expansion) {
            fprintf(OUTPUT_, "<tr><td class='ep' title='["); 
        }
        else {
            fprintf(OUTPUT_, "<tr><td class='p' title='["); 
        }
        break;        
    case -2: 
        if (expansion) {
            fprintf(OUTPUT_, "<tr><td class='es' title='["); 
        }
        else {
            fprintf(OUTPUT_, "<tr><td class='s' title='["); 
        }
        break;
    default: 
        if (expansion) {
            fprintf(OUTPUT_, "<tr><td class='et' title='["); 
        }
        else {
            fprintf(OUTPUT_, "<tr><td class='t' title='["); 
        }
    }

    uint8_t sortkey[32];
    ucol_setStrength(COLLATOR_, UCOL_TERTIARY);
    ucol_getSortKey(COLLATOR_, element.ch, element.count, sortkey, 32);
    int i = 0;
    while (sortkey[i] != 0) {
        if (sortkey[i] == 1) {
            fprintf(OUTPUT_, " | ");
        }
        else {
            fprintf(OUTPUT_, "%02x", sortkey[i]);
        }

        i ++;
    }

    fprintf(OUTPUT_, "]'>");
    
    UErrorCode error = U_ZERO_ERROR;
    char       utf8[64];
    UChar      nfc[32];
    int32_t    length = unorm_normalize(element.ch, element.count, UNORM_NFC, 0, nfc, 
                                        32, &error);
    if (U_FAILURE(error)) {
        fprintf(stdout, "Error normalizing contractions to NFC\n");
    }
    u_strToUTF8(utf8, 64, &length, nfc, length, &error);
    if (U_FAILURE(error)) {
        fprintf(stdout, "Error converting UChar to utf8\n");
        return;
    }
    
    fprintf(OUTPUT_, "%s<br>", utf8);
    fprintf(OUTPUT_, "<tt>");
    outputUChar(element.ch, element.count);

    if (compare == 0) {
        fprintf(OUTPUT_, "</tt></td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>Q</td><td>");
    }
    else if (compare == -1) {
        fprintf(OUTPUT_, "</tt></td><td>P</td><td>&nbsp;</td><td>&nbsp;</td><td>&nbsp;</td><td>");
    }
    else if (compare == -2) {
        fprintf(OUTPUT_, "</tt></td><td>&nbsp;</td><td>S</td><td>&nbsp;</td><td>&nbsp;</td><td>");
    }
    else if (compare == -3) {
        fprintf(OUTPUT_, "</tt></td><td>&nbsp;</td><td>&nbsp;</td><td>T</td><td>&nbsp;</td><td>");
    }

    i = 0;
    while (i < element.count) {
        char    str[128];
        UChar32 codepoint;
        UTF_NEXT_CHAR(element.ch, i, element.count, codepoint);
        int32_t temp = u_charName(codepoint, U_UNICODE_CHAR_NAME, str, 128, 
                                      &error);
        if (U_FAILURE(error)) {
            fprintf(stdout, "Error getting character name\n");
            return;
        }
        if (element.tailored) {
            fprintf(OUTPUT_, "<b>");
        }
        fprintf(OUTPUT_, "%s", str);
        if (element.tailored) {
            fprintf(OUTPUT_, " *</b>");
        }
        if (i < element.count) {
            fprintf(OUTPUT_, "<br>\n");
        }
    }

    fprintf(OUTPUT_, "</td></tr>\n");
}

/**
* Checks if codepoint belongs to scripts
* @param script list
* @param scriptcount number of scripts
* @param codepoint to test
* @return TRUE if codepoint belongs to scripts
*/
UBool checkInScripts(UScriptCode script[], int scriptcount, 
                     UChar32 codepoint)
{
    UErrorCode error = U_ZERO_ERROR;
    for (int i = 0; i < scriptcount; i ++) {
        if (script[i] == USCRIPT_HAN && options[10].doesOccur) { 
            if ((codepoint >= 0x2E80 && codepoint <= 0x2EE4) ||
                (codepoint >= 0x2A672 && codepoint <= 0x2A6D6)) {
                // reduce han
                return TRUE;
            }
        }
        else if (uscript_getScript(codepoint, &error) == script[i]) {
            return TRUE;
        }
        if (U_FAILURE(error)) {
            fprintf(stdout, "Error checking character in scripts\n");
            return FALSE;
        }
    }
    return FALSE;
}

/**
* Checks if the set of codepoints belongs to the script
* @param script list
* @param scriptcount number of scripts
* @param scriptelem
* @return TRUE if all codepoints belongs to the script
*/
inline UBool checkInScripts(UScriptCode script[], int scriptcount,
                           ScriptElement scriptelem)
{
    int i = 0;
    while (i < scriptelem.count) {
        UChar32     codepoint;
        UTF_NEXT_CHAR(scriptelem.ch, i, scriptelem.count, codepoint);
        UErrorCode  error = U_ZERO_ERROR;
        if (checkInScripts(script, scriptcount, codepoint)) {
            return TRUE;
        }
    }
    return FALSE;
}

/**
* Gets the script elements and contractions belonging to the script
* @param elems output list
* @param locale locale
* @return number of script elements
* Add by Richard
*/
int getScriptElementsFromExemplars(ScriptElement scriptelem[], const char* locale) {
	UErrorCode error = U_ZERO_ERROR;
    UChar32    codepoint = 0;

	UResourceBundle* ures = ures_open(NULL, locale, &error);
	if (U_FAILURE(error)) {
		fprintf(stdout, "Can not find resource bundle for locale: %s\n", locale);
        return -1;
	}
	int32_t length;
	const UChar* exemplarChars = ures_getStringByKey(ures, "ExemplarCharacters", &length, &error);
	
	if (U_FAILURE(error)) {
		fprintf(stdout, "Can not find ExemplarCharacters in resource bundle\n");
        return -1;
	}

	UChar* upperChars = new UChar[length*2];
	if (upperChars == 0) {
		fprintf(stdout, "Memory error\n");
        return -1;
	}

	int32_t destLength = u_strToUpper(upperChars, length*2, exemplarChars, -1, locale, &error);
	if (U_FAILURE(error)) {
		fprintf(stdout, "Error when u_strToUpper() \n");
        return -1;
	}

	UChar* pattern = new UChar[length + destLength + 10];
	UChar left[2] = {0x005b, 0x0};
	UChar right[2] = {0x005d, 0x0};
	pattern = u_strcpy(pattern, left);
	pattern = u_strcat(pattern, exemplarChars);
	pattern = u_strcat(pattern, upperChars);
	pattern = u_strcat(pattern, right);

	UnicodeSet * uniset = new UnicodeSet(UnicodeString(pattern), error);
	if (U_FAILURE(error)) {
		fprintf(stdout, "Can not open USet \n");
        return -1;
	}
	
	UnicodeSetIterator* usetiter = new UnicodeSetIterator(*uniset);

	int32_t count = 0;

	while (usetiter -> next()) {
		if (usetiter -> isString()) {
			UnicodeString strItem = usetiter -> getString();

			scriptelem[count].count = 0;
			for (int i = 0; i < strItem.length(); i++) {
				codepoint = strItem.char32At(i);
				UTF16_APPEND_CHAR_UNSAFE(scriptelem[count].ch, 
										scriptelem[count].count, codepoint);
				scriptelem[count].tailored = FALSE;
			}
		} else {
			codepoint = usetiter -> getCodepoint();
			scriptelem[count].count = 0;
			UTF16_APPEND_CHAR_UNSAFE(scriptelem[count].ch, 
				                     scriptelem[count].count, codepoint);
			scriptelem[count].tailored = FALSE;
		}

		count++;
	}

	return count;
}

/**
* Gets the script elements and contractions belonging to the script
* @param script list
* @param scriptcount number of scripts
* @param elems output list
* @return number of script elements
*/
int getScriptElements(UScriptCode script[], int scriptcount, 
                      ScriptElement scriptelem[])
{
    UErrorCode error = U_ZERO_ERROR;
    UChar32    codepoint = 0;
    int        count     = 0;
    while (codepoint <= UCHAR_MAX_VALUE) { 
        if (checkInScripts(script, scriptcount, codepoint)) {
            scriptelem[count].count = 0;
            UTF16_APPEND_CHAR_UNSAFE(scriptelem[count].ch, 
                                     scriptelem[count].count, codepoint);
            scriptelem[count].tailored = FALSE;
            count ++;
        }
        if (U_FAILURE(error)) {
            fprintf(stdout, "Error determining codepoint in script\n");
            return -1;
        }
        codepoint ++;
    }

    const UChar           *current  = NULL;
          uint32_t         strength = 0;
          uint32_t         chOffset = 0; 
          uint32_t         chLen    = 0;
          uint32_t         exOffset = 0; 
          uint32_t         exLen    = 0;
          uint32_t         prefixOffset = 0; 
          uint32_t         prefixLen    = 0;
          uint8_t          specs    = 0;
          UBool            rstart   = TRUE;
          UColTokenParser  src;
          UColOptionSet    opts;
          UParseError      parseError;

    int32_t  rulelength = ucol_getRulesEx(COLLATOR_, UCOL_FULL_RULES, NULL, 0);
    src.source       = (UChar *)malloc(sizeof(UChar) * 
                                (rulelength + UCOL_TOK_EXTRA_RULE_SPACE_SIZE));
    rulelength = ucol_getRulesEx(COLLATOR_, UCOL_FULL_RULES, src.source, 
                                 rulelength);
    src.current      = src.source;
    src.end          = src.source + rulelength;
    src.extraCurrent = src.end;
    src.extraEnd     = src.end + UCOL_TOK_EXTRA_RULE_SPACE_SIZE;
    src.opts         = &opts;
        
	/*
	ucol_tok_parseNextToken(&src, &strength, &chOffset, 
                                              &chLen, &exOffset, &exLen,
                                              &prefixOffset, &prefixLen,
                                              &specs, rstart, &parseError,
                                              &error)
    */
    while ((current = ucol_tok_parseNextToken(&src, rstart, &parseError,
                                              &error)) != NULL) {
        // contractions handled here
        if (chLen > 1) {
            u_strncpy(scriptelem[count].ch, src.source + chOffset, chLen);
            scriptelem[count].count = chLen;
            if (checkInScripts(script, scriptcount, scriptelem[count])) {
                scriptelem[count].tailored     = FALSE;
                count ++;
            }
        }
        rstart = FALSE;
    }
    if (U_FAILURE(error)) {
        fprintf(stdout, "Error parsing rules: %s\n", u_errorName(error));
    }
	// rule might have been reallocated, so delete this instead
    free(src.source); 
    return count;
}

int compareCodepoints(const void *elem1, const void *elem2)
{
    UChar *ch1 = ((ScriptElement *)elem1)->ch; // key
    UChar *ch2 = ((ScriptElement *)elem2)->ch;
    ch1[((ScriptElement *)elem1)->count] = 0;
    ch2[((ScriptElement *)elem2)->count] = 0;

    // compare the 2 codepoints
    return u_strcmp(ch1, ch2);
}

UBool hasSubNFD(ScriptElement &se, ScriptElement &key)
{
    UChar *ch1 = se.ch; 
    UChar *ch2 = key.ch; // key
    ch1[se.count] = 0;
    ch2[key.count] = 0;
    
    // compare the 2 codepoints
    if (u_strstr(ch1, ch2) != NULL) {
        return TRUE;
    }

    // check the decomposition 
    UChar      norm[32];
    UErrorCode error = U_ZERO_ERROR;
    int        size  = unorm_normalize(ch1, se.count, UNORM_NFD, 0, norm, 32, 
                                       &error);    
    if (U_FAILURE(error)) {
        fprintf(stdout, "Error normalizing\n");
    }
    if (u_strstr(norm, ch2) != NULL) {
        return TRUE;
    }
    return FALSE;
}

/**
* Marks tailored elements
* @param script list
* @param scriptcount number of scripts
* @param scriptelem script element list
* @param scriptelemlength size of the script element list
*/
void markTailored(UScriptCode script[], int scriptcount, 
                  ScriptElement scriptelem[], int scriptelemlength)
{
          int32_t  rulelength;
    const UChar   *rule = ucol_getRules(COLLATOR_, &rulelength);
    
    const UChar           *current  = NULL;
          uint32_t         strength = 0;
          uint32_t         chOffset = 0; 
          uint32_t         chLen    = 0;
          uint32_t         exOffset = 0; 
          uint32_t         exLen    = 0;
          uint32_t         prefixOffset = 0; 
          uint32_t         prefixLen    = 0;
          uint8_t          specs    = 0;
          UBool            rstart   = TRUE;
          UColTokenParser  src;
          UColOptionSet    opts;
          UParseError      parseError;
    
    src.opts         = &opts;
    src.source       = (UChar *)malloc(
               (rulelength + UCOL_TOK_EXTRA_RULE_SPACE_SIZE) * sizeof(UChar));
    memcpy(src.source, rule, rulelength * sizeof(UChar));
	src.current      = src.source;
    src.end          = (UChar *)src.source + rulelength;
    src.extraCurrent = src.end;
    src.extraEnd     = src.end + UCOL_TOK_EXTRA_RULE_SPACE_SIZE;

    UErrorCode    error = U_ZERO_ERROR;
        
    while ((current = ucol_tok_parseNextToken(&src, rstart, &parseError,
                                              &error)) != NULL) {
        if (chLen >= 1 && strength != UCOL_TOK_RESET) {
            // skipping the reset characters and non useful stuff.
            ScriptElement se;
            u_strncpy(se.ch, src.source + chOffset, chLen);
            se.count = chLen;

            if (checkInScripts(script, scriptcount, se)) {
                /*
                ScriptElement *tse = (ScriptElement *)bsearch(&se, scriptelem, 
                                                              scriptelemlength, 
                                                         sizeof(ScriptElement), 
                                                         compareCodepoints);
                */
                for (int i = 0; i < scriptelemlength; i ++) {
                    if (!scriptelem[i].tailored && 
                        hasSubNFD(scriptelem[i], se)) {
                        scriptelem[i].tailored = TRUE;
                    }
                }
            }
        }
        rstart = FALSE;
    }
    free(src.source);
    if (U_FAILURE(error)) {
        fprintf(stdout, "Error parsing rules\n");
    }
}

/**
* Checks if the collation iterator has more than 1 collation element
* @parem coleiter collation element iterator
* @return TRUE if collation iterator has more than 1 collation element
*/
UBool hasExpansions(UCollationElements *coleiter)
{
    UErrorCode error = U_ZERO_ERROR;
    int32_t    ce    = ucol_next(coleiter, &error);
    int        count = 0;

    if (U_FAILURE(error)) {
        fprintf(stdout, "Error getting next collation element\n");
    }
    while (ce != UCOL_NULLORDER) {
        if ((UCOL_PRIMARYORDER(ce) != 0) && !isContinuation(ce)) {
            count ++;
            if (count == 2) {
                return TRUE;
            }
        }
        ce = ucol_next(coleiter, &error);
        if (U_FAILURE(error)) {
            fprintf(stdout, "Error getting next collation element\n");
        }
    }
    return FALSE;
}

/**
* Prints the footer for index.html
* @param file output file
*/
void outputHTMLFooter()
{
    fprintf(OUTPUT_, "</table>\n");
    fprintf(OUTPUT_, "</body>\n");
    fprintf(OUTPUT_, "</html>\n");
}

/**
* Serialize the codepoints from start to end into an html file.
* Arranging them into ascending collation order.
* @param script code list
* @param scriptcount number of scripts
*/
//void serializeScripts(UScriptCode script[], int scriptcount) 
//Richard
void serializeScripts(UScriptCode script[], int scriptcount, const char* locale = NULL) 
{
    UErrorCode  error  = U_ZERO_ERROR;
    
    ScriptElement *scriptelem = 
                     (ScriptElement *)malloc(sizeof(ScriptElement) * 0x20000);
    if (scriptelem == NULL) {
        fprintf(stdout, "Memory error\n");
        return;
    }
    int count = 0;
    if(locale) {
      count = getScriptElementsFromExemplars(scriptelem, locale);
    } else {
      count = getScriptElements(script, scriptcount, scriptelem); 
    }

    // Sort script elements using Quicksort algorithm:
    qsort(scriptelem, count, sizeof(ScriptElement), compareCodepoints);
    markTailored(script, scriptcount, scriptelem, count);
    // Sort script elements using Quicksort algorithm:
    qsort(scriptelem, count, sizeof(ScriptElement), compareSortKey);

    UCollationElements* coleiter = ucol_openElements(COLLATOR_, 
                                                     scriptelem[0].ch,
                                                     scriptelem[0].count,
                                                     &error);
    if (U_FAILURE(error)) {
        fprintf(stdout, "Error creating collation element iterator\n");
        return;
    }

    outputScriptElem(scriptelem[0], -1, hasExpansions(coleiter));
    for (int i = 0; i < count - 1; i ++) {
        ucol_setText(coleiter, scriptelem[i + 1].ch, scriptelem[i + 1].count,
                     &error);
        if (U_FAILURE(error)) {
            fprintf(stdout, "Error setting text in collation element iterator\n");
            return;
        }
        outputScriptElem(scriptelem[i + 1], 
                         compareSortKey(scriptelem + i, scriptelem + i + 1),
                         hasExpansions(coleiter));
    }
    free(scriptelem);
    outputHTMLFooter();
}

/**
* Prints the header for the html
* @param locale name
* @param script
* @param scriptcount number of scripts
*/
void outputHTMLHeader(const char *locale, UScriptCode script[], 
                      int scriptcount)
{
    fprintf(OUTPUT_, "<html>\n");
    fprintf(OUTPUT_, "<head>\n");
    fprintf(OUTPUT_, "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">\n");
    fprintf(OUTPUT_, "<meta http-equiv=\"Content-Language\" content=\"en-us\">\n");
    fprintf(OUTPUT_, "<link rel=\"stylesheet\" href=\"charts.css\" type=\"text/css\">\n");
    fprintf(OUTPUT_, "<title>ICU Collation charts</title>\n");
    fprintf(OUTPUT_, "<base target=\"main\">\n");
    fprintf(OUTPUT_, "</head>\n");

    fprintf(OUTPUT_, "<body bgcolor=#FFFFFF>\n");
    fprintf(OUTPUT_, "<!--\n");
    fprintf(OUTPUT_, "This file contains sorted characters in ascending order according to the locale stated\n");
    fprintf(OUTPUT_, "If the character is in red, it is tailored in the collation rules.\n");
    fprintf(OUTPUT_, "Background colours have certain meanings:\n");
    fprintf(OUTPUT_, "White - equals the previous character\n");
    fprintf(OUTPUT_, "dark blue - primary greater than the previous character\n");
    fprintf(OUTPUT_, "blue - secondary greater than the previous character\n");
    fprintf(OUTPUT_, "light blue - tertiary greater than the previous character\n");
    fprintf(OUTPUT_, "--!>\n");

    fprintf(OUTPUT_, "<table border=0>\n");
    UChar      displayname[64];
    UErrorCode error = U_ZERO_ERROR;
    int32_t size = uloc_getDisplayName(locale, "en_US", displayname, 64, &error);
    char       utf8displayname[128];
    if (U_FAILURE(error)) {
        utf8displayname[0] = 0;
    }
    else {
        int32_t utf8size = 0;
        u_strToUTF8(utf8displayname, 128, &utf8size, displayname, size, &error);
    }

    fprintf(OUTPUT_, "<tr><th>Locale</th><td class='noborder'>%s</td></tr>\n", utf8displayname);
    fprintf(OUTPUT_, "<tr><th>Script(s)</th>");
    fprintf(OUTPUT_, "<td class='noborder'>");
    for (int i = 0; i < scriptcount; i ++) {
        fprintf(OUTPUT_, "%s", uscript_getName(script[i]));
        if (i + 1 != scriptcount) {
            fprintf(OUTPUT_, ", ");
        }
    }
    fprintf(OUTPUT_, "</td></tr>\n");
    
    fprintf(OUTPUT_, "<tr><th>Rules</th><td class='noborder'><a href=\"http://dev.icu-project.org/cgi-bin/viewcvs.cgi/*checkout*/icu/source/data/coll/%s.txt\">%s.txt</a></td></tr>\n", locale, locale);
    
    UVersionInfo version;
    ucol_getVersion(COLLATOR_, version);
    fprintf(OUTPUT_, "<tr><th>Collator version</th><td class='noborder'>%d.%d.%d.%d</td></tr>\n", 
                      version[0], version[1], version[2], version[3]);
    
    UColAttribute attr = UCOL_FRENCH_COLLATION;
    while (attr < UCOL_ATTRIBUTE_COUNT) {
        UColAttributeValue value = ucol_getAttribute(COLLATOR_, attr, &error);
        if (U_FAILURE(error)) {
            fprintf(stdout, "Error getting attribute\n");
            return;
        }
        if (value != UCOL_DEFAULT) {
            if (attr == UCOL_FRENCH_COLLATION && value != UCOL_OFF) {
                fprintf(OUTPUT_, "<tr><th>French Collation</th><td class='noborder'>on, code %d</td></tr>\n", value);
            }
            if (attr == UCOL_ALTERNATE_HANDLING && value != UCOL_NON_IGNORABLE) {
                fprintf(OUTPUT_, "<tr><th>Alternate Handling</th><td class='noborder'>shifted, code%d</td></tr>\n", value);
            }
            if (attr == UCOL_CASE_FIRST && value != UCOL_OFF) {
                fprintf(OUTPUT_, "<tr><th>Case First</th><td class='noborder'>on, code %d</td></tr>\n", value);
            }
            if (attr == UCOL_CASE_LEVEL && value != UCOL_OFF) {
                fprintf(OUTPUT_, "<tr><th>Case Level</th><td class='noborder'>on, code %d</td></tr>\n", value);
            }
            if (attr == UCOL_NORMALIZATION_MODE && value != UCOL_OFF) {
                fprintf(OUTPUT_, "<tr><th>Normalization</th><td class='noborder'>on, code %d</td></tr>\n", value);
            }
            if (attr == UCOL_STRENGTH && value != UCOL_TERTIARY) {
                fprintf(OUTPUT_, "<tr><th>Strength</th><td class='noborder'>code %d</td></tr>\n", value);
            }
            if (attr == UCOL_HIRAGANA_QUATERNARY_MODE && value != UCOL_OFF) {
                fprintf(OUTPUT_, "<tr><th>Hiragana Quaternary</th><td class='noborder'>on, code %d</td></tr>\n", value);
            }
        }
        attr = (UColAttribute)(attr + 1);
    }

    // Get UNIX-style time and display as number and string.
    time_t ltime;
    time( &ltime );
    fprintf(OUTPUT_, "<tr><th>Date Generated</th><td class='noborder'>%s</td></tr>", ctime(&ltime));
     
    fprintf(OUTPUT_, "</table>\n");

    fprintf(OUTPUT_, "<p><a href=help.html>How to read the table</a><br>\n");
    fprintf(OUTPUT_, "<a href=http://www.jtcsv.com/cgi-bin/icu-bugs/ target=new>Submit a bug</a></p>\n");
    fprintf(OUTPUT_, "\n<table>\n");
    fprintf(OUTPUT_, "\n<tr><th>Codepoint</th><th>P</th><th>S</th><th>T</th><th>Q</th><th>Name</th></tr>\n");
}

/**
* Prints the header for index.html
* @param file output file
*/
void outputListHTMLHeader(FILE *file)
{
    fprintf(file, "<html>\n");
    fprintf(file, "<head>\n");
    fprintf(file, "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">\n");
    fprintf(file, "<meta http-equiv=\"Content-Language\" content=\"en-us\">\n");
    fprintf(file, "<title>ICU Collation Charts</title>\n");
    fprintf(file, "<base target=\"main\">\n");
    fprintf(file, "</head>\n");
    fprintf(file, "<body bgcolor=#FFFFFF>\n");
    fprintf(file, "<h2 align=center>ICU Collation Charts</h2>\n");
    fprintf(file, "<p align=center>\n");
    fprintf(file, "<a href=http://www.unicode.org/charts/collation/ target=new>UCA Charts</a><br>");
}

/**
* Prints the footer for index.html
* @param file output file
*/
void outputListHTMLFooter(FILE *file)
{
    fprintf(file, "</p>\n");
	//fprintf(file, "<center><image src=http://oss.software.ibm.com/icu/images/w24.gif></center>\n");
    fprintf(file, "</body>\n");
    fprintf(file, "</html>\n");
}

/**
* Gets all scripts and serialize their codepoints into an html file.
*/
void serializeScripts() {
    char filename[128];
    int  dirlength = 0;

    if (options[4].doesOccur) {
        strcpy(filename, options[4].value);
        dirlength = appendDirSeparator(filename);
    } else {
      filename[0] = 0;
    }

    const char    *locale;
          int32_t  localelist = 0;
          int32_t  localesize;
        
    localesize = ucol_countAvailable();
    locale     = ucol_getAvailable(localelist);

    strcat(filename, "list.html");
    FILE *list = fopen(filename, "w");
    filename[dirlength] = 0;
    if (list == NULL) {
        fprintf(stdout, "Cannot open file: %s\n", filename);
        return;
    }

    outputListHTMLHeader(list);
    fprintf(list, "<blockquote>\n");
    while (TRUE) {
        UErrorCode error = U_ZERO_ERROR;
        COLLATOR_ = ucol_open(locale, &error);
        if (U_FAILURE(error)) {
            fprintf(stdout, "Collator creation failed:");
            fprintf(stdout, u_errorName(error));
            return;
        }
        if ((error != U_USING_FALLBACK_WARNING && // not tailored
            error != U_USING_DEFAULT_WARNING) ||
            checkLocaleForLanguage(locale)) {
            fprintf(list, "<a href=%s.html>%s</a> ", locale, locale);
	        setAttributes(COLLATOR_, &error);
            if (U_FAILURE(error)) {
               fprintf(stdout, "Collator attribute setting failed:");
               fprintf(stdout, u_errorName(error));
               return;
            }

            UScriptCode scriptcode[32];
            uint32_t scriptcount = uscript_getCode(locale, scriptcode, 32, 
                                                   &error);
            if (U_FAILURE(error)) {
                fprintf(stdout, "Error getting lcale scripts\n");
                return;
            }

            strcat(filename, locale);
            strcat(filename, ".html");
            OUTPUT_ = fopen(filename, "w");
            if (OUTPUT_ == NULL) {
                fprintf(stdout, "Cannot open file:%s\n", filename);
                return;
            }
            outputHTMLHeader(locale, scriptcode, scriptcount);
			fprintf(stdout, "%s\n", locale);
			
            if(options[12].doesOccur) {
              // use whole scripts
			  serializeScripts(scriptcode, scriptcount); 
            } else {
              // use exemplar chars
              serializeScripts(scriptcode, scriptcount, locale);
            }
            fclose(OUTPUT_);
        }
        ucol_close(COLLATOR_);

        filename[dirlength] = 0;
        localelist ++;
        if (localelist == localesize) {
            break;
        }
        locale = ucol_getAvailable(localelist);
    }
    fprintf(list, "<br><a href=help.html>help</a><br>");
    fprintf(list, "</blockquote>\n");
    outputListHTMLFooter(list);
    fclose(list);
}

/** 
* Main   --  process command line, read in and pre-process the test file,
*            call other functions to do the actual tests.
*/
int main(int argc, char *argv[]) {
    
    argc = u_parseArgs(argc, argv, sizeof(options)/sizeof(options[0]), 
                       options);
    
    // error handling, printing usage message
    if (argc < 0) {
        fprintf(stdout, "error in command line argument: ");
        fprintf(stdout, argv[-argc]);
        fprintf(stdout, "\n");
    }
    if (argc < 0 || options[0].doesOccur || options[1].doesOccur) {
        fprintf(stdout, "Usage: dumpce options...\n"
                        "--help\n"
                        "    Display this message.\n"
                        "--locale name|all\n"
                        "    ICU locale to use. Default is en_US\n"
                        "--serialize\n"
                        "    Serializes the collation elements in -locale or all locales available and outputs them into --outputdir/locale_ce.txt\n"
                        "--destdir dir_name\n"
                        "    Path for outputing the serialized collation elements. Defaults to stdout if no defined\n"
                        "--sourcedir dir_name\n"
                        "    Path for the input rule file for collation\n"
                        "--attribute name=value,name=value...\n" 
                        "    Pairs of attribute names and values for setting\n"
                        "--rule filename\n" 
                        "    Name of file containing the collation rules.\n"
                        "--normalizaton mode\n" 
                        "    UNormalizationMode mode to be used.\n"
                        "--scripts\n" 
                        "    Codepoints from all scripts are sorted and serialized.\n"
                        "--reducehan\n" 
                        "    Only 200 Han script characters will be displayed with the use of --scripts.\n"
                        "--wholescripts\n"
                        "    Show collation order for whole scripts instead of just for exemplar characters of a locale\n\n");

        fprintf(stdout, "Example to generate *.txt files : dumpce --serialize --locale af --destdir /temp --attribute UCOL_STRENGTH=UCOL_DEFAULT_STRENGTH,4=17\n\n");
        fprintf(stdout, "Example to generate *.html files for oss web display: dumpce --scripts --destdir /temp --reducehan\n");
        return argc < 0 ? U_ILLEGAL_ARGUMENT_ERROR : U_ZERO_ERROR;
    }

    OUTPUT_ = stdout;
    if (options[6].doesOccur) {
        fprintf(stdout, "attributes %s\n", options[6].value);
        parseAttributes();
    }
    if (options[3].doesOccur) {
        serialize();
    }
    if (options[9].doesOccur) {
        serializeScripts();
    }
    return 0;
}
