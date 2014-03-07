/*  
**********************************************************************
*   Copyright (C) 2002-2009, International Business Machines
*   Corporation and others.  All Rights Reserved.
**********************************************************************
*   file name:  ucnv_u7.c
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2002jul01
*   created by: Markus W. Scherer
*
*   UTF-7 converter implementation. Used to be in ucnv_utf.c.
*/

#include "unicode/utypes.h"

#if !UCONFIG_NO_CONVERSION

#include "unicode/ucnv.h"
#include "ucnv_bld.h"
#include "ucnv_cnv.h"

/* UTF-7 -------------------------------------------------------------------- */

/*
 * UTF-7 is a stateful encoding of Unicode.
 * It is defined in RFC 2152. (http://www.ietf.org/rfc/rfc2152.txt)
 * It was intended for use in Internet email systems, using in its bytewise
 * encoding only a subset of 7-bit US-ASCII.
 * UTF-7 is deprecated in favor of UTF-8/16/32 and SCSU, but still
 * occasionally used.
 *
 * For converting Unicode to UTF-7, the RFC allows to encode some US-ASCII
 * characters directly or in base64. Especially, the characters in set O
 * as defined in the RFC (see below) may be encoded directly but are not
 * allowed in, e.g., email headers.
 * By default, the ICU UTF-7 converter encodes set O directly.
 * By choosing the option "version=1", set O will be escaped instead.
 * For example:
 *     utf7Converter=ucnv_open("UTF-7,version=1");
 *
 * For details about email headers see RFC 2047.
 */

/*
 * Tests for US-ASCII characters belonging to character classes
 * defined in UTF-7.
 *
 * Set D (directly encoded characters) consists of the following
 * characters: the upper and lower case letters A through Z
 * and a through z, the 10 digits 0-9, and the following nine special
 * characters (note that "+" and "=" are omitted):
 *     '(),-./:?
 *
 * Set O (optional direct characters) consists of the following
 * characters (note that "\" and "~" are omitted):
 *     !"#$%&*;<=>@[]^_`{|}
 *
 * According to the rules in RFC 2152, the byte values for the following
 * US-ASCII characters are not used in UTF-7 and are therefore illegal:
 * - all C0 control codes except for CR LF TAB
 * - BACKSLASH
 * - TILDE
 * - DEL
 * - all codes beyond US-ASCII, i.e. all >127
 */
#define inSetD(c) \
    ((uint8_t)((c)-97)<26 || (uint8_t)((c)-65)<26 || /* letters */ \
     (uint8_t)((c)-48)<10 ||    /* digits */ \
     (uint8_t)((c)-39)<3 ||     /* '() */ \
     (uint8_t)((c)-44)<4 ||     /* ,-./ */ \
     (c)==58 || (c)==63         /* :? */ \
    )

#define inSetO(c) \
    ((uint8_t)((c)-33)<6 ||         /* !"#$%& */ \
     (uint8_t)((c)-59)<4 ||         /* ;<=> */ \
     (uint8_t)((c)-93)<4 ||         /* ]^_` */ \
     (uint8_t)((c)-123)<3 ||        /* {|} */ \
     (c)==42 || (c)==64 || (c)==91  /* *@[ */ \
    )

#define isCRLFTAB(c) ((c)==13 || (c)==10 || (c)==9)
#define isCRLFSPTAB(c) ((c)==32 || (c)==13 || (c)==10 || (c)==9)

#define PLUS  43
#define MINUS 45
#define BACKSLASH 92
#define TILDE 126

/* legal byte values: all US-ASCII graphic characters from space to before tilde, and CR LF TAB */
#define isLegalUTF7(c) (((uint8_t)((c)-32)<94 && (c)!=BACKSLASH) || isCRLFTAB(c))

/* encode directly sets D and O and CR LF SP TAB */
static const UBool encodeDirectlyMaximum[128]={
 /* 0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,

    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1,

    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0
};

/* encode directly set D and CR LF SP TAB but not set O */
static const UBool encodeDirectlyRestricted[128]={
 /* 0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

    1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1,

    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,

    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0
};

static const uint8_t
toBase64[64]={
    /* A-Z */
    65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77,
    78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90,
    /* a-z */
    97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109,
    110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122,
    /* 0-9 */
    48, 49, 50, 51, 52, 53, 54, 55, 56, 57,
    /* +/ */
    43, 47
};

static const int8_t
fromBase64[128]={
    /* C0 controls, -1 for legal ones (CR LF TAB), -3 for illegal ones */
    -3, -3, -3, -3, -3, -3, -3, -3, -3, -1, -1, -3, -3, -1, -3, -3,
    -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3,

    /* general punctuation with + and / and a special value (-2) for - */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -2, -1, 63,
    /* digits */
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,

    /* A-Z */
    -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -3, -1, -1, -1,

    /* a-z */
    -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -3, -3
};

/*
 * converter status values:
 *
 * toUnicodeStatus:
 *     24 inDirectMode (boolean)
 * 23..16 base64Counter (-1..7)
 * 15..0  bits (up to 14 bits incoming base64)
 *
 * fromUnicodeStatus:
 * 31..28 version (0: set O direct  1: set O escaped)
 *     24 inDirectMode (boolean)
 * 23..16 base64Counter (0..2)
 *  7..0  bits (6 bits outgoing base64)
 *
 */

static void
_UTF7Reset(UConverter *cnv, UConverterResetChoice choice) {
    if(choice<=UCNV_RESET_TO_UNICODE) {
        /* reset toUnicode */
        cnv->toUnicodeStatus=0x1000000; /* inDirectMode=TRUE */
        cnv->toULength=0;
    }
    if(choice!=UCNV_RESET_TO_UNICODE) {
        /* reset fromUnicode */
        cnv->fromUnicodeStatus=(cnv->fromUnicodeStatus&0xf0000000)|0x1000000; /* keep version, inDirectMode=TRUE */
    }
}

static void
_UTF7Open(UConverter *cnv,
          UConverterLoadArgs *pArgs,
          UErrorCode *pErrorCode) {
    if(UCNV_GET_VERSION(cnv)<=1) {
        /* TODO(markus): Should just use cnv->options rather than copying the version number. */
        cnv->fromUnicodeStatus=UCNV_GET_VERSION(cnv)<<28;
        _UTF7Reset(cnv, UCNV_RESET_BOTH);
    } else {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
    }
}

static void
_UTF7ToUnicodeWithOffsets(UConverterToUnicodeArgs *pArgs,
                          UErrorCode *pErrorCode) {
    UConverter *cnv;
    const uint8_t *source, *sourceLimit;
    UChar *target;
    const UChar *targetLimit;
    int32_t *offsets;

    uint8_t *bytes;
    uint8_t byteIndex;

    int32_t length, targetCapacity;

    /* UTF-7 state */
    uint16_t bits;
    int8_t base64Counter;
    UBool inDirectMode;

    int8_t base64Value;

    int32_t sourceIndex, nextSourceIndex;

    uint8_t b;
    /* set up the local pointers */
    cnv=pArgs->converter;

    source=(const uint8_t *)pArgs->source;
    sourceLimit=(const uint8_t *)pArgs->sourceLimit;
    target=pArgs->target;
    targetLimit=pArgs->targetLimit;
    offsets=pArgs->offsets;
    /* get the state machine state */
    {
        uint32_t status=cnv->toUnicodeStatus;
        inDirectMode=(UBool)((status>>24)&1);
        base64Counter=(int8_t)(status>>16);
        bits=(uint16_t)status;
    }
    bytes=cnv->toUBytes;
    byteIndex=cnv->toULength;

    /* sourceIndex=-1 if the current character began in the previous buffer */
    sourceIndex=byteIndex==0 ? 0 : -1;
    nextSourceIndex=0;

    if(inDirectMode) {
directMode:
        /*
         * In Direct Mode, most US-ASCII characters are encoded directly, i.e.,
         * with their US-ASCII byte values.
         * Backslash and Tilde and most control characters are not allowed in UTF-7.
         * A plus sign starts Unicode (or "escape") Mode.
         *
         * In Direct Mode, only the sourceIndex is used.
         */
        byteIndex=0;
        length=(int32_t)(sourceLimit-source);
        targetCapacity=(int32_t)(targetLimit-target);
        if(length>targetCapacity) {
            length=targetCapacity;
        }
        while(length>0) {
            b=*source++;
            if(!isLegalUTF7(b)) {
                /* illegal */
                bytes[0]=b;
                byteIndex=1;
                *pErrorCode=U_ILLEGAL_CHAR_FOUND;
                break;
            } else if(b!=PLUS) {
                /* write directly encoded character */
                *target++=b;
                if(offsets!=NULL) {
                    *offsets++=sourceIndex++;
                }
            } else /* PLUS */ {
                /* switch to Unicode mode */
                nextSourceIndex=++sourceIndex;
                inDirectMode=FALSE;
                byteIndex=0;
                bits=0;
                base64Counter=-1;
                goto unicodeMode;
            }
            --length;
        }
        if(source<sourceLimit && target>=targetLimit) {
            /* target is full */
            *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
        }
    } else {
unicodeMode:
        /*
         * In Unicode (or "escape") Mode, UTF-16BE is base64-encoded.
         * The base64 sequence ends with any character that is not in the base64 alphabet.
         * A terminating minus sign is consumed.
         *
         * In Unicode Mode, the sourceIndex has the index to the start of the current
         * base64 bytes, while nextSourceIndex is precisely parallel to source,
         * keeping the index to the following byte.
         * Note that in 2 out of 3 cases, UChars overlap within a base64 byte.
         */
        while(source<sourceLimit) {
            if(target<targetLimit) {
                bytes[byteIndex++]=b=*source++;
                ++nextSourceIndex;
                if(b>=126) {
                    /* illegal - test other illegal US-ASCII values by base64Value==-3 */
                    inDirectMode=TRUE;
                    *pErrorCode=U_ILLEGAL_CHAR_FOUND;
                    break;
                } else if((base64Value=fromBase64[b])>=0) {
                    /* collect base64 bytes into UChars */
                    switch(base64Counter) {
                    case -1: /* -1 is immediately after the + */
                    case 0:
                        bits=base64Value;
                        base64Counter=1;
                        break;
                    case 1:
                    case 3:
                    case 4:
                    case 6:
                        bits=(uint16_t)((bits<<6)|base64Value);
                        ++base64Counter;
                        break;
                    case 2:
                        *target++=(UChar)((bits<<4)|(base64Value>>2));
                        if(offsets!=NULL) {
                            *offsets++=sourceIndex;
                            sourceIndex=nextSourceIndex-1;
                        }
                        bytes[0]=b; /* keep this byte in case an error occurs */
                        byteIndex=1;
                        bits=(uint16_t)(base64Value&3);
                        base64Counter=3;
                        break;
                    case 5:
                        *target++=(UChar)((bits<<2)|(base64Value>>4));
                        if(offsets!=NULL) {
                            *offsets++=sourceIndex;
                            sourceIndex=nextSourceIndex-1;
                        }
                        bytes[0]=b; /* keep this byte in case an error occurs */
                        byteIndex=1;
                        bits=(uint16_t)(base64Value&15);
                        base64Counter=6;
                        break;
                    case 7:
                        *target++=(UChar)((bits<<6)|base64Value);
                        if(offsets!=NULL) {
                            *offsets++=sourceIndex;
                            sourceIndex=nextSourceIndex;
                        }
                        byteIndex=0;
                        bits=0;
                        base64Counter=0;
                        break;
                    default:
                        /* will never occur */
                        break;
                    }
                } else if(base64Value==-2) {
                    /* minus sign terminates the base64 sequence */
                    inDirectMode=TRUE;
                    if(base64Counter==-1) {
                        /* +- i.e. a minus immediately following a plus */
                        *target++=PLUS;
                        if(offsets!=NULL) {
                            *offsets++=sourceIndex-1;
                        }
                    } else {
                        /* absorb the minus and leave the Unicode Mode */
                        if(bits!=0) {
                            /* bits are illegally left over, a UChar is incomplete */
                            *pErrorCode=U_ILLEGAL_CHAR_FOUND;
                            break;
                        }
                    }
                    sourceIndex=nextSourceIndex;
                    goto directMode;
                } else if(base64Value==-1) /* for any legal character except base64 and minus sign */ {
                    /* leave the Unicode Mode */
                    inDirectMode=TRUE;
                    if(base64Counter==-1) {
                        /* illegal: + immediately followed by something other than base64 or minus sign */
                        /* include the plus sign in the reported sequence */
                        --sourceIndex;
                        bytes[0]=PLUS;
                        bytes[1]=b;
                        byteIndex=2;
                        *pErrorCode=U_ILLEGAL_CHAR_FOUND;
                        break;
                    } else if(bits==0) {
                        /* un-read the character in case it is a plus sign */
                        --source;
                        sourceIndex=nextSourceIndex-1;
                        goto directMode;
                    } else {
                        /* bits are illegally left over, a UChar is incomplete */
                        *pErrorCode=U_ILLEGAL_CHAR_FOUND;
                        break;
                    }
                } else /* base64Value==-3 for illegal characters */ {
                    /* illegal */
                    inDirectMode=TRUE;
                    *pErrorCode=U_ILLEGAL_CHAR_FOUND;
                    break;
                }
            } else {
                /* target is full */
                *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
                break;
            }
        }
    }

    if(U_SUCCESS(*pErrorCode) && pArgs->flush && source==sourceLimit && bits==0) {
        /*
         * if we are in Unicode mode, then the byteIndex might not be 0,
         * but that is ok if bits==0
         * -> we set byteIndex=0 at the end of the stream to avoid a truncated error
         * (not true for IMAP-mailbox-name where we must end in direct mode)
         */
        byteIndex=0;
    }

    /* set the converter state back into UConverter */
    cnv->toUnicodeStatus=((uint32_t)inDirectMode<<24)|((uint32_t)((uint8_t)base64Counter)<<16)|(uint32_t)bits;
    cnv->toULength=byteIndex;

    /* write back the updated pointers */
    pArgs->source=(const char *)source;
    pArgs->target=target;
    pArgs->offsets=offsets;
    return;
}

static void
_UTF7FromUnicodeWithOffsets(UConverterFromUnicodeArgs *pArgs,
                            UErrorCode *pErrorCode) {
    UConverter *cnv;
    const UChar *source, *sourceLimit;
    uint8_t *target, *targetLimit;
    int32_t *offsets;

    int32_t length, targetCapacity, sourceIndex;
    UChar c;

    /* UTF-7 state */
    const UBool *encodeDirectly;
    uint8_t bits;
    int8_t base64Counter;
    UBool inDirectMode;

    /* set up the local pointers */
    cnv=pArgs->converter;

    /* set up the local pointers */
    source=pArgs->source;
    sourceLimit=pArgs->sourceLimit;
    target=(uint8_t *)pArgs->target;
    targetLimit=(uint8_t *)pArgs->targetLimit;
    offsets=pArgs->offsets;

    /* get the state machine state */
    {
        uint32_t status=cnv->fromUnicodeStatus;
        encodeDirectly= status<0x10000000 ? encodeDirectlyMaximum : encodeDirectlyRestricted;
        inDirectMode=(UBool)((status>>24)&1);
        base64Counter=(int8_t)(status>>16);
        bits=(uint8_t)status;
    }

    /* UTF-7 always encodes UTF-16 code units, therefore we need only a simple sourceIndex */
    sourceIndex=0;

    if(inDirectMode) {
directMode:
        length=(int32_t)(sourceLimit-source);
        targetCapacity=(int32_t)(targetLimit-target);
        if(length>targetCapacity) {
            length=targetCapacity;
        }
        while(length>0) {
            c=*source++;
            /* currently always encode CR LF SP TAB directly */
            if(c<=127 && encodeDirectly[c]) {
                /* encode directly */
                *target++=(uint8_t)c;
                if(offsets!=NULL) {
                    *offsets++=sourceIndex++;
                }
            } else if(c==PLUS) {
                /* output +- for + */
                *target++=PLUS;
                if(target<targetLimit) {
                    *target++=MINUS;
                    if(offsets!=NULL) {
                        *offsets++=sourceIndex;
                        *offsets++=sourceIndex++;
                    }
                    /* realign length and targetCapacity */
                    goto directMode;
                } else {
                    if(offsets!=NULL) {
                        *offsets++=sourceIndex++;
                    }
                    cnv->charErrorBuffer[0]=MINUS;
                    cnv->charErrorBufferLength=1;
                    *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
                    break;
                }
            } else {
                /* un-read this character and switch to Unicode Mode */
                --source;
                *target++=PLUS;
                if(offsets!=NULL) {
                    *offsets++=sourceIndex;
                }
                inDirectMode=FALSE;
                base64Counter=0;
                goto unicodeMode;
            }
            --length;
        }
        if(source<sourceLimit && target>=targetLimit) {
            /* target is full */
            *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
        }
    } else {
unicodeMode:
        while(source<sourceLimit) {
            if(target<targetLimit) {
                c=*source++;
                if(c<=127 && encodeDirectly[c]) {
                    /* encode directly */
                    inDirectMode=TRUE;

                    /* trick: back out this character to make this easier */
                    --source;

                    /* terminate the base64 sequence */
                    if(base64Counter!=0) {
                        /* write remaining bits for the previous character */
                        *target++=toBase64[bits];
                        if(offsets!=NULL) {
                            *offsets++=sourceIndex-1;
                        }
                    }
                    if(fromBase64[c]!=-1) {
                        /* need to terminate with a minus */
                        if(target<targetLimit) {
                            *target++=MINUS;
                            if(offsets!=NULL) {
                                *offsets++=sourceIndex-1;
                            }
                        } else {
                            cnv->charErrorBuffer[0]=MINUS;
                            cnv->charErrorBufferLength=1;
                            *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
                            break;
                        }
                    }
                    goto directMode;
                } else {
                    /*
                     * base64 this character:
                     * Output 2 or 3 base64 bytes for the remaining bits of the previous character
                     * and the bits of this character, each implicitly in UTF-16BE.
                     *
                     * Here, bits is an 8-bit variable because only 6 bits need to be kept from one
                     * character to the next. The actual 2 or 4 bits are shifted to the left edge
                     * of the 6-bits field 5..0 to make the termination of the base64 sequence easier.
                     */
                    switch(base64Counter) {
                    case 0:
                        *target++=toBase64[c>>10];
                        if(target<targetLimit) {
                            *target++=toBase64[(c>>4)&0x3f];
                            if(offsets!=NULL) {
                                *offsets++=sourceIndex;
                                *offsets++=sourceIndex++;
                            }
                        } else {
                            if(offsets!=NULL) {
                                *offsets++=sourceIndex++;
                            }
                            cnv->charErrorBuffer[0]=toBase64[(c>>4)&0x3f];
                            cnv->charErrorBufferLength=1;
                            *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
                        }
                        bits=(uint8_t)((c&15)<<2);
                        base64Counter=1;
                        break;
                    case 1:
                        *target++=toBase64[bits|(c>>14)];
                        if(target<targetLimit) {
                            *target++=toBase64[(c>>8)&0x3f];
                            if(target<targetLimit) {
                                *target++=toBase64[(c>>2)&0x3f];
                                if(offsets!=NULL) {
                                    *offsets++=sourceIndex;
                                    *offsets++=sourceIndex;
                                    *offsets++=sourceIndex++;
                                }
                            } else {
                                if(offsets!=NULL) {
                                    *offsets++=sourceIndex;
                                    *offsets++=sourceIndex++;
                                }
                                cnv->charErrorBuffer[0]=toBase64[(c>>2)&0x3f];
                                cnv->charErrorBufferLength=1;
                                *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
                            }
                        } else {
                            if(offsets!=NULL) {
                                *offsets++=sourceIndex++;
                            }
                            cnv->charErrorBuffer[0]=toBase64[(c>>8)&0x3f];
                            cnv->charErrorBuffer[1]=toBase64[(c>>2)&0x3f];
                            cnv->charErrorBufferLength=2;
                            *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
                        }
                        bits=(uint8_t)((c&3)<<4);
                        base64Counter=2;
                        break;
                    case 2:
                        *target++=toBase64[bits|(c>>12)];
                        if(target<targetLimit) {
                            *target++=toBase64[(c>>6)&0x3f];
                            if(target<targetLimit) {
                                *target++=toBase64[c&0x3f];
                                if(offsets!=NULL) {
                                    *offsets++=sourceIndex;
                                    *offsets++=sourceIndex;
                                    *offsets++=sourceIndex++;
                                }
                            } else {
                                if(offsets!=NULL) {
                                    *offsets++=sourceIndex;
                                    *offsets++=sourceIndex++;
                                }
                                cnv->charErrorBuffer[0]=toBase64[c&0x3f];
                                cnv->charErrorBufferLength=1;
                                *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
                            }
                        } else {
                            if(offsets!=NULL) {
                                *offsets++=sourceIndex++;
                            }
                            cnv->charErrorBuffer[0]=toBase64[(c>>6)&0x3f];
                            cnv->charErrorBuffer[1]=toBase64[c&0x3f];
                            cnv->charErrorBufferLength=2;
                            *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
                        }
                        bits=0;
                        base64Counter=0;
                        break;
                    default:
                        /* will never occur */
                        break;
                    }
                }
            } else {
                /* target is full */
                *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
                break;
            }
        }
    }

    if(pArgs->flush && source>=sourceLimit) {
        /* flush remaining bits to the target */
        if(!inDirectMode && base64Counter!=0) {
            if(target<targetLimit) {
                *target++=toBase64[bits];
                if(offsets!=NULL) {
                    *offsets++=sourceIndex-1;
                }
            } else {
                cnv->charErrorBuffer[cnv->charErrorBufferLength++]=toBase64[bits];
                *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
            }
        }
        /* reset the state for the next conversion */
        cnv->fromUnicodeStatus=(cnv->fromUnicodeStatus&0xf0000000)|0x1000000; /* keep version, inDirectMode=TRUE */
    } else {
        /* set the converter state back into UConverter */
        cnv->fromUnicodeStatus=
            (cnv->fromUnicodeStatus&0xf0000000)|    /* keep version*/
            ((uint32_t)inDirectMode<<24)|((uint32_t)base64Counter<<16)|(uint32_t)bits;
    }

    /* write back the updated pointers */
    pArgs->source=source;
    pArgs->target=(char *)target;
    pArgs->offsets=offsets;
    return;
}

static const char *
_UTF7GetName(const UConverter *cnv) {
    switch(cnv->fromUnicodeStatus>>28) {
    case 1:
        return "UTF-7,version=1";
    default:
        return "UTF-7";
    }
}

static const UConverterImpl _UTF7Impl={
    UCNV_UTF7,

    NULL,
    NULL,

    _UTF7Open,
    NULL,
    _UTF7Reset,

    _UTF7ToUnicodeWithOffsets,
    _UTF7ToUnicodeWithOffsets,
    _UTF7FromUnicodeWithOffsets,
    _UTF7FromUnicodeWithOffsets,
    NULL,

    NULL,
    _UTF7GetName,
    NULL, /* we don't need writeSub() because we never call a callback at fromUnicode() */
    NULL,
    ucnv_getCompleteUnicodeSet
};

static const UConverterStaticData _UTF7StaticData={
    sizeof(UConverterStaticData),
    "UTF-7",
    0, /* TODO CCSID for UTF-7 */
    UCNV_IBM, UCNV_UTF7,
    1, 4,
    { 0x3f, 0, 0, 0 }, 1, /* the subchar is not used */
    FALSE, FALSE,
    0,
    0,
    { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 } /* reserved */
};

const UConverterSharedData _UTF7Data={
    sizeof(UConverterSharedData), ~((uint32_t)0),
    NULL, NULL, &_UTF7StaticData, FALSE, &_UTF7Impl,
    0
};

/* IMAP mailbox name encoding ----------------------------------------------- */

/*
 * RFC 2060: INTERNET MESSAGE ACCESS PROTOCOL - VERSION 4rev1
 * http://www.ietf.org/rfc/rfc2060.txt
 *
 * 5.1.3.  Mailbox International Naming Convention
 *
 * By convention, international mailbox names are specified using a
 * modified version of the UTF-7 encoding described in [UTF-7].  The
 * purpose of these modifications is to correct the following problems
 * with UTF-7:
 *
 *    1) UTF-7 uses the "+" character for shifting; this conflicts with
 *       the common use of "+" in mailbox names, in particular USENET
 *       newsgroup names.
 *
 *    2) UTF-7's encoding is BASE64 which uses the "/" character; this
 *       conflicts with the use of "/" as a popular hierarchy delimiter.
 *
 *    3) UTF-7 prohibits the unencoded usage of "\"; this conflicts with
 *       the use of "\" as a popular hierarchy delimiter.
 *
 *    4) UTF-7 prohibits the unencoded usage of "~"; this conflicts with
 *       the use of "~" in some servers as a home directory indicator.
 *
 *    5) UTF-7 permits multiple alternate forms to represent the same
 *       string; in particular, printable US-ASCII chararacters can be
 *       represented in encoded form.
 *
 * In modified UTF-7, printable US-ASCII characters except for "&"
 * represent themselves; that is, characters with octet values 0x20-0x25
 * and 0x27-0x7e.  The character "&" (0x26) is represented by the two-
 * octet sequence "&-".
 *
 * All other characters (octet values 0x00-0x1f, 0x7f-0xff, and all
 * Unicode 16-bit octets) are represented in modified BASE64, with a
 * further modification from [UTF-7] that "," is used instead of "/".
 * Modified BASE64 MUST NOT be used to represent any printing US-ASCII
 * character which can represent itself.
 *
 * "&" is used to shift to modified BASE64 and "-" to shift back to US-
 * ASCII.  All names start in US-ASCII, and MUST end in US-ASCII (that
 * is, a name that ends with a Unicode 16-bit octet MUST end with a "-
 * ").
 *
 * For example, here is a mailbox name which mixes English, Japanese,
 * and Chinese text: ~peter/mail/&ZeVnLIqe-/&U,BTFw-
 */

/*
 * Tests for US-ASCII characters belonging to character classes
 * defined in UTF-7.
 *
 * Set D (directly encoded characters) consists of the following
 * characters: the upper and lower case letters A through Z
 * and a through z, the 10 digits 0-9, and the following nine special
 * characters (note that "+" and "=" are omitted):
 *     '(),-./:?
 *
 * Set O (optional direct characters) consists of the following
 * characters (note that "\" and "~" are omitted):
 *     !"#$%&*;<=>@[]^_`{|}
 *
 * According to the rules in RFC 2152, the byte values for the following
 * US-ASCII characters are not used in UTF-7 and are therefore illegal:
 * - all C0 control codes except for CR LF TAB
 * - BACKSLASH
 * - TILDE
 * - DEL
 * - all codes beyond US-ASCII, i.e. all >127
 */

/* uses '&' not '+' to start a base64 sequence */
#define AMPERSAND 0x26
#define COMMA 0x2c
#define SLASH 0x2f

/* legal byte values: all US-ASCII graphic characters 0x20..0x7e */
#define isLegalIMAP(c) (0x20<=(c) && (c)<=0x7e)

/* direct-encode all of printable ASCII 0x20..0x7e except '&' 0x26 */
#define inSetDIMAP(c) (isLegalIMAP(c) && c!=AMPERSAND)

#define TO_BASE64_IMAP(n) ((n)<63 ? toBase64[n] : COMMA)
#define FROM_BASE64_IMAP(c) ((c)==COMMA ? 63 : (c)==SLASH ? -1 : fromBase64[c])

/*
 * converter status values:
 *
 * toUnicodeStatus:
 *     24 inDirectMode (boolean)
 * 23..16 base64Counter (-1..7)
 * 15..0  bits (up to 14 bits incoming base64)
 *
 * fromUnicodeStatus:
 *     24 inDirectMode (boolean)
 * 23..16 base64Counter (0..2)
 *  7..0  bits (6 bits outgoing base64)
 *
 * ignore bits 31..25
 */

static void
_IMAPToUnicodeWithOffsets(UConverterToUnicodeArgs *pArgs,
                          UErrorCode *pErrorCode) {
    UConverter *cnv;
    const uint8_t *source, *sourceLimit;
    UChar *target;
    const UChar *targetLimit;
    int32_t *offsets;

    uint8_t *bytes;
    uint8_t byteIndex;

    int32_t length, targetCapacity;

    /* UTF-7 state */
    uint16_t bits;
    int8_t base64Counter;
    UBool inDirectMode;

    int8_t base64Value;

    int32_t sourceIndex, nextSourceIndex;

    UChar c;
    uint8_t b;

    /* set up the local pointers */
    cnv=pArgs->converter;

    source=(const uint8_t *)pArgs->source;
    sourceLimit=(const uint8_t *)pArgs->sourceLimit;
    target=pArgs->target;
    targetLimit=pArgs->targetLimit;
    offsets=pArgs->offsets;
    /* get the state machine state */
    {
        uint32_t status=cnv->toUnicodeStatus;
        inDirectMode=(UBool)((status>>24)&1);
        base64Counter=(int8_t)(status>>16);
        bits=(uint16_t)status;
    }
    bytes=cnv->toUBytes;
    byteIndex=cnv->toULength;

    /* sourceIndex=-1 if the current character began in the previous buffer */
    sourceIndex=byteIndex==0 ? 0 : -1;
    nextSourceIndex=0;

    if(inDirectMode) {
directMode:
        /*
         * In Direct Mode, US-ASCII characters are encoded directly, i.e.,
         * with their US-ASCII byte values.
         * An ampersand starts Unicode (or "escape") Mode.
         *
         * In Direct Mode, only the sourceIndex is used.
         */
        byteIndex=0;
        length=(int32_t)(sourceLimit-source);
        targetCapacity=(int32_t)(targetLimit-target);
        if(length>targetCapacity) {
            length=targetCapacity;
        }
        while(length>0) {
            b=*source++;
            if(!isLegalIMAP(b)) {
                /* illegal */
                bytes[0]=b;
                byteIndex=1;
                *pErrorCode=U_ILLEGAL_CHAR_FOUND;
                break;
            } else if(b!=AMPERSAND) {
                /* write directly encoded character */
                *target++=b;
                if(offsets!=NULL) {
                    *offsets++=sourceIndex++;
                }
            } else /* AMPERSAND */ {
                /* switch to Unicode mode */
                nextSourceIndex=++sourceIndex;
                inDirectMode=FALSE;
                byteIndex=0;
                bits=0;
                base64Counter=-1;
                goto unicodeMode;
            }
            --length;
        }
        if(source<sourceLimit && target>=targetLimit) {
            /* target is full */
            *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
        }
    } else {
unicodeMode:
        /*
         * In Unicode (or "escape") Mode, UTF-16BE is base64-encoded.
         * The base64 sequence ends with any character that is not in the base64 alphabet.
         * A terminating minus sign is consumed.
         * US-ASCII must not be base64-ed.
         *
         * In Unicode Mode, the sourceIndex has the index to the start of the current
         * base64 bytes, while nextSourceIndex is precisely parallel to source,
         * keeping the index to the following byte.
         * Note that in 2 out of 3 cases, UChars overlap within a base64 byte.
         */
        while(source<sourceLimit) {
            if(target<targetLimit) {
                bytes[byteIndex++]=b=*source++;
                ++nextSourceIndex;
                if(b>0x7e) {
                    /* illegal - test other illegal US-ASCII values by base64Value==-3 */
                    inDirectMode=TRUE;
                    *pErrorCode=U_ILLEGAL_CHAR_FOUND;
                    break;
                } else if((base64Value=FROM_BASE64_IMAP(b))>=0) {
                    /* collect base64 bytes into UChars */
                    switch(base64Counter) {
                    case -1: /* -1 is immediately after the & */
                    case 0:
                        bits=base64Value;
                        base64Counter=1;
                        break;
                    case 1:
                    case 3:
                    case 4:
                    case 6:
                        bits=(uint16_t)((bits<<6)|base64Value);
                        ++base64Counter;
                        break;
                    case 2:
                        c=(UChar)((bits<<4)|(base64Value>>2));
                        if(isLegalIMAP(c)) {
                            /* illegal */
                            inDirectMode=TRUE;
                            *pErrorCode=U_ILLEGAL_CHAR_FOUND;
                            goto endloop;
                        }
                        *target++=c;
                        if(offsets!=NULL) {
                            *offsets++=sourceIndex;
                            sourceIndex=nextSourceIndex-1;
                        }
                        bytes[0]=b; /* keep this byte in case an error occurs */
                        byteIndex=1;
                        bits=(uint16_t)(base64Value&3);
                        base64Counter=3;
                        break;
                    case 5:
                        c=(UChar)((bits<<2)|(base64Value>>4));
                        if(isLegalIMAP(c)) {
                            /* illegal */
                            inDirectMode=TRUE;
                            *pErrorCode=U_ILLEGAL_CHAR_FOUND;
                            goto endloop;
                        }
                        *target++=c;
                        if(offsets!=NULL) {
                            *offsets++=sourceIndex;
                            sourceIndex=nextSourceIndex-1;
                        }
                        bytes[0]=b; /* keep this byte in case an error occurs */
                        byteIndex=1;
                        bits=(uint16_t)(base64Value&15);
                        base64Counter=6;
                        break;
                    case 7:
                        c=(UChar)((bits<<6)|base64Value);
                        if(isLegalIMAP(c)) {
                            /* illegal */
                            inDirectMode=TRUE;
                            *pErrorCode=U_ILLEGAL_CHAR_FOUND;
                            goto endloop;
                        }
                        *target++=c;
                        if(offsets!=NULL) {
                            *offsets++=sourceIndex;
                            sourceIndex=nextSourceIndex;
                        }
                        byteIndex=0;
                        bits=0;
                        base64Counter=0;
                        break;
                    default:
                        /* will never occur */
                        break;
                    }
                } else if(base64Value==-2) {
                    /* minus sign terminates the base64 sequence */
                    inDirectMode=TRUE;
                    if(base64Counter==-1) {
                        /* &- i.e. a minus immediately following an ampersand */
                        *target++=AMPERSAND;
                        if(offsets!=NULL) {
                            *offsets++=sourceIndex-1;
                        }
                    } else {
                        /* absorb the minus and leave the Unicode Mode */
                        if(bits!=0 || (base64Counter!=0 && base64Counter!=3 && base64Counter!=6)) {
                            /* bits are illegally left over, a UChar is incomplete */
                            /* base64Counter other than 0, 3, 6 means non-minimal zero-padding, also illegal */
                            *pErrorCode=U_ILLEGAL_CHAR_FOUND;
                            break;
                        }
                    }
                    sourceIndex=nextSourceIndex;
                    goto directMode;
                } else {
                    if(base64Counter==-1) {
                        /* illegal: & immediately followed by something other than base64 or minus sign */
                        /* include the ampersand in the reported sequence */
                        --sourceIndex;
                        bytes[0]=AMPERSAND;
                        bytes[1]=b;
                        byteIndex=2;
                    }
                    /* base64Value==-1 for characters that are illegal only in Unicode mode */
                    /* base64Value==-3 for illegal characters */
                    /* illegal */
                    inDirectMode=TRUE;
                    *pErrorCode=U_ILLEGAL_CHAR_FOUND;
                    break;
                }
            } else {
                /* target is full */
                *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
                break;
            }
        }
    }
endloop:

    /*
     * the end of the input stream and detection of truncated input
     * are handled by the framework, but here we must check if we are in Unicode
     * mode and byteIndex==0 because we must end in direct mode
     *
     * conditions:
     *   successful
     *   in Unicode mode and byteIndex==0
     *   end of input and no truncated input
     */
    if( U_SUCCESS(*pErrorCode) &&
        !inDirectMode && byteIndex==0 &&
        pArgs->flush && source>=sourceLimit
    ) {
        if(base64Counter==-1) {
            /* & at the very end of the input */
            /* make the ampersand the reported sequence */
            bytes[0]=AMPERSAND;
            byteIndex=1;
        }
        /* else if(base64Counter!=-1) byteIndex remains 0 because there is no particular byte sequence */

        inDirectMode=TRUE; /* avoid looping */
        *pErrorCode=U_TRUNCATED_CHAR_FOUND;
    }

    /* set the converter state back into UConverter */
    cnv->toUnicodeStatus=((uint32_t)inDirectMode<<24)|((uint32_t)((uint8_t)base64Counter)<<16)|(uint32_t)bits;
    cnv->toULength=byteIndex;

    /* write back the updated pointers */
    pArgs->source=(const char *)source;
    pArgs->target=target;
    pArgs->offsets=offsets;
    return;
}

static void
_IMAPFromUnicodeWithOffsets(UConverterFromUnicodeArgs *pArgs,
                            UErrorCode *pErrorCode) {
    UConverter *cnv;
    const UChar *source, *sourceLimit;
    uint8_t *target, *targetLimit;
    int32_t *offsets;

    int32_t length, targetCapacity, sourceIndex;
    UChar c;
    uint8_t b;

    /* UTF-7 state */
    uint8_t bits;
    int8_t base64Counter;
    UBool inDirectMode;

    /* set up the local pointers */
    cnv=pArgs->converter;

    /* set up the local pointers */
    source=pArgs->source;
    sourceLimit=pArgs->sourceLimit;
    target=(uint8_t *)pArgs->target;
    targetLimit=(uint8_t *)pArgs->targetLimit;
    offsets=pArgs->offsets;

    /* get the state machine state */
    {
        uint32_t status=cnv->fromUnicodeStatus;
        inDirectMode=(UBool)((status>>24)&1);
        base64Counter=(int8_t)(status>>16);
        bits=(uint8_t)status;
    }

    /* UTF-7 always encodes UTF-16 code units, therefore we need only a simple sourceIndex */
    sourceIndex=0;

    if(inDirectMode) {
directMode:
        length=(int32_t)(sourceLimit-source);
        targetCapacity=(int32_t)(targetLimit-target);
        if(length>targetCapacity) {
            length=targetCapacity;
        }
        while(length>0) {
            c=*source++;
            /* encode 0x20..0x7e except '&' directly */
            if(inSetDIMAP(c)) {
                /* encode directly */
                *target++=(uint8_t)c;
                if(offsets!=NULL) {
                    *offsets++=sourceIndex++;
                }
            } else if(c==AMPERSAND) {
                /* output &- for & */
                *target++=AMPERSAND;
                if(target<targetLimit) {
                    *target++=MINUS;
                    if(offsets!=NULL) {
                        *offsets++=sourceIndex;
                        *offsets++=sourceIndex++;
                    }
                    /* realign length and targetCapacity */
                    goto directMode;
                } else {
                    if(offsets!=NULL) {
                        *offsets++=sourceIndex++;
                    }
                    cnv->charErrorBuffer[0]=MINUS;
                    cnv->charErrorBufferLength=1;
                    *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
                    break;
                }
            } else {
                /* un-read this character and switch to Unicode Mode */
                --source;
                *target++=AMPERSAND;
                if(offsets!=NULL) {
                    *offsets++=sourceIndex;
                }
                inDirectMode=FALSE;
                base64Counter=0;
                goto unicodeMode;
            }
            --length;
        }
        if(source<sourceLimit && target>=targetLimit) {
            /* target is full */
            *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
        }
    } else {
unicodeMode:
        while(source<sourceLimit) {
            if(target<targetLimit) {
                c=*source++;
                if(isLegalIMAP(c)) {
                    /* encode directly */
                    inDirectMode=TRUE;

                    /* trick: back out this character to make this easier */
                    --source;

                    /* terminate the base64 sequence */
                    if(base64Counter!=0) {
                        /* write remaining bits for the previous character */
                        *target++=TO_BASE64_IMAP(bits);
                        if(offsets!=NULL) {
                            *offsets++=sourceIndex-1;
                        }
                    }
                    /* need to terminate with a minus */
                    if(target<targetLimit) {
                        *target++=MINUS;
                        if(offsets!=NULL) {
                            *offsets++=sourceIndex-1;
                        }
                    } else {
                        cnv->charErrorBuffer[0]=MINUS;
                        cnv->charErrorBufferLength=1;
                        *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
                        break;
                    }
                    goto directMode;
                } else {
                    /*
                     * base64 this character:
                     * Output 2 or 3 base64 bytes for the remaining bits of the previous character
                     * and the bits of this character, each implicitly in UTF-16BE.
                     *
                     * Here, bits is an 8-bit variable because only 6 bits need to be kept from one
                     * character to the next. The actual 2 or 4 bits are shifted to the left edge
                     * of the 6-bits field 5..0 to make the termination of the base64 sequence easier.
                     */
                    switch(base64Counter) {
                    case 0:
                        b=(uint8_t)(c>>10);
                        *target++=TO_BASE64_IMAP(b);
                        if(target<targetLimit) {
                            b=(uint8_t)((c>>4)&0x3f);
                            *target++=TO_BASE64_IMAP(b);
                            if(offsets!=NULL) {
                                *offsets++=sourceIndex;
                                *offsets++=sourceIndex++;
                            }
                        } else {
                            if(offsets!=NULL) {
                                *offsets++=sourceIndex++;
                            }
                            b=(uint8_t)((c>>4)&0x3f);
                            cnv->charErrorBuffer[0]=TO_BASE64_IMAP(b);
                            cnv->charErrorBufferLength=1;
                            *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
                        }
                        bits=(uint8_t)((c&15)<<2);
                        base64Counter=1;
                        break;
                    case 1:
                        b=(uint8_t)(bits|(c>>14));
                        *target++=TO_BASE64_IMAP(b);
                        if(target<targetLimit) {
                            b=(uint8_t)((c>>8)&0x3f);
                            *target++=TO_BASE64_IMAP(b);
                            if(target<targetLimit) {
                                b=(uint8_t)((c>>2)&0x3f);
                                *target++=TO_BASE64_IMAP(b);
                                if(offsets!=NULL) {
                                    *offsets++=sourceIndex;
                                    *offsets++=sourceIndex;
                                    *offsets++=sourceIndex++;
                                }
                            } else {
                                if(offsets!=NULL) {
                                    *offsets++=sourceIndex;
                                    *offsets++=sourceIndex++;
                                }
                                b=(uint8_t)((c>>2)&0x3f);
                                cnv->charErrorBuffer[0]=TO_BASE64_IMAP(b);
                                cnv->charErrorBufferLength=1;
                                *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
                            }
                        } else {
                            if(offsets!=NULL) {
                                *offsets++=sourceIndex++;
                            }
                            b=(uint8_t)((c>>8)&0x3f);
                            cnv->charErrorBuffer[0]=TO_BASE64_IMAP(b);
                            b=(uint8_t)((c>>2)&0x3f);
                            cnv->charErrorBuffer[1]=TO_BASE64_IMAP(b);
                            cnv->charErrorBufferLength=2;
                            *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
                        }
                        bits=(uint8_t)((c&3)<<4);
                        base64Counter=2;
                        break;
                    case 2:
                        b=(uint8_t)(bits|(c>>12));
                        *target++=TO_BASE64_IMAP(b);
                        if(target<targetLimit) {
                            b=(uint8_t)((c>>6)&0x3f);
                            *target++=TO_BASE64_IMAP(b);
                            if(target<targetLimit) {
                                b=(uint8_t)(c&0x3f);
                                *target++=TO_BASE64_IMAP(b);
                                if(offsets!=NULL) {
                                    *offsets++=sourceIndex;
                                    *offsets++=sourceIndex;
                                    *offsets++=sourceIndex++;
                                }
                            } else {
                                if(offsets!=NULL) {
                                    *offsets++=sourceIndex;
                                    *offsets++=sourceIndex++;
                                }
                                b=(uint8_t)(c&0x3f);
                                cnv->charErrorBuffer[0]=TO_BASE64_IMAP(b);
                                cnv->charErrorBufferLength=1;
                                *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
                            }
                        } else {
                            if(offsets!=NULL) {
                                *offsets++=sourceIndex++;
                            }
                            b=(uint8_t)((c>>6)&0x3f);
                            cnv->charErrorBuffer[0]=TO_BASE64_IMAP(b);
                            b=(uint8_t)(c&0x3f);
                            cnv->charErrorBuffer[1]=TO_BASE64_IMAP(b);
                            cnv->charErrorBufferLength=2;
                            *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
                        }
                        bits=0;
                        base64Counter=0;
                        break;
                    default:
                        /* will never occur */
                        break;
                    }
                }
            } else {
                /* target is full */
                *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
                break;
            }
        }
    }

    if(pArgs->flush && source>=sourceLimit) {
        /* flush remaining bits to the target */
        if(!inDirectMode) {
            if(base64Counter!=0) {
                if(target<targetLimit) {
                    *target++=TO_BASE64_IMAP(bits);
                    if(offsets!=NULL) {
                        *offsets++=sourceIndex-1;
                    }
                } else {
                    cnv->charErrorBuffer[cnv->charErrorBufferLength++]=TO_BASE64_IMAP(bits);
                    *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
                }
            }
            /* need to terminate with a minus */
            if(target<targetLimit) {
                *target++=MINUS;
                if(offsets!=NULL) {
                    *offsets++=sourceIndex-1;
                }
            } else {
                cnv->charErrorBuffer[cnv->charErrorBufferLength++]=MINUS;
                *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
            }
        }
        /* reset the state for the next conversion */
        cnv->fromUnicodeStatus=(cnv->fromUnicodeStatus&0xf0000000)|0x1000000; /* keep version, inDirectMode=TRUE */
    } else {
        /* set the converter state back into UConverter */
        cnv->fromUnicodeStatus=
            (cnv->fromUnicodeStatus&0xf0000000)|    /* keep version*/
            ((uint32_t)inDirectMode<<24)|((uint32_t)base64Counter<<16)|(uint32_t)bits;
    }

    /* write back the updated pointers */
    pArgs->source=source;
    pArgs->target=(char *)target;
    pArgs->offsets=offsets;
    return;
}

static const UConverterImpl _IMAPImpl={
    UCNV_IMAP_MAILBOX,

    NULL,
    NULL,

    _UTF7Open,
    NULL,
    _UTF7Reset,

    _IMAPToUnicodeWithOffsets,
    _IMAPToUnicodeWithOffsets,
    _IMAPFromUnicodeWithOffsets,
    _IMAPFromUnicodeWithOffsets,
    NULL,

    NULL,
    NULL,
    NULL, /* we don't need writeSub() because we never call a callback at fromUnicode() */
    NULL,
    ucnv_getCompleteUnicodeSet
};

static const UConverterStaticData _IMAPStaticData={
    sizeof(UConverterStaticData),
    "IMAP-mailbox-name",
    0, /* TODO CCSID for IMAP-mailbox-name */
    UCNV_IBM, UCNV_IMAP_MAILBOX,
    1, 4,
    { 0x3f, 0, 0, 0 }, 1, /* the subchar is not used */
    FALSE, FALSE,
    0,
    0,
    { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 } /* reserved */
};

const UConverterSharedData _IMAPData={
    sizeof(UConverterSharedData), ~((uint32_t)0),
    NULL, NULL, &_IMAPStaticData, FALSE, &_IMAPImpl,
    0
};

#endif
