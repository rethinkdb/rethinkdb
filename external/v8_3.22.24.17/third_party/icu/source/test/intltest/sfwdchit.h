/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2003, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

#ifndef SFDWCHIT_H
#define SFDWCHIT_H

#include "unicode/chariter.h"
#include "intltest.h"

class SimpleFwdCharIterator : public ForwardCharacterIterator {
public:
    // not used -- SimpleFwdCharIterator(const UnicodeString& s);
    SimpleFwdCharIterator(UChar *s, int32_t len, UBool adopt = FALSE);

    virtual ~SimpleFwdCharIterator();

  /**
   * Returns true when both iterators refer to the same
   * character in the same character-storage object.  
   */
  // not used -- virtual UBool operator==(const ForwardCharacterIterator& that) const;
        
  /**
   * Generates a hash code for this iterator.  
   */
  virtual int32_t hashCode(void) const;
        
  /**
   * Returns a UClassID for this ForwardCharacterIterator ("poor man's
   * RTTI").<P> Despite the fact that this function is public,
   * DO NOT CONSIDER IT PART OF CHARACTERITERATOR'S API!  
   */
  virtual UClassID getDynamicClassID(void) const;

  /**
   * Gets the current code unit for returning and advances to the next code unit
   * in the iteration range
   * (toward endIndex()).  If there are
   * no more code units to return, returns DONE.
   */
  virtual UChar         nextPostInc(void);
        
  /**
   * Gets the current code point for returning and advances to the next code point
   * in the iteration range
   * (toward endIndex()).  If there are
   * no more code points to return, returns DONE.
   */
  virtual UChar32       next32PostInc(void);
        
  /**
   * Returns FALSE if there are no more code units or code points
   * at or after the current position in the iteration range.
   * This is used with nextPostInc() or next32PostInc() in forward
   * iteration.
   */
  virtual UBool        hasNext();

protected:
    SimpleFwdCharIterator() {}
    SimpleFwdCharIterator(const SimpleFwdCharIterator &other)
        : ForwardCharacterIterator(other) {}
    SimpleFwdCharIterator &operator=(const SimpleFwdCharIterator&) { return *this; }
private:
    static const int32_t            kInvalidHashCode;
    static const int32_t            kEmptyHashCode;

    UChar *fStart, *fEnd, *fCurrent;
    int32_t fLen;
    UBool fBogus;
    int32_t fHashCode;
};

#endif
