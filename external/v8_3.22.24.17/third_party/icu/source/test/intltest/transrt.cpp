/*
**********************************************************************
*   Copyright (C) 2000-2010, International Business Machines
*   Corporation and others.  All Rights Reserved.
**********************************************************************
*   Date        Name        Description
*   05/23/00    aliu        Creation.
**********************************************************************
*/

#include "unicode/utypes.h"

#if !UCONFIG_NO_TRANSLITERATION

#include "unicode/translit.h"
#include "rbt.h"
#include "unicode/calendar.h"
#include "unicode/uniset.h"
#include "unicode/uchar.h"
#include "unicode/normlzr.h"
#include "unicode/uchar.h"
#include "unicode/parseerr.h"
#include "unicode/usetiter.h"
#include "unicode/putil.h"
#include "unicode/uversion.h"
#include "unicode/locid.h"
#include "unicode/ulocdata.h"
#include "unicode/utf8.h"
#include "putilimp.h"
#include "cmemory.h"
#include "transrt.h"
#include "testutil.h"
#include <string.h>
#include <stdio.h>

#define CASE(id,test) case id:                          \
                          name = #test;                 \
                          if (exec) {                   \
                              logln(#test "---");       \
                              logln((UnicodeString)""); \
                              UDate t = uprv_getUTCtime(); \
                              test();                   \
                              t = uprv_getUTCtime() - t; \
                              logln((UnicodeString)#test " took " + t/U_MILLIS_PER_DAY + " seconds"); \
                          }                             \
                          break

#define EXHAUSTIVE(id,test) case id:                            \
                              if(quick==FALSE){                 \
                                  name = #test;                 \
                                  if (exec){                    \
                                      logln(#test "---");       \
                                      logln((UnicodeString)""); \
                                      test();                   \
                                  }                             \
                              }else{                            \
                                name="";                        \
                              }                                 \
                              break
void
TransliteratorRoundTripTest::runIndexedTest(int32_t index, UBool exec,
                                   const char* &name, char* /*par*/) {
    switch (index) {
        CASE(0, TestCyrillic);
        // CASE(0,TestKana);
        CASE(1,TestHiragana);
        CASE(2,TestKatakana);
        CASE(3,TestJamo);
        CASE(4,TestHangul);
        CASE(5,TestGreek);
        CASE(6,TestGreekUNGEGN);
        CASE(7,Testel);
        CASE(8,TestDevanagariLatin);
        CASE(9,TestInterIndic);
        CASE(10, TestHebrew);
        CASE(11, TestArabic);
        CASE(12, TestHan);
        default: name = ""; break;
    }
}


//--------------------------------------------------------------------
// TransliteratorPointer
//--------------------------------------------------------------------

/**
 * A transliterator pointer wrapper that deletes the contained
 * pointer automatically when the wrapper goes out of scope.
 * Sometimes called a "janitor" or "smart pointer".
 */
class TransliteratorPointer {
    Transliterator* t;
    // disallowed:
    TransliteratorPointer(const TransliteratorPointer& rhs);
    TransliteratorPointer& operator=(const TransliteratorPointer& rhs);
public:
    TransliteratorPointer(Transliterator* adopted) {
        t = adopted;
    }
    ~TransliteratorPointer() {
        delete t;
    }
    inline Transliterator* operator->() { return t; }
    inline operator const Transliterator*() const { return t; }
    inline operator Transliterator*() { return t; }
};

//--------------------------------------------------------------------
// Legal
//--------------------------------------------------------------------

class Legal {
public:
    Legal() {}
    virtual ~Legal() {}
    virtual UBool is(const UnicodeString& /*sourceString*/) const {return TRUE;}
};

class LegalJamo : public Legal {
    // any initial must be followed by a medial (or initial)
    // any medial must follow an initial (or medial)
    // any final must follow a medial (or final)
public:
    LegalJamo() {}
    virtual ~LegalJamo() {}
    virtual UBool is(const UnicodeString& sourceString) const;
            int   getType(UChar c) const;
};

UBool LegalJamo::is(const UnicodeString& sourceString) const {
    int t;
    UnicodeString decomp;
    UErrorCode ec = U_ZERO_ERROR;
    Normalizer::decompose(sourceString, FALSE, 0, decomp, ec); 
    if (U_FAILURE(ec)) {
        return FALSE;
    }      
    for (int i = 0; i < decomp.length(); ++i) { // don't worry about surrogates             
        switch (getType(decomp.charAt(i))) {
        case 0: t = getType(decomp.charAt(i+1));
                if (t != 0 && t != 1) { return FALSE; }
                break;
        case 1: t = getType(decomp.charAt(i-1));
                if (t != 0 && t != 1) { return FALSE; }
                break;
        case 2: t = getType(decomp.charAt(i-1));
                if (t != 1 && t != 2) { return FALSE; }
                break;
        }
    }              
    return TRUE;
}

int LegalJamo::getType(UChar c) const {
    if (0x1100 <= c && c <= 0x1112) 
        return 0;
    else if (0x1161 <= c && c  <= 0x1175) 
             return 1;
         else if (0x11A8 <= c && c  <= 0x11C2) 
                  return 2;
    return -1; // other
}

class LegalGreek : public Legal {
    UBool full;
public:
    LegalGreek(UBool _full) { full = _full; }
    virtual ~LegalGreek() {}

    virtual UBool is(const UnicodeString& sourceString) const;

    static UBool isVowel(UChar c);
    
    static UBool isRho(UChar c);
};

UBool LegalGreek::is(const UnicodeString& sourceString) const { 
    UnicodeString decomp;
    UErrorCode ec = U_ZERO_ERROR;
    Normalizer::decompose(sourceString, FALSE, 0, decomp, ec);
                
    // modern is simpler: don't care about anything but a grave
    if (full == FALSE) {
        // A special case which is legal but should be
        // excluded from round trip
        // if (sourceString == UnicodeString("\\u039C\\u03C0", "")) {
        //    return FALSE;
        // }       
        for (int32_t i = 0; i < decomp.length(); ++i) {
            UChar c = decomp.charAt(i);
            // exclude all the accents
            if (c == 0x0313 || c == 0x0314 || c == 0x0300 || c == 0x0302
                || c == 0x0342 || c == 0x0345
                ) return FALSE;
        }
        return TRUE;
    }

    // Legal greek has breathing marks IFF there is a vowel or RHO at the start
    // IF it has them, it has exactly one.
    // IF it starts with a RHO, then the breathing mark must come before the second letter.
    // Since there are no surrogates in greek, don't worry about them
    UBool firstIsVowel = FALSE;
    UBool firstIsRho = FALSE;
    UBool noLetterYet = TRUE;
    int32_t breathingCount = 0;
    int32_t letterCount = 0;
    for (int32_t i = 0; i < decomp.length(); ++i) {
        UChar c = decomp.charAt(i);
        if (u_isalpha(c)) {
            ++letterCount;
            if (noLetterYet) {
                noLetterYet =  FALSE;
                firstIsVowel = isVowel(c);
                firstIsRho = isRho(c);
            }
            if (firstIsRho && letterCount == 2 && breathingCount == 0) {
                return FALSE;
            }
        }
        if (c == 0x0313 || c == 0x0314) {
            ++breathingCount;
        }
    }
    
    if (firstIsVowel || firstIsRho) return breathingCount == 1;
    return breathingCount == 0;
}

UBool LegalGreek::isVowel(UChar c) {
    switch (c) {
    case 0x03B1:
    case 0x03B5:
    case 0x03B7:
    case 0x03B9:
    case 0x03BF:
    case 0x03C5:
    case 0x03C9:
    case 0x0391:
    case 0x0395:
    case 0x0397:
    case 0x0399:
    case 0x039F:
    case 0x03A5:
    case 0x03A9:
        return TRUE;
    }
    return FALSE;
}

UBool LegalGreek::isRho(UChar c) {
    switch (c) {
    case 0x03C1:
    case 0x03A1:
        return TRUE;
    }
    return FALSE;
}

// AbbreviatedUnicodeSetIterator Interface ---------------------------------------------
//
//      Iterate over a UnicodeSet, only returning a sampling of the contained code points.
//        density is the approximate total number of code points to returned for the entire set.
//

class AbbreviatedUnicodeSetIterator : public UnicodeSetIterator {
public :

    AbbreviatedUnicodeSetIterator();
    virtual ~AbbreviatedUnicodeSetIterator();
    void reset(UnicodeSet& set, UBool abb = FALSE, int32_t density = 100);

    /**
     * ICU "poor man's RTTI", returns a UClassID for this class.
     */
    static inline UClassID getStaticClassID() { return (UClassID)&fgClassID; }

    /**
     * ICU "poor man's RTTI", returns a UClassID for the actual class.
     */
    virtual inline UClassID getDynamicClassID() const { return getStaticClassID(); }

private :
    UBool abbreviated;
    int32_t perRange;           // The maximum number of code points to be returned from each range
    virtual void loadRange(int32_t range);

    /**
     * The address of this static class variable serves as this class's ID
     * for ICU "poor man's RTTI".
     */
    static const char fgClassID;
};

// AbbreviatedUnicodeSetIterator Implementation ---------------------------------------

const char AbbreviatedUnicodeSetIterator::fgClassID=0;

AbbreviatedUnicodeSetIterator::AbbreviatedUnicodeSetIterator() :
    UnicodeSetIterator(), abbreviated(FALSE) {
}

AbbreviatedUnicodeSetIterator::~AbbreviatedUnicodeSetIterator() {
}
        
void AbbreviatedUnicodeSetIterator::reset(UnicodeSet& newSet, UBool abb, int32_t density) {
    UnicodeSetIterator::reset(newSet);
    abbreviated = abb;
    perRange = newSet.getRangeCount();
    if (perRange != 0) {
        perRange = density / perRange;
    }
}

void AbbreviatedUnicodeSetIterator::loadRange(int32_t myRange) {
    UnicodeSetIterator::loadRange(myRange);
    if (abbreviated && (endElement > nextElement + perRange)) {
        endElement = nextElement + perRange;
    }
}

//--------------------------------------------------------------------
// RTTest Interface
//--------------------------------------------------------------------

class RTTest : public IntlTest {

    // PrintWriter out;

    UnicodeString transliteratorID; 
    int32_t errorLimit;
    int32_t errorCount;
    int32_t pairLimit;
    UnicodeSet sourceRange;
    UnicodeSet targetRange;
    UnicodeSet toSource;
    UnicodeSet toTarget;
    UnicodeSet roundtripExclusionsSet;
    IntlTest* parent;
    Legal* legalSource; // NOT owned
    UnicodeSet badCharacters;

public:

    /*
     * create a test for the given script transliterator.
     */
    RTTest(const UnicodeString& transliteratorIDStr);

    virtual ~RTTest();

    void setErrorLimit(int32_t limit);

    void setPairLimit(int32_t limit);

    void test(const UnicodeString& sourceRange,
              const UnicodeString& targetRange,
              const char* roundtripExclusions,
              IntlTest* parent,
              UBool     quick,
              Legal* adoptedLegal,
              int32_t density = 100);

private:

    // Added to do better equality check.
        
    static UBool isSame(const UnicodeString& a, const UnicodeString& b);
             
    static UBool isCamel(const UnicodeString& a);

    UBool checkIrrelevants(Transliterator *t, const UnicodeString& irrelevants);

    void test2(UBool quick, int32_t density);

    void logWrongScript(const UnicodeString& label,
                        const UnicodeString& from,
                        const UnicodeString& to);
   
    void logNotCanonical(const UnicodeString& label, 
                         const UnicodeString& from, 
                         const UnicodeString& to,
                         const UnicodeString& fromCan,
                         const UnicodeString& toCan);

    void logFails(const UnicodeString& label);

    void logToRulesFails(const UnicodeString& label, 
                         const UnicodeString& from, 
                         const UnicodeString& to, 
                         const UnicodeString& toCan);

    void logRoundTripFailure(const UnicodeString& from,
                             const UnicodeString& toID,
                             const UnicodeString& to,
                             const UnicodeString& backID,
                             const UnicodeString& back);
};

//--------------------------------------------------------------------
// RTTest Implementation
//--------------------------------------------------------------------

/*
 * create a test for the given script transliterator.
 */
RTTest::RTTest(const UnicodeString& transliteratorIDStr) {
    transliteratorID = transliteratorIDStr;
    errorLimit = 500;
    errorCount = 0;
    pairLimit  = 0x10000;
}

RTTest::~RTTest() {
}

void RTTest::setErrorLimit(int32_t limit) {
    errorLimit = limit;
}

void RTTest::setPairLimit(int32_t limit) {
    pairLimit = limit;
}

UBool RTTest::isSame(const UnicodeString& a, const UnicodeString& b) {
    if (a == b) return TRUE;
    if (a.caseCompare(b, U_FOLD_CASE_DEFAULT)==0 && isCamel(a)) return TRUE;
    UnicodeString aa, bb;
    UErrorCode ec = U_ZERO_ERROR;
    Normalizer::decompose(a, FALSE, 0, aa, ec);
    Normalizer::decompose(b, FALSE, 0, bb, ec);
    if (aa == bb) return TRUE;
    if (aa.caseCompare(bb, U_FOLD_CASE_DEFAULT)==0 && isCamel(aa)) return TRUE;
    return FALSE;
}

UBool RTTest::isCamel(const UnicodeString& a) {
    // see if string is of the form aB; e.g. lower, then upper or title
    UChar32 cp;
    UBool haveLower = FALSE;
    for (int32_t i = 0; i < a.length(); i += UTF_CHAR_LENGTH(cp)) {
        cp = a.char32At(i);
        int8_t t = u_charType(cp);
        switch (t) {
        case U_UPPERCASE_LETTER:
            if (haveLower) return TRUE;
            break;
        case U_TITLECASE_LETTER:
            if (haveLower) return TRUE;
            // drop through, since second letter is lower.
        case U_LOWERCASE_LETTER:
            haveLower = TRUE;
            break;
        }
    }
    return FALSE;
}

void RTTest::test(const UnicodeString& sourceRangeVal,
                  const UnicodeString& targetRangeVal,
                  const char* roundtripExclusions,
                  IntlTest* logVal, UBool quickRt, 
                  Legal* adoptedLegal,
                  int32_t density)
{

    UErrorCode status = U_ZERO_ERROR;

    this->parent = logVal;
    this->legalSource = adoptedLegal;
    
    UnicodeSet neverOk("[:Other:]", status);
    UnicodeSet okAnyway("[^[:Letter:]]", status);

    if (U_FAILURE(status)) {
        parent->dataerrln("FAIL: Initializing UnicodeSet with [:Other:] or [^[:Letter:]] - Error: %s", u_errorName(status));
        return;
    }

    this->sourceRange.clear();
    this->sourceRange.applyPattern(sourceRangeVal, status);
    if (U_FAILURE(status)) {
        parent->errln("FAIL: UnicodeSet::applyPattern(" +
                   sourceRangeVal + ")");
        return;
    }
    this->sourceRange.removeAll(neverOk);
    
    this->targetRange.clear();
    this->targetRange.applyPattern(targetRangeVal, status);
    if (U_FAILURE(status)) {
        parent->errln("FAIL: UnicodeSet::applyPattern(" +
                   targetRangeVal + ")");
        return;
    }
    this->targetRange.removeAll(neverOk);
    
    this->toSource.clear();
    this->toSource.applyPattern(sourceRangeVal, status);
    if (U_FAILURE(status)) {
        parent->errln("FAIL: UnicodeSet::applyPattern(" +
                   sourceRangeVal + ")");
        return;
    }
    this->toSource.addAll(okAnyway);

    this->toTarget.clear();
    this->toTarget.applyPattern(targetRangeVal, status);
    if (U_FAILURE(status)) {
        parent->errln("FAIL: UnicodeSet::applyPattern(" +
                   targetRangeVal + ")");
        return;
    }
    this->toTarget.addAll(okAnyway);

    this->roundtripExclusionsSet.clear();
    if (roundtripExclusions != NULL && strlen(roundtripExclusions) > 0) {
        this->roundtripExclusionsSet.applyPattern(UnicodeString(roundtripExclusions, -1, US_INV), status);
        if (U_FAILURE(status)) {
            parent->errln("FAIL: UnicodeSet::applyPattern(%s)", roundtripExclusions);
            return;
        }
    }
    
    badCharacters.clear();
    badCharacters.applyPattern("[:Other:]", status);
    if (U_FAILURE(status)) {
        parent->errln("FAIL: UnicodeSet::applyPattern([:Other:])");
        return;
    }

    test2(quickRt, density);

    if (errorCount > 0) {
        char str[100];
        int32_t length = transliteratorID.extract(str, 100, NULL, status);
        str[length] = 0;
        parent->errln("FAIL: %s errors: %d %s", str, errorCount, (errorCount > errorLimit ? " (at least!)" : " ")); // + ", see " + logFileName);
    } else {
        char str[100];
        int32_t length = transliteratorID.extract(str, 100, NULL, status);
        str[length] = 0;
        parent->logln("%s ok", str);
    }
}

UBool RTTest::checkIrrelevants(Transliterator *t, 
                               const UnicodeString& irrelevants) {
    for (int i = 0; i < irrelevants.length(); ++i) {
        UChar c = irrelevants.charAt(i);
        UnicodeString srcStr(c);
        UnicodeString targ = srcStr;
        t->transliterate(targ);
        if (srcStr == targ) return TRUE;
    }
    return FALSE;
}

void RTTest::test2(UBool quickRt, int32_t density) {

    UnicodeString srcStr, targ, reverse;
    UErrorCode status = U_ZERO_ERROR;
    UParseError parseError ;
    TransliteratorPointer sourceToTarget(
        Transliterator::createInstance(transliteratorID, UTRANS_FORWARD, parseError,
                                       status));
    if ((Transliterator *)sourceToTarget == NULL) {
        parent->errln("FAIL: createInstance(" + transliteratorID +
                   ") returned NULL. Error: " + u_errorName(status)
                   + "\n\tpreContext : " + prettify(parseError.preContext) 
                   + "\n\tpostContext : " + prettify(parseError.postContext));
        
                return;
    }
    TransliteratorPointer targetToSource(sourceToTarget->createInverse(status));
    if ((Transliterator *)targetToSource == NULL) {
        parent->errln("FAIL: " + transliteratorID +
                   ".createInverse() returned NULL. Error:" + u_errorName(status)          
                   + "\n\tpreContext : " + prettify(parseError.preContext) 
                   + "\n\tpostContext : " + prettify(parseError.postContext));
        return;
    }

    AbbreviatedUnicodeSetIterator usi;
    AbbreviatedUnicodeSetIterator usi2;

    parent->logln("Checking that at least one irrelevant character is not NFC'ed");
    // string is from NFC_NO in the UCD
    UnicodeString irrelevants = CharsToUnicodeString("\\u2000\\u2001\\u2126\\u212A\\u212B\\u2329"); 

    if (checkIrrelevants(sourceToTarget, irrelevants) == FALSE) {
        logFails("Source-Target, irrelevants");
    }
    if (checkIrrelevants(targetToSource, irrelevants) == FALSE) {
        logFails("Target-Source, irrelevants");
    }
            
    if (!quickRt){
      parent->logln("Checking that toRules works");
      UnicodeString rules = "";
       
      UParseError parseError;
      rules = sourceToTarget->toRules(rules, TRUE);
      // parent->logln((UnicodeString)"toRules => " + rules);
      TransliteratorPointer sourceToTarget2(Transliterator::createFromRules(
                                                       "s2t2", rules, 
                                                       UTRANS_FORWARD,
                                                       parseError, status));
      if (U_FAILURE(status)) {
          parent->errln("FAIL: createFromRules %s\n", u_errorName(status));
          return;
      }

      rules = targetToSource->toRules(rules, FALSE);
      TransliteratorPointer targetToSource2(Transliterator::createFromRules(
                                                       "t2s2", rules, 
                                                       UTRANS_FORWARD,
                                                       parseError, status));
      if (U_FAILURE(status)) {
          parent->errln("FAIL: createFromRules %s\n", u_errorName(status));
          return;
      }

      usi.reset(sourceRange);
      for (;;) {
          if (!usi.next() || usi.isString()) break;
          UChar32 c = usi.getCodepoint();
                    
          UnicodeString srcStr((UChar32)c);
          UnicodeString targ = srcStr;
          sourceToTarget->transliterate(targ);
          UnicodeString targ2 = srcStr;
          sourceToTarget2->transliterate(targ2);
          if (targ != targ2) {
              logToRulesFails("Source-Target, toRules", srcStr, targ, targ2);
          }
      }
      
      usi.reset(targetRange);
      for (;;) {
          if (!usi.next() || usi.isString()) break;
          UChar32 c = usi.getCodepoint();
              
          UnicodeString srcStr((UChar32)c);
          UnicodeString targ = srcStr;
          targetToSource->transliterate(targ);
          UnicodeString targ2 = srcStr;
          targetToSource2->transliterate(targ2);
          if (targ != targ2) {
              logToRulesFails("Target-Source, toRules", srcStr, targ, targ2);
          }
      }
    }      

    parent->logln("Checking that all source characters convert to target - Singles");

    UnicodeSet failSourceTarg;
    usi.reset(sourceRange);
    for (;;) {
        if (!usi.next() || usi.isString()) break;
        UChar32 c = usi.getCodepoint();
                
        UnicodeString srcStr((UChar32)c);
        UnicodeString targ = srcStr;
        sourceToTarget->transliterate(targ);
        if (toTarget.containsAll(targ) == FALSE
            || badCharacters.containsSome(targ) == TRUE) {
            UnicodeString targD;
            Normalizer::decompose(targ, FALSE, 0, targD, status);
            if (U_FAILURE(status)) {
                parent->errln("FAIL: Internal error during decomposition %s\n", u_errorName(status));
                return;
            }
            if (toTarget.containsAll(targD) == FALSE || 
                badCharacters.containsSome(targD) == TRUE) {
                logWrongScript("Source-Target", srcStr, targ);
                failSourceTarg.add(c);
                continue;
            }
        }

        UnicodeString cs2;
        Normalizer::decompose(srcStr, FALSE, 0, cs2, status);
        if (U_FAILURE(status)) {
            parent->errln("FAIL: Internal error during decomposition %s\n", u_errorName(status));
            return;
        }
        UnicodeString targ2 = cs2;
        sourceToTarget->transliterate(targ2);
        if (targ != targ2) {
            logNotCanonical("Source-Target", srcStr, targ,cs2, targ2);
        }
    }

    parent->logln("Checking that all source characters convert to target - Doubles");

    UnicodeSet sourceRangeMinusFailures(sourceRange);
    sourceRangeMinusFailures.removeAll(failSourceTarg);
            
    usi.reset(sourceRangeMinusFailures, quickRt, density);
    for (;;) { 
        if (!usi.next() || usi.isString()) break;
        UChar32 c = usi.getCodepoint();
             
        usi2.reset(sourceRangeMinusFailures, quickRt, density);
        for (;;) {
            if (!usi2.next() || usi2.isString()) break;
            UChar32 d = usi2.getCodepoint();
                    
            UnicodeString srcStr;
            srcStr += (UChar32)c;
            srcStr += (UChar32)d;
            UnicodeString targ = srcStr;
            sourceToTarget->transliterate(targ);
            if (toTarget.containsAll(targ) == FALSE || 
                badCharacters.containsSome(targ) == TRUE)
            {
                UnicodeString targD;
                Normalizer::decompose(targ, FALSE, 0, targD, status);
                if (U_FAILURE(status)) {
                    parent->errln("FAIL: Internal error during decomposition %s\n", u_errorName(status));
                    return;
                }
                if (toTarget.containsAll(targD) == FALSE ||
                    badCharacters.containsSome(targD) == TRUE) {
                    logWrongScript("Source-Target", srcStr, targ);
                    continue;
                }
            }
            UnicodeString cs2;
            Normalizer::decompose(srcStr, FALSE, 0, cs2, status);
            if (U_FAILURE(status)) {
                parent->errln("FAIL: Internal error during decomposition %s\n", u_errorName(status));
                return;
            }
            UnicodeString targ2 = cs2;
            sourceToTarget->transliterate(targ2);
            if (targ != targ2) {
                logNotCanonical("Source-Target", srcStr, targ, cs2,targ2);
            }
        }
    }

    parent->logln("Checking that target characters convert to source and back - Singles");

    UnicodeSet failTargSource;
    UnicodeSet failRound;

    usi.reset(targetRange);
    for (;;) {
        if (!usi.next()) break;
        
        if(usi.isString()){
            srcStr = usi.getString();
        }else{
            srcStr = (UnicodeString)usi.getCodepoint();
        }

        UChar32 c = srcStr.char32At(0);
        
        targ = srcStr;
        targetToSource->transliterate(targ);
        reverse = targ;
        sourceToTarget->transliterate(reverse);

        if (toSource.containsAll(targ) == FALSE ||
            badCharacters.containsSome(targ) == TRUE) {
            UnicodeString targD;
            Normalizer::decompose(targ, FALSE, 0, targD, status);
            if (U_FAILURE(status)) {
                parent->errln("FAIL: Internal error during decomposition%s\n", u_errorName(status));
                return;
            }
            if (toSource.containsAll(targD) == FALSE) {
                logWrongScript("Target-Source", srcStr, targ);
                failTargSource.add(c);
                continue;
            }
            if (badCharacters.containsSome(targD) == TRUE) {
                logWrongScript("Target-Source*", srcStr, targ);
                failTargSource.add(c);
                continue;
            }
        }
        if (isSame(srcStr, reverse) == FALSE && 
            roundtripExclusionsSet.contains(c) == FALSE
            && roundtripExclusionsSet.contains(srcStr)==FALSE) {
            logRoundTripFailure(srcStr,targetToSource->getID(), targ,sourceToTarget->getID(), reverse);
            failRound.add(c);
            continue;
        } 
        
        UnicodeString targ2;
        Normalizer::decompose(targ, FALSE, 0, targ2, status);
        if (U_FAILURE(status)) {
            parent->errln("FAIL: Internal error during decomposition%s\n", u_errorName(status));
            return;
        }
        UnicodeString reverse2 = targ2;
        sourceToTarget->transliterate(reverse2);
        if (reverse != reverse2) {
            logNotCanonical("Target-Source", targ, reverse, targ2, reverse2);
        }
    }

    parent->logln("Checking that target characters convert to source and back - Doubles");
    int32_t count = 0;

    UnicodeSet targetRangeMinusFailures(targetRange);
    targetRangeMinusFailures.removeAll(failTargSource);
    targetRangeMinusFailures.removeAll(failRound);

    usi.reset(targetRangeMinusFailures, quickRt, density);
    UnicodeString targ2;
    UnicodeString reverse2;
    UnicodeString targD;
    for (;;) {
        if (!usi.next() || usi.isString()) break;
        UChar32 c = usi.getCodepoint();
        if (++count > pairLimit) {
            //throw new TestTruncated("Test truncated at " + pairLimit + " x 64k pairs");
            parent->logln("");
            parent->logln((UnicodeString)"Test truncated at " + pairLimit + " x 64k pairs");
            return;
        }

        usi2.reset(targetRangeMinusFailures, quickRt, density);
        for (;;) {
            if (!usi2.next() || usi2.isString())
                break;
            UChar32 d = usi2.getCodepoint();
            srcStr.truncate(0);  // empty the variable without construction/destruction
            srcStr += c;
            srcStr += d;

            targ = srcStr;
            targetToSource->transliterate(targ);
            reverse = targ;
            sourceToTarget->transliterate(reverse);

            if (toSource.containsAll(targ) == FALSE || 
                badCharacters.containsSome(targ) == TRUE) 
            {
                targD.truncate(0);  // empty the variable without construction/destruction
                Normalizer::decompose(targ, FALSE, 0, targD, status);
                if (U_FAILURE(status)) {
                    parent->errln("FAIL: Internal error during decomposition%s\n", 
                               u_errorName(status));
                    return;
                }
                if (toSource.containsAll(targD) == FALSE 
                    || badCharacters.containsSome(targD) == TRUE)
                {
                    logWrongScript("Target-Source", srcStr, targ);
                    continue;
                }
            }
            if (isSame(srcStr, reverse) == FALSE && 
                roundtripExclusionsSet.contains(c) == FALSE&&
                roundtripExclusionsSet.contains(d) == FALSE &&
                roundtripExclusionsSet.contains(srcStr)== FALSE)
            {
                logRoundTripFailure(srcStr,targetToSource->getID(), targ, sourceToTarget->getID(),reverse);
                continue;
            } 
        
            targ2.truncate(0);  // empty the variable without construction/destruction
            Normalizer::decompose(targ, FALSE, 0, targ2, status);
            if (U_FAILURE(status)) {
                parent->errln("FAIL: Internal error during decomposition%s\n", u_errorName(status));
                return;
            }
            reverse2 = targ2;
            sourceToTarget->transliterate(reverse2);
            if (reverse != reverse2) {
                logNotCanonical("Target-Source", targ,reverse, targ2, reverse2);
            }
        }
    }
    parent->logln("");
}

void RTTest::logWrongScript(const UnicodeString& label,
                            const UnicodeString& from,
                            const UnicodeString& to) {
    parent->errln((UnicodeString)"FAIL " +
               label + ": " +
               from + "(" + TestUtility::hex(from) + ") => " +
               to + "(" + TestUtility::hex(to) + ")");
    ++errorCount;
}

void RTTest::logNotCanonical(const UnicodeString& label,
                             const UnicodeString& from,
                             const UnicodeString& to,
                             const UnicodeString& fromCan,
                             const UnicodeString& toCan) {
    parent->errln((UnicodeString)"FAIL (can.equiv)" +
               label + ": " +
               from + "(" + TestUtility::hex(from) + ") => " +
               to + "(" + TestUtility::hex(to) + ")" +
               fromCan + "(" + TestUtility::hex(fromCan) + ") => " +
               toCan + " (" +
               TestUtility::hex(toCan) + ")"
               );
    ++errorCount;
}

void RTTest::logFails(const UnicodeString& label) {
    parent->errln((UnicodeString)"<br>FAIL " + label);
    ++errorCount;
}

void RTTest::logToRulesFails(const UnicodeString& label, 
                             const UnicodeString& from, 
                             const UnicodeString& to, 
                             const UnicodeString& otherTo)
{
    parent->errln((UnicodeString)"FAIL: " +
               label + ": " +
               from + "(" + TestUtility::hex(from) + ") => " +
               to + "(" + TestUtility::hex(to) + ")" +
               "!=" +
               otherTo + " (" +
               TestUtility::hex(otherTo) + ")"
               );
    ++errorCount;
}


void RTTest::logRoundTripFailure(const UnicodeString& from,
                                 const UnicodeString& toID,
                                 const UnicodeString& to,
                                 const UnicodeString& backID,
                                 const UnicodeString& back) {
    if (legalSource->is(from) == FALSE) return; // skip illegals

    parent->errln((UnicodeString)"FAIL Roundtrip: " +
               from + "(" + TestUtility::hex(from) + ") => " +
               to + "(" + TestUtility::hex(to) + ")  "+toID+" => " +
               back + "(" + TestUtility::hex(back) + ") "+backID+" => ");
    ++errorCount;
}

//--------------------------------------------------------------------
// Specific Tests
//--------------------------------------------------------------------

    /*
    Note: Unicode 3.2 added new Hiragana/Katakana characters:
    
3095..3096    ; 3.2 #   [2] HIRAGANA LETTER SMALL KA..HIRAGANA LETTER SMALL KE
309F..30A0    ; 3.2 #   [2] HIRAGANA DIGRAPH YORI..KATAKANA-HIRAGANA DOUBLE HYPHEN
30FF          ; 3.2 #       KATAKANA DIGRAPH KOTO
31F0..31FF    ; 3.2 #  [16] KATAKANA LETTER SMALL KU..KATAKANA LETTER SMALL RO

    Unicode 5.2 added another Hiragana character:
1F200         ; 5.2 #       SQUARE HIRAGANA HOKA

    We will not add them to the rules until they are more supported (e.g. in fonts on Windows)
    A bug has been filed to remind us to do this: #1979.
    */

static const char KATAKANA[] = "[[[:katakana:][\\u30A1-\\u30FA\\u30FC]]-[\\u30FF\\u31F0-\\u31FF]-[:^age=5.2:]]";
static const char HIRAGANA[] = "[[[:hiragana:][\\u3040-\\u3094]]-[\\u3095-\\u3096\\u309F-\\u30A0\\U0001F200-\\U0001F2FF]-[:^age=5.2:]]";
static const char LENGTH[] = "[\\u30FC]";
static const char HALFWIDTH_KATAKANA[] = "[\\uFF65-\\uFF9D]";
static const char KATAKANA_ITERATION[] = "[\\u30FD\\u30FE]";
static const char HIRAGANA_ITERATION[] = "[\\u309D\\u309E]";
static const int32_t TEMP_MAX=256;

void TransliteratorRoundTripTest::TestKana() {
    RTTest test("Katakana-Hiragana");
    Legal *legal = new Legal();
    char temp[TEMP_MAX];
    strcpy(temp, "[");
    strcat(temp, HALFWIDTH_KATAKANA);
    strcat(temp, LENGTH);
    strcat(temp, "]");
    test.test(KATAKANA, UnicodeString("[") + HIRAGANA + LENGTH + UnicodeString("]"), 
              temp, 
              this, quick, legal);
    delete legal;
}

void TransliteratorRoundTripTest::TestHiragana() {
    RTTest test("Latin-Hiragana");
    Legal *legal = new Legal();
    test.test(UnicodeString("[a-zA-Z]", ""), 
              UnicodeString(HIRAGANA, -1, US_INV), 
              HIRAGANA_ITERATION, this, quick, legal);
    delete legal;
}

void TransliteratorRoundTripTest::TestKatakana() {
    RTTest test("Latin-Katakana");
    Legal *legal = new Legal();
    char temp[TEMP_MAX];
    strcpy(temp, "[");
    strcat(temp, KATAKANA_ITERATION);
    strcat(temp, HALFWIDTH_KATAKANA);
    strcat(temp, "]");
    test.test(UnicodeString("[a-zA-Z]", ""), 
              UnicodeString(KATAKANA, -1, US_INV),
              temp, 
              this, quick, legal);
    delete legal;
}

void TransliteratorRoundTripTest::TestJamo() {
    RTTest t("Latin-Jamo");
    Legal *legal = new LegalJamo();
    t.test(UnicodeString("[a-zA-Z]", ""), 
           UnicodeString("[\\u1100-\\u1112 \\u1161-\\u1175 \\u11A8-\\u11C2]", 
                         ""), 
           NULL, this, quick, legal);
    delete legal;
}

void TransliteratorRoundTripTest::TestHangul() {
    RTTest t("Latin-Hangul");
    Legal *legal = new Legal();
    if (quick) t.setPairLimit(1000);
    t.test(UnicodeString("[a-zA-Z]", ""), 
           UnicodeString("[\\uAC00-\\uD7A4]", ""), 
           NULL, this, quick, legal, 1);
    delete legal;
}


#define ASSERT_SUCCESS(status) {if (U_FAILURE(status)) { \
     errcheckln(status, "error at file %s, line %d, status = %s", __FILE__, __LINE__, \
         u_errorName(status)); \
         return;}}
    

static void writeStringInU8(FILE *out, const UnicodeString &s) {
    int i;
    for (i=0; i<s.length(); i=s.moveIndex32(i, 1)) {
        UChar32  c = s.char32At(i);
        uint8_t  bufForOneChar[10];
        UBool    isError = FALSE;
        int32_t  destIdx = 0;
        U8_APPEND(bufForOneChar, destIdx, (int32_t)sizeof(bufForOneChar), c, isError);
        fwrite(bufForOneChar, 1, destIdx, out);
    }
}
        



void TransliteratorRoundTripTest::TestHan() {
    UErrorCode  status = U_ZERO_ERROR;
    LocalULocaleDataPointer uld(ulocdata_open("zh",&status));
    LocalUSetPointer USetExemplars(ulocdata_getExemplarSet(uld.getAlias(), uset_openEmpty(), 0, ULOCDATA_ES_STANDARD, &status));
    ASSERT_SUCCESS(status);

    UnicodeString source;
    UChar32       c;
    int           i;
    for (i=0; ;i++) {
        // Add all of the Chinese exemplar chars to the string "source".
        c = uset_charAt(USetExemplars.getAlias(), i);
        if (c == (UChar32)-1) {
            break;
        }
        source.append(c);
    }

    // transform with Han translit
    Transliterator *hanTL = Transliterator::createInstance("Han-Latin", UTRANS_FORWARD, status);
    ASSERT_SUCCESS(status);
    UnicodeString target=source;
    hanTL->transliterate(target);
    // now verify that there are no Han characters left
    UnicodeSet allHan("[:han:]", status);
    ASSERT_SUCCESS(status);
    if (allHan.containsSome(target)) {
        errln("file %s, line %d, No Han must be left after Han-Latin transliteration",
            __FILE__, __LINE__);
    }

    // check the pinyin translit
    Transliterator *pn = Transliterator::createInstance("Latin-NumericPinyin", UTRANS_FORWARD, status);
    ASSERT_SUCCESS(status);
    UnicodeString target2 = target;
    pn->transliterate(target2);

    // verify that there are no marks
    Transliterator *nfd = Transliterator::createInstance("nfd", UTRANS_FORWARD, status);
    ASSERT_SUCCESS(status);

    UnicodeString nfded = target2;
    nfd->transliterate(nfded);
    UnicodeSet allMarks(UNICODE_STRING_SIMPLE("[\\u0304\\u0301\\u030C\\u0300\\u0306]"), status); // look only for Pinyin tone marks, not all marks (there are some others in there)
    ASSERT_SUCCESS(status);
    assertFalse("NumericPinyin must contain no marks", allMarks.containsSome(nfded));

    // verify roundtrip
    Transliterator *np = pn->createInverse(status);
    ASSERT_SUCCESS(status);
    UnicodeString target3 = target2;
    np->transliterate(target3);
    UBool roundtripOK = (target3.compare(target) == 0);
    assertTrue("NumericPinyin must roundtrip", roundtripOK);
    if (!roundtripOK) {
        const char *filename = "numeric-pinyin.log.txt";
        FILE *out = fopen(filename, "w");
        errln("Creating log file %s\n", filename);
        fprintf(out, "Pinyin:                ");
        writeStringInU8(out, target);
        fprintf(out, "\nPinyin-Numeric-Pinyin: ");
        writeStringInU8(out, target2);
        fprintf(out, "\nNumeric-Pinyin-Pinyin: ");
        writeStringInU8(out, target3);
        fprintf(out, "\n");
        fclose(out);
    }

    delete hanTL;
    delete pn;
    delete nfd;
    delete np;
}


void TransliteratorRoundTripTest::TestGreek() {

    // CLDR bug #1911: This test should be moved into CLDR.
    // It is left in its current state as a regression test.
    
//    if (isICUVersionAtLeast(ICU_39)) {
//        // We temporarily filter against Unicode 4.1, but we only do this
//        // before version 3.4.
//        errln("FAIL: TestGreek needs to be updated to remove delete the [:Age=4.0:] filter ");
//        return;
//    } else {
//        logln("Warning: TestGreek needs to be updated to remove delete the section marked [:Age=4.0:] filter");
//    }
    
    RTTest test("Latin-Greek");
    LegalGreek *legal = new LegalGreek(TRUE);

    test.test(UnicodeString("[a-zA-Z]", ""), 
        UnicodeString("[\\u003B\\u00B7[[:Greek:]&[:Letter:]]-["
            "\\u1D26-\\u1D2A" // L&   [5] GREEK LETTER SMALL CAPITAL GAMMA..GREEK LETTER SMALL CAPITAL PSI
            "\\u1D5D-\\u1D61" // Lm   [5] MODIFIER LETTER SMALL BETA..MODIFIER LETTER SMALL CHI
            "\\u1D66-\\u1D6A" // L&   [5] GREEK SUBSCRIPT SMALL LETTER BETA..GREEK SUBSCRIPT SMALL LETTER CHI
            "\\u03D7-\\u03EF" // \N{GREEK KAI SYMBOL}..\N{COPTIC SMALL LETTER DEI}
            "] & [:Age=4.0:]]",

              //UnicodeString("[[\\u003B\\u00B7[:Greek:]-[\\u0374\\u0385\\u1fcd\\u1fce\\u1fdd\\u1fde\\u1fed-\\u1fef\\u1ffd\\u03D7-\\u03EF]]&[:Age=3.2:]]", 
                            ""),
              "[\\u00B5\\u037A\\u03D0-\\u03F5\\u03f9]", /* exclusions */
              this, quick, legal, 50);


    delete legal;
}


void TransliteratorRoundTripTest::TestGreekUNGEGN() {

    // CLDR bug #1911: This test should be moved into CLDR.
    // It is left in its current state as a regression test.

//    if (isICUVersionAtLeast(ICU_39)) {
//        // We temporarily filter against Unicode 4.1, but we only do this
//        // before version 3.4.
//        errln("FAIL: TestGreek needs to be updated to remove delete the [:Age=4.0:] filter ");
//        return;
//    } else {
//        logln("Warning: TestGreek needs to be updated to remove delete the section marked [:Age=4.0:] filter");
//    }

    RTTest test("Latin-Greek/UNGEGN");
    LegalGreek *legal = new LegalGreek(FALSE);

    test.test(UnicodeString("[a-zA-Z]", ""), 
        UnicodeString("[\\u003B\\u00B7[[:Greek:]&[:Letter:]]-["
            "\\u1D26-\\u1D2A" // L&   [5] GREEK LETTER SMALL CAPITAL GAMMA..GREEK LETTER SMALL CAPITAL PSI
            "\\u1D5D-\\u1D61" // Lm   [5] MODIFIER LETTER SMALL BETA..MODIFIER LETTER SMALL CHI
            "\\u1D66-\\u1D6A" // L&   [5] GREEK SUBSCRIPT SMALL LETTER BETA..GREEK SUBSCRIPT SMALL LETTER CHI
            "\\u03D7-\\u03EF" // \N{GREEK KAI SYMBOL}..\N{COPTIC SMALL LETTER DEI}
            "] & [:Age=4.0:]]",
              //UnicodeString("[[\\u003B\\u00B7[:Greek:]-[\\u0374\\u0385\\u1fce\\u1fde\\u03D7-\\u03EF]]&[:Age=3.2:]]", 
                            ""), 
              "[\\u0385\\u00B5\\u037A\\u03D0-\\uFFFF {\\u039C\\u03C0}]", /* roundtrip exclusions */
              this, quick, legal);

    delete legal;
}

void TransliteratorRoundTripTest::Testel() {
    
    // CLDR bug #1911: This test should be moved into CLDR.
    // It is left in its current state as a regression test.

//    if (isICUVersionAtLeast(ICU_39)) {
//        // We temporarily filter against Unicode 4.1, but we only do this
//        // before version 3.4.
//        errln("FAIL: TestGreek needs to be updated to remove delete the [:Age=4.0:] filter ");
//        return;
//    } else {
//        logln("Warning: TestGreek needs to be updated to remove delete the section marked [:Age=4.0:] filter");
//    }

    RTTest test("Latin-el");
    LegalGreek *legal = new LegalGreek(FALSE);

    test.test(UnicodeString("[a-zA-Z]", ""), 
        UnicodeString("[\\u003B\\u00B7[[:Greek:]&[:Letter:]]-["
            "\\u1D26-\\u1D2A" // L&   [5] GREEK LETTER SMALL CAPITAL GAMMA..GREEK LETTER SMALL CAPITAL PSI
            "\\u1D5D-\\u1D61" // Lm   [5] MODIFIER LETTER SMALL BETA..MODIFIER LETTER SMALL CHI
            "\\u1D66-\\u1D6A" // L&   [5] GREEK SUBSCRIPT SMALL LETTER BETA..GREEK SUBSCRIPT SMALL LETTER CHI
            "\\u03D7-\\u03EF" // \N{GREEK KAI SYMBOL}..\N{COPTIC SMALL LETTER DEI}
            "] & [:Age=4.0:]]",
              //UnicodeString("[[\\u003B\\u00B7[:Greek:]-[\\u0374\\u0385\\u1fce\\u1fde\\u03D7-\\u03EF]]&[:Age=3.2:]]", 
                            ""), 
              "[\\u00B5\\u037A\\u03D0-\\uFFFF {\\u039C\\u03C0}]", /* exclusions */
              this, quick, legal);


    delete legal;
}


void TransliteratorRoundTripTest::TestArabic() {
    UnicodeString ARABIC("[\\u060C\\u061B\\u061F\\u0621\\u0627-\\u063A\\u0641-\\u0655\\u0660-\\u066C\\u067E\\u0686\\u0698\\u06A4\\u06AD\\u06AF\\u06CB-\\u06CC\\u06F0-\\u06F9]", -1, US_INV);
    Legal *legal = new Legal();
    RTTest test("Latin-Arabic");
        test.test(UNICODE_STRING_SIMPLE("[a-zA-Z\\u02BE\\u02BF\\u207F]"), ARABIC, "[a-zA-Z\\u02BE\\u02BF\\u207F]",this, quick, legal); //
   delete legal;
}
class LegalHebrew : public Legal {
private:
    UnicodeSet FINAL;
    UnicodeSet NON_FINAL;
    UnicodeSet LETTER;
public:
    LegalHebrew(UErrorCode& error);
    virtual ~LegalHebrew() {}
    virtual UBool is(const UnicodeString& sourceString) const;
};

LegalHebrew::LegalHebrew(UErrorCode& error){
    FINAL.applyPattern(UNICODE_STRING_SIMPLE("[\\u05DA\\u05DD\\u05DF\\u05E3\\u05E5]"), error);
    NON_FINAL.applyPattern(UNICODE_STRING_SIMPLE("[\\u05DB\\u05DE\\u05E0\\u05E4\\u05E6]"), error);
    LETTER.applyPattern("[:letter:]", error);
}
UBool LegalHebrew::is(const UnicodeString& sourceString)const{
 
    if (sourceString.length() == 0) return TRUE;
    // don't worry about surrogates.
    for (int i = 0; i < sourceString.length(); ++i) {
        UChar ch = sourceString.charAt(i);
        UChar next = i+1 == sourceString.length() ? 0x0000 : sourceString.charAt(i);
        if (FINAL.contains(ch)) {
            if (LETTER.contains(next)) return FALSE;
        } else if (NON_FINAL.contains(ch)) {
            if (!LETTER.contains(next)) return FALSE;
        }
    }
    return TRUE;
}
void TransliteratorRoundTripTest::TestHebrew() {
    // CLDR bug #1911: This test should be moved into CLDR.
    // It is left in its current state as a regression test.
//    if (isICUVersionAtLeast(ICU_39)) {
//        // We temporarily filter against Unicode 4.1, but we only do this
//        // before version 3.4.
//        errln("FAIL: TestHebrew needs to be updated to remove delete the [:Age=4.0:] filter ");
//        return;
//    } else {
//        logln("Warning: TestHebrew needs to be updated to remove delete the section marked [:Age=4.0:] filter");
//    }
    //long start = System.currentTimeMillis();
    UErrorCode error = U_ZERO_ERROR;
    LegalHebrew* legal = new LegalHebrew(error);
    if(U_FAILURE(error)){
        dataerrln("Could not construct LegalHebrew object. Error: %s", u_errorName(error));
        return;
    }
    RTTest test("Latin-Hebrew");
    test.test(UNICODE_STRING_SIMPLE("[a-zA-Z\\u02BC\\u02BB]"), UNICODE_STRING_SIMPLE("[[[:hebrew:]-[\\u05BD\\uFB00-\\uFBFF]]&[:Age=4.0:]]"), "[\\u05F0\\u05F1\\u05F2]", this, quick, legal);
   
    //showElapsed(start, "TestHebrew");
    delete legal;
}
void TransliteratorRoundTripTest::TestCyrillic() {
    RTTest test("Latin-Cyrillic");
    Legal *legal = new Legal();

    test.test(UnicodeString("[a-zA-Z\\u0110\\u0111\\u02BA\\u02B9]", ""), 
              UnicodeString("[[\\u0400-\\u045F] & [:Age=3.2:]]", ""), NULL, this, quick, 
              legal);

    delete legal;
}


// Inter-Indic Tests ----------------------------------
class LegalIndic :public Legal{
    UnicodeSet vowelSignSet;
    UnicodeSet avagraha;
    UnicodeSet nukta;
    UnicodeSet virama;
    UnicodeSet sanskritStressSigns;
    UnicodeSet chandrabindu;
    
public:
    LegalIndic();
    virtual UBool is(const UnicodeString& sourceString) const;
    virtual ~LegalIndic() {};
};
UBool LegalIndic::is(const UnicodeString& sourceString) const{
    int cp=sourceString.charAt(0);
    
    // A vowel sign cannot be the first char
    if(vowelSignSet.contains(cp)){
        return FALSE;
    }else if(avagraha.contains(cp)){
        return FALSE;
    }else if(virama.contains(cp)){
        return FALSE;
    }else if(nukta.contains(cp)){
        return FALSE;
    }else if(sanskritStressSigns.contains(cp)){
        return FALSE;
    }else if(chandrabindu.contains(cp) && 
                ((sourceString.length()>1) && 
                    vowelSignSet.contains(sourceString.charAt(1)))){
        return FALSE;
    }
    return TRUE;
}
LegalIndic::LegalIndic(){
        UErrorCode status = U_ZERO_ERROR;
        vowelSignSet.addAll( UnicodeSet("[\\u0902\\u0903\\u0904\\u093e-\\u094c\\u0962\\u0963]",status));/* Devanagari */
        vowelSignSet.addAll( UnicodeSet("[\\u0982\\u0983\\u09be-\\u09cc\\u09e2\\u09e3\\u09D7]",status));/* Bengali */
        vowelSignSet.addAll( UnicodeSet("[\\u0a02\\u0a03\\u0a3e-\\u0a4c\\u0a62\\u0a63\\u0a70\\u0a71]",status));/* Gurmukhi */
        vowelSignSet.addAll( UnicodeSet("[\\u0a82\\u0a83\\u0abe-\\u0acc\\u0ae2\\u0ae3]",status));/* Gujarati */
        vowelSignSet.addAll( UnicodeSet("[\\u0b02\\u0b03\\u0b3e-\\u0b4c\\u0b62\\u0b63\\u0b56\\u0b57]",status));/* Oriya */
        vowelSignSet.addAll( UnicodeSet("[\\u0b82\\u0b83\\u0bbe-\\u0bcc\\u0be2\\u0be3\\u0bd7]",status));/* Tamil */
        vowelSignSet.addAll( UnicodeSet("[\\u0c02\\u0c03\\u0c3e-\\u0c4c\\u0c62\\u0c63\\u0c55\\u0c56]",status));/* Telugu */
        vowelSignSet.addAll( UnicodeSet("[\\u0c82\\u0c83\\u0cbe-\\u0ccc\\u0ce2\\u0ce3\\u0cd5\\u0cd6]",status));/* Kannada */
        vowelSignSet.addAll( UnicodeSet("[\\u0d02\\u0d03\\u0d3e-\\u0d4c\\u0d62\\u0d63\\u0d57]",status));/* Malayalam */

        avagraha.addAll(UnicodeSet("[\\u093d\\u09bd\\u0abd\\u0b3d\\u0cbd]",status));
        nukta.addAll(UnicodeSet("[\\u093c\\u09bc\\u0a3c\\u0abc\\u0b3c\\u0cbc]",status));
        virama.addAll(UnicodeSet("[\\u094d\\u09cd\\u0a4d\\u0acd\\u0b4d\\u0bcd\\u0c4d\\u0ccd\\u0d4d]",status));
        sanskritStressSigns.addAll(UnicodeSet("[\\u0951\\u0952\\u0953\\u0954\\u097d]",status));
        chandrabindu.addAll(UnicodeSet("[\\u0901\\u0981\\u0A81\\u0b01\\u0c01]",status));

    }

static const char latinForIndic[] = "[['.0-9A-Za-z~\\u00C0-\\u00C5\\u00C7-\\u00CF\\u00D1-\\u00D6\\u00D9-\\u00DD"
                                   "\\u00E0-\\u00E5\\u00E7-\\u00EF\\u00F1-\\u00F6\\u00F9-\\u00FD\\u00FF-\\u010F"
                                   "\\u0112-\\u0125\\u0128-\\u0130\\u0134-\\u0137\\u0139-\\u013E\\u0143-\\u0148"
                                   "\\u014C-\\u0151\\u0154-\\u0165\\u0168-\\u017E\\u01A0-\\u01A1\\u01AF-\\u01B0"
                                   "\\u01CD-\\u01DC\\u01DE-\\u01E3\\u01E6-\\u01ED\\u01F0\\u01F4-\\u01F5\\u01F8-\\u01FB"
                                   "\\u0200-\\u021B\\u021E-\\u021F\\u0226-\\u0233\\u0294\\u0303-\\u0304\\u0306\\u0314-\\u0315"
                                   "\\u0325\\u040E\\u0419\\u0439\\u045E\\u04C1-\\u04C2\\u04D0-\\u04D1\\u04D6-\\u04D7"
                                   "\\u04E2-\\u04E3\\u04EE-\\u04EF\\u1E00-\\u1E99\\u1EA0-\\u1EF9\\u1F01\\u1F03\\u1F05"
                                   "\\u1F07\\u1F09\\u1F0B\\u1F0D\\u1F0F\\u1F11\\u1F13\\u1F15\\u1F19\\u1F1B\\u1F1D\\u1F21"
                                   "\\u1F23\\u1F25\\u1F27\\u1F29\\u1F2B\\u1F2D\\u1F2F\\u1F31\\u1F33\\u1F35\\u1F37\\u1F39"
                                   "\\u1F3B\\u1F3D\\u1F3F\\u1F41\\u1F43\\u1F45\\u1F49\\u1F4B\\u1F4D\\u1F51\\u1F53\\u1F55"
                                   "\\u1F57\\u1F59\\u1F5B\\u1F5D\\u1F5F\\u1F61\\u1F63\\u1F65\\u1F67\\u1F69\\u1F6B\\u1F6D"
                                   "\\u1F6F\\u1F81\\u1F83\\u1F85\\u1F87\\u1F89\\u1F8B\\u1F8D\\u1F8F\\u1F91\\u1F93\\u1F95"
                                   "\\u1F97\\u1F99\\u1F9B\\u1F9D\\u1F9F\\u1FA1\\u1FA3\\u1FA5\\u1FA7\\u1FA9\\u1FAB\\u1FAD"
                                   "\\u1FAF-\\u1FB1\\u1FB8-\\u1FB9\\u1FD0-\\u1FD1\\u1FD8-\\u1FD9\\u1FE0-\\u1FE1\\u1FE5"
                                   "\\u1FE8-\\u1FE9\\u1FEC\\u212A-\\u212B\\uE04D\\uE064]"
                                   "-[\\uE000-\\uE080 \\u01E2\\u01E3]& [[:latin:][:mark:]]]";

void TransliteratorRoundTripTest::TestDevanagariLatin() {
    {
        UErrorCode status = U_ZERO_ERROR;
        UParseError parseError;
        TransliteratorPointer t1(Transliterator::createInstance("[\\u0964-\\u0965\\u0981-\\u0983\\u0985-\\u098C\\u098F-\\u0990\\u0993-\\u09A8\\u09AA-\\u09B0\\u09B2\\u09B6-\\u09B9\\u09BC\\u09BE-\\u09C4\\u09C7-\\u09C8\\u09CB-\\u09CD\\u09D7\\u09DC-\\u09DD\\u09DF-\\u09E3\\u09E6-\\u09FA];NFD;Bengali-InterIndic;InterIndic-Gujarati;NFC;",UTRANS_FORWARD, parseError, status));
        if((Transliterator *)t1 != NULL){
            TransliteratorPointer t2(t1->createInverse(status));
            if(U_FAILURE(status)){
                errln("FAIL: could not create the Inverse:-( \n");
            }
        }else {
            dataerrln("FAIL: could not create the transliterator. Error: %s\n", u_errorName(status));
        }

    }
    RTTest test("Latin-Devanagari");
    Legal *legal = new LegalIndic();
    // CLDR bug #1911: This test should be moved into CLDR.
    // It is left in its current state as a regression test.
//    if (isICUVersionAtLeast(ICU_39)) {
//        // We temporarily filter against Unicode 4.1, but we only do this
//        // before version 3.4.
//        errln("FAIL: TestDevanagariLatin needs to be updated to remove delete the [:Age=4.1:] filter ");
//        return;
//    } else {
//        logln("Warning: TestDevanagariLatin needs to be updated to remove delete the section marked [:Age=4.1:] filter");
//    }
    test.test(UnicodeString(latinForIndic, ""), 
        UnicodeString("[[[:Devanagari:][\\u094d][\\u0964\\u0965]]&[:Age=4.1:]]", ""), "[\\u0965\\u0904]", this, quick, 
            legal, 50);

    delete legal;
}

/* Defined this way for HP/UX11CC :-( */
static const int32_t INTER_INDIC_ARRAY_WIDTH = 4;
static const char * const interIndicArray[] = {

    "BENGALI-DEVANAGARI", "[:BENGALI:]", "[:Devanagari:]", 
    "[\\u0904\\u0951-\\u0954\\u0943-\\u0949\\u094a\\u0962\\u0963\\u090D\\u090e\\u0911\\u0912\\u0929\\u0933\\u0934\\u0935\\u093d\\u0950\\u0958\\u0959\\u095a\\u095b\\u095e\\u097d]", /*roundtrip exclusions*/

    "DEVANAGARI-BENGALI", "[:Devanagari:]", "[:BENGALI:]",
    "[\\u0951-\\u0954\\u0951-\\u0954\\u09D7\\u090D\\u090e\\u0911\\u0912\\u0929\\u0933\\u0934\\u0935\\u093d\\u0950\\u0958\\u0959\\u095a\\u095b\\u095e\\u09f0\\u09f1\\u09f2-\\u09fa\\u09ce]", /*roundtrip exclusions*/

    "GURMUKHI-DEVANAGARI", "[:GURMUKHI:]", "[:Devanagari:]", 
    "[\\u0904\\u0901\\u0902\\u0936\\u0933\\u0951-\\u0954\\u0902\\u0903\\u0943-\\u0949\\u094a\\u0962\\u0963\\u090B\\u090C\\u090D\\u090e\\u0911\\u0912\\u0934\\u0937\\u093D\\u0950\\u0960\\u0961\\u097d]", /*roundtrip exclusions*/

    "DEVANAGARI-GURMUKHI", "[:Devanagari:]", "[:GURMUKHI:]",
    "[\\u0904\\u0A02\\u0946\\u0A5C\\u0951-\\u0954\\u0A70\\u0A71\\u090B\\u090C\\u090D\\u090e\\u0911\\u0912\\u0934\\u0937\\u093D\\u0950\\u0960\\u0961\\u0a72\\u0a73\\u0a74]", /*roundtrip exclusions*/

    "GUJARATI-DEVANAGARI", "[:GUJARATI:]", "[:Devanagari:]", 
    "[\\u0946\\u094A\\u0962\\u0963\\u0951-\\u0954\\u0961\\u090c\\u090e\\u0912\\u097d]", /*roundtrip exclusions*/

    "DEVANAGARI-GUJARATI", "[:Devanagari:]", "[:GUJARATI:]",
    "[\\u0951-\\u0954\\u0961\\u090c\\u090e\\u0912]", /*roundtrip exclusions*/

    "ORIYA-DEVANAGARI", "[:ORIYA:]", "[:Devanagari:]", 
    "[\\u0904\\u0943-\\u094a\\u0962\\u0963\\u0951-\\u0954\\u0950\\u090D\\u090e\\u0912\\u0911\\u0931\\u0935\\u097d]", /*roundtrip exclusions*/

    "DEVANAGARI-ORIYA", "[:Devanagari:]", "[:ORIYA:]",
    "[\\u0b5f\\u0b56\\u0b57\\u0b70\\u0b71\\u0950\\u090D\\u090e\\u0912\\u0911\\u0931]", /*roundtrip exclusions*/

    "Tamil-DEVANAGARI", "[:tamil:]", "[:Devanagari:]", 
    "[\\u0901\\u0904\\u093c\\u0943-\\u094a\\u0951-\\u0954\\u0962\\u0963\\u090B\\u090C\\u090D\\u0911\\u0916\\u0917\\u0918\\u091B\\u091D\\u0920\\u0921\\u0922\\u0925\\u0926\\u0927\\u092B\\u092C\\u092D\\u0936\\u093d\\u0950[\\u0958-\\u0961]\\u097d]", /*roundtrip exclusions*/

    "DEVANAGARI-Tamil", "[:Devanagari:]", "[:tamil:]", 
    "[\\u0bd7\\u0BF0\\u0BF1\\u0BF2]", /*roundtrip exclusions*/

    "Telugu-DEVANAGARI", "[:telugu:]", "[:Devanagari:]", 
    "[\\u0904\\u093c\\u0950\\u0945\\u0949\\u0951-\\u0954\\u0962\\u0963\\u090D\\u0911\\u093d\\u0929\\u0934[\\u0958-\\u095f]\\u097d]", /*roundtrip exclusions*/

    "DEVANAGARI-TELUGU", "[:Devanagari:]", "[:TELUGU:]",
    "[\\u0c55\\u0c56\\u0950\\u090D\\u0911\\u093d\\u0929\\u0934[\\u0958-\\u095f]]", /*roundtrip exclusions*/

    "KANNADA-DEVANAGARI", "[:KANNADA:]", "[:Devanagari:]", 
    "[\\u0901\\u0904\\u0946\\u093c\\u0950\\u0945\\u0949\\u0951-\\u0954\\u0962\\u0963\\u0950\\u090D\\u0911\\u093d\\u0929\\u0934[\\u0958-\\u095f]\\u097d]", /*roundtrip exclusions*/

    "DEVANAGARI-KANNADA", "[:Devanagari:]", "[:KANNADA:]",
    "[{\\u0cb0\\u0cbc}{\\u0cb3\\u0cbc}\\u0cde\\u0cd5\\u0cd6\\u0950\\u090D\\u0911\\u093d\\u0929\\u0934[\\u0958-\\u095f]]", /*roundtrip exclusions*/ 

    "MALAYALAM-DEVANAGARI", "[:MALAYALAM:]", "[:Devanagari:]", 
    "[\\u0901\\u0904\\u094a\\u094b\\u094c\\u093c\\u0950\\u0944\\u0945\\u0949\\u0951-\\u0954\\u0962\\u0963\\u090D\\u0911\\u093d\\u0929\\u0934[\\u0958-\\u095f]\\u097d]", /*roundtrip exclusions*/

    "DEVANAGARI-MALAYALAM", "[:Devanagari:]", "[:MALAYALAM:]",
    "[\\u0d4c\\u0d57\\u0950\\u090D\\u0911\\u093d\\u0929\\u0934[\\u0958-\\u095f]]", /*roundtrip exclusions*/

    "GURMUKHI-BENGALI", "[:GURMUKHI:]", "[:BENGALI:]",  
    "[\\u0981\\u0982\\u09b6\\u09e2\\u09e3\\u09c3\\u09c4\\u09d7\\u098B\\u098C\\u09B7\\u09E0\\u09E1\\u09F0\\u09F1\\u09f2-\\u09fa\\u09ce]", /*roundtrip exclusions*/

    "BENGALI-GURMUKHI", "[:BENGALI:]", "[:GURMUKHI:]",
    "[\\u0A02\\u0a5c\\u0a47\\u0a70\\u0a71\\u0A33\\u0A35\\u0A59\\u0A5A\\u0A5B\\u0A5E\\u0A72\\u0A73\\u0A74]", /*roundtrip exclusions*/

    "GUJARATI-BENGALI", "[:GUJARATI:]", "[:BENGALI:]", 
    "[\\u09d7\\u09e2\\u09e3\\u098c\\u09e1\\u09f0\\u09f1\\u09f2-\\u09fa\\u09ce]", /*roundtrip exclusions*/

    "BENGALI-GUJARATI", "[:BENGALI:]", "[:GUJARATI:]",
    "[\\u0A82\\u0a83\\u0Ac9\\u0Ac5\\u0ac7\\u0A8D\\u0A91\\u0AB3\\u0AB5\\u0ABD\\u0AD0]", /*roundtrip exclusions*/

    "ORIYA-BENGALI", "[:ORIYA:]", "[:BENGALI:]", 
    "[\\u09c4\\u09e2\\u09e3\\u09f0\\u09f1\\u09f2-\\u09fa\\u09ce]", /*roundtrip exclusions*/

    "BENGALI-ORIYA", "[:BENGALI:]", "[:ORIYA:]",
    "[\\u0b35\\u0b71\\u0b5f\\u0b56\\u0b33\\u0b3d]", /*roundtrip exclusions*/

    "Tamil-BENGALI", "[:tamil:]", "[:BENGALI:]", 
    "[\\u0981\\u09bc\\u09c3\\u09c4\\u09e2\\u09e3\\u09f0\\u09f1\\u098B\\u098C\\u0996\\u0997\\u0998\\u099B\\u099D\\u09A0\\u09A1\\u09A2\\u09A5\\u09A6\\u09A7\\u09AB\\u09AC\\u09AD\\u09B6\\u09DC\\u09DD\\u09DF\\u09E0\\u09E1\\u09f2-\\u09fa\\u09ce]", /*roundtrip exclusions*/

    "BENGALI-Tamil", "[:BENGALI:]", "[:tamil:]",
    "[\\u0bc6\\u0bc7\\u0bca\\u0B8E\\u0B92\\u0BA9\\u0BB1\\u0BB3\\u0BB4\\u0BB5\\u0BF0\\u0BF1\\u0BF2]", /*roundtrip exclusions*/

    "Telugu-BENGALI", "[:telugu:]", "[:BENGALI:]", 
    "[\\u09e2\\u09e3\\u09bc\\u09d7\\u09f0\\u09f1\\u09dc\\u09dd\\u09df\\u09f2-\\u09fa\\u09ce]", /*roundtrip exclusions*/

    "BENGALI-TELUGU", "[:BENGALI:]", "[:TELUGU:]",
    "[\\u0c55\\u0c56\\u0c47\\u0c46\\u0c4a\\u0C0E\\u0C12\\u0C31\\u0C33\\u0C35]", /*roundtrip exclusions*/

    "KANNADA-BENGALI", "[:KANNADA:]", "[:BENGALI:]", 
    "[\\u0981\\u09e2\\u09e3\\u09bc\\u09d7\\u09dc\\u09dd\\u09df\\u09f0\\u09f1\\u09f2-\\u09fa\\u09ce]", /*roundtrip exclusions*/

    "BENGALI-KANNADA", "[:BENGALI:]", "[:KANNADA:]",
    "[{\\u0cb0\\u0cbc}{\\u0cb3\\u0cbc}\\u0cc6\\u0cca\\u0cd5\\u0cd6\\u0cc7\\u0C8E\\u0C92\\u0CB1\\u0cb3\\u0cb5\\u0cde]", /*roundtrip exclusions*/ 

    "MALAYALAM-BENGALI", "[:MALAYALAM:]", "[:BENGALI:]", 
    "[\\u0981\\u09e2\\u09e3\\u09bc\\u09c4\\u09f0\\u09f1\\u09dc\\u09dd\\u09df\\u09dc\\u09dd\\u09df\\u09f2-\\u09fa\\u09ce]", /*roundtrip exclusions*/

    "BENGALI-MALAYALAM", "[:BENGALI:]", "[:MALAYALAM:]",
    "[\\u0d46\\u0d4a\\u0d47\\u0d31-\\u0d35\\u0d0e\\u0d12]", /*roundtrip exclusions*/

    "GUJARATI-GURMUKHI", "[:GUJARATI:]", "[:GURMUKHI:]", 
    "[\\u0A02\\u0ab3\\u0ab6\\u0A70\\u0a71\\u0a82\\u0a83\\u0ac3\\u0ac4\\u0ac5\\u0ac9\\u0a5c\\u0a72\\u0a73\\u0a74\\u0a8b\\u0a8d\\u0a91\\u0abd]", /*roundtrip exclusions*/

    "GURMUKHI-GUJARATI", "[:GURMUKHI:]", "[:GUJARATI:]",
    "[\\u0a5c\\u0A70\\u0a71\\u0a72\\u0a73\\u0a74\\u0a82\\u0a83\\u0a8b\\u0a8c\\u0a8d\\u0a91\\u0ab3\\u0ab6\\u0ab7\\u0abd\\u0ac3\\u0ac4\\u0ac5\\u0ac9\\u0ad0\\u0ae0\\u0ae1]", /*roundtrip exclusions*/

    "ORIYA-GURMUKHI", "[:ORIYA:]", "[:GURMUKHI:]", 
    "[\\u0A01\\u0A02\\u0a5c\\u0a21\\u0a47\\u0a71\\u0b02\\u0b03\\u0b33\\u0b36\\u0b43\\u0b56\\u0b57\\u0B0B\\u0B0C\\u0B37\\u0B3D\\u0B5F\\u0B60\\u0B61\\u0a35\\u0a72\\u0a73\\u0a74]", /*roundtrip exclusions*/

    "GURMUKHI-ORIYA", "[:GURMUKHI:]", "[:ORIYA:]",
    "[\\u0b01\\u0b02\\u0b03\\u0b33\\u0b36\\u0b43\\u0b56\\u0b57\\u0B0B\\u0B0C\\u0B37\\u0B3D\\u0B5F\\u0B60\\u0B61\\u0b70\\u0b71]", /*roundtrip exclusions*/

    "TAMIL-GURMUKHI", "[:TAMIL:]", "[:GURMUKHI:]", 
    "[\\u0A01\\u0A02\\u0a33\\u0a36\\u0a3c\\u0a70\\u0a71\\u0a47\\u0A16\\u0A17\\u0A18\\u0A1B\\u0A1D\\u0A20\\u0A21\\u0A22\\u0A25\\u0A26\\u0A27\\u0A2B\\u0A2C\\u0A2D\\u0A59\\u0A5A\\u0A5B\\u0A5C\\u0A5E\\u0A72\\u0A73\\u0A74]", /*roundtrip exclusions*/

    "GURMUKHI-TAMIL", "[:GURMUKHI:]", "[:TAMIL:]",
    "[\\u0b82\\u0bc6\\u0bca\\u0bd7\\u0bb7\\u0bb3\\u0b83\\u0B8E\\u0B92\\u0BA9\\u0BB1\\u0BB4\\u0bb6\\u0BF0\\u0BF1\\u0BF2]", /*roundtrip exclusions*/

    "TELUGU-GURMUKHI", "[:TELUGU:]", "[:GURMUKHI:]", 
    "[\\u0A02\\u0a33\\u0a36\\u0a3c\\u0a70\\u0a71\\u0A59\\u0A5A\\u0A5B\\u0A5C\\u0A5E\\u0A72\\u0A73\\u0A74]", /*roundtrip exclusions*/

    "GURMUKHI-TELUGU", "[:GURMUKHI:]", "[:TELUGU:]",
    "[\\u0c01\\u0c02\\u0c03\\u0c33\\u0c36\\u0c44\\u0c43\\u0c46\\u0c4a\\u0c56\\u0c55\\u0C0B\\u0C0C\\u0C0E\\u0C12\\u0C31\\u0C37\\u0C60\\u0C61]", /*roundtrip exclusions*/

    "KANNADA-GURMUKHI", "[:KANNADA:]", "[:GURMUKHI:]", 
    "[\\u0A01\\u0A02\\u0a33\\u0a36\\u0a3c\\u0a70\\u0a71\\u0A59\\u0A5A\\u0A5B\\u0A5C\\u0A5E\\u0A72\\u0A73\\u0A74]", /*roundtrip exclusions*/

    "GURMUKHI-KANNADA", "[:GURMUKHI:]", "[:KANNADA:]",
    "[{\\u0cb0\\u0cbc}{\\u0cb3\\u0cbc}\\u0c82\\u0c83\\u0cb3\\u0cb6\\u0cc4\\u0cc3\\u0cc6\\u0cca\\u0cd5\\u0cd6\\u0C8B\\u0C8C\\u0C8E\\u0C92\\u0CB1\\u0CB7\\u0cbd\\u0CE0\\u0CE1\\u0cde]", /*roundtrip exclusions*/

    "MALAYALAM-GURMUKHI", "[:MALAYALAM:]", "[:GURMUKHI:]", 
    "[\\u0A01\\u0A02\\u0a4b\\u0a4c\\u0a33\\u0a36\\u0a3c\\u0a70\\u0a71\\u0A59\\u0A5A\\u0A5B\\u0A5C\\u0A5E\\u0A72\\u0A73\\u0A74]", /*roundtrip exclusions*/

    "GURMUKHI-MALAYALAM", "[:GURMUKHI:]", "[:MALAYALAM:]",
    "[\\u0d02\\u0d03\\u0d33\\u0d36\\u0d43\\u0d46\\u0d4a\\u0d4c\\u0d57\\u0D0B\\u0D0C\\u0D0E\\u0D12\\u0D31\\u0D34\\u0D37\\u0D60\\u0D61]", /*roundtrip exclusions*/

    "GUJARATI-ORIYA", "[:GUJARATI:]", "[:ORIYA:]", 
    "[\\u0b56\\u0b57\\u0B0C\\u0B5F\\u0B61\\u0b70\\u0b71]", /*roundtrip exclusions*/

    "ORIYA-GUJARATI", "[:ORIYA:]", "[:GUJARATI:]",
    "[\\u0Ac4\\u0Ac5\\u0Ac9\\u0Ac7\\u0A8D\\u0A91\\u0AB5\\u0Ad0]", /*roundtrip exclusions*/

    "TAMIL-GUJARATI", "[:TAMIL:]", "[:GUJARATI:]", 
    "[\\u0A81\\u0a8c\\u0abc\\u0ac3\\u0Ac4\\u0Ac5\\u0Ac9\\u0Ac7\\u0A8B\\u0A8D\\u0A91\\u0A96\\u0A97\\u0A98\\u0A9B\\u0A9D\\u0AA0\\u0AA1\\u0AA2\\u0AA5\\u0AA6\\u0AA7\\u0AAB\\u0AAC\\u0AAD\\u0AB6\\u0ABD\\u0AD0\\u0AE0\\u0AE1]", /*roundtrip exclusions*/

    "GUJARATI-TAMIL", "[:GUJARATI:]", "[:TAMIL:]",
    "[\\u0Bc6\\u0Bca\\u0Bd7\\u0B8E\\u0B92\\u0BA9\\u0BB1\\u0BB4\\u0BF0\\u0BF1\\u0BF2]", /*roundtrip exclusions*/

    "TELUGU-GUJARATI", "[:TELUGU:]", "[:GUJARATI:]", 
    "[\\u0abc\\u0Ac5\\u0Ac9\\u0A8D\\u0A91\\u0ABD\\u0Ad0]", /*roundtrip exclusions*/

    "GUJARATI-TELUGU", "[:GUJARATI:]", "[:TELUGU:]",
    "[\\u0c46\\u0c4a\\u0c55\\u0c56\\u0C0C\\u0C0E\\u0C12\\u0C31\\u0C61]", /*roundtrip exclusions*/

    "KANNADA-GUJARATI", "[:KANNADA:]", "[:GUJARATI:]", 
    "[\\u0A81\\u0abc\\u0Ac5\\u0Ac9\\u0A8D\\u0A91\\u0ABD\\u0Ad0]", /*roundtrip exclusions*/

    "GUJARATI-KANNADA", "[:GUJARATI:]", "[:KANNADA:]",
    "[{\\u0cb0\\u0cbc}{\\u0cb3\\u0cbc}\\u0cc6\\u0cca\\u0cd5\\u0cd6\\u0C8C\\u0C8E\\u0C92\\u0CB1\\u0CDE\\u0CE1]", /*roundtrip exclusions*/

    "MALAYALAM-GUJARATI", "[:MALAYALAM:]", "[:GUJARATI:]", 
    "[\\u0A81\\u0ac4\\u0acb\\u0acc\\u0abc\\u0Ac5\\u0Ac9\\u0A8D\\u0A91\\u0ABD\\u0Ad0]", /*roundtrip exclusions*/

    "GUJARATI-MALAYALAM", "[:GUJARATI:]", "[:MALAYALAM:]",
    "[\\u0d46\\u0d4a\\u0d4c\\u0d55\\u0d57\\u0D0C\\u0D0E\\u0D12\\u0D31\\u0D34\\u0D61]", /*roundtrip exclusions*/

    "TAMIL-ORIYA", "[:TAMIL:]", "[:ORIYA:]", 
    "[\\u0B01\\u0b3c\\u0b43\\u0b56\\u0B0B\\u0B0C\\u0B16\\u0B17\\u0B18\\u0B1B\\u0B1D\\u0B20\\u0B21\\u0B22\\u0B25\\u0B26\\u0B27\\u0B2B\\u0B2C\\u0B2D\\u0B36\\u0B3D\\u0B5C\\u0B5D\\u0B5F\\u0B60\\u0B61\\u0b70\\u0b71]", /*roundtrip exclusions*/

    "ORIYA-TAMIL", "[:ORIYA:]", "[:TAMIL:]",
    "[\\u0bc6\\u0bca\\u0bc7\\u0B8E\\u0B92\\u0BA9\\u0BB1\\u0BB4\\u0BB5\\u0BF0\\u0BF1\\u0BF2]", /*roundtrip exclusions*/

    "TELUGU-ORIYA", "[:TELUGU:]", "[:ORIYA:]", 
    "[\\u0b3c\\u0b57\\u0b56\\u0B3D\\u0B5C\\u0B5D\\u0B5F\\u0b70\\u0b71]", /*roundtrip exclusions*/

    "ORIYA-TELUGU", "[:ORIYA:]", "[:TELUGU:]",
    "[\\u0c44\\u0c46\\u0c4a\\u0c55\\u0c47\\u0C0E\\u0C12\\u0C31\\u0C35]", /*roundtrip exclusions*/

    "KANNADA-ORIYA", "[:KANNADA:]", "[:ORIYA:]", 
    "[\\u0B01\\u0b3c\\u0b57\\u0B3D\\u0B5C\\u0B5D\\u0B5F\\u0b70\\u0b71]", /*roundtrip exclusions*/

    "ORIYA-KANNADA", "[:ORIYA:]", "[:KANNADA:]",
    "[{\\u0cb0\\u0cbc}{\\u0cb3\\u0cbc}\\u0cc4\\u0cc6\\u0cca\\u0cd5\\u0cc7\\u0C8E\\u0C92\\u0CB1\\u0CB5\\u0CDE]", /*roundtrip exclusions*/

    "MALAYALAM-ORIYA", "[:MALAYALAM:]", "[:ORIYA:]", 
    "[\\u0B01\\u0b3c\\u0b56\\u0B3D\\u0B5C\\u0B5D\\u0B5F\\u0b70\\u0b71]", /*roundtrip exclusions*/

    "ORIYA-MALAYALAM", "[:ORIYA:]", "[:MALAYALAM:]",
    "[\\u0D47\\u0D46\\u0D4a\\u0D0E\\u0D12\\u0D31\\u0D34\\u0D35]", /*roundtrip exclusions*/

    "TELUGU-TAMIL", "[:TELUGU:]", "[:TAMIL:]", 
    "[\\u0bd7\\u0ba9\\u0bb4\\u0BF0\\u0BF1\\u0BF2]", /*roundtrip exclusions*/

    "TAMIL-TELUGU", "[:TAMIL:]", "[:TELUGU:]",
    "[\\u0C01\\u0c43\\u0c44\\u0c46\\u0c47\\u0c55\\u0c56\\u0c66\\u0C0B\\u0C0C\\u0C16\\u0C17\\u0C18\\u0C1B\\u0C1D\\u0C20\\u0C21\\u0C22\\u0C25\\u0C26\\u0C27\\u0C2B\\u0C2C\\u0C2D\\u0C36\\u0C60\\u0C61]", /*roundtrip exclusions*/

    "KANNADA-TAMIL", "[:KANNADA:]", "[:TAMIL:]", 
    "[\\u0bd7\\u0bc6\\u0ba9\\u0bb4\\u0BF0\\u0BF1\\u0BF2]", /*roundtrip exclusions*/

    "TAMIL-KANNADA", "[:TAMIL:]", "[:KANNADA:]",
    "[\\u0cc3\\u0cc4\\u0cc6\\u0cc7\\u0cd5\\u0cd6\\u0C8B\\u0C8C\\u0C96\\u0C97\\u0C98\\u0C9B\\u0C9D\\u0CA0\\u0CA1\\u0CA2\\u0CA5\\u0CA6\\u0CA7\\u0CAB\\u0CAC\\u0CAD\\u0CB6\\u0cbc\\u0cbd\\u0CDE\\u0CE0\\u0CE1]", /*roundtrip exclusions*/

    "MALAYALAM-TAMIL", "[:MALAYALAM:]", "[:TAMIL:]", 
    "[\\u0ba9\\u0BF0\\u0BF1\\u0BF2]", /*roundtrip exclusions*/

    "TAMIL-MALAYALAM", "[:TAMIL:]", "[:MALAYALAM:]",
    "[\\u0d43\\u0d12\\u0D0B\\u0D0C\\u0D16\\u0D17\\u0D18\\u0D1B\\u0D1D\\u0D20\\u0D21\\u0D22\\u0D25\\u0D26\\u0D27\\u0D2B\\u0D2C\\u0D2D\\u0D36\\u0D60\\u0D61]", /*roundtrip exclusions*/

    "KANNADA-TELUGU", "[:KANNADA:]", "[:TELUGU:]", 
    "[\\u0C01\\u0c3f\\u0c46\\u0c48\\u0c4a]", /*roundtrip exclusions*/

    "TELUGU-KANNADA", "[:TELUGU:]", "[:KANNADA:]",
    "[\\u0cc8\\u0cd5\\u0cd6\\u0cbc\\u0cbd\\u0CDE]", /*roundtrip exclusions*/

    "MALAYALAM-TELUGU", "[:MALAYALAM:]", "[:TELUGU:]", 
    "[\\u0C01\\u0c44\\u0c4a\\u0c4c\\u0c4b\\u0c55\\u0c56]", /*roundtrip exclusions*/

    "TELUGU-MALAYALAM", "[:TELUGU:]", "[:MALAYALAM:]",
    "[\\u0d4c\\u0d57\\u0D34]", /*roundtrip exclusions*/

    "MALAYALAM-KANNADA", "[:MALAYALAM:]", "[:KANNADA:]", 
    "[\\u0cbc\\u0cbd\\u0cc4\\u0cc6\\u0cca\\u0ccc\\u0ccb\\u0cd5\\u0cd6\\u0cDe]", /*roundtrip exclusions*/

    "KANNADA-MALAYALAM", "[:KANNADA:]", "[:MALAYALAM:]",
    "[\\u0d4c\\u0d57\\u0d46\\u0D34]", /*roundtrip exclusions*/

    "Latin-Bengali",latinForIndic, "[[:Bengali:][\\u0964\\u0965]]", 
    "[\\u0965\\u09f0-\\u09fa\\u09ce]" /*roundtrip exclusions*/ ,

    "Latin-Gurmukhi", latinForIndic, "[[:Gurmukhi:][\\u0964\\u0965]]", 
    "[\\u0a01\\u0965\\u0a02\\u0a72\\u0a73\\u0a74]" /*roundtrip exclusions*/,

    "Latin-Gujarati",latinForIndic, "[[:Gujarati:][\\u0964\\u0965]]", 
    "[\\u0965]" /*roundtrip exclusions*/,

    "Latin-Oriya",latinForIndic, "[[:Oriya:][\\u0964\\u0965]]", 
    "[\\u0965\\u0b70]" /*roundtrip exclusions*/,

    "Latin-Tamil",latinForIndic, "[:Tamil:]", 
    "[\\u0BF0\\u0BF1\\u0BF2]" /*roundtrip exclusions*/,

    "Latin-Telugu",latinForIndic, "[:Telugu:]", 
    NULL /*roundtrip exclusions*/,

    "Latin-Kannada",latinForIndic, "[:Kannada:]", 
    NULL /*roundtrip exclusions*/,

    "Latin-Malayalam",latinForIndic, "[:Malayalam:]", 
    NULL /*roundtrip exclusions*/  
};

void TransliteratorRoundTripTest::TestDebug(const char* name,const char fromSet[],
                                            const char* toSet,const char* exclusions){

    RTTest test(name);
    Legal *legal = new LegalIndic();
    test.test(UnicodeString(fromSet,""),UnicodeString(toSet,""),exclusions,this,quick,legal);
}

void TransliteratorRoundTripTest::TestInterIndic() {
    //TestDebug("Latin-Gurmukhi", latinForIndic, "[:Gurmukhi:]","[\\u0965\\u0a02\\u0a72\\u0a73\\u0a74]",TRUE);
    int32_t num = (int32_t)(sizeof(interIndicArray)/(INTER_INDIC_ARRAY_WIDTH*sizeof(char*)));
    if(quick){
        logln("Testing only 5 of %i. Skipping rest (use -e for exhaustive)",num);
        num = 5;
    }
    // CLDR bug #1911: This test should be moved into CLDR.
    // It is left in its current state as a regression test.
//    if (isICUVersionAtLeast(ICU_39)) {
//        // We temporarily filter against Unicode 4.1, but we only do this
//        // before version 3.4.
//        errln("FAIL: TestInterIndic needs to be updated to remove delete the [:Age=4.1:] filter ");
//        return;
//    } else {
//        logln("Warning: TestInterIndic needs to be updated to remove delete the section marked [:Age=4.1:] filter");
//    }
    for(int i = 0; i < num;i++){
        RTTest test(interIndicArray[i*INTER_INDIC_ARRAY_WIDTH + 0]);
        Legal *legal = new LegalIndic();
        logln(UnicodeString("Stress testing ") + interIndicArray[i*INTER_INDIC_ARRAY_WIDTH + 0]);
        /* Uncomment lines below  when transliterator is fixed */
        /*
        test.test(  interIndicArray[i*INTER_INDIC_ARRAY_WIDTH + 1], 
                    interIndicArray[i*INTER_INDIC_ARRAY_WIDTH + 2], 
                    interIndicArray[i*INTER_INDIC_ARRAY_WIDTH + 3], // roundtrip exclusions 
                    this, quick, legal, 50);
        */
        /* comment lines below  when transliterator is fixed */
        // start
        UnicodeString source("[");
        source.append(interIndicArray[i*INTER_INDIC_ARRAY_WIDTH + 1]);
        source.append(" & [:Age=4.1:]]");
        UnicodeString target("[");
        target.append(interIndicArray[i*INTER_INDIC_ARRAY_WIDTH + 2]);
        target.append(" & [:Age=4.1:]]");
        test.test(  source, 
                    target, 
                    interIndicArray[i*INTER_INDIC_ARRAY_WIDTH + 3], // roundtrip exclusions 
                    this, quick, legal, 50);
        // end
        delete legal;
    }
}

// end indic tests ----------------------------------------------------------

#endif /* #if !UCONFIG_NO_TRANSLITERATION */
