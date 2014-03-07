/*
*******************************************************************************
* Copyright (C) 2007-2008, International Business Machines Corporation and
* others. All Rights Reserved.
*******************************************************************************
*
* File PLURRULE_IMPL.H
*
*******************************************************************************
*/


#ifndef PLURRULE_IMPLE
#define PLURRULE_IMPLE

/**
 * \file
 * \brief C++ API: Defines rules for mapping positive long values onto a small set of keywords.
 */
 
#if !UCONFIG_NO_FORMATTING

#include "unicode/format.h"
#include "unicode/locid.h"
#include "unicode/parseerr.h"
#include "unicode/utypes.h"
#include "uvector.h"
#include "hash.h"

U_NAMESPACE_BEGIN

#define DOT               ((UChar)0x002E)
#define SINGLE_QUOTE      ((UChar)0x0027)
#define SLASH             ((UChar)0x002F)
#define BACKSLASH         ((UChar)0x005C)
#define SPACE             ((UChar)0x0020)
#define QUOTATION_MARK    ((UChar)0x0022)
#define NUMBER_SIGN       ((UChar)0x0023)
#define ASTERISK          ((UChar)0x002A)
#define COMMA             ((UChar)0x002C)
#define HYPHEN            ((UChar)0x002D)
#define U_ZERO            ((UChar)0x0030)
#define U_ONE             ((UChar)0x0031)
#define U_TWO             ((UChar)0x0032)
#define U_THREE           ((UChar)0x0033)
#define U_FOUR            ((UChar)0x0034)
#define U_FIVE            ((UChar)0x0035)
#define U_SIX             ((UChar)0x0036)
#define U_SEVEN           ((UChar)0x0037)
#define U_EIGHT           ((UChar)0x0038)
#define U_NINE            ((UChar)0x0039)
#define COLON             ((UChar)0x003A)
#define SEMI_COLON        ((UChar)0x003B)
#define CAP_A             ((UChar)0x0041)
#define CAP_B             ((UChar)0x0042)
#define CAP_R             ((UChar)0x0052)
#define CAP_Z             ((UChar)0x005A)
#define LOWLINE           ((UChar)0x005F)
#define LEFTBRACE         ((UChar)0x007B)
#define RIGHTBRACE        ((UChar)0x007D)

#define LOW_A             ((UChar)0x0061)
#define LOW_B             ((UChar)0x0062)
#define LOW_C             ((UChar)0x0063)
#define LOW_D             ((UChar)0x0064)
#define LOW_E             ((UChar)0x0065)
#define LOW_F             ((UChar)0x0066)
#define LOW_G             ((UChar)0x0067)
#define LOW_H             ((UChar)0x0068)
#define LOW_I             ((UChar)0x0069)
#define LOW_J             ((UChar)0x006a)
#define LOW_K             ((UChar)0x006B)
#define LOW_L             ((UChar)0x006C)
#define LOW_M             ((UChar)0x006D)
#define LOW_N             ((UChar)0x006E)
#define LOW_O             ((UChar)0x006F)
#define LOW_P             ((UChar)0x0070)
#define LOW_Q             ((UChar)0x0071)
#define LOW_R             ((UChar)0x0072)
#define LOW_S             ((UChar)0x0073)
#define LOW_T             ((UChar)0x0074)
#define LOW_U             ((UChar)0x0075)
#define LOW_V             ((UChar)0x0076)
#define LOW_W             ((UChar)0x0077)
#define LOW_Y             ((UChar)0x0079)
#define LOW_Z             ((UChar)0x007A)


#define PLURAL_RANGE_HIGH  0x7fffffff;


class UnicodeSet;

typedef enum PluralKey {
  pZero,
  pOne,
  pTwo,
  pFew,
  pMany,
  pOther,
  pLast
}PluralKey;

typedef enum tokenType {
  none,
  tLetter,
  tNumber,
  tComma,
  tSemiColon,
  tSpace,
  tColon,
  tDot,
  tKeyword,
  tZero,
  tOne,
  tTwo,
  tFew,
  tMany,
  tOther,
  tAnd,
  tOr,
  tMod,
  tNot,
  tIn,
  tWithin,
  tNotIn,
  tVariableN,
  tIs,
  tLeftBrace,
  tRightBrace
}tokenType;

class RuleParser : public UMemory {
public:
    RuleParser();
    virtual ~RuleParser();
    void getNextToken(const UnicodeString& ruleData, int32_t *ruleIndex, UnicodeString& token, 
                            tokenType& type, UErrorCode &status);
    void checkSyntax(tokenType prevType, tokenType curType, UErrorCode &status);
private:
    UnicodeSet      *idStartFilter;
    UnicodeSet      *idContinueFilter;
    
    void getKeyType(const UnicodeString& token, tokenType& type, UErrorCode &status);
    UBool inRange(UChar ch, tokenType& type);
    UBool isValidKeyword(const UnicodeString& token);
};

class AndConstraint : public UMemory  {
public:
    typedef enum RuleOp {
        NONE,
        MOD
    } RuleOp;
    RuleOp  op;
    int32_t opNum;
    int32_t  rangeLow;
    int32_t  rangeHigh;
    UBool   notIn;
    UBool   integerOnly;
    AndConstraint *next;
    
    AndConstraint();
    AndConstraint(const AndConstraint& other);
    virtual ~AndConstraint();
    AndConstraint* add();
    UBool isFulfilled(double number);
    int32_t updateRepeatLimit(int32_t maxLimit);
};

class OrConstraint : public UMemory  {
public:
    AndConstraint *childNode;
    OrConstraint *next;
    OrConstraint();
    
    OrConstraint(const OrConstraint& other);
    virtual ~OrConstraint();
    AndConstraint* add();
    UBool isFulfilled(double number);
};

class RuleChain : public UMemory  {
public:
    OrConstraint *ruleHeader;
    UnicodeString keyword;
    RuleChain();
    RuleChain(const RuleChain& other);
    RuleChain *next;
    
    virtual ~RuleChain();
    UnicodeString select(double number) const;
    void dumpRules(UnicodeString& result);
    int32_t getRepeatLimit();  
    UErrorCode getKeywords(int32_t maxArraySize, UnicodeString *keywords, int32_t& arraySize) const;
    UBool isKeyword(const UnicodeString& keyword) const;
    void setRepeatLimit();
private:
    int32_t repeatLimit;
};

class PluralKeywordEnumeration : public StringEnumeration {
public:
    PluralKeywordEnumeration(RuleChain *header, UErrorCode& status);
    virtual ~PluralKeywordEnumeration();
    static UClassID U_EXPORT2 getStaticClassID(void);
    virtual UClassID getDynamicClassID(void) const;
    virtual const UnicodeString* snext(UErrorCode& status);
    virtual void reset(UErrorCode& status);
    virtual int32_t count(UErrorCode& status) const;
private:
    int32_t pos;
    UVector fKeywordNames;
};

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */

#endif // _PLURRULE_IMPL
//eof
