/***************************************************************************
*
*   Copyright (C) 2000-2007, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
************************************************************************
*   Date        Name        Description
*   01/03/2000  Madhu        Creation.
*   03/2000     Madhu        Added additional tests
***********************************************************************/
/**
 * IntlTestTransliterator is the medium level test class for Transliterator 
 */

#include "unicode/utypes.h"

#if !UCONFIG_NO_TRANSLITERATION

#include "ittrans.h"
#include "transtst.h"
#include "transapi.h"
#include "cpdtrtst.h"
#include "transrt.h"
#include "jamotest.h"
#include "trnserr.h"
#include "reptest.h"

#define CASE(id,test) case id:                                \
                          name = #test;                       \
                          if (exec) {                         \
                              logln(#test "---"); logln();    \
                              test t;                         \
                              callTest(t, par);               \
                          }                                   \
                          break

void IntlTestTransliterator::runIndexedTest( int32_t index, UBool exec, const char* &name, char* par )
{
    if (exec) logln("TestSuite Transliterator");
    switch (index) {
        CASE(0, TransliteratorTest);
        CASE(1, TransliteratorAPITest);
        CASE(2, CompoundTransliteratorTest);
        CASE(3, TransliteratorRoundTripTest);
        CASE(4, JamoTest);
        CASE(5, TransliteratorErrorTest);
        CASE(6, ReplaceableTest);
#if !UCONFIG_NO_TRANSLITERATION && defined(U_USE_UNICODE_FILTER_LOGIC_OBSOLETE_2_8)
        CASE(7, UnicodeFilterLogicTest);
#endif

        default: name=""; break;
    }
}

#endif /* #if !UCONFIG_NO_TRANSLITERATION */
