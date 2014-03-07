/*
*******************************************************************************
*   Copyright (C) 2010, International Business Machines
*   Corporation and others.  All Rights Reserved.
*******************************************************************************
*   file name:  charstr.cpp
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2010may19
*   created by: Markus W. Scherer
*/

#include "unicode/utypes.h"
#include "charstr.h"
#include "cmemory.h"
#include "cstring.h"

U_NAMESPACE_BEGIN

CharString &CharString::copyFrom(const CharString &s, UErrorCode &errorCode) {
    if(U_SUCCESS(errorCode) && this!=&s && ensureCapacity(s.len+1, 0, errorCode)) {
        len=s.len;
        uprv_memcpy(buffer.getAlias(), s.buffer.getAlias(), len+1);
    }
    return *this;
}

CharString &CharString::truncate(int32_t newLength) {
    if(newLength<0) {
        newLength=0;
    }
    if(newLength<len) {
        buffer[len=newLength]=0;
    }
    return *this;
}

CharString &CharString::append(char c, UErrorCode &errorCode) {
    if(ensureCapacity(len+2, 0, errorCode)) {
        buffer[len++]=c;
        buffer[len]=0;
    }
    return *this;
}

CharString &CharString::append(const char *s, int32_t sLength, UErrorCode &errorCode) {
    if(U_FAILURE(errorCode)) {
        return *this;
    }
    if(sLength<-1 || (s==NULL && sLength!=0)) {
        errorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return *this;
    }
    if(sLength<0) {
        sLength=uprv_strlen(s);
    }
    if(sLength>0) {
        if(s==(buffer.getAlias()+len)) {
            // The caller wrote into the getAppendBuffer().
            if(sLength>=(buffer.getCapacity()-len)) {
                // The caller wrote too much.
                errorCode=U_INTERNAL_PROGRAM_ERROR;
            } else {
                buffer[len+=sLength]=0;
            }
        } else if(buffer.getAlias()<=s && s<(buffer.getAlias()+len) &&
                  sLength>=(buffer.getCapacity()-len)
        ) {
            // (Part of) this string is appended to itself which requires reallocation,
            // so we have to make a copy of the substring and append that.
            return append(CharString(s, sLength, errorCode), errorCode);
        } else if(ensureCapacity(len+sLength+1, 0, errorCode)) {
            uprv_memcpy(buffer.getAlias()+len, s, sLength);
            buffer[len+=sLength]=0;
        }
    }
    return *this;
}

char *CharString::getAppendBuffer(int32_t minCapacity,
                                  int32_t desiredCapacityHint,
                                  int32_t &resultCapacity,
                                  UErrorCode &errorCode) {
    if(U_FAILURE(errorCode)) {
        resultCapacity=0;
        return NULL;
    }
    int32_t appendCapacity=buffer.getCapacity()-len-1;  // -1 for NUL
    if(appendCapacity>=minCapacity) {
        resultCapacity=appendCapacity;
        return buffer.getAlias()+len;
    }
    if(ensureCapacity(len+minCapacity+1, len+desiredCapacityHint+1, errorCode)) {
        resultCapacity=buffer.getCapacity()-len-1;
        return buffer.getAlias()+len;
    }
    resultCapacity=0;
    return NULL;
}

CharString &CharString::appendInvariantChars(const UnicodeString &s, UErrorCode &errorCode) {
    if(ensureCapacity(len+s.length()+1, 0, errorCode)) {
        len+=s.extract(0, 0x7fffffff, buffer.getAlias()+len, buffer.getCapacity()-len, US_INV);
    }
    return *this;
}

UBool CharString::ensureCapacity(int32_t capacity,
                                 int32_t desiredCapacityHint,
                                 UErrorCode &errorCode) {
    if(U_FAILURE(errorCode)) {
        return FALSE;
    }
    if(capacity>buffer.getCapacity()) {
        if(desiredCapacityHint==0) {
            desiredCapacityHint=capacity+buffer.getCapacity();
        }
        if( (desiredCapacityHint<=capacity || buffer.resize(desiredCapacityHint, len+1)==NULL) &&
            buffer.resize(capacity, len+1)==NULL
        ) {
            errorCode=U_MEMORY_ALLOCATION_ERROR;
            return FALSE;
        }
    }
    return TRUE;
}

U_NAMESPACE_END
