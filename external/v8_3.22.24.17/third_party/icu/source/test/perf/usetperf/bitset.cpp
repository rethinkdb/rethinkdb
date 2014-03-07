/*
**********************************************************************
* Copyright (c) 2002-2005, International Business Machines
* Corporation and others.  All Rights Reserved.
**********************************************************************
* 2002-09-20 aliu Created.
*/

#include "unicode/utypes.h"
#include "cmemory.h"
#include "bitset.h"

// TODO: have a separate capacity, so the len can just be set to
// zero in the clearAll() method, and growth can be smarter.

const int32_t SLOP = 8;

const int32_t BYTES_PER_WORD = sizeof(int32_t);

BitSet::BitSet() {
    len = SLOP;
    data = (int32_t*) uprv_malloc(len * BYTES_PER_WORD);
    clearAll();
}

BitSet::~BitSet() {
    uprv_free(data);
}

UBool BitSet::get(int32_t bitIndex) const {
    uint32_t longIndex = bitIndex >> 5;
    int32_t bitInLong = bitIndex & 0x1F;
    return (longIndex < len) ? (((data[longIndex] >> bitInLong) & 1) != 0)
        : FALSE;
}

void BitSet::set(int32_t bitIndex) {
    uint32_t longIndex = bitIndex >> 5;
    int32_t bitInLong = bitIndex & 0x1F;
    if (longIndex >= len) {
        ensureCapacity(longIndex+1);
    }
    data[longIndex] |= (1 << bitInLong);
}

void BitSet::clearAll() {
    for (uint32_t i=0; i<len; ++i) data[i] = 0;
}

void BitSet::ensureCapacity(uint32_t minLen) {
    uint32_t newLen = len;
    while (newLen < minLen) newLen <<= 1; // grow exponentially
    int32_t* newData = (int32_t*) uprv_malloc(newLen * BYTES_PER_WORD);
    uprv_memcpy(newData, data, len * BYTES_PER_WORD);
    uprv_free(data);
    data = newData;
    int32_t* p = data + len;
    int32_t* limit = data + newLen;
    while (p < limit) *p++ = 0;
    len = newLen;
}

//eof
