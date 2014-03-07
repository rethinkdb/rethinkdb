/*
*******************************************************************************
* Copyright (C) 1997-2010, International Business Machines Corporation and    *
* others. All Rights Reserved.                                                *
*******************************************************************************
*
* File DECIMFMT.CPP
*
* Modification History:
*
*   Date        Name        Description
*   02/19/97    aliu        Converted from java.
*   03/20/97    clhuang     Implemented with new APIs.
*   03/31/97    aliu        Moved isLONG_MIN to DigitList, and fixed it.
*   04/3/97     aliu        Rewrote parsing and formatting completely, and
*                           cleaned up and debugged.  Actually works now.
*                           Implemented NAN and INF handling, for both parsing
*                           and formatting.  Extensive testing & debugging.
*   04/10/97    aliu        Modified to compile on AIX.
*   04/16/97    aliu        Rewrote to use DigitList, which has been resurrected.
*                           Changed DigitCount to int per code review.
*   07/09/97    helena      Made ParsePosition into a class.
*   08/26/97    aliu        Extensive changes to applyPattern; completely
*                           rewritten from the Java.
*   09/09/97    aliu        Ported over support for exponential formats.
*   07/20/98    stephen     JDK 1.2 sync up.
*                             Various instances of '0' replaced with 'NULL'
*                             Check for grouping size in subFormat()
*                             Brought subParse() in line with Java 1.2
*                             Added method appendAffix()
*   08/24/1998  srl         Removed Mutex calls. This is not a thread safe class!
*   02/22/99    stephen     Removed character literals for EBCDIC safety
*   06/24/99    helena      Integrated Alan's NF enhancements and Java2 bug fixes
*   06/28/99    stephen     Fixed bugs in toPattern().
*   06/29/99    stephen     Fixed operator= to copy fFormatWidth, fPad,
*                             fPadPosition
********************************************************************************
*/

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "fphdlimp.h"
#include "unicode/decimfmt.h"
#include "unicode/choicfmt.h"
#include "unicode/ucurr.h"
#include "unicode/ustring.h"
#include "unicode/dcfmtsym.h"
#include "unicode/ures.h"
#include "unicode/uchar.h"
#include "unicode/curramt.h"
#include "unicode/currpinf.h"
#include "unicode/plurrule.h"
#include "ucurrimp.h"
#include "charstr.h"
#include "cmemory.h"
#include "util.h"
#include "digitlst.h"
#include "cstring.h"
#include "umutex.h"
#include "uassert.h"
#include "putilimp.h"
#include <math.h>
#include "hash.h"


U_NAMESPACE_BEGIN

/* For currency parsing purose,
 * Need to remember all prefix patterns and suffix patterns of
 * every currency format pattern,
 * including the pattern of default currecny style
 * and plural currency style. And the patterns are set through applyPattern.
 */
struct AffixPatternsForCurrency : public UMemory {
	// negative prefix pattern
	UnicodeString negPrefixPatternForCurrency;
	// negative suffix pattern
	UnicodeString negSuffixPatternForCurrency;
	// positive prefix pattern
	UnicodeString posPrefixPatternForCurrency;
	// positive suffix pattern
	UnicodeString posSuffixPatternForCurrency;
	int8_t patternType;

	AffixPatternsForCurrency(const UnicodeString& negPrefix,
							 const UnicodeString& negSuffix,
							 const UnicodeString& posPrefix,
							 const UnicodeString& posSuffix,
							 int8_t type) {
		negPrefixPatternForCurrency = negPrefix;
		negSuffixPatternForCurrency = negSuffix;
		posPrefixPatternForCurrency = posPrefix;
		posSuffixPatternForCurrency = posSuffix;
		patternType = type;
	}
};

/* affix for currency formatting when the currency sign in the pattern
 * equals to 3, such as the pattern contains 3 currency sign or
 * the formatter style is currency plural format style.
 */
struct AffixesForCurrency : public UMemory {
	// negative prefix
	UnicodeString negPrefixForCurrency;
	// negative suffix
	UnicodeString negSuffixForCurrency;
	// positive prefix
	UnicodeString posPrefixForCurrency;
	// positive suffix
	UnicodeString posSuffixForCurrency;

	int32_t formatWidth;

	AffixesForCurrency(const UnicodeString& negPrefix,
					   const UnicodeString& negSuffix,
					   const UnicodeString& posPrefix,
					   const UnicodeString& posSuffix) {
		negPrefixForCurrency = negPrefix;
		negSuffixForCurrency = negSuffix;
		posPrefixForCurrency = posPrefix;
		posSuffixForCurrency = posSuffix;
	}
};

U_CDECL_BEGIN

/**
 * @internal ICU 4.2
 */
static UBool U_CALLCONV decimfmtAffixValueComparator(UHashTok val1, UHashTok val2);

/**
 * @internal ICU 4.2
 */
static UBool U_CALLCONV decimfmtAffixPatternValueComparator(UHashTok val1, UHashTok val2);


static UBool
U_CALLCONV decimfmtAffixValueComparator(UHashTok val1, UHashTok val2) {
    const AffixesForCurrency* affix_1 =
        (AffixesForCurrency*)val1.pointer;
    const AffixesForCurrency* affix_2 =
        (AffixesForCurrency*)val2.pointer;
    return affix_1->negPrefixForCurrency == affix_2->negPrefixForCurrency &&
           affix_1->negSuffixForCurrency == affix_2->negSuffixForCurrency &&
           affix_1->posPrefixForCurrency == affix_2->posPrefixForCurrency &&
           affix_1->posSuffixForCurrency == affix_2->posSuffixForCurrency;
}


static UBool
U_CALLCONV decimfmtAffixPatternValueComparator(UHashTok val1, UHashTok val2) {
    const AffixPatternsForCurrency* affix_1 =
        (AffixPatternsForCurrency*)val1.pointer;
    const AffixPatternsForCurrency* affix_2 =
        (AffixPatternsForCurrency*)val2.pointer;
    return affix_1->negPrefixPatternForCurrency ==
           affix_2->negPrefixPatternForCurrency &&
           affix_1->negSuffixPatternForCurrency ==
           affix_2->negSuffixPatternForCurrency &&
           affix_1->posPrefixPatternForCurrency ==
           affix_2->posPrefixPatternForCurrency &&
           affix_1->posSuffixPatternForCurrency ==
           affix_2->posSuffixPatternForCurrency &&
           affix_1->patternType == affix_2->patternType;
}

U_CDECL_END


//#define FMT_DEBUG

#ifdef FMT_DEBUG
#include <stdio.h>
static void debugout(UnicodeString s) {
    char buf[2000];
    s.extract((int32_t) 0, s.length(), buf);
    printf("%s\n", buf);
}
#define debug(x) printf("%s\n", x);
#else
#define debugout(x)
#define debug(x)
#endif



// *****************************************************************************
// class DecimalFormat
// *****************************************************************************

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(DecimalFormat)

// Constants for characters used in programmatic (unlocalized) patterns.
#define kPatternZeroDigit            ((UChar)0x0030) /*'0'*/
#define kPatternSignificantDigit     ((UChar)0x0040) /*'@'*/
#define kPatternGroupingSeparator    ((UChar)0x002C) /*','*/
#define kPatternDecimalSeparator     ((UChar)0x002E) /*'.'*/
#define kPatternPerMill              ((UChar)0x2030)
#define kPatternPercent              ((UChar)0x0025) /*'%'*/
#define kPatternDigit                ((UChar)0x0023) /*'#'*/
#define kPatternSeparator            ((UChar)0x003B) /*';'*/
#define kPatternExponent             ((UChar)0x0045) /*'E'*/
#define kPatternPlus                 ((UChar)0x002B) /*'+'*/
#define kPatternMinus                ((UChar)0x002D) /*'-'*/
#define kPatternPadEscape            ((UChar)0x002A) /*'*'*/
#define kQuote                       ((UChar)0x0027) /*'\''*/
/**
 * The CURRENCY_SIGN is the standard Unicode symbol for currency.  It
 * is used in patterns and substitued with either the currency symbol,
 * or if it is doubled, with the international currency symbol.  If the
 * CURRENCY_SIGN is seen in a pattern, then the decimal separator is
 * replaced with the monetary decimal separator.
 */
#define kCurrencySign                ((UChar)0x00A4)
#define kDefaultPad                  ((UChar)0x0020) /* */

const int32_t DecimalFormat::kDoubleIntegerDigits  = 309;
const int32_t DecimalFormat::kDoubleFractionDigits = 340;

const int32_t DecimalFormat::kMaxScientificIntegerDigits = 8;

/**
 * These are the tags we expect to see in normal resource bundle files associated
 * with a locale.
 */
const char DecimalFormat::fgNumberPatterns[]="NumberPatterns"; // Deprecated - not used
static const char fgNumberElements[]="NumberElements";
static const char fgLatn[]="latn";
static const char fgPatterns[]="patterns";
static const char fgDecimalFormat[]="decimalFormat";
static const char fgCurrencyFormat[]="currencyFormat";
static const UChar fgTripleCurrencySign[] = {0xA4, 0xA4, 0xA4, 0};

inline int32_t _min(int32_t a, int32_t b) { return (a<b) ? a : b; }
inline int32_t _max(int32_t a, int32_t b) { return (a<b) ? b : a; }

//------------------------------------------------------------------------------
// Constructs a DecimalFormat instance in the default locale.

DecimalFormat::DecimalFormat(UErrorCode& status) {
    init();
    UParseError parseError;
    construct(status, parseError);
}

//------------------------------------------------------------------------------
// Constructs a DecimalFormat instance with the specified number format
// pattern in the default locale.

DecimalFormat::DecimalFormat(const UnicodeString& pattern,
                             UErrorCode& status) {
    init();
    UParseError parseError;
    construct(status, parseError, &pattern);
}

//------------------------------------------------------------------------------
// Constructs a DecimalFormat instance with the specified number format
// pattern and the number format symbols in the default locale.  The
// created instance owns the symbols.

DecimalFormat::DecimalFormat(const UnicodeString& pattern,
                             DecimalFormatSymbols* symbolsToAdopt,
                             UErrorCode& status) {
    init();
    UParseError parseError;
    if (symbolsToAdopt == NULL)
        status = U_ILLEGAL_ARGUMENT_ERROR;
    construct(status, parseError, &pattern, symbolsToAdopt);
}

DecimalFormat::DecimalFormat(  const UnicodeString& pattern,
                    DecimalFormatSymbols* symbolsToAdopt,
                    UParseError& parseErr,
                    UErrorCode& status) {
    init();
    if (symbolsToAdopt == NULL)
        status = U_ILLEGAL_ARGUMENT_ERROR;
    construct(status,parseErr, &pattern, symbolsToAdopt);
}

//------------------------------------------------------------------------------
// Constructs a DecimalFormat instance with the specified number format
// pattern and the number format symbols in the default locale.  The
// created instance owns the clone of the symbols.

DecimalFormat::DecimalFormat(const UnicodeString& pattern,
                             const DecimalFormatSymbols& symbols,
                             UErrorCode& status) {
    init();
    UParseError parseError;
    construct(status, parseError, &pattern, new DecimalFormatSymbols(symbols));
}

//------------------------------------------------------------------------------
// Constructs a DecimalFormat instance with the specified number format
// pattern, the number format symbols, and the number format style.
// The created instance owns the clone of the symbols.

DecimalFormat::DecimalFormat(const UnicodeString& pattern,
                             DecimalFormatSymbols* symbolsToAdopt,
                             NumberFormat::EStyles style,
                             UErrorCode& status) {
    init();
    fStyle = style;
    UParseError parseError;
    construct(status, parseError, &pattern, symbolsToAdopt);
}

//-----------------------------------------------------------------------------
// Common DecimalFormat initialization.
//    Put all fields of an uninitialized object into a known state.
//    Common code, shared by all constructors.
void
DecimalFormat::init() {
    fPosPrefixPattern = 0;
    fPosSuffixPattern = 0;
    fNegPrefixPattern = 0;
    fNegSuffixPattern = 0;
    fCurrencyChoice = 0;
    fMultiplier = NULL;
    fGroupingSize = 0;
    fGroupingSize2 = 0;
    fDecimalSeparatorAlwaysShown = FALSE;
    fSymbols = NULL;
    fUseSignificantDigits = FALSE;
    fMinSignificantDigits = 1;
    fMaxSignificantDigits = 6;
    fUseExponentialNotation = FALSE;
    fMinExponentDigits = 0;
    fExponentSignAlwaysShown = FALSE;
    fRoundingIncrement = 0;
    fRoundingMode = kRoundHalfEven;
    fPad = 0;
    fFormatWidth = 0;
    fPadPosition = kPadBeforePrefix;
    fStyle = NumberFormat::kNumberStyle;
    fCurrencySignCount = 0;
    fAffixPatternsForCurrency = NULL;
    fAffixesForCurrency = NULL;
    fPluralAffixesForCurrency = NULL;
    fCurrencyPluralInfo = NULL;
}

//------------------------------------------------------------------------------
// Constructs a DecimalFormat instance with the specified number format
// pattern and the number format symbols in the desired locale.  The
// created instance owns the symbols.

void
DecimalFormat::construct(UErrorCode&             status,
                         UParseError&           parseErr,
                         const UnicodeString*   pattern,
                         DecimalFormatSymbols*  symbolsToAdopt)
{
    fSymbols = symbolsToAdopt; // Do this BEFORE aborting on status failure!!!
    fRoundingIncrement = NULL;
    fRoundingMode = kRoundHalfEven;
    fPad = kPatternPadEscape;
    fPadPosition = kPadBeforePrefix;
    if (U_FAILURE(status))
        return;

    fPosPrefixPattern = fPosSuffixPattern = NULL;
    fNegPrefixPattern = fNegSuffixPattern = NULL;
    setMultiplier(1);
    fGroupingSize = 3;
    fGroupingSize2 = 0;
    fDecimalSeparatorAlwaysShown = FALSE;
    fUseExponentialNotation = FALSE;
    fMinExponentDigits = 0;

    if (fSymbols == NULL)
    {
        fSymbols = new DecimalFormatSymbols(Locale::getDefault(), status);
        /* test for NULL */
        if (fSymbols == 0) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return;
        }
    }

    UnicodeString str;
    // Uses the default locale's number format pattern if there isn't
    // one specified.
    if (pattern == NULL)
    {
        int32_t len = 0;
        UResourceBundle *resource = ures_open(NULL, Locale::getDefault().getName(), &status);

        resource = ures_getByKey(resource, fgNumberElements, resource, &status);
        // TODO : Get the pattern based on the active numbering system for the locale. Right now assumes "latn".
        resource = ures_getByKey(resource, fgLatn, resource, &status);
        resource = ures_getByKey(resource, fgPatterns, resource, &status);
        const UChar *resStr = ures_getStringByKey(resource, fgDecimalFormat, &len, &status);
        str.setTo(TRUE, resStr, len);
        pattern = &str;
        ures_close(resource);
    }

    if (U_FAILURE(status))
    {
        return;
    }

    if (pattern->indexOf((UChar)kCurrencySign) >= 0) {
        // If it looks like we are going to use a currency pattern
        // then do the time consuming lookup.
        setCurrencyForSymbols();
    } else {
        setCurrencyInternally(NULL, status);
    }

    const UnicodeString* patternUsed;
    UnicodeString currencyPluralPatternForOther;
    // apply pattern
    if (fStyle == NumberFormat::kPluralCurrencyStyle) {
        fCurrencyPluralInfo = new CurrencyPluralInfo(fSymbols->getLocale(), status);
        if (U_FAILURE(status)) {
            return;
        }

        // the pattern used in format is not fixed until formatting,
        // in which, the number is known and
        // will be used to pick the right pattern based on plural count.
        // Here, set the pattern as the pattern of plural count == "other".
        // For most locale, the patterns are probably the same for all
        // plural count. If not, the right pattern need to be re-applied
        // during format.
        fCurrencyPluralInfo->getCurrencyPluralPattern("other", currencyPluralPatternForOther);
        patternUsed = &currencyPluralPatternForOther;
        // TODO: not needed?
        setCurrencyForSymbols();

    } else {
        patternUsed = pattern;
    }

    if (patternUsed->indexOf(kCurrencySign) != -1) {
        // initialize for currency, not only for plural format,
        // but also for mix parsing
        if (fCurrencyPluralInfo == NULL) {
           fCurrencyPluralInfo = new CurrencyPluralInfo(fSymbols->getLocale(), status);
           if (U_FAILURE(status)) {
               return;
           }
        }
        // need it for mix parsing
        setupCurrencyAffixPatterns(status);
        // expanded affixes for plural names
        if (patternUsed->indexOf(fgTripleCurrencySign) != -1) {
            setupCurrencyAffixes(*patternUsed, TRUE, TRUE, status);
        }
    }

    applyPatternWithoutExpandAffix(*patternUsed,FALSE, parseErr, status);

    // expand affixes
    if (fCurrencySignCount != fgCurrencySignCountInPluralFormat) {
        expandAffixAdjustWidth(NULL);
    }

    // If it was a currency format, apply the appropriate rounding by
    // resetting the currency. NOTE: this copies fCurrency on top of itself.
    if (fCurrencySignCount > fgCurrencySignCountZero) {
        setCurrencyInternally(getCurrency(), status);
    }
}


void
DecimalFormat::setupCurrencyAffixPatterns(UErrorCode& status) {
    if (U_FAILURE(status)) {
        return;
    }
    UParseError parseErr;
    fAffixPatternsForCurrency = initHashForAffixPattern(status);
    if (U_FAILURE(status)) {
        return;
    }

    // Save the default currency patterns of this locale.
    // Here, chose onlyApplyPatternWithoutExpandAffix without
    // expanding the affix patterns into affixes.
    UnicodeString currencyPattern;
    UErrorCode error = U_ZERO_ERROR;   
    
    UResourceBundle *resource = ures_open(NULL, fSymbols->getLocale().getName(), &error);
    resource = ures_getByKey(resource, fgNumberElements, resource, &error);
    // TODO : Get the pattern based on the active numbering system for the locale. Right now assumes "latn".
    resource = ures_getByKey(resource, fgLatn, resource, &error);
    resource = ures_getByKey(resource, fgPatterns, resource, &error);
    int32_t patLen = 0;
    const UChar *patResStr = ures_getStringByKey(resource, fgCurrencyFormat,  &patLen, &error);
    ures_close(resource);

    if (U_SUCCESS(error)) {
        applyPatternWithoutExpandAffix(UnicodeString(patResStr, patLen), false,
                                       parseErr, status);
        AffixPatternsForCurrency* affixPtn = new AffixPatternsForCurrency(
                                                    *fNegPrefixPattern,
                                                    *fNegSuffixPattern,
                                                    *fPosPrefixPattern,
                                                    *fPosSuffixPattern,
                                                    UCURR_SYMBOL_NAME);
        fAffixPatternsForCurrency->put("default", affixPtn, status);
    }

    // save the unique currency plural patterns of this locale.
    Hashtable* pluralPtn = fCurrencyPluralInfo->fPluralCountToCurrencyUnitPattern;
    const UHashElement* element = NULL;
    int32_t pos = -1;
    Hashtable pluralPatternSet;
    while ((element = pluralPtn->nextElement(pos)) != NULL) {
        const UHashTok valueTok = element->value;
        const UnicodeString* value = (UnicodeString*)valueTok.pointer;
        const UHashTok keyTok = element->key;
        const UnicodeString* key = (UnicodeString*)keyTok.pointer;
        if (pluralPatternSet.geti(*value) != 1) {
            pluralPatternSet.puti(*value, 1, status);
            applyPatternWithoutExpandAffix(*value, false, parseErr, status);
            AffixPatternsForCurrency* affixPtn = new AffixPatternsForCurrency(
                                                    *fNegPrefixPattern,
                                                    *fNegSuffixPattern,
                                                    *fPosPrefixPattern,
                                                    *fPosSuffixPattern,
                                                    UCURR_LONG_NAME);
            fAffixPatternsForCurrency->put(*key, affixPtn, status);
        }
    }
}


void
DecimalFormat::setupCurrencyAffixes(const UnicodeString& pattern,
                                    UBool setupForCurrentPattern,
                                    UBool setupForPluralPattern,
                                    UErrorCode& status) {
    if (U_FAILURE(status)) {
        return;
    }
    UParseError parseErr;
    if (setupForCurrentPattern) {
        if (fAffixesForCurrency) {
            deleteHashForAffix(fAffixesForCurrency);
        }
        fAffixesForCurrency = initHashForAffix(status);
        if (U_SUCCESS(status)) {
            applyPatternWithoutExpandAffix(pattern, false, parseErr, status);
            const PluralRules* pluralRules = fCurrencyPluralInfo->getPluralRules();
            StringEnumeration* keywords = pluralRules->getKeywords(status);
            if (U_SUCCESS(status)) {
                const char* pluralCountCh;
                while ((pluralCountCh = keywords->next(NULL, status)) != NULL) {
                    if ( U_SUCCESS(status) ) {
                        UnicodeString pluralCount = UnicodeString(pluralCountCh);
                        expandAffixAdjustWidth(&pluralCount);
                        AffixesForCurrency* affix = new AffixesForCurrency(
                            fNegativePrefix, fNegativeSuffix, fPositivePrefix, fPositiveSuffix);
                        fAffixesForCurrency->put(pluralCount, affix, status);
                    }
                }
            }
            delete keywords;
        }
    }

    if (U_FAILURE(status)) {
        return;
    }

    if (setupForPluralPattern) {
        if (fPluralAffixesForCurrency) {
            deleteHashForAffix(fPluralAffixesForCurrency);
        }
        fPluralAffixesForCurrency = initHashForAffix(status);
        if (U_SUCCESS(status)) {
            const PluralRules* pluralRules = fCurrencyPluralInfo->getPluralRules();
            StringEnumeration* keywords = pluralRules->getKeywords(status);
            if (U_SUCCESS(status)) {
                const char* pluralCountCh;
                while ((pluralCountCh = keywords->next(NULL, status)) != NULL) {
                    if ( U_SUCCESS(status) ) {
                        UnicodeString pluralCount = UnicodeString(pluralCountCh);
                        UnicodeString ptn;
                        fCurrencyPluralInfo->getCurrencyPluralPattern(pluralCount, ptn);
                        applyPatternInternally(pluralCount, ptn, false, parseErr, status);
                        AffixesForCurrency* affix = new AffixesForCurrency(
                            fNegativePrefix, fNegativeSuffix, fPositivePrefix, fPositiveSuffix);
                        fPluralAffixesForCurrency->put(pluralCount, affix, status);
                    }
                }
            }
            delete keywords;
        }
    }
}


//------------------------------------------------------------------------------

DecimalFormat::~DecimalFormat()
{
    delete fPosPrefixPattern;
    delete fPosSuffixPattern;
    delete fNegPrefixPattern;
    delete fNegSuffixPattern;
    delete fCurrencyChoice;
    delete fMultiplier;
    delete fSymbols;
    delete fRoundingIncrement;
    deleteHashForAffixPattern();
    deleteHashForAffix(fAffixesForCurrency);
    deleteHashForAffix(fPluralAffixesForCurrency);
    delete fCurrencyPluralInfo;
}

//------------------------------------------------------------------------------
// copy constructor

DecimalFormat::DecimalFormat(const DecimalFormat &source) :
    NumberFormat(source) {
    init();
    *this = source;
}

//------------------------------------------------------------------------------
// assignment operator

static void _copy_us_ptr(UnicodeString** pdest, const UnicodeString* source) {
    if (source == NULL) {
        delete *pdest;
        *pdest = NULL;
    } else if (*pdest == NULL) {
        *pdest = new UnicodeString(*source);
    } else {
        **pdest  = *source;
    }
}

DecimalFormat&
DecimalFormat::operator=(const DecimalFormat& rhs)
{
    if(this != &rhs) {
        NumberFormat::operator=(rhs);
        fPositivePrefix = rhs.fPositivePrefix;
        fPositiveSuffix = rhs.fPositiveSuffix;
        fNegativePrefix = rhs.fNegativePrefix;
        fNegativeSuffix = rhs.fNegativeSuffix;
        _copy_us_ptr(&fPosPrefixPattern, rhs.fPosPrefixPattern);
        _copy_us_ptr(&fPosSuffixPattern, rhs.fPosSuffixPattern);
        _copy_us_ptr(&fNegPrefixPattern, rhs.fNegPrefixPattern);
        _copy_us_ptr(&fNegSuffixPattern, rhs.fNegSuffixPattern);
        if (rhs.fCurrencyChoice == 0) {
            delete fCurrencyChoice;
            fCurrencyChoice = 0;
        } else {
            fCurrencyChoice = (ChoiceFormat*) rhs.fCurrencyChoice->clone();
        }
        setRoundingIncrement(rhs.getRoundingIncrement());
        fRoundingMode = rhs.fRoundingMode;
        setMultiplier(rhs.getMultiplier());
        fGroupingSize = rhs.fGroupingSize;
        fGroupingSize2 = rhs.fGroupingSize2;
        fDecimalSeparatorAlwaysShown = rhs.fDecimalSeparatorAlwaysShown;
        if(fSymbols == NULL) {
            fSymbols = new DecimalFormatSymbols(*rhs.fSymbols);
        } else {
            *fSymbols = *rhs.fSymbols;
        }
        fUseExponentialNotation = rhs.fUseExponentialNotation;
        fExponentSignAlwaysShown = rhs.fExponentSignAlwaysShown;
        /*Bertrand A. D. Update 98.03.17*/
        fCurrencySignCount = rhs.fCurrencySignCount;
        /*end of Update*/
        fMinExponentDigits = rhs.fMinExponentDigits;

        /* sfb 990629 */
        fFormatWidth = rhs.fFormatWidth;
        fPad = rhs.fPad;
        fPadPosition = rhs.fPadPosition;
        /* end sfb */
        fMinSignificantDigits = rhs.fMinSignificantDigits;
        fMaxSignificantDigits = rhs.fMaxSignificantDigits;
        fUseSignificantDigits = rhs.fUseSignificantDigits;
        fFormatPattern = rhs.fFormatPattern;
        fStyle = rhs.fStyle;
        fCurrencySignCount = rhs.fCurrencySignCount;
        if (rhs.fCurrencyPluralInfo) {
            delete fCurrencyPluralInfo;
            fCurrencyPluralInfo = rhs.fCurrencyPluralInfo->clone();
        }
        if (rhs.fAffixPatternsForCurrency) {
            UErrorCode status = U_ZERO_ERROR;
            deleteHashForAffixPattern();
            fAffixPatternsForCurrency = initHashForAffixPattern(status);
            copyHashForAffixPattern(rhs.fAffixPatternsForCurrency,
                                    fAffixPatternsForCurrency, status);
        }
        if (rhs.fAffixesForCurrency) {
            UErrorCode status = U_ZERO_ERROR;
            deleteHashForAffix(fAffixesForCurrency);
            fAffixesForCurrency = initHashForAffixPattern(status);
            copyHashForAffix(rhs.fAffixesForCurrency, fAffixesForCurrency, status);
        }
        if (rhs.fPluralAffixesForCurrency) {
            UErrorCode status = U_ZERO_ERROR;
            deleteHashForAffix(fPluralAffixesForCurrency);
            fPluralAffixesForCurrency = initHashForAffixPattern(status);
            copyHashForAffix(rhs.fPluralAffixesForCurrency, fPluralAffixesForCurrency, status);
        }
    }
    return *this;
}

//------------------------------------------------------------------------------

UBool
DecimalFormat::operator==(const Format& that) const
{
    if (this == &that)
        return TRUE;

    // NumberFormat::operator== guarantees this cast is safe
    const DecimalFormat* other = (DecimalFormat*)&that;

#ifdef FMT_DEBUG
    // This code makes it easy to determine why two format objects that should
    // be equal aren't.
    UBool first = TRUE;
    if (!NumberFormat::operator==(that)) {
        if (first) { printf("[ "); first = FALSE; } else { printf(", "); }
        debug("NumberFormat::!=");
    } else {
    if (!((fPosPrefixPattern == other->fPosPrefixPattern && // both null
              fPositivePrefix == other->fPositivePrefix)
           || (fPosPrefixPattern != 0 && other->fPosPrefixPattern != 0 &&
               *fPosPrefixPattern  == *other->fPosPrefixPattern))) {
        if (first) { printf("[ "); first = FALSE; } else { printf(", "); }
        debug("Pos Prefix !=");
    }
    if (!((fPosSuffixPattern == other->fPosSuffixPattern && // both null
           fPositiveSuffix == other->fPositiveSuffix)
          || (fPosSuffixPattern != 0 && other->fPosSuffixPattern != 0 &&
              *fPosSuffixPattern  == *other->fPosSuffixPattern))) {
        if (first) { printf("[ "); first = FALSE; } else { printf(", "); }
        debug("Pos Suffix !=");
    }
    if (!((fNegPrefixPattern == other->fNegPrefixPattern && // both null
           fNegativePrefix == other->fNegativePrefix)
          || (fNegPrefixPattern != 0 && other->fNegPrefixPattern != 0 &&
              *fNegPrefixPattern  == *other->fNegPrefixPattern))) {
        if (first) { printf("[ "); first = FALSE; } else { printf(", "); }
        debug("Neg Prefix ");
        if (fNegPrefixPattern == NULL) {
            debug("NULL(");
            debugout(fNegativePrefix);
            debug(")");
        } else {
            debugout(*fNegPrefixPattern);
        }
        debug(" != ");
        if (other->fNegPrefixPattern == NULL) {
            debug("NULL(");
            debugout(other->fNegativePrefix);
            debug(")");
        } else {
            debugout(*other->fNegPrefixPattern);
        }
    }
    if (!((fNegSuffixPattern == other->fNegSuffixPattern && // both null
           fNegativeSuffix == other->fNegativeSuffix)
          || (fNegSuffixPattern != 0 && other->fNegSuffixPattern != 0 &&
              *fNegSuffixPattern  == *other->fNegSuffixPattern))) {
        if (first) { printf("[ "); first = FALSE; } else { printf(", "); }
        debug("Neg Suffix ");
        if (fNegSuffixPattern == NULL) {
            debug("NULL(");
            debugout(fNegativeSuffix);
            debug(")");
        } else {
            debugout(*fNegSuffixPattern);
        }
        debug(" != ");
        if (other->fNegSuffixPattern == NULL) {
            debug("NULL(");
            debugout(other->fNegativeSuffix);
            debug(")");
        } else {
            debugout(*other->fNegSuffixPattern);
        }
    }
    if (!((fRoundingIncrement == other->fRoundingIncrement) // both null
          || (fRoundingIncrement != NULL &&
              other->fRoundingIncrement != NULL &&
              *fRoundingIncrement == *other->fRoundingIncrement))) {
        if (first) { printf("[ "); first = FALSE; } else { printf(", "); }
        debug("Rounding Increment !=");
              }
    if (getMultiplier() != other->getMultiplier()) {
        if (first) { printf("[ "); first = FALSE; }
        printf("Multiplier %ld != %ld", getMultiplier(), other->getMultiplier());
    }
    if (fGroupingSize != other->fGroupingSize) {
        if (first) { printf("[ "); first = FALSE; } else { printf(", "); }
        printf("Grouping Size %ld != %ld", fGroupingSize, other->fGroupingSize);
    }
    if (fGroupingSize2 != other->fGroupingSize2) {
        if (first) { printf("[ "); first = FALSE; } else { printf(", "); }
        printf("Secondary Grouping Size %ld != %ld", fGroupingSize2, other->fGroupingSize2);
    }
    if (fDecimalSeparatorAlwaysShown != other->fDecimalSeparatorAlwaysShown) {
        if (first) { printf("[ "); first = FALSE; } else { printf(", "); }
        printf("Dec Sep Always %d != %d", fDecimalSeparatorAlwaysShown, other->fDecimalSeparatorAlwaysShown);
    }
    if (fUseExponentialNotation != other->fUseExponentialNotation) {
        if (first) { printf("[ "); first = FALSE; } else { printf(", "); }
        debug("Use Exp !=");
    }
    if (!(!fUseExponentialNotation ||
          fMinExponentDigits != other->fMinExponentDigits)) {
        if (first) { printf("[ "); first = FALSE; } else { printf(", "); }
        debug("Exp Digits !=");
    }
    if (*fSymbols != *(other->fSymbols)) {
        if (first) { printf("[ "); first = FALSE; } else { printf(", "); }
        debug("Symbols !=");
    }
    // TODO Add debug stuff for significant digits here
    if (fUseSignificantDigits != other->fUseSignificantDigits) {
        debug("fUseSignificantDigits !=");
    }
    if (fUseSignificantDigits &&
        fMinSignificantDigits != other->fMinSignificantDigits) {
        debug("fMinSignificantDigits !=");
    }
    if (fUseSignificantDigits &&
        fMaxSignificantDigits != other->fMaxSignificantDigits) {
        debug("fMaxSignificantDigits !=");
    }

    if (!first) { printf(" ]"); }
    if (fCurrencySignCount != other->fCurrencySignCount) {
        debug("fCurrencySignCount !=");
    }
    if (fCurrencyPluralInfo == other->fCurrencyPluralInfo) {
        debug("fCurrencyPluralInfo == ");
        if (fCurrencyPluralInfo == NULL) {
            debug("fCurrencyPluralInfo == NULL");
        }
    }
    if (fCurrencyPluralInfo != NULL && other->fCurrencyPluralInfo != NULL &&
         *fCurrencyPluralInfo != *(other->fCurrencyPluralInfo)) {
        debug("fCurrencyPluralInfo !=");
    }
    if (fCurrencyPluralInfo != NULL && other->fCurrencyPluralInfo == NULL ||
        fCurrencyPluralInfo == NULL && other->fCurrencyPluralInfo != NULL) {
        debug("fCurrencyPluralInfo one NULL, the other not");
    }
    if (fCurrencyPluralInfo == NULL && other->fCurrencyPluralInfo == NULL) {
        debug("fCurrencyPluralInfo == ");
    }
    }
#endif

    return (NumberFormat::operator==(that) &&
            ((fCurrencySignCount == fgCurrencySignCountInPluralFormat) ?
            (fAffixPatternsForCurrency->equals(*other->fAffixPatternsForCurrency)) :
            (((fPosPrefixPattern == other->fPosPrefixPattern && // both null
              fPositivePrefix == other->fPositivePrefix)
             || (fPosPrefixPattern != 0 && other->fPosPrefixPattern != 0 &&
                 *fPosPrefixPattern  == *other->fPosPrefixPattern)) &&
            ((fPosSuffixPattern == other->fPosSuffixPattern && // both null
              fPositiveSuffix == other->fPositiveSuffix)
             || (fPosSuffixPattern != 0 && other->fPosSuffixPattern != 0 &&
                 *fPosSuffixPattern  == *other->fPosSuffixPattern)) &&
            ((fNegPrefixPattern == other->fNegPrefixPattern && // both null
              fNegativePrefix == other->fNegativePrefix)
             || (fNegPrefixPattern != 0 && other->fNegPrefixPattern != 0 &&
                 *fNegPrefixPattern  == *other->fNegPrefixPattern)) &&
            ((fNegSuffixPattern == other->fNegSuffixPattern && // both null
              fNegativeSuffix == other->fNegativeSuffix)
             || (fNegSuffixPattern != 0 && other->fNegSuffixPattern != 0 &&
                 *fNegSuffixPattern  == *other->fNegSuffixPattern)))) &&
            ((fRoundingIncrement == other->fRoundingIncrement) // both null
             || (fRoundingIncrement != NULL &&
                 other->fRoundingIncrement != NULL &&
                 *fRoundingIncrement == *other->fRoundingIncrement)) &&
        getMultiplier() == other->getMultiplier() &&
        fGroupingSize == other->fGroupingSize &&
        fGroupingSize2 == other->fGroupingSize2 &&
        fDecimalSeparatorAlwaysShown == other->fDecimalSeparatorAlwaysShown &&
        fUseExponentialNotation == other->fUseExponentialNotation &&
        (!fUseExponentialNotation ||
         fMinExponentDigits == other->fMinExponentDigits) &&
        *fSymbols == *(other->fSymbols) &&
        fUseSignificantDigits == other->fUseSignificantDigits &&
        (!fUseSignificantDigits ||
         (fMinSignificantDigits == other->fMinSignificantDigits &&
          fMaxSignificantDigits == other->fMaxSignificantDigits)) &&
        fCurrencySignCount == other->fCurrencySignCount &&
        ((fCurrencyPluralInfo == other->fCurrencyPluralInfo &&
          fCurrencyPluralInfo == NULL) ||
         (fCurrencyPluralInfo != NULL && other->fCurrencyPluralInfo != NULL &&
         *fCurrencyPluralInfo == *(other->fCurrencyPluralInfo))));
}

//------------------------------------------------------------------------------

Format*
DecimalFormat::clone() const
{
    return new DecimalFormat(*this);
}

//------------------------------------------------------------------------------

UnicodeString&
DecimalFormat::format(int32_t number,
                      UnicodeString& appendTo,
                      FieldPosition& fieldPosition) const
{
    return format((int64_t)number, appendTo, fieldPosition);
}

UnicodeString&
DecimalFormat::format(int32_t number,
                      UnicodeString& appendTo,
                      FieldPositionIterator* posIter,
                      UErrorCode& status) const
{
    return format((int64_t)number, appendTo, posIter, status);
}

//------------------------------------------------------------------------------

UnicodeString&
DecimalFormat::format(int64_t number,
                      UnicodeString& appendTo,
                      FieldPosition& fieldPosition) const
{
    FieldPositionOnlyHandler handler(fieldPosition);
    return _format(number, appendTo, handler);
}

UnicodeString&
DecimalFormat::format(int64_t number,
                      UnicodeString& appendTo,
                      FieldPositionIterator* posIter,
                      UErrorCode& status) const
{
    FieldPositionIteratorHandler handler(posIter, status);
    return _format(number, appendTo, handler);
}

UnicodeString&
DecimalFormat::_format(int64_t number,
                       UnicodeString& appendTo,
                       FieldPositionHandler& handler) const
{
    UErrorCode status = U_ZERO_ERROR;
    DigitList digits;
    digits.set(number);
    return _format(digits, appendTo, handler, status);
}

//------------------------------------------------------------------------------

UnicodeString&
DecimalFormat::format(  double number,
                        UnicodeString& appendTo,
                        FieldPosition& fieldPosition) const
{
    FieldPositionOnlyHandler handler(fieldPosition);
    return _format(number, appendTo, handler);
}

UnicodeString&
DecimalFormat::format(  double number,
                        UnicodeString& appendTo,
                        FieldPositionIterator* posIter,
                        UErrorCode& status) const
{
  FieldPositionIteratorHandler handler(posIter, status);
  return _format(number, appendTo, handler);
}

UnicodeString&
DecimalFormat::_format( double number,
                        UnicodeString& appendTo,
                        FieldPositionHandler& handler) const
{
    // Special case for NaN, sets the begin and end index to be the
    // the string length of localized name of NaN.
    // TODO:  let NaNs go through DigitList.
    if (uprv_isNaN(number))
    {
        int begin = appendTo.length();
        appendTo += getConstSymbol(DecimalFormatSymbols::kNaNSymbol);

        handler.addAttribute(kIntegerField, begin, appendTo.length());

        addPadding(appendTo, handler, 0, 0);
        return appendTo;
    }

    UErrorCode status = U_ZERO_ERROR;
    DigitList digits;
    digits.set(number);
    _format(digits, appendTo, handler, status);
    // No way to return status from here.
    return appendTo;
}

//------------------------------------------------------------------------------


UnicodeString&
DecimalFormat::format(const StringPiece &number,
                      UnicodeString &toAppendTo,
                      FieldPositionIterator *posIter,
                      UErrorCode &status) const
{
    DigitList   dnum;
    dnum.set(number, status);
    if (U_FAILURE(status)) {
        return toAppendTo;
    }
    FieldPositionIteratorHandler handler(posIter, status);
    _format(dnum, toAppendTo, handler, status);
    return toAppendTo;
}


UnicodeString&
DecimalFormat::format(const DigitList &number,
                      UnicodeString &appendTo,
                      FieldPositionIterator *posIter,
                      UErrorCode &status) const {
    FieldPositionIteratorHandler handler(posIter, status);
    _format(number, appendTo, handler, status);
    return appendTo;
}



UnicodeString&
DecimalFormat::format(const DigitList &number,
                     UnicodeString& appendTo,
                     FieldPosition& pos,
                     UErrorCode &status) const {
    FieldPositionOnlyHandler handler(pos);
    _format(number, appendTo, handler, status);
    return appendTo;
}



UnicodeString&
DecimalFormat::_format(const DigitList &number,
                        UnicodeString& appendTo,
                        FieldPositionHandler& handler,
                        UErrorCode &status) const
{
    // Special case for NaN, sets the begin and end index to be the
    // the string length of localized name of NaN.
    if (number.isNaN())
    {
        int begin = appendTo.length();
        appendTo += getConstSymbol(DecimalFormatSymbols::kNaNSymbol);

        handler.addAttribute(kIntegerField, begin, appendTo.length());

        addPadding(appendTo, handler, 0, 0);
        return appendTo;
    }

    // Do this BEFORE checking to see if value is infinite or negative! Sets the
    // begin and end index to be length of the string composed of
    // localized name of Infinite and the positive/negative localized
    // signs.

    DigitList adjustedNum(number);  // Copy, so we do not alter the original. 
    adjustedNum.setRoundingMode(fRoundingMode);
    if (fMultiplier != NULL) {
        adjustedNum.mult(*fMultiplier, status);
    }

    /* 
     * Note: sign is important for zero as well as non-zero numbers.
     * Proper detection of -0.0 is needed to deal with the
     * issues raised by bugs 4106658, 4106667, and 4147706.  Liu 7/6/98.
     */
    UBool isNegative = !adjustedNum.isPositive();

    // Apply rounding after multiplier
    if (fRoundingIncrement != NULL) {
        adjustedNum.div(*fRoundingIncrement, status);
        adjustedNum.toIntegralValue();
        adjustedNum.mult(*fRoundingIncrement, status);
        adjustedNum.trim();
    }

    // Special case for INFINITE,
    if (adjustedNum.isInfinite()) {
        int32_t prefixLen = appendAffix(appendTo, adjustedNum.getDouble(), handler, isNegative, TRUE);

        int begin = appendTo.length();
        appendTo += getConstSymbol(DecimalFormatSymbols::kInfinitySymbol);

        handler.addAttribute(kIntegerField, begin, appendTo.length());

        int32_t suffixLen = appendAffix(appendTo, adjustedNum.getDouble(), handler, isNegative, FALSE);

        addPadding(appendTo, handler, prefixLen, suffixLen);
        return appendTo;
    }

    if (fUseExponentialNotation || areSignificantDigitsUsed()) {
        int32_t sigDigits = precision();
        if (sigDigits > 0) {
            adjustedNum.round(sigDigits);
        }
    } else {
        // Fixed point format.  Round to a set number of fraction digits.
        int32_t numFractionDigits = precision();
        adjustedNum.roundFixedPoint(numFractionDigits);
    }
 
    return subformat(appendTo, handler, adjustedNum, FALSE);
}


UnicodeString&
DecimalFormat::format(  const Formattable& obj,
                        UnicodeString& appendTo,
                        FieldPosition& fieldPosition,
                        UErrorCode& status) const
{
    return NumberFormat::format(obj, appendTo, fieldPosition, status);
}

/**
 * Return true if a grouping separator belongs at the given
 * position, based on whether grouping is in use and the values of
 * the primary and secondary grouping interval.
 * @param pos the number of integer digits to the right of
 * the current position.  Zero indicates the position after the
 * rightmost integer digit.
 * @return true if a grouping character belongs at the current
 * position.
 */
UBool DecimalFormat::isGroupingPosition(int32_t pos) const {
    UBool result = FALSE;
    if (isGroupingUsed() && (pos > 0) && (fGroupingSize > 0)) {
        if ((fGroupingSize2 > 0) && (pos > fGroupingSize)) {
            result = ((pos - fGroupingSize) % fGroupingSize2) == 0;
        } else {
            result = pos % fGroupingSize == 0;
        }
    }
    return result;
}

//------------------------------------------------------------------------------

/**
 * Complete the formatting of a finite number.  On entry, the DigitList must
 * be filled in with the correct digits.
 */
UnicodeString&
DecimalFormat::subformat(UnicodeString& appendTo,
                         FieldPositionHandler& handler,
                         DigitList&     digits,
                         UBool          isInteger) const
{
    // char zero = '0'; 
    // DigitList returns digits as '0' thru '9', so we will need to 
    // always need to subtract the character 0 to get the numeric value to use for indexing.

    UChar32 localizedDigits[10];
    localizedDigits[0] = getConstSymbol(DecimalFormatSymbols::kZeroDigitSymbol).char32At(0);
    localizedDigits[1] = getConstSymbol(DecimalFormatSymbols::kOneDigitSymbol).char32At(0);
    localizedDigits[2] = getConstSymbol(DecimalFormatSymbols::kTwoDigitSymbol).char32At(0);
    localizedDigits[3] = getConstSymbol(DecimalFormatSymbols::kThreeDigitSymbol).char32At(0);
    localizedDigits[4] = getConstSymbol(DecimalFormatSymbols::kFourDigitSymbol).char32At(0);
    localizedDigits[5] = getConstSymbol(DecimalFormatSymbols::kFiveDigitSymbol).char32At(0);
    localizedDigits[6] = getConstSymbol(DecimalFormatSymbols::kSixDigitSymbol).char32At(0);
    localizedDigits[7] = getConstSymbol(DecimalFormatSymbols::kSevenDigitSymbol).char32At(0);
    localizedDigits[8] = getConstSymbol(DecimalFormatSymbols::kEightDigitSymbol).char32At(0);
    localizedDigits[9] = getConstSymbol(DecimalFormatSymbols::kNineDigitSymbol).char32At(0);

    const UnicodeString *grouping ;
    if(fCurrencySignCount > fgCurrencySignCountZero) {
        grouping = &getConstSymbol(DecimalFormatSymbols::kMonetaryGroupingSeparatorSymbol);
    }else{
        grouping = &getConstSymbol(DecimalFormatSymbols::kGroupingSeparatorSymbol);
    }
    const UnicodeString *decimal;
    if(fCurrencySignCount > fgCurrencySignCountZero) {
        decimal = &getConstSymbol(DecimalFormatSymbols::kMonetarySeparatorSymbol);
    } else {
        decimal = &getConstSymbol(DecimalFormatSymbols::kDecimalSeparatorSymbol);
    }
    UBool useSigDig = areSignificantDigitsUsed();
    int32_t maxIntDig = getMaximumIntegerDigits();
    int32_t minIntDig = getMinimumIntegerDigits();

    // Appends the prefix.
    double doubleValue = digits.getDouble();
    int32_t prefixLen = appendAffix(appendTo, doubleValue, handler, !digits.isPositive(), TRUE);

    if (fUseExponentialNotation)
    {
        int currentLength = appendTo.length();
        int intBegin = currentLength;
        int intEnd = -1;
        int fracBegin = -1;

        int32_t minFracDig = 0;
        if (useSigDig) {
            maxIntDig = minIntDig = 1;
            minFracDig = getMinimumSignificantDigits() - 1;
        } else {
            minFracDig = getMinimumFractionDigits();
            if (maxIntDig > kMaxScientificIntegerDigits) {
                maxIntDig = 1;
                if (maxIntDig < minIntDig) {
                    maxIntDig = minIntDig;
                }
            }
            if (maxIntDig > minIntDig) {
                minIntDig = 1;
            }
        }

        // Minimum integer digits are handled in exponential format by
        // adjusting the exponent.  For example, 0.01234 with 3 minimum
        // integer digits is "123.4E-4".

        // Maximum integer digits are interpreted as indicating the
        // repeating range.  This is useful for engineering notation, in
        // which the exponent is restricted to a multiple of 3.  For
        // example, 0.01234 with 3 maximum integer digits is "12.34e-3".
        // If maximum integer digits are defined and are larger than
        // minimum integer digits, then minimum integer digits are
        // ignored.
        digits.reduce();   // Removes trailing zero digits.
        int32_t exponent = digits.getDecimalAt();
        if (maxIntDig > 1 && maxIntDig != minIntDig) {
            // A exponent increment is defined; adjust to it.
            exponent = (exponent > 0) ? (exponent - 1) / maxIntDig
                                      : (exponent / maxIntDig) - 1;
            exponent *= maxIntDig;
        } else {
            // No exponent increment is defined; use minimum integer digits.
            // If none is specified, as in "#E0", generate 1 integer digit.
            exponent -= (minIntDig > 0 || minFracDig > 0)
                        ? minIntDig : 1;
        }

        // We now output a minimum number of digits, and more if there
        // are more digits, up to the maximum number of digits.  We
        // place the decimal point after the "integer" digits, which
        // are the first (decimalAt - exponent) digits.
        int32_t minimumDigits =  minIntDig + minFracDig;
        // The number of integer digits is handled specially if the number
        // is zero, since then there may be no digits.
        int32_t integerDigits = digits.isZero() ? minIntDig :
            digits.getDecimalAt() - exponent;
        int32_t totalDigits = digits.getCount();
        if (minimumDigits > totalDigits)
            totalDigits = minimumDigits;
        if (integerDigits > totalDigits)
            totalDigits = integerDigits;

        // totalDigits records total number of digits needs to be processed
        int32_t i;
        for (i=0; i<totalDigits; ++i)
        {
            if (i == integerDigits)
            {
                intEnd = appendTo.length();
                handler.addAttribute(kIntegerField, intBegin, intEnd);

                appendTo += *decimal;

                fracBegin = appendTo.length();
                handler.addAttribute(kDecimalSeparatorField, fracBegin - 1, fracBegin);
            }
            // Restores the digit character or pads the buffer with zeros.
            UChar32 c = (UChar32)((i < digits.getCount()) ?
                          localizedDigits[digits.getDigitValue(i)] :
                          localizedDigits[0]);
            appendTo += c;
        }

        currentLength = appendTo.length();

        if (intEnd < 0) {
            handler.addAttribute(kIntegerField, intBegin, currentLength);
        }
        if (fracBegin > 0) {
            handler.addAttribute(kFractionField, fracBegin, currentLength);
        }

        // The exponent is output using the pattern-specified minimum
        // exponent digits.  There is no maximum limit to the exponent
        // digits, since truncating the exponent would appendTo in an
        // unacceptable inaccuracy.
        appendTo += getConstSymbol(DecimalFormatSymbols::kExponentialSymbol);

        handler.addAttribute(kExponentSymbolField, currentLength, appendTo.length());
        currentLength = appendTo.length();

        // For zero values, we force the exponent to zero.  We
        // must do this here, and not earlier, because the value
        // is used to determine integer digit count above.
        if (digits.isZero())
            exponent = 0;

        if (exponent < 0) {
            appendTo += getConstSymbol(DecimalFormatSymbols::kMinusSignSymbol);
            handler.addAttribute(kExponentSignField, currentLength, appendTo.length());
        } else if (fExponentSignAlwaysShown) {
            appendTo += getConstSymbol(DecimalFormatSymbols::kPlusSignSymbol);
            handler.addAttribute(kExponentSignField, currentLength, appendTo.length());
        }

        currentLength = appendTo.length();

        DigitList expDigits;
        expDigits.set(exponent);
        {
            int expDig = fMinExponentDigits;
            if (fUseExponentialNotation && expDig < 1) {
                expDig = 1;
            }
            for (i=expDigits.getDecimalAt(); i<expDig; ++i)
                appendTo += (localizedDigits[0]);
        }
        for (i=0; i<expDigits.getDecimalAt(); ++i)
        {
            UChar32 c = (UChar32)((i < expDigits.getCount()) ?
                          localizedDigits[expDigits.getDigitValue(i)] : 
                          localizedDigits[0]);
            appendTo += c;
        }

        handler.addAttribute(kExponentField, currentLength, appendTo.length());
    }
    else  // Not using exponential notation
    {
        int currentLength = appendTo.length();
        int intBegin = currentLength;

        int32_t sigCount = 0;
        int32_t minSigDig = getMinimumSignificantDigits();
        int32_t maxSigDig = getMaximumSignificantDigits();
        if (!useSigDig) {
            minSigDig = 0;
            maxSigDig = INT32_MAX;
        }

        // Output the integer portion.  Here 'count' is the total
        // number of integer digits we will display, including both
        // leading zeros required to satisfy getMinimumIntegerDigits,
        // and actual digits present in the number.
        int32_t count = useSigDig ?
            _max(1, digits.getDecimalAt()) : minIntDig;
        if (digits.getDecimalAt() > 0 && count < digits.getDecimalAt()) {
            count = digits.getDecimalAt();
        }

        // Handle the case where getMaximumIntegerDigits() is smaller
        // than the real number of integer digits.  If this is so, we
        // output the least significant max integer digits.  For example,
        // the value 1997 printed with 2 max integer digits is just "97".

        int32_t digitIndex = 0; // Index into digitList.fDigits[]
        if (count > maxIntDig && maxIntDig >= 0) {
            count = maxIntDig;
            digitIndex = digits.getDecimalAt() - count;
        }

        int32_t sizeBeforeIntegerPart = appendTo.length();

        int32_t i;
        for (i=count-1; i>=0; --i)
        {
            if (i < digits.getDecimalAt() && digitIndex < digits.getCount() &&
                sigCount < maxSigDig) {
                // Output a real digit
                appendTo += (UChar32)localizedDigits[digits.getDigitValue(digitIndex++)];
                ++sigCount;
            }
            else
            {
                // Output a zero (leading or trailing)
                appendTo += localizedDigits[0];
                if (sigCount > 0) {
                    ++sigCount;
                }
            }

            // Output grouping separator if necessary.
            if (isGroupingPosition(i)) {
                currentLength = appendTo.length();
                appendTo.append(*grouping);
                handler.addAttribute(kGroupingSeparatorField, currentLength, appendTo.length());
            }
        }

        // TODO(dlf): this looks like it was a bug, we marked the int field as ending
        // before the zero was generated.
        // Record field information for caller.
        // if (fieldPosition.getField() == NumberFormat::kIntegerField)
        //     fieldPosition.setEndIndex(appendTo.length());

        // Determine whether or not there are any printable fractional
        // digits.  If we've used up the digits we know there aren't.
        UBool fractionPresent = (!isInteger && digitIndex < digits.getCount()) ||
            (useSigDig ? (sigCount < minSigDig) : (getMinimumFractionDigits() > 0));

        // If there is no fraction present, and we haven't printed any
        // integer digits, then print a zero.  Otherwise we won't print
        // _any_ digits, and we won't be able to parse this string.
        if (!fractionPresent && appendTo.length() == sizeBeforeIntegerPart)
            appendTo += localizedDigits[0];

        currentLength = appendTo.length();
        handler.addAttribute(kIntegerField, intBegin, currentLength);

        // Output the decimal separator if we always do so.
        if (fDecimalSeparatorAlwaysShown || fractionPresent) {
            appendTo += *decimal;
            handler.addAttribute(kDecimalSeparatorField, currentLength, appendTo.length());
            currentLength = appendTo.length();
        }

        int fracBegin = currentLength;

        count = useSigDig ? INT32_MAX : getMaximumFractionDigits();
        if (useSigDig && (sigCount == maxSigDig ||
                          (sigCount >= minSigDig && digitIndex == digits.getCount()))) {
            count = 0;
        }

        for (i=0; i < count; ++i) {
            // Here is where we escape from the loop.  We escape
            // if we've output the maximum fraction digits
            // (specified in the for expression above).  We also
            // stop when we've output the minimum digits and
            // either: we have an integer, so there is no
            // fractional stuff to display, or we're out of
            // significant digits.
            if (!useSigDig && i >= getMinimumFractionDigits() &&
                (isInteger || digitIndex >= digits.getCount())) {
                break;
            }

            // Output leading fractional zeros.  These are zeros
            // that come after the decimal but before any
            // significant digits.  These are only output if
            // abs(number being formatted) < 1.0.
            if (-1-i > (digits.getDecimalAt()-1)) {
                appendTo += localizedDigits[0];
                continue;
            }

            // Output a digit, if we have any precision left, or a
            // zero if we don't.  We don't want to output noise digits.
            if (!isInteger && digitIndex < digits.getCount()) {
                appendTo += (UChar32)localizedDigits[digits.getDigitValue(digitIndex++)];
            } else {
                appendTo += localizedDigits[0];
            }

            // If we reach the maximum number of significant
            // digits, or if we output all the real digits and
            // reach the minimum, then we are done.
            ++sigCount;
            if (useSigDig &&
                (sigCount == maxSigDig ||
                 (digitIndex == digits.getCount() && sigCount >= minSigDig))) {
                break;
            }
        }

        handler.addAttribute(kFractionField, fracBegin, appendTo.length());
    }

    int32_t suffixLen = appendAffix(appendTo, doubleValue, handler, !digits.isPositive(), FALSE);

    addPadding(appendTo, handler, prefixLen, suffixLen);
    return appendTo;
}

/**
 * Inserts the character fPad as needed to expand result to fFormatWidth.
 * @param result the string to be padded
 */
void DecimalFormat::addPadding(UnicodeString& appendTo,
                               FieldPositionHandler& handler,
                               int32_t prefixLen,
                               int32_t suffixLen) const
{
    if (fFormatWidth > 0) {
        int32_t len = fFormatWidth - appendTo.length();
        if (len > 0) {
            UnicodeString padding;
            for (int32_t i=0; i<len; ++i) {
                padding += fPad;
            }
            switch (fPadPosition) {
            case kPadAfterPrefix:
                appendTo.insert(prefixLen, padding);
                break;
            case kPadBeforePrefix:
                appendTo.insert(0, padding);
                break;
            case kPadBeforeSuffix:
                appendTo.insert(appendTo.length() - suffixLen, padding);
                break;
            case kPadAfterSuffix:
                appendTo += padding;
                break;
            }
            if (fPadPosition == kPadBeforePrefix || fPadPosition == kPadAfterPrefix) {
                handler.shiftLast(len);
            }
        }
    }
}

//------------------------------------------------------------------------------

void
DecimalFormat::parse(const UnicodeString& text,
                     Formattable& result,
                     UErrorCode& status) const
{
    NumberFormat::parse(text, result, status);
}

void
DecimalFormat::parse(const UnicodeString& text,
                     Formattable& result,
                     ParsePosition& parsePosition) const {
    parse(text, result, parsePosition, FALSE);
}

Formattable& DecimalFormat::parseCurrency(const UnicodeString& text,
                                          Formattable& result,
                                          ParsePosition& pos) const {
    parse(text, result, pos, TRUE);
    return result;
}

/**
 * Parses the given text as either a number or a currency amount.
 * @param text the string to parse
 * @param result output parameter for the result
 * @param parsePosition input-output position; on input, the
 * position within text to match; must have 0 <= pos.getIndex() <
 * text.length(); on output, the position after the last matched
 * character. If the parse fails, the position in unchanged upon
 * output.
 * @param parseCurrency if true, a currency amount is parsed;
 * otherwise a Number is parsed
 */
void DecimalFormat::parse(const UnicodeString& text,
                          Formattable& result,
                          ParsePosition& parsePosition,
                          UBool parseCurrency) const {
    int32_t backup;
    int32_t i = backup = parsePosition.getIndex();

    // clear any old contents in the result.  In particular, clears any DigitList
    //   that it may be holding.
    result.setLong(0);

    // Handle NaN as a special case:

    // Skip padding characters, if around prefix
    if (fFormatWidth > 0 && (fPadPosition == kPadBeforePrefix ||
                             fPadPosition == kPadAfterPrefix)) {
        i = skipPadding(text, i);
    }
    // If the text is composed of the representation of NaN, returns NaN.length
    const UnicodeString *nan = &getConstSymbol(DecimalFormatSymbols::kNaNSymbol);
    int32_t nanLen = (text.compare(i, nan->length(), *nan)
                      ? 0 : nan->length());
    if (nanLen) {
        i += nanLen;
        if (fFormatWidth > 0 && (fPadPosition == kPadBeforeSuffix ||
                                 fPadPosition == kPadAfterSuffix)) {
            i = skipPadding(text, i);
        }
        parsePosition.setIndex(i);
        result.setDouble(uprv_getNaN());
        return;
    }

    // NaN parse failed; start over
    i = backup;

    // status is used to record whether a number is infinite.
    UBool status[fgStatusLength];
    UChar curbuf[4];
    UChar* currency = parseCurrency ? curbuf : NULL;
    DigitList *digits = new DigitList;
    if (digits == NULL) {
        return;    // no way to report error from here.
    }

    if (fCurrencySignCount > fgCurrencySignCountZero) {
        if (!parseForCurrency(text, parsePosition, *digits,
                              status, currency)) {
            delete digits;
            return;
        }
    } else {
        if (!subparse(text,
                      fNegPrefixPattern, fNegSuffixPattern,
                      fPosPrefixPattern, fPosSuffixPattern,
                      FALSE, UCURR_SYMBOL_NAME,
                      parsePosition, *digits, status, currency)) {
            parsePosition.setIndex(backup);
            delete digits;
            return;
        }
    }

    // Handle infinity
    if (status[fgStatusInfinite]) {
        double inf = uprv_getInfinity();
        result.setDouble(digits->isPositive() ? inf : -inf);
        delete digits;    // TODO:  set the dl to infinity, and let it fall into the code below.
    }

    else {

        if (fMultiplier != NULL) {
            UErrorCode ec = U_ZERO_ERROR;
            digits->div(*fMultiplier, ec);
        }

        // Negative zero special case:
        //    if parsing integerOnly, change to +0, which goes into an int32 in a Formattable.
        //    if not parsing integerOnly, leave as -0, which a double can represent.
        if (digits->isZero() && !digits->isPositive() && isParseIntegerOnly()) {
            digits->setPositive(TRUE);
        }
        result.adoptDigitList(digits);
    }

    if (parseCurrency) {
        UErrorCode ec = U_ZERO_ERROR;
        Formattable n(result);
        result.adoptObject(new CurrencyAmount(n, curbuf, ec));
        U_ASSERT(U_SUCCESS(ec)); // should always succeed
    }
}



UBool
DecimalFormat::parseForCurrency(const UnicodeString& text,
                                ParsePosition& parsePosition,
                                DigitList& digits,
                                UBool* status,
                                UChar* currency) const {
    int origPos = parsePosition.getIndex();
    int maxPosIndex = origPos;
    int maxErrorPos = -1;
    // First, parse against current pattern.
    // Since current pattern could be set by applyPattern(),
    // it could be an arbitrary pattern, and it may not be the one
    // defined in current locale.
    UBool tmpStatus[fgStatusLength];
    ParsePosition tmpPos(origPos);
    DigitList tmpDigitList;
    UBool found;
    if (fStyle == NumberFormat::kPluralCurrencyStyle) {
        found = subparse(text,
                         fNegPrefixPattern, fNegSuffixPattern,
                         fPosPrefixPattern, fPosSuffixPattern,
                         TRUE, UCURR_LONG_NAME,
                         tmpPos, tmpDigitList, tmpStatus, currency);
    } else {
        found = subparse(text,
                         fNegPrefixPattern, fNegSuffixPattern,
                         fPosPrefixPattern, fPosSuffixPattern,
                         TRUE, UCURR_SYMBOL_NAME,
                         tmpPos, tmpDigitList, tmpStatus, currency);
    }
    if (found) {
        if (tmpPos.getIndex() > maxPosIndex) {
            maxPosIndex = tmpPos.getIndex();
            for (int32_t i = 0; i < fgStatusLength; ++i) {
                status[i] = tmpStatus[i];
            }
            digits = tmpDigitList;
        }
    } else {
        maxErrorPos = tmpPos.getErrorIndex();
    }
    // Then, parse against affix patterns.
    // Those are currency patterns and currency plural patterns.
    int32_t pos = -1;
    const UHashElement* element = NULL;
    while ( (element = fAffixPatternsForCurrency->nextElement(pos)) != NULL ) {
        const UHashTok keyTok = element->key;
        const UHashTok valueTok = element->value;
        const AffixPatternsForCurrency* affixPtn = (AffixPatternsForCurrency*)valueTok.pointer;
        UBool tmpStatus[fgStatusLength];
        ParsePosition tmpPos(origPos);
        DigitList tmpDigitList;
        UBool result = subparse(text,
                                &affixPtn->negPrefixPatternForCurrency,
                                &affixPtn->negSuffixPatternForCurrency,
                                &affixPtn->posPrefixPatternForCurrency,
                                &affixPtn->posSuffixPatternForCurrency,
                                TRUE, affixPtn->patternType,
                                tmpPos, tmpDigitList, tmpStatus, currency);
        if (result) {
            found = true;
            if (tmpPos.getIndex() > maxPosIndex) {
                maxPosIndex = tmpPos.getIndex();
                for (int32_t i = 0; i < fgStatusLength; ++i) {
                    status[i] = tmpStatus[i];
                }
                digits = tmpDigitList;
            }
        } else {
            maxErrorPos = (tmpPos.getErrorIndex() > maxErrorPos) ?
                          tmpPos.getErrorIndex() : maxErrorPos;
        }
    }
    // Finally, parse against simple affix to find the match.
    // For example, in TestMonster suite,
    // if the to-be-parsed text is "-\u00A40,00".
    // complexAffixCompare will not find match,
    // since there is no ISO code matches "\u00A4",
    // and the parse stops at "\u00A4".
    // We will just use simple affix comparison (look for exact match)
    // to pass it.
    UBool tmpStatus_2[fgStatusLength];
    ParsePosition tmpPos_2(origPos);
    DigitList tmpDigitList_2;
    // set currencySignCount to 0 so that compareAffix function will
    // fall to compareSimpleAffix path, not compareComplexAffix path.
    // ?? TODO: is it right? need "false"?
    UBool result = subparse(text,
                            &fNegativePrefix, &fNegativeSuffix,
                            &fPositivePrefix, &fPositiveSuffix,
                            FALSE, UCURR_SYMBOL_NAME,
                            tmpPos_2, tmpDigitList_2, tmpStatus_2,
                            currency);
    if (result) {
        if (tmpPos_2.getIndex() > maxPosIndex) {
            maxPosIndex = tmpPos_2.getIndex();
            for (int32_t i = 0; i < fgStatusLength; ++i) {
                status[i] = tmpStatus_2[i];
            }
            digits = tmpDigitList_2;
        }
        found = true;
    } else {
            maxErrorPos = (tmpPos_2.getErrorIndex() > maxErrorPos) ?
                          tmpPos_2.getErrorIndex() : maxErrorPos;
    }

    if (!found) {
        //parsePosition.setIndex(origPos);
        parsePosition.setErrorIndex(maxErrorPos);
    } else {
        parsePosition.setIndex(maxPosIndex);
        parsePosition.setErrorIndex(-1);
    }
    return found;
}


/**
 * Parse the given text into a number.  The text is parsed beginning at
 * parsePosition, until an unparseable character is seen.
 * @param text the string to parse.
 * @param negPrefix negative prefix.
 * @param negSuffix negative suffix.
 * @param posPrefix positive prefix.
 * @param posSuffix positive suffix.
 * @param currencyParsing whether it is currency parsing or not.
 * @param type the currency type to parse against, LONG_NAME only or not.
 * @param parsePosition The position at which to being parsing.  Upon
 * return, the first unparsed character.
 * @param digits the DigitList to set to the parsed value.
 * @param status output param containing boolean status flags indicating
 * whether the value was infinite and whether it was positive.
 * @param currency return value for parsed currency, for generic
 * currency parsing mode, or NULL for normal parsing. In generic
 * currency parsing mode, any currency is parsed, not just the
 * currency that this formatter is set to.
 */
UBool DecimalFormat::subparse(const UnicodeString& text,
                              const UnicodeString* negPrefix,
                              const UnicodeString* negSuffix,
                              const UnicodeString* posPrefix,
                              const UnicodeString* posSuffix,
                              UBool currencyParsing,
                              int8_t type,
                              ParsePosition& parsePosition,
                              DigitList& digits, UBool* status,
                              UChar* currency) const
{
    //  The parsing process builds up the number as char string, in the neutral format that
    //  will be acceptable to the decNumber library, then at the end passes that string
    //  off for conversion to a decNumber.
    UErrorCode err = U_ZERO_ERROR;
    CharString parsedNum;
    digits.setToZero();

    int32_t position = parsePosition.getIndex();
    int32_t oldStart = position;

    // Match padding before prefix
    if (fFormatWidth > 0 && fPadPosition == kPadBeforePrefix) {
        position = skipPadding(text, position);
    }

    // Match positive and negative prefixes; prefer longest match.
    int32_t posMatch = compareAffix(text, position, FALSE, TRUE, posPrefix, currencyParsing, type, currency);
    int32_t negMatch = compareAffix(text, position, TRUE, TRUE, negPrefix,currencyParsing,  type, currency);
    if (posMatch >= 0 && negMatch >= 0) {
        if (posMatch > negMatch) {
            negMatch = -1;
        } else if (negMatch > posMatch) {
            posMatch = -1;
        }
    }
    if (posMatch >= 0) {
        position += posMatch;
        parsedNum.append('+', err);
    } else if (negMatch >= 0) {
        position += negMatch;
        parsedNum.append('-', err);
    } else {
        parsePosition.setErrorIndex(position);
        return FALSE;
    }

    // Match padding before prefix
    if (fFormatWidth > 0 && fPadPosition == kPadAfterPrefix) {
        position = skipPadding(text, position);
    }

    // process digits or Inf, find decimal position
    const UnicodeString *inf = &getConstSymbol(DecimalFormatSymbols::kInfinitySymbol);
    int32_t infLen = (text.compare(position, inf->length(), *inf)
        ? 0 : inf->length());
    position += infLen; // infLen is non-zero when it does equal to infinity
    status[fgStatusInfinite] = (UBool)infLen;
    if (infLen) {
        parsedNum.append("Infinity", err);
    } else {
        // We now have a string of digits, possibly with grouping symbols,
        // and decimal points.  We want to process these into a DigitList.
        // We don't want to put a bunch of leading zeros into the DigitList
        // though, so we keep track of the location of the decimal point,
        // put only significant digits into the DigitList, and adjust the
        // exponent as needed.

        UChar32 zero = getConstSymbol(DecimalFormatSymbols::kZeroDigitSymbol).char32At(0);

        const UnicodeString *decimal;
        if(fCurrencySignCount > fgCurrencySignCountZero) {
            decimal = &getConstSymbol(DecimalFormatSymbols::kMonetarySeparatorSymbol);
        } else {
            decimal = &getConstSymbol(DecimalFormatSymbols::kDecimalSeparatorSymbol);
        }
        const UnicodeString *grouping = &getConstSymbol(DecimalFormatSymbols::kGroupingSeparatorSymbol);
        UBool sawDecimal = FALSE;
        UBool sawDigit = FALSE;
        int32_t backup = -1;
        int32_t digit;
        int32_t textLength = text.length(); // One less pointer to follow
        int32_t groupingLen = grouping->length();
        int32_t decimalLen = decimal->length();

        // We have to track digitCount ourselves, because digits.fCount will
        // pin when the maximum allowable digits is reached.
        int32_t digitCount = 0;

        for (; position < textLength; )
        {
            UChar32 ch = text.char32At(position);

            /* We recognize all digit ranges, not only the Latin digit range
             * '0'..'9'.  We do so by using the Character.digit() method,
             * which converts a valid Unicode digit to the range 0..9.
             *
             * The character 'ch' may be a digit.  If so, place its value
             * from 0 to 9 in 'digit'.  First try using the locale digit,
             * which may or MAY NOT be a standard Unicode digit range.  If
             * this fails, try using the standard Unicode digit ranges by
             * calling Character.digit().  If this also fails, digit will 
             * have a value outside the range 0..9.
             */
            digit = ch - zero;
            if (digit < 0 || digit > 9)
            {
                digit = u_charDigitValue(ch);
            }
            
            // As a last resort, look through the localized digits if the zero digit
            // is not a "standard" Unicode digit.
            if ( (digit < 0 || digit > 9) && u_charDigitValue(zero) != 0) {
                digit = 0;
                if ( getConstSymbol((DecimalFormatSymbols::ENumberFormatSymbol)(DecimalFormatSymbols::kZeroDigitSymbol)).char32At(0) == ch ) {
                    break;
                }
                for (digit = 1 ; digit < 10 ; digit++ ) {
                    if ( getConstSymbol((DecimalFormatSymbols::ENumberFormatSymbol)(DecimalFormatSymbols::kOneDigitSymbol+digit-1)).char32At(0) == ch ) {
                        break;
                    }
                }
            }

            if (digit >= 0 && digit <= 9)
            {
                // Cancel out backup setting (see grouping handler below)
                backup = -1;

                sawDigit = TRUE;
                // output a regular non-zero digit.
                ++digitCount;
                parsedNum.append((char)(digit + '0'), err);
                position += U16_LENGTH(ch);
            }
            else if (groupingLen > 0 && !text.compare(position, groupingLen, *grouping) && isGroupingUsed())
            {
                // Ignore grouping characters, if we are using them, but require
                // that they be followed by a digit.  Otherwise we backup and
                // reprocess them.
                backup = position;
                position += groupingLen;
            }
            else if (!text.compare(position, decimalLen, *decimal) && !isParseIntegerOnly() && !sawDecimal)
            {
                // If we're only parsing integers, or if we ALREADY saw the
                // decimal, then don't parse this one.

                parsedNum.append('.', err);
                sawDecimal = TRUE;
                position += decimalLen;
            }
            else {
                const UnicodeString *tmp;
                tmp = &getConstSymbol(DecimalFormatSymbols::kExponentialSymbol);
                if (!text.compare(position, tmp->length(), *tmp))    // error code is set below if !sawDigit 
                {
                    // Parse sign, if present
                    int32_t pos = position + tmp->length();
                    char exponentSign = '+';

                    if (pos < textLength)
                    {
                        tmp = &getConstSymbol(DecimalFormatSymbols::kPlusSignSymbol);
                        if (!text.compare(pos, tmp->length(), *tmp))
                        {
                            pos += tmp->length();
                        }
                        else {
                            tmp = &getConstSymbol(DecimalFormatSymbols::kMinusSignSymbol);
                            if (!text.compare(pos, tmp->length(), *tmp))
                            {
                                exponentSign = '-';
                                pos += tmp->length();
                            }
                        }
                    }

                    UBool sawExponentDigit = FALSE;
                    while (pos < textLength) {
                        ch = text[(int32_t)pos];
                        digit = ch - zero;

                        if (digit < 0 || digit > 9) {
                            digit = u_charDigitValue(ch);
                        }
                        if (0 <= digit && digit <= 9) {
                            if (!sawExponentDigit) {
                                parsedNum.append('E', err);
                                parsedNum.append(exponentSign, err);
                                sawExponentDigit = TRUE;
                            }
                            ++pos;
                            parsedNum.append((char)(digit + '0'), err);
                        } else {
                            break;
                        }
                    }

                    if (sawExponentDigit) {
                        position = pos; // Advance past the exponent
                    }

                    break; // Whether we fail or succeed, we exit this loop
                }
                else {
                    break;
                }
            }
        }

        if (backup != -1)
        {
            position = backup;
        }

        // If there was no decimal point we have an integer

        // If none of the text string was recognized.  For example, parse
        // "x" with pattern "#0.00" (return index and error index both 0)
        // parse "$" with pattern "$#0.00". (return index 0 and error index
        // 1).
        if (!sawDigit && digitCount == 0) {
            parsePosition.setIndex(oldStart);
            parsePosition.setErrorIndex(oldStart);
            return FALSE;
        }
    }

    // Match padding before suffix
    if (fFormatWidth > 0 && fPadPosition == kPadBeforeSuffix) {
        position = skipPadding(text, position);
    }

    // Match positive and negative suffixes; prefer longest match.
    if (posMatch >= 0) {
        posMatch = compareAffix(text, position, FALSE, FALSE, posSuffix, currencyParsing, type, currency);
    }
    if (negMatch >= 0) {
        negMatch = compareAffix(text, position, TRUE, FALSE, negSuffix, currencyParsing, type, currency);
    }
    if (posMatch >= 0 && negMatch >= 0) {
        if (posMatch > negMatch) {
            negMatch = -1;
        } else if (negMatch > posMatch) {
            posMatch = -1;
        }
    }

    // Fail if neither or both
    if ((posMatch >= 0) == (negMatch >= 0)) {
        parsePosition.setErrorIndex(position);
        return FALSE;
    }

    position += (posMatch>=0 ? posMatch : negMatch);

    // Match padding before suffix
    if (fFormatWidth > 0 && fPadPosition == kPadAfterSuffix) {
        position = skipPadding(text, position);
    }

    parsePosition.setIndex(position);

    parsedNum.data()[0] = (posMatch >= 0) ? '+' : '-';

    if(parsePosition.getIndex() == oldStart)
    {
        parsePosition.setErrorIndex(position);
        return FALSE;
    }
    digits.set(parsedNum.toStringPiece(), err);

    if (U_FAILURE(err)) {
        parsePosition.setErrorIndex(position);
        return FALSE;
    }
    return TRUE;
}

/**
 * Starting at position, advance past a run of pad characters, if any.
 * Return the index of the first character after position that is not a pad
 * character.  Result is >= position.
 */
int32_t DecimalFormat::skipPadding(const UnicodeString& text, int32_t position) const {
    int32_t padLen = U16_LENGTH(fPad);
    while (position < text.length() &&
           text.char32At(position) == fPad) {
        position += padLen;
    }
    return position;
}

/**
 * Return the length matched by the given affix, or -1 if none.
 * Runs of white space in the affix, match runs of white space in
 * the input.  Pattern white space and input white space are
 * determined differently; see code.
 * @param text input text
 * @param pos offset into input at which to begin matching
 * @param isNegative
 * @param isPrefix
 * @param affixPat affix pattern used for currency affix comparison.
 * @param currencyParsing whether it is currency parsing or not
 * @param type the currency type to parse against, LONG_NAME only or not.
 * @param currency return value for parsed currency, for generic
 * currency parsing mode, or null for normal parsing. In generic
 * currency parsing mode, any currency is parsed, not just the
 * currency that this formatter is set to.
 * @return length of input that matches, or -1 if match failure
 */
int32_t DecimalFormat::compareAffix(const UnicodeString& text,
                                    int32_t pos,
                                    UBool isNegative,
                                    UBool isPrefix,
                                    const UnicodeString* affixPat,
                                    UBool currencyParsing,
                                    int8_t type,
                                    UChar* currency) const
{
    const UnicodeString *patternToCompare;
    if (fCurrencyChoice != NULL || currency != NULL ||
        (fCurrencySignCount > fgCurrencySignCountZero && currencyParsing)) {

        if (affixPat != NULL) {
            return compareComplexAffix(*affixPat, text, pos, type, currency);
        }
    }

    if (isNegative) {
        if (isPrefix) {
            patternToCompare = &fNegativePrefix;
        }
        else {
            patternToCompare = &fNegativeSuffix;
        }
    }
    else {
        if (isPrefix) {
            patternToCompare = &fPositivePrefix;
        }
        else {
            patternToCompare = &fPositiveSuffix;
        }
    }
    return compareSimpleAffix(*patternToCompare, text, pos);
}

/**
 * Return the length matched by the given affix, or -1 if none.
 * Runs of white space in the affix, match runs of white space in
 * the input.  Pattern white space and input white space are
 * determined differently; see code.
 * @param affix pattern string, taken as a literal
 * @param input input text
 * @param pos offset into input at which to begin matching
 * @return length of input that matches, or -1 if match failure
 */
int32_t DecimalFormat::compareSimpleAffix(const UnicodeString& affix,
                                          const UnicodeString& input,
                                          int32_t pos) {
    int32_t start = pos;
    for (int32_t i=0; i<affix.length(); ) {
        UChar32 c = affix.char32At(i);
        int32_t len = U16_LENGTH(c);
        if (uprv_isRuleWhiteSpace(c)) {
            // We may have a pattern like: \u200F \u0020
            //        and input text like: \u200F \u0020
            // Note that U+200F and U+0020 are RuleWhiteSpace but only
            // U+0020 is UWhiteSpace.  So we have to first do a direct
            // match of the run of RULE whitespace in the pattern,
            // then match any extra characters.
            UBool literalMatch = FALSE;
            while (pos < input.length() &&
                   input.char32At(pos) == c) {
                literalMatch = TRUE;
                i += len;
                pos += len;
                if (i == affix.length()) {
                    break;
                }
                c = affix.char32At(i);
                len = U16_LENGTH(c);
                if (!uprv_isRuleWhiteSpace(c)) {
                    break;
                }
            }

            // Advance over run in pattern
            i = skipRuleWhiteSpace(affix, i);

            // Advance over run in input text
            // Must see at least one white space char in input,
            // unless we've already matched some characters literally.
            int32_t s = pos;
            pos = skipUWhiteSpace(input, pos);
            if (pos == s && !literalMatch) {
                return -1;
            }

            // If we skip UWhiteSpace in the input text, we need to skip it in the pattern.
            // Otherwise, the previous lines may have skipped over text (such as U+00A0) that
            // is also in the affix.
            i = skipUWhiteSpace(affix, i);
        } else {
            if (pos < input.length() &&
                input.char32At(pos) == c) {
                i += len;
                pos += len;
            } else {
                return -1;
            }
        }
    }
    return pos - start;
}

/**
 * Skip over a run of zero or more isRuleWhiteSpace() characters at
 * pos in text.
 */
int32_t DecimalFormat::skipRuleWhiteSpace(const UnicodeString& text, int32_t pos) {
    while (pos < text.length()) {
        UChar32 c = text.char32At(pos);
        if (!uprv_isRuleWhiteSpace(c)) {
            break;
        }
        pos += U16_LENGTH(c);
    }
    return pos;
}

/**
 * Skip over a run of zero or more isUWhiteSpace() characters at pos
 * in text.
 */
int32_t DecimalFormat::skipUWhiteSpace(const UnicodeString& text, int32_t pos) {
    while (pos < text.length()) {
        UChar32 c = text.char32At(pos);
        if (!u_isUWhiteSpace(c)) {
            break;
        }
        pos += U16_LENGTH(c);
    }
    return pos;
}

/**
 * Return the length matched by the given affix, or -1 if none.
 * @param affixPat pattern string
 * @param input input text
 * @param pos offset into input at which to begin matching
 * @param type the currency type to parse against, LONG_NAME only or not.
 * @param currency return value for parsed currency, for generic
 * currency parsing mode, or null for normal parsing. In generic
 * currency parsing mode, any currency is parsed, not just the
 * currency that this formatter is set to.
 * @return length of input that matches, or -1 if match failure
 */
int32_t DecimalFormat::compareComplexAffix(const UnicodeString& affixPat,
                                           const UnicodeString& text,
                                           int32_t pos,
                                           int8_t type,
                                           UChar* currency) const
{
    int32_t start = pos;
    U_ASSERT(currency != NULL ||
             (fCurrencyChoice != NULL && *getCurrency() != 0) ||
             fCurrencySignCount > fgCurrencySignCountZero);

    for (int32_t i=0;
         i<affixPat.length() && pos >= 0; ) {
        UChar32 c = affixPat.char32At(i);
        i += U16_LENGTH(c);

        if (c == kQuote) {
            U_ASSERT(i <= affixPat.length());
            c = affixPat.char32At(i);
            i += U16_LENGTH(c);

            const UnicodeString* affix = NULL;

            switch (c) {
            case kCurrencySign: {
                // since the currency names in choice format is saved
                // the same way as other currency names,
                // do not need to do currency choice parsing here.
                // the general currency parsing parse against all names,
                // including names in choice format.
                UBool intl = i<affixPat.length() &&
                    affixPat.char32At(i) == kCurrencySign;
                if (intl) {
                    ++i;
                }
                UBool plural = i<affixPat.length() &&
                    affixPat.char32At(i) == kCurrencySign;
                if (plural) {
                    ++i;
                    intl = FALSE;
                }
                // Parse generic currency -- anything for which we
                // have a display name, or any 3-letter ISO code.
                // Try to parse display name for our locale; first
                // determine our locale.
                const char* loc = fCurrencyPluralInfo->getLocale().getName();
                ParsePosition ppos(pos);
                UChar curr[4];
                UErrorCode ec = U_ZERO_ERROR;
                // Delegate parse of display name => ISO code to Currency
                uprv_parseCurrency(loc, text, ppos, type, curr, ec);

                // If parse succeeds, populate currency[0]
                if (U_SUCCESS(ec) && ppos.getIndex() != pos) {
                    if (currency) {
                        u_strcpy(currency, curr);
                    }
                    pos = ppos.getIndex();
                } else {
                    pos = -1;
                }
                continue;
            }
            case kPatternPercent:
                affix = &getConstSymbol(DecimalFormatSymbols::kPercentSymbol);
                break;
            case kPatternPerMill:
                affix = &getConstSymbol(DecimalFormatSymbols::kPerMillSymbol);
                break;
            case kPatternPlus:
                affix = &getConstSymbol(DecimalFormatSymbols::kPlusSignSymbol);
                break;
            case kPatternMinus:
                affix = &getConstSymbol(DecimalFormatSymbols::kMinusSignSymbol);
                break;
            default:
                // fall through to affix!=0 test, which will fail
                break;
            }

            if (affix != NULL) {
                pos = match(text, pos, *affix);
                continue;
            }
        }

        pos = match(text, pos, c);
        if (uprv_isRuleWhiteSpace(c)) {
            i = skipRuleWhiteSpace(affixPat, i);
        }
    }
    return pos - start;
}

/**
 * Match a single character at text[pos] and return the index of the
 * next character upon success.  Return -1 on failure.  If
 * isRuleWhiteSpace(ch) then match a run of white space in text.
 */
int32_t DecimalFormat::match(const UnicodeString& text, int32_t pos, UChar32 ch) {
    if (uprv_isRuleWhiteSpace(ch)) {
        // Advance over run of white space in input text
        // Must see at least one white space char in input
        int32_t s = pos;
        pos = skipRuleWhiteSpace(text, pos);
        if (pos == s) {
            return -1;
        }
        return pos;
    }
    return (pos >= 0 && text.char32At(pos) == ch) ?
        (pos + U16_LENGTH(ch)) : -1;
}

/**
 * Match a string at text[pos] and return the index of the next
 * character upon success.  Return -1 on failure.  Match a run of
 * white space in str with a run of white space in text.
 */
int32_t DecimalFormat::match(const UnicodeString& text, int32_t pos, const UnicodeString& str) {
    for (int32_t i=0; i<str.length() && pos >= 0; ) {
        UChar32 ch = str.char32At(i);
        i += U16_LENGTH(ch);
        if (uprv_isRuleWhiteSpace(ch)) {
            i = skipRuleWhiteSpace(str, i);
        }
        pos = match(text, pos, ch);
    }
    return pos;
}

//------------------------------------------------------------------------------
// Gets the pointer to the localized decimal format symbols

const DecimalFormatSymbols*
DecimalFormat::getDecimalFormatSymbols() const
{
    return fSymbols;
}

//------------------------------------------------------------------------------
// De-owning the current localized symbols and adopt the new symbols.

void
DecimalFormat::adoptDecimalFormatSymbols(DecimalFormatSymbols* symbolsToAdopt)
{
    if (symbolsToAdopt == NULL) {
        return; // do not allow caller to set fSymbols to NULL
    }

    UBool sameSymbols = FALSE;
    if (fSymbols != NULL) {
        sameSymbols = (UBool)(getConstSymbol(DecimalFormatSymbols::kCurrencySymbol) ==
            symbolsToAdopt->getConstSymbol(DecimalFormatSymbols::kCurrencySymbol) &&
            getConstSymbol(DecimalFormatSymbols::kIntlCurrencySymbol) ==
            symbolsToAdopt->getConstSymbol(DecimalFormatSymbols::kIntlCurrencySymbol));
        delete fSymbols;
    }

    fSymbols = symbolsToAdopt;
    if (!sameSymbols) {
        // If the currency symbols are the same, there is no need to recalculate.
        setCurrencyForSymbols();
    }
    expandAffixes(NULL);
}
//------------------------------------------------------------------------------
// Setting the symbols is equlivalent to adopting a newly created localized
// symbols.

void
DecimalFormat::setDecimalFormatSymbols(const DecimalFormatSymbols& symbols)
{
    adoptDecimalFormatSymbols(new DecimalFormatSymbols(symbols));
}


const CurrencyPluralInfo*
DecimalFormat::getCurrencyPluralInfo(void) const
{
    return fCurrencyPluralInfo;
}


void
DecimalFormat::adoptCurrencyPluralInfo(CurrencyPluralInfo* toAdopt)
{
    if (toAdopt != NULL) {
        delete fCurrencyPluralInfo;
        fCurrencyPluralInfo = toAdopt;
        // re-set currency affix patterns and currency affixes.
        if (fCurrencySignCount > fgCurrencySignCountZero) {
            UErrorCode status = U_ZERO_ERROR;
            if (fAffixPatternsForCurrency) {
                deleteHashForAffixPattern();
            }
            setupCurrencyAffixPatterns(status);
            if (fCurrencySignCount == fgCurrencySignCountInPluralFormat) {
                // only setup the affixes of the plural pattern.
                setupCurrencyAffixes(fFormatPattern, FALSE, TRUE, status);
            }
        }
    }
}

void
DecimalFormat::setCurrencyPluralInfo(const CurrencyPluralInfo& info)
{
    adoptCurrencyPluralInfo(info.clone());
}


/**
 * Update the currency object to match the symbols.  This method
 * is used only when the caller has passed in a symbols object
 * that may not be the default object for its locale.
 */
void
DecimalFormat::setCurrencyForSymbols() {
    /*Bug 4212072
      Update the affix strings accroding to symbols in order to keep
      the affix strings up to date.
      [Richard/GCL]
    */

    // With the introduction of the Currency object, the currency
    // symbols in the DFS object are ignored.  For backward
    // compatibility, we check any explicitly set DFS object.  If it
    // is a default symbols object for its locale, we change the
    // currency object to one for that locale.  If it is custom,
    // we set the currency to null.
    UErrorCode ec = U_ZERO_ERROR;
    const UChar* c = NULL;
    const char* loc = fSymbols->getLocale().getName();
    UChar intlCurrencySymbol[4];
    ucurr_forLocale(loc, intlCurrencySymbol, 4, &ec);
    UnicodeString currencySymbol;

    uprv_getStaticCurrencyName(intlCurrencySymbol, loc, currencySymbol, ec);
    if (U_SUCCESS(ec)
        && getConstSymbol(DecimalFormatSymbols::kCurrencySymbol) == currencySymbol
        && getConstSymbol(DecimalFormatSymbols::kIntlCurrencySymbol) == intlCurrencySymbol)
    {
        // Trap an error in mapping locale to currency.  If we can't
        // map, then don't fail and set the currency to "".
        c = intlCurrencySymbol;
    }
    ec = U_ZERO_ERROR; // reset local error code!
    setCurrencyInternally(c, ec);
}


//------------------------------------------------------------------------------
// Gets the positive prefix of the number pattern.

UnicodeString&
DecimalFormat::getPositivePrefix(UnicodeString& result) const
{
    result = fPositivePrefix;
    return result;
}

//------------------------------------------------------------------------------
// Sets the positive prefix of the number pattern.

void
DecimalFormat::setPositivePrefix(const UnicodeString& newValue)
{
    fPositivePrefix = newValue;
    delete fPosPrefixPattern;
    fPosPrefixPattern = 0;
}

//------------------------------------------------------------------------------
// Gets the negative prefix  of the number pattern.

UnicodeString&
DecimalFormat::getNegativePrefix(UnicodeString& result) const
{
    result = fNegativePrefix;
    return result;
}

//------------------------------------------------------------------------------
// Gets the negative prefix  of the number pattern.

void
DecimalFormat::setNegativePrefix(const UnicodeString& newValue)
{
    fNegativePrefix = newValue;
    delete fNegPrefixPattern;
    fNegPrefixPattern = 0;
}

//------------------------------------------------------------------------------
// Gets the positive suffix of the number pattern.

UnicodeString&
DecimalFormat::getPositiveSuffix(UnicodeString& result) const
{
    result = fPositiveSuffix;
    return result;
}

//------------------------------------------------------------------------------
// Sets the positive suffix of the number pattern.

void
DecimalFormat::setPositiveSuffix(const UnicodeString& newValue)
{
    fPositiveSuffix = newValue;
    delete fPosSuffixPattern;
    fPosSuffixPattern = 0;
}

//------------------------------------------------------------------------------
// Gets the negative suffix of the number pattern.

UnicodeString&
DecimalFormat::getNegativeSuffix(UnicodeString& result) const
{
    result = fNegativeSuffix;
    return result;
}

//------------------------------------------------------------------------------
// Sets the negative suffix of the number pattern.

void
DecimalFormat::setNegativeSuffix(const UnicodeString& newValue)
{
    fNegativeSuffix = newValue;
    delete fNegSuffixPattern;
    fNegSuffixPattern = 0;
}

//------------------------------------------------------------------------------
// Gets the multiplier of the number pattern.
//   Multipliers are stored as decimal numbers (DigitLists) because that
//      is the most convenient for muliplying or dividing the numbers to be formatted.
//   A NULL multiplier implies one, and the scaling operations are skipped.

int32_t 
DecimalFormat::getMultiplier() const
{
    if (fMultiplier == NULL) {
        return 1;
    } else {
        return fMultiplier->getLong();
    }
}

//------------------------------------------------------------------------------
// Sets the multiplier of the number pattern.
void
DecimalFormat::setMultiplier(int32_t newValue)
{
//  if (newValue == 0) {
//      throw new IllegalArgumentException("Bad multiplier: " + newValue);
//  }
    if (newValue == 0) {
        newValue = 1;     // one being the benign default value for a multiplier.
    }
    if (newValue == 1) {
        delete fMultiplier;
        fMultiplier = NULL;
    } else {
        if (fMultiplier == NULL) {
            fMultiplier = new DigitList;
        }
        if (fMultiplier != NULL) {
            fMultiplier->set(newValue);
        }
    }
}

/**
 * Get the rounding increment.
 * @return A positive rounding increment, or 0.0 if rounding
 * is not in effect.
 * @see #setRoundingIncrement
 * @see #getRoundingMode
 * @see #setRoundingMode
 */
double DecimalFormat::getRoundingIncrement() const {
    if (fRoundingIncrement == NULL) {
        return 0.0;
    } else {
        return fRoundingIncrement->getDouble();
    }
}

/**
 * Set the rounding increment.  This method also controls whether
 * rounding is enabled.
 * @param newValue A positive rounding increment, or 0.0 to disable rounding.
 * Negative increments are equivalent to 0.0.
 * @see #getRoundingIncrement
 * @see #getRoundingMode
 * @see #setRoundingMode
 */
void DecimalFormat::setRoundingIncrement(double newValue) {
    if (newValue > 0.0) {
        if (fRoundingIncrement == NULL) {
            fRoundingIncrement = new DigitList();
        }
        if (fRoundingIncrement != NULL) {
            fRoundingIncrement->set(newValue);
            return;
        }
    }
    // These statements are executed if newValue is less than 0.0
    // or fRoundingIncrement could not be created.
    delete fRoundingIncrement;
    fRoundingIncrement = NULL;
}

/**
 * Get the rounding mode.
 * @return A rounding mode
 * @see #setRoundingIncrement
 * @see #getRoundingIncrement
 * @see #setRoundingMode
 */
DecimalFormat::ERoundingMode DecimalFormat::getRoundingMode() const {
    return fRoundingMode;
}

/**
 * Set the rounding mode.  This has no effect unless the rounding
 * increment is greater than zero.
 * @param roundingMode A rounding mode
 * @see #setRoundingIncrement
 * @see #getRoundingIncrement
 * @see #getRoundingMode
 */
void DecimalFormat::setRoundingMode(ERoundingMode roundingMode) {
    fRoundingMode = roundingMode;
}

/**
 * Get the width to which the output of <code>format()</code> is padded.
 * @return the format width, or zero if no padding is in effect
 * @see #setFormatWidth
 * @see #getPadCharacter
 * @see #setPadCharacter
 * @see #getPadPosition
 * @see #setPadPosition
 */
int32_t DecimalFormat::getFormatWidth() const {
    return fFormatWidth;
}

/**
 * Set the width to which the output of <code>format()</code> is padded.
 * This method also controls whether padding is enabled.
 * @param width the width to which to pad the result of
 * <code>format()</code>, or zero to disable padding.  A negative
 * width is equivalent to 0.
 * @see #getFormatWidth
 * @see #getPadCharacter
 * @see #setPadCharacter
 * @see #getPadPosition
 * @see #setPadPosition
 */
void DecimalFormat::setFormatWidth(int32_t width) {
    fFormatWidth = (width > 0) ? width : 0;
}

UnicodeString DecimalFormat::getPadCharacterString() const {
    return fPad;
}

void DecimalFormat::setPadCharacter(const UnicodeString &padChar) {
    if (padChar.length() > 0) {
        fPad = padChar.char32At(0);
    }
    else {
        fPad = kDefaultPad;
    }
}

/**
 * Get the position at which padding will take place.  This is the location
 * at which padding will be inserted if the result of <code>format()</code>
 * is shorter than the format width.
 * @return the pad position, one of <code>kPadBeforePrefix</code>,
 * <code>kPadAfterPrefix</code>, <code>kPadBeforeSuffix</code>, or
 * <code>kPadAfterSuffix</code>.
 * @see #setFormatWidth
 * @see #getFormatWidth
 * @see #setPadCharacter
 * @see #getPadCharacter
 * @see #setPadPosition
 * @see #kPadBeforePrefix
 * @see #kPadAfterPrefix
 * @see #kPadBeforeSuffix
 * @see #kPadAfterSuffix
 */
DecimalFormat::EPadPosition DecimalFormat::getPadPosition() const {
    return fPadPosition;
}

/**
 * <strong><font face=helvetica color=red>NEW</font></strong>
 * Set the position at which padding will take place.  This is the location
 * at which padding will be inserted if the result of <code>format()</code>
 * is shorter than the format width.  This has no effect unless padding is
 * enabled.
 * @param padPos the pad position, one of <code>kPadBeforePrefix</code>,
 * <code>kPadAfterPrefix</code>, <code>kPadBeforeSuffix</code>, or
 * <code>kPadAfterSuffix</code>.
 * @see #setFormatWidth
 * @see #getFormatWidth
 * @see #setPadCharacter
 * @see #getPadCharacter
 * @see #getPadPosition
 * @see #kPadBeforePrefix
 * @see #kPadAfterPrefix
 * @see #kPadBeforeSuffix
 * @see #kPadAfterSuffix
 */
void DecimalFormat::setPadPosition(EPadPosition padPos) {
    fPadPosition = padPos;
}

/**
 * Return whether or not scientific notation is used.
 * @return TRUE if this object formats and parses scientific notation
 * @see #setScientificNotation
 * @see #getMinimumExponentDigits
 * @see #setMinimumExponentDigits
 * @see #isExponentSignAlwaysShown
 * @see #setExponentSignAlwaysShown
 */
UBool DecimalFormat::isScientificNotation() {
    return fUseExponentialNotation;
}

/**
 * Set whether or not scientific notation is used.
 * @param useScientific TRUE if this object formats and parses scientific
 * notation
 * @see #isScientificNotation
 * @see #getMinimumExponentDigits
 * @see #setMinimumExponentDigits
 * @see #isExponentSignAlwaysShown
 * @see #setExponentSignAlwaysShown
 */
void DecimalFormat::setScientificNotation(UBool useScientific) {
    fUseExponentialNotation = useScientific;
}

/**
 * Return the minimum exponent digits that will be shown.
 * @return the minimum exponent digits that will be shown
 * @see #setScientificNotation
 * @see #isScientificNotation
 * @see #setMinimumExponentDigits
 * @see #isExponentSignAlwaysShown
 * @see #setExponentSignAlwaysShown
 */
int8_t DecimalFormat::getMinimumExponentDigits() const {
    return fMinExponentDigits;
}

/**
 * Set the minimum exponent digits that will be shown.  This has no
 * effect unless scientific notation is in use.
 * @param minExpDig a value >= 1 indicating the fewest exponent digits
 * that will be shown.  Values less than 1 will be treated as 1.
 * @see #setScientificNotation
 * @see #isScientificNotation
 * @see #getMinimumExponentDigits
 * @see #isExponentSignAlwaysShown
 * @see #setExponentSignAlwaysShown
 */
void DecimalFormat::setMinimumExponentDigits(int8_t minExpDig) {
    fMinExponentDigits = (int8_t)((minExpDig > 0) ? minExpDig : 1);
}

/**
 * Return whether the exponent sign is always shown.
 * @return TRUE if the exponent is always prefixed with either the
 * localized minus sign or the localized plus sign, false if only negative
 * exponents are prefixed with the localized minus sign.
 * @see #setScientificNotation
 * @see #isScientificNotation
 * @see #setMinimumExponentDigits
 * @see #getMinimumExponentDigits
 * @see #setExponentSignAlwaysShown
 */
UBool DecimalFormat::isExponentSignAlwaysShown() {
    return fExponentSignAlwaysShown;
}

/**
 * Set whether the exponent sign is always shown.  This has no effect
 * unless scientific notation is in use.
 * @param expSignAlways TRUE if the exponent is always prefixed with either
 * the localized minus sign or the localized plus sign, false if only
 * negative exponents are prefixed with the localized minus sign.
 * @see #setScientificNotation
 * @see #isScientificNotation
 * @see #setMinimumExponentDigits
 * @see #getMinimumExponentDigits
 * @see #isExponentSignAlwaysShown
 */
void DecimalFormat::setExponentSignAlwaysShown(UBool expSignAlways) {
    fExponentSignAlwaysShown = expSignAlways;
}

//------------------------------------------------------------------------------
// Gets the grouping size of the number pattern.  For example, thousand or 10
// thousand groupings.

int32_t
DecimalFormat::getGroupingSize() const
{
    return fGroupingSize;
}

//------------------------------------------------------------------------------
// Gets the grouping size of the number pattern.

void
DecimalFormat::setGroupingSize(int32_t newValue)
{
    fGroupingSize = newValue;
}

//------------------------------------------------------------------------------

int32_t
DecimalFormat::getSecondaryGroupingSize() const
{
    return fGroupingSize2;
}

//------------------------------------------------------------------------------

void
DecimalFormat::setSecondaryGroupingSize(int32_t newValue)
{
    fGroupingSize2 = newValue;
}

//------------------------------------------------------------------------------
// Checks if to show the decimal separator.

UBool
DecimalFormat::isDecimalSeparatorAlwaysShown() const
{
    return fDecimalSeparatorAlwaysShown;
}

//------------------------------------------------------------------------------
// Sets to always show the decimal separator.

void
DecimalFormat::setDecimalSeparatorAlwaysShown(UBool newValue)
{
    fDecimalSeparatorAlwaysShown = newValue;
}

//------------------------------------------------------------------------------
// Emits the pattern of this DecimalFormat instance.

UnicodeString&
DecimalFormat::toPattern(UnicodeString& result) const
{
    return toPattern(result, FALSE);
}

//------------------------------------------------------------------------------
// Emits the localized pattern this DecimalFormat instance.

UnicodeString&
DecimalFormat::toLocalizedPattern(UnicodeString& result) const
{
    return toPattern(result, TRUE);
}

//------------------------------------------------------------------------------
/**
 * Expand the affix pattern strings into the expanded affix strings.  If any
 * affix pattern string is null, do not expand it.  This method should be
 * called any time the symbols or the affix patterns change in order to keep
 * the expanded affix strings up to date.
 * This method also will be called before formatting if format currency
 * plural names, since the plural name is not a static one, it is
 * based on the currency plural count, the affix will be known only
 * after the currency plural count is know.
 * In which case, the parameter
 * 'pluralCount' will be a non-null currency plural count.
 * In all other cases, the 'pluralCount' is null, which means it is not needed.
 */
void DecimalFormat::expandAffixes(const UnicodeString* pluralCount) {
    FieldPositionHandler none;
    if (fPosPrefixPattern != 0) {
      expandAffix(*fPosPrefixPattern, fPositivePrefix, 0, none, FALSE, pluralCount);
    }
    if (fPosSuffixPattern != 0) {
      expandAffix(*fPosSuffixPattern, fPositiveSuffix, 0, none, FALSE, pluralCount);
    }
    if (fNegPrefixPattern != 0) {
      expandAffix(*fNegPrefixPattern, fNegativePrefix, 0, none, FALSE, pluralCount);
    }
    if (fNegSuffixPattern != 0) {
      expandAffix(*fNegSuffixPattern, fNegativeSuffix, 0, none, FALSE, pluralCount);
    }
#ifdef FMT_DEBUG
    UnicodeString s;
    s.append("[")
        .append(*fPosPrefixPattern).append("|").append(*fPosSuffixPattern)
        .append(";") .append(*fNegPrefixPattern).append("|").append(*fNegSuffixPattern)
        .append("]->[")
        .append(fPositivePrefix).append("|").append(fPositiveSuffix)
        .append(";") .append(fNegativePrefix).append("|").append(fNegativeSuffix)
        .append("]\n");
    debugout(s);
#endif
}

/**
 * Expand an affix pattern into an affix string.  All characters in the
 * pattern are literal unless prefixed by kQuote.  The following characters
 * after kQuote are recognized: PATTERN_PERCENT, PATTERN_PER_MILLE,
 * PATTERN_MINUS, and kCurrencySign.  If kCurrencySign is doubled (kQuote +
 * kCurrencySign + kCurrencySign), it is interpreted as an international
 * currency sign. If CURRENCY_SIGN is tripled, it is interpreted as
 * currency plural long names, such as "US Dollars".
 * Any other character after a kQuote represents itself.
 * kQuote must be followed by another character; kQuote may not occur by
 * itself at the end of the pattern.
 *
 * This method is used in two distinct ways.  First, it is used to expand
 * the stored affix patterns into actual affixes.  For this usage, doFormat
 * must be false.  Second, it is used to expand the stored affix patterns
 * given a specific number (doFormat == true), for those rare cases in
 * which a currency format references a ChoiceFormat (e.g., en_IN display
 * name for INR).  The number itself is taken from digitList.
 *
 * When used in the first way, this method has a side effect: It sets
 * currencyChoice to a ChoiceFormat object, if the currency's display name
 * in this locale is a ChoiceFormat pattern (very rare).  It only does this
 * if currencyChoice is null to start with.
 *
 * @param pattern the non-null, fPossibly empty pattern
 * @param affix string to receive the expanded equivalent of pattern.
 * Previous contents are deleted.
 * @param doFormat if false, then the pattern will be expanded, and if a
 * currency symbol is encountered that expands to a ChoiceFormat, the
 * currencyChoice member variable will be initialized if it is null.  If
 * doFormat is true, then it is assumed that the currencyChoice has been
 * created, and it will be used to format the value in digitList.
 * @param pluralCount the plural count. It is only used for currency
 *                    plural format. In which case, it is the plural
 *                    count of the currency amount. For example,
 *                    in en_US, it is the singular "one", or the plural
 *                    "other". For all other cases, it is null, and
 *                    is not being used.
 */
void DecimalFormat::expandAffix(const UnicodeString& pattern,
                                UnicodeString& affix,
                                double number,
                                FieldPositionHandler& handler,
                                UBool doFormat,
                                const UnicodeString* pluralCount) const {
    affix.remove();
    for (int i=0; i<pattern.length(); ) {
        UChar32 c = pattern.char32At(i);
        i += U16_LENGTH(c);
        if (c == kQuote) {
            c = pattern.char32At(i);
            i += U16_LENGTH(c);
            int beginIdx = affix.length();
            switch (c) {
            case kCurrencySign: {
                // As of ICU 2.2 we use the currency object, and
                // ignore the currency symbols in the DFS, unless
                // we have a null currency object.  This occurs if
                // resurrecting a pre-2.2 object or if the user
                // sets a custom DFS.
                UBool intl = i<pattern.length() &&
                    pattern.char32At(i) == kCurrencySign;
                UBool plural = FALSE;
                if (intl) {
                    ++i;
                    plural = i<pattern.length() &&
                        pattern.char32At(i) == kCurrencySign;
                    if (plural) {
                        intl = FALSE;
                        ++i;
                    }
                }
                const UChar* currencyUChars = getCurrency();
                if (currencyUChars[0] != 0) {
                    UErrorCode ec = U_ZERO_ERROR;
                    if (plural && pluralCount != NULL) {
                        // plural name is only needed when pluralCount != null,
                        // which means when formatting currency plural names.
                        // For other cases, pluralCount == null,
                        // and plural names are not needed.
                        int32_t len;
                        // TODO: num of char in plural count
                        char pluralCountChar[10];
                        if (pluralCount->length() >= 10) {
                            break;
                        }
                        pluralCount->extract(0, pluralCount->length(), pluralCountChar);
                        UBool isChoiceFormat;
                        const UChar* s = ucurr_getPluralName(currencyUChars,
                            fSymbols != NULL ? fSymbols->getLocale().getName() :
                            Locale::getDefault().getName(), &isChoiceFormat,
                            pluralCountChar, &len, &ec);
                        affix += UnicodeString(s, len);
                        handler.addAttribute(kCurrencyField, beginIdx, affix.length());
                    } else if(intl) {
                        affix += currencyUChars;
                        handler.addAttribute(kCurrencyField, beginIdx, affix.length());
                    } else {
                        int32_t len;
                        UBool isChoiceFormat;
                        // If fSymbols is NULL, use default locale
                        const UChar* s = ucurr_getName(currencyUChars,
                            fSymbols != NULL ? fSymbols->getLocale().getName() : Locale::getDefault().getName(),
                            UCURR_SYMBOL_NAME, &isChoiceFormat, &len, &ec);
                        if (isChoiceFormat) {
                            // Two modes here: If doFormat is false, we set up
                            // currencyChoice.  If doFormat is true, we use the
                            // previously created currencyChoice to format the
                            // value in digitList.
                            if (!doFormat) {
                                // If the currency is handled by a ChoiceFormat,
                                // then we're not going to use the expanded
                                // patterns.  Instantiate the ChoiceFormat and
                                // return.
                                if (fCurrencyChoice == NULL) {
                                    // TODO Replace double-check with proper thread-safe code
                                    ChoiceFormat* fmt = new ChoiceFormat(s, ec);
                                    if (U_SUCCESS(ec)) {
                                        umtx_lock(NULL);
                                        if (fCurrencyChoice == NULL) {
                                            // Cast away const
                                            ((DecimalFormat*)this)->fCurrencyChoice = fmt;
                                            fmt = NULL;
                                        }
                                        umtx_unlock(NULL);
                                        delete fmt;
                                    }
                                }
                                // We could almost return null or "" here, since the
                                // expanded affixes are almost not used at all
                                // in this situation.  However, one method --
                                // toPattern() -- still does use the expanded
                                // affixes, in order to set up a padding
                                // pattern.  We use the CURRENCY_SIGN as a
                                // placeholder.
                                affix.append(kCurrencySign);
                            } else {
                                if (fCurrencyChoice != NULL) {
                                    FieldPosition pos(0); // ignored
                                    if (number < 0) {
                                        number = -number;
                                    }
                                    fCurrencyChoice->format(number, affix, pos);
                                } else {
                                    // We only arrive here if the currency choice
                                    // format in the locale data is INVALID.
                                    affix += currencyUChars;
                                    handler.addAttribute(kCurrencyField, beginIdx, affix.length());
                                }
                            }
                            continue;
                        }
                        affix += UnicodeString(s, len);
                        handler.addAttribute(kCurrencyField, beginIdx, affix.length());
                    }
                } else {
                    if(intl) {
                        affix += getConstSymbol(DecimalFormatSymbols::kIntlCurrencySymbol);
                    } else {
                        affix += getConstSymbol(DecimalFormatSymbols::kCurrencySymbol);
                    }
                    handler.addAttribute(kCurrencyField, beginIdx, affix.length());
                }
                break;
            }
            case kPatternPercent:
                affix += getConstSymbol(DecimalFormatSymbols::kPercentSymbol);
                handler.addAttribute(kPercentField, beginIdx, affix.length());
                break;
            case kPatternPerMill:
                affix += getConstSymbol(DecimalFormatSymbols::kPerMillSymbol);
                handler.addAttribute(kPermillField, beginIdx, affix.length());
                break;
            case kPatternPlus:
                affix += getConstSymbol(DecimalFormatSymbols::kPlusSignSymbol);
                handler.addAttribute(kSignField, beginIdx, affix.length());
                break;
            case kPatternMinus:
                affix += getConstSymbol(DecimalFormatSymbols::kMinusSignSymbol);
                handler.addAttribute(kSignField, beginIdx, affix.length());
                break;
            default:
                affix.append(c);
                break;
            }
        }
        else {
            affix.append(c);
        }
    }
}

/**
 * Append an affix to the given StringBuffer.
 * @param buf buffer to append to
 * @param isNegative
 * @param isPrefix
 */
int32_t DecimalFormat::appendAffix(UnicodeString& buf, double number,
                                   FieldPositionHandler& handler,
                                   UBool isNegative, UBool isPrefix) const {
    // plural format precedes choice format
    if (fCurrencyChoice != 0 &&
        fCurrencySignCount != fgCurrencySignCountInPluralFormat) {
        const UnicodeString* affixPat;
        if (isPrefix) {
            affixPat = isNegative ? fNegPrefixPattern : fPosPrefixPattern;
        } else {
            affixPat = isNegative ? fNegSuffixPattern : fPosSuffixPattern;
        }
        if (affixPat) {
            UnicodeString affixBuf;
            expandAffix(*affixPat, affixBuf, number, handler, TRUE, NULL);
            buf.append(affixBuf);
            return affixBuf.length();
        }
        // else someone called a function that reset the pattern.
    }

    const UnicodeString* affix;
    if (fCurrencySignCount == fgCurrencySignCountInPluralFormat) {
        UnicodeString pluralCount = fCurrencyPluralInfo->getPluralRules()->select(number);
        AffixesForCurrency* oneSet;
        if (fStyle == NumberFormat::kPluralCurrencyStyle) {
            oneSet = (AffixesForCurrency*)fPluralAffixesForCurrency->get(pluralCount);
        } else {
            oneSet = (AffixesForCurrency*)fAffixesForCurrency->get(pluralCount);
        }
        if (isPrefix) {
            affix = isNegative ? &oneSet->negPrefixForCurrency :
                                 &oneSet->posPrefixForCurrency;
        } else {
            affix = isNegative ? &oneSet->negSuffixForCurrency :
                                 &oneSet->posSuffixForCurrency;
        }
    } else {
        if (isPrefix) {
            affix = isNegative ? &fNegativePrefix : &fPositivePrefix;
        } else {
            affix = isNegative ? &fNegativeSuffix : &fPositiveSuffix;
        }
    }

    int32_t begin = (int) buf.length();

    buf.append(*affix);

    if (handler.isRecording()) {
      int32_t offset = (int) (*affix).indexOf(getConstSymbol(DecimalFormatSymbols::kCurrencySymbol));
      if (offset > -1) {
        UnicodeString aff = getConstSymbol(DecimalFormatSymbols::kCurrencySymbol);
        handler.addAttribute(kCurrencyField, begin + offset, begin + offset + aff.length());
      }

      offset = (int) (*affix).indexOf(getConstSymbol(DecimalFormatSymbols::kIntlCurrencySymbol));
      if (offset > -1) {
        UnicodeString aff = getConstSymbol(DecimalFormatSymbols::kIntlCurrencySymbol);
        handler.addAttribute(kCurrencyField, begin + offset, begin + offset + aff.length());
      }

      offset = (int) (*affix).indexOf(getConstSymbol(DecimalFormatSymbols::kMinusSignSymbol));
      if (offset > -1) {
        UnicodeString aff = getConstSymbol(DecimalFormatSymbols::kMinusSignSymbol);
        handler.addAttribute(kSignField, begin + offset, begin + offset + aff.length());
      }

      offset = (int) (*affix).indexOf(getConstSymbol(DecimalFormatSymbols::kPercentSymbol));
      if (offset > -1) {
        UnicodeString aff = getConstSymbol(DecimalFormatSymbols::kPercentSymbol);
        handler.addAttribute(kPercentField, begin + offset, begin + offset + aff.length());
      }

      offset = (int) (*affix).indexOf(getConstSymbol(DecimalFormatSymbols::kPerMillSymbol));
      if (offset > -1) {
        UnicodeString aff = getConstSymbol(DecimalFormatSymbols::kPerMillSymbol);
        handler.addAttribute(kPermillField, begin + offset, begin + offset + aff.length());
      }
    }
    return affix->length();
}

/**
 * Appends an affix pattern to the given StringBuffer, quoting special
 * characters as needed.  Uses the internal affix pattern, if that exists,
 * or the literal affix, if the internal affix pattern is null.  The
 * appended string will generate the same affix pattern (or literal affix)
 * when passed to toPattern().
 *
 * @param appendTo the affix string is appended to this
 * @param affixPattern a pattern such as fPosPrefixPattern; may be null
 * @param expAffix a corresponding expanded affix, such as fPositivePrefix.
 * Ignored unless affixPattern is null.  If affixPattern is null, then
 * expAffix is appended as a literal affix.
 * @param localized true if the appended pattern should contain localized
 * pattern characters; otherwise, non-localized pattern chars are appended
 */
void DecimalFormat::appendAffixPattern(UnicodeString& appendTo,
                                       const UnicodeString* affixPattern,
                                       const UnicodeString& expAffix,
                                       UBool localized) const {
    if (affixPattern == 0) {
        appendAffixPattern(appendTo, expAffix, localized);
    } else {
        int i;
        for (int pos=0; pos<affixPattern->length(); pos=i) {
            i = affixPattern->indexOf(kQuote, pos);
            if (i < 0) {
                UnicodeString s;
                affixPattern->extractBetween(pos, affixPattern->length(), s);
                appendAffixPattern(appendTo, s, localized);
                break;
            }
            if (i > pos) {
                UnicodeString s;
                affixPattern->extractBetween(pos, i, s);
                appendAffixPattern(appendTo, s, localized);
            }
            UChar32 c = affixPattern->char32At(++i);
            ++i;
            if (c == kQuote) {
                appendTo.append(c).append(c);
                // Fall through and append another kQuote below
            } else if (c == kCurrencySign &&
                       i<affixPattern->length() &&
                       affixPattern->char32At(i) == kCurrencySign) {
                ++i;
                appendTo.append(c).append(c);
            } else if (localized) {
                switch (c) {
                case kPatternPercent:
                    appendTo += getConstSymbol(DecimalFormatSymbols::kPercentSymbol);
                    break;
                case kPatternPerMill:
                    appendTo += getConstSymbol(DecimalFormatSymbols::kPerMillSymbol);
                    break;
                case kPatternPlus:
                    appendTo += getConstSymbol(DecimalFormatSymbols::kPlusSignSymbol);
                    break;
                case kPatternMinus:
                    appendTo += getConstSymbol(DecimalFormatSymbols::kMinusSignSymbol);
                    break;
                default:
                    appendTo.append(c);
                }
            } else {
                appendTo.append(c);
            }
        }
    }
}

/**
 * Append an affix to the given StringBuffer, using quotes if
 * there are special characters.  Single quotes themselves must be
 * escaped in either case.
 */
void
DecimalFormat::appendAffixPattern(UnicodeString& appendTo,
                                  const UnicodeString& affix,
                                  UBool localized) const {
    UBool needQuote;
    if(localized) {
        needQuote = affix.indexOf(getConstSymbol(DecimalFormatSymbols::kZeroDigitSymbol)) >= 0
            || affix.indexOf(getConstSymbol(DecimalFormatSymbols::kGroupingSeparatorSymbol)) >= 0
            || affix.indexOf(getConstSymbol(DecimalFormatSymbols::kDecimalSeparatorSymbol)) >= 0
            || affix.indexOf(getConstSymbol(DecimalFormatSymbols::kPercentSymbol)) >= 0
            || affix.indexOf(getConstSymbol(DecimalFormatSymbols::kPerMillSymbol)) >= 0
            || affix.indexOf(getConstSymbol(DecimalFormatSymbols::kDigitSymbol)) >= 0
            || affix.indexOf(getConstSymbol(DecimalFormatSymbols::kPatternSeparatorSymbol)) >= 0
            || affix.indexOf(getConstSymbol(DecimalFormatSymbols::kPlusSignSymbol)) >= 0
            || affix.indexOf(getConstSymbol(DecimalFormatSymbols::kMinusSignSymbol)) >= 0
            || affix.indexOf(kCurrencySign) >= 0;
    }
    else {
        needQuote = affix.indexOf(kPatternZeroDigit) >= 0
            || affix.indexOf(kPatternGroupingSeparator) >= 0
            || affix.indexOf(kPatternDecimalSeparator) >= 0
            || affix.indexOf(kPatternPercent) >= 0
            || affix.indexOf(kPatternPerMill) >= 0
            || affix.indexOf(kPatternDigit) >= 0
            || affix.indexOf(kPatternSeparator) >= 0
            || affix.indexOf(kPatternExponent) >= 0
            || affix.indexOf(kPatternPlus) >= 0
            || affix.indexOf(kPatternMinus) >= 0
            || affix.indexOf(kCurrencySign) >= 0;
    }
    if (needQuote)
        appendTo += (UChar)0x0027 /*'\''*/;
    if (affix.indexOf((UChar)0x0027 /*'\''*/) < 0)
        appendTo += affix;
    else {
        for (int32_t j = 0; j < affix.length(); ) {
            UChar32 c = affix.char32At(j);
            j += U16_LENGTH(c);
            appendTo += c;
            if (c == 0x0027 /*'\''*/)
                appendTo += c;
        }
    }
    if (needQuote)
        appendTo += (UChar)0x0027 /*'\''*/;
}

//------------------------------------------------------------------------------

UnicodeString&
DecimalFormat::toPattern(UnicodeString& result, UBool localized) const
{
    if (fStyle == NumberFormat::kPluralCurrencyStyle) {
        // the prefix or suffix pattern might not be defined yet,
        // so they can not be synthesized,
        // instead, get them directly.
        // but it might not be the actual pattern used in formatting.
        // the actual pattern used in formatting depends on the
        // formatted number's plural count.
        result = fFormatPattern;
        return result;
    }
    result.remove();
    UChar32 zero, sigDigit = kPatternSignificantDigit;
    UnicodeString digit, group;
    int32_t i;
    int32_t roundingDecimalPos = 0; // Pos of decimal in roundingDigits
    UnicodeString roundingDigits;
    int32_t padPos = (fFormatWidth > 0) ? fPadPosition : -1;
    UnicodeString padSpec;
    UBool useSigDig = areSignificantDigitsUsed();

    if (localized) {
        digit.append(getConstSymbol(DecimalFormatSymbols::kDigitSymbol));
        group.append(getConstSymbol(DecimalFormatSymbols::kGroupingSeparatorSymbol));
        zero = getConstSymbol(DecimalFormatSymbols::kZeroDigitSymbol).char32At(0);
        if (useSigDig) {
            sigDigit = getConstSymbol(DecimalFormatSymbols::kSignificantDigitSymbol).char32At(0);
        }
    }
    else {
        digit.append((UChar)kPatternDigit);
        group.append((UChar)kPatternGroupingSeparator);
        zero = (UChar32)kPatternZeroDigit;
    }
    if (fFormatWidth > 0) {
        if (localized) {
            padSpec.append(getConstSymbol(DecimalFormatSymbols::kPadEscapeSymbol));
        }
        else {
            padSpec.append((UChar)kPatternPadEscape);
        }
        padSpec.append(fPad);
    }
    if (fRoundingIncrement != NULL) {
        for(i=0; i<fRoundingIncrement->getCount(); ++i) {
          roundingDigits.append(zero+(fRoundingIncrement->getDigitValue(i))); // Convert to Unicode digit
        }
        roundingDecimalPos = fRoundingIncrement->getDecimalAt();
    }
    for (int32_t part=0; part<2; ++part) {
        if (padPos == kPadBeforePrefix) {
            result.append(padSpec);
        }
        appendAffixPattern(result,
                    (part==0 ? fPosPrefixPattern : fNegPrefixPattern),
                    (part==0 ? fPositivePrefix : fNegativePrefix),
                    localized);
        if (padPos == kPadAfterPrefix && ! padSpec.isEmpty()) {
            result.append(padSpec);
        }
        int32_t sub0Start = result.length();
        int32_t g = isGroupingUsed() ? _max(0, fGroupingSize) : 0;
        if (g > 0 && fGroupingSize2 > 0 && fGroupingSize2 != fGroupingSize) {
            g += fGroupingSize2;
        }
        int32_t maxDig = 0, minDig = 0, maxSigDig = 0;
        if (useSigDig) {
            minDig = getMinimumSignificantDigits();
            maxDig = maxSigDig = getMaximumSignificantDigits();
        } else {
            minDig = getMinimumIntegerDigits();
            maxDig = getMaximumIntegerDigits();
        }
        if (fUseExponentialNotation) {
            if (maxDig > kMaxScientificIntegerDigits) {
                maxDig = 1;
            }
        } else if (useSigDig) {
            maxDig = _max(maxDig, g+1);
        } else {
            maxDig = _max(_max(g, getMinimumIntegerDigits()),
                          roundingDecimalPos) + 1;
        }
        for (i = maxDig; i > 0; --i) {
            if (!fUseExponentialNotation && i<maxDig &&
                isGroupingPosition(i)) {
                result.append(group);
            }
            if (useSigDig) {
                //  #@,@###   (maxSigDig == 5, minSigDig == 2)
                //  65 4321   (1-based pos, count from the right)
                // Use # if pos > maxSigDig or 1 <= pos <= (maxSigDig - minSigDig)
                // Use @ if (maxSigDig - minSigDig) < pos <= maxSigDig
                if (maxSigDig >= i && i > (maxSigDig - minDig)) {
                    result.append(sigDigit);
                } else {
                    result.append(digit);
                }
            } else {
                if (! roundingDigits.isEmpty()) {
                    int32_t pos = roundingDecimalPos - i;
                    if (pos >= 0 && pos < roundingDigits.length()) {
                        result.append((UChar) (roundingDigits.char32At(pos) - kPatternZeroDigit + zero));
                        continue;
                    }
                }
                if (i<=minDig) {
                    result.append(zero);
                } else {
                    result.append(digit);
                }
            }
        }
        if (!useSigDig) {
            if (getMaximumFractionDigits() > 0 || fDecimalSeparatorAlwaysShown) {
                if (localized) {
                    result += getConstSymbol(DecimalFormatSymbols::kDecimalSeparatorSymbol);
                }
                else {
                    result.append((UChar)kPatternDecimalSeparator);
                }
            }
            int32_t pos = roundingDecimalPos;
            for (i = 0; i < getMaximumFractionDigits(); ++i) {
                if (! roundingDigits.isEmpty() && pos < roundingDigits.length()) {
                    if (pos < 0) {
                        result.append(zero);
                    }
                    else {
                        result.append((UChar)(roundingDigits.char32At(pos) - kPatternZeroDigit + zero));
                    }
                    ++pos;
                    continue;
                }
                if (i<getMinimumFractionDigits()) {
                    result.append(zero);
                }
                else {
                    result.append(digit);
                }
            }
        }
        if (fUseExponentialNotation) {
            if (localized) {
                result += getConstSymbol(DecimalFormatSymbols::kExponentialSymbol);
            }
            else {
                result.append((UChar)kPatternExponent);
            }
            if (fExponentSignAlwaysShown) {
                if (localized) {
                    result += getConstSymbol(DecimalFormatSymbols::kPlusSignSymbol);
                }
                else {
                    result.append((UChar)kPatternPlus);
                }
            }
            for (i=0; i<fMinExponentDigits; ++i) {
                result.append(zero);
            }
        }
        if (! padSpec.isEmpty() && !fUseExponentialNotation) {
            int32_t add = fFormatWidth - result.length() + sub0Start
                - ((part == 0)
                   ? fPositivePrefix.length() + fPositiveSuffix.length()
                   : fNegativePrefix.length() + fNegativeSuffix.length());
            while (add > 0) {
                result.insert(sub0Start, digit);
                ++maxDig;
                --add;
                // Only add a grouping separator if we have at least
                // 2 additional characters to be added, so we don't
                // end up with ",###".
                if (add>1 && isGroupingPosition(maxDig)) {
                    result.insert(sub0Start, group);
                    --add;
                }
            }
        }
        if (fPadPosition == kPadBeforeSuffix && ! padSpec.isEmpty()) {
            result.append(padSpec);
        }
        if (part == 0) {
            appendAffixPattern(result, fPosSuffixPattern, fPositiveSuffix, localized);
            if (fPadPosition == kPadAfterSuffix && ! padSpec.isEmpty()) {
                result.append(padSpec);
            }
            UBool isDefault = FALSE;
            if ((fNegSuffixPattern == fPosSuffixPattern && // both null
                 fNegativeSuffix == fPositiveSuffix)
                || (fNegSuffixPattern != 0 && fPosSuffixPattern != 0 &&
                    *fNegSuffixPattern == *fPosSuffixPattern))
            {
                if (fNegPrefixPattern != NULL && fPosPrefixPattern != NULL)
                {
                    int32_t length = fPosPrefixPattern->length();
                    isDefault = fNegPrefixPattern->length() == (length+2) &&
                        (*fNegPrefixPattern)[(int32_t)0] == kQuote &&
                        (*fNegPrefixPattern)[(int32_t)1] == kPatternMinus &&
                        fNegPrefixPattern->compare(2, length, *fPosPrefixPattern, 0, length) == 0;
                }
                if (!isDefault &&
                    fNegPrefixPattern == NULL && fPosPrefixPattern == NULL)
                {
                    int32_t length = fPositivePrefix.length();
                    isDefault = fNegativePrefix.length() == (length+1) &&
                        fNegativePrefix.compare(getConstSymbol(DecimalFormatSymbols::kMinusSignSymbol)) == 0 &&
                        fNegativePrefix.compare(1, length, fPositivePrefix, 0, length) == 0;
                }
            }
            if (isDefault) {
                break; // Don't output default negative subpattern
            } else {
                if (localized) {
                    result += getConstSymbol(DecimalFormatSymbols::kPatternSeparatorSymbol);
                }
                else {
                    result.append((UChar)kPatternSeparator);
                }
            }
        } else {
            appendAffixPattern(result, fNegSuffixPattern, fNegativeSuffix, localized);
            if (fPadPosition == kPadAfterSuffix && ! padSpec.isEmpty()) {
                result.append(padSpec);
            }
        }
    }

    return result;
}

//------------------------------------------------------------------------------

void
DecimalFormat::applyPattern(const UnicodeString& pattern, UErrorCode& status)
{
    UParseError parseError;
    applyPattern(pattern, FALSE, parseError, status);
}

//------------------------------------------------------------------------------

void
DecimalFormat::applyPattern(const UnicodeString& pattern,
                            UParseError& parseError,
                            UErrorCode& status)
{
    applyPattern(pattern, FALSE, parseError, status);
}
//------------------------------------------------------------------------------

void
DecimalFormat::applyLocalizedPattern(const UnicodeString& pattern, UErrorCode& status)
{
    UParseError parseError;
    applyPattern(pattern, TRUE,parseError,status);
}

//------------------------------------------------------------------------------

void
DecimalFormat::applyLocalizedPattern(const UnicodeString& pattern,
                                     UParseError& parseError,
                                     UErrorCode& status)
{
    applyPattern(pattern, TRUE,parseError,status);
}

//------------------------------------------------------------------------------

void
DecimalFormat::applyPatternWithoutExpandAffix(const UnicodeString& pattern,
                                              UBool localized,
                                              UParseError& parseError,
                                              UErrorCode& status)
{
    if (U_FAILURE(status))
    {
        return;
    }
    // Clear error struct
    parseError.offset = -1;
    parseError.preContext[0] = parseError.postContext[0] = (UChar)0;

    // Set the significant pattern symbols
    UChar32 zeroDigit               = kPatternZeroDigit; // '0'
    UChar32 sigDigit                = kPatternSignificantDigit; // '@'
    UnicodeString groupingSeparator ((UChar)kPatternGroupingSeparator);
    UnicodeString decimalSeparator  ((UChar)kPatternDecimalSeparator);
    UnicodeString percent           ((UChar)kPatternPercent);
    UnicodeString perMill           ((UChar)kPatternPerMill);
    UnicodeString digit             ((UChar)kPatternDigit); // '#'
    UnicodeString separator         ((UChar)kPatternSeparator);
    UnicodeString exponent          ((UChar)kPatternExponent);
    UnicodeString plus              ((UChar)kPatternPlus);
    UnicodeString minus             ((UChar)kPatternMinus);
    UnicodeString padEscape         ((UChar)kPatternPadEscape);
    // Substitute with the localized symbols if necessary
    if (localized) {
        zeroDigit = getConstSymbol(DecimalFormatSymbols::kZeroDigitSymbol).char32At(0);
        sigDigit = getConstSymbol(DecimalFormatSymbols::kSignificantDigitSymbol).char32At(0);
        groupingSeparator.  remove().append(getConstSymbol(DecimalFormatSymbols::kGroupingSeparatorSymbol));
        decimalSeparator.   remove().append(getConstSymbol(DecimalFormatSymbols::kDecimalSeparatorSymbol));
        percent.            remove().append(getConstSymbol(DecimalFormatSymbols::kPercentSymbol));
        perMill.            remove().append(getConstSymbol(DecimalFormatSymbols::kPerMillSymbol));
        digit.              remove().append(getConstSymbol(DecimalFormatSymbols::kDigitSymbol));
        separator.          remove().append(getConstSymbol(DecimalFormatSymbols::kPatternSeparatorSymbol));
        exponent.           remove().append(getConstSymbol(DecimalFormatSymbols::kExponentialSymbol));
        plus.               remove().append(getConstSymbol(DecimalFormatSymbols::kPlusSignSymbol));
        minus.              remove().append(getConstSymbol(DecimalFormatSymbols::kMinusSignSymbol));
        padEscape.          remove().append(getConstSymbol(DecimalFormatSymbols::kPadEscapeSymbol));
    }
    UChar nineDigit = (UChar)(zeroDigit + 9);
    int32_t digitLen = digit.length();
    int32_t groupSepLen = groupingSeparator.length();
    int32_t decimalSepLen = decimalSeparator.length();

    int32_t pos = 0;
    int32_t patLen = pattern.length();
    // Part 0 is the positive pattern.  Part 1, if present, is the negative
    // pattern.
    for (int32_t part=0; part<2 && pos<patLen; ++part) {
        // The subpart ranges from 0 to 4: 0=pattern proper, 1=prefix,
        // 2=suffix, 3=prefix in quote, 4=suffix in quote.  Subpart 0 is
        // between the prefix and suffix, and consists of pattern
        // characters.  In the prefix and suffix, percent, perMill, and
        // currency symbols are recognized and translated.
        int32_t subpart = 1, sub0Start = 0, sub0Limit = 0, sub2Limit = 0;

        // It's important that we don't change any fields of this object
        // prematurely.  We set the following variables for the multiplier,
        // grouping, etc., and then only change the actual object fields if
        // everything parses correctly.  This also lets us register
        // the data from part 0 and ignore the part 1, except for the
        // prefix and suffix.
        UnicodeString prefix;
        UnicodeString suffix;
        int32_t decimalPos = -1;
        int32_t multiplier = 1;
        int32_t digitLeftCount = 0, zeroDigitCount = 0, digitRightCount = 0, sigDigitCount = 0;
        int8_t groupingCount = -1;
        int8_t groupingCount2 = -1;
        int32_t padPos = -1;
        UChar32 padChar = 0;
        int32_t roundingPos = -1;
        DigitList roundingInc;
        int8_t expDigits = -1;
        UBool expSignAlways = FALSE;

        // The affix is either the prefix or the suffix.
        UnicodeString* affix = &prefix;

        int32_t start = pos;
        UBool isPartDone = FALSE;
        UChar32 ch;

        for (; !isPartDone && pos < patLen; ) {
            // Todo: account for surrogate pairs
            ch = pattern.char32At(pos);
            switch (subpart) {
            case 0: // Pattern proper subpart (between prefix & suffix)
                // Process the digits, decimal, and grouping characters.  We
                // record five pieces of information.  We expect the digits
                // to occur in the pattern ####00.00####, and we record the
                // number of left digits, zero (central) digits, and right
                // digits.  The position of the last grouping character is
                // recorded (should be somewhere within the first two blocks
                // of characters), as is the position of the decimal point,
                // if any (should be in the zero digits).  If there is no
                // decimal point, then there should be no right digits.
                if (pattern.compare(pos, digitLen, digit) == 0) {
                    if (zeroDigitCount > 0 || sigDigitCount > 0) {
                        ++digitRightCount;
                    } else {
                        ++digitLeftCount;
                    }
                    if (groupingCount >= 0 && decimalPos < 0) {
                        ++groupingCount;
                    }
                    pos += digitLen;
                } else if ((ch >= zeroDigit && ch <= nineDigit) ||
                           ch == sigDigit) {
                    if (digitRightCount > 0) {
                        // Unexpected '0'
                        debug("Unexpected '0'")
                        status = U_UNEXPECTED_TOKEN;
                        syntaxError(pattern,pos,parseError);
                        return;
                    }
                    if (ch == sigDigit) {
                        ++sigDigitCount;
                    } else {
                        ++zeroDigitCount;
                        if (ch != zeroDigit && roundingPos < 0) {
                            roundingPos = digitLeftCount + zeroDigitCount;
                        }
                        if (roundingPos >= 0) {
                            roundingInc.append((char)(ch - zeroDigit + '0'));
                        }
                    }
                    if (groupingCount >= 0 && decimalPos < 0) {
                        ++groupingCount;
                    }
                    pos += U16_LENGTH(ch);
                } else if (pattern.compare(pos, groupSepLen, groupingSeparator) == 0) {
                    if (decimalPos >= 0) {
                        // Grouping separator after decimal
                        debug("Grouping separator after decimal")
                        status = U_UNEXPECTED_TOKEN;
                        syntaxError(pattern,pos,parseError);
                        return;
                    }
                    groupingCount2 = groupingCount;
                    groupingCount = 0;
                    pos += groupSepLen;
                } else if (pattern.compare(pos, decimalSepLen, decimalSeparator) == 0) {
                    if (decimalPos >= 0) {
                        // Multiple decimal separators
                        debug("Multiple decimal separators")
                        status = U_MULTIPLE_DECIMAL_SEPARATORS;
                        syntaxError(pattern,pos,parseError);
                        return;
                    }
                    // Intentionally incorporate the digitRightCount,
                    // even though it is illegal for this to be > 0
                    // at this point.  We check pattern syntax below.
                    decimalPos = digitLeftCount + zeroDigitCount + digitRightCount;
                    pos += decimalSepLen;
                } else {
                    if (pattern.compare(pos, exponent.length(), exponent) == 0) {
                        if (expDigits >= 0) {
                            // Multiple exponential symbols
                            debug("Multiple exponential symbols")
                            status = U_MULTIPLE_EXPONENTIAL_SYMBOLS;
                            syntaxError(pattern,pos,parseError);
                            return;
                        }
                        if (groupingCount >= 0) {
                            // Grouping separator in exponential pattern
                            debug("Grouping separator in exponential pattern")
                            status = U_MALFORMED_EXPONENTIAL_PATTERN;
                            syntaxError(pattern,pos,parseError);
                            return;
                        }
                        pos += exponent.length();
                        // Check for positive prefix
                        if (pos < patLen
                            && pattern.compare(pos, plus.length(), plus) == 0) {
                            expSignAlways = TRUE;
                            pos += plus.length();
                        }
                        // Use lookahead to parse out the exponential part of the
                        // pattern, then jump into suffix subpart.
                        expDigits = 0;
                        while (pos < patLen &&
                               pattern.char32At(pos) == zeroDigit) {
                            ++expDigits;
                            pos += U16_LENGTH(zeroDigit);
                        }

                        // 1. Require at least one mantissa pattern digit
                        // 2. Disallow "#+ @" in mantissa
                        // 3. Require at least one exponent pattern digit
                        if (((digitLeftCount + zeroDigitCount) < 1 &&
                             (sigDigitCount + digitRightCount) < 1) ||
                            (sigDigitCount > 0 && digitLeftCount > 0) ||
                            expDigits < 1) {
                            // Malformed exponential pattern
                            debug("Malformed exponential pattern")
                            status = U_MALFORMED_EXPONENTIAL_PATTERN;
                            syntaxError(pattern,pos,parseError);
                            return;
                        }
                    }
                    // Transition to suffix subpart
                    subpart = 2; // suffix subpart
                    affix = &suffix;
                    sub0Limit = pos;
                    continue;
                }
                break;
            case 1: // Prefix subpart
            case 2: // Suffix subpart
                // Process the prefix / suffix characters
                // Process unquoted characters seen in prefix or suffix
                // subpart.

                // Several syntax characters implicitly begins the
                // next subpart if we are in the prefix; otherwise
                // they are illegal if unquoted.
                if (!pattern.compare(pos, digitLen, digit) ||
                    !pattern.compare(pos, groupSepLen, groupingSeparator) ||
                    !pattern.compare(pos, decimalSepLen, decimalSeparator) ||
                    (ch >= zeroDigit && ch <= nineDigit) ||
                    ch == sigDigit) {
                    if (subpart == 1) { // prefix subpart
                        subpart = 0; // pattern proper subpart
                        sub0Start = pos; // Reprocess this character
                        continue;
                    } else {
                        status = U_UNQUOTED_SPECIAL;
                        syntaxError(pattern,pos,parseError);
                        return;
                    }
                } else if (ch == kCurrencySign) {
                    affix->append(kQuote); // Encode currency
                    // Use lookahead to determine if the currency sign is
                    // doubled or not.
                    U_ASSERT(U16_LENGTH(kCurrencySign) == 1);
                    if ((pos+1) < pattern.length() && pattern[pos+1] == kCurrencySign) {
                        affix->append(kCurrencySign);
                        ++pos; // Skip over the doubled character
                        if ((pos+1) < pattern.length() &&
                            pattern[pos+1] == kCurrencySign) {
                            affix->append(kCurrencySign);
                            ++pos; // Skip over the doubled character
                            fCurrencySignCount = fgCurrencySignCountInPluralFormat;
                        } else {
                            fCurrencySignCount = fgCurrencySignCountInISOFormat;
                        }
                    } else {
                        fCurrencySignCount = fgCurrencySignCountInSymbolFormat;
                    }
                    // Fall through to append(ch)
                } else if (ch == kQuote) {
                    // A quote outside quotes indicates either the opening
                    // quote or two quotes, which is a quote literal.  That is,
                    // we have the first quote in 'do' or o''clock.
                    U_ASSERT(U16_LENGTH(kQuote) == 1);
                    ++pos;
                    if (pos < pattern.length() && pattern[pos] == kQuote) {
                        affix->append(kQuote); // Encode quote
                        // Fall through to append(ch)
                    } else {
                        subpart += 2; // open quote
                        continue;
                    }
                } else if (pattern.compare(pos, separator.length(), separator) == 0) {
                    // Don't allow separators in the prefix, and don't allow
                    // separators in the second pattern (part == 1).
                    if (subpart == 1 || part == 1) {
                        // Unexpected separator
                        debug("Unexpected separator")
                        status = U_UNEXPECTED_TOKEN;
                        syntaxError(pattern,pos,parseError);
                        return;
                    }
                    sub2Limit = pos;
                    isPartDone = TRUE; // Go to next part
                    pos += separator.length();
                    break;
                } else if (pattern.compare(pos, percent.length(), percent) == 0) {
                    // Next handle characters which are appended directly.
                    if (multiplier != 1) {
                        // Too many percent/perMill characters
                        debug("Too many percent characters")
                        status = U_MULTIPLE_PERCENT_SYMBOLS;
                        syntaxError(pattern,pos,parseError);
                        return;
                    }
                    affix->append(kQuote); // Encode percent/perMill
                    affix->append(kPatternPercent); // Use unlocalized pattern char
                    multiplier = 100;
                    pos += percent.length();
                    break;
                } else if (pattern.compare(pos, perMill.length(), perMill) == 0) {
                    // Next handle characters which are appended directly.
                    if (multiplier != 1) {
                        // Too many percent/perMill characters
                        debug("Too many perMill characters")
                        status = U_MULTIPLE_PERMILL_SYMBOLS;
                        syntaxError(pattern,pos,parseError);
                        return;
                    }
                    affix->append(kQuote); // Encode percent/perMill
                    affix->append(kPatternPerMill); // Use unlocalized pattern char
                    multiplier = 1000;
                    pos += perMill.length();
                    break;
                } else if (pattern.compare(pos, padEscape.length(), padEscape) == 0) {
                    if (padPos >= 0 ||               // Multiple pad specifiers
                        (pos+1) == pattern.length()) { // Nothing after padEscape
                        debug("Multiple pad specifiers")
                        status = U_MULTIPLE_PAD_SPECIFIERS;
                        syntaxError(pattern,pos,parseError);
                        return;
                    }
                    padPos = pos;
                    pos += padEscape.length();
                    padChar = pattern.char32At(pos);
                    pos += U16_LENGTH(padChar);
                    break;
                } else if (pattern.compare(pos, minus.length(), minus) == 0) {
                    affix->append(kQuote); // Encode minus
                    affix->append(kPatternMinus);
                    pos += minus.length();
                    break;
                } else if (pattern.compare(pos, plus.length(), plus) == 0) {
                    affix->append(kQuote); // Encode plus
                    affix->append(kPatternPlus);
                    pos += plus.length();
                    break;
                }
                // Unquoted, non-special characters fall through to here, as
                // well as other code which needs to append something to the
                // affix.
                affix->append(ch);
                pos += U16_LENGTH(ch);
                break;
            case 3: // Prefix subpart, in quote
            case 4: // Suffix subpart, in quote
                // A quote within quotes indicates either the closing
                // quote or two quotes, which is a quote literal.  That is,
                // we have the second quote in 'do' or 'don''t'.
                if (ch == kQuote) {
                    ++pos;
                    if (pos < pattern.length() && pattern[pos] == kQuote) {
                        affix->append(kQuote); // Encode quote
                        // Fall through to append(ch)
                    } else {
                        subpart -= 2; // close quote
                        continue;
                    }
                }
                affix->append(ch);
                pos += U16_LENGTH(ch);
                break;
            }
        }

        if (sub0Limit == 0) {
            sub0Limit = pattern.length();
        }

        if (sub2Limit == 0) {
            sub2Limit = pattern.length();
        }

        /* Handle patterns with no '0' pattern character.  These patterns
         * are legal, but must be recodified to make sense.  "##.###" ->
         * "#0.###".  ".###" -> ".0##".
         *
         * We allow patterns of the form "####" to produce a zeroDigitCount
         * of zero (got that?); although this seems like it might make it
         * possible for format() to produce empty strings, format() checks
         * for this condition and outputs a zero digit in this situation.
         * Having a zeroDigitCount of zero yields a minimum integer digits
         * of zero, which allows proper round-trip patterns.  We don't want
         * "#" to become "#0" when toPattern() is called (even though that's
         * what it really is, semantically).
         */
        if (zeroDigitCount == 0 && sigDigitCount == 0 &&
            digitLeftCount > 0 && decimalPos >= 0) {
            // Handle "###.###" and "###." and ".###"
            int n = decimalPos;
            if (n == 0)
                ++n; // Handle ".###"
            digitRightCount = digitLeftCount - n;
            digitLeftCount = n - 1;
            zeroDigitCount = 1;
        }

        // Do syntax checking on the digits, decimal points, and quotes.
        if ((decimalPos < 0 && digitRightCount > 0 && sigDigitCount == 0) ||
            (decimalPos >= 0 &&
             (sigDigitCount > 0 ||
              decimalPos < digitLeftCount ||
              decimalPos > (digitLeftCount + zeroDigitCount))) ||
            groupingCount == 0 || groupingCount2 == 0 ||
            (sigDigitCount > 0 && zeroDigitCount > 0) ||
            subpart > 2)
        { // subpart > 2 == unmatched quote
            debug("Syntax error")
            status = U_PATTERN_SYNTAX_ERROR;
            syntaxError(pattern,pos,parseError);
            return;
        }

        // Make sure pad is at legal position before or after affix.
        if (padPos >= 0) {
            if (padPos == start) {
                padPos = kPadBeforePrefix;
            } else if (padPos+2 == sub0Start) {
                padPos = kPadAfterPrefix;
            } else if (padPos == sub0Limit) {
                padPos = kPadBeforeSuffix;
            } else if (padPos+2 == sub2Limit) {
                padPos = kPadAfterSuffix;
            } else {
                // Illegal pad position
                debug("Illegal pad position")
                status = U_ILLEGAL_PAD_POSITION;
                syntaxError(pattern,pos,parseError);
                return;
            }
        }

        if (part == 0) {
            delete fPosPrefixPattern;
            delete fPosSuffixPattern;
            delete fNegPrefixPattern;
            delete fNegSuffixPattern;
            fPosPrefixPattern = new UnicodeString(prefix);
            /* test for NULL */
            if (fPosPrefixPattern == 0) {
                status = U_MEMORY_ALLOCATION_ERROR;
                return;
            }
            fPosSuffixPattern = new UnicodeString(suffix);
            /* test for NULL */
            if (fPosSuffixPattern == 0) {
                status = U_MEMORY_ALLOCATION_ERROR;
                delete fPosPrefixPattern;
                return;
            }
            fNegPrefixPattern = 0;
            fNegSuffixPattern = 0;

            fUseExponentialNotation = (expDigits >= 0);
            if (fUseExponentialNotation) {
                fMinExponentDigits = expDigits;
            }
            fExponentSignAlwaysShown = expSignAlways;
            int32_t digitTotalCount = digitLeftCount + zeroDigitCount + digitRightCount;
            // The effectiveDecimalPos is the position the decimal is at or
            // would be at if there is no decimal.  Note that if
            // decimalPos<0, then digitTotalCount == digitLeftCount +
            // zeroDigitCount.
            int32_t effectiveDecimalPos = decimalPos >= 0 ? decimalPos : digitTotalCount;
            UBool isSigDig = (sigDigitCount > 0);
            setSignificantDigitsUsed(isSigDig);
            if (isSigDig) {
                setMinimumSignificantDigits(sigDigitCount);
                setMaximumSignificantDigits(sigDigitCount + digitRightCount);
            } else {
                int32_t minInt = effectiveDecimalPos - digitLeftCount;
                setMinimumIntegerDigits(minInt);
                setMaximumIntegerDigits(fUseExponentialNotation
                    ? digitLeftCount + getMinimumIntegerDigits()
                    : kDoubleIntegerDigits);
                setMaximumFractionDigits(decimalPos >= 0
                    ? (digitTotalCount - decimalPos) : 0);
                setMinimumFractionDigits(decimalPos >= 0
                    ? (digitLeftCount + zeroDigitCount - decimalPos) : 0);
            }
            setGroupingUsed(groupingCount > 0);
            fGroupingSize = (groupingCount > 0) ? groupingCount : 0;
            fGroupingSize2 = (groupingCount2 > 0 && groupingCount2 != groupingCount)
                ? groupingCount2 : 0;
            setMultiplier(multiplier);
            setDecimalSeparatorAlwaysShown(decimalPos == 0
                    || decimalPos == digitTotalCount);
            if (padPos >= 0) {
                fPadPosition = (EPadPosition) padPos;
                // To compute the format width, first set up sub0Limit -
                // sub0Start.  Add in prefix/suffix length later.

                // fFormatWidth = prefix.length() + suffix.length() +
                //    sub0Limit - sub0Start;
                fFormatWidth = sub0Limit - sub0Start;
                fPad = padChar;
            } else {
                fFormatWidth = 0;
            }
            if (roundingPos >= 0) {
                roundingInc.setDecimalAt(effectiveDecimalPos - roundingPos);
                if (fRoundingIncrement != NULL) {
                    *fRoundingIncrement = roundingInc;
                } else {
                    fRoundingIncrement = new DigitList(roundingInc);
                    /* test for NULL */
                    if (fRoundingIncrement == NULL) {
                        status = U_MEMORY_ALLOCATION_ERROR;
                        delete fPosPrefixPattern;
                        delete fPosSuffixPattern;
                        return;
                    }
                }
                fRoundingIncrement->getDouble();   // forces caching of double in the DigitList,
                                                   //    makes getting it thread safe.
                fRoundingMode = kRoundHalfEven;
            } else {
                setRoundingIncrement(0.0);
            }
        } else {
            fNegPrefixPattern = new UnicodeString(prefix);
            /* test for NULL */
            if (fNegPrefixPattern == 0) {
                status = U_MEMORY_ALLOCATION_ERROR;
                return;
            }
            fNegSuffixPattern = new UnicodeString(suffix);
            /* test for NULL */
            if (fNegSuffixPattern == 0) {
                delete fNegPrefixPattern;
                status = U_MEMORY_ALLOCATION_ERROR;
                return;
            }
        }
    }

    if (pattern.length() == 0) {
        delete fNegPrefixPattern;
        delete fNegSuffixPattern;
        fNegPrefixPattern = NULL;
        fNegSuffixPattern = NULL;
        if (fPosPrefixPattern != NULL) {
            fPosPrefixPattern->remove();
        } else {
            fPosPrefixPattern = new UnicodeString();
            /* test for NULL */
            if (fPosPrefixPattern == 0) {
                status = U_MEMORY_ALLOCATION_ERROR;
                return;
            }
        }
        if (fPosSuffixPattern != NULL) {
            fPosSuffixPattern->remove();
        } else {
            fPosSuffixPattern = new UnicodeString();
            /* test for NULL */
            if (fPosSuffixPattern == 0) {
                delete fPosPrefixPattern;
                status = U_MEMORY_ALLOCATION_ERROR;
                return;
            }
        }

        setMinimumIntegerDigits(0);
        setMaximumIntegerDigits(kDoubleIntegerDigits);
        setMinimumFractionDigits(0);
        setMaximumFractionDigits(kDoubleFractionDigits);

        fUseExponentialNotation = FALSE;
        fCurrencySignCount = 0;
        setGroupingUsed(FALSE);
        fGroupingSize = 0;
        fGroupingSize2 = 0;
        setMultiplier(1);
        setDecimalSeparatorAlwaysShown(FALSE);
        fFormatWidth = 0;
        setRoundingIncrement(0.0);
    }

    // If there was no negative pattern, or if the negative pattern is
    // identical to the positive pattern, then prepend the minus sign to the
    // positive pattern to form the negative pattern.
    if (fNegPrefixPattern == NULL ||
        (*fNegPrefixPattern == *fPosPrefixPattern
         && *fNegSuffixPattern == *fPosSuffixPattern)) {
        _copy_us_ptr(&fNegSuffixPattern, fPosSuffixPattern);
        if (fNegPrefixPattern == NULL) {
            fNegPrefixPattern = new UnicodeString();
            /* test for NULL */
            if (fNegPrefixPattern == 0) {
                status = U_MEMORY_ALLOCATION_ERROR;
                return;
            }
        } else {
            fNegPrefixPattern->remove();
        }
        fNegPrefixPattern->append(kQuote).append(kPatternMinus)
            .append(*fPosPrefixPattern);
    }
#ifdef FMT_DEBUG
    UnicodeString s;
    s.append("\"").append(pattern).append("\"->");
    debugout(s);
#endif

    // save the pattern
    fFormatPattern = pattern;
}


void
DecimalFormat::expandAffixAdjustWidth(const UnicodeString* pluralCount) {
    expandAffixes(pluralCount);
    if (fFormatWidth > 0) {
        // Finish computing format width (see above)
            // TODO: how to handle fFormatWidth,
            // need to save in f(Plural)AffixesForCurrecy?
            fFormatWidth += fPositivePrefix.length() + fPositiveSuffix.length();
    }
}


void
DecimalFormat::applyPattern(const UnicodeString& pattern,
                            UBool localized,
                            UParseError& parseError,
                            UErrorCode& status)
{
    // do the following re-set first. since they change private data by
    // apply pattern again.
    if (pattern.indexOf(kCurrencySign) != -1) {
        if (fCurrencyPluralInfo == NULL) {
            // initialize currencyPluralInfo if needed
            fCurrencyPluralInfo = new CurrencyPluralInfo(fSymbols->getLocale(), status);
        }
        if (fAffixPatternsForCurrency == NULL) {
            setupCurrencyAffixPatterns(status);
        }
        if (pattern.indexOf(fgTripleCurrencySign) != -1) {
            // only setup the affixes of the current pattern.
            setupCurrencyAffixes(pattern, TRUE, FALSE, status);
        }
    }
    applyPatternWithoutExpandAffix(pattern, localized, parseError, status);
    expandAffixAdjustWidth(NULL);
}


void
DecimalFormat::applyPatternInternally(const UnicodeString& pluralCount,
                                      const UnicodeString& pattern,
                                      UBool localized,
                                      UParseError& parseError,
                                      UErrorCode& status) {
    applyPatternWithoutExpandAffix(pattern, localized, parseError, status);
    expandAffixAdjustWidth(&pluralCount);
}


/**
 * Sets the maximum number of digits allowed in the integer portion of a
 * number. This override limits the integer digit count to 309.
 * @see NumberFormat#setMaximumIntegerDigits
 */
void DecimalFormat::setMaximumIntegerDigits(int32_t newValue) {
    NumberFormat::setMaximumIntegerDigits(_min(newValue, kDoubleIntegerDigits));
}

/**
 * Sets the minimum number of digits allowed in the integer portion of a
 * number. This override limits the integer digit count to 309.
 * @see NumberFormat#setMinimumIntegerDigits
 */
void DecimalFormat::setMinimumIntegerDigits(int32_t newValue) {
    NumberFormat::setMinimumIntegerDigits(_min(newValue, kDoubleIntegerDigits));
}

/**
 * Sets the maximum number of digits allowed in the fraction portion of a
 * number. This override limits the fraction digit count to 340.
 * @see NumberFormat#setMaximumFractionDigits
 */
void DecimalFormat::setMaximumFractionDigits(int32_t newValue) {
    NumberFormat::setMaximumFractionDigits(_min(newValue, kDoubleFractionDigits));
}

/**
 * Sets the minimum number of digits allowed in the fraction portion of a
 * number. This override limits the fraction digit count to 340.
 * @see NumberFormat#setMinimumFractionDigits
 */
void DecimalFormat::setMinimumFractionDigits(int32_t newValue) {
    NumberFormat::setMinimumFractionDigits(_min(newValue, kDoubleFractionDigits));
}

int32_t DecimalFormat::getMinimumSignificantDigits() const {
    return fMinSignificantDigits;
}

int32_t DecimalFormat::getMaximumSignificantDigits() const {
    return fMaxSignificantDigits;
}

void DecimalFormat::setMinimumSignificantDigits(int32_t min) {
    if (min < 1) {
        min = 1;
    }
    // pin max sig dig to >= min
    int32_t max = _max(fMaxSignificantDigits, min);
    fMinSignificantDigits = min;
    fMaxSignificantDigits = max;
}

void DecimalFormat::setMaximumSignificantDigits(int32_t max) {
    if (max < 1) {
        max = 1;
    }
    // pin min sig dig to 1..max
    U_ASSERT(fMinSignificantDigits >= 1);
    int32_t min = _min(fMinSignificantDigits, max);
    fMinSignificantDigits = min;
    fMaxSignificantDigits = max;
}

UBool DecimalFormat::areSignificantDigitsUsed() const {
    return fUseSignificantDigits;
}

void DecimalFormat::setSignificantDigitsUsed(UBool useSignificantDigits) {
    fUseSignificantDigits = useSignificantDigits;
}

void DecimalFormat::setCurrencyInternally(const UChar* theCurrency,
                                          UErrorCode& ec) {
    // If we are a currency format, then modify our affixes to
    // encode the currency symbol for the given currency in our
    // locale, and adjust the decimal digits and rounding for the
    // given currency.

    // Note: The code is ordered so that this object is *not changed*
    // until we are sure we are going to succeed.

    // NULL or empty currency is *legal* and indicates no currency.
    UBool isCurr = (theCurrency && *theCurrency);

    double rounding = 0.0;
    int32_t frac = 0;
    if (fCurrencySignCount > fgCurrencySignCountZero && isCurr) {
        rounding = ucurr_getRoundingIncrement(theCurrency, &ec);
        frac = ucurr_getDefaultFractionDigits(theCurrency, &ec);
    }

    NumberFormat::setCurrency(theCurrency, ec);
    if (U_FAILURE(ec)) return;

    if (fCurrencySignCount > fgCurrencySignCountZero) {
        // NULL or empty currency is *legal* and indicates no currency.
        if (isCurr) {
            setRoundingIncrement(rounding);
            setMinimumFractionDigits(frac);
            setMaximumFractionDigits(frac);
        }
        expandAffixes(NULL);
    }
}

void DecimalFormat::setCurrency(const UChar* theCurrency, UErrorCode& ec) {
    // set the currency before compute affixes to get the right currency names
    NumberFormat::setCurrency(theCurrency, ec);
    if (fFormatPattern.indexOf(fgTripleCurrencySign) != -1) {
        UnicodeString savedPtn = fFormatPattern;
        setupCurrencyAffixes(fFormatPattern, TRUE, TRUE, ec);
        UParseError parseErr;
        applyPattern(savedPtn, FALSE, parseErr, ec);
    }
    // set the currency after apply pattern to get the correct rounding/fraction
    setCurrencyInternally(theCurrency, ec);
}

// Deprecated variant with no UErrorCode parameter
void DecimalFormat::setCurrency(const UChar* theCurrency) {
    UErrorCode ec = U_ZERO_ERROR;
    setCurrency(theCurrency, ec);
}

void DecimalFormat::getEffectiveCurrency(UChar* result, UErrorCode& ec) const {
    if (fSymbols == NULL) {
        ec = U_MEMORY_ALLOCATION_ERROR;
        return;
    }
    ec = U_ZERO_ERROR;
    const UChar* c = getCurrency();
    if (*c == 0) {
        const UnicodeString &intl =
            fSymbols->getConstSymbol(DecimalFormatSymbols::kIntlCurrencySymbol);
        c = intl.getBuffer(); // ok for intl to go out of scope
    }
    u_strncpy(result, c, 3);
    result[3] = 0;
}

/**
 * Return the number of fraction digits to display, or the total
 * number of digits for significant digit formats and exponential
 * formats.
 */
int32_t
DecimalFormat::precision() const {
    if (areSignificantDigitsUsed()) {
        return getMaximumSignificantDigits();
    } else if (fUseExponentialNotation) {
        return getMinimumIntegerDigits() + getMaximumFractionDigits();
    } else {
        return getMaximumFractionDigits();
    }
}


// TODO: template algorithm
Hashtable*
DecimalFormat::initHashForAffix(UErrorCode& status) {
    if ( U_FAILURE(status) ) {
        return NULL;
    }
    Hashtable* hTable;
    if ( (hTable = new Hashtable(TRUE, status)) == NULL ) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return NULL;
    }
    hTable->setValueComparator(decimfmtAffixValueComparator);
    return hTable;
}

Hashtable*
DecimalFormat::initHashForAffixPattern(UErrorCode& status) {
    if ( U_FAILURE(status) ) {
        return NULL;
    }
    Hashtable* hTable;
    if ( (hTable = new Hashtable(TRUE, status)) == NULL ) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return NULL;
    }
    hTable->setValueComparator(decimfmtAffixPatternValueComparator);
    return hTable;
}

void
DecimalFormat::deleteHashForAffix(Hashtable*& table)
{
    if ( table == NULL ) {
        return;
    }
    int32_t pos = -1;
    const UHashElement* element = NULL;
    while ( (element = table->nextElement(pos)) != NULL ) {
        const UHashTok keyTok = element->key;
        const UHashTok valueTok = element->value;
        const AffixesForCurrency* value = (AffixesForCurrency*)valueTok.pointer;
        delete value;
    }
    delete table;
    table = NULL;
}



void
DecimalFormat::deleteHashForAffixPattern()
{
    if ( fAffixPatternsForCurrency == NULL ) {
        return;
    }
    int32_t pos = -1;
    const UHashElement* element = NULL;
    while ( (element = fAffixPatternsForCurrency->nextElement(pos)) != NULL ) {
        const UHashTok keyTok = element->key;
        const UHashTok valueTok = element->value;
        const AffixPatternsForCurrency* value = (AffixPatternsForCurrency*)valueTok.pointer;
        delete value;
    }
    delete fAffixPatternsForCurrency;
    fAffixPatternsForCurrency = NULL;
}


void
DecimalFormat::copyHashForAffixPattern(const Hashtable* source,
                                       Hashtable* target,
                                       UErrorCode& status) {
    if ( U_FAILURE(status) ) {
        return;
    }
    int32_t pos = -1;
    const UHashElement* element = NULL;
    if ( source ) {
        while ( (element = source->nextElement(pos)) != NULL ) {
            const UHashTok keyTok = element->key;
            const UnicodeString* key = (UnicodeString*)keyTok.pointer;
            const UHashTok valueTok = element->value;
            const AffixPatternsForCurrency* value = (AffixPatternsForCurrency*)valueTok.pointer;
            AffixPatternsForCurrency* copy = new AffixPatternsForCurrency(
                value->negPrefixPatternForCurrency,
                value->negSuffixPatternForCurrency,
                value->posPrefixPatternForCurrency,
                value->posSuffixPatternForCurrency,
                value->patternType);
            target->put(UnicodeString(*key), copy, status);
            if ( U_FAILURE(status) ) {
                return;
            }
        }
    }
}



void
DecimalFormat::copyHashForAffix(const Hashtable* source,
                                Hashtable* target,
                                UErrorCode& status) {
    if ( U_FAILURE(status) ) {
        return;
    }
    int32_t pos = -1;
    const UHashElement* element = NULL;
    if ( source ) {
        while ( (element = source->nextElement(pos)) != NULL ) {
            const UHashTok keyTok = element->key;
            const UnicodeString* key = (UnicodeString*)keyTok.pointer;

            const UHashTok valueTok = element->value;
            const AffixesForCurrency* value = (AffixesForCurrency*)valueTok.pointer;
            AffixesForCurrency* copy = new AffixesForCurrency(
                value->negPrefixForCurrency,
                value->negSuffixForCurrency,
                value->posPrefixForCurrency,
                value->posSuffixForCurrency);
            target->put(UnicodeString(*key), copy, status);
            if ( U_FAILURE(status) ) {
                return;
            }
        }
    }
}

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */

//eof
