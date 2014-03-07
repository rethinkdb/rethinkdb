/*
 * Copyright (C) 2008-2009, International Business Machines Corporation and Others.
 * All rights reserved.
 */

#include "unicode/utypes.h"
#include "cmemory.h"
#include "unicode/bms.h"
#include "unicode/unistr.h"
#include "unicode/colldata.h"
#include "unicode/bmsearch.h"


#if !UCONFIG_NO_COLLATION && !UCONFIG_NO_BREAK_ITERATION


//#define USE_SAFE_CASTS
#ifdef USE_SAFE_CASTS
#define STATIC_CAST(type,value) static_cast<type>(value)
#define CONST_CAST(type,value) const_cast<type>(value)
#else
#define STATIC_CAST(type,value) (type) (value)
#define CONST_CAST(type,value) (type) (value)
#endif

U_NAMESPACE_USE

U_CAPI UCD * U_EXPORT2
ucd_open(UCollator *coll, UErrorCode *status)
{
    return STATIC_CAST(UCD *, CollData::open(coll, *status));
}

U_CAPI void U_EXPORT2
ucd_close(UCD *ucd)
{
    CollData *data = STATIC_CAST(CollData *, ucd);

    CollData::close(data);
}

U_CAPI UCollator * U_EXPORT2
ucd_getCollator(UCD *ucd)
{
    CollData *data = STATIC_CAST(CollData *, ucd);

    return data->getCollator();
}

U_CAPI void U_EXPORT2
ucd_freeCache()
{
    CollData::freeCollDataCache();
}

U_CAPI void U_EXPORT2
ucd_flushCache()
{
    CollData::flushCollDataCache();
}

struct BMS
{
    BoyerMooreSearch *bms;
    const UnicodeString *targetString;
};

U_CAPI BMS * U_EXPORT2
bms_open(UCD *ucd,
         const UChar *pattern, int32_t patternLength,
         const UChar *target,  int32_t targetLength,
         UErrorCode  *status)
{
    BMS *bms = STATIC_CAST(BMS *, uprv_malloc(sizeof(BMS)));

    if (bms == NULL) {
        *status = U_MEMORY_ALLOCATION_ERROR;
        return NULL;
    }

    CollData *data = (CollData *) ucd;
    UnicodeString patternString(pattern, patternLength);

    if (target != NULL) {
        bms->targetString = new UnicodeString(target, targetLength);
        
        if (bms->targetString == NULL) {
            bms->bms = NULL;
            *status = U_MEMORY_ALLOCATION_ERROR;
            return bms;
        }
    } else {
        bms->targetString = NULL;
    }

    bms->bms = new BoyerMooreSearch(data, patternString, bms->targetString, *status);

    if (bms->bms == NULL) {
        *status = U_MEMORY_ALLOCATION_ERROR;
    }

    return bms;
}

U_CAPI void U_EXPORT2
bms_close(BMS *bms)
{
    delete bms->bms;

    delete bms->targetString;

    uprv_free(bms);
}

U_CAPI UBool U_EXPORT2
bms_empty(BMS *bms)
{
    return bms->bms->empty();
}

U_CAPI UCD * U_EXPORT2
bms_getData(BMS *bms)
{
    return STATIC_CAST(UCD *, bms->bms->getData());
}

U_CAPI UBool U_EXPORT2
bms_search(BMS *bms, int32_t offset, int32_t *start, int32_t *end)
{
    return bms->bms->search(offset, *start, *end);
}

U_CAPI void U_EXPORT2
bms_setTargetString(BMS *bms, const UChar *target, int32_t targetLength, UErrorCode *status)
{
    if (U_FAILURE(*status)) {
        return;
    }

    if (bms->targetString != NULL) {
        delete bms->targetString;
    }

    if (target != NULL) {
        bms->targetString = new UnicodeString(target, targetLength);
    } else {
        bms->targetString = NULL;
    }

    bms->bms->setTargetString(bms->targetString, *status);
}

#endif
