/*
*******************************************************************************
*   Copyright (C) 2000-2009, International Business Machines
*   Corporation and others.  All Rights Reserved.
*******************************************************************************
*   Date        Name        Description
*   03/22/00    aliu        Creation.
*   07/13/00    Madhu       Added more tests 
*******************************************************************************
*/

#include "cintltst.h"
#include "uhash.h"
#include "unicode/ctest.h"
#include "unicode/ustring.h"
#include "cstring.h"

/**********************************************************************
 * Prototypes
 *********************************************************************/

static void TestBasic(void);
static void TestOtherAPI(void);
static void hashIChars(void);

static int32_t U_EXPORT2 U_CALLCONV hashChars(const UHashTok key);

static UBool U_EXPORT2 U_CALLCONV isEqualChars(const UHashTok key1, const UHashTok key2);

static void _put(UHashtable* hash,
                 const char* key,
                 int32_t value,
                 int32_t expectedOldValue);

static void _get(UHashtable* hash,
          const char* key,
          int32_t expectedValue);

static void _remove(UHashtable* hash,
             const char* key,
             int32_t expectedValue);

void addHashtableTest(TestNode** root);

/**********************************************************************
 * UHashTok wrapper functions
 *********************************************************************/

static UBool
_compareChars(const void* a, const void* b) {
    UHashTok s, t;
    s.pointer = (void *)a;
    t.pointer = (void *)b;
    return uhash_compareChars(s, t);
}

static UBool
_compareIChars(const void* a, const void* b) {
    UHashTok s, t;
    s.pointer = (void *)a;
    t.pointer = (void *)b;
    return uhash_compareIChars(s, t);
}

static UBool
_compareUChars(const void* a, const void* b) {
    UHashTok s, t;
    s.pointer = (void *)a;
    t.pointer = (void *)b;
    return uhash_compareUChars(s, t);
}

static UBool
_compareLong(int32_t a, int32_t b) {
    UHashTok s, t;
    s.integer = a;
    t.integer = b;
    return uhash_compareLong(s, t);
}

/**********************************************************************
 * FW Registration
 *********************************************************************/

void addHashtableTest(TestNode** root) {
   
    addTest(root, &TestBasic,   "tsutil/chashtst/TestBasic");
    addTest(root, &TestOtherAPI, "tsutil/chashtst/TestOtherAPI");
    addTest(root, &hashIChars, "tsutil/chashtst/hashIChars");
    
}

/**********************************************************************
 * Test Functions
 *********************************************************************/

static void TestBasic(void) {
    static const char one[4] =   {0x6F, 0x6E, 0x65, 0}; /* "one" */
    static const char one2[4] =  {0x6F, 0x6E, 0x65, 0}; /* Get around compiler optimizations */
    static const char two[4] =   {0x74, 0x77, 0x6F, 0}; /* "two" */
    static const char three[6] = {0x74, 0x68, 0x72, 0x65, 0x65, 0}; /* "three" */
    static const char omega[6] = {0x6F, 0x6D, 0x65, 0x67, 0x61, 0}; /* "omega" */
    UErrorCode status = U_ZERO_ERROR;
    UHashtable *hash;

    hash = uhash_open(hashChars, isEqualChars, NULL,  &status);
    if (U_FAILURE(status)) {
        log_err("FAIL: uhash_open failed with %s and returned 0x%08x\n",
                u_errorName(status), hash);
        return;
    }
    if (hash == NULL) {
        log_err("FAIL: uhash_open returned NULL\n");
        return;
    }
    log_verbose("Ok: uhash_open returned 0x%08X\n", hash);

    _put(hash, one, 1, 0);
    _put(hash, omega, 24, 0);
    _put(hash, two, 2, 0);
    _put(hash, three, 3, 0);
    _put(hash, one, -1, 1);
    _put(hash, two, -2, 2);
    _put(hash, omega, 48, 24);
    _put(hash, one, 100, -1);
    _get(hash, three, 3);
    _remove(hash, two, -2);
    _get(hash, two, 0);
    _get(hash, one, 100);
    _put(hash, two, 200, 0);
    _get(hash, omega, 48);
    _get(hash, two, 200);

    if(_compareChars((void*)one, (void*)three) == TRUE ||
        _compareChars((void*)one, (void*)one2) != TRUE ||
        _compareChars((void*)one, (void*)one) != TRUE ||
        _compareChars((void*)one, NULL) == TRUE  )  {
        log_err("FAIL: compareChars failed\n");
    }
    if(_compareIChars((void*)one, (void*)three) == TRUE ||
        _compareIChars((void*)one, (void*)one) != TRUE ||
        _compareIChars((void*)one, (void*)one2) != TRUE ||
        _compareIChars((void*)one, NULL) == TRUE  )  {
        log_err("FAIL: compareIChars failed\n");
    }
     
    uhash_close(hash);

}

static void TestOtherAPI(void){
    
    UErrorCode status = U_ZERO_ERROR;
    UHashtable *hash;

    /* Use the correct type when cast to void * */
    static const UChar one[4]   = {0x006F, 0x006E, 0x0065, 0}; /* L"one" */
    static const UChar one2[4]  = {0x006F, 0x006E, 0x0065, 0}; /* Get around compiler optimizations */
    static const UChar two[4]   = {0x0074, 0x0077, 0x006F, 0}; /* L"two" */
    static const UChar two2[4]  = {0x0074, 0x0077, 0x006F, 0}; /* L"two" */
    static const UChar three[6] = {0x0074, 0x0068, 0x0072, 0x0065, 0x0065, 0}; /* L"three" */
    static const UChar four[6]  = {0x0066, 0x006F, 0x0075, 0x0072, 0}; /* L"four" */
    static const UChar five[6]  = {0x0066, 0x0069, 0x0076, 0x0065, 0}; /* L"five" */
    static const UChar five2[6] = {0x0066, 0x0069, 0x0076, 0x0065, 0}; /* L"five" */

    hash = uhash_open(uhash_hashUChars, uhash_compareUChars, NULL,  &status);
    if (U_FAILURE(status)) {
        log_err("FAIL: uhash_open failed with %s and returned 0x%08x\n",
                u_errorName(status), hash);
        return;
    }
    if (hash == NULL) {
        log_err("FAIL: uhash_open returned NULL\n");
        return;
    }
    log_verbose("Ok: uhash_open returned 0x%08X\n", hash);

    uhash_puti(hash, (void*)one, 1, &status);
    if(uhash_count(hash) != 1){
         log_err("FAIL: uhas_count() failed. Expected: 1, Got: %d\n", uhash_count(hash));
    }
    if(uhash_find(hash, (void*)two) != NULL){
        log_err("FAIL: uhash_find failed\n");
    }
    uhash_puti(hash, (void*)two, 2, &status);
    uhash_puti(hash, (void*)three, 3, &status);
    uhash_puti(hash, (void*)four, 4, &status);
    uhash_puti(hash, (void*)five, 5, &status);
    
    if(uhash_count(hash) != 5){
        log_err("FAIL: uhas_count() failed. Expected: 5, Got: %d\n", uhash_count(hash));
    }
    
    if(uhash_geti(hash, (void*)two2) != 2){
        log_err("FAIL: uhash_geti failed\n");
    }
    
    if(uhash_find(hash, (void*)two2) == NULL){
        log_err("FAIL: uhash_find of \"two\" failed\n");
    }
    
    if(uhash_removei(hash, (void*)five2) != 5){
        log_err("FAIL: uhash_remove() failed\n");
    }
    if(uhash_count(hash) != 4){
        log_err("FAIL: uhas_count() failed. Expected: 4, Got: %d\n", uhash_count(hash));
    }

    uhash_put(hash, (void*)one, NULL, &status);
    if(uhash_count(hash) != 3){
        log_err("FAIL: uhash_put() with value=NULL didn't remove the key value pair\n");
    }
    status=U_ILLEGAL_ARGUMENT_ERROR;
    uhash_puti(hash, (void*)one, 1, &status);
    if(uhash_count(hash) != 3){
        log_err("FAIL: uhash_put() with value!=NULL should fail when status != U_ZERO_ERROR \n");
    }
    
    status=U_ZERO_ERROR;
    uhash_puti(hash, (void*)one, 1, &status);
    if(uhash_count(hash) != 4){
        log_err("FAIL: uhash_put() with value!=NULL didn't replace the key value pair\n");
    }

    if(_compareUChars((void*)one, (void*)two) == TRUE ||
        _compareUChars((void*)one, (void*)one) != TRUE ||
        _compareUChars((void*)one, (void*)one2) != TRUE ||
        _compareUChars((void*)one, NULL) == TRUE  )  {
        log_err("FAIL: compareUChars failed\n");
    }
   
    uhash_removeAll(hash);
    if(uhash_count(hash) != 0){
        log_err("FAIL: uhas_count() failed. Expected: 0, Got: %d\n", uhash_count(hash));
    }

    uhash_setKeyComparator(hash, uhash_compareLong);
    uhash_setKeyHasher(hash, uhash_hashLong);
    uhash_iputi(hash, 1001, 1, &status);
    uhash_iputi(hash, 1002, 2, &status);
    uhash_iputi(hash, 1003, 3, &status);
    if(_compareLong(1001, 1002) == TRUE ||
        _compareLong(1001, 1001) != TRUE ||
        _compareLong(1001, 0) == TRUE  )  {
        log_err("FAIL: compareLong failed\n");
    }
    /*set the resize policy to just GROW and SHRINK*/
         /*how to test this??*/
    uhash_setResizePolicy(hash, U_GROW_AND_SHRINK);
    uhash_iputi(hash, 1004, 4, &status);
    uhash_iputi(hash, 1005, 5, &status);
    uhash_iputi(hash, 1006, 6, &status);
    if(uhash_count(hash) != 6){
        log_err("FAIL: uhash_count() failed. Expected: 6, Got: %d\n", uhash_count(hash));
    }
    if(uhash_iremovei(hash, 1004) != 4){
        log_err("FAIL: uhash_remove failed\n");
    }
    if(uhash_iremovei(hash, 1004) != 0){
        log_err("FAIL: uhash_remove failed\n");
    }

    uhash_removeAll(hash);
    uhash_iput(hash, 2004, (void*)one, &status);
    uhash_iput(hash, 2005, (void*)two, &status);
    if(uhash_count(hash) != 2){
        log_err("FAIL: uhash_count() failed. Expected: 2, Got: %d\n", uhash_count(hash));
    }
    if(uhash_iremove(hash, 2004) != (void*)one){
        log_err("FAIL: uhash_remove failed\n");
    }
    if(uhash_iremove(hash, 2004) != NULL){
        log_err("FAIL: uhash_remove failed\n");
    }
    if(uhash_count(hash) != 1){
        log_err("FAIL: uhash_count() failed. Expected: 1, Got: %d\n", uhash_count(hash));
    }

    uhash_close(hash);

}

static void hashIChars(void) {
    static const char which[] = "which";
    static const char WHICH2[] = "WHICH";
    static const char where[] = "where";
    UErrorCode status = U_ZERO_ERROR;
    UHashtable *hash;

    hash = uhash_open(uhash_hashIChars, uhash_compareIChars, NULL, &status);
    if (U_FAILURE(status)) {
        log_err("FAIL: uhash_open failed with %s and returned 0x%08x\n",
                u_errorName(status), hash);
        return;
    }
    if (hash == NULL) {
        log_err("FAIL: uhash_open returned NULL\n");
        return;
    }
    log_verbose("Ok: uhash_open returned 0x%08X\n", hash);

    _put(hash, which, 1, 0);
    _put(hash, WHICH2, 2, 1);
    _put(hash, where, 3, 0);
    if(uhash_count(hash) != 2){
         log_err("FAIL: uhas_count() failed. Expected: 1, Got: %d\n", uhash_count(hash));
    }
    _remove(hash, which, 2);

    uhash_close(hash);
}


/**********************************************************************
 * uhash Callbacks
 *********************************************************************/

/**
 * This hash function is designed to collide a lot to test key equality
 * resolution.  It only uses the first char.
 */
static int32_t U_EXPORT2 U_CALLCONV hashChars(const UHashTok key) {
    return *(const char*) key.pointer;
}

static UBool U_EXPORT2 U_CALLCONV isEqualChars(const UHashTok key1, const UHashTok key2) {
    return (UBool)((key1.pointer != NULL) &&
        (key2.pointer != NULL) &&
        (uprv_strcmp((const char*)key1.pointer, (const char*)key2.pointer) == 0));
}

/**********************************************************************
 * Wrapper Functions
 *********************************************************************/

static void _put(UHashtable* hash,
          const char* key,
          int32_t value,
          int32_t expectedOldValue) {
    UErrorCode status = U_ZERO_ERROR;
    int32_t oldValue =
        uhash_puti(hash, (void*) key, value, &status);
    if (U_FAILURE(status)) {
        log_err("FAIL: uhash_put(%s) failed with %s and returned %ld\n",
                key, u_errorName(status), oldValue);
    } else if (oldValue != expectedOldValue) {
        log_err("FAIL: uhash_put(%s) returned old value %ld; expected %ld\n",
                key, oldValue, expectedOldValue);
    } else {
        log_verbose("Ok: uhash_put(%s, %d) returned old value %ld\n",
                    key, value, oldValue);
    }
}

static void _get(UHashtable* hash,
          const char* key,
          int32_t expectedValue) {
    UErrorCode status = U_ZERO_ERROR;
    int32_t value = uhash_geti(hash, key);
    if (U_FAILURE(status)) {
        log_err("FAIL: uhash_get(%s) failed with %s and returned %ld\n",
                key, u_errorName(status), value);
    } else if (value != expectedValue) {
        log_err("FAIL: uhash_get(%s) returned %ld; expected %ld\n",
                key, value, expectedValue);
    } else {
        log_verbose("Ok: uhash_get(%s) returned value %ld\n",
                    key, value);
    }
}

static void _remove(UHashtable* hash,
             const char* key,
             int32_t expectedValue) {
    int32_t value = uhash_removei(hash, key);
    if (value != expectedValue) {
        log_err("FAIL: uhash_remove(%s) returned %ld; expected %ld\n",
                key, value, expectedValue);
    } else {
        log_verbose("Ok: uhash_remove(%s) returned old value %ld\n",
                    key, value);
    }
}

