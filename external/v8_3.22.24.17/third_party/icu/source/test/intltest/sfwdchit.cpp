/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2003, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/
/*   file name:  sfwdchit.cpp
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*/

#include "sfwdchit.h"
#include "unicode/ustring.h"
#include "unicode/unistr.h"
#include "uhash.h"
#include "cmemory.h"

// A hash code of kInvalidHashCode indicates that the has code needs
// to be computed. A hash code of kEmptyHashCode is used for empty keys
// and for any key whose computed hash code is kInvalidHashCode.
const int32_t SimpleFwdCharIterator::kInvalidHashCode = 0;
const int32_t SimpleFwdCharIterator::kEmptyHashCode = 1;

#if 0 // not used
SimpleFwdCharIterator::SimpleFwdCharIterator(const UnicodeString& s) {

    fHashCode = kInvalidHashCode;
    fLen = s.length();
    fStart = new UChar[fLen];
    if(fStart == NULL) {
        fBogus = TRUE;
    } else {
        fEnd = fStart+fLen;
        fCurrent = fStart;
        fBogus = FALSE;
        s.extract(0, fLen, fStart);          
    }
    
}
#endif

SimpleFwdCharIterator::SimpleFwdCharIterator(UChar *s, int32_t len, UBool adopt) {

    fHashCode = kInvalidHashCode;

    fLen = len==-1 ? u_strlen(s) : len;

    if(adopt == FALSE) {
        fStart = new UChar[fLen];
        if(fStart == NULL) {
            fBogus = TRUE;
        } else {
            uprv_memcpy(fStart, s, fLen);
            fEnd = fStart+fLen;
            fCurrent = fStart;
            fBogus = FALSE;
        }
    } else { // adopt = TRUE
        fCurrent = fStart = s;
        fEnd = fStart + fLen;
        fBogus = FALSE;
    }

}

SimpleFwdCharIterator::~SimpleFwdCharIterator() {
    delete[] fStart;
}

#if 0 // not used
UBool SimpleFwdCharIterator::operator==(const ForwardCharacterIterator& that) const {
    if(this == &that) {
        return TRUE;
    }
/*
    if(that->fHashCode != kInvalidHashCode && this->fHashCode = that->fHashCode) {
        return TRUE;
    }

    if(this->fStart == that->fStart) {
        return TRUE;
    }

    if(this->fLen == that->fLen && uprv_memcmp(this->fStart, that->fStart, this->fLen) {
        return TRUE;
    }
*/
    return FALSE;
}
#endif

int32_t SimpleFwdCharIterator::hashCode(void) const {
    if (fHashCode == kInvalidHashCode)
    {
        UHashTok key;
        key.pointer = fStart;
        ((SimpleFwdCharIterator *)this)->fHashCode = uhash_hashUChars(key);
    }
    return fHashCode;
}
        
UClassID SimpleFwdCharIterator::getDynamicClassID(void) const {
    return NULL;
}

UChar SimpleFwdCharIterator::nextPostInc(void) {
    if(fCurrent == fEnd) {
        return ForwardCharacterIterator::DONE;
    } else {
        return *(fCurrent)++;
    }
}
        
UChar32 SimpleFwdCharIterator::next32PostInc(void) {
    return ForwardCharacterIterator::DONE;
}
        
UBool SimpleFwdCharIterator::hasNext() {
    return fCurrent < fEnd;
}
