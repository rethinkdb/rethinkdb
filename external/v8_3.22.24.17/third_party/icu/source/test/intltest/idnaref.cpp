/*
 *******************************************************************************
 *
 *   Copyright (C) 2003-2007, International Business Machines
 *   Corporation and others.  All Rights Reserved.
 *
 *******************************************************************************
 *   file name:  idnaref.cpp
 *   encoding:   US-ASCII
 *   tab size:   8 (not used)
 *   indentation:4
 *
 *   created on: 2003feb1
 *   created by: Ram Viswanadha
 */

#include "unicode/utypes.h"

#if !UCONFIG_NO_IDNA && !UCONFIG_NO_TRANSLITERATION
#include "idnaref.h"
#include "punyref.h"
#include "ustr_imp.h"
#include "cmemory.h"
#include "sprpimpl.h"
#include "nptrans.h"
#include "testidna.h"
#include "punycode.h"
#include "unicode/ustring.h"

/* it is official IDNA ACE Prefix is "xn--" */
static const UChar ACE_PREFIX[] ={ 0x0078,0x006E,0x002d,0x002d } ;
#define ACE_PREFIX_LENGTH 4

#define MAX_LABEL_LENGTH 63
#define HYPHEN      0x002D
/* The Max length of the labels should not be more than 64 */
#define MAX_LABEL_BUFFER_SIZE 100
#define MAX_IDN_BUFFER_SIZE   300

#define CAPITAL_A        0x0041
#define CAPITAL_Z        0x005A
#define LOWER_CASE_DELTA 0x0020
#define FULL_STOP        0x002E


inline static UBool
startsWithPrefix(const UChar* src , int32_t srcLength){
    UBool startsWithPrefix = TRUE;

    if(srcLength < ACE_PREFIX_LENGTH){
        return FALSE;
    }

    for(int8_t i=0; i< ACE_PREFIX_LENGTH; i++){
        if(u_tolower(src[i]) != ACE_PREFIX[i]){
            startsWithPrefix = FALSE;
        }
    }
    return startsWithPrefix;
}

inline static UChar
toASCIILower(UChar ch){
    if(CAPITAL_A <= ch && ch <= CAPITAL_Z){
        return ch + LOWER_CASE_DELTA;
    }
    return ch;
}

inline static int32_t
compareCaseInsensitiveASCII(const UChar* s1, int32_t s1Len,
                            const UChar* s2, int32_t s2Len){
    if(s1Len != s2Len){
        return (s1Len > s2Len) ? s1Len : s2Len;
    }
    UChar c1,c2;
    int32_t rc;

    for(int32_t i =0;/* no condition */;i++) {
        /* If we reach the ends of both strings then they match */
        if(i == s1Len) {
            return 0;
        }

        c1 = s1[i];
        c2 = s2[i];

        /* Case-insensitive comparison */
        if(c1!=c2) {
            rc=(int32_t)toASCIILower(c1)-(int32_t)toASCIILower(c2);
            if(rc!=0) {
                return rc;
            }
        }
    }

}

static UErrorCode getError(enum punycode_status status){
    switch(status){
    case punycode_success:
        return U_ZERO_ERROR;
    case punycode_bad_input:   /* Input is invalid.                         */
        return U_INVALID_CHAR_FOUND;
    case punycode_big_output:  /* Output would exceed the space provided.   */
        return U_BUFFER_OVERFLOW_ERROR;
    case punycode_overflow :    /* Input requires wider integers to process. */
        return U_INDEX_OUTOFBOUNDS_ERROR;
    default:
        return U_INTERNAL_PROGRAM_ERROR;
    }
}

static inline int32_t convertASCIIToUChars(const char* src,UChar* dest, int32_t length){
    int i;
    for(i=0;i<length;i++){
        dest[i] = src[i];
    }
    return i;
}
static inline int32_t convertUCharsToASCII(const UChar* src,char* dest, int32_t length){
    int i;
    for(i=0;i<length;i++){
        dest[i] = (char)src[i];
    }
    return i;
}
// wrapper around the reference Punycode implementation
static int32_t convertToPuny(const UChar* src, int32_t srcLength,
                             UChar* dest, int32_t destCapacity,
                             UErrorCode& status){
    uint32_t b1Stack[MAX_LABEL_BUFFER_SIZE];
    int32_t b1Len = 0, b1Capacity = MAX_LABEL_BUFFER_SIZE;
    uint32_t* b1 = b1Stack;
    char b2Stack[MAX_LABEL_BUFFER_SIZE];
    char* b2 = b2Stack;
    int32_t b2Len =MAX_LABEL_BUFFER_SIZE ;
    punycode_status error;
    unsigned char* caseFlags = NULL;

    u_strToUTF32((UChar32*)b1,b1Capacity,&b1Len,src,srcLength,&status);
    if(status == U_BUFFER_OVERFLOW_ERROR){
        // redo processing of string
        /* we do not have enough room so grow the buffer*/
        b1 =  (uint32_t*) uprv_malloc(b1Len * sizeof(uint32_t));
        if(b1==NULL){
            status = U_MEMORY_ALLOCATION_ERROR;
            goto CLEANUP;
        }

        status = U_ZERO_ERROR; // reset error

        u_strToUTF32((UChar32*)b1,b1Len,&b1Len,src,srcLength,&status);
    }
    if(U_FAILURE(status)){
        goto CLEANUP;
    }

    //caseFlags = (unsigned char*) uprv_malloc(b1Len *sizeof(unsigned char));

    error = punycode_encode(b1Len,b1,caseFlags, (uint32_t*)&b2Len, b2);
    status = getError(error);

    if(status == U_BUFFER_OVERFLOW_ERROR){
        /* we do not have enough room so grow the buffer*/
        b2 = (char*) uprv_malloc( b2Len * sizeof(char));
        if(b2==NULL){
            status = U_MEMORY_ALLOCATION_ERROR;
            goto CLEANUP;
        }

        status = U_ZERO_ERROR; // reset error

        punycode_status error = punycode_encode(b1Len,b1,caseFlags, (uint32_t*)&b2Len, b2);
        status = getError(error);
    }
    if(U_FAILURE(status)){
        goto CLEANUP;
    }

    if(b2Len < destCapacity){
          convertASCIIToUChars(b2,dest,b2Len);
    }else{
        status =U_BUFFER_OVERFLOW_ERROR;
    }

CLEANUP:
    if(b1Stack != b1){
        uprv_free(b1);
    }
    if(b2Stack != b2){
        uprv_free(b2);
    }
    uprv_free(caseFlags);

    return b2Len;
}

static int32_t convertFromPuny(  const UChar* src, int32_t srcLength,
                                 UChar* dest, int32_t destCapacity,
                                 UErrorCode& status){
    char b1Stack[MAX_LABEL_BUFFER_SIZE];
    char* b1 = b1Stack;
    int32_t destLen =0;

    convertUCharsToASCII(src, b1,srcLength);

    uint32_t b2Stack[MAX_LABEL_BUFFER_SIZE];
    uint32_t* b2 = b2Stack;
    int32_t b2Len =MAX_LABEL_BUFFER_SIZE;
    unsigned char* caseFlags = NULL; //(unsigned char*) uprv_malloc(srcLength * sizeof(unsigned char*));
    punycode_status error = punycode_decode(srcLength,b1,(uint32_t*)&b2Len,b2,caseFlags);
    status = getError(error);
    if(status == U_BUFFER_OVERFLOW_ERROR){
        b2 =  (uint32_t*) uprv_malloc(b2Len * sizeof(uint32_t));
        if(b2 == NULL){
            status = U_MEMORY_ALLOCATION_ERROR;
            goto CLEANUP;
        }
        error = punycode_decode(srcLength,b1,(uint32_t*)&b2Len,b2,caseFlags);
        status = getError(error);
    }

    if(U_FAILURE(status)){
        goto CLEANUP;
    }

    u_strFromUTF32(dest,destCapacity,&destLen,(UChar32*)b2,b2Len,&status);

CLEANUP:
    if(b1Stack != b1){
        uprv_free(b1);
    }
    if(b2Stack != b2){
        uprv_free(b2);
    }
    uprv_free(caseFlags);

    return destLen;
}


U_CFUNC int32_t U_EXPORT2
idnaref_toASCII(const UChar* src, int32_t srcLength,
              UChar* dest, int32_t destCapacity,
              int32_t options,
              UParseError* parseError,
              UErrorCode* status){

    if(status == NULL || U_FAILURE(*status)){
        return 0;
    }
    if((src == NULL) || (srcLength < -1) || (destCapacity<0) || (!dest && destCapacity > 0)){
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }
    UChar b1Stack[MAX_LABEL_BUFFER_SIZE], b2Stack[MAX_LABEL_BUFFER_SIZE];
    //initialize pointers to stack buffers
    UChar  *b1 = b1Stack, *b2 = b2Stack;
    int32_t b1Len=0, b2Len=0,
            b1Capacity = MAX_LABEL_BUFFER_SIZE,
            b2Capacity = MAX_LABEL_BUFFER_SIZE ,
            reqLength=0;

    //get the options
    UBool allowUnassigned   = (UBool)((options & IDNAREF_ALLOW_UNASSIGNED) != 0);
    UBool useSTD3ASCIIRules = (UBool)((options & IDNAREF_USE_STD3_RULES) != 0);

    UBool* caseFlags = NULL;

    // assume the source contains all ascii codepoints
    UBool srcIsASCII  = TRUE;
    // assume the source contains all LDH codepoints
    UBool srcIsLDH = TRUE;
    int32_t j=0;

    if(srcLength == -1){
        srcLength = u_strlen(src);
    }

    // step 1
    for( j=0;j<srcLength;j++){
        if(src[j] > 0x7F){
            srcIsASCII = FALSE;
        }
        b1[b1Len++] = src[j];
    }
    // step 2
    NamePrepTransform* prep = TestIDNA::getInstance(*status);

    if(U_FAILURE(*status)){
        goto CLEANUP;
    }

    b1Len = prep->process(src,srcLength,b1, b1Capacity,allowUnassigned,parseError,*status);

    if(*status == U_BUFFER_OVERFLOW_ERROR){
        // redo processing of string
        /* we do not have enough room so grow the buffer*/
        b1 = (UChar*) uprv_malloc(b1Len * U_SIZEOF_UCHAR);
        if(b1==NULL){
            *status = U_MEMORY_ALLOCATION_ERROR;
            goto CLEANUP;
        }

        *status = U_ZERO_ERROR; // reset error

        b1Len = prep->process(src,srcLength,b1, b1Len,allowUnassigned, parseError, *status);
    }
    // error bail out
    if(U_FAILURE(*status)){
        goto CLEANUP;
    }

    if(b1Len == 0){
        *status = U_IDNA_ZERO_LENGTH_LABEL_ERROR;
        goto CLEANUP;
    }

    srcIsASCII = TRUE;
    // step 3 & 4
    for( j=0;j<b1Len;j++){
        if(b1[j] > 0x7F){// check if output of usprep_prepare is all ASCII
            srcIsASCII = FALSE;
        }else if(prep->isLDHChar(b1[j])==FALSE){  // if the char is in ASCII range verify that it is an LDH character{
            srcIsLDH = FALSE;
        }
    }

    if(useSTD3ASCIIRules == TRUE){
        // verify 3a and 3b
        if( srcIsLDH == FALSE /* source contains some non-LDH characters */
            || b1[0] ==  HYPHEN || b1[b1Len-1] == HYPHEN){
            *status = U_IDNA_STD3_ASCII_RULES_ERROR;
            goto CLEANUP;
        }
    }
    if(srcIsASCII){
        if(b1Len <= destCapacity){
            uprv_memmove(dest, b1, b1Len * U_SIZEOF_UCHAR);
            reqLength = b1Len;
        }else{
            reqLength = b1Len;
            goto CLEANUP;
        }
    }else{
        // step 5 : verify the sequence does not begin with ACE prefix
        if(!startsWithPrefix(b1,b1Len)){

            //step 6: encode the sequence with punycode
            //caseFlags = (UBool*) uprv_malloc(b1Len * sizeof(UBool));

            b2Len = convertToPuny(b1,b1Len, b2,b2Capacity,*status);
            //b2Len = u_strToPunycode(b2,b2Capacity,b1,b1Len, caseFlags, status);
            if(*status == U_BUFFER_OVERFLOW_ERROR){
                // redo processing of string
                /* we do not have enough room so grow the buffer*/
                b2 = (UChar*) uprv_malloc(b2Len * U_SIZEOF_UCHAR);
                if(b2 == NULL){
                    *status = U_MEMORY_ALLOCATION_ERROR;
                    goto CLEANUP;
                }

                *status = U_ZERO_ERROR; // reset error

                b2Len = convertToPuny(b1, b1Len, b2, b2Len, *status);
                //b2Len = u_strToPunycode(b2,b2Len,b1,b1Len, caseFlags, status);

            }
            //error bail out
            if(U_FAILURE(*status)){
                goto CLEANUP;
            }
            reqLength = b2Len+ACE_PREFIX_LENGTH;

            if(reqLength > destCapacity){
                *status = U_BUFFER_OVERFLOW_ERROR;
                goto CLEANUP;
            }
            //Step 7: prepend the ACE prefix
            uprv_memcpy(dest,ACE_PREFIX,ACE_PREFIX_LENGTH * U_SIZEOF_UCHAR);
            //Step 6: copy the contents in b2 into dest
            uprv_memcpy(dest+ACE_PREFIX_LENGTH, b2, b2Len * U_SIZEOF_UCHAR);

        }else{
            *status = U_IDNA_ACE_PREFIX_ERROR;
            goto CLEANUP;
        }
    }

    if(reqLength > MAX_LABEL_LENGTH){
        *status = U_IDNA_LABEL_TOO_LONG_ERROR;
    }

CLEANUP:
    if(b1 != b1Stack){
        uprv_free(b1);
    }
    if(b2 != b2Stack){
        uprv_free(b2);
    }
    uprv_free(caseFlags);

//    delete prep;

    return u_terminateUChars(dest, destCapacity, reqLength, status);
}


U_CFUNC int32_t U_EXPORT2
idnaref_toUnicode(const UChar* src, int32_t srcLength,
                UChar* dest, int32_t destCapacity,
                int32_t options,
                UParseError* parseError,
                UErrorCode* status){

    if(status == NULL || U_FAILURE(*status)){
        return 0;
    }
    if((src == NULL) || (srcLength < -1) || (destCapacity<0) || (!dest && destCapacity > 0)){
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }



    UChar b1Stack[MAX_LABEL_BUFFER_SIZE], b2Stack[MAX_LABEL_BUFFER_SIZE], b3Stack[MAX_LABEL_BUFFER_SIZE];

    //initialize pointers to stack buffers
    UChar  *b1 = b1Stack, *b2 = b2Stack, *b1Prime=NULL, *b3=b3Stack;
    int32_t b1Len, b2Len, b1PrimeLen, b3Len,
            b1Capacity = MAX_LABEL_BUFFER_SIZE,
            b2Capacity = MAX_LABEL_BUFFER_SIZE,
            b3Capacity = MAX_LABEL_BUFFER_SIZE,
            reqLength=0;
//    UParseError parseError;

    NamePrepTransform* prep = TestIDNA::getInstance(*status);
    b1Len = 0;
    UBool* caseFlags = NULL;

    //get the options
    UBool allowUnassigned   = (UBool)((options & IDNAREF_ALLOW_UNASSIGNED) != 0);
    UBool useSTD3ASCIIRules = (UBool)((options & IDNAREF_USE_STD3_RULES) != 0);

    UBool srcIsASCII = TRUE;
    UBool srcIsLDH = TRUE;
    int32_t failPos =0;

    if(U_FAILURE(*status)){
        goto CLEANUP;
    }
    // step 1: find out if all the codepoints in src are ASCII
    if(srcLength==-1){
        srcLength = 0;
        for(;src[srcLength]!=0;){
            if(src[srcLength]> 0x7f){
                srcIsASCII = FALSE;
            }if(prep->isLDHChar(src[srcLength])==FALSE){
                // here we do not assemble surrogates
                // since we know that LDH code points
                // are in the ASCII range only
                srcIsLDH = FALSE;
                failPos = srcLength;
            }
            srcLength++;
        }
    }else{
        for(int32_t j=0; j<srcLength; j++){
            if(src[j]> 0x7f){
                srcIsASCII = FALSE;
            }else if(prep->isLDHChar(src[j])==FALSE){
                // here we do not assemble surrogates
                // since we know that LDH code points
                // are in the ASCII range only
                srcIsLDH = FALSE;
                failPos = j;
            }
        }
    }

    if(srcIsASCII == FALSE){
        // step 2: process the string
        b1Len = prep->process(src,srcLength,b1,b1Capacity,allowUnassigned, parseError, *status);
        if(*status == U_BUFFER_OVERFLOW_ERROR){
            // redo processing of string
            /* we do not have enough room so grow the buffer*/
            b1 = (UChar*) uprv_malloc(b1Len * U_SIZEOF_UCHAR);
            if(b1==NULL){
                *status = U_MEMORY_ALLOCATION_ERROR;
                goto CLEANUP;
            }

            *status = U_ZERO_ERROR; // reset error

            b1Len = prep->process(src,srcLength,b1, b1Len,allowUnassigned, parseError, *status);
        }
        //bail out on error
        if(U_FAILURE(*status)){
            goto CLEANUP;
        }
    }else{

        // copy everything to b1
        if(srcLength < b1Capacity){
            uprv_memmove(b1,src, srcLength * U_SIZEOF_UCHAR);
        }else{
            /* we do not have enough room so grow the buffer*/
            b1 = (UChar*) uprv_malloc(srcLength * U_SIZEOF_UCHAR);
            if(b1==NULL){
                *status = U_MEMORY_ALLOCATION_ERROR;
                goto CLEANUP;
            }
            uprv_memmove(b1,src, srcLength * U_SIZEOF_UCHAR);
        }
        b1Len = srcLength;
    }
    //step 3: verify ACE Prefix
    if(startsWithPrefix(src,srcLength)){

        //step 4: Remove the ACE Prefix
        b1Prime = b1 + ACE_PREFIX_LENGTH;
        b1PrimeLen  = b1Len - ACE_PREFIX_LENGTH;

        //step 5: Decode using punycode
        b2Len = convertFromPuny(b1Prime,b1PrimeLen, b2, b2Capacity, *status);
        //b2Len = u_strFromPunycode(b2, b2Capacity,b1Prime,b1PrimeLen, caseFlags, status);

        if(*status == U_BUFFER_OVERFLOW_ERROR){
            // redo processing of string
            /* we do not have enough room so grow the buffer*/
            b2 = (UChar*) uprv_malloc(b2Len * U_SIZEOF_UCHAR);
            if(b2==NULL){
                *status = U_MEMORY_ALLOCATION_ERROR;
                goto CLEANUP;
            }

            *status = U_ZERO_ERROR; // reset error

            b2Len =  convertFromPuny(b1Prime,b1PrimeLen, b2, b2Len, *status);
            //b2Len = u_strFromPunycode(b2, b2Len,b1Prime,b1PrimeLen,caseFlags, status);
        }


        //step 6:Apply toASCII
        b3Len = idnaref_toASCII(b2,b2Len,b3,b3Capacity,options,parseError, status);

        if(*status == U_BUFFER_OVERFLOW_ERROR){
            // redo processing of string
            /* we do not have enough room so grow the buffer*/
            b3 = (UChar*) uprv_malloc(b3Len * U_SIZEOF_UCHAR);
            if(b3==NULL){
                *status = U_MEMORY_ALLOCATION_ERROR;
                goto CLEANUP;
            }

            *status = U_ZERO_ERROR; // reset error

            b3Len =  idnaref_toASCII(b2,b2Len,b3,b3Len, options, parseError, status);

        }
        //bail out on error
        if(U_FAILURE(*status)){
            goto CLEANUP;
        }

        //step 7: verify
        if(compareCaseInsensitiveASCII(b1, b1Len, b3, b3Len) !=0){
            *status = U_IDNA_VERIFICATION_ERROR;
            goto CLEANUP;
        }

        //step 8: return output of step 5
        reqLength = b2Len;
        if(b2Len <= destCapacity) {
            uprv_memmove(dest, b2, b2Len * U_SIZEOF_UCHAR);
        }
    }else{
        // verify that STD3 ASCII rules are satisfied
        if(useSTD3ASCIIRules == TRUE){
            if( srcIsLDH == FALSE /* source contains some non-LDH characters */
                || src[0] ==  HYPHEN || src[srcLength-1] == HYPHEN){
                *status = U_IDNA_STD3_ASCII_RULES_ERROR;

                /* populate the parseError struct */
                if(srcIsLDH==FALSE){
                    // failPos is always set the index of failure
                    uprv_syntaxError(src,failPos, srcLength,parseError);
                }else if(src[0] == HYPHEN){
                    // fail position is 0
                    uprv_syntaxError(src,0,srcLength,parseError);
                }else{
                    // the last index in the source is always length-1
                    uprv_syntaxError(src, (srcLength>0) ? srcLength-1 : srcLength, srcLength,parseError);
                }

                goto CLEANUP;
            }
        }
        //copy the source to destination
        if(srcLength <= destCapacity){
            uprv_memmove(dest,src,srcLength * U_SIZEOF_UCHAR);
        }
        reqLength = srcLength;
    }

CLEANUP:

    if(b1 != b1Stack){
        uprv_free(b1);
    }
    if(b2 != b2Stack){
        uprv_free(b2);
    }
    uprv_free(caseFlags);

    // The RFC states that 
    // <quote>
    // ToUnicode never fails. If any step fails, then the original input
    // is returned immediately in that step.
    // </quote>
    // So if any step fails lets copy source to destination
    if(U_FAILURE(*status)){
        //copy the source to destination
        if(dest && srcLength <= destCapacity){
          if(srcLength == -1) {
            uprv_memmove(dest,src,u_strlen(src)* U_SIZEOF_UCHAR);
          } else {
            uprv_memmove(dest,src,srcLength * U_SIZEOF_UCHAR);
          }
        }
        reqLength = srcLength;
        *status = U_ZERO_ERROR;
    }
    return u_terminateUChars(dest, destCapacity, reqLength, status);
}


static int32_t
getNextSeparator(UChar *src,int32_t srcLength,NamePrepTransform* prep,
                 UChar **limit,
                 UBool *done,
                 UErrorCode *status){
    if(srcLength == -1){
        int32_t i;
        for(i=0 ; ;i++){
            if(src[i] == 0){
                *limit = src + i; // point to null
                *done = TRUE;
                return i;
            }
            if(prep->isLabelSeparator(src[i],*status)){
                *limit = src + (i+1); // go past the delimiter
                return i;

            }
        }
    }else{
        int32_t i;
        for(i=0;i<srcLength;i++){
            if(prep->isLabelSeparator(src[i],*status)){
                *limit = src + (i+1); // go past the delimiter
                return i;
            }
        }
        // we have not found the delimiter
        if(i==srcLength){
            *limit = src+srcLength;
            *done = TRUE;
        }
        return i;
    }
}

U_CFUNC int32_t U_EXPORT2
idnaref_IDNToASCII(  const UChar* src, int32_t srcLength,
                   UChar* dest, int32_t destCapacity,
                   int32_t options,
                   UParseError* parseError,
                   UErrorCode* status){

    if(status == NULL || U_FAILURE(*status)){
        return 0;
    }
    if((src == NULL) || (srcLength < -1) || (destCapacity<0) || (!dest && destCapacity > 0)){
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    int32_t reqLength = 0;
//    UParseError parseError;

    NamePrepTransform* prep = TestIDNA::getInstance(*status);

    //initialize pointers to stack buffers
    UChar b1Stack[MAX_LABEL_BUFFER_SIZE];
    UChar  *b1 = b1Stack;
    int32_t b1Len, labelLen;
    UChar* delimiter = (UChar*)src;
    UChar* labelStart = (UChar*)src;
    int32_t remainingLen = srcLength;
    int32_t b1Capacity = MAX_LABEL_BUFFER_SIZE;

    //get the options
//    UBool allowUnassigned   = (UBool)((options & IDNAREF_ALLOW_UNASSIGNED) != 0);
//    UBool useSTD3ASCIIRules = (UBool)((options & IDNAREF_USE_STD3_RULES) != 0);
    UBool done = FALSE;

    if(U_FAILURE(*status)){
        goto CLEANUP;
    }


    if(srcLength == -1){
        for(;;){

            if(*delimiter == 0){
                break;
            }

            labelLen = getNextSeparator(labelStart, -1, prep, &delimiter, &done, status);
            b1Len = 0;
            if(!(labelLen==0 && done)){// make sure this is not a root label separator.

                b1Len = idnaref_toASCII(labelStart, labelLen, b1, b1Capacity,
                                        options, parseError, status);

                if(*status == U_BUFFER_OVERFLOW_ERROR){
                    // redo processing of string
                    /* we do not have enough room so grow the buffer*/
                    b1 = (UChar*) uprv_malloc(b1Len * U_SIZEOF_UCHAR);
                    if(b1==NULL){
                        *status = U_MEMORY_ALLOCATION_ERROR;
                        goto CLEANUP;
                    }

                    *status = U_ZERO_ERROR; // reset error

                    b1Len = idnaref_toASCII(labelStart, labelLen, b1, b1Len,
                                            options, parseError, status);

                }
            }

            if(U_FAILURE(*status)){
                goto CLEANUP;
            }
            int32_t tempLen = (reqLength + b1Len );
            // copy to dest
            if( tempLen< destCapacity){
                uprv_memmove(dest+reqLength, b1, b1Len * U_SIZEOF_UCHAR);
            }

            reqLength = tempLen;

            // add the label separator
            if(done == FALSE){
                if(reqLength < destCapacity){
                    dest[reqLength] = FULL_STOP;
                }
                reqLength++;
            }

            labelStart = delimiter;
        }
    }else{
        for(;;){

            if(delimiter == src+srcLength){
                break;
            }

            labelLen = getNextSeparator(labelStart, remainingLen, prep, &delimiter, &done, status);

            b1Len = idnaref_toASCII(labelStart, labelLen, b1, b1Capacity,
                                    options,parseError, status);

            if(*status == U_BUFFER_OVERFLOW_ERROR){
                // redo processing of string
                /* we do not have enough room so grow the buffer*/
                b1 = (UChar*) uprv_malloc(b1Len * U_SIZEOF_UCHAR);
                if(b1==NULL){
                    *status = U_MEMORY_ALLOCATION_ERROR;
                    goto CLEANUP;
                }

                *status = U_ZERO_ERROR; // reset error

                b1Len = idnaref_toASCII(labelStart, labelLen, b1, b1Len,
                                        options, parseError, status);

            }

            if(U_FAILURE(*status)){
                goto CLEANUP;
            }
            int32_t tempLen = (reqLength + b1Len );
            // copy to dest
            if( tempLen< destCapacity){
                uprv_memmove(dest+reqLength, b1, b1Len * U_SIZEOF_UCHAR);
            }

            reqLength = tempLen;

            // add the label separator
            if(done == FALSE){
                if(reqLength < destCapacity){
                    dest[reqLength] = FULL_STOP;
                }
                reqLength++;
            }

            labelStart = delimiter;
            remainingLen = srcLength - (delimiter - src);
        }
    }


CLEANUP:

    if(b1 != b1Stack){
        uprv_free(b1);
    }

//   delete prep;

    return u_terminateUChars(dest, destCapacity, reqLength, status);
}

U_CFUNC int32_t U_EXPORT2
idnaref_IDNToUnicode(  const UChar* src, int32_t srcLength,
                     UChar* dest, int32_t destCapacity,
                     int32_t options,
                     UParseError* parseError,
                     UErrorCode* status){

    if(status == NULL || U_FAILURE(*status)){
        return 0;
    }
    if((src == NULL) || (srcLength < -1) || (destCapacity<0) || (!dest && destCapacity > 0)){
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    int32_t reqLength = 0;

    UBool done = FALSE;

    NamePrepTransform* prep = TestIDNA::getInstance(*status);

    //initialize pointers to stack buffers
    UChar b1Stack[MAX_LABEL_BUFFER_SIZE];
    UChar  *b1 = b1Stack;
    int32_t b1Len, labelLen;
    UChar* delimiter = (UChar*)src;
    UChar* labelStart = (UChar*)src;
    int32_t remainingLen = srcLength;
    int32_t b1Capacity = MAX_LABEL_BUFFER_SIZE;

    //get the options
//    UBool allowUnassigned   = (UBool)((options & IDNAREF_ALLOW_UNASSIGNED) != 0);
//    UBool useSTD3ASCIIRules = (UBool)((options & IDNAREF_USE_STD3_RULES) != 0);

    if(U_FAILURE(*status)){
        goto CLEANUP;
    }

    if(srcLength == -1){
        for(;;){

            if(*delimiter == 0){
                break;
            }

            labelLen = getNextSeparator(labelStart, -1, prep, &delimiter, &done, status);

           if(labelLen==0 && done==FALSE){
                *status = U_IDNA_ZERO_LENGTH_LABEL_ERROR;
            }
            b1Len = idnaref_toUnicode(labelStart, labelLen, b1, b1Capacity,
                                      options, parseError, status);

            if(*status == U_BUFFER_OVERFLOW_ERROR){
                // redo processing of string
                /* we do not have enough room so grow the buffer*/
                b1 = (UChar*) uprv_malloc(b1Len * U_SIZEOF_UCHAR);
                if(b1==NULL){
                    *status = U_MEMORY_ALLOCATION_ERROR;
                    goto CLEANUP;
                }

                *status = U_ZERO_ERROR; // reset error

                b1Len = idnaref_toUnicode( labelStart, labelLen, b1, b1Len,
                                           options, parseError, status);

            }

            if(U_FAILURE(*status)){
                goto CLEANUP;
            }
            int32_t tempLen = (reqLength + b1Len );
            // copy to dest
            if( tempLen< destCapacity){
                uprv_memmove(dest+reqLength, b1, b1Len * U_SIZEOF_UCHAR);
            }

            reqLength = tempLen;
            // add the label separator
            if(done == FALSE){
                if(reqLength < destCapacity){
                    dest[reqLength] = FULL_STOP;
                }
                reqLength++;
            }

            labelStart = delimiter;
        }
    }else{
        for(;;){

            if(delimiter == src+srcLength){
                break;
            }

            labelLen = getNextSeparator(labelStart, remainingLen, prep, &delimiter, &done, status);

            if(labelLen==0 && done==FALSE){
                *status = U_IDNA_ZERO_LENGTH_LABEL_ERROR;
            }

            b1Len = idnaref_toUnicode( labelStart,labelLen, b1, b1Capacity,
                                       options, parseError, status);

            if(*status == U_BUFFER_OVERFLOW_ERROR){
                // redo processing of string
                /* we do not have enough room so grow the buffer*/
                b1 = (UChar*) uprv_malloc(b1Len * U_SIZEOF_UCHAR);
                if(b1==NULL){
                    *status = U_MEMORY_ALLOCATION_ERROR;
                    goto CLEANUP;
                }

                *status = U_ZERO_ERROR; // reset error

                b1Len = idnaref_toUnicode( labelStart, labelLen, b1, b1Len,
                                           options, parseError, status);

            }

            if(U_FAILURE(*status)){
                goto CLEANUP;
            }
            int32_t tempLen = (reqLength + b1Len );
            // copy to dest
            if( tempLen< destCapacity){
                uprv_memmove(dest+reqLength, b1, b1Len * U_SIZEOF_UCHAR);
            }

            reqLength = tempLen;

            // add the label separator
            if(done == FALSE){
                if(reqLength < destCapacity){
                    dest[reqLength] = FULL_STOP;
                }
                reqLength++;
            }

            labelStart = delimiter;
            remainingLen = srcLength - (delimiter - src);
        }
    }

CLEANUP:

    if(b1 != b1Stack){
        uprv_free(b1);
    }

//    delete prep;

    return u_terminateUChars(dest, destCapacity, reqLength, status);
}

U_CFUNC int32_t U_EXPORT2
idnaref_compare(  const UChar *s1, int32_t length1,
                const UChar *s2, int32_t length2,
                int32_t options,
                UErrorCode* status){

    if(status == NULL || U_FAILURE(*status)){
        return -1;
    }

    UChar b1Stack[MAX_IDN_BUFFER_SIZE], b2Stack[MAX_IDN_BUFFER_SIZE];
    UChar *b1 = b1Stack, *b2 = b2Stack;
    int32_t b1Len, b2Len, b1Capacity = MAX_IDN_BUFFER_SIZE, b2Capacity = MAX_IDN_BUFFER_SIZE;
    int32_t result = -1;

    UParseError parseError;

    b1Len = idnaref_IDNToASCII(s1, length1, b1, b1Capacity, options, &parseError, status);
    if(*status == U_BUFFER_OVERFLOW_ERROR){
        // redo processing of string
        /* we do not have enough room so grow the buffer*/
        b1 = (UChar*) uprv_malloc(b1Len * U_SIZEOF_UCHAR);
        if(b1==NULL){
            *status = U_MEMORY_ALLOCATION_ERROR;
            goto CLEANUP;
        }

        *status = U_ZERO_ERROR; // reset error

        b1Len = idnaref_IDNToASCII(s1,length1,b1,b1Len, options, &parseError, status);

    }

    b2Len = idnaref_IDNToASCII(s2,length2,b2,b2Capacity,options, &parseError, status);
    if(*status == U_BUFFER_OVERFLOW_ERROR){
        // redo processing of string
        /* we do not have enough room so grow the buffer*/
        b2 = (UChar*) uprv_malloc(b2Len * U_SIZEOF_UCHAR);
        if(b2==NULL){
            *status = U_MEMORY_ALLOCATION_ERROR;
            goto CLEANUP;
        }

        *status = U_ZERO_ERROR; // reset error

        b2Len = idnaref_IDNToASCII(s2,length2,b2,b2Len,options, &parseError, status);

    }
    // when toASCII is applied all label separators are replaced with FULL_STOP
    result = compareCaseInsensitiveASCII(b1,b1Len,b2,b2Len);

CLEANUP:
    if(b1 != b1Stack){
        uprv_free(b1);
    }

    if(b2 != b2Stack){
        uprv_free(b2);
    }

    return result;
}
#endif /* #if !UCONFIG_NO_IDNA */
