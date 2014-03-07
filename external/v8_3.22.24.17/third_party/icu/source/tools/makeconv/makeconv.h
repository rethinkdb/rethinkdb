/*
*******************************************************************************
*
*   Copyright (C) 2000-2010, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  makeconv.h
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2000nov01
*   created by: Markus W. Scherer
*/

#ifndef __MAKECONV_H__
#define __MAKECONV_H__

#include "unicode/utypes.h"
#include "ucnv_bld.h"
#include "unewdata.h"
#include "ucm.h"

/* exports from makeconv.c */
U_CFUNC UBool VERBOSE;
U_CFUNC UBool SMALL;
U_CFUNC UBool IGNORE_SISO_CHECK;

/* converter table type for writing */
enum {
    TABLE_NONE,
    TABLE_BASE,
    TABLE_EXT,
    TABLE_BASE_AND_EXT
};

/* abstract converter generator struct, C++ - style */
struct NewConverter;
typedef struct NewConverter NewConverter;

struct NewConverter {
    void
    (*close)(NewConverter *cnvData);

    /** is this byte sequence valid? */
    UBool
    (*isValid)(NewConverter *cnvData,
               const uint8_t *bytes, int32_t length);

    UBool
    (*addTable)(NewConverter *cnvData, UCMTable *table, UConverterStaticData *staticData);

    uint32_t
    (*write)(NewConverter *cnvData, const UConverterStaticData *staticData,
             UNewDataMemory *pData, int32_t tableType);
};

#endif /* __MAKECONV_H__ */
