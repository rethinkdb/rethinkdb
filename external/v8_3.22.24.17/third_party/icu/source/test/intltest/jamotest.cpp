/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2010, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************
************************************************************************
*   Date        Name        Description
*   02/28/2001  aliu        Creation
*   03/01/2001  George      port to HP/UX
************************************************************************/

#include "unicode/utypes.h"

#if !UCONFIG_NO_TRANSLITERATION

#include "jamotest.h"
#include "unicode/utypes.h"
#include "unicode/translit.h"
#include "cpdtrans.h"

// SEP is the disambiguation separator used by Latin-Jamo and Jamo-Latin
#define SEP "-"

JamoTest::JamoTest()
{
    UParseError parseError;
    UErrorCode status = U_ZERO_ERROR;
    NAME_JAMO = Transliterator::createFromRules("Name-Jamo",
                                            UnicodeString(JAMO_NAMES_RULES, -1, US_INV),
                                            UTRANS_FORWARD, parseError, status);

    if (U_FAILURE(status)) {
        delete NAME_JAMO;
        NAME_JAMO = NULL;
    }
    status = U_ZERO_ERROR;
    JAMO_NAME = Transliterator::createFromRules("Jamo-Name",
                                            UnicodeString(JAMO_NAMES_RULES, -1, US_INV),
                                            UTRANS_REVERSE, parseError, status);
    if (U_FAILURE(status)) {
        delete JAMO_NAME;
        JAMO_NAME = NULL;
    }
}

JamoTest::~JamoTest()
{
    delete NAME_JAMO;
    delete JAMO_NAME;
}

void
JamoTest::runIndexedTest(int32_t index, UBool exec,
                         const char* &name, char* /*par*/) {
    switch (index) {
        TESTCASE(0,TestJamo);
        TESTCASE(1,TestRealText);
        TESTCASE(2,TestPiecemeal);
        default: name = ""; break;
    }
}

void
JamoTest::TestJamo() {
    UParseError parseError;
    UErrorCode status = U_ZERO_ERROR;
    Transliterator* latinJamo = Transliterator::createInstance("Latin-Jamo", UTRANS_FORWARD, parseError, status);

    if (latinJamo == 0 || U_FAILURE(status)) {
        dataerrln("FAIL: createInstance() returned 0 - %s", u_errorName(status));
        return;
    }

    Transliterator* jamoLatin = latinJamo->createInverse(status);

    if (jamoLatin == 0) {
        delete latinJamo;
        errln("FAIL: createInverse() returned 0");
        return;
    }

    static const char* CASE[] = {
        // Column 1 is the latin text L1 to be fed to Latin-Jamo
        // to yield output J.
        
        // Column 2 is expected value of J.  J is fed to
        // Jamo-Latin to yield output L2.
        
        // Column 3 is expected value of L2.  If the expected
        // value of L2 is L1, then L2 is NULL.

                // add tests for the update to fix problems where it didn't follow the standard
                // see also http://www.unicode.org/cldr/data/charts/transforms/Latin-Hangul.html
                "gach", "(Gi)(A)(Cf)", NULL,
                "geumhui", "(Gi)(EU)(Mf)(Hi)(YI)", NULL,
                "choe", "(Ci)(OE)", NULL,
                "wo", "(IEUNG)(WEO)", NULL,
                "Wonpil", "(IEUNG)(WEO)(Nf)(Pi)(I)(L)", "wonpil",
                "GIPPEUM", "(Gi)(I)(BB)(EU)(Mf)", "gippeum",
                "EUTTEUM", "(IEUNG)(EU)(DD)(EU)(Mf)", "eutteum",
                "KKOTNAE", "(GGi)(O)(Tf)(Ni)(AE)", "kkotnae",
                "gaga", "(Gi)(A)(Gi)(A)", NULL,
                "gag-a", "(Gi)(A)(Gf)(IEUNG)(A)", NULL,
                "gak-ka", "(Gi)(A)(Kf)(Ki)(A)", NULL,
                "gakka", "(Gi)(A)(GGi)(A)", NULL,
                "gakk-a", "(Gi)(A)(GGf)(IEUNG)(A)", NULL,
                "gakkka", "(Gi)(A)(GGf)(Ki)(A)", NULL,
                "gak-kka", "(Gi)(A)(Kf)(GGi)(A)", NULL,

        "bab", "(Bi)(A)(Bf)", NULL,
        "babb", "(Bi)(A)(Bf)(Bi)(EU)", "babbeu",
        "babbba", "(Bi)(A)(Bf)(Bi)(EU)(Bi)(A)", "babbeuba",
        "bagg", "(Bi)(A)(Gf)(Gi)(EU)", "baggeu",
        "baggga", "(Bi)(A)(Gf)(Gi)(EU)(Gi)(A)", "baggeuga",
        //"bag" SEP "gga", "(Bi)(A)(Gf)" SEP "(Gi)(EU)(Gi)(A)", "bag" SEP "geuga",
        "kabsa", "(Ki)(A)(Bf)(Si)(A)", NULL,
        "kabska", "(Ki)(A)(BS)(Ki)(A)", NULL,
        "gabsbka", "(Gi)(A)(BS)(Bi)(EU)(Ki)(A)", "gabsbeuka", // not (Kf)
        "gga", "(Gi)(EU)(Gi)(A)", "geuga",
        "bsa", "(Bi)(EU)(Si)(A)", "beusa",
        "agg", "(IEUNG)(A)(Gf)(Gi)(EU)", "aggeu",
        "agga", "(IEUNG)(A)(Gf)(Gi)(A)", NULL,
        "la", "(R)(A)", NULL,
        "bs", "(Bi)(EU)(Sf)", "beus",
        "kalgga", "(Ki)(A)(L)(Gi)(EU)(Gi)(A)", "kalgeuga",
        
        // 'r' in a final position is treated like 'l'
        "karka", "(Ki)(A)(L)(Ki)(A)", "kalka",
    };

    enum { CASE_length = sizeof(CASE) / sizeof(CASE[0]) };
    
    int32_t i;
    for (i=0; i<CASE_length; i+=3) {
        UnicodeString jamo = nameToJamo(CASE[i+1]);
        if (CASE[i+2] == NULL) {
            expect(*latinJamo, CASE[i], jamo, *jamoLatin);
        } else {
            // Handle case where round-trip is expected to fail
            expect(*latinJamo, CASE[i], jamo);
            expect(*jamoLatin, jamo, CASE[i+2]);
        }
    }

    delete latinJamo;
    delete jamoLatin;
}

/**
 * Test various step-at-a-time transformation of hangul to jamo to
 * latin and back.
 */
void JamoTest::TestPiecemeal(void) {
    UnicodeString hangul; hangul.append((UChar)0xBC0F);
    UnicodeString jamo = nameToJamo("(Mi)(I)(Cf)");
    UnicodeString latin("mic");
    UnicodeString latin2("mich");

    Transliterator *t = NULL;
    UErrorCode status = U_ZERO_ERROR;

    t = Transliterator::createInstance("NFD", UTRANS_FORWARD, status); // was Hangul-Jamo
    if (U_FAILURE(status) || t == 0) {
        dataerrln("FAIL: createInstance failed");
        return;
    }
    expect(*t, hangul, jamo);
    delete t;

    t = Transliterator::createInstance("NFC", UTRANS_FORWARD, status); // was Jamo-Hangul
    if (U_FAILURE(status) || t == 0) {
        errln("FAIL: createInstance failed");
        return;
    }
    expect(*t, jamo, hangul);
    delete t;

    t = Transliterator::createInstance("Latin-Jamo", UTRANS_FORWARD, status);
    if (U_FAILURE(status) || t == 0) {
        dataerrln("FAIL: createInstance failed - %s", u_errorName(status));
        return;
    }
    expect(*t, latin, jamo);
    delete t;

    t = Transliterator::createInstance("Jamo-Latin", UTRANS_FORWARD, status);
    if (U_FAILURE(status) || t == 0) {
        errln("FAIL: createInstance failed");
        return;
    }
    expect(*t, jamo, latin2);
    delete t;

    t = Transliterator::createInstance("Hangul-Latin", UTRANS_FORWARD, status);
    if (U_FAILURE(status) || t == 0) {
        errln("FAIL: createInstance failed");
        return;
    }
    expect(*t, hangul, latin2);
    delete t;

    t = Transliterator::createInstance("Latin-Hangul", UTRANS_FORWARD, status);
    if (U_FAILURE(status) || t == 0) {
        errln("FAIL: createInstance failed");
        return;
    }
    expect(*t, latin, hangul);
    delete t;

    t = Transliterator::createInstance("Hangul-Latin; Latin-Jamo", UTRANS_FORWARD, status);
    if (U_FAILURE(status) || t == 0) {
        errln("FAIL: createInstance failed");
        return;
    }
    expect(*t, hangul, jamo);
    delete t;

    t = Transliterator::createInstance("Jamo-Latin; Latin-Hangul", UTRANS_FORWARD, status);
    if (U_FAILURE(status) || t == 0) {
        errln("FAIL: createInstance failed");
        return;
    }
    expect(*t, jamo, hangul);
    delete t;

    t = Transliterator::createInstance("Hangul-Latin; Latin-Hangul", UTRANS_FORWARD, status);
    if (U_FAILURE(status) || t == 0) {
        errln("FAIL: createInstance failed");
        return;
    }
    expect(*t, hangul, hangul);
    delete t;
}

void
JamoTest::TestRealText() {
    // Test text taken from the Unicode web site
     static const char* const WHAT_IS_UNICODE[] = {
      "\\uc720\\ub2c8\\ucf54\\ub4dc\\uc5d0", "\\ub300\\ud574", "?",

      "\\uc5b4\\ub5a4", "\\ud50c\\ub7ab\\ud3fc,", "\\uc5b4\\ub5a4",
      "\\ud504\\ub85c\\uadf8\\ub7a8,", "\\uc5b4\\ub5a4", "\\uc5b8\\uc5b4\\uc5d0\\ub3c4",
      "\\uc0c1\\uad00\\uc5c6\\uc774", "\\uc720\\ub2c8\\ucf54\\ub4dc\\ub294", "\\ubaa8\\ub4e0",
      "\\ubb38\\uc790\\uc5d0", "\\ub300\\ud574", "\\uace0\\uc720", "\\ubc88\\ud638\\ub97c",
      "\\uc81c\\uacf5\\ud569\\ub2c8\\ub2e4.",

      "\\uae30\\ubcf8\\uc801\\uc73c\\ub85c", "\\ucef4\\ud4e8\\ud130\\ub294",
      "\\uc22b\\uc790\\ub9cc", "\\ucc98\\ub9ac\\ud569\\ub2c8\\ub2e4.", "\\uae00\\uc790\\ub098",
      "\\ub2e4\\ub978", "\\ubb38\\uc790\\uc5d0\\ub3c4", "\\uc22b\\uc790\\ub97c",
      "\\uc9c0\\uc815\\ud558\\uc5ec",
      "\\uc800\\uc7a5\\ud569\\ub2c8\\ub2e4.", "\\uc720\\ub2c8\\ucf54\\ub4dc\\uac00",
      "\\uac1c\\ubc1c\\ub418\\uae30", "\\uc804\\uc5d0\\ub294", "\\uc774\\ub7ec\\ud55c",
      "\\uc22b\\uc790\\ub97c", "\\uc9c0\\uc815\\ud558\\uae30", "\\uc704\\ud574", "\\uc218\\ubc31",
      "\\uac00\\uc9c0\\uc758", "\\ub2e4\\ub978", "\\uae30\\ud638\\ud654",
      "\\uc2dc\\uc2a4\\ud15c\\uc744",
      "\\uc0ac\\uc6a9\\ud588\\uc2b5\\ub2c8\\ub2e4.", "\\ub2e8\\uc77c", "\\uae30\\ud638\\ud654",
      "\\ubc29\\ubc95\\uc73c\\ub85c\\ub294", "\\ubaa8\\ub4e0", "\\ubb38\\uc790\\ub97c",
      "\\ud3ec\\ud568\\ud560", "\\uc218", "\\uc5c6\\uc5c8\\uc2b5\\ub2c8\\ub2e4.", "\\uc608\\ub97c",
      "\\ub4e4\\uc5b4", "\\uc720\\ub7fd", "\\uc5f0\\ud569\\uc5d0\\uc11c\\ub9cc",
      "\\ubcf4\\ub354\\ub77c\\ub3c4", "\\ubaa8\\ub4e0", "\\uac01", "\\ub098\\ub77c\\ubcc4",
      "\\uc5b8\\uc5b4\\ub97c", "\\ucc98\\ub9ac\\ud558\\ub824\\uba74", "\\uc5ec\\ub7ec",
      "\\uac1c\\uc758", "\\ub2e4\\ub978", "\\uae30\\ud638\\ud654", "\\ubc29\\ubc95\\uc774",
      "\\ud544\\uc694\\ud569\\ub2c8\\ub2e4.", "\\uc601\\uc5b4\\uc640", "\\uac19\\uc740",
      "\\ub2e8\\uc77c", "\\uc5b8\\uc5b4\\uc758", "\\uacbd\\uc6b0\\ub3c4",
      "\\uacf5\\ud1b5\\uc801\\uc73c\\ub85c", "\\uc0ac\\uc6a9\\ub418\\ub294", "\\ubaa8\\ub4e0",
      "\\uae00\\uc790,", "\\ubb38\\uc7a5", "\\ubd80\\ud638", "\\ubc0f",
      "\\ud14c\\ud06c\\ub2c8\\uceec", "\\uae30\\ud638\\uc5d0", "\\ub9de\\ub294", "\\ub2e8\\uc77c",
      "\\uae30\\ud638\\ud654", "\\ubc29\\ubc95\\uc744", "\\uac16\\uace0", "\\uc788\\uc9c0",
      "\\ubabb\\ud558\\uc600\\uc2b5\\ub2c8\\ub2e4.",

      "\\uc774\\ub7ec\\ud55c", "\\uae30\\ud638\\ud654", "\\uc2dc\\uc2a4\\ud15c\\uc740",
      "\\ub610\\ud55c", "\\ub2e4\\ub978", "\\uae30\\ud638\\ud654", "\\uc2dc\\uc2a4\\ud15c\\uacfc",
      "\\ucda9\\ub3cc\\ud569\\ub2c8\\ub2e4.", "\\uc989", "\\ub450", "\\uac00\\uc9c0",
      "\\uae30\\ud638\\ud654", "\\ubc29\\ubc95\\uc774", "\\ub450", "\\uac1c\\uc758", "\\ub2e4\\ub978",
      "\\ubb38\\uc790\\uc5d0", "\\ub300\\ud574", "\\uac19\\uc740", "\\ubc88\\ud638\\ub97c",
      "\\uc0ac\\uc6a9\\ud558\\uac70\\ub098", "\\uac19\\uc740", "\\ubb38\\uc790\\uc5d0",
      "\\ub300\\ud574", "\\ub2e4\\ub978", "\\ubc88\\ud638\\ub97c", "\\uc0ac\\uc6a9\\ud560", "\\uc218",
      "\\uc788\\uc2b5\\ub2c8\\ub2e4.", "\\uc8fc\\uc5b4\\uc9c4", "\\ubaa8\\ub4e0",
      "\\ucef4\\ud4e8\\ud130(\\ud2b9\\ud788", "\\uc11c\\ubc84)\\ub294", "\\uc11c\\ub85c",
      "\\ub2e4\\ub978", "\\uc5ec\\ub7ec", "\\uac00\\uc9c0", "\\uae30\\ud638\\ud654",
      "\\ubc29\\ubc95\\uc744", "\\uc9c0\\uc6d0\\ud574\\uc57c",
      "\\ud569\\ub2c8\\ub2e4.", "\\uadf8\\ub7ec\\ub098,", "\\ub370\\uc774\\ud130\\ub97c",
      "\\uc11c\\ub85c", "\\ub2e4\\ub978", "\\uae30\\ud638\\ud654", "\\ubc29\\ubc95\\uc774\\ub098",
      "\\ud50c\\ub7ab\\ud3fc", "\\uac04\\uc5d0", "\\uc804\\ub2ec\\ud560", "\\ub54c\\ub9c8\\ub2e4",
      "\\uadf8", "\\ub370\\uc774\\ud130\\ub294", "\\ud56d\\uc0c1", "\\uc190\\uc0c1\\uc758",
      "\\uc704\\ud5d8\\uc744", "\\uacaa\\uac8c", "\\ub429\\ub2c8\\ub2e4.",

      "\\uc720\\ub2c8\\ucf54\\ub4dc\\ub85c", "\\ubaa8\\ub4e0", "\\uac83\\uc744",
      "\\ud574\\uacb0\\ud560", "\\uc218", "\\uc788\\uc2b5\\ub2c8\\ub2e4!",
      "\\uc720\\ub2c8\\ucf54\\ub4dc\\ub294", "\\uc0ac\\uc6a9", "\\uc911\\uc778",
      "\\ud50c\\ub7ab\\ud3fc,", "\\ud504\\ub85c\\uadf8\\ub7a8,", "\\uc5b8\\uc5b4\\uc5d0",
      "\\uad00\\uacc4\\uc5c6\\uc774", "\\ubb38\\uc790\\ub9c8\\ub2e4", "\\uace0\\uc720\\ud55c",
      "\\uc22b\\uc790\\ub97c",
      "\\uc81c\\uacf5\\ud569\\ub2c8\\ub2e4.", "\\uc720\\ub2c8\\ucf54\\ub4dc",
      "\\ud45c\\uc900\\uc740", // "Apple, HP, IBM, JustSystem, Microsoft, Oracle, SAP, "
      // "Sun, Sybase, Unisys "
      "\\ubc0f", "\\uae30\\ud0c0", "\\uc5ec\\ub7ec",
      "\\ud68c\\uc0ac\\uc640", "\\uac19\\uc740", "\\uc5c5\\uacc4",
      "\\uc120\\ub450\\uc8fc\\uc790\\uc5d0", "\\uc758\\ud574",
      "\\ucc44\\ud0dd\\ub418\\uc5c8\\uc2b5\\ub2c8\\ub2e4.", "\\uc720\\ub2c8\\ucf54\\ub4dc\\ub294",
      // "XML, Java, ECMAScript(JavaScript), LDAP, CORBA 3.0, WML "
      "\\ub4f1\\uacfc",
      "\\uac19\\uc774", "\\ud604\\uc7ac", "\\ub110\\ub9ac", "\\uc0ac\\uc6a9\\ub418\\ub294",
      "\\ud45c\\uc900\\uc5d0\\uc11c", "\\ud544\\uc694\\ud558\\uba70", "\\uc774\\ub294", // "ISO/IEC",
      "10646\\uc744", "\\uad6c\\ud604\\ud558\\ub294", "\\uacf5\\uc2dd\\uc801\\uc778",
      "\\ubc29\\ubc95\\uc785\\ub2c8\\ub2e4.", "\\uc774\\ub294", "\\ub9ce\\uc740", "\\uc6b4\\uc601",
      "\\uccb4\\uc81c,", "\\uc694\\uc998", "\\uc0ac\\uc6a9\\ub418\\ub294", "\\ubaa8\\ub4e0",
      "\\ube0c\\ub77c\\uc6b0\\uc800", "\\ubc0f", "\\uae30\\ud0c0", "\\ub9ce\\uc740",
      "\\uc81c\\ud488\\uc5d0\\uc11c",
      "\\uc9c0\\uc6d0\\ub429\\ub2c8\\ub2e4.", "\\uc720\\ub2c8\\ucf54\\ub4dc",
      "\\ud45c\\uc900\\uc758", "\\ubd80\\uc0c1\\uacfc", "\\uc774\\ub97c",
      "\\uc9c0\\uc6d0\\ud558\\ub294", "\\ub3c4\\uad6c\\uc758", "\\uac00\\uc6a9\\uc131\\uc740",
      "\\ucd5c\\uadfc", "\\uc804", "\\uc138\\uacc4\\uc5d0", "\\ubd88\\uace0", "\\uc788\\ub294",
      "\\uae30\\uc220", "\\uacbd\\ud5a5\\uc5d0\\uc11c", "\\uac00\\uc7a5", "\\uc911\\uc694\\ud55c",
      "\\ubd80\\ubd84\\uc744", "\\ucc28\\uc9c0\\ud558\\uace0", "\\uc788\\uc2b5\\ub2c8\\ub2e4.",

      "\\uc720\\ub2c8\\ucf54\\ub4dc\\ub97c",
      // Replaced a hyphen with a space to make the test case work with CLDR1.5  
      //"\\ud074\\ub77c\\uc774\\uc5b8\\ud2b8-\\uc11c\\ubc84", "\\ub610\\ub294",
      "\\ud074\\ub77c\\uc774\\uc5b8\\ud2b8 \\uc11c\\ubc84", "\\ub610\\ub294",
      // Replaced a hyphen with a space.
      //"\\ub2e4\\uc911-\\uc5f0\\uacb0", "\\uc751\\uc6a9", "\\ud504\\ub85c\\uadf8\\ub7a8\\uacfc",
      "\\ub2e4\\uc911 \\uc5f0\\uacb0", "\\uc751\\uc6a9", "\\ud504\\ub85c\\uadf8\\ub7a8\\uacfc",
      "\\uc6f9", "\\uc0ac\\uc774\\ud2b8\\uc5d0", "\\ud1b5\\ud569\\ud558\\uba74",
      "\\ub808\\uac70\\uc2dc", "\\ubb38\\uc790", "\\uc138\\ud2b8", "\\uc0ac\\uc6a9\\uc5d0",
      "\\uc788\\uc5b4\\uc11c", "\\uc0c1\\ub2f9\\ud55c", "\\ube44\\uc6a9", "\\uc808\\uac10",
      "\\ud6a8\\uacfc\\uac00",
      "\\ub098\\ud0c0\\ub0a9\\ub2c8\\ub2e4.", "\\uc720\\ub2c8\\ucf54\\ub4dc\\ub97c",
      "\\ud1b5\\ud574", "\\ub9ac\\uc5d4\\uc9c0\\ub2c8\\uc5b4\\ub9c1", "\\uc5c6\\uc774",
      "\\ub2e4\\uc911", "\\ud50c\\ub7ab\\ud3fc,", "\\uc5b8\\uc5b4", "\\ubc0f", "\\uad6d\\uac00",
      "\\uac04\\uc5d0", "\\ub2e8\\uc77c", "\\uc18c\\ud504\\ud2b8\\uc6e8\\uc5b4",
      "\\ud50c\\ub7ab\\ud3fc", "\\ub610\\ub294", "\\ub2e8\\uc77c", "\\uc6f9",
      "\\uc0ac\\uc774\\ud2b8\\ub97c", "\\ubaa9\\ud45c\\ub85c", "\\uc0bc\\uc744", "\\uc218",
      "\\uc788\\uc2b5\\ub2c8\\ub2e4.", "\\uc774\\ub97c", "\\uc0ac\\uc6a9\\ud558\\uba74",
      "\\ub370\\uc774\\ud130\\ub97c", "\\uc190\\uc0c1", "\\uc5c6\\uc774", "\\uc5ec\\ub7ec",
      "\\uc2dc\\uc2a4\\ud15c\\uc744", "\\ud1b5\\ud574", "\\uc804\\uc1a1\\ud560", "\\uc218",
      "\\uc788\\uc2b5\\ub2c8\\ub2e4.",

      "\\uc720\\ub2c8\\ucf54\\ub4dc", "\\ucf58\\uc18c\\uc2dc\\uc5c4\\uc5d0", "\\ub300\\ud574",
      "\\uc720\\ub2c8\\ucf54\\ub4dc", "\\ucf58\\uc18c\\uc2dc\\uc5c4\\uc740",
      "\\ube44\\uc601\\ub9ac", "\\uc870\\uc9c1\\uc73c\\ub85c\\uc11c", "\\ud604\\ub300",
      "\\uc18c\\ud504\\ud2b8\\uc6e8\\uc5b4", "\\uc81c\\ud488\\uacfc",
      "\\ud45c\\uc900\\uc5d0\\uc11c", "\\ud14d\\uc2a4\\ud2b8\\uc758", "\\ud45c\\ud604\\uc744",
      "\\uc9c0\\uc815\\ud558\\ub294", "\\uc720\\ub2c8\\ucf54\\ub4dc", "\\ud45c\\uc900\\uc758",
      "\\uc0ac\\uc6a9\\uc744", "\\uac1c\\ubc1c\\ud558\\uace0", "\\ud655\\uc7a5\\ud558\\uba70",
      "\\uc7a5\\ub824\\ud558\\uae30", "\\uc704\\ud574",
      "\\uc138\\uc6cc\\uc84c\\uc2b5\\ub2c8\\ub2e4.", "\\ucf58\\uc18c\\uc2dc\\uc5c4",
      "\\uba64\\ubc84\\uc27d\\uc740", "\\ucef4\\ud4e8\\ud130\\uc640", "\\uc815\\ubcf4",
      "\\ucc98\\ub9ac", "\\uc0b0\\uc5c5\\uc5d0", "\\uc885\\uc0ac\\ud558\\uace0", "\\uc788\\ub294",
      "\\uad11\\ubc94\\uc704\\ud55c", "\\ud68c\\uc0ac", "\\ubc0f", "\\uc870\\uc9c1\\uc758",
      "\\ubc94\\uc704\\ub97c",
      "\\ub098\\ud0c0\\ub0c5\\ub2c8\\ub2e4.", "\\ucf58\\uc18c\\uc2dc\\uc5c4\\uc758",
      "\\uc7ac\\uc815\\uc740", "\\uc804\\uc801\\uc73c\\ub85c", "\\ud68c\\ube44\\uc5d0",
      "\\uc758\\ud574", "\\ucda9\\ub2f9\\ub429\\ub2c8\\ub2e4.", "\\uc720\\ub2c8\\ucf54\\ub4dc",
      "\\ucee8\\uc18c\\uc2dc\\uc5c4\\uc5d0\\uc11c\\uc758", "\\uba64\\ubc84\\uc27d\\uc740",
      "\\uc804", "\\uc138\\uacc4", "\\uc5b4\\ub290", "\\uacf3\\uc5d0\\uc11c\\ub098",
      "\\uc720\\ub2c8\\ucf54\\ub4dc", "\\ud45c\\uc900\\uc744", "\\uc9c0\\uc6d0\\ud558\\uace0",
      "\\uadf8", "\\ud655\\uc7a5\\uacfc", "\\uad6c\\ud604\\uc744",
      "\\uc9c0\\uc6d0\\ud558\\uace0\\uc790\\ud558\\ub294", "\\uc870\\uc9c1\\uacfc",
      "\\uac1c\\uc778\\uc5d0\\uac8c", "\\uac1c\\ubc29\\ub418\\uc5b4",
      "\\uc788\\uc2b5\\ub2c8\\ub2e4.",

      "\\ub354", "\\uc790\\uc138\\ud55c", "\\ub0b4\\uc6a9\\uc740", "\\uc6a9\\uc5b4\\uc9d1,",
      "\\uc608\\uc81c", "\\uc720\\ub2c8\\ucf54\\ub4dc", "\\uc0ac\\uc6a9", "\\uac00\\ub2a5",
      "\\uc81c\\ud488,", "\\uae30\\uc220", "\\uc815\\ubcf4", "\\ubc0f", "\\uae30\\ud0c0",
      "\\uc720\\uc6a9\\ud55c", "\\uc815\\ubcf4\\ub97c",
      "\\ucc38\\uc870\\ud558\\uc2ed\\uc2dc\\uc624."
    };

    enum { WHAT_IS_UNICODE_length = sizeof(WHAT_IS_UNICODE) / sizeof(WHAT_IS_UNICODE[0]) };

    UParseError parseError;
    UErrorCode status = U_ZERO_ERROR;
    Transliterator* latinJamo = Transliterator::createInstance("Latin-Jamo", UTRANS_FORWARD, parseError, status);
    Transliterator* jamoHangul = Transliterator::createInstance("NFC(NFD)", UTRANS_FORWARD, parseError, status);
    if (latinJamo == 0 || jamoHangul == 0 || U_FAILURE(status)) {
        delete latinJamo;
        delete jamoHangul;
        dataerrln("FAIL: createInstance returned NULL - %s", u_errorName(status));
        return;
    }
    Transliterator* jamoLatin = latinJamo->createInverse(status);
    Transliterator* hangulJamo = jamoHangul->createInverse(status);
    if (jamoLatin == 0 || hangulJamo == 0) {
        errln("FAIL: createInverse returned NULL");
        delete latinJamo;
        delete jamoLatin;
        delete jamoHangul;
        delete hangulJamo;
        return;
    }

    Transliterator* tarray[4] =
        { hangulJamo, jamoLatin, latinJamo, jamoHangul };
    CompoundTransliterator rt(tarray, 4);
    
    UnicodeString buf;
    int32_t total = 0;
    int32_t errors = 0;
    int32_t i;
    for (i=0; i < WHAT_IS_UNICODE_length; ++i) {
        ++total;
        UnicodeString hangul = UnicodeString(WHAT_IS_UNICODE[i], -1, US_INV);
        hangul = hangul.unescape(); // Parse backslash-u escapes
        UnicodeString hangulX = hangul;
        rt.transliterate(hangulX);
        if (hangul != hangulX) {
            ++errors;
            UnicodeString jamo = hangul; hangulJamo->transliterate(jamo);
            UnicodeString latin = jamo; jamoLatin->transliterate(latin);
            UnicodeString jamo2 = latin; latinJamo->transliterate(jamo2);
            UnicodeString hangul2 = jamo2; jamoHangul->transliterate(hangul2);
            
            buf.remove(0);
            buf.append("FAIL: ");
            if (hangul2 != hangulX) {
                buf.append((UnicodeString)"(Weird: " + hangulX + " != " + hangul2 + ")");
            }
            // The Hangul-Jamo conversion is not usually the
            // bug here, so we hide it from display.
            // Uncomment lines to see the Hangul.
            buf.append(//hangul + " => " +
                       jamoToName(jamo) + " => " +
                       latin + " => " + jamoToName(jamo2)
                       //+ " => " + hangul2
                       );
            errln(prettify(buf));
        }
    }
    if (errors != 0) {
        errln((UnicodeString)"Test word failures: " + errors + " out of " + total);
    } else {
        logln((UnicodeString)"All " + total + " test words passed");
    }

    delete latinJamo;
    delete jamoLatin;
    delete jamoHangul;
    delete hangulJamo;
}

// Override TransliteratorTest
void
JamoTest::expectAux(const UnicodeString& tag,
                    const UnicodeString& summary, UBool pass,
                    const UnicodeString& expectedResult) {
    UnicodeString jsum = jamoToName(summary);
    UnicodeString jexp = jamoToName(expectedResult);
    TransliteratorTest::expectAux(tag, jsum, pass, jexp);
}

const char* JamoTest::JAMO_NAMES_RULES =
        "'(Gi)' <> \\u1100;"
        "'(GGi)' <> \\u1101;"
        "'(Ni)' <> \\u1102;"
        "'(Di)' <> \\u1103;"
        "'(DD)' <> \\u1104;"
        "'(R)' <> \\u1105;"
        "'(Mi)' <> \\u1106;"
        "'(Bi)' <> \\u1107;"
        "'(BB)' <> \\u1108;"
        "'(Si)' <> \\u1109;"
        "'(SSi)' <> \\u110A;"
        "'(IEUNG)' <> \\u110B;"
        "'(Ji)' <> \\u110C;"
        "'(JJ)' <> \\u110D;"
        "'(Ci)' <> \\u110E;"
        "'(Ki)' <> \\u110F;"
        "'(Ti)' <> \\u1110;"
        "'(Pi)' <> \\u1111;"
        "'(Hi)' <> \\u1112;"
        
        "'(A)' <> \\u1161;"
        "'(AE)' <> \\u1162;"
        "'(YA)' <> \\u1163;"
        "'(YAE)' <> \\u1164;"
        "'(EO)' <> \\u1165;"
        "'(E)' <> \\u1166;"
        "'(YEO)' <> \\u1167;"
        "'(YE)' <> \\u1168;"
        "'(O)' <> \\u1169;"
        "'(WA)' <> \\u116A;"
        "'(WAE)' <> \\u116B;"
        "'(OE)' <> \\u116C;"
        "'(YO)' <> \\u116D;"
        "'(U)' <> \\u116E;"
        "'(WEO)' <> \\u116F;"
        "'(WE)' <> \\u1170;"
        "'(WI)' <> \\u1171;"
        "'(YU)' <> \\u1172;"
        "'(EU)' <> \\u1173;"
        "'(YI)' <> \\u1174;"
        "'(I)' <> \\u1175;"

        "'(Gf)' <> \\u11A8;"
        "'(GGf)' <> \\u11A9;"
        "'(GS)' <> \\u11AA;"
        "'(Nf)' <> \\u11AB;"
        "'(NJ)' <> \\u11AC;"
        "'(NH)' <> \\u11AD;"
        "'(Df)' <> \\u11AE;"
        "'(L)' <> \\u11AF;"
        "'(LG)' <> \\u11B0;"
        "'(LM)' <> \\u11B1;"
        "'(LB)' <> \\u11B2;"
        "'(LS)' <> \\u11B3;"
        "'(LT)' <> \\u11B4;"
        "'(LP)' <> \\u11B5;"
        "'(LH)' <> \\u11B6;"
        "'(Mf)' <> \\u11B7;"
        "'(Bf)' <> \\u11B8;"
        "'(BS)' <> \\u11B9;"
        "'(Sf)' <> \\u11BA;"
        "'(SSf)' <> \\u11BB;"
        "'(NG)' <> \\u11BC;"
        "'(Jf)' <> \\u11BD;"
        "'(Cf)' <> \\u11BE;"
        "'(Kf)' <> \\u11BF;"
        "'(Tf)' <> \\u11C0;"
        "'(Pf)' <> \\u11C1;"
        "'(Hf)' <> \\u11C2;";

/**
 * Convert short names to actual jamo.  E.g., "x(LG)y" returns
 * "x\u11B0y".  See JAMO_NAMES for table of names.
 */
UnicodeString
JamoTest::nameToJamo(const UnicodeString& input) { 
    if (NAME_JAMO == 0) {
        errln("Failed to create NAME_JAMO");
        return input;   /* failure! */
    }
    UnicodeString result(input);
    NAME_JAMO->transliterate(result);
    return result;
}

/**
 * Convert jamo to short names.  E.g., "x\u11B0y" returns
 * "x(LG)y".  See JAMO_NAMES for table of names.
 */
UnicodeString
JamoTest::jamoToName(const UnicodeString& input) {
    if (NAME_JAMO == 0) {
        errln("Failed to create NAME_JAMO");
        return input;   /* failure! */
    }
    UnicodeString result(input);
    JAMO_NAME->transliterate(result);
    return result;
}

#endif /* #if !UCONFIG_NO_TRANSLITERATION */
