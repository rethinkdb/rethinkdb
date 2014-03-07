/********************************************************************
 * COPYRIGHT:
 * Copyright (c) 1997-2010, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/dcfmtsym.h"
#include "unicode/decimfmt.h"
#include "unicode/unum.h"
#include "tsdcfmsy.h"

void IntlTestDecimalFormatSymbols::runIndexedTest( int32_t index, UBool exec, const char* &name, char* /*par*/ )
{
    if (exec) logln("TestSuite DecimalFormatSymbols");
    switch (index) {
        case 0: name = "DecimalFormatSymbols test";
                if (exec) {
                    logln("DecimalFormatSymbols test---"); logln("");
                    testSymbols(/*par*/);
                }
                break;

        default: name = ""; break;
    }
}

/**
 * Test the API of DecimalFormatSymbols; primarily a simple get/set set.
 */
void IntlTestDecimalFormatSymbols::testSymbols(/* char *par */)
{
    UErrorCode status = U_ZERO_ERROR;

    DecimalFormatSymbols fr(Locale::getFrench(), status);
    if(U_FAILURE(status)) {
        errcheckln(status, "ERROR: Couldn't create French DecimalFormatSymbols - %s", u_errorName(status));
        return;
    }

    status = U_ZERO_ERROR;
    DecimalFormatSymbols en(Locale::getEnglish(), status);
    if(U_FAILURE(status)) {
        errcheckln(status, "ERROR: Couldn't create English DecimalFormatSymbols - %s", u_errorName(status));
        return;
    }

    if(en == fr || ! (en != fr) ) {
        errln("ERROR: English DecimalFormatSymbols equal to French");
    }

    // just do some VERY basic tests to make sure that get/set work

    UnicodeString zero = en.getSymbol(DecimalFormatSymbols::kZeroDigitSymbol);
    fr.setSymbol(DecimalFormatSymbols::kZeroDigitSymbol, zero);
    if(fr.getSymbol(DecimalFormatSymbols::kZeroDigitSymbol) != en.getSymbol(DecimalFormatSymbols::kZeroDigitSymbol)) {
        errln("ERROR: get/set ZeroDigit failed");
    }

    UnicodeString group = en.getSymbol(DecimalFormatSymbols::kGroupingSeparatorSymbol);
    fr.setSymbol(DecimalFormatSymbols::kGroupingSeparatorSymbol, group);
    if(fr.getSymbol(DecimalFormatSymbols::kGroupingSeparatorSymbol) != en.getSymbol(DecimalFormatSymbols::kGroupingSeparatorSymbol)) {
        errln("ERROR: get/set GroupingSeparator failed");
    }

    UnicodeString decimal = en.getSymbol(DecimalFormatSymbols::kDecimalSeparatorSymbol);
    fr.setSymbol(DecimalFormatSymbols::kDecimalSeparatorSymbol, decimal);
    if(fr.getSymbol(DecimalFormatSymbols::kDecimalSeparatorSymbol) != en.getSymbol(DecimalFormatSymbols::kDecimalSeparatorSymbol)) {
        errln("ERROR: get/set DecimalSeparator failed");
    }

    UnicodeString perMill = en.getSymbol(DecimalFormatSymbols::kPerMillSymbol);
    fr.setSymbol(DecimalFormatSymbols::kPerMillSymbol, perMill);
    if(fr.getSymbol(DecimalFormatSymbols::kPerMillSymbol) != en.getSymbol(DecimalFormatSymbols::kPerMillSymbol)) {
        errln("ERROR: get/set PerMill failed");
    }

    UnicodeString percent = en.getSymbol(DecimalFormatSymbols::kPercentSymbol);
    fr.setSymbol(DecimalFormatSymbols::kPercentSymbol, percent);
    if(fr.getSymbol(DecimalFormatSymbols::kPercentSymbol) != en.getSymbol(DecimalFormatSymbols::kPercentSymbol)) {
        errln("ERROR: get/set Percent failed");
    }

    UnicodeString digit(en.getSymbol(DecimalFormatSymbols::kDigitSymbol));
    fr.setSymbol(DecimalFormatSymbols::kDigitSymbol, digit);
    if(fr.getSymbol(DecimalFormatSymbols::kDigitSymbol) != en.getSymbol(DecimalFormatSymbols::kDigitSymbol)) {
        errln("ERROR: get/set Percent failed");
    }

    UnicodeString patternSeparator = en.getSymbol(DecimalFormatSymbols::kPatternSeparatorSymbol);
    fr.setSymbol(DecimalFormatSymbols::kPatternSeparatorSymbol, patternSeparator);
    if(fr.getSymbol(DecimalFormatSymbols::kPatternSeparatorSymbol) != en.getSymbol(DecimalFormatSymbols::kPatternSeparatorSymbol)) {
        errln("ERROR: get/set PatternSeparator failed");
    }

    UnicodeString infinity(en.getSymbol(DecimalFormatSymbols::kInfinitySymbol));
    fr.setSymbol(DecimalFormatSymbols::kInfinitySymbol, infinity);
    UnicodeString infinity2(fr.getSymbol(DecimalFormatSymbols::kInfinitySymbol));
    if(infinity != infinity2) {
        errln("ERROR: get/set Infinity failed");
    }

    UnicodeString nan(en.getSymbol(DecimalFormatSymbols::kNaNSymbol));
    fr.setSymbol(DecimalFormatSymbols::kNaNSymbol, nan);
    UnicodeString nan2(fr.getSymbol(DecimalFormatSymbols::kNaNSymbol));
    if(nan != nan2) {
        errln("ERROR: get/set NaN failed");
    }

    UnicodeString minusSign = en.getSymbol(DecimalFormatSymbols::kMinusSignSymbol);
    fr.setSymbol(DecimalFormatSymbols::kMinusSignSymbol, minusSign);
    if(fr.getSymbol(DecimalFormatSymbols::kMinusSignSymbol) != en.getSymbol(DecimalFormatSymbols::kMinusSignSymbol)) {
        errln("ERROR: get/set MinusSign failed");
    }

    UnicodeString exponential(en.getSymbol(DecimalFormatSymbols::kExponentialSymbol));
    fr.setSymbol(DecimalFormatSymbols::kExponentialSymbol, exponential);
    if(fr.getSymbol(DecimalFormatSymbols::kExponentialSymbol) != en.getSymbol(DecimalFormatSymbols::kExponentialSymbol)) {
        errln("ERROR: get/set Exponential failed");
    }

    // Test get currency spacing before the currency.
    status = U_ZERO_ERROR;
    for (int32_t i = 0; i < (int32_t)DecimalFormatSymbols::kCurrencySpacingCount; i++) {
        UnicodeString enCurrencyPattern = en.getPatternForCurrencySpacing(
             (DecimalFormatSymbols::ECurrencySpacing)i, TRUE, status);
        if(U_FAILURE(status)) {
            errln("Error: cannot get CurrencyMatch for locale:en");
            status = U_ZERO_ERROR;
        }
        UnicodeString frCurrencyPattern = fr.getPatternForCurrencySpacing(
             (DecimalFormatSymbols::ECurrencySpacing)i, TRUE, status);
        if(U_FAILURE(status)) {
            errln("Error: cannot get CurrencyMatch for locale:fr");
        }
        if (enCurrencyPattern != frCurrencyPattern) {
           errln("ERROR: get CurrencySpacing failed");
        }
    }
    // Test get currencySpacing after the currency.
    status = U_ZERO_ERROR;
    for (int32_t i = 0; i < DecimalFormatSymbols::kCurrencySpacingCount; i++) {
        UnicodeString enCurrencyPattern = en.getPatternForCurrencySpacing(
            (DecimalFormatSymbols::ECurrencySpacing)i, FALSE, status);
        if(U_FAILURE(status)) {
            errln("Error: cannot get CurrencyMatch for locale:en");
            status = U_ZERO_ERROR;
        }
        UnicodeString frCurrencyPattern = fr.getPatternForCurrencySpacing(
             (DecimalFormatSymbols::ECurrencySpacing)i, FALSE, status);
        if(U_FAILURE(status)) {
            errln("Error: cannot get CurrencyMatch for locale:fr");
        }
        if (enCurrencyPattern != frCurrencyPattern) {
            errln("ERROR: get CurrencySpacing failed");
        }
    }
    // Test set curerncySpacing APIs
    status = U_ZERO_ERROR;
    UnicodeString dash = UnicodeString("-");
    en.setPatternForCurrencySpacing(DecimalFormatSymbols::kInsert, TRUE, dash);
    UnicodeString enCurrencyInsert = en.getPatternForCurrencySpacing(
        DecimalFormatSymbols::kInsert, TRUE, status);
    if (dash != enCurrencyInsert) {
        errln("Error: Failed to setCurrencyInsert for locale:en");
    }

    status = U_ZERO_ERROR;
    DecimalFormatSymbols foo(status);

    DecimalFormatSymbols bar(foo);

    en = fr;

    if(en != fr || foo != bar) {
        errln("ERROR: Copy Constructor or Assignment failed");
    }

    // test get/setSymbol()
    if((int) UNUM_FORMAT_SYMBOL_COUNT != (int) DecimalFormatSymbols::kFormatSymbolCount) {
        errln("unum.h and decimfmt.h have inconsistent numbers of format symbols!");
        return;
    }

    int i;
    for(i = 0; i < (int)DecimalFormatSymbols::kFormatSymbolCount; ++i) {
        foo.setSymbol((DecimalFormatSymbols::ENumberFormatSymbol)i, UnicodeString((UChar32)(0x10330 + i)));
    }
    for(i = 0; i < (int)DecimalFormatSymbols::kFormatSymbolCount; ++i) {
        if(foo.getSymbol((DecimalFormatSymbols::ENumberFormatSymbol)i) != UnicodeString((UChar32)(0x10330 + i))) {
            errln("get/setSymbol did not roundtrip, got " +
                  foo.getSymbol((DecimalFormatSymbols::ENumberFormatSymbol)i) +
                  ", expected " +
                  UnicodeString((UChar32)(0x10330 + i)));
        }
    }

    DecimalFormatSymbols sym(Locale::getUS(), status);

    UnicodeString customDecSeperator("S");
    Verify(34.5, (UnicodeString)"00.00", sym, (UnicodeString)"34.50");
    sym.setSymbol(DecimalFormatSymbols::kDecimalSeparatorSymbol, customDecSeperator);
    Verify(34.5, (UnicodeString)"00.00", sym, (UnicodeString)"34S50");
    sym.setSymbol(DecimalFormatSymbols::kPercentSymbol, (UnicodeString)"P");
    Verify(34.5, (UnicodeString)"00 %", sym, (UnicodeString)"3450 P");
    sym.setSymbol(DecimalFormatSymbols::kCurrencySymbol, (UnicodeString)"D");
    Verify(34.5, CharsToUnicodeString("\\u00a4##.##"), sym, (UnicodeString)"D34.5");
    sym.setSymbol(DecimalFormatSymbols::kGroupingSeparatorSymbol, (UnicodeString)"|");
    Verify(3456.5, (UnicodeString)"0,000.##", sym, (UnicodeString)"3|456S5");

}

void IntlTestDecimalFormatSymbols::Verify(double value, const UnicodeString& pattern, DecimalFormatSymbols sym, const UnicodeString& expected){
    UErrorCode status = U_ZERO_ERROR;
    DecimalFormat *df = new DecimalFormat(pattern, sym, status);
    if(U_FAILURE(status)){
        errln("ERROR: construction of decimal format failed");
    }
    UnicodeString buffer;
    FieldPosition pos(FieldPosition::DONT_CARE);
    buffer = df->format(value, buffer, pos);
    if(buffer != expected){
        errln((UnicodeString)"ERROR: format failed after setSymbols()\n Expected " +
            expected + ", Got " + buffer);
    }
    delete df;
}

#endif /* #if !UCONFIG_NO_FORMATTING */
