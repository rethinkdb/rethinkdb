/*
******************************************************************************
*
*   Copyright (C) 2002-2010, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
******************************************************************************
*   file name:  bocu1tst.c
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2002may27
*   created by: Markus W. Scherer
*
*   This is the reference implementation of BOCU-1,
*   the MIME-friendly form of the Binary Ordered Compression for Unicode,
*   taken directly from ### http://source.icu-project.org/repos/icu/icuhtml/trunk/design/conversion/bocu1/
*   The files bocu1.h and bocu1.c from the design folder are taken
*   verbatim (minus copyright and #include) and copied together into this file.
*   The reference code and some of the reference bocu1tst.c
*   is modified to run as part of the ICU cintltst
*   test framework (minus main(), log_ln() etc. instead of printf()).
*
*   This reference implementation is used here to verify
*   the ICU BOCU-1 implementation, which is
*   adapted for ICU conversion APIs and optimized.
*   ### links in design doc to here and to ucnvbocu.c
*/

#include "unicode/utypes.h"
#include "unicode/ustring.h"
#include "unicode/ucnv.h"
#include "cmemory.h"
#include "cintltst.h"

#define LENGTHOF(array) (sizeof(array)/sizeof((array)[0]))

/* icuhtml/design/conversion/bocu1/bocu1.h ---------------------------------- */

/* BOCU-1 constants and macros ---------------------------------------------- */

/*
 * BOCU-1 encodes the code points of a Unicode string as
 * a sequence of byte-encoded differences (slope detection),
 * preserving lexical order.
 *
 * Optimize the difference-taking for runs of Unicode text within
 * small scripts:
 *
 * Most small scripts are allocated within aligned 128-blocks of Unicode
 * code points. Lexical order is preserved if the "previous code point" state
 * is always moved into the middle of such a block.
 *
 * Additionally, "prev" is moved from anywhere in the Unihan and Hangul
 * areas into the middle of those areas.
 *
 * C0 control codes and space are encoded with their US-ASCII bytes.
 * "prev" is reset for C0 controls but not for space.
 */

/* initial value for "prev": middle of the ASCII range */
#define BOCU1_ASCII_PREV        0x40

/* bounding byte values for differences */
#define BOCU1_MIN               0x21
#define BOCU1_MIDDLE            0x90
#define BOCU1_MAX_LEAD          0xfe

/* add the L suffix to make computations with BOCU1_MAX_TRAIL work on 16-bit compilers */
#define BOCU1_MAX_TRAIL         0xffL
#define BOCU1_RESET             0xff

/* number of lead bytes */
#define BOCU1_COUNT             (BOCU1_MAX_LEAD-BOCU1_MIN+1)

/* adjust trail byte counts for the use of some C0 control byte values */
#define BOCU1_TRAIL_CONTROLS_COUNT  20
#define BOCU1_TRAIL_BYTE_OFFSET     (BOCU1_MIN-BOCU1_TRAIL_CONTROLS_COUNT)

/* number of trail bytes */
#define BOCU1_TRAIL_COUNT       ((BOCU1_MAX_TRAIL-BOCU1_MIN+1)+BOCU1_TRAIL_CONTROLS_COUNT)

/*
 * number of positive and negative single-byte codes
 * (counting 0==BOCU1_MIDDLE among the positive ones)
 */
#define BOCU1_SINGLE            64

/* number of lead bytes for positive and negative 2/3/4-byte sequences */
#define BOCU1_LEAD_2            43
#define BOCU1_LEAD_3            3
#define BOCU1_LEAD_4            1

/* The difference value range for single-byters. */
#define BOCU1_REACH_POS_1   (BOCU1_SINGLE-1)
#define BOCU1_REACH_NEG_1   (-BOCU1_SINGLE)

/* The difference value range for double-byters. */
#define BOCU1_REACH_POS_2   (BOCU1_REACH_POS_1+BOCU1_LEAD_2*BOCU1_TRAIL_COUNT)
#define BOCU1_REACH_NEG_2   (BOCU1_REACH_NEG_1-BOCU1_LEAD_2*BOCU1_TRAIL_COUNT)

/* The difference value range for 3-byters. */
#define BOCU1_REACH_POS_3   \
    (BOCU1_REACH_POS_2+BOCU1_LEAD_3*BOCU1_TRAIL_COUNT*BOCU1_TRAIL_COUNT)

#define BOCU1_REACH_NEG_3   (BOCU1_REACH_NEG_2-BOCU1_LEAD_3*BOCU1_TRAIL_COUNT*BOCU1_TRAIL_COUNT)

/* The lead byte start values. */
#define BOCU1_START_POS_2   (BOCU1_MIDDLE+BOCU1_REACH_POS_1+1)
#define BOCU1_START_POS_3   (BOCU1_START_POS_2+BOCU1_LEAD_2)
#define BOCU1_START_POS_4   (BOCU1_START_POS_3+BOCU1_LEAD_3)
     /* ==BOCU1_MAX_LEAD */

#define BOCU1_START_NEG_2   (BOCU1_MIDDLE+BOCU1_REACH_NEG_1)
#define BOCU1_START_NEG_3   (BOCU1_START_NEG_2-BOCU1_LEAD_2)
#define BOCU1_START_NEG_4   (BOCU1_START_NEG_3-BOCU1_LEAD_3)
     /* ==BOCU1_MIN+1 */

/* The length of a byte sequence, according to the lead byte (!=BOCU1_RESET). */
#define BOCU1_LENGTH_FROM_LEAD(lead) \
    ((BOCU1_START_NEG_2<=(lead) && (lead)<BOCU1_START_POS_2) ? 1 : \
     (BOCU1_START_NEG_3<=(lead) && (lead)<BOCU1_START_POS_3) ? 2 : \
     (BOCU1_START_NEG_4<=(lead) && (lead)<BOCU1_START_POS_4) ? 3 : 4)

/* The length of a byte sequence, according to its packed form. */
#define BOCU1_LENGTH_FROM_PACKED(packed) \
    ((uint32_t)(packed)<0x04000000 ? (packed)>>24 : 4)

/*
 * 12 commonly used C0 control codes (and space) are only used to encode
 * themselves directly,
 * which makes BOCU-1 MIME-usable and reasonably safe for
 * ASCII-oriented software.
 *
 * These controls are
 *  0   NUL
 *
 *  7   BEL
 *  8   BS
 *
 *  9   TAB
 *  a   LF
 *  b   VT
 *  c   FF
 *  d   CR
 *
 *  e   SO
 *  f   SI
 *
 * 1a   SUB
 * 1b   ESC
 *
 * The other 20 C0 controls are also encoded directly (to preserve order)
 * but are also used as trail bytes in difference encoding
 * (for better compression).
 */
#define BOCU1_TRAIL_TO_BYTE(t) ((t)>=BOCU1_TRAIL_CONTROLS_COUNT ? (t)+BOCU1_TRAIL_BYTE_OFFSET : bocu1TrailToByte[t])

/*
 * Byte value map for control codes,
 * from external byte values 0x00..0x20
 * to trail byte values 0..19 (0..0x13) as used in the difference calculation.
 * External byte values that are illegal as trail bytes are mapped to -1.
 */
static const int8_t
bocu1ByteToTrail[BOCU1_MIN]={
/*  0     1     2     3     4     5     6     7    */
    -1,   0x00, 0x01, 0x02, 0x03, 0x04, 0x05, -1,

/*  8     9     a     b     c     d     e     f    */
    -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,

/*  10    11    12    13    14    15    16    17   */
    0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d,

/*  18    19    1a    1b    1c    1d    1e    1f   */
    0x0e, 0x0f, -1,   -1,   0x10, 0x11, 0x12, 0x13,

/*  20   */
    -1
};

/*
 * Byte value map for control codes,
 * from trail byte values 0..19 (0..0x13) as used in the difference calculation
 * to external byte values 0x00..0x20.
 */
static const int8_t
bocu1TrailToByte[BOCU1_TRAIL_CONTROLS_COUNT]={
/*  0     1     2     3     4     5     6     7    */
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x10, 0x11,

/*  8     9     a     b     c     d     e     f    */
    0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19,

/*  10    11    12    13   */
    0x1c, 0x1d, 0x1e, 0x1f
};

/**
 * Integer division and modulo with negative numerators
 * yields negative modulo results and quotients that are one more than
 * what we need here.
 * This macro adjust the results so that the modulo-value m is always >=0.
 *
 * For positive n, the if() condition is always FALSE.
 *
 * @param n Number to be split into quotient and rest.
 *          Will be modified to contain the quotient.
 * @param d Divisor.
 * @param m Output variable for the rest (modulo result).
 */
#define NEGDIVMOD(n, d, m) { \
    (m)=(n)%(d); \
    (n)/=(d); \
    if((m)<0) { \
        --(n); \
        (m)+=(d); \
    } \
}

/* State for BOCU-1 decoder function. */
struct Bocu1Rx {
    int32_t prev, count, diff;
};

typedef struct Bocu1Rx Bocu1Rx;

/* Function prototypes ------------------------------------------------------ */

/* see bocu1.c */
U_CFUNC int32_t
packDiff(int32_t diff);

U_CFUNC int32_t
encodeBocu1(int32_t *pPrev, int32_t c);

U_CFUNC int32_t
decodeBocu1(Bocu1Rx *pRx, uint8_t b);

/* icuhtml/design/conversion/bocu1/bocu1.c ---------------------------------- */

/* BOCU-1 implementation functions ------------------------------------------ */

/**
 * Compute the next "previous" value for differencing
 * from the current code point.
 *
 * @param c current code point, 0..0x10ffff
 * @return "previous code point" state value
 */
static U_INLINE int32_t
bocu1Prev(int32_t c) {
    /* compute new prev */
    if(0x3040<=c && c<=0x309f) {
        /* Hiragana is not 128-aligned */
        return 0x3070;
    } else if(0x4e00<=c && c<=0x9fa5) {
        /* CJK Unihan */
        return 0x4e00-BOCU1_REACH_NEG_2;
    } else if(0xac00<=c && c<=0xd7a3) {
        /* Korean Hangul (cast to int32_t to avoid wraparound on 16-bit compilers) */
        return ((int32_t)0xd7a3+(int32_t)0xac00)/2;
    } else {
        /* mostly small scripts */
        return (c&~0x7f)+BOCU1_ASCII_PREV;
    }
}

/**
 * Encode a difference -0x10ffff..0x10ffff in 1..4 bytes
 * and return a packed integer with them.
 *
 * The encoding favors small absolut differences with short encodings
 * to compress runs of same-script characters.
 *
 * @param diff difference value -0x10ffff..0x10ffff
 * @return
 *      0x010000zz for 1-byte sequence zz
 *      0x0200yyzz for 2-byte sequence yy zz
 *      0x03xxyyzz for 3-byte sequence xx yy zz
 *      0xwwxxyyzz for 4-byte sequence ww xx yy zz (ww>0x03)
 */
U_CFUNC int32_t
packDiff(int32_t diff) {
    int32_t result, m, lead, count, shift;

    if(diff>=BOCU1_REACH_NEG_1) {
        /* mostly positive differences, and single-byte negative ones */
        if(diff<=BOCU1_REACH_POS_1) {
            /* single byte */
            return 0x01000000|(BOCU1_MIDDLE+diff);
        } else if(diff<=BOCU1_REACH_POS_2) {
            /* two bytes */
            diff-=BOCU1_REACH_POS_1+1;
            lead=BOCU1_START_POS_2;
            count=1;
        } else if(diff<=BOCU1_REACH_POS_3) {
            /* three bytes */
            diff-=BOCU1_REACH_POS_2+1;
            lead=BOCU1_START_POS_3;
            count=2;
        } else {
            /* four bytes */
            diff-=BOCU1_REACH_POS_3+1;
            lead=BOCU1_START_POS_4;
            count=3;
        }
    } else {
        /* two- and four-byte negative differences */
        if(diff>=BOCU1_REACH_NEG_2) {
            /* two bytes */
            diff-=BOCU1_REACH_NEG_1;
            lead=BOCU1_START_NEG_2;
            count=1;
        } else if(diff>=BOCU1_REACH_NEG_3) {
            /* three bytes */
            diff-=BOCU1_REACH_NEG_2;
            lead=BOCU1_START_NEG_3;
            count=2;
        } else {
            /* four bytes */
            diff-=BOCU1_REACH_NEG_3;
            lead=BOCU1_START_NEG_4;
            count=3;
        }
    }

    /* encode the length of the packed result */
    if(count<3) {
        result=(count+1)<<24;
    } else /* count==3, MSB used for the lead byte */ {
        result=0;
    }

    /* calculate trail bytes like digits in itoa() */
    shift=0;
    do {
        NEGDIVMOD(diff, BOCU1_TRAIL_COUNT, m);
        result|=BOCU1_TRAIL_TO_BYTE(m)<<shift;
        shift+=8;
    } while(--count>0);

    /* add lead byte */
    result|=(lead+diff)<<shift;

    return result;
}

/**
 * BOCU-1 encoder function.
 *
 * @param pPrev pointer to the integer that holds
 *        the "previous code point" state;
 *        the initial value should be 0 which
 *        encodeBocu1 will set to the actual BOCU-1 initial state value
 * @param c the code point to encode
 * @return the packed 1/2/3/4-byte encoding, see packDiff(),
 *         or 0 if an error occurs
 *
 * @see packDiff
 */
U_CFUNC int32_t
encodeBocu1(int32_t *pPrev, int32_t c) {
    int32_t prev;

    if(pPrev==NULL || c<0 || c>0x10ffff) {
        /* illegal argument */
        return 0;
    }

    prev=*pPrev;
    if(prev==0) {
        /* lenient handling of initial value 0 */
        prev=*pPrev=BOCU1_ASCII_PREV;
    }

    if(c<=0x20) {
        /*
         * ISO C0 control & space:
         * Encode directly for MIME compatibility,
         * and reset state except for space, to not disrupt compression.
         */
        if(c!=0x20) {
            *pPrev=BOCU1_ASCII_PREV;
        }
        return 0x01000000|c;
    }

    /*
     * all other Unicode code points c==U+0021..U+10ffff
     * are encoded with the difference c-prev
     *
     * a new prev is computed from c,
     * placed in the middle of a 0x80-block (for most small scripts) or
     * in the middle of the Unihan and Hangul blocks
     * to statistically minimize the following difference
     */
    *pPrev=bocu1Prev(c);
    return packDiff(c-prev);
}

/**
 * Function for BOCU-1 decoder; handles multi-byte lead bytes.
 *
 * @param pRx pointer to the decoder state structure
 * @param b lead byte;
 *          BOCU1_MIN<=b<BOCU1_START_NEG_2 or BOCU1_START_POS_2<=b<=BOCU1_MAX_LEAD
 * @return -1 (state change only)
 *
 * @see decodeBocu1
 */
static int32_t
decodeBocu1LeadByte(Bocu1Rx *pRx, uint8_t b) {
    int32_t c, count;

    if(b>=BOCU1_START_NEG_2) {
        /* positive difference */
        if(b<BOCU1_START_POS_3) {
            /* two bytes */
            c=((int32_t)b-BOCU1_START_POS_2)*BOCU1_TRAIL_COUNT+BOCU1_REACH_POS_1+1;
            count=1;
        } else if(b<BOCU1_START_POS_4) {
            /* three bytes */
            c=((int32_t)b-BOCU1_START_POS_3)*BOCU1_TRAIL_COUNT*BOCU1_TRAIL_COUNT+BOCU1_REACH_POS_2+1;
            count=2;
        } else {
            /* four bytes */
            c=BOCU1_REACH_POS_3+1;
            count=3;
        }
    } else {
        /* negative difference */
        if(b>=BOCU1_START_NEG_3) {
            /* two bytes */
            c=((int32_t)b-BOCU1_START_NEG_2)*BOCU1_TRAIL_COUNT+BOCU1_REACH_NEG_1;
            count=1;
        } else if(b>BOCU1_MIN) {
            /* three bytes */
            c=((int32_t)b-BOCU1_START_NEG_3)*BOCU1_TRAIL_COUNT*BOCU1_TRAIL_COUNT+BOCU1_REACH_NEG_2;
            count=2;
        } else {
            /* four bytes */
            c=-BOCU1_TRAIL_COUNT*BOCU1_TRAIL_COUNT*BOCU1_TRAIL_COUNT+BOCU1_REACH_NEG_3;
            count=3;
        }
    }

    /* set the state for decoding the trail byte(s) */
    pRx->diff=c;
    pRx->count=count;
    return -1;
}

/**
 * Function for BOCU-1 decoder; handles multi-byte trail bytes.
 *
 * @param pRx pointer to the decoder state structure
 * @param b trail byte
 * @return result value, same as decodeBocu1
 *
 * @see decodeBocu1
 */
static int32_t
decodeBocu1TrailByte(Bocu1Rx *pRx, uint8_t b) {
    int32_t t, c, count;

    if(b<=0x20) {
        /* skip some C0 controls and make the trail byte range contiguous */
        t=bocu1ByteToTrail[b];
        if(t<0) {
            /* illegal trail byte value */
            pRx->prev=BOCU1_ASCII_PREV;
            pRx->count=0;
            return -99;
        }
#if BOCU1_MAX_TRAIL<0xff
    } else if(b>BOCU1_MAX_TRAIL) {
        return -99;
#endif
    } else {
        t=(int32_t)b-BOCU1_TRAIL_BYTE_OFFSET;
    }

    /* add trail byte into difference and decrement count */
    c=pRx->diff;
    count=pRx->count;

    if(count==1) {
        /* final trail byte, deliver a code point */
        c=pRx->prev+c+t;
        if(0<=c && c<=0x10ffff) {
            /* valid code point result */
            pRx->prev=bocu1Prev(c);
            pRx->count=0;
            return c;
        } else {
            /* illegal code point result */
            pRx->prev=BOCU1_ASCII_PREV;
            pRx->count=0;
            return -99;
        }
    }

    /* intermediate trail byte */
    if(count==2) {
        pRx->diff=c+t*BOCU1_TRAIL_COUNT;
    } else /* count==3 */ {
        pRx->diff=c+t*BOCU1_TRAIL_COUNT*BOCU1_TRAIL_COUNT;
    }
    pRx->count=count-1;
    return -1;
}

/**
 * BOCU-1 decoder function.
 *
 * @param pRx pointer to the decoder state structure;
 *        the initial values should be 0 which
 *        decodeBocu1 will set to actual initial state values
 * @param b an input byte
 * @return
 *      0..0x10ffff for a result code point
 *      -1 if only the state changed without code point output
 *     <-1 if an error occurs
 */
U_CFUNC int32_t
decodeBocu1(Bocu1Rx *pRx, uint8_t b) {
    int32_t prev, c, count;

    if(pRx==NULL) {
        /* illegal argument */
        return -99;
    }

    prev=pRx->prev;
    if(prev==0) {
        /* lenient handling of initial 0 values */
        prev=pRx->prev=BOCU1_ASCII_PREV;
        count=pRx->count=0;
    } else {
        count=pRx->count;
    }

    if(count==0) {
        /* byte in lead position */
        if(b<=0x20) {
            /*
             * Direct-encoded C0 control code or space.
             * Reset prev for C0 control codes but not for space.
             */
            if(b!=0x20) {
                pRx->prev=BOCU1_ASCII_PREV;
            }
            return b;
        }

        /*
         * b is a difference lead byte.
         *
         * Return a code point directly from a single-byte difference.
         *
         * For multi-byte difference lead bytes, set the decoder state
         * with the partial difference value from the lead byte and
         * with the number of trail bytes.
         *
         * For four-byte differences, the signedness also affects the
         * first trail byte, which has special handling farther below.
         */
        if(b>=BOCU1_START_NEG_2 && b<BOCU1_START_POS_2) {
            /* single-byte difference */
            c=prev+((int32_t)b-BOCU1_MIDDLE);
            pRx->prev=bocu1Prev(c);
            return c;
        } else if(b==BOCU1_RESET) {
            /* only reset the state, no code point */
            pRx->prev=BOCU1_ASCII_PREV;
            return -1;
        } else {
            return decodeBocu1LeadByte(pRx, b);
        }
    } else {
        /* trail byte in any position */
        return decodeBocu1TrailByte(pRx, b);
    }
}

/* icuhtml/design/conversion/bocu1/bocu1tst.c ------------------------------- */

/* test code ---------------------------------------------------------------- */

/* test code options */

/* ignore comma when processing name lists in testText() */
#define TEST_IGNORE_COMMA       1

/**
 * Write a packed BOCU-1 byte sequence into a byte array,
 * without overflow check.
 * Test function.
 *
 * @param packed packed BOCU-1 byte sequence, see packDiff()
 * @param p pointer to byte array
 * @return number of bytes
 *
 * @see packDiff
 */
static int32_t
writePacked(int32_t packed, uint8_t *p) {
    int32_t count=BOCU1_LENGTH_FROM_PACKED(packed);
    switch(count) {
    case 4:
        *p++=(uint8_t)(packed>>24);
    case 3:
        *p++=(uint8_t)(packed>>16);
    case 2:
        *p++=(uint8_t)(packed>>8);
    case 1:
        *p++=(uint8_t)packed;
    default:
        break;
    }

    return count;
}

/**
 * Unpack a packed BOCU-1 non-C0/space byte sequence and get
 * the difference to initialPrev.
 * Used only for round-trip testing of the difference encoding and decoding.
 * Test function.
 *
 * @param initialPrev bogus "previous code point" value to make sure that
 *                    the resulting code point is in the range 0..0x10ffff
 * @param packed packed BOCU-1 byte sequence
 * @return the difference to initialPrev
 *
 * @see packDiff
 * @see writeDiff
 */
static int32_t
unpackDiff(int32_t initialPrev, int32_t packed) {
    Bocu1Rx rx={ 0, 0, 0 };
    int32_t count;

    rx.prev=initialPrev;
    count=BOCU1_LENGTH_FROM_PACKED(packed);
    switch(count) {
    case 4:
        decodeBocu1(&rx, (uint8_t)(packed>>24));
    case 3:
        decodeBocu1(&rx, (uint8_t)(packed>>16));
    case 2:
        decodeBocu1(&rx, (uint8_t)(packed>>8));
    case 1:
        /* subtract initial prev */
        return decodeBocu1(&rx, (uint8_t)packed)-initialPrev;
    default:
        return -0x7fffffff;
    }
}

/**
 * Encode one difference value -0x10ffff..+0x10ffff in 1..4 bytes,
 * preserving lexical order.
 * Also checks for roundtripping of the difference encoding.
 * Test function.
 *
 * @param diff difference value to test, -0x10ffff..0x10ffff
 * @param p pointer to output byte array
 * @return p advanced by number of bytes output
 *
 * @see unpackDiff
 */
static uint8_t *
writeDiff(int32_t diff, uint8_t *p) {
    /* generate the difference as a packed value and serialize it */
    int32_t packed, initialPrev;

    packed=packDiff(diff);

    /*
     * bogus initial "prev" to work around
     * code point range check in decodeBocu1()
     */
    if(diff<=0) {
        initialPrev=0x10ffff;
    } else {
        initialPrev=-1;
    }

    if(diff!=unpackDiff(initialPrev, packed)) {
        log_err("error: unpackDiff(packDiff(diff=%ld)=0x%08lx)=%ld!=diff\n",
                diff, packed, unpackDiff(initialPrev, packed));
    }
    return p+writePacked(packed, p);
}

/**
 * Encode a UTF-16 string in BOCU-1.
 * Does not check for overflows, but otherwise useful function.
 *
 * @param s input UTF-16 string
 * @param length number of UChar code units in s
 * @param p pointer to output byte array
 * @return number of bytes output
 */
static int32_t
writeString(const UChar *s, int32_t length, uint8_t *p) {
    uint8_t *p0;
    int32_t c, prev, i;

    prev=0;
    p0=p;
    i=0;
    while(i<length) {
        UTF_NEXT_CHAR(s, i, length, c);
        p+=writePacked(encodeBocu1(&prev, c), p);
    }
    return (int32_t)(p-p0);
}

/**
 * Decode a BOCU-1 byte sequence to a UTF-16 string.
 * Does not check for overflows, but otherwise useful function.
 *
 * @param p pointer to input BOCU-1 bytes
 * @param length number of input bytes
 * @param s point to output UTF-16 string array
 * @return number of UChar code units output
 */
static int32_t
readString(const uint8_t *p, int32_t length, UChar *s) {
    Bocu1Rx rx={ 0, 0, 0 };
    int32_t c, i, sLength;

    i=sLength=0;
    while(i<length) {
        c=decodeBocu1(&rx, p[i++]);
        if(c<-1) {
            log_err("error: readString detects encoding error at string index %ld\n", i);
            return -1;
        }
        if(c>=0) {
            UTF_APPEND_CHAR_UNSAFE(s, sLength, c);
        }
    }
    return sLength;
}

static U_INLINE char
hexDigit(uint8_t digit) {
    return digit<=9 ? (char)('0'+digit) : (char)('a'-10+digit);
}

/**
 * Pretty-print 0-terminated byte values.
 * Helper function for test output.
 *
 * @param bytes 0-terminated byte array to print
 */
static void
printBytes(uint8_t *bytes, char *out) {
    int i;
    uint8_t b;

    i=0;
    while((b=*bytes++)!=0) {
        *out++=' ';
        *out++=hexDigit((uint8_t)(b>>4));
        *out++=hexDigit((uint8_t)(b&0xf));
        ++i;
    }
    i=3*(5-i);
    while(i>0) {
        *out++=' ';
        --i;
    }
    *out=0;
}

/**
 * Basic BOCU-1 test function, called when there are no command line arguments.
 * Prints some of the #define values and performs round-trip tests of the
 * difference encoding and decoding.
 */
static void
TestBOCU1RefDiff(void) {
    char buf1[80], buf2[80];
    uint8_t prev[5], level[5];
    int32_t i, cmp, countErrors;

    log_verbose("reach of single bytes: %ld\n", 1+BOCU1_REACH_POS_1-BOCU1_REACH_NEG_1);
    log_verbose("reach of 2 bytes     : %ld\n", 1+BOCU1_REACH_POS_2-BOCU1_REACH_NEG_2);
    log_verbose("reach of 3 bytes     : %ld\n\n", 1+BOCU1_REACH_POS_3-BOCU1_REACH_NEG_3);

    log_verbose("    BOCU1_REACH_NEG_1 %8ld    BOCU1_REACH_POS_1 %8ld\n", BOCU1_REACH_NEG_1, BOCU1_REACH_POS_1);
    log_verbose("    BOCU1_REACH_NEG_2 %8ld    BOCU1_REACH_POS_2 %8ld\n", BOCU1_REACH_NEG_2, BOCU1_REACH_POS_2);
    log_verbose("    BOCU1_REACH_NEG_3 %8ld    BOCU1_REACH_POS_3 %8ld\n\n", BOCU1_REACH_NEG_3, BOCU1_REACH_POS_3);

    log_verbose("    BOCU1_MIDDLE      0x%02x\n", BOCU1_MIDDLE);
    log_verbose("    BOCU1_START_NEG_2 0x%02x    BOCU1_START_POS_2 0x%02x\n", BOCU1_START_NEG_2, BOCU1_START_POS_2);
    log_verbose("    BOCU1_START_NEG_3 0x%02x    BOCU1_START_POS_3 0x%02x\n\n", BOCU1_START_NEG_3, BOCU1_START_POS_3);

    /* test packDiff() & unpackDiff() with some specific values */
    writeDiff(0, level);
    writeDiff(1, level);
    writeDiff(65, level);
    writeDiff(130, level);
    writeDiff(30000, level);
    writeDiff(1000000, level);
    writeDiff(-65, level);
    writeDiff(-130, level);
    writeDiff(-30000, level);
    writeDiff(-1000000, level);

    /* test that each value is smaller than any following one */
    countErrors=0;
    i=-0x10ffff;
    *writeDiff(i, prev)=0;

    /* show first number and bytes */
    printBytes(prev, buf1);
    log_verbose("              wD(%8ld)                    %s\n", i, buf1);

    for(++i; i<=0x10ffff; ++i) {
        *writeDiff(i, level)=0;
        cmp=strcmp((const char *)prev, (const char *)level);
        if(BOCU1_LENGTH_FROM_LEAD(level[0])!=(int32_t)strlen((const char *)level)) {
            log_verbose("BOCU1_LENGTH_FROM_LEAD(0x%02x)=%ld!=%ld=strlen(writeDiff(%ld))\n",
                   level[0], BOCU1_LENGTH_FROM_LEAD(level[0]), strlen((const char *)level), i);
        }
        if(cmp<0) {
            if(i==0 || i==1 || strlen((const char *)prev)!=strlen((const char *)level)) {
                /*
                 * if the result is good, then print only if the length changed
                 * to get little but interesting output
                 */
                printBytes(prev, buf1);
                printBytes(level, buf2);
                log_verbose("ok:    strcmp(wD(%8ld), wD(%8ld))=%2d  %s%s\n", i-1, i, cmp, buf1, buf2);
            }
        } else {
            ++countErrors;
            printBytes(prev, buf1);
            printBytes(level, buf2);
            log_verbose("wrong: strcmp(wD(%8ld), wD(%8ld))=%2d  %s%s\n", i-1, i, cmp, buf1, buf2);
        }
        /* remember the previous bytes */
        memcpy(prev, level, 4);
    }

    /* show last number and bytes */
    printBytes((uint8_t *)"", buf1);
    printBytes(prev, buf2);
    log_verbose("                            wD(%8ld)      %s%s\n", i-1, buf1, buf2);

    if(countErrors==0) {
        log_verbose("writeDiff(-0x10ffff..0x10ffff) works fine\n");
    } else {
        log_err("writeDiff(-0x10ffff..0x10ffff) violates lexical ordering in %d cases\n", countErrors);
    }

    /* output signature byte sequence */
    i=0;
    writePacked(encodeBocu1(&i, 0xfeff), level);
    log_verbose("\nBOCU-1 signature byte sequence: %02x %02x %02x\n",
            level[0], level[1], level[2]);
}

/* cintltst code ------------------------------------------------------------ */

static const int32_t DEFAULT_BUFFER_SIZE = 30000;


/* test one string with the ICU and the reference BOCU-1 implementations */
static void
roundtripBOCU1(UConverter *bocu1, int32_t number, const UChar *text, int32_t length) {
    UChar *roundtripRef, *roundtripICU;
    char *bocu1Ref, *bocu1ICU;

    int32_t bocu1RefLength, bocu1ICULength, roundtripRefLength, roundtripICULength;
    UErrorCode errorCode;

    roundtripRef = malloc(DEFAULT_BUFFER_SIZE * sizeof(UChar));
    roundtripICU = malloc(DEFAULT_BUFFER_SIZE * sizeof(UChar));
    bocu1Ref = malloc(DEFAULT_BUFFER_SIZE);
    bocu1ICU = malloc(DEFAULT_BUFFER_SIZE);

    /* Unicode -> BOCU-1 */
    bocu1RefLength=writeString(text, length, (uint8_t *)bocu1Ref);

    errorCode=U_ZERO_ERROR;
    bocu1ICULength=ucnv_fromUChars(bocu1, bocu1ICU, DEFAULT_BUFFER_SIZE, text, length, &errorCode);
    if(U_FAILURE(errorCode)) {
        log_err("ucnv_fromUChars(BOCU-1, text(%d)[%d]) failed: %s\n", number, length, u_errorName(errorCode));
        return;
    }

    if(bocu1RefLength!=bocu1ICULength || 0!=uprv_memcmp(bocu1Ref, bocu1ICU, bocu1RefLength)) {
        log_err("Unicode(%d)[%d] -> BOCU-1: reference[%d]!=ICU[%d]\n", number, length, bocu1RefLength, bocu1ICULength);
        return;
    }

    /* BOCU-1 -> Unicode */
    roundtripRefLength=readString((uint8_t *)bocu1Ref, bocu1RefLength, roundtripRef);
    if(roundtripRefLength<0) {
        free(roundtripICU);
        return; /* readString() found an error and reported it */
    }

    roundtripICULength=ucnv_toUChars(bocu1, roundtripICU, DEFAULT_BUFFER_SIZE, bocu1ICU, bocu1ICULength, &errorCode);
    if(U_FAILURE(errorCode)) {
        log_err("ucnv_toUChars(BOCU-1, text(%d)[%d]) failed: %s\n", number, length, u_errorName(errorCode));
        return;
    }

    if(length!=roundtripRefLength || 0!=u_memcmp(text, roundtripRef, length)) {
        log_err("BOCU-1 -> Unicode: original(%d)[%d]!=reference[%d]\n", number, length, roundtripRefLength);
        return;
    }
    if(roundtripRefLength!=roundtripICULength || 0!=u_memcmp(roundtripRef, roundtripICU, roundtripRefLength)) {
        log_err("BOCU-1 -> Unicode: reference(%d)[%d]!=ICU[%d]\n", number, roundtripRefLength, roundtripICULength);
        return;
    }
    free(roundtripRef);
    free(roundtripICU);
    free(bocu1Ref);
    free(bocu1ICU);
}

static const UChar feff[]={ 0xfeff };
static const UChar ascii[]={ 0x61, 0x62, 0x20, 0x63, 0x61 };
static const UChar crlf[]={ 0xd, 0xa, 0x20 };
static const UChar nul[]={ 0 };
static const UChar latin[]={ 0xdf, 0xe6 };
static const UChar devanagari[]={ 0x930, 0x20, 0x918, 0x909 };
static const UChar hiragana[]={ 0x3086, 0x304d, 0x20, 0x3053, 0x4000 };
static const UChar unihan[]={ 0x4e00, 0x7777, 0x20, 0x9fa5, 0x4e00 };
static const UChar hangul[]={ 0xac00, 0xbcde, 0x20, 0xd7a3 };
static const UChar surrogates[]={ 0xdc00, 0xd800 }; /* single surrogates, unmatched! */
static const UChar plane1[]={ 0xd800, 0xdc00 };
static const UChar plane2[]={ 0xd845, 0xdddd };
static const UChar plane15[]={ 0xdbbb, 0xddee, 0x20 };
static const UChar plane16[]={ 0xdbff, 0xdfff };
static const UChar c0[]={ 1, 0xe40, 0x20, 9 };

static const struct {
    const UChar *s;
    int32_t length;
} strings[]={
    { feff,         LENGTHOF(feff) },
    { ascii,        LENGTHOF(ascii) },
    { crlf,         LENGTHOF(crlf) },
    { nul,          LENGTHOF(nul) },
    { latin,        LENGTHOF(latin) },
    { devanagari,   LENGTHOF(devanagari) },
    { hiragana,     LENGTHOF(hiragana) },
    { unihan,       LENGTHOF(unihan) },
    { hangul,       LENGTHOF(hangul) },
    { surrogates,   LENGTHOF(surrogates) },
    { plane1,       LENGTHOF(plane1) },
    { plane2,       LENGTHOF(plane2) },
    { plane15,      LENGTHOF(plane15) },
    { plane16,      LENGTHOF(plane16) },
    { c0,           LENGTHOF(c0) }
};

/*
 * Verify that the ICU BOCU-1 implementation produces the same results as
 * the reference implementation from the design folder.
 * Generate some texts and convert them with both converters, verifying
 * identical results and roundtripping.
 */
static void
TestBOCU1(void) {
    UChar *text;
    int32_t i, length;

    UConverter *bocu1;
    UErrorCode errorCode;

    errorCode=U_ZERO_ERROR;
    bocu1=ucnv_open("BOCU-1", &errorCode);
    if(U_FAILURE(errorCode)) {
        log_err("error: unable to open BOCU-1 converter: %s\n", u_errorName(errorCode));
        return;
    }

    text = malloc(DEFAULT_BUFFER_SIZE * sizeof(UChar));

    /* text 1: each of strings[] once */
    length=0;
    for(i=0; i<LENGTHOF(strings); ++i) {
        u_memcpy(text+length, strings[i].s, strings[i].length);
        length+=strings[i].length;
    }
    roundtripBOCU1(bocu1, 1, text, length);

    /* text 2: each of strings[] twice */
    length=0;
    for(i=0; i<LENGTHOF(strings); ++i) {
        u_memcpy(text+length, strings[i].s, strings[i].length);
        length+=strings[i].length;
        u_memcpy(text+length, strings[i].s, strings[i].length);
        length+=strings[i].length;
    }
    roundtripBOCU1(bocu1, 2, text, length);

    /* text 3: each of strings[] many times (set step vs. |strings| so that all strings are used) */
    length=0;
    for(i=1; length<5000; i+=7) {
        if(i>=LENGTHOF(strings)) {
            i-=LENGTHOF(strings);
        }
        u_memcpy(text+length, strings[i].s, strings[i].length);
        length+=strings[i].length;
    }
    roundtripBOCU1(bocu1, 3, text, length);

    ucnv_close(bocu1);
    free(text);
}

U_CFUNC void addBOCU1Tests(TestNode** root);

U_CFUNC void
addBOCU1Tests(TestNode** root) {
    addTest(root, TestBOCU1RefDiff, "tsconv/bocu1tst/TestBOCU1RefDiff");
    addTest(root, TestBOCU1, "tsconv/bocu1tst/TestBOCU1");
}
