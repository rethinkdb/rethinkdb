/*
******************************************************************************
*
*   Copyright (C) 2001-2008, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
******************************************************************************
*   file name:  trietest.c
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2008sep01 (starting from a copy of trietest.c)
*   created by: Markus W. Scherer
*/

#include <stdio.h>
#include "unicode/utypes.h"
#include "utrie2.h"
#include "utrie.h"
#include "cstring.h"
#include "cmemory.h"
#include "udataswp.h"
#include "cintltst.h"

#define LENGTHOF(array) (int32_t)(sizeof(array)/sizeof((array)[0]))

void addTrie2Test(TestNode** root);

/* Values for setting possibly overlapping, out-of-order ranges of values */
typedef struct SetRange {
    UChar32 start, limit;
    uint32_t value;
    UBool overwrite;
} SetRange;

/*
 * Values for testing:
 * value is set from the previous boundary's limit to before
 * this boundary's limit
 *
 * There must be an entry with limit 0 and the intialValue.
 * It may be preceded by an entry with negative limit and the errorValue.
 */
typedef struct CheckRange {
    UChar32 limit;
    uint32_t value;
} CheckRange;

static int32_t
skipSpecialValues(const CheckRange checkRanges[], int32_t countCheckRanges) {
    int32_t i;
    for(i=0; i<countCheckRanges && checkRanges[i].limit<=0; ++i) {}
    return i;
}

static int32_t
getSpecialValues(const CheckRange checkRanges[], int32_t countCheckRanges,
                 uint32_t *pInitialValue, uint32_t *pErrorValue) {
    int32_t i=0;
    if(i<countCheckRanges && checkRanges[i].limit<0) {
        *pErrorValue=checkRanges[i++].value;
    } else {
        *pErrorValue=0xbad;
    }
    if(i<countCheckRanges && checkRanges[i].limit==0) {
        *pInitialValue=checkRanges[i++].value;
    } else {
        *pInitialValue=0;
    }
    return i;
}

/* utrie2_enum() callback, modifies a value */
static uint32_t U_CALLCONV
testEnumValue(const void *context, uint32_t value) {
    return value^0x5555;
}

/* utrie2_enum() callback, verifies a range */
static UBool U_CALLCONV
testEnumRange(const void *context, UChar32 start, UChar32 end, uint32_t value) {
    const CheckRange **pb=(const CheckRange **)context;
    const CheckRange *b=(*pb)++;
    UChar32 limit=end+1;
    
    value^=0x5555;
    if(start!=(b-1)->limit || limit!=b->limit || value!=b->value) {
        log_err("error: utrie2_enum() delivers wrong range [U+%04lx..U+%04lx].0x%lx instead of [U+%04lx..U+%04lx].0x%lx\n",
            (long)start, (long)end, (long)value,
            (long)(b-1)->limit, (long)b->limit-1, (long)b->value);
    }
    return TRUE;
}

static void
testTrieEnum(const char *testName,
             const UTrie2 *trie,
             const CheckRange checkRanges[], int32_t countCheckRanges) {
    /* skip over special values */
    while(countCheckRanges>0 && checkRanges[0].limit<=0) {
        ++checkRanges;
        --countCheckRanges;
    }
    utrie2_enum(trie, testEnumValue, testEnumRange, &checkRanges);
}

/* verify all expected values via UTRIE2_GETxx() */
static void
testTrieGetters(const char *testName,
                const UTrie2 *trie, UTrie2ValueBits valueBits,
                const CheckRange checkRanges[], int32_t countCheckRanges) {
    uint32_t initialValue, errorValue;
    uint32_t value, value2;
    UChar32 start, limit;
    int32_t i, countSpecials;

    UBool isFrozen=utrie2_isFrozen(trie);
    const char *const typeName= isFrozen ? "frozen trie" : "newTrie";

    countSpecials=getSpecialValues(checkRanges, countCheckRanges, &initialValue, &errorValue);

    start=0;
    for(i=countSpecials; i<countCheckRanges; ++i) {
        limit=checkRanges[i].limit;
        value=checkRanges[i].value;

        while(start<limit) {
            if(isFrozen) {
                if(start<=0xffff) {
                    if(!U_IS_LEAD(start)) {
                        if(valueBits==UTRIE2_16_VALUE_BITS) {
                            value2=UTRIE2_GET16_FROM_U16_SINGLE_LEAD(trie, start);
                        } else {
                            value2=UTRIE2_GET32_FROM_U16_SINGLE_LEAD(trie, start);
                        }
                        if(value!=value2) {
                            log_err("error: %s(%s).fromBMP(U+%04lx)==0x%lx instead of 0x%lx\n",
                                    typeName, testName, (long)start, (long)value2, (long)value);
                        }
                    }
                } else {
                    if(valueBits==UTRIE2_16_VALUE_BITS) {
                        value2=UTRIE2_GET16_FROM_SUPP(trie, start);
                    } else {
                        value2=UTRIE2_GET32_FROM_SUPP(trie, start);
                    }
                    if(value!=value2) {
                        log_err("error: %s(%s).fromSupp(U+%04lx)==0x%lx instead of 0x%lx\n",
                                typeName, testName, (long)start, (long)value2, (long)value);
                    }
                }
                if(valueBits==UTRIE2_16_VALUE_BITS) {
                    value2=UTRIE2_GET16(trie, start);
                } else {
                    value2=UTRIE2_GET32(trie, start);
                }
                if(value!=value2) {
                    log_err("error: %s(%s).get(U+%04lx)==0x%lx instead of 0x%lx\n",
                            typeName, testName, (long)start, (long)value2, (long)value);
                }
            }
            value2=utrie2_get32(trie, start);
            if(value!=value2) {
                log_err("error: %s(%s).get32(U+%04lx)==0x%lx instead of 0x%lx\n",
                        typeName, testName, (long)start, (long)value2, (long)value);
            }
            ++start;
        }
    }

    if(isFrozen) {
        /* test linear ASCII range from the data array pointer (access to "internal" field) */
        start=0;
        for(i=countSpecials; i<countCheckRanges && start<=0x7f; ++i) {
            limit=checkRanges[i].limit;
            value=checkRanges[i].value;

            while(start<limit && start<=0x7f) {
                if(valueBits==UTRIE2_16_VALUE_BITS) {
                    value2=trie->data16[start];
                } else {
                    value2=trie->data32[start];
                }
                if(value!=value2) {
                    log_err("error: %s(%s).asciiData[U+%04lx]==0x%lx instead of 0x%lx\n",
                            typeName, testName, (long)start, (long)value2, (long)value);
                }
                ++start;
            }
        }
        while(start<=0xbf) {
            if(valueBits==UTRIE2_16_VALUE_BITS) {
                value2=trie->data16[start];
            } else {
                value2=trie->data32[start];
            }
            if(errorValue!=value2) {
                log_err("error: %s(%s).badData[U+%04lx]==0x%lx instead of 0x%lx\n",
                        typeName, testName, (long)start, (long)value2, (long)errorValue);
            }
            ++start;
        }
    }

    if(0!=strncmp(testName, "dummy", 5) && 0!=strncmp(testName, "trie1", 5)) {
        /* test values for lead surrogate code units */
        for(start=0xd7ff; start<0xdc01; ++start) {
            switch(start) {
            case 0xd7ff:
            case 0xdc00:
                value=errorValue;
                break;
            case 0xd800:
                value=90;
                break;
            case 0xd999:
                value=94;
                break;
            case 0xdbff:
                value=99;
                break;
            default:
                value=initialValue;
                break;
            }
            if(isFrozen && U_IS_LEAD(start)) {
                if(valueBits==UTRIE2_16_VALUE_BITS) {
                    value2=UTRIE2_GET16_FROM_U16_SINGLE_LEAD(trie, start);
                } else {
                    value2=UTRIE2_GET32_FROM_U16_SINGLE_LEAD(trie, start);
                }
                if(value2!=value) {
                    log_err("error: %s(%s).LSCU(U+%04lx)==0x%lx instead of 0x%lx\n",
                            typeName, testName, (long)start, (long)value2, (long)value);
                }
            }
            value2=utrie2_get32FromLeadSurrogateCodeUnit(trie, start);
            if(value2!=value) {
                log_err("error: %s(%s).lscu(U+%04lx)==0x%lx instead of 0x%lx\n",
                        typeName, testName, (long)start, (long)value2, (long)value);
            }
        }
    }

    /* test errorValue */
    if(isFrozen) {
        if(valueBits==UTRIE2_16_VALUE_BITS) {
            value=UTRIE2_GET16(trie, -1);
            value2=UTRIE2_GET16(trie, 0x110000);
        } else {
            value=UTRIE2_GET32(trie, -1);
            value2=UTRIE2_GET32(trie, 0x110000);
        }
        if(value!=errorValue || value2!=errorValue) {
            log_err("error: %s(%s).get(out of range) != errorValue\n",
                    typeName, testName);
        }
    }
    value=utrie2_get32(trie, -1);
    value2=utrie2_get32(trie, 0x110000);
    if(value!=errorValue || value2!=errorValue) {
        log_err("error: %s(%s).get32(out of range) != errorValue\n",
                typeName, testName);
    }
}

static void
testTrieUTF16(const char *testName,
              const UTrie2 *trie, UTrie2ValueBits valueBits,
              const CheckRange checkRanges[], int32_t countCheckRanges) {
    UChar s[200];
    uint32_t values[100];

    const UChar *p, *limit;

    uint32_t value;
    UChar32 prevCP, c, c2;
    int32_t i, length, sIndex, countValues;

    /* write a string */
    prevCP=0;
    length=countValues=0;
    for(i=skipSpecialValues(checkRanges, countCheckRanges); i<countCheckRanges; ++i) {
        value=checkRanges[i].value;
        /* write three code points */
        U16_APPEND_UNSAFE(s, length, prevCP);   /* start of the range */
        values[countValues++]=value;
        c=checkRanges[i].limit;
        prevCP=(prevCP+c)/2;                    /* middle of the range */
        U16_APPEND_UNSAFE(s, length, prevCP);
        values[countValues++]=value;
        prevCP=c;
        --c;                                    /* end of the range */
        U16_APPEND_UNSAFE(s, length, c);
        values[countValues++]=value;
    }
    limit=s+length;

    /* try forward */
    p=s;
    i=0;
    while(p<limit) {
        sIndex=(int32_t)(p-s);
        U16_NEXT(s, sIndex, length, c2);
        c=0x33;
        if(valueBits==UTRIE2_16_VALUE_BITS) {
            UTRIE2_U16_NEXT16(trie, p, limit, c, value);
        } else {
            UTRIE2_U16_NEXT32(trie, p, limit, c, value);
        }
        if(value!=values[i]) {
            log_err("error: wrong value from UTRIE2_NEXT(%s)(U+%04lx): 0x%lx instead of 0x%lx\n",
                    testName, (long)c, (long)value, (long)values[i]);
        }
        if(c!=c2) {
            log_err("error: wrong code point from UTRIE2_NEXT(%s): U+%04lx != U+%04lx\n",
                    testName, (long)c, (long)c2);
            continue;
        }
        ++i;
    }

    /* try backward */
    p=limit;
    i=countValues;
    while(s<p) {
        --i;
        sIndex=(int32_t)(p-s);
        U16_PREV(s, 0, sIndex, c2);
        c=0x33;
        if(valueBits==UTRIE2_16_VALUE_BITS) {
            UTRIE2_U16_PREV16(trie, s, p, c, value);
        } else {
            UTRIE2_U16_PREV32(trie, s, p, c, value);
        }
        if(value!=values[i]) {
            log_err("error: wrong value from UTRIE2_PREV(%s)(U+%04lx): 0x%lx instead of 0x%lx\n",
                    testName, (long)c, (long)value, (long)values[i]);
        }
        if(c!=c2) {
            log_err("error: wrong code point from UTRIE2_PREV(%s): U+%04lx != U+%04lx\n",
                    testName, c, c2);
        }
    }
}

static void
testTrieUTF8(const char *testName,
             const UTrie2 *trie, UTrie2ValueBits valueBits,
             const CheckRange checkRanges[], int32_t countCheckRanges) {
    static const uint8_t illegal[]={
        0xc0, 0x80,                         /* non-shortest U+0000 */
        0xc1, 0xbf,                         /* non-shortest U+007f */
        0xc2,                               /* truncated */
        0xe0, 0x90, 0x80,                   /* non-shortest U+0400 */
        0xe0, 0xa0,                         /* truncated */
        0xed, 0xa0, 0x80,                   /* lead surrogate U+d800 */
        0xed, 0xbf, 0xbf,                   /* trail surrogate U+dfff */
        0xf0, 0x8f, 0xbf, 0xbf,             /* non-shortest U+ffff */
        0xf0, 0x90, 0x80,                   /* truncated */
        0xf4, 0x90, 0x80, 0x80,             /* beyond-Unicode U+110000 */
        0xf8, 0x80, 0x80, 0x80,             /* truncated */
        0xf8, 0x80, 0x80, 0x80, 0x80,       /* 5-byte UTF-8 */
        0xfd, 0xbf, 0xbf, 0xbf, 0xbf,       /* truncated */
        0xfd, 0xbf, 0xbf, 0xbf, 0xbf, 0xbf, /* 6-byte UTF-8 */
        0xfe,
        0xff
    };
    uint8_t s[600];
    uint32_t values[200];

    const uint8_t *p, *limit;

    uint32_t initialValue, errorValue;
    uint32_t value, bytes;
    UChar32 prevCP, c;
    int32_t i, countSpecials, length, countValues;
    int32_t prev8, i8;

    countSpecials=getSpecialValues(checkRanges, countCheckRanges, &initialValue, &errorValue);

    /* write a string */
    prevCP=0;
    length=countValues=0;
    /* first a couple of trail bytes in lead position */
    s[length++]=0x80;
    values[countValues++]=errorValue;
    s[length++]=0xbf;
    values[countValues++]=errorValue;
    prev8=i8=0;
    for(i=countSpecials; i<countCheckRanges; ++i) {
        value=checkRanges[i].value;
        /* write three legal (or surrogate) code points */
        U8_APPEND_UNSAFE(s, length, prevCP);    /* start of the range */
        values[countValues++]=U_IS_SURROGATE(prevCP) ? errorValue : value;
        c=checkRanges[i].limit;
        prevCP=(prevCP+c)/2;                    /* middle of the range */
        U8_APPEND_UNSAFE(s, length, prevCP);
        values[countValues++]=U_IS_SURROGATE(prevCP) ? errorValue : value;
        prevCP=c;
        --c;                                    /* end of the range */
        U8_APPEND_UNSAFE(s, length, c);
        values[countValues++]=U_IS_SURROGATE(c) ? errorValue : value;
        /* write an illegal byte sequence */
        if(i8<sizeof(illegal)) {
            U8_FWD_1(illegal, i8, sizeof(illegal));
            while(prev8<i8) {
                s[length++]=illegal[prev8++];
            }
            values[countValues++]=errorValue;
        }
    }
    /* write the remaining illegal byte sequences */
    while(i8<sizeof(illegal)) {
        U8_FWD_1(illegal, i8, sizeof(illegal));
        while(prev8<i8) {
            s[length++]=illegal[prev8++];
        }
        values[countValues++]=errorValue;
    }
    limit=s+length;

    /* try forward */
    p=s;
    i=0;
    while(p<limit) {
        prev8=i8=(int32_t)(p-s);
        U8_NEXT(s, i8, length, c);
        if(valueBits==UTRIE2_16_VALUE_BITS) {
            UTRIE2_U8_NEXT16(trie, p, limit, value);
        } else {
            UTRIE2_U8_NEXT32(trie, p, limit, value);
        }
        bytes=0;
        if(value!=values[i] || i8!=(p-s)) {
            while(prev8<i8) {
                bytes=(bytes<<8)|s[prev8++];
            }
        }
        if(value!=values[i]) {
            log_err("error: wrong value from UTRIE2_U8_NEXT(%s)(%lx->U+%04lx): 0x%lx instead of 0x%lx\n",
                    testName, (unsigned long)bytes, (long)c, (long)value, (long)values[i]);
        }
        if(i8!=(p-s)) {
            log_err("error: wrong end index from UTRIE2_U8_NEXT(%s)(%lx->U+%04lx): %ld != %ld\n",
                    testName, (unsigned long)bytes, (long)c, (long)(p-s), (long)i8);
            continue;
        }
        ++i;
    }

    /* try backward */
    p=limit;
    i=countValues;
    while(s<p) {
        --i;
        prev8=i8=(int32_t)(p-s);
        U8_PREV(s, 0, i8, c);
        if(valueBits==UTRIE2_16_VALUE_BITS) {
            UTRIE2_U8_PREV16(trie, s, p, value);
        } else {
            UTRIE2_U8_PREV32(trie, s, p, value);
        }
        bytes=0;
        if(value!=values[i] || i8!=(p-s)) {
            int32_t k=i8;
            while(k<prev8) {
                bytes=(bytes<<8)|s[k++];
            }
        }
        if(value!=values[i]) {
            log_err("error: wrong value from UTRIE2_U8_PREV(%s)(%lx->U+%04lx): 0x%lx instead of 0x%lx\n",
                    testName, (unsigned long)bytes, (long)c, (long)value, (long)values[i]);
        }
        if(i8!=(p-s)) {
            log_err("error: wrong end index from UTRIE2_U8_PREV(%s)(%lx->U+%04lx): %ld != %ld\n",
                    testName, (unsigned long)bytes, (long)c, (long)(p-s), (long)i8);
            continue;
        }
    }
}

static void
testFrozenTrie(const char *testName,
               UTrie2 *trie, UTrie2ValueBits valueBits,
               const CheckRange checkRanges[], int32_t countCheckRanges) {
    UErrorCode errorCode;
    uint32_t value, value2;

    if(!utrie2_isFrozen(trie)) {
        log_err("error: utrie2_isFrozen(frozen %s) returned FALSE (not frozen)\n",
                testName);
        return;
    }

    testTrieGetters(testName, trie, valueBits, checkRanges, countCheckRanges);
    testTrieEnum(testName, trie, checkRanges, countCheckRanges);
    testTrieUTF16(testName, trie, valueBits, checkRanges, countCheckRanges);
    testTrieUTF8(testName, trie, valueBits, checkRanges, countCheckRanges);

    errorCode=U_ZERO_ERROR;
    value=utrie2_get32(trie, 1);
    utrie2_set32(trie, 1, 234, &errorCode);
    value2=utrie2_get32(trie, 1);
    if(errorCode!=U_NO_WRITE_PERMISSION || value2!=value) {
        log_err("error: utrie2_set32(frozen %s) failed: it set %s != U_NO_WRITE_PERMISSION\n",
                testName, u_errorName(errorCode));
        return;
    }

    errorCode=U_ZERO_ERROR;
    utrie2_setRange32(trie, 1, 5, 234, TRUE, &errorCode);
    value2=utrie2_get32(trie, 1);
    if(errorCode!=U_NO_WRITE_PERMISSION || value2!=value) {
        log_err("error: utrie2_setRange32(frozen %s) failed: it set %s != U_NO_WRITE_PERMISSION\n",
                testName, u_errorName(errorCode));
        return;
    }

    errorCode=U_ZERO_ERROR;
    value=utrie2_get32FromLeadSurrogateCodeUnit(trie, 0xd801);
    utrie2_set32ForLeadSurrogateCodeUnit(trie, 0xd801, 234, &errorCode);
    value2=utrie2_get32FromLeadSurrogateCodeUnit(trie, 0xd801);
    if(errorCode!=U_NO_WRITE_PERMISSION || value2!=value) {
        log_err("error: utrie2_set32ForLeadSurrogateCodeUnit(frozen %s) failed: "
                "it set %s != U_NO_WRITE_PERMISSION\n",
                testName, u_errorName(errorCode));
        return;
    }
}

static void
testNewTrie(const char *testName, const UTrie2 *trie,
            const CheckRange checkRanges[], int32_t countCheckRanges) {
    /* The valueBits are ignored for an unfrozen trie. */
    testTrieGetters(testName, trie, UTRIE2_COUNT_VALUE_BITS, checkRanges, countCheckRanges);
    testTrieEnum(testName, trie, checkRanges, countCheckRanges);
}

static void
testTrieSerialize(const char *testName,
                  UTrie2 *trie, UTrie2ValueBits valueBits,
                  UBool withSwap,
                  const CheckRange checkRanges[], int32_t countCheckRanges) {
    uint32_t storage[10000];
    int32_t length1, length2, length3;
    UTrie2ValueBits otherValueBits;
    UErrorCode errorCode;

    /* clone the trie so that the caller can reuse the original */
    errorCode=U_ZERO_ERROR;
    trie=utrie2_clone(trie, &errorCode);
    if(U_FAILURE(errorCode)) {
        log_err("error: utrie2_clone(unfrozen %s) failed - %s\n",
                testName, u_errorName(errorCode));
        return;
    }

    /*
     * This is not a loop, but simply a block that we can exit with "break"
     * when something goes wrong.
     */
    do {
        errorCode=U_ZERO_ERROR;
        utrie2_serialize(trie, storage, sizeof(storage), &errorCode);
        if(errorCode!=U_ILLEGAL_ARGUMENT_ERROR) {
            log_err("error: utrie2_serialize(unfrozen %s) set %s != U_ILLEGAL_ARGUMENT_ERROR\n",
                    testName, u_errorName(errorCode));
            break;
        }
        errorCode=U_ZERO_ERROR;
        utrie2_freeze(trie, valueBits, &errorCode);
        if(U_FAILURE(errorCode) || !utrie2_isFrozen(trie)) {
            log_err("error: utrie2_freeze(%s) failed: %s isFrozen: %d\n",
                    testName, u_errorName(errorCode), utrie2_isFrozen(trie));
            break;
        }
        otherValueBits= valueBits==UTRIE2_16_VALUE_BITS ? UTRIE2_32_VALUE_BITS : UTRIE2_16_VALUE_BITS;
        utrie2_freeze(trie, otherValueBits, &errorCode);
        if(errorCode!=U_ILLEGAL_ARGUMENT_ERROR) {
            log_err("error: utrie2_freeze(already-frozen with other valueBits %s) "
                    "set %s != U_ILLEGAL_ARGUMENT_ERROR\n",
                    testName, u_errorName(errorCode));
            break;
        }
        errorCode=U_ZERO_ERROR;
        if(withSwap) {
            /* clone a frozen trie */
            UTrie2 *clone=utrie2_clone(trie, &errorCode);
            if(U_FAILURE(errorCode)) {
                log_err("error: cloning a frozen UTrie2 failed (%s) - %s\n",
                        testName, u_errorName(errorCode));
                errorCode=U_ZERO_ERROR;  /* continue with the original */
            } else {
                utrie2_close(trie);
                trie=clone;
            }
        }
        length1=utrie2_serialize(trie, NULL, 0, &errorCode);
        if(errorCode!=U_BUFFER_OVERFLOW_ERROR) {
            log_err("error: utrie2_serialize(%s) preflighting set %s != U_BUFFER_OVERFLOW_ERROR\n",
                    testName, u_errorName(errorCode));
            break;
        }
        errorCode=U_ZERO_ERROR;
        length2=utrie2_serialize(trie, storage, sizeof(storage), &errorCode);
        if(errorCode==U_BUFFER_OVERFLOW_ERROR) {
            log_err("error: utrie2_serialize(%s) needs more memory\n", testName);
            break;
        }
        if(U_FAILURE(errorCode)) {
            log_err("error: utrie2_serialize(%s) failed: %s\n", testName, u_errorName(errorCode));
            break;
        }
        if(length1!=length2) {
            log_err("error: trie serialization (%s) lengths different: "
                    "preflight vs. serialize\n", testName);
            break;
        }

        testFrozenTrie(testName, trie, valueBits, checkRanges, countCheckRanges);
        utrie2_close(trie);
        trie=NULL;

        if(withSwap) {
            uint32_t swapped[10000];
            int32_t swappedLength;

            UDataSwapper *ds;

            /* swap to opposite-endian */
            uprv_memset(swapped, 0x55, length2);
            ds=udata_openSwapper(U_IS_BIG_ENDIAN, U_CHARSET_FAMILY,
                                 !U_IS_BIG_ENDIAN, U_CHARSET_FAMILY, &errorCode);
            swappedLength=utrie2_swap(ds, storage, -1, NULL, &errorCode);
            if(U_FAILURE(errorCode) || swappedLength!=length2) {
                log_err("error: utrie2_swap(%s to OE preflighting) failed (%s) "
                        "or before/after lengths different\n",
                        testName, u_errorName(errorCode));
                udata_closeSwapper(ds);
                break;
            }
            swappedLength=utrie2_swap(ds, storage, length2, swapped, &errorCode);
            udata_closeSwapper(ds);
            if(U_FAILURE(errorCode) || swappedLength!=length2) {
                log_err("error: utrie2_swap(%s to OE) failed (%s) or before/after lengths different\n",
                        testName, u_errorName(errorCode));
                break;
            }

            /* swap back to platform-endian */
            uprv_memset(storage, 0xaa, length2);
            ds=udata_openSwapper(!U_IS_BIG_ENDIAN, U_CHARSET_FAMILY,
                                 U_IS_BIG_ENDIAN, U_CHARSET_FAMILY, &errorCode);
            swappedLength=utrie2_swap(ds, swapped, -1, NULL, &errorCode);
            if(U_FAILURE(errorCode) || swappedLength!=length2) {
                log_err("error: utrie2_swap(%s to PE preflighting) failed (%s) "
                        "or before/after lengths different\n",
                        testName, u_errorName(errorCode));
                udata_closeSwapper(ds);
                break;
            }
            swappedLength=utrie2_swap(ds, swapped, length2, storage, &errorCode);
            udata_closeSwapper(ds);
            if(U_FAILURE(errorCode) || swappedLength!=length2) {
                log_err("error: utrie2_swap(%s to PE) failed (%s) or before/after lengths different\n",
                        testName, u_errorName(errorCode));
                break;
            }
        }

        trie=utrie2_openFromSerialized(valueBits, storage, length2, &length3, &errorCode);
        if(U_FAILURE(errorCode)) {
            log_err("error: utrie2_openFromSerialized(%s) failed, %s\n", testName, u_errorName(errorCode));
            break;
        }
        if((valueBits==UTRIE2_16_VALUE_BITS)!=(trie->data32==NULL)) {
            log_err("error: trie serialization (%s) did not preserve 32-bitness\n", testName);
            break;
        }
        if(length2!=length3) {
            log_err("error: trie serialization (%s) lengths different: "
                    "serialize vs. unserialize\n", testName);
            break;
        }
        /* overwrite the storage that is not supposed to be needed */
        uprv_memset((char *)storage+length3, 0xfa, (int32_t)(sizeof(storage)-length3));

        utrie2_freeze(trie, valueBits, &errorCode);
        if(U_FAILURE(errorCode) || !utrie2_isFrozen(trie)) {
            log_err("error: utrie2_freeze(unserialized %s) failed: %s isFrozen: %d\n",
                    testName, u_errorName(errorCode), utrie2_isFrozen(trie));
            break;
        }
        utrie2_freeze(trie, otherValueBits, &errorCode);
        if(errorCode!=U_ILLEGAL_ARGUMENT_ERROR) {
            log_err("error: utrie2_freeze(unserialized with other valueBits %s) "
                    "set %s != U_ILLEGAL_ARGUMENT_ERROR\n",
                    testName, u_errorName(errorCode));
            break;
        }
        errorCode=U_ZERO_ERROR;
        if(withSwap) {
            /* clone an unserialized trie */
            UTrie2 *clone=utrie2_clone(trie, &errorCode);
            if(U_FAILURE(errorCode)) {
                log_err("error: utrie2_clone(unserialized %s) failed - %s\n",
                        testName, u_errorName(errorCode));
                errorCode=U_ZERO_ERROR;
                /* no need to break: just test the original trie */
            } else {
                utrie2_close(trie);
                trie=clone;
                uprv_memset(storage, 0, sizeof(storage));
            }
        }
        testFrozenTrie(testName, trie, valueBits, checkRanges, countCheckRanges);
        {
            /* clone-as-thawed an unserialized trie */
            UTrie2 *clone=utrie2_cloneAsThawed(trie, &errorCode);
            if(U_FAILURE(errorCode) || utrie2_isFrozen(clone)) {
                log_err("error: utrie2_cloneAsThawed(unserialized %s) failed - "
                        "%s (isFrozen: %d)\n",
                        testName, u_errorName(errorCode), clone!=NULL && utrie2_isFrozen(trie));
                break;
            } else {
                utrie2_close(trie);
                trie=clone;
            }
        }
        {
            uint32_t value, value2;

            value=utrie2_get32(trie, 0xa1);
            utrie2_set32(trie, 0xa1, 789, &errorCode);
            value2=utrie2_get32(trie, 0xa1);
            utrie2_set32(trie, 0xa1, value, &errorCode);
            if(U_FAILURE(errorCode) || value2!=789) {
                log_err("error: modifying a cloneAsThawed UTrie2 (%s) failed - %s\n",
                        testName, u_errorName(errorCode));
            }
        }
        testNewTrie(testName, trie, checkRanges, countCheckRanges);
    } while(0);

    utrie2_close(trie);
}

static UTrie2 *
testTrieSerializeAllValueBits(const char *testName,
                              UTrie2 *trie, UBool withClone,
                              const CheckRange checkRanges[], int32_t countCheckRanges) {
    char name[40];

    /* verify that all the expected values are in the unfrozen trie */
    testNewTrie(testName, trie, checkRanges, countCheckRanges);

    /*
     * Test with both valueBits serializations,
     * and that utrie2_serialize() can be called multiple times.
     */
    uprv_strcpy(name, testName);
    uprv_strcat(name, ".16");
    testTrieSerialize(name, trie,
                      UTRIE2_16_VALUE_BITS, withClone,
                      checkRanges, countCheckRanges);

    if(withClone) {
        /*
         * try cloning after the first serialization;
         * clone-as-thawed just to sometimes try it on an unfrozen trie
         */
        UErrorCode errorCode=U_ZERO_ERROR;
        UTrie2 *clone=utrie2_cloneAsThawed(trie, &errorCode);
        if(U_FAILURE(errorCode)) {
            log_err("error: utrie2_cloneAsThawed(%s) after serialization failed - %s\n",
                    testName, u_errorName(errorCode));
        } else {
            utrie2_close(trie);
            trie=clone;

            testNewTrie(testName, trie, checkRanges, countCheckRanges);
        }
    }

    uprv_strcpy(name, testName);
    uprv_strcat(name, ".32");
    testTrieSerialize(name, trie,
                      UTRIE2_32_VALUE_BITS, withClone,
                      checkRanges, countCheckRanges);

    return trie; /* could be the clone */
}

static UTrie2 *
makeTrieWithRanges(const char *testName, UBool withClone,
                   const SetRange setRanges[], int32_t countSetRanges,
                   const CheckRange checkRanges[], int32_t countCheckRanges) {
    UTrie2 *trie;
    uint32_t initialValue, errorValue;
    uint32_t value;
    UChar32 start, limit;
    int32_t i;
    UErrorCode errorCode;
    UBool overwrite;

    log_verbose("\ntesting Trie '%s'\n", testName);
    errorCode=U_ZERO_ERROR;
    getSpecialValues(checkRanges, countCheckRanges, &initialValue, &errorValue);
    trie=utrie2_open(initialValue, errorValue, &errorCode);
    if(U_FAILURE(errorCode)) {
        log_err("error: utrie2_open(%s) failed: %s\n", testName, u_errorName(errorCode));
        return NULL;
    }

    /* set values from setRanges[] */
    for(i=0; i<countSetRanges; ++i) {
        if(withClone && i==countSetRanges/2) {
            /* switch to a clone in the middle of setting values */
            UTrie2 *clone=utrie2_clone(trie, &errorCode);
            if(U_FAILURE(errorCode)) {
                log_err("error: utrie2_clone(%s) failed - %s\n",
                        testName, u_errorName(errorCode));
                errorCode=U_ZERO_ERROR;  /* continue with the original */
            } else {
                utrie2_close(trie);
                trie=clone;
            }
        }
        start=setRanges[i].start;
        limit=setRanges[i].limit;
        value=setRanges[i].value;
        overwrite=setRanges[i].overwrite;
        if((limit-start)==1 && overwrite) {
            utrie2_set32(trie, start, value, &errorCode);
        } else {
            utrie2_setRange32(trie, start, limit-1, value, overwrite, &errorCode);
        }
    }

    /* set some values for lead surrogate code units */
    utrie2_set32ForLeadSurrogateCodeUnit(trie, 0xd800, 90, &errorCode);
    utrie2_set32ForLeadSurrogateCodeUnit(trie, 0xd999, 94, &errorCode);
    utrie2_set32ForLeadSurrogateCodeUnit(trie, 0xdbff, 99, &errorCode);
    if(U_SUCCESS(errorCode)) {
        return trie;
    } else {
        log_err("error: setting values into a trie (%s) failed - %s\n",
                testName, u_errorName(errorCode));
        utrie2_close(trie);
        return NULL;
    }
}

static void
testTrieRanges(const char *testName, UBool withClone,
               const SetRange setRanges[], int32_t countSetRanges,
               const CheckRange checkRanges[], int32_t countCheckRanges) {
    UTrie2 *trie=makeTrieWithRanges(testName, withClone,
                                    setRanges, countSetRanges,
                                    checkRanges, countCheckRanges);
    if(trie!=NULL) {
        trie=testTrieSerializeAllValueBits(testName, trie, withClone,
                                           checkRanges, countCheckRanges);
        utrie2_close(trie);
    }
}

/* test data ----------------------------------------------------------------*/

/* set consecutive ranges, even with value 0 */
static const SetRange
setRanges1[]={
    { 0,        0x40,     0,      FALSE },
    { 0x40,     0xe7,     0x1234, FALSE },
    { 0xe7,     0x3400,   0,      FALSE },
    { 0x3400,   0x9fa6,   0x6162, FALSE },
    { 0x9fa6,   0xda9e,   0x3132, FALSE },
    { 0xdada,   0xeeee,   0x87ff, FALSE },
    { 0xeeee,   0x11111,  1,      FALSE },
    { 0x11111,  0x44444,  0x6162, FALSE },
    { 0x44444,  0x60003,  0,      FALSE },
    { 0xf0003,  0xf0004,  0xf,    FALSE },
    { 0xf0004,  0xf0006,  0x10,   FALSE },
    { 0xf0006,  0xf0007,  0x11,   FALSE },
    { 0xf0007,  0xf0040,  0x12,   FALSE },
    { 0xf0040,  0x110000, 0,      FALSE }
};

static const CheckRange
checkRanges1[]={
    { 0,        0 },
    { 0x40,     0 },
    { 0xe7,     0x1234 },
    { 0x3400,   0 },
    { 0x9fa6,   0x6162 },
    { 0xda9e,   0x3132 },
    { 0xdada,   0 },
    { 0xeeee,   0x87ff },
    { 0x11111,  1 },
    { 0x44444,  0x6162 },
    { 0xf0003,  0 },
    { 0xf0004,  0xf },
    { 0xf0006,  0x10 },
    { 0xf0007,  0x11 },
    { 0xf0040,  0x12 },
    { 0x110000, 0 }
};

/* set some interesting overlapping ranges */
static const SetRange
setRanges2[]={
    { 0x21,     0x7f,     0x5555, TRUE },
    { 0x2f800,  0x2fedc,  0x7a,   TRUE },
    { 0x72,     0xdd,     3,      TRUE },
    { 0xdd,     0xde,     4,      FALSE },
    { 0x201,    0x240,    6,      TRUE },  /* 3 consecutive blocks with the same pattern but */
    { 0x241,    0x280,    6,      TRUE },  /* discontiguous value ranges, testing utrie2_enum() */
    { 0x281,    0x2c0,    6,      TRUE },
    { 0x2f987,  0x2fa98,  5,      TRUE },
    { 0x2f777,  0x2f883,  0,      TRUE },
    { 0x2f900,  0x2ffaa,  1,      FALSE },
    { 0x2ffaa,  0x2ffab,  2,      TRUE },
    { 0x2ffbb,  0x2ffc0,  7,      TRUE }
};

static const CheckRange
checkRanges2[]={
    { 0,        0 },
    { 0x21,     0 },
    { 0x72,     0x5555 },
    { 0xdd,     3 },
    { 0xde,     4 },
    { 0x201,    0 },
    { 0x240,    6 },
    { 0x241,    0 },
    { 0x280,    6 },
    { 0x281,    0 },
    { 0x2c0,    6 },
    { 0x2f883,  0 },
    { 0x2f987,  0x7a },
    { 0x2fa98,  5 },
    { 0x2fedc,  0x7a },
    { 0x2ffaa,  1 },
    { 0x2ffab,  2 },
    { 0x2ffbb,  0 },
    { 0x2ffc0,  7 },
    { 0x110000, 0 }
};

static const CheckRange
checkRanges2_d800[]={
    { 0x10000,  0 },
    { 0x10400,  0 }
};

static const CheckRange
checkRanges2_d87e[]={
    { 0x2f800,  6 },
    { 0x2f883,  0 },
    { 0x2f987,  0x7a },
    { 0x2fa98,  5 },
    { 0x2fc00,  0x7a }
};

static const CheckRange
checkRanges2_d87f[]={
    { 0x2fc00,  0 },
    { 0x2fedc,  0x7a },
    { 0x2ffaa,  1 },
    { 0x2ffab,  2 },
    { 0x2ffbb,  0 },
    { 0x2ffc0,  7 },
    { 0x30000,  0 }
};

static const CheckRange
checkRanges2_dbff[]={
    { 0x10fc00, 0 },
    { 0x110000, 0 }
};

/* use a non-zero initial value */
static const SetRange
setRanges3[]={
    { 0x31,     0xa4,     1, FALSE },
    { 0x3400,   0x6789,   2, FALSE },
    { 0x8000,   0x89ab,   9, TRUE },
    { 0x9000,   0xa000,   4, TRUE },
    { 0xabcd,   0xbcde,   3, TRUE },
    { 0x55555,  0x110000, 6, TRUE },  /* highStart<U+ffff with non-initialValue */
    { 0xcccc,   0x55555,  6, TRUE }
};

static const CheckRange
checkRanges3[]={
    { 0,        9 },  /* non-zero initialValue */
    { 0x31,     9 },
    { 0xa4,     1 },
    { 0x3400,   9 },
    { 0x6789,   2 },
    { 0x9000,   9 },
    { 0xa000,   4 },
    { 0xabcd,   9 },
    { 0xbcde,   3 },
    { 0xcccc,   9 },
    { 0x110000, 6 }
};

/* empty or single-value tries, testing highStart==0 */
static const SetRange
setRangesEmpty[]={
    { 0,        0,        0, FALSE },  /* need some values for it to compile */
};

static const CheckRange
checkRangesEmpty[]={
    { 0,        3 },
    { 0x110000, 3 }
};

static const SetRange
setRangesSingleValue[]={
    { 0,        0x110000, 5, TRUE },
};

static const CheckRange
checkRangesSingleValue[]={
    { 0,        3 },
    { 0x110000, 5 }
};

static void
TrieTest(void) {
    testTrieRanges("set1", FALSE,
        setRanges1, LENGTHOF(setRanges1),
        checkRanges1, LENGTHOF(checkRanges1));
    testTrieRanges("set2-overlap", FALSE,
        setRanges2, LENGTHOF(setRanges2),
        checkRanges2, LENGTHOF(checkRanges2));
    testTrieRanges("set3-initial-9", FALSE,
        setRanges3, LENGTHOF(setRanges3),
        checkRanges3, LENGTHOF(checkRanges3));
    testTrieRanges("set-empty", FALSE,
        setRangesEmpty, 0,
        checkRangesEmpty, LENGTHOF(checkRangesEmpty));
    testTrieRanges("set-single-value", FALSE,
        setRangesSingleValue, LENGTHOF(setRangesSingleValue),
        checkRangesSingleValue, LENGTHOF(checkRangesSingleValue));

    testTrieRanges("set2-overlap.withClone", TRUE,
        setRanges2, LENGTHOF(setRanges2),
        checkRanges2, LENGTHOF(checkRanges2));
}

static void
EnumNewTrieForLeadSurrogateTest(void) {
    static const char *const testName="enum-for-lead";
    UTrie2 *trie=makeTrieWithRanges(testName, FALSE,
                                    setRanges2, LENGTHOF(setRanges2),
                                    checkRanges2, LENGTHOF(checkRanges2));
    while(trie!=NULL) {
        const CheckRange *checkRanges;

        checkRanges=checkRanges2_d800+1;
        utrie2_enumForLeadSurrogate(trie, 0xd800,
                                    testEnumValue, testEnumRange,
                                    &checkRanges);
        checkRanges=checkRanges2_d87e+1;
        utrie2_enumForLeadSurrogate(trie, 0xd87e,
                                    testEnumValue, testEnumRange,
                                    &checkRanges);
        checkRanges=checkRanges2_d87f+1;
        utrie2_enumForLeadSurrogate(trie, 0xd87f,
                                    testEnumValue, testEnumRange,
                                    &checkRanges);
        checkRanges=checkRanges2_dbff+1;
        utrie2_enumForLeadSurrogate(trie, 0xdbff,
                                    testEnumValue, testEnumRange,
                                    &checkRanges);
        if(!utrie2_isFrozen(trie)) {
            UErrorCode errorCode=U_ZERO_ERROR;
            utrie2_freeze(trie, UTRIE2_16_VALUE_BITS, &errorCode);
            if(U_FAILURE(errorCode)) {
                log_err("error: utrie2_freeze(%s) failed\n", testName);
                utrie2_close(trie);
                return;
            }
        } else {
            utrie2_close(trie);
            break;
        }
    }
}

/* test utrie2_openDummy() -------------------------------------------------- */

static void
dummyTest(UTrie2ValueBits valueBits) {
    CheckRange
    checkRanges[]={
        { -1,       0 },
        { 0,        0 },
        { 0x110000, 0 }
    };

    UTrie2 *trie;
    UErrorCode errorCode;

    const char *testName;
    uint32_t initialValue, errorValue;

    if(valueBits==UTRIE2_16_VALUE_BITS) {
        testName="dummy.16";
        initialValue=0x313;
        errorValue=0xaffe;
    } else {
        testName="dummy.32";
        initialValue=0x01234567;
        errorValue=0x89abcdef;
    }
    checkRanges[0].value=errorValue;
    checkRanges[1].value=checkRanges[2].value=initialValue;

    errorCode=U_ZERO_ERROR;
    trie=utrie2_openDummy(valueBits, initialValue, errorValue, &errorCode);
    if(U_FAILURE(errorCode)) {
        log_err("utrie2_openDummy(valueBits=%d) failed - %s\n", valueBits, u_errorName(errorCode));
        return;
    }

    testFrozenTrie(testName, trie, valueBits, checkRanges, LENGTHOF(checkRanges));
    utrie2_close(trie);
}

static void
DummyTrieTest(void) {
    dummyTest(UTRIE2_16_VALUE_BITS);
    dummyTest(UTRIE2_32_VALUE_BITS);
}

/* test builder memory management ------------------------------------------- */

static void
FreeBlocksTest(void) {
    static const CheckRange
    checkRanges[]={
        { 0,        1 },
        { 0x740,    1 },
        { 0x780,    2 },
        { 0x880,    3 },
        { 0x110000, 1 }
    };
    static const char *const testName="free-blocks";

    UTrie2 *trie;
    int32_t i;
    UErrorCode errorCode;

    errorCode=U_ZERO_ERROR;
    trie=utrie2_open(1, 0xbad, &errorCode);
    if(U_FAILURE(errorCode)) {
        log_err("error: utrie2_open(%s) failed: %s\n", testName, u_errorName(errorCode));
        return;
    }

    /*
     * Repeatedly set overlapping same-value ranges to stress the free-data-block management.
     * If it fails, it will overflow the data array.
     */
    for(i=0; i<(0x120000>>UTRIE2_SHIFT_2)/2; ++i) {
        utrie2_setRange32(trie, 0x740, 0x840-1, 1, TRUE, &errorCode);
        utrie2_setRange32(trie, 0x780, 0x880-1, 1, TRUE, &errorCode);
        utrie2_setRange32(trie, 0x740, 0x840-1, 2, TRUE, &errorCode);
        utrie2_setRange32(trie, 0x780, 0x880-1, 3, TRUE, &errorCode);
    }
    /* make blocks that will be free during compaction */
    utrie2_setRange32(trie, 0x1000, 0x3000-1, 2, TRUE, &errorCode);
    utrie2_setRange32(trie, 0x2000, 0x4000-1, 3, TRUE, &errorCode);
    utrie2_setRange32(trie, 0x1000, 0x4000-1, 1, TRUE, &errorCode);
    /* set some values for lead surrogate code units */
    utrie2_set32ForLeadSurrogateCodeUnit(trie, 0xd800, 90, &errorCode);
    utrie2_set32ForLeadSurrogateCodeUnit(trie, 0xd999, 94, &errorCode);
    utrie2_set32ForLeadSurrogateCodeUnit(trie, 0xdbff, 99, &errorCode);
    if(U_FAILURE(errorCode)) {
        log_err("error: setting lots of ranges into a trie (%s) failed - %s\n",
                testName, u_errorName(errorCode));
        utrie2_close(trie);
        return;
    }

    trie=testTrieSerializeAllValueBits(testName, trie, FALSE,
                                       checkRanges, LENGTHOF(checkRanges));
    utrie2_close(trie);
}

static void
GrowDataArrayTest(void) {
    static const CheckRange
    checkRanges[]={
        { 0,        1 },
        { 0x720,    2 },
        { 0x7a0,    3 },
        { 0x8a0,    4 },
        { 0x110000, 5 }
    };
    static const char *const testName="grow-data";

    UTrie2 *trie;
    int32_t i;
    UErrorCode errorCode;

    errorCode=U_ZERO_ERROR;
    trie=utrie2_open(1, 0xbad, &errorCode);
    if(U_FAILURE(errorCode)) {
        log_err("error: utrie2_open(%s) failed: %s\n", testName, u_errorName(errorCode));
        return;
    }

    /*
     * Use utrie2_set32() not utrie2_setRange32() to write non-initialValue-data.
     * Should grow/reallocate the data array to a sufficient length.
     */
    for(i=0; i<0x1000; ++i) {
        utrie2_set32(trie, i, 2, &errorCode);
    }
    for(i=0x720; i<0x1100; ++i) { /* some overlap */
        utrie2_set32(trie, i, 3, &errorCode);
    }
    for(i=0x7a0; i<0x900; ++i) {
        utrie2_set32(trie, i, 4, &errorCode);
    }
    for(i=0x8a0; i<0x110000; ++i) {
        utrie2_set32(trie, i, 5, &errorCode);
    }
    for(i=0xd800; i<0xdc00; ++i) {
        utrie2_set32ForLeadSurrogateCodeUnit(trie, i, 1, &errorCode);
    }
    /* set some values for lead surrogate code units */
    utrie2_set32ForLeadSurrogateCodeUnit(trie, 0xd800, 90, &errorCode);
    utrie2_set32ForLeadSurrogateCodeUnit(trie, 0xd999, 94, &errorCode);
    utrie2_set32ForLeadSurrogateCodeUnit(trie, 0xdbff, 99, &errorCode);
    if(U_FAILURE(errorCode)) {
        log_err("error: setting lots of values into a trie (%s) failed - %s\n",
                testName, u_errorName(errorCode));
        utrie2_close(trie);
        return;
    }

    trie=testTrieSerializeAllValueBits(testName, trie, FALSE,
                                          checkRanges, LENGTHOF(checkRanges));
    utrie2_close(trie);
}

/* versions 1 and 2 --------------------------------------------------------- */

static void
GetVersionTest(void) {
    uint32_t data[4];
    if( /* version 1 */
        (data[0]=0x54726965, 1!=utrie2_getVersion(data, sizeof(data), FALSE)) ||
        (data[0]=0x54726965, 1!=utrie2_getVersion(data, sizeof(data), TRUE)) ||
        (data[0]=0x65697254, 0!=utrie2_getVersion(data, sizeof(data), FALSE)) ||
        (data[0]=0x65697254, 1!=utrie2_getVersion(data, sizeof(data), TRUE)) ||
        /* version 2 */
        (data[0]=0x54726932, 2!=utrie2_getVersion(data, sizeof(data), FALSE)) ||
        (data[0]=0x54726932, 2!=utrie2_getVersion(data, sizeof(data), TRUE)) ||
        (data[0]=0x32697254, 0!=utrie2_getVersion(data, sizeof(data), FALSE)) ||
        (data[0]=0x32697254, 2!=utrie2_getVersion(data, sizeof(data), TRUE)) ||
        /* illegal arguments */
        (data[0]=0x54726932, 0!=utrie2_getVersion(NULL, sizeof(data), FALSE)) ||
        (data[0]=0x54726932, 0!=utrie2_getVersion(data, 3, FALSE)) ||
        (data[0]=0x54726932, 0!=utrie2_getVersion((char *)data+1, sizeof(data), FALSE)) ||
        /* unknown signature values */
        (data[0]=0x11223344, 0!=utrie2_getVersion(data, sizeof(data), FALSE)) ||
        (data[0]=0x54726933, 0!=utrie2_getVersion(data, sizeof(data), FALSE))
    ) {
        log_err("error: utrie2_getVersion() is not working as expected\n");
    }
}

static UNewTrie *
makeNewTrie1WithRanges(const char *testName,
                       const SetRange setRanges[], int32_t countSetRanges,
                       const CheckRange checkRanges[], int32_t countCheckRanges) {
    UNewTrie *newTrie;
    uint32_t initialValue, errorValue;
    uint32_t value;
    UChar32 start, limit;
    int32_t i;
    UErrorCode errorCode;
    UBool overwrite, ok;

    log_verbose("\ntesting Trie '%s'\n", testName);
    errorCode=U_ZERO_ERROR;
    getSpecialValues(checkRanges, countCheckRanges, &initialValue, &errorValue);
    newTrie=utrie_open(NULL, NULL, 2000,
                       initialValue, initialValue,
                       FALSE);
    if(U_FAILURE(errorCode)) {
        log_err("error: utrie_open(%s) failed: %s\n", testName, u_errorName(errorCode));
        return NULL;
    }

    /* set values from setRanges[] */
    ok=TRUE;
    for(i=0; i<countSetRanges; ++i) {
        start=setRanges[i].start;
        limit=setRanges[i].limit;
        value=setRanges[i].value;
        overwrite=setRanges[i].overwrite;
        if((limit-start)==1 && overwrite) {
            ok&=utrie_set32(newTrie, start, value);
        } else {
            ok&=utrie_setRange32(newTrie, start, limit, value, overwrite);
        }
    }
    if(ok) {
        return newTrie;
    } else {
        log_err("error: setting values into a trie1 (%s) failed\n", testName);
        utrie_close(newTrie);
        return NULL;
    }
}

static void
testTrie2FromTrie1(const char *testName,
                   const SetRange setRanges[], int32_t countSetRanges,
                   const CheckRange checkRanges[], int32_t countCheckRanges) {
    uint32_t memory1_16[3000], memory1_32[3000];
    int32_t length16, length32;
    UChar lead;

    char name[40];

    UNewTrie *newTrie1_16, *newTrie1_32;
    UTrie trie1_16, trie1_32;
    UTrie2 *trie2;
    uint32_t initialValue, errorValue;
    UErrorCode errorCode;

    newTrie1_16=makeNewTrie1WithRanges(testName,
                                       setRanges, countSetRanges,
                                       checkRanges, countCheckRanges);
    if(newTrie1_16==NULL) {
        return;
    }
    newTrie1_32=utrie_clone(NULL, newTrie1_16, NULL, 0);
    if(newTrie1_32==NULL) {
        utrie_close(newTrie1_16);
        return;
    }
    errorCode=U_ZERO_ERROR;
    length16=utrie_serialize(newTrie1_16, memory1_16, sizeof(memory1_16),
                             NULL, TRUE, &errorCode);
    length32=utrie_serialize(newTrie1_32, memory1_32, sizeof(memory1_32),
                             NULL, FALSE, &errorCode);
    utrie_unserialize(&trie1_16, memory1_16, length16, &errorCode);
    utrie_unserialize(&trie1_32, memory1_32, length32, &errorCode);
    utrie_close(newTrie1_16);
    utrie_close(newTrie1_32);
    if(U_FAILURE(errorCode)) {
        log_err("error: utrie_serialize or unserialize(%s) failed: %s\n",
                testName, u_errorName(errorCode));
        return;
    }

    getSpecialValues(checkRanges, countCheckRanges, &initialValue, &errorValue);

    uprv_strcpy(name, testName);
    uprv_strcat(name, ".16");
    trie2=utrie2_fromUTrie(&trie1_16, errorValue, &errorCode);
    if(U_SUCCESS(errorCode)) {
        testFrozenTrie(name, trie2, UTRIE2_16_VALUE_BITS, checkRanges, countCheckRanges);
        for(lead=0xd800; lead<0xdc00; ++lead) {
            uint32_t value1, value2;
            value1=UTRIE_GET16_FROM_LEAD(&trie1_16, lead);
            value2=UTRIE2_GET16_FROM_U16_SINGLE_LEAD(trie2, lead);
            if(value1!=value2) {
                log_err("error: utrie2_fromUTrie(%s) wrong value %ld!=%ld "
                        "from lead surrogate code unit U+%04lx\n",
                        name, (long)value2, (long)value1, (long)lead);
                break;
            }
        }
    }
    utrie2_close(trie2);

    uprv_strcpy(name, testName);
    uprv_strcat(name, ".32");
    trie2=utrie2_fromUTrie(&trie1_32, errorValue, &errorCode);
    if(U_SUCCESS(errorCode)) {
        testFrozenTrie(name, trie2, UTRIE2_32_VALUE_BITS, checkRanges, countCheckRanges);
        for(lead=0xd800; lead<0xdc00; ++lead) {
            uint32_t value1, value2;
            value1=UTRIE_GET32_FROM_LEAD(&trie1_32, lead);
            value2=UTRIE2_GET32_FROM_U16_SINGLE_LEAD(trie2, lead);
            if(value1!=value2) {
                log_err("error: utrie2_fromUTrie(%s) wrong value %ld!=%ld "
                        "from lead surrogate code unit U+%04lx\n",
                        name, (long)value2, (long)value1, (long)lead);
                break;
            }
        }
    }
    utrie2_close(trie2);
}

static void
Trie12ConversionTest(void) {
    testTrie2FromTrie1("trie1->trie2",
                       setRanges2, LENGTHOF(setRanges2),
                       checkRanges2, LENGTHOF(checkRanges2));
}

void
addTrie2Test(TestNode** root) {
    addTest(root, &TrieTest, "tsutil/trie2test/TrieTest");
    addTest(root, &EnumNewTrieForLeadSurrogateTest,
                  "tsutil/trie2test/EnumNewTrieForLeadSurrogateTest");
    addTest(root, &DummyTrieTest, "tsutil/trie2test/DummyTrieTest");
    addTest(root, &FreeBlocksTest, "tsutil/trie2test/FreeBlocksTest");
    addTest(root, &GrowDataArrayTest, "tsutil/trie2test/GrowDataArrayTest");
    addTest(root, &GetVersionTest, "tsutil/trie2test/GetVersionTest");
    addTest(root, &Trie12ConversionTest, "tsutil/trie2test/Trie12ConversionTest");
}
