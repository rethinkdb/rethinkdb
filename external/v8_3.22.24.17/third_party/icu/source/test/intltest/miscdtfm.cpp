/***********************************************************************
 * Copyright (c) 1997-2010, International Business Machines Corporation
 * and others. All Rights Reserved.
 ***********************************************************************/
 
#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "miscdtfm.h"

#include "unicode/format.h"
#include "unicode/decimfmt.h"
#include "unicode/datefmt.h"
#include "unicode/smpdtfmt.h"
#include "unicode/dtfmtsym.h"
#include "unicode/locid.h"
#include "unicode/msgfmt.h"
#include "unicode/numfmt.h"
#include "unicode/choicfmt.h"
#include "unicode/gregocal.h"

// *****************************************************************************
// class DateFormatMiscTests
// *****************************************************************************

#define CASE(id,test) case id: name = #test; if (exec) { logln(#test "---"); logln((UnicodeString)""); test(); } break;

void 
DateFormatMiscTests::runIndexedTest( int32_t index, UBool exec, const char* &name, char* /*par*/ )
{
    // if (exec) logln((UnicodeString)"TestSuite DateFormatMiscTests");
    switch (index) {
        CASE(0, test4097450)
        CASE(1, test4099975)
        CASE(2, test4117335)

        default: name = ""; break;
    }
}

UBool 
DateFormatMiscTests::failure(UErrorCode status, const char* msg)
{
    if(U_FAILURE(status)) {
        errcheckln(status, UnicodeString("FAIL: ") + msg + " failed, error " + u_errorName(status));
        return TRUE;
    }

    return FALSE;
}

/*
 * @bug 4097450
 */
void
DateFormatMiscTests::test4097450()
{
    //
    // Date parse requiring 4 digit year.
    //
    UnicodeString  dstring [] = {
        UnicodeString("97"),
        UnicodeString("1997"),  
        UnicodeString("97"),
        UnicodeString("1997"),
        UnicodeString("01"),
        UnicodeString("2001"),  
        UnicodeString("01"),
        UnicodeString("2001"),  
        UnicodeString("1"),
        UnicodeString("1"),
        UnicodeString("11"),  
        UnicodeString("11"),
        UnicodeString("111"), 
        UnicodeString("111")
    };
    
    UnicodeString dformat [] = {
        UnicodeString("yy"),  
        UnicodeString("yy"),
        UnicodeString("yyyy"),
        UnicodeString("yyyy"),
        UnicodeString("yy"),  
        UnicodeString("yy"),
        UnicodeString("yyyy"),
        UnicodeString("yyyy"),
        UnicodeString("yy"),
        UnicodeString("yyyy"),
        UnicodeString("yy"),
        UnicodeString("yyyy"), 
        UnicodeString("yy"),
        UnicodeString("yyyy")
    };
    
/*    UBool dresult [] = {
        TRUE, 
        FALSE, 
        FALSE,  
        TRUE,
        TRUE, 
        FALSE, 
        FALSE,  
        TRUE,
        FALSE,
        FALSE,
        TRUE, 
        FALSE,
        FALSE, 
        FALSE
    };*/

    UErrorCode status = U_ZERO_ERROR;
    SimpleDateFormat *formatter;
    SimpleDateFormat *resultFormatter = new SimpleDateFormat((UnicodeString)"yyyy", status);
    if (U_FAILURE(status)) {
        dataerrln("Fail new SimpleDateFormat: %s", u_errorName(status));
        return;
    }

    logln("Format\tSource\tResult");
    logln("-------\t-------\t-------");
    for (int i = 0; i < 14/*dstring.length*/; i++)
    {
        log(dformat[i] + "\t" + dstring[i] + "\t");
        formatter = new SimpleDateFormat(dformat[i], status);
        if(failure(status, "new SimpleDateFormat")) return;
        //try {
        UnicodeString str;
        FieldPosition pos(FieldPosition::DONT_CARE);
        logln(resultFormatter->format(formatter->parse(dstring[i], status), str, pos));
        failure(status, "resultFormatter->format");
            //if ( !dresult[i] ) System.out.print("   <-- error!");
        /*}
        catch (ParseException exception) {
            //if ( dresult[i] ) System.out.print("   <-- error!");
            System.out.print("exception --> " + exception);
        }*/
        delete formatter;
        logln();
    }

    delete resultFormatter;
}

/*
 * @bug 4099975
 */
void
DateFormatMiscTests::test4099975()
{
    /** 
     * Test Constructor SimpleDateFormat::SimpleDateFormat (const UnicodeString & pattern, 
     *                                    const DateFormatSymbols & formatData, UErrorCode & status )
     * The DateFormatSymbols object is NOT adopted; Modifying the original DateFormatSymbols
     * should not change the SimpleDateFormat's behavior.
     */
    UDate d = Calendar::getNow();
    {
        UErrorCode status = U_ZERO_ERROR;
        DateFormatSymbols* symbols = new DateFormatSymbols(Locale::getUS(), status);
        if(failure(status, "new DateFormatSymbols")) return;
        SimpleDateFormat *df = new SimpleDateFormat(UnicodeString("E hh:mm"), *symbols, status);
        if(failure(status, "new SimpleDateFormat")) return;
        UnicodeString format0;
        format0 = df->format(d, format0);
        UnicodeString localizedPattern0;
        localizedPattern0 = df->toLocalizedPattern(localizedPattern0, status);
        failure(status, "df->toLocalizedPattern");
        symbols->setLocalPatternChars(UnicodeString("abcdefghijklmonpqr")); // change value of field
        UnicodeString format1;
        format1 = df->format(d, format1);
        if (format0 != format1) {
            errln(UnicodeString("Formats are different. format0: ") + format0 
                + UnicodeString("; format1: ") + format1);
        }
        UnicodeString localizedPattern1;
        localizedPattern1 = df->toLocalizedPattern(localizedPattern1, status);
        failure(status, "df->toLocalizedPattern");
        if (localizedPattern0 != localizedPattern1) {
            errln(UnicodeString("Pattern has been changed. localizedPattern0: ") + localizedPattern0 
                + UnicodeString("; localizedPattern1: ") + localizedPattern1);
        }
        delete symbols;
        delete df;
    }
    /*
     * Test void SimpleDateFormat::setDateFormatSymbols (  const DateFormatSymbols & newFormatSymbols ) 
     * Modifying the original DateFormatSymbols should not change the SimpleDateFormat's behavior.
     */
    {
        UErrorCode status = U_ZERO_ERROR;
        DateFormatSymbols* symbols = new DateFormatSymbols(Locale::getUS(), status);
        if(failure(status, "new DateFormatSymbols")) return;
        SimpleDateFormat *df = new SimpleDateFormat(UnicodeString("E hh:mm"), status);
        if(failure(status, "new SimpleDateFormat")) return;
        df->setDateFormatSymbols(*symbols);
        UnicodeString format0;
        format0 = df->format(d, format0);
        UnicodeString localizedPattern0;
        localizedPattern0 = df->toLocalizedPattern(localizedPattern0, status);
        failure(status, "df->toLocalizedPattern");
        symbols->setLocalPatternChars(UnicodeString("abcdefghijklmonpqr")); // change value of field
        UnicodeString format1;
        format1 = df->format(d, format1);
        if (format0 != format1) {
            errln(UnicodeString("Formats are different. format0: ") + format0 
                + UnicodeString("; format1: ") + format1);
        }
        UnicodeString localizedPattern1;
        localizedPattern1 = df->toLocalizedPattern(localizedPattern1, status);
        failure(status, "df->toLocalizedPattern");
        if (localizedPattern0 != localizedPattern1) {
            errln(UnicodeString("Pattern has been changed. localizedPattern0: ") + localizedPattern0 
                + UnicodeString("; localizedPattern1: ") + localizedPattern1);
        }
        delete symbols;
        delete df;

    }
    //Test the pointer version of the constructor (and the adoptDateFormatSymbols method)
    {
        UErrorCode status = U_ZERO_ERROR;
        DateFormatSymbols* symbols = new DateFormatSymbols(Locale::getUS(), status);
        if(failure(status, "new DateFormatSymbols")) return;
        SimpleDateFormat *df = new SimpleDateFormat(UnicodeString("E hh:mm"), symbols, status);
        if(failure(status, "new SimpleDateFormat")) return;
        UnicodeString format0;
        format0 = df->format(d, format0);
        UnicodeString localizedPattern0;
        localizedPattern0 = df->toLocalizedPattern(localizedPattern0, status);
        failure(status, "df->toLocalizedPattern");
        symbols->setLocalPatternChars(UnicodeString("abcdefghijklmonpqr")); // change value of field
        UnicodeString format1;
        format1 = df->format(d, format1);
        if (format0 != format1) {
            errln(UnicodeString("Formats are different. format0: ") + format0 
                + UnicodeString("; format1: ") + format1);
        }
        UnicodeString localizedPattern1;
        localizedPattern1 = df->toLocalizedPattern(localizedPattern1, status);
        failure(status, "df->toLocalizedPattern");
        if (localizedPattern0 == localizedPattern1) {
            errln(UnicodeString("Pattern should have been changed. localizedPattern0: ") + localizedPattern0 
                + UnicodeString("; localizedPattern1: ") + localizedPattern1);
        }
        //delete symbols; the caller is no longer responsible for deleting the symbols
        delete df;
    }
    //
    {
        UErrorCode status = U_ZERO_ERROR;
        DateFormatSymbols* symbols = new DateFormatSymbols(Locale::getUS(), status);
        failure(status, "new DateFormatSymbols");
        SimpleDateFormat *df = new SimpleDateFormat(UnicodeString("E hh:mm"), status);
        if(failure(status, "new SimpleDateFormat")) return;
        df-> adoptDateFormatSymbols(symbols);
        UnicodeString format0;
        format0 = df->format(d, format0);
        UnicodeString localizedPattern0;
        localizedPattern0 = df->toLocalizedPattern(localizedPattern0, status);
        failure(status, "df->toLocalizedPattern");
        symbols->setLocalPatternChars(UnicodeString("abcdefghijklmonpqr")); // change value of field
        UnicodeString format1;
        format1 = df->format(d, format1);
        if (format0 != format1) {
            errln(UnicodeString("Formats are different. format0: ") + format0 
                + UnicodeString("; format1: ") + format1);
        }
        UnicodeString localizedPattern1;
        localizedPattern1 = df->toLocalizedPattern(localizedPattern1, status);
        failure(status, "df->toLocalizedPattern");
        if (localizedPattern0 == localizedPattern1) {
            errln(UnicodeString("Pattern should have been changed. localizedPattern0: ") + localizedPattern0 
                + UnicodeString("; localizedPattern1: ") + localizedPattern1);
        }
        //delete symbols; the caller is no longer responsible for deleting the symbols
        delete df;
    }
}

/*
 * @test @(#)bug4117335.java    1.1 3/5/98
 *
 * @bug 4117335
 */
void
DateFormatMiscTests::test4117335()
{
    //UnicodeString bc = "\u7d00\u5143\u524d";
    UChar bcC [] = {
        0x7D00,
        0x5143,
        0x524D
    };
    UnicodeString bc(bcC, 3, 3);

    //UnicodeString ad = "\u897f\u66a6";
    UChar adC [] = {
        0x897F,
        0x66A6
    };
    UnicodeString ad(adC, 2, 2);
    
    //UnicodeString jstLong = "\u65e5\u672c\u6a19\u6e96\u6642";
    UChar jstLongC [] = {
        0x65e5,
        0x672c,
        0x6a19,
        0x6e96,
        0x6642
    };
    UChar jdtLongC [] = {0x65E5, 0x672C, 0x590F, 0x6642, 0x9593};

    UnicodeString jstLong(jstLongC, 5, 5);

    UnicodeString jstShort = "JST";
    
    UnicodeString tzID = "Asia/Tokyo";

    UnicodeString jdtLong(jdtLongC, 5, 5);
 
    UnicodeString jdtShort = "JDT";
    UErrorCode status = U_ZERO_ERROR;
    DateFormatSymbols *symbols = new DateFormatSymbols(Locale::getJapan(), status);
    if(U_FAILURE(status)) {
      errcheckln(status, "Failure creating DateFormatSymbols, %s", u_errorName(status));
      delete symbols;
      return;
    }
    failure(status, "new DateFormatSymbols");
    int32_t eraCount = 0;
    const UnicodeString *eras = symbols->getEraNames(eraCount);
    
    logln(UnicodeString("BC = ") + eras[0]);
    if (eras[0] != bc) {
        errln("*** Should have been " + bc);
        //throw new Exception("Error in BC");
    }

    logln(UnicodeString("AD = ") + eras[1]);
    if (eras[1] != ad) {
        errln("*** Should have been " + ad);
        //throw new Exception("Error in AD");
    }

    int32_t rowCount, colCount;
    const UnicodeString **zones = symbols->getZoneStrings(rowCount, colCount);
    //don't hard code the index .. compute it.
    int32_t index = -1;
    for (int32_t i = 0; i < rowCount; ++i) {
        if (tzID == (zones[i][0])) {
            index = i;
            break;
        }
    }
    logln(UnicodeString("Long zone name = ") + zones[index][1]);
    if (zones[index][1] != jstLong) {
        errln("*** Should have been " + prettify(jstLong)+ " but it is: " + prettify(zones[index][1]));
        //throw new Exception("Error in long TZ name");
    }
    logln(UnicodeString("Short zone name = ") + zones[index][2]);
    if (zones[index][2] != jstShort) {
        errln("*** Should have been " + prettify(jstShort) + " but it is: " + prettify(zones[index][2]));
        //throw new Exception("Error in short TZ name");
    }
    logln(UnicodeString("Long zone name = ") + zones[index][3]);
    if (zones[index][3] != jdtLong) {
        errln("*** Should have been " + prettify(jstLong) + " but it is: " + prettify(zones[index][3]));
        //throw new Exception("Error in long TZ name");
    }
    logln(UnicodeString("SHORT zone name = ") + zones[index][4]);
    if (zones[index][4] != jdtShort) {
        errln("*** Should have been " + prettify(jstShort)+ " but it is: " + prettify(zones[index][4]));
        //throw new Exception("Error in short TZ name");
    }
    delete symbols;

}

#endif /* #if !UCONFIG_NO_FORMATTING */
