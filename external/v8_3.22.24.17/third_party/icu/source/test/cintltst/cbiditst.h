/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2007, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/
/*   file name:  cbiditst.h
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 1999sep22
*   created by: Markus W. Scherer
*/

#ifndef CBIDITST_H
#define CBIDITST_H

#include "unicode/utypes.h"
#include "unicode/uchar.h"
#include "unicode/ubidi.h"

#ifdef XP_CPLUSPLUS
extern "C" {
#endif

#define MAX_STRING_LENGTH 200

/* length of an array */
#define ARRAY_LENGTH(array) (sizeof(array)/sizeof(array[0]))

/*  Comparing the description of the BiDi algorithm with this implementation
    is easier with the same names for the BiDi types in the code as there.
    See UCharDirection in uchar.h .
*/
#define L   U_LEFT_TO_RIGHT
#define R   U_RIGHT_TO_LEFT
#define EN  U_EUROPEAN_NUMBER
#define ES  U_EUROPEAN_NUMBER_SEPARATOR
#define ET  U_EUROPEAN_NUMBER_TERMINATOR
#define AN  U_ARABIC_NUMBER
#define CS  U_COMMON_NUMBER_SEPARATOR
#define B   U_BLOCK_SEPARATOR
#define S   U_SEGMENT_SEPARATOR
#define WS  U_WHITE_SPACE_NEUTRAL
#define ON  U_OTHER_NEUTRAL
#define LRE U_LEFT_TO_RIGHT_EMBEDDING
#define LRO U_LEFT_TO_RIGHT_OVERRIDE
#define AL  U_RIGHT_TO_LEFT_ARABIC
#define RLE U_RIGHT_TO_LEFT_EMBEDDING
#define RLO U_RIGHT_TO_LEFT_OVERRIDE
#define PDF U_POP_DIRECTIONAL_FORMAT
#define NSM U_DIR_NON_SPACING_MARK
#define BN  U_BOUNDARY_NEUTRAL

extern const char * const
dirPropNames[U_CHAR_DIRECTION_COUNT];

extern UChar
charFromDirProp[U_CHAR_DIRECTION_COUNT];

typedef struct {
    const uint8_t *text;
    int32_t length;
    UBiDiLevel paraLevel;
    int32_t lineStart, lineLimit;
    UBiDiDirection direction;
    UBiDiLevel resultLevel;
    const UBiDiLevel *levels;
    const uint8_t *visualMap;
} BiDiTestData;

extern const BiDiTestData
tests[];

extern const int
bidiTestCount;

#ifdef XP_CPLUSPLUS
}
#endif

#endif
