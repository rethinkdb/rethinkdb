/*
*******************************************************************************
*   Copyright (C) 2001-2003, International Business Machines
*   Corporation and others.  All Rights Reserved.
*******************************************************************************
*   file name:  bocsu.c
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   Author: Markus W. Scherer
*
*   Modification history:
*   05/18/2001  weiv    Made into separate module
*/


#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "bocsu.h"

/*
 * encode one difference value -0x10ffff..+0x10ffff in 1..3 bytes,
 * preserving lexical order
 */
U_CFUNC uint8_t *
u_writeDiff(int32_t diff, uint8_t *p) {
    if(diff>=SLOPE_REACH_NEG_1) {
        if(diff<=SLOPE_REACH_POS_1) {
            *p++=(uint8_t)(SLOPE_MIDDLE+diff);
        } else if(diff<=SLOPE_REACH_POS_2) {
            *p++=(uint8_t)(SLOPE_START_POS_2+(diff/SLOPE_TAIL_COUNT));
            *p++=(uint8_t)(SLOPE_MIN+diff%SLOPE_TAIL_COUNT);
        } else if(diff<=SLOPE_REACH_POS_3) {
            p[2]=(uint8_t)(SLOPE_MIN+diff%SLOPE_TAIL_COUNT);
            diff/=SLOPE_TAIL_COUNT;
            p[1]=(uint8_t)(SLOPE_MIN+diff%SLOPE_TAIL_COUNT);
            *p=(uint8_t)(SLOPE_START_POS_3+(diff/SLOPE_TAIL_COUNT));
            p+=3;
        } else {
            p[3]=(uint8_t)(SLOPE_MIN+diff%SLOPE_TAIL_COUNT);
            diff/=SLOPE_TAIL_COUNT;
            p[2]=(uint8_t)(SLOPE_MIN+diff%SLOPE_TAIL_COUNT);
            diff/=SLOPE_TAIL_COUNT;
            p[1]=(uint8_t)(SLOPE_MIN+diff%SLOPE_TAIL_COUNT);
            *p=SLOPE_MAX;
            p+=4;
        }
    } else {
        int32_t m;

        if(diff>=SLOPE_REACH_NEG_2) {
            NEGDIVMOD(diff, SLOPE_TAIL_COUNT, m);
            *p++=(uint8_t)(SLOPE_START_NEG_2+diff);
            *p++=(uint8_t)(SLOPE_MIN+m);
        } else if(diff>=SLOPE_REACH_NEG_3) {
            NEGDIVMOD(diff, SLOPE_TAIL_COUNT, m);
            p[2]=(uint8_t)(SLOPE_MIN+m);
            NEGDIVMOD(diff, SLOPE_TAIL_COUNT, m);
            p[1]=(uint8_t)(SLOPE_MIN+m);
            *p=(uint8_t)(SLOPE_START_NEG_3+diff);
            p+=3;
        } else {
            NEGDIVMOD(diff, SLOPE_TAIL_COUNT, m);
            p[3]=(uint8_t)(SLOPE_MIN+m);
            NEGDIVMOD(diff, SLOPE_TAIL_COUNT, m);
            p[2]=(uint8_t)(SLOPE_MIN+m);
            NEGDIVMOD(diff, SLOPE_TAIL_COUNT, m);
            p[1]=(uint8_t)(SLOPE_MIN+m);
            *p=SLOPE_MIN;
            p+=4;
        }
    }
    return p;
}

/* How many bytes would writeDiff() write? */
static int32_t
lengthOfDiff(int32_t diff) {
    if(diff>=SLOPE_REACH_NEG_1) {
        if(diff<=SLOPE_REACH_POS_1) {
            return 1;
        } else if(diff<=SLOPE_REACH_POS_2) {
            return 2;
        } else if(diff<=SLOPE_REACH_POS_3) {
            return 3;
        } else {
            return 4;
        }
    } else {
        if(diff>=SLOPE_REACH_NEG_2) {
            return 2;
        } else if(diff>=SLOPE_REACH_NEG_3) {
            return 3;
        } else {
            return 4;
        }
    }
}

/*
 * Encode the code points of a string as
 * a sequence of byte-encoded differences (slope detection),
 * preserving lexical order.
 *
 * Optimize the difference-taking for runs of Unicode text within
 * small scripts:
 *
 * Most small scripts are allocated within aligned 128-blocks of Unicode
 * code points. Lexical order is preserved if "prev" is always moved
 * into the middle of such a block.
 *
 * Additionally, "prev" is moved from anywhere in the Unihan
 * area into the middle of that area.
 * Note that the identical-level run in a sort key is generated from
 * NFD text - there are never Hangul characters included.
 */
U_CFUNC int32_t
u_writeIdenticalLevelRun(const UChar *s, int32_t length, uint8_t *p) {
    uint8_t *p0;
    int32_t c, prev;
    int32_t i;

    prev=0;
    p0=p;
    i=0;
    while(i<length) {
        if(prev<0x4e00 || prev>=0xa000) {
            prev=(prev&~0x7f)-SLOPE_REACH_NEG_1;
        } else {
            /*
             * Unihan U+4e00..U+9fa5:
             * double-bytes down from the upper end
             */
            prev=0x9fff-SLOPE_REACH_POS_2;
        }

        UTF_NEXT_CHAR(s, i, length, c);
        p=u_writeDiff(c-prev, p);
        prev=c;
    }
    return (int32_t)(p-p0);
}

U_CFUNC int32_t
u_writeIdenticalLevelRunTwoChars(UChar32 first, UChar32 second, uint8_t *p) {
    uint8_t *p0 = p;
    if(first<0x4e00 || first>=0xa000) {
        first=(first&~0x7f)-SLOPE_REACH_NEG_1;
    } else {
        /*
         * Unihan U+4e00..U+9fa5:
         * double-bytes down from the upper end
         */
        first=0x9fff-SLOPE_REACH_POS_2;
    }

    p=u_writeDiff(second-first, p);
    return (int32_t)(p-p0);
}

/* How many bytes would writeIdenticalLevelRun() write? */
U_CFUNC int32_t
u_lengthOfIdenticalLevelRun(const UChar *s, int32_t length) {
    int32_t c, prev;
    int32_t i, runLength;

    prev=0;
    runLength=0;
    i=0;
    while(i<length) {
        if(prev<0x4e00 || prev>=0xa000) {
            prev=(prev&~0x7f)-SLOPE_REACH_NEG_1;
        } else {
            /*
             * Unihan U+4e00..U+9fa5:
             * double-bytes down from the upper end
             */
            prev=0x9fff-SLOPE_REACH_POS_2;
        }

        UTF_NEXT_CHAR(s, i, length, c);
        runLength+=lengthOfDiff(c-prev);
        prev=c;
    }
    return runLength;
}

#endif /* #if !UCONFIG_NO_COLLATION */
