/*
*******************************************************************************
*
*   Copyright (C) 2002-2010, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  punycode.c
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2002jan31
*   created by: Markus W. Scherer
*/


/* This ICU code derived from: */
/*
punycode.c 0.4.0 (2001-Nov-17-Sat)
http://www.cs.berkeley.edu/~amc/idn/
Adam M. Costello
http://www.nicemice.net/amc/

Disclaimer and license

    Regarding this entire document or any portion of it (including
    the pseudocode and C code), the author makes no guarantees and
    is not responsible for any damage resulting from its use.  The
    author grants irrevocable permission to anyone to use, modify,
    and distribute it in any way that does not diminish the rights
    of anyone else to use, modify, and distribute it, provided that
    redistributed derivative works do not contain misleading author or
    version information.  Derivative works need not be licensed under
    similar terms.
*/
/*
 * ICU modifications:
 * - ICU data types and coding conventions
 * - ICU string buffer handling with implicit source lengths
 *   and destination preflighting
 * - UTF-16 handling
 */

#include "unicode/utypes.h"

#if !UCONFIG_NO_IDNA

#include "ustr_imp.h"
#include "cstring.h"
#include "cmemory.h"
#include "punycode.h"
#include "unicode/ustring.h"


/* Punycode ----------------------------------------------------------------- */

/* Punycode parameters for Bootstring */
#define BASE            36
#define TMIN            1
#define TMAX            26
#define SKEW            38
#define DAMP            700
#define INITIAL_BIAS    72
#define INITIAL_N       0x80

/* "Basic" Unicode/ASCII code points */
#define _HYPHEN         0X2d
#define DELIMITER       _HYPHEN

#define _ZERO_          0X30
#define _NINE           0x39

#define _SMALL_A        0X61
#define _SMALL_Z        0X7a

#define _CAPITAL_A      0X41
#define _CAPITAL_Z      0X5a

#define IS_BASIC(c) ((c)<0x80)
#define IS_BASIC_UPPERCASE(c) (_CAPITAL_A<=(c) && (c)<=_CAPITAL_Z)

/**
 * digitToBasic() returns the basic code point whose value
 * (when used for representing integers) is d, which must be in the
 * range 0 to BASE-1. The lowercase form is used unless the uppercase flag is
 * nonzero, in which case the uppercase form is used.
 */
static U_INLINE char
digitToBasic(int32_t digit, UBool uppercase) {
    /*  0..25 map to ASCII a..z or A..Z */
    /* 26..35 map to ASCII 0..9         */
    if(digit<26) {
        if(uppercase) {
            return (char)(_CAPITAL_A+digit);
        } else {
            return (char)(_SMALL_A+digit);
        }
    } else {
        return (char)((_ZERO_-26)+digit);
    }
}

/**
 * basicToDigit[] contains the numeric value of a basic code
 * point (for use in representing integers) in the range 0 to
 * BASE-1, or -1 if b is does not represent a value.
 */
static const int8_t
basicToDigit[256]={
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,

    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    26, 27, 28, 29, 30, 31, 32, 33, 34, 35, -1, -1, -1, -1, -1, -1,

    -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,

    -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,

    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,

    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,

    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,

    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};

static U_INLINE char
asciiCaseMap(char b, UBool uppercase) {
    if(uppercase) {
        if(_SMALL_A<=b && b<=_SMALL_Z) {
            b-=(_SMALL_A-_CAPITAL_A);
        }
    } else {
        if(_CAPITAL_A<=b && b<=_CAPITAL_Z) {
            b+=(_SMALL_A-_CAPITAL_A);
        }
    }
    return b;
}

/* Punycode-specific Bootstring code ---------------------------------------- */

/*
 * The following code omits the {parts} of the pseudo-algorithm in the spec
 * that are not used with the Punycode parameter set.
 */

/* Bias adaptation function. */
static int32_t
adaptBias(int32_t delta, int32_t length, UBool firstTime) {
    int32_t count;

    if(firstTime) {
        delta/=DAMP;
    } else {
        delta/=2;
    }

    delta+=delta/length;
    for(count=0; delta>((BASE-TMIN)*TMAX)/2; count+=BASE) {
        delta/=(BASE-TMIN);
    }

    return count+(((BASE-TMIN+1)*delta)/(delta+SKEW));
}

#define MAX_CP_COUNT    200

U_CFUNC int32_t
u_strToPunycode(const UChar *src, int32_t srcLength,
                UChar *dest, int32_t destCapacity,
                const UBool *caseFlags,
                UErrorCode *pErrorCode) {

    int32_t cpBuffer[MAX_CP_COUNT];
    int32_t n, delta, handledCPCount, basicLength, destLength, bias, j, m, q, k, t, srcCPCount;
    UChar c, c2;

    /* argument checking */
    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return 0;
    }

    if(src==NULL || srcLength<-1 || (dest==NULL && destCapacity!=0)) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    /*
     * Handle the basic code points and
     * convert extended ones to UTF-32 in cpBuffer (caseFlag in sign bit):
     */
    srcCPCount=destLength=0;
    if(srcLength==-1) {
        /* NUL-terminated input */
        for(j=0; /* no condition */; ++j) {
            if((c=src[j])==0) {
                break;
            }
            if(srcCPCount==MAX_CP_COUNT) {
                /* too many input code points */
                *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
                return 0;
            }
            if(IS_BASIC(c)) {
                cpBuffer[srcCPCount++]=0;
                if(destLength<destCapacity) {
                    dest[destLength]=
                        caseFlags!=NULL ?
                            asciiCaseMap((char)c, caseFlags[j]) :
                            (char)c;
                }
                ++destLength;
            } else {
                n=(caseFlags!=NULL && caseFlags[j])<<31L;
                if(UTF_IS_SINGLE(c)) {
                    n|=c;
                } else if(UTF_IS_LEAD(c) && UTF_IS_TRAIL(c2=src[j+1])) {
                    ++j;
                    n|=(int32_t)UTF16_GET_PAIR_VALUE(c, c2);
                } else {
                    /* error: unmatched surrogate */
                    *pErrorCode=U_INVALID_CHAR_FOUND;
                    return 0;
                }
                cpBuffer[srcCPCount++]=n;
            }
        }
    } else {
        /* length-specified input */
        for(j=0; j<srcLength; ++j) {
            if(srcCPCount==MAX_CP_COUNT) {
                /* too many input code points */
                *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
                return 0;
            }
            c=src[j];
            if(IS_BASIC(c)) {
                cpBuffer[srcCPCount++]=0;
                if(destLength<destCapacity) {
                    dest[destLength]=
                        caseFlags!=NULL ?
                            asciiCaseMap((char)c, caseFlags[j]) :
                            (char)c;
                }
                ++destLength;
            } else {
                n=(caseFlags!=NULL && caseFlags[j])<<31L;
                if(UTF_IS_SINGLE(c)) {
                    n|=c;
                } else if(UTF_IS_LEAD(c) && (j+1)<srcLength && UTF_IS_TRAIL(c2=src[j+1])) {
                    ++j;
                    n|=(int32_t)UTF16_GET_PAIR_VALUE(c, c2);
                } else {
                    /* error: unmatched surrogate */
                    *pErrorCode=U_INVALID_CHAR_FOUND;
                    return 0;
                }
                cpBuffer[srcCPCount++]=n;
            }
        }
    }

    /* Finish the basic string - if it is not empty - with a delimiter. */
    basicLength=destLength;
    if(basicLength>0) {
        if(destLength<destCapacity) {
            dest[destLength]=DELIMITER;
        }
        ++destLength;
    }

    /*
     * handledCPCount is the number of code points that have been handled
     * basicLength is the number of basic code points
     * destLength is the number of chars that have been output
     */

    /* Initialize the state: */
    n=INITIAL_N;
    delta=0;
    bias=INITIAL_BIAS;

    /* Main encoding loop: */
    for(handledCPCount=basicLength; handledCPCount<srcCPCount; /* no op */) {
        /*
         * All non-basic code points < n have been handled already.
         * Find the next larger one:
         */
        for(m=0x7fffffff, j=0; j<srcCPCount; ++j) {
            q=cpBuffer[j]&0x7fffffff; /* remove case flag from the sign bit */
            if(n<=q && q<m) {
                m=q;
            }
        }

        /*
         * Increase delta enough to advance the decoder's
         * <n,i> state to <m,0>, but guard against overflow:
         */
        if(m-n>(0x7fffffff-MAX_CP_COUNT-delta)/(handledCPCount+1)) {
            *pErrorCode=U_INTERNAL_PROGRAM_ERROR;
            return 0;
        }
        delta+=(m-n)*(handledCPCount+1);
        n=m;

        /* Encode a sequence of same code points n */
        for(j=0; j<srcCPCount; ++j) {
            q=cpBuffer[j]&0x7fffffff; /* remove case flag from the sign bit */
            if(q<n) {
                ++delta;
            } else if(q==n) {
                /* Represent delta as a generalized variable-length integer: */
                for(q=delta, k=BASE; /* no condition */; k+=BASE) {

                    /** RAM: comment out the old code for conformance with draft-ietf-idn-punycode-03.txt

                    t=k-bias;
                    if(t<TMIN) {
                        t=TMIN;
                    } else if(t>TMAX) {
                        t=TMAX;
                    }
                    */

                    t=k-bias;
                    if(t<TMIN) {
                        t=TMIN;
                    } else if(k>=(bias+TMAX)) {
                        t=TMAX;
                    }

                    if(q<t) {
                        break;
                    }

                    if(destLength<destCapacity) {
                        dest[destLength]=digitToBasic(t+(q-t)%(BASE-t), 0);
                    }
                    ++destLength;
                    q=(q-t)/(BASE-t);
                }

                if(destLength<destCapacity) {
                    dest[destLength]=digitToBasic(q, (UBool)(cpBuffer[j]<0));
                }
                ++destLength;
                bias=adaptBias(delta, handledCPCount+1, (UBool)(handledCPCount==basicLength));
                delta=0;
                ++handledCPCount;
            }
        }

        ++delta;
        ++n;
    }

    return u_terminateUChars(dest, destCapacity, destLength, pErrorCode);
}

U_CFUNC int32_t
u_strFromPunycode(const UChar *src, int32_t srcLength,
                  UChar *dest, int32_t destCapacity,
                  UBool *caseFlags,
                  UErrorCode *pErrorCode) {
    int32_t n, destLength, i, bias, basicLength, j, in, oldi, w, k, digit, t,
            destCPCount, firstSupplementaryIndex, cpLength;
    UChar b;

    /* argument checking */
    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return 0;
    }

    if(src==NULL || srcLength<-1 || (dest==NULL && destCapacity!=0)) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    if(srcLength==-1) {
        srcLength=u_strlen(src);
    }

    /*
     * Handle the basic code points:
     * Let basicLength be the number of input code points
     * before the last delimiter, or 0 if there is none,
     * then copy the first basicLength code points to the output.
     *
     * The two following loops iterate backward.
     */
    for(j=srcLength; j>0;) {
        if(src[--j]==DELIMITER) {
            break;
        }
    }
    destLength=basicLength=destCPCount=j;

    while(j>0) {
        b=src[--j];
        if(!IS_BASIC(b)) {
            *pErrorCode=U_INVALID_CHAR_FOUND;
            return 0;
        }

        if(j<destCapacity) {
            dest[j]=(UChar)b;

            if(caseFlags!=NULL) {
                caseFlags[j]=IS_BASIC_UPPERCASE(b);
            }
        }
    }

    /* Initialize the state: */
    n=INITIAL_N;
    i=0;
    bias=INITIAL_BIAS;
    firstSupplementaryIndex=1000000000;

    /*
     * Main decoding loop:
     * Start just after the last delimiter if any
     * basic code points were copied; start at the beginning otherwise.
     */
    for(in=basicLength>0 ? basicLength+1 : 0; in<srcLength; /* no op */) {
        /*
         * in is the index of the next character to be consumed, and
         * destCPCount is the number of code points in the output array.
         *
         * Decode a generalized variable-length integer into delta,
         * which gets added to i.  The overflow checking is easier
         * if we increase i as we go, then subtract off its starting
         * value at the end to obtain delta.
         */
        for(oldi=i, w=1, k=BASE; /* no condition */; k+=BASE) {
            if(in>=srcLength) {
                *pErrorCode=U_ILLEGAL_CHAR_FOUND;
                return 0;
            }

            digit=basicToDigit[(uint8_t)src[in++]];
            if(digit<0) {
                *pErrorCode=U_INVALID_CHAR_FOUND;
                return 0;
            }
            if(digit>(0x7fffffff-i)/w) {
                /* integer overflow */
                *pErrorCode=U_ILLEGAL_CHAR_FOUND;
                return 0;
            }

            i+=digit*w;
            /** RAM: comment out the old code for conformance with draft-ietf-idn-punycode-03.txt  
            t=k-bias;
            if(t<TMIN) {
                t=TMIN;
            } else if(t>TMAX) {
                t=TMAX;
            }
            */
            t=k-bias;
            if(t<TMIN) {
                t=TMIN;
            } else if(k>=(bias+TMAX)) {
                t=TMAX;
            }
            if(digit<t) {
                break;
            }

            if(w>0x7fffffff/(BASE-t)) {
                /* integer overflow */
                *pErrorCode=U_ILLEGAL_CHAR_FOUND;
                return 0;
            }
            w*=BASE-t;
        }

        /*
         * Modification from sample code:
         * Increments destCPCount here,
         * where needed instead of in for() loop tail.
         */
        ++destCPCount;
        bias=adaptBias(i-oldi, destCPCount, (UBool)(oldi==0));

        /*
         * i was supposed to wrap around from (incremented) destCPCount to 0,
         * incrementing n each time, so we'll fix that now:
         */
        if(i/destCPCount>(0x7fffffff-n)) {
            /* integer overflow */
            *pErrorCode=U_ILLEGAL_CHAR_FOUND;
            return 0;
        }

        n+=i/destCPCount;
        i%=destCPCount;
        /* not needed for Punycode: */
        /* if (decode_digit(n) <= BASE) return punycode_invalid_input; */

        if(n>0x10ffff || UTF_IS_SURROGATE(n)) {
            /* Unicode code point overflow */
            *pErrorCode=U_ILLEGAL_CHAR_FOUND;
            return 0;
        }

        /* Insert n at position i of the output: */
        cpLength=UTF_CHAR_LENGTH(n);
        if((destLength+cpLength)<=destCapacity) {
            int32_t codeUnitIndex;

            /*
             * Handle indexes when supplementary code points are present.
             *
             * In almost all cases, there will be only BMP code points before i
             * and even in the entire string.
             * This is handled with the same efficiency as with UTF-32.
             *
             * Only the rare cases with supplementary code points are handled
             * more slowly - but not too bad since this is an insertion anyway.
             */
            if(i<=firstSupplementaryIndex) {
                codeUnitIndex=i;
                if(cpLength>1) {
                    firstSupplementaryIndex=codeUnitIndex;
                } else {
                    ++firstSupplementaryIndex;
                }
            } else {
                codeUnitIndex=firstSupplementaryIndex;
                UTF_FWD_N(dest, codeUnitIndex, destLength, i-codeUnitIndex);
            }

            /* use the UChar index codeUnitIndex instead of the code point index i */
            if(codeUnitIndex<destLength) {
                uprv_memmove(dest+codeUnitIndex+cpLength,
                             dest+codeUnitIndex,
                             (destLength-codeUnitIndex)*U_SIZEOF_UCHAR);
                if(caseFlags!=NULL) {
                    uprv_memmove(caseFlags+codeUnitIndex+cpLength,
                                 caseFlags+codeUnitIndex,
                                 destLength-codeUnitIndex);
                }
            }
            if(cpLength==1) {
                /* BMP, insert one code unit */
                dest[codeUnitIndex]=(UChar)n;
            } else {
                /* supplementary character, insert two code units */
                dest[codeUnitIndex]=UTF16_LEAD(n);
                dest[codeUnitIndex+1]=UTF16_TRAIL(n);
            }
            if(caseFlags!=NULL) {
                /* Case of last character determines uppercase flag: */
                caseFlags[codeUnitIndex]=IS_BASIC_UPPERCASE(src[in-1]);
                if(cpLength==2) {
                    caseFlags[codeUnitIndex+1]=FALSE;
                }
            }
        }
        destLength+=cpLength;
        ++i;
    }

    return u_terminateUChars(dest, destCapacity, destLength, pErrorCode);
}

/* ### check notes on overflow handling - only necessary if not IDNA? are these Punycode functions to be public? */

#endif /* #if !UCONFIG_NO_IDNA */
