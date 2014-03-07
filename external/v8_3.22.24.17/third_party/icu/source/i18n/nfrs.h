/*
******************************************************************************
*   Copyright (C) 1997-2009, International Business Machines
*   Corporation and others.  All Rights Reserved.
******************************************************************************
*   file name:  nfrs.h
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
* Modification history
* Date        Name      Comments
* 10/11/2001  Doug      Ported from ICU4J
*/

#ifndef NFRS_H
#define NFRS_H

#include "unicode/uobject.h"
#include "unicode/rbnf.h"

#if U_HAVE_RBNF

#include "unicode/utypes.h"
#include "unicode/umisc.h"

#include "nfrlist.h"

U_NAMESPACE_BEGIN

class NFRuleSet : public UMemory {
 public:
  NFRuleSet(UnicodeString* descriptions, int32_t index, UErrorCode& status);
  void parseRules(UnicodeString& rules, const RuleBasedNumberFormat* owner, UErrorCode& status);
  void makeIntoFractionRuleSet() { fIsFractionRuleSet = TRUE; }

  ~NFRuleSet();

  UBool operator==(const NFRuleSet& rhs) const;
  UBool operator!=(const NFRuleSet& rhs) const { return !operator==(rhs); }

  UBool isPublic() const { return fIsPublic; }

  UBool isParseable() const { 
      UnicodeString prefixpart = UNICODE_STRING_SIMPLE("-prefixpart");
      UnicodeString postfix = UNICODE_STRING_SIMPLE("-postfix");
      UnicodeString postfx = UNICODE_STRING_SIMPLE("-postfx");

      return ( name.indexOf(prefixpart) == -1 && name.indexOf(postfix) == -1 && name.indexOf(postfx) == -1 );
  }

  UBool isFractionRuleSet() const { return fIsFractionRuleSet; }

  void  getName(UnicodeString& result) const { result.setTo(name); }
  UBool isNamed(const UnicodeString& _name) const { return this->name == _name; }

  void  format(int64_t number, UnicodeString& toAppendTo, int32_t pos) const;
  void  format(double number, UnicodeString& toAppendTo, int32_t pos) const;

  UBool parse(const UnicodeString& text, ParsePosition& pos, double upperBound, Formattable& result) const;

  void appendRules(UnicodeString& result) const; // toString

 private:
  NFRule * findNormalRule(int64_t number) const;
  NFRule * findDoubleRule(double number) const;
  NFRule * findFractionRuleSetRule(double number) const;

 private:
  UnicodeString name;
  NFRuleList rules;
  NFRule *negativeNumberRule;
  NFRule *fractionRules[3];
  UBool fIsFractionRuleSet;
  UBool fIsPublic;
  int32_t fRecursionCount;

  NFRuleSet(const NFRuleSet &other); // forbid copying of this class
  NFRuleSet &operator=(const NFRuleSet &other); // forbid copying of this class
};

// utilities from old llong.h
// convert mantissa portion of double to int64
int64_t util64_fromDouble(double d);

// raise radix to the power exponent, only non-negative exponents
int64_t util64_pow(int32_t radix, uint32_t exponent);

// convert n to digit string in buffer, return length of string
uint32_t util64_tou(int64_t n, UChar* buffer, uint32_t buflen, uint32_t radix = 10, UBool raw = FALSE);

#ifdef RBNF_DEBUG
int64_t util64_utoi(const UChar* str, uint32_t radix = 10);
uint32_t util64_toa(int64_t n, char* buffer, uint32_t buflen, uint32_t radix = 10, UBool raw = FALSE);
int64_t util64_atoi(const char* str, uint32_t radix);
#endif


U_NAMESPACE_END

/* U_HAVE_RBNF */
#endif

// NFRS_H
#endif

