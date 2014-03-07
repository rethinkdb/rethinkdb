/*
 *******************************************************************************
 *   Copyright (C) 2003-2007, International Business Machines
 *   Corporation and others.  All Rights Reserved.
 *******************************************************************************
 *
 * File prscmnts.cpp
 *
 * Modification History:
 *
 *   Date          Name        Description
 *   08/22/2003    ram         Creation.
 *******************************************************************************
 */

#include "unicode/regex.h"
#include "unicode/unistr.h"
#include "unicode/parseerr.h"
#include "prscmnts.h"
#include <stdio.h>
#include <stdlib.h>

U_NAMESPACE_USE

#if UCONFIG_NO_REGULAR_EXPRESSIONS==0 /* donot compile when RegularExpressions not available */

#define MAX_SPLIT_STRINGS 20

const char *patternStrings[UPC_LIMIT]={
    "^translate\\s*(.*)",
    "^note\\s*(.*)"
};

U_CFUNC int32_t 
removeText(UChar *source, int32_t srcLen, 
           UnicodeString patString,uint32_t options,  
           UnicodeString replaceText, UErrorCode *status){

    if(status == NULL || U_FAILURE(*status)){
        return 0;
    }

    UnicodeString src(source, srcLen);

    RegexMatcher    myMatcher(patString, src, options, *status);
    if(U_FAILURE(*status)){
        return 0;
    }
    UnicodeString dest;


    dest = myMatcher.replaceAll(replaceText,*status);
    
    
    return dest.extract(source, srcLen, *status);

}
U_CFUNC int32_t
trim(UChar *src, int32_t srcLen, UErrorCode *status){
     srcLen = removeText(src, srcLen, "^[ \\r\\n]+ ", 0, "", status); // remove leading new lines
     srcLen = removeText(src, srcLen, "^\\s+", 0, "", status); // remove leading spaces
     srcLen = removeText(src, srcLen, "\\s+$", 0, "", status); // remvoe trailing spcaes
     return srcLen;
}

U_CFUNC int32_t 
removeCmtText(UChar* source, int32_t srcLen, UErrorCode* status){
    srcLen = trim(source, srcLen, status);
    UnicodeString     patString = "^\\s*?\\*\\s*?";     // remove pattern like " * " at the begining of the line
    srcLen = removeText(source, srcLen, patString, UREGEX_MULTILINE, "", status);
    return removeText(source, srcLen, "[ \\r\\n]+", 0, " ", status);// remove new lines;
}

U_CFUNC int32_t 
getText(const UChar* source, int32_t srcLen,
        UChar** dest, int32_t destCapacity,
        UnicodeString patternString, 
        UErrorCode* status){
    
    if(status == NULL || U_FAILURE(*status)){
        return 0;
    }

    UnicodeString     stringArray[MAX_SPLIT_STRINGS];
    RegexPattern      *pattern = RegexPattern::compile("@", 0, *status);
    UnicodeString src (source,srcLen);
    
    if (U_FAILURE(*status)) {
        return 0;
    }
    pattern->split(src, stringArray, MAX_SPLIT_STRINGS, *status);
    
    RegexMatcher matcher(patternString, UREGEX_DOTALL, *status);
    if (U_FAILURE(*status)) {
        return 0;
    }
    for(int32_t i=0; i<MAX_SPLIT_STRINGS; i++){
        matcher.reset(stringArray[i]);
        if(matcher.lookingAt(*status)){
            UnicodeString out = matcher.group(1, *status);

            return out.extract(*dest, destCapacity,*status);
        }
    }
    return 0;
}


#define AT_SIGN  0x0040

U_CFUNC int32_t
getDescription( const UChar* source, int32_t srcLen,
                UChar** dest, int32_t destCapacity,
                UErrorCode* status){
    if(status == NULL || U_FAILURE(*status)){
        return 0;
    }

    UnicodeString     stringArray[MAX_SPLIT_STRINGS];
    RegexPattern      *pattern = RegexPattern::compile("@", UREGEX_MULTILINE, *status);
    UnicodeString src(source, srcLen);
    
    if (U_FAILURE(*status)) {
        return 0;
    }
    pattern->split(src, stringArray,MAX_SPLIT_STRINGS , *status);

    if(stringArray[0].indexOf((UChar)AT_SIGN)==-1){
        int32_t destLen =  stringArray[0].extract(*dest, destCapacity, *status);
        return trim(*dest, destLen, status);
    }
    return 0;
}

U_CFUNC int32_t
getCount(const UChar* source, int32_t srcLen, 
         UParseCommentsOption option, UErrorCode *status){
    
    if(status == NULL || U_FAILURE(*status)){
        return 0;
    }

    UnicodeString     stringArray[MAX_SPLIT_STRINGS];
    RegexPattern      *pattern = RegexPattern::compile("@", UREGEX_MULTILINE, *status);
    UnicodeString src (source, srcLen);


    if (U_FAILURE(*status)) {
        return 0;
    }
    int32_t retLen = pattern->split(src, stringArray, MAX_SPLIT_STRINGS, *status);
    
    RegexMatcher matcher(patternStrings[option], UREGEX_DOTALL, *status);
    if (U_FAILURE(*status)) {
        return 0;
    } 
    int32_t count = 0;
    for(int32_t i=0; i<retLen; i++){
        matcher.reset(stringArray[i]);
        if(matcher.lookingAt(*status)){
            count++;
        }
    }
    if(option == UPC_TRANSLATE && count > 1){
        fprintf(stderr, "Multiple @translate tags cannot be supported.\n");
        exit(U_UNSUPPORTED_ERROR);
    }
    return count;
}

U_CFUNC int32_t 
getAt(const UChar* source, int32_t srcLen,
        UChar** dest, int32_t destCapacity,
        int32_t index,
        UParseCommentsOption option,
        UErrorCode* status){

    if(status == NULL || U_FAILURE(*status)){
        return 0;
    }

    UnicodeString     stringArray[MAX_SPLIT_STRINGS];
    RegexPattern      *pattern = RegexPattern::compile("@", UREGEX_MULTILINE, *status);
    UnicodeString src (source, srcLen);


    if (U_FAILURE(*status)) {
        return 0;
    }
    int32_t retLen = pattern->split(src, stringArray, MAX_SPLIT_STRINGS, *status);
    
    RegexMatcher matcher(patternStrings[option], UREGEX_DOTALL, *status);
    if (U_FAILURE(*status)) {
        return 0;
    } 
    int32_t count = 0;
    for(int32_t i=0; i<retLen; i++){
        matcher.reset(stringArray[i]);
        if(matcher.lookingAt(*status)){
            if(count == index){
                UnicodeString out = matcher.group(1, *status);
                return out.extract(*dest, destCapacity,*status);
            }
            count++;
            
        }
    }
    return 0;

}

U_CFUNC int32_t
getTranslate( const UChar* source, int32_t srcLen,
              UChar** dest, int32_t destCapacity,
              UErrorCode* status){
    UnicodeString     notePatternString = "^translate\\s*?(.*)"; 
    
    int32_t destLen = getText(source, srcLen, dest, destCapacity, notePatternString, status);
    return trim(*dest, destLen, status);
}

U_CFUNC int32_t 
getNote(const UChar* source, int32_t srcLen,
        UChar** dest, int32_t destCapacity,
        UErrorCode* status){

    UnicodeString     notePatternString = "^note\\s*?(.*)"; 
    int32_t destLen =  getText(source, srcLen, dest, destCapacity, notePatternString, status);
    return trim(*dest, destLen, status);

}

#endif /* UCONFIG_NO_REGULAR_EXPRESSIONS */

