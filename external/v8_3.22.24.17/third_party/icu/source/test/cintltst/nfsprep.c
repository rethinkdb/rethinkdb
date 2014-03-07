/*
 *******************************************************************************
 *
 *   Copyright (C) 2003-2007, International Business Machines
 *   Corporation and others.  All Rights Reserved.
 *
 *******************************************************************************
 *   file name:  nfsprep.c
 *   encoding:   US-ASCII
 *   tab size:   8 (not used)
 *   indentation:4
 *
 *   created on: 2003jul11
 *   created by: Ram Viswanadha
 */

#include "unicode/utypes.h"

#if !UCONFIG_NO_IDNA

#include "nfsprep.h"
#include "ustr_imp.h"
#include "cintltst.h"

#define LENGTHOF(array) (int32_t)(sizeof(array)/sizeof((array)[0]))
#define NFS4_MAX_BUFFER_SIZE 1000
#define PREFIX_SUFFIX_SEPARATOR 0x0040 /* '@' */


const char* NFS4DataFileNames[5] ={
    "nfscss",
    "nfscsi",
    "nfscis",
    "nfsmxp",
    "nfsmxs"
};


int32_t
nfs4_prepare( const char* src, int32_t srcLength,
              char* dest, int32_t destCapacity,
              NFS4ProfileState state,
              UParseError* parseError,
              UErrorCode*  status){
    
    UChar b1Stack[NFS4_MAX_BUFFER_SIZE], 
          b2Stack[NFS4_MAX_BUFFER_SIZE]; 
    char  b3Stack[NFS4_MAX_BUFFER_SIZE];

    /* initialize pointers to stack buffers */
    UChar *b1 = b1Stack, *b2 = b2Stack;
    char  *b3=b3Stack;
    int32_t b1Len=0, b2Len=0, b3Len=0,
            b1Capacity = NFS4_MAX_BUFFER_SIZE, 
            b2Capacity = NFS4_MAX_BUFFER_SIZE,
            b3Capacity = NFS4_MAX_BUFFER_SIZE,
            reqLength=0;
    
    UStringPrepProfile* profile = NULL;
    /* get the test data path */
    const char *testdatapath = NULL;

    if(status==NULL || U_FAILURE(*status)){
        return 0;
    }
    if((src==NULL) || (srcLength < -1) || (destCapacity<0) || (!dest && destCapacity > 0)){
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }
    testdatapath = loadTestData(status);

    /* convert the string from UTF-8 to UTF-16 */
    u_strFromUTF8(b1,b1Capacity,&b1Len,src,srcLength,status);
    if(*status == U_BUFFER_OVERFLOW_ERROR){

        /* reset the status */
        *status = U_ZERO_ERROR;
        
        b1 = (UChar*) malloc(b1Len * U_SIZEOF_UCHAR);
        if(b1==NULL){
            *status = U_MEMORY_ALLOCATION_ERROR;
            goto CLEANUP;
        }

        b1Capacity = b1Len;
        u_strFromUTF8(b1, b1Capacity, &b1Len, src, srcLength, status);
    }

    /* open the profile */
    profile = usprep_open(testdatapath, NFS4DataFileNames[state],  status);
    /* prepare the string */
    b2Len = usprep_prepare(profile, b1, b1Len, b2, b2Capacity, USPREP_DEFAULT, parseError, status);
    if(*status == U_BUFFER_OVERFLOW_ERROR){
        *status = U_ZERO_ERROR;
        b2 = (UChar*) malloc(b2Len * U_SIZEOF_UCHAR);
        if(b2== NULL){
            *status = U_MEMORY_ALLOCATION_ERROR;
            goto CLEANUP;
        }
        b2Len = usprep_prepare(profile, b1, b1Len, b2, b2Len, USPREP_DEFAULT, parseError, status);
    }
    
    /* convert the string back to UTF-8 */
    u_strToUTF8(b3,b3Capacity, &b3Len, b2, b2Len, status);
    if(*status == U_BUFFER_OVERFLOW_ERROR){
        *status = U_ZERO_ERROR;
        b3 = (char*) malloc(b3Len);
        if(b3== NULL){
            *status = U_MEMORY_ALLOCATION_ERROR;
            goto CLEANUP;
        }
        b3Capacity = b3Len;
        u_strToUTF8(b3,b3Capacity, &b3Len, b2, b2Len, status);
    }

    reqLength = b3Len;
    if(dest!=NULL && reqLength <= destCapacity){
        memmove(dest, b3, reqLength);
    }

CLEANUP:
    if(b1!=b1Stack){
        free(b1);
    }
    if(b2!=b2Stack){
        free(b2);
    }
    if(b3!=b3Stack){
        free(b3);
    }

    return u_terminateChars(dest, destCapacity, reqLength, status);
}

/* sorted array for binary search*/
static const char* special_prefixes[]={
    "\x0041\x004e\x004f\x004e\x0059\x004d\x004f\x0055\x0053",    
    "\x0041\x0055\x0054\x0048\x0045\x004e\x0054\x0049\x0043\x0041\x0054\x0045\x0044",
    "\x0042\x0041\x0054\x0043\x0048", 
    "\x0044\x0049\x0041\x004c\x0055\x0050", 
    "\x0045\x0056\x0045\x0052\x0059\x004f\x004e\x0045", 
    "\x0047\x0052\x004f\x0055\x0050",
    "\x0049\x004e\x0054\x0045\x0052\x0041\x0043\x0054\x0049\x0056\x0045",  
    "\x004e\x0045\x0054\x0057\x004f\x0052\x004b", 
    "\x004f\x0057\x004e\x0045\x0052",
};


/* binary search the sorted array */
static int 
findStringIndex(const char* const *sortedArr, int32_t sortedArrLen, const char* target, int32_t targetLen){

    int left, middle, right,rc;

    left =0;
    right= sortedArrLen-1;

    while(left <= right){
        middle = (left+right)/2;
        rc=strncmp(sortedArr[middle],target, targetLen);
    
        if(rc<0){
            left = middle+1;
        }else if(rc >0){
            right = middle -1;
        }else{
            return middle;
        }
    }
    return -1;
}

static void 
getPrefixSuffix(const char *src, int32_t srcLength, 
                const char **prefix, int32_t *prefixLen,
                const char **suffix, int32_t *suffixLen,
                UErrorCode *status){

    int32_t i=0;
    *prefix = src;
    while(i<srcLength){
        if(src[i] == PREFIX_SUFFIX_SEPARATOR){
            if((i+1) == srcLength){
                /* we reached the end of the string */
                *suffix = NULL;
                i++;
                break;
            }
            i++;/* the prefix contains the separator */
            *suffix = src + i;
            break;
        }
        i++;
    }
    *prefixLen = i;
    *suffixLen = srcLength - i;
    /* special prefixes must not be followed by suffixes! */
    if((findStringIndex(special_prefixes,LENGTHOF(special_prefixes), *prefix, *prefixLen-1) != -1) && (*suffix != NULL)){
        *status = U_PARSE_ERROR;
        return;
    }
            
}

int32_t
nfs4_mixed_prepare( const char* src, int32_t srcLength,
                    char* dest, int32_t destCapacity,
                    UParseError* parseError,
                    UErrorCode*  status){

    const char *prefix = NULL, *suffix = NULL;
    int32_t prefixLen=0, suffixLen=0;
    char  pStack[NFS4_MAX_BUFFER_SIZE], 
          sStack[NFS4_MAX_BUFFER_SIZE]; 
    char *p=pStack, *s=sStack;
    int32_t pLen=0, sLen=0, reqLen=0,
            pCapacity = NFS4_MAX_BUFFER_SIZE,
            sCapacity = NFS4_MAX_BUFFER_SIZE;


    if(status==NULL || U_FAILURE(*status)){
        return 0;
    }
    if((src==NULL) || (srcLength < -1) || (destCapacity<0) || (!dest && destCapacity > 0)){
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }
    if(srcLength == -1){
        srcLength = (int32_t)strlen(src);
    }
    getPrefixSuffix(src, srcLength, &prefix, &prefixLen, &suffix, &suffixLen, status); 

    /* prepare the prefix */
    pLen = nfs4_prepare(prefix, prefixLen, p, pCapacity, NFS4_MIXED_PREP_PREFIX, parseError, status);
    if(*status == U_BUFFER_OVERFLOW_ERROR){
        *status = U_ZERO_ERROR;
        p = (char*) malloc(pLen);
        if(p == NULL){
           *status = U_MEMORY_ALLOCATION_ERROR;
           goto CLEANUP;
        }
        pLen = nfs4_prepare(prefix, prefixLen, p, pLen, NFS4_MIXED_PREP_PREFIX, parseError, status);
    }

    /* prepare the suffix */
    if(suffix != NULL){
        sLen = nfs4_prepare(suffix, suffixLen, s, sCapacity, NFS4_MIXED_PREP_SUFFIX, parseError, status);
        if(*status == U_BUFFER_OVERFLOW_ERROR){
            *status = U_ZERO_ERROR;
            s = (char*) malloc(pLen);
            if(s == NULL){
               *status = U_MEMORY_ALLOCATION_ERROR;
               goto CLEANUP;
            }
            sLen = nfs4_prepare(suffix, suffixLen, s, sLen, NFS4_MIXED_PREP_SUFFIX, parseError, status);
        }
    }
    reqLen = pLen+sLen+1 /* for the delimiter */;
    if(dest != NULL && reqLen <= destCapacity){
        memmove(dest, p, pLen);
        /* add the suffix */
        if(suffix!=NULL){
            dest[pLen++] = PREFIX_SUFFIX_SEPARATOR;
            memmove(dest+pLen, s, sLen);
        }
    }

CLEANUP:
    if(p != pStack){
        free(p);
    }
    if(s != sStack){
        free(s);
    }
    
    return u_terminateChars(dest, destCapacity, reqLen, status);
}

int32_t
nfs4_cis_prepare(   const char* src, int32_t srcLength,
                    char* dest, int32_t destCapacity,
                    UParseError* parseError,
                    UErrorCode*  status){
    return nfs4_prepare(src, srcLength, dest, destCapacity, NFS4_CIS_PREP, parseError, status);
}


int32_t
nfs4_cs_prepare(    const char* src, int32_t srcLength,
                    char* dest, int32_t destCapacity,
                    UBool isCaseSensitive,
                    UParseError* parseError,
                    UErrorCode*  status){
    if(isCaseSensitive){
        return nfs4_prepare(src, srcLength, dest, destCapacity, NFS4_CS_PREP_CS, parseError, status);
    }else{
        return nfs4_prepare(src, srcLength, dest, destCapacity, NFS4_CS_PREP_CI, parseError, status);
    }
}

#endif
/*
 * Hey, Emacs, please set the following:
 *
 * Local Variables:
 * indent-tabs-mode: nil
 * End:
 *
 */

