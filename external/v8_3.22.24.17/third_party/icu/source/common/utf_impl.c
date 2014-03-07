/*
******************************************************************************
*
*   Copyright (C) 1999-2006, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
******************************************************************************
*   file name:  utf_impl.c
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 1999sep13
*   created by: Markus W. Scherer
*
*   This file provides implementation functions for macros in the utfXX.h
*   that would otherwise be too long as macros.
*/

/* set import/export definitions */
#ifndef U_UTF8_IMPL
#   define U_UTF8_IMPL
#endif

#include "unicode/utypes.h"

/*
 * This table could be replaced on many machines by
 * a few lines of assembler code using an
 * "index of first 0-bit from msb" instruction and
 * one or two more integer instructions.
 *
 * For example, on an i386, do something like
 * - MOV AL, leadByte
 * - NOT AL         (8-bit, leave b15..b8==0..0, reverse only b7..b0)
 * - MOV AH, 0
 * - BSR BX, AX     (16-bit)
 * - MOV AX, 6      (result)
 * - JZ finish      (ZF==1 if leadByte==0xff)
 * - SUB AX, BX (result)
 * -finish:
 * (BSR: Bit Scan Reverse, scans for a 1-bit, starting from the MSB)
 *
 * In Unicode, all UTF-8 byte sequences with more than 4 bytes are illegal;
 * lead bytes above 0xf4 are illegal.
 * We keep them in this table for skipping long ISO 10646-UTF-8 sequences.
 */
U_EXPORT const uint8_t 
utf8_countTrailBytes[256]={
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,

    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    3, 3, 3, 3, 3,
    3, 3, 3,    /* illegal in Unicode */
    4, 4, 4, 4, /* illegal in Unicode */
    5, 5,       /* illegal in Unicode */
    0, 0        /* illegal bytes 0xfe and 0xff */
};

static const UChar32
utf8_minLegal[4]={ 0, 0x80, 0x800, 0x10000 };

static const UChar32
utf8_errorValue[6]={
    UTF8_ERROR_VALUE_1, UTF8_ERROR_VALUE_2, UTF_ERROR_VALUE, 0x10ffff,
    0x3ffffff, 0x7fffffff
};

/*
 * Handle the non-inline part of the U8_NEXT() macro and its obsolete sibling
 * UTF8_NEXT_CHAR_SAFE().
 *
 * The "strict" parameter controls the error behavior:
 * <0  "Safe" behavior of U8_NEXT(): All illegal byte sequences yield a negative
 *     code point result.
 *  0  Obsolete "safe" behavior of UTF8_NEXT_CHAR_SAFE(..., FALSE):
 *     All illegal byte sequences yield a positive code point such that this
 *     result code point would be encoded with the same number of bytes as
 *     the illegal sequence.
 * >0  Obsolete "strict" behavior of UTF8_NEXT_CHAR_SAFE(..., TRUE):
 *     Same as the obsolete "safe" behavior, but non-characters are also treated
 *     like illegal sequences.
 *
 * The special negative (<0) value -2 is used for lenient treatment of surrogate
 * code points as legal. Some implementations use this for roundtripping of
 * Unicode 16-bit strings that are not well-formed UTF-16, that is, they
 * contain unpaired surrogates.
 *
 * Note that a UBool is the same as an int8_t.
 */
U_CAPI UChar32 U_EXPORT2
utf8_nextCharSafeBody(const uint8_t *s, int32_t *pi, int32_t length, UChar32 c, UBool strict) {
    int32_t i=*pi;
    uint8_t count=UTF8_COUNT_TRAIL_BYTES(c);
    if((i)+count<=(length)) {
        uint8_t trail, illegal=0;

        UTF8_MASK_LEAD_BYTE((c), count);
        /* count==0 for illegally leading trail bytes and the illegal bytes 0xfe and 0xff */
        switch(count) {
        /* each branch falls through to the next one */
        case 5:
        case 4:
            /* count>=4 is always illegal: no more than 3 trail bytes in Unicode's UTF-8 */
            illegal=1;
            break;
        case 3:
            trail=s[(i)++];
            (c)=((c)<<6)|(trail&0x3f);
            if(c<0x110) {
                illegal|=(trail&0xc0)^0x80;
            } else {
                /* code point>0x10ffff, outside Unicode */
                illegal=1;
                break;
            }
        case 2:
            trail=s[(i)++];
            (c)=((c)<<6)|(trail&0x3f);
            illegal|=(trail&0xc0)^0x80;
        case 1:
            trail=s[(i)++];
            (c)=((c)<<6)|(trail&0x3f);
            illegal|=(trail&0xc0)^0x80;
            break;
        case 0:
            if(strict>=0) {
                return UTF8_ERROR_VALUE_1;
            } else {
                return U_SENTINEL;
            }
        /* no default branch to optimize switch()  - all values are covered */
        }

        /*
         * All the error handling should return a value
         * that needs count bytes so that UTF8_GET_CHAR_SAFE() works right.
         *
         * Starting with Unicode 3.0.1, non-shortest forms are illegal.
         * Starting with Unicode 3.2, surrogate code points must not be
         * encoded in UTF-8, and there are no irregular sequences any more.
         *
         * U8_ macros (new in ICU 2.4) return negative values for error conditions.
         */

        /* correct sequence - all trail bytes have (b7..b6)==(10)? */
        /* illegal is also set if count>=4 */
        if(illegal || (c)<utf8_minLegal[count] || (UTF_IS_SURROGATE(c) && strict!=-2)) {
            /* error handling */
            uint8_t errorCount=count;
            /* don't go beyond this sequence */
            i=*pi;
            while(count>0 && UTF8_IS_TRAIL(s[i])) {
                ++(i);
                --count;
            }
            if(strict>=0) {
                c=utf8_errorValue[errorCount-count];
            } else {
                c=U_SENTINEL;
            }
        } else if((strict)>0 && UTF_IS_UNICODE_NONCHAR(c)) {
            /* strict: forbid non-characters like U+fffe */
            c=utf8_errorValue[count];
        }
    } else /* too few bytes left */ {
        /* error handling */
        int32_t i0=i;
        /* don't just set (i)=(length) in case there is an illegal sequence */
        while((i)<(length) && UTF8_IS_TRAIL(s[i])) {
            ++(i);
        }
        if(strict>=0) {
            c=utf8_errorValue[i-i0];
        } else {
            c=U_SENTINEL;
        }
    }
    *pi=i;
    return c;
}

U_CAPI int32_t U_EXPORT2
utf8_appendCharSafeBody(uint8_t *s, int32_t i, int32_t length, UChar32 c, UBool *pIsError) {
    if((uint32_t)(c)<=0x7ff) {
        if((i)+1<(length)) {
            (s)[(i)++]=(uint8_t)(((c)>>6)|0xc0);
            (s)[(i)++]=(uint8_t)(((c)&0x3f)|0x80);
            return i;
        }
    } else if((uint32_t)(c)<=0xffff) {
        /* Starting with Unicode 3.2, surrogate code points must not be encoded in UTF-8. */
        if((i)+2<(length) && !U_IS_SURROGATE(c)) {
            (s)[(i)++]=(uint8_t)(((c)>>12)|0xe0);
            (s)[(i)++]=(uint8_t)((((c)>>6)&0x3f)|0x80);
            (s)[(i)++]=(uint8_t)(((c)&0x3f)|0x80);
            return i;
        }
    } else if((uint32_t)(c)<=0x10ffff) {
        if((i)+3<(length)) {
            (s)[(i)++]=(uint8_t)(((c)>>18)|0xf0);
            (s)[(i)++]=(uint8_t)((((c)>>12)&0x3f)|0x80);
            (s)[(i)++]=(uint8_t)((((c)>>6)&0x3f)|0x80);
            (s)[(i)++]=(uint8_t)(((c)&0x3f)|0x80);
            return i;
        }
    }
    /* c>0x10ffff or not enough space, write an error value */
    if(pIsError!=NULL) {
        *pIsError=TRUE;
    } else {
        length-=i;
        if(length>0) {
            int32_t offset;
            if(length>3) {
                length=3;
            }
            s+=i;
            offset=0;
            c=utf8_errorValue[length-1];
            UTF8_APPEND_CHAR_UNSAFE(s, offset, c);
            i=i+offset;
        }
    }
    return i;
}

U_CAPI UChar32 U_EXPORT2
utf8_prevCharSafeBody(const uint8_t *s, int32_t start, int32_t *pi, UChar32 c, UBool strict) {
    int32_t i=*pi;
    uint8_t b, count=1, shift=6;

    /* extract value bits from the last trail byte */
    c&=0x3f;

    for(;;) {
        if(i<=start) {
            /* no lead byte at all */
            if(strict>=0) {
                return UTF8_ERROR_VALUE_1;
            } else {
                return U_SENTINEL;
            }
            /*break;*/
        }

        /* read another previous byte */
        b=s[--i];
        if((uint8_t)(b-0x80)<0x7e) { /* 0x80<=b<0xfe */
            if(b&0x40) {
                /* lead byte, this will always end the loop */
                uint8_t shouldCount=UTF8_COUNT_TRAIL_BYTES(b);

                if(count==shouldCount) {
                    /* set the new position */
                    *pi=i;
                    UTF8_MASK_LEAD_BYTE(b, count);
                    c|=(UChar32)b<<shift;
                    if(count>=4 || c>0x10ffff || c<utf8_minLegal[count] || (UTF_IS_SURROGATE(c) && strict!=-2) || (strict>0 && UTF_IS_UNICODE_NONCHAR(c))) {
                        /* illegal sequence or (strict and non-character) */
                        if(count>=4) {
                            count=3;
                        }
                        if(strict>=0) {
                            c=utf8_errorValue[count];
                        } else {
                            c=U_SENTINEL;
                        }
                    } else {
                        /* exit with correct c */
                    }
                } else {
                    /* the lead byte does not match the number of trail bytes */
                    /* only set the position to the lead byte if it would
                       include the trail byte that we started with */
                    if(count<shouldCount) {
                        *pi=i;
                        if(strict>=0) {
                            c=utf8_errorValue[count];
                        } else {
                            c=U_SENTINEL;
                        }
                    } else {
                        if(strict>=0) {
                            c=UTF8_ERROR_VALUE_1;
                        } else {
                            c=U_SENTINEL;
                        }
                    }
                }
                break;
            } else if(count<5) {
                /* trail byte */
                c|=(UChar32)(b&0x3f)<<shift;
                ++count;
                shift+=6;
            } else {
                /* more than 5 trail bytes is illegal */
                if(strict>=0) {
                    c=UTF8_ERROR_VALUE_1;
                } else {
                    c=U_SENTINEL;
                }
                break;
            }
        } else {
            /* single-byte character precedes trailing bytes */
            if(strict>=0) {
                c=UTF8_ERROR_VALUE_1;
            } else {
                c=U_SENTINEL;
            }
            break;
        }
    }
    return c;
}

U_CAPI int32_t U_EXPORT2
utf8_back1SafeBody(const uint8_t *s, int32_t start, int32_t i) {
    /* i had been decremented once before the function call */
    int32_t I=i, Z;
    uint8_t b;

    /* read at most the 6 bytes s[Z] to s[i], inclusively */
    if(I-5>start) {
        Z=I-5;
    } else {
        Z=start;
    }

    /* return I if the sequence starting there is long enough to include i */
    do {
        b=s[I];
        if((uint8_t)(b-0x80)>=0x7e) { /* not 0x80<=b<0xfe */
            break;
        } else if(b>=0xc0) {
            if(UTF8_COUNT_TRAIL_BYTES(b)>=(i-I)) {
                return I;
            } else {
                break;
            }
        }
    } while(Z<=--I);

    /* return i itself to be consistent with the FWD_1 macro */
    return i;
}
