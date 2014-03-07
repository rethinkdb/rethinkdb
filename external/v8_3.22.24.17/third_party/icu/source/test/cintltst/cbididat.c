/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2007, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/
/*   file name:  cbididat.c
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 1999sep22
*   created by: Markus W. Scherer
*/

#include "unicode/utypes.h"
#include "unicode/uchar.h"
#include "unicode/ubidi.h"
#include "cbiditst.h"

const char * const
dirPropNames[U_CHAR_DIRECTION_COUNT]={
    "L", "R", "EN", "ES", "ET", "AN", "CS", "B", "S", "WS", "ON",
    "LRE", "LRO", "AL", "RLE", "RLO", "PDF", "NSM", "BN"
};

UChar
charFromDirProp[U_CHAR_DIRECTION_COUNT]={
 /* L     R      EN    ES    ET    AN     CS    B    S    WS    ON */
    0x61, 0x5d0, 0x30, 0x2f, 0x25, 0x660, 0x2c, 0xa, 0x9, 0x20, 0x26,
 /* LRE     LRO     AL     RLE     RLO     PDF     NSM    BN */
    0x202a, 0x202d, 0x627, 0x202b, 0x202e, 0x202c, 0x308, 0x200c
};

static const uint8_t
testText1[]={
    L, L, WS, L, WS, EN, L, B
};

static const UBiDiLevel
testLevels1[]={
    0, 0, 0, 0, 0, 0, 0, 0
};

static const uint8_t
testVisualMap1[]={
    0, 1, 2, 3, 4, 5, 6, 7
};

static const uint8_t
testText2[]={
    R, AL, WS, R, AL, WS, R
};

static const UBiDiLevel
testLevels2[]={
    1, 1, 1, 1, 1, 1, 1
};

static const uint8_t
testVisualMap2[]={
    6, 5, 4, 3, 2, 1, 0
};

static const uint8_t
testText3[]={
    L, L, WS, EN, CS, WS, EN, CS, EN, WS, L, L
};

static const UBiDiLevel
testLevels3[]={
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static const uint8_t
testVisualMap3[]={
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11
};

static const uint8_t
testText4[]={
    L, AL, AL, AL, L, AL, AL, L, WS, EN, CS, WS, EN, CS, EN, WS, L, L
};

static const UBiDiLevel
testLevels4[]={
    0, 1, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static const uint8_t
testVisualMap4[]={
    0, 3, 2, 1, 4, 6, 5, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17
};

static const uint8_t
testText5[]={
    AL, R, AL, WS, EN, CS, WS, EN, CS, EN, WS, R, R, WS, L, L
};

static const UBiDiLevel
testLevels5[]={
    1, 1, 1, 1, 2, 1, 1, 2, 2, 2, 1, 1, 1, 1, 2, 2
};

static const uint8_t
testVisualMap5[]={
    15, 14, 13, 12, 11, 10, 9, 6, 7, 8, 5, 4, 3, 2, 0, 1
};

static const uint8_t
testText6[]={
    R, EN, NSM, ET
};

static const UBiDiLevel
testLevels6[]={
    1, 2, 2, 2
};

static const uint8_t
testVisualMap6[]={
    3, 0, 1, 2
};

#if 0
static const uint8_t
testText7[]={
    /* empty */
};

static const UBiDiLevel
testLevels7[]={
};

static const uint8_t
testVisualMap7[]={
};

#endif

static const uint8_t
testText8[]={
    RLE, WS, R, R, R, WS, PDF, WS, B
};

static const UBiDiLevel
testLevels8[]={
    1, 1, 1, 1, 1, 1, 1, 1, 1
};

static const uint8_t
testVisualMap8[]={
    8, 7, 6, 5, 4, 3, 2, 1, 0
};

static const uint8_t
testText9[]={
    LRE, LRE, LRE, LRE, LRE, LRE, LRE, LRE, LRE, LRE, LRE, LRE, LRE, LRE, LRE,
    LRE, LRE, LRE, LRE, LRE, LRE, LRE, LRE, LRE, LRE, LRE, LRE, LRE, LRE, LRE,
    AN, RLO, NSM, LRE, PDF, RLE, ES, EN, ON
};

static const UBiDiLevel
testLevels9[]={
    62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 61, 61, 61, 61, 61, 61, 61, 61
};

static const uint8_t
testVisualMap9[]={
    8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 7, 6, 5, 4, 3, 2, 1, 0
};

static const uint8_t
testText10[]={
    LRE, LRE, LRE, LRE, LRE, LRE, LRE, LRE, LRE, LRE, LRE, LRE, LRE, LRE, LRE,
    LRE, LRE, LRE, LRE, LRE, LRE, LRE, LRE, LRE, LRE, LRE, LRE, LRE, LRE, LRE,
    LRE, BN, CS, RLO, S, PDF, EN, LRO, AN, ES
};

static const UBiDiLevel
testLevels10[]={
    60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 0, 0, 62, 62, 62, 62, 60
};

static const uint8_t
testVisualMap10[]={
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39
};

static const uint8_t
testText11[]={
    S, WS, NSM, RLE, WS, L, L, L, WS, LRO, WS, R, R, R, WS, RLO, WS, L, L,
    L, WS, LRE, WS, R, R, R, WS, PDF, WS, L, L, L, WS, PDF, WS, 
    AL, AL, AL, WS, PDF, WS, L, L, L, WS, PDF, WS, L, L, L, WS, PDF, 
    ON, PDF, BN, BN, ON, PDF
};

static const UBiDiLevel
testLevels11[]={
    0, 0, 0, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 4, 4, 5, 5, 5, 4, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static const uint8_t
testVisualMap11[]={
    0, 1, 2, 44, 43, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 31, 30, 29, 28, 27, 26, 20, 21, 24, 23, 22, 25, 19, 18, 17, 16, 15, 14, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 3, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57
};

static const uint8_t
testText12[]={
    NSM, WS, L, L, L, L, L, L, L, WS, L, L, L, L, WS, 
    R, R, R, R, R, WS, L, L, L, L, L, L, L, WS, WS, AL, 
    AL, AL, AL, WS, EN, EN, ES, EN, EN, CS, S, EN, EN, CS, WS, 
    EN, EN, WS, AL, AL, AL, AL, AL, B, L, L, L, L, L, L, 
    L, L, WS, AN, AN, CS, AN, AN, WS
};

static const UBiDiLevel
testLevels12[]={
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 1, 2, 2, 1, 0, 2, 2, 1, 1, 2, 2, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 0
};

static const uint8_t
testVisualMap12[]={
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 19, 18, 17, 16, 15, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 40, 39, 38, 37, 36, 34, 35, 33, 31, 32, 30, 41, 52, 53, 51, 50, 48, 49, 47, 46, 45, 44, 43, 42, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69
};

static const UBiDiLevel
testLevels13[]={
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 1, 2, 2, 1, 0, 2, 2, 1, 1, 2, 2, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 0
};

static const uint8_t
testVisualMap13[]={
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 19, 18, 17, 16, 15, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 40, 39, 38, 37, 36, 34, 35, 33, 31, 32, 30, 41, 52, 53, 51, 50, 48, 49, 47, 46, 45, 44, 43, 42, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69
};

static const UBiDiLevel
testLevels14[]={
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 4, 4, 3, 4, 4, 3, 2, 4, 4, 3, 3, 4, 4, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 4, 4, 4, 4, 4, 2
};

static const uint8_t
testVisualMap14[]={
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 19, 18, 17, 16, 15, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 40, 39, 38, 37, 36, 34, 35, 33, 31, 32, 30, 41, 52, 53, 51, 50, 48, 49, 47, 46, 45, 44, 43, 42, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69
};

static const UBiDiLevel
testLevels15[]={
    5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 5, 5, 5, 5, 5, 5, 5, 6, 6, 5, 6, 6, 5, 5, 6, 6, 5, 5, 6, 6, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 5, 6, 6, 6, 6, 6, 5
};

static const uint8_t
testVisualMap15[]={
    69, 68, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 55, 54, 53, 52, 51, 50, 49, 42, 43, 44, 45, 46, 47, 48, 41, 40, 39, 38, 37, 36, 35, 33, 34, 32, 30, 31, 29, 28, 26, 27, 25, 24, 22, 23, 21, 20, 19, 18, 17, 16, 15, 7, 8, 9, 10, 11, 12, 13, 14, 6, 1, 2, 3, 4, 5, 0
};

static const UBiDiLevel
testLevels16[]={
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 1, 2, 2, 1, 0, 2, 2, 1, 1, 2, 2, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 0
};

static const uint8_t
testVisualMap16[]={
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 19, 18, 17, 16, 15, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 40, 39, 38, 37, 36, 34, 35, 33, 31, 32, 30, 41, 52, 53, 51, 50, 48, 49, 47, 46, 45, 44, 43, 42, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69
};

static const uint8_t
testText13[]={
    ON, L, RLO, CS, R, WS, AN, AN, PDF, LRE, R, L, LRO, WS, BN, ON, S, LRE, LRO, B
};

static const UBiDiLevel
testLevels17[]={
    0, 0, 1, 1, 1, 1, 1, 1, 3, 3, 3, 2, 4, 4, 4, 4, 0, 0, 0, 0
};

static const uint8_t
testVisualMap17[]={
    0, 1, 15, 14, 13, 12, 11, 10, 4, 3, 2, 5, 6, 7, 8, 9, 16, 17, 18, 19
};

static const UBiDiLevel
testLevels18[]={
    0, 0, 1, 1, 1, 0
};

static const uint8_t
testVisualMap18[]={
    0, 1, 4, 3, 2, 5
};

static const uint8_t
testText14[]={
    RLO, RLO, AL, AL, WS, EN, ES, ON, WS, S, S, PDF, LRO, WS, AL, ET, RLE, ON, EN, B
};

static const UBiDiLevel
testLevels19[]={
    1
};

static const uint8_t
testVisualMap19[]={
    0
};

static const uint8_t
testText15[]={
    R, L, CS, L
};

static const UBiDiLevel
testLevels20[]={
    2
};

static const uint8_t
testText16[]={
    L, L, L, WS, L, L, L, WS, L, L, L
};

static const UBiDiLevel
testLevels21[]={
    2, 2, 2, 2, 2, 2, 2, 1
};

static const uint8_t
testVisualMap20[]={
    1, 2, 3, 4, 5, 6, 7, 0
};

static const uint8_t
testText17[]={
    R, R, R, WS, R, R, R, WS, R, R, R
};

static const UBiDiLevel
testLevels22[]={
    1, 1, 1, 1, 1, 1, 1, 0
};

static const uint8_t
testVisualMap21[]={
    6, 5, 4, 3, 2, 1, 0, 7
};

static const uint8_t
testTextXX[]={
    L
};

static const UBiDiLevel
testLevelsXX[]={
    2
};

static const uint8_t
testVisualMapXX[]={
    0
};

const BiDiTestData
tests[]={
    {testText1,  ARRAY_LENGTH(testText1),  UBIDI_DEFAULT_LTR, -1, -1,
        UBIDI_LTR, 0,
        testLevels1, testVisualMap1},
    {testText2,  ARRAY_LENGTH(testText2),  UBIDI_DEFAULT_LTR, -1, -1,
        UBIDI_RTL, 1,
        testLevels2, testVisualMap2},
    {testText3,  ARRAY_LENGTH(testText3),  UBIDI_DEFAULT_LTR, -1, -1,
        UBIDI_LTR, 0,
        testLevels3, testVisualMap3},
    {testText4,  ARRAY_LENGTH(testText4),  UBIDI_DEFAULT_LTR, -1, -1,
        UBIDI_MIXED, 0,
        testLevels4, testVisualMap4},
    {testText5,  ARRAY_LENGTH(testText5),  UBIDI_DEFAULT_LTR, -1, -1,
        UBIDI_MIXED, 1,
        testLevels5, testVisualMap5},
    {testText6,  ARRAY_LENGTH(testText6),  UBIDI_DEFAULT_LTR, -1, -1,
        UBIDI_MIXED, 1,
        testLevels6, testVisualMap6},
    {NULL,       0,                        UBIDI_DEFAULT_LTR, -1, -1,
        UBIDI_LTR, 0,
        NULL, NULL},
    {testText8,  ARRAY_LENGTH(testText8),  UBIDI_DEFAULT_LTR, -1, -1,
        UBIDI_RTL, 1,
        testLevels8, testVisualMap8},
    {testText9,  ARRAY_LENGTH(testText9),  UBIDI_DEFAULT_LTR, -1, -1,
        UBIDI_MIXED, 0,
        testLevels9, testVisualMap9},
    {testText10, ARRAY_LENGTH(testText10), UBIDI_DEFAULT_LTR, -1, -1,
        UBIDI_MIXED, 0,
        testLevels10, testVisualMap10},
    {testText11, ARRAY_LENGTH(testText11), UBIDI_DEFAULT_LTR, -1, -1,
        UBIDI_MIXED, 0,
        testLevels11, testVisualMap11},
    {testText12, ARRAY_LENGTH(testText12), UBIDI_DEFAULT_LTR, -1, -1,
        UBIDI_MIXED, 0,
        testLevels12, testVisualMap12},
    {testText12, ARRAY_LENGTH(testText12), UBIDI_DEFAULT_RTL, -1, -1,
        UBIDI_MIXED, 0,
        testLevels13, testVisualMap13},
    {testText12, ARRAY_LENGTH(testText12), 2, -1, -1,
        UBIDI_MIXED, 2,
        testLevels14, testVisualMap14},
    {testText12, ARRAY_LENGTH(testText12), 5, -1, -1,
        UBIDI_MIXED, 5,
        testLevels15, testVisualMap15},
    {testText12, ARRAY_LENGTH(testText12), UBIDI_DEFAULT_LTR, -1, -1,
        UBIDI_MIXED, 0,
        testLevels16, testVisualMap16},
    {testText13, ARRAY_LENGTH(testText13), UBIDI_DEFAULT_LTR, -1, -1,
        UBIDI_MIXED, 0,
        testLevels17, testVisualMap17},
    {testText13, ARRAY_LENGTH(testText13), UBIDI_DEFAULT_LTR, 0, 6,
        UBIDI_MIXED, 0,
        testLevels18, testVisualMap18},
    {testText14, ARRAY_LENGTH(testText14), UBIDI_DEFAULT_LTR, 13, 14,
        UBIDI_RTL, 1,
        testLevels19, testVisualMap19},
    {testText15, ARRAY_LENGTH(testText15), UBIDI_DEFAULT_LTR, 2, 3,
        UBIDI_LTR, 2,
        testLevels20, testVisualMap19},
    {testText16, ARRAY_LENGTH(testText16), UBIDI_RTL, 0, 8,
        UBIDI_MIXED, 1,
        testLevels21, testVisualMap20},
    {testText17, ARRAY_LENGTH(testText17), UBIDI_LTR, 0, 8,
        UBIDI_MIXED, 0,
        testLevels22, testVisualMap21},
    {testTextXX, ARRAY_LENGTH(testTextXX), UBIDI_RTL, -1, -1, 
        UBIDI_MIXED, 1, testLevelsXX, testVisualMapXX}
};

const int
bidiTestCount=ARRAY_LENGTH(tests);
