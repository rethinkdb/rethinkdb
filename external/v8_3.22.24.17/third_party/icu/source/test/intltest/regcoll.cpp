/********************************************************************
 * COPYRIGHT:
 * Copyright (c) 1997-2010, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/coll.h"
#include "unicode/tblcoll.h"
#include "unicode/unistr.h"
#include "unicode/sortkey.h"
#include "regcoll.h"
#include "sfwdchit.h"
#include "testutil.h"
#include "cmemory.h"

#define ARRAY_LENGTH(array) ((int32_t)(sizeof array / sizeof array[0]))

CollationRegressionTest::CollationRegressionTest()
{
    UErrorCode status = U_ZERO_ERROR;

    en_us = (RuleBasedCollator *)Collator::createInstance(Locale::getUS(), status);
    if(U_FAILURE(status)) {
      delete en_us;
      en_us = 0;
      errcheckln(status, "Collator creation failed with %s", u_errorName(status));
      return;
    }
}

CollationRegressionTest::~CollationRegressionTest()
{
    delete en_us;
}


    // @bug 4048446
//
// CollationElementIterator.reset() doesn't work
//
void CollationRegressionTest::Test4048446(/* char* par */)
{
    const UnicodeString test1 = "XFILE What subset of all possible test cases has the highest probability of detecting the most errors?";
    const UnicodeString test2 = "Xf_ile What subset of all possible test cases has the lowest probability of detecting the least errors?";
    CollationElementIterator *i1 = en_us->createCollationElementIterator(test1);
    CollationElementIterator *i2 = en_us->createCollationElementIterator(test1);
    UErrorCode status = U_ZERO_ERROR;

    if (i1 == NULL|| i2 == NULL)
    {
        errln("Could not create CollationElementIterator's");
        delete i1;
        delete i2;
        return;
    }

    while (i1->next(status) != CollationElementIterator::NULLORDER)
    {
        if (U_FAILURE(status))
        {
            errln("error calling next()");

            delete i1;
            delete i2;
            return;
        }
    }

    i1->reset();

    assertEqual(*i1, *i2);

    delete i1;
    delete i2;
}

// @bug 4051866
//
// Collator -> rules -> Collator round-trip broken for expanding characters
//
void CollationRegressionTest::Test4051866(/* char* par */)
{
/*
    RuleBasedCollator c1 = new RuleBasedCollator("< o "
                                                +"& oe ,o\u3080"
                                                +"& oe ,\u1530 ,O"
                                                +"& OE ,O\u3080"
                                                +"& OE ,\u1520"
                                                +"< p ,P");
*/

    UnicodeString rules;
    UErrorCode status = U_ZERO_ERROR;

    rules += "< o ";
    rules += "& oe ,o";
    rules += (UChar)0x3080;
    rules += "& oe ,";
    rules += (UChar)0x1530;
    rules += " ,O";
    rules += "& OE ,O";
    rules += (UChar)0x3080;
    rules += "& OE ,";
    rules += (UChar)0x1520;
    rules += "< p ,P";

    // Build a collator containing expanding characters
    RuleBasedCollator *c1 = new RuleBasedCollator(rules, status);

    // Build another using the rules from  the first
    RuleBasedCollator *c2 = new RuleBasedCollator(c1->getRules(), status);

    // Make sure they're the same
    if (!(c1->getRules() == c2->getRules()))
    {
        errln("Rules are not equal");
    }

    delete c2;
    delete c1;
}

// @bug 4053636
//
// Collator thinks "black-bird" == "black"
//
void CollationRegressionTest::Test4053636(/* char* par */)
{
    if (en_us->equals("black_bird", "black"))
    {
        errln("black-bird == black");
    }
}

// @bug 4054238
//
// CollationElementIterator will not work correctly if the associated
// Collator object's mode is changed
//
void CollationRegressionTest::Test4054238(/* char* par */)
{
    const UChar chars3[] = {0x61, 0x00FC, 0x62, 0x65, 0x63, 0x6b, 0x20, 0x47, 0x72, 0x00F6, 0x00DF, 0x65, 0x20, 0x4c, 0x00FC, 0x62, 0x63, 0x6b, 0};
    const UnicodeString test3(chars3);
    RuleBasedCollator *c = (RuleBasedCollator *) en_us->clone();

    // NOTE: The Java code uses en_us to create the CollationElementIterators
    // but I'm pretty sure that's wrong, so I've changed this to use c.
    UErrorCode status = U_ZERO_ERROR;
    c->setAttribute(UCOL_NORMALIZATION_MODE, UCOL_ON, status);
    CollationElementIterator *i1 = c->createCollationElementIterator(test3);
    delete i1;
    delete c;
}

// @bug 4054734
//
// Collator::IDENTICAL documented but not implemented
//
void CollationRegressionTest::Test4054734(/* char* par */)
{
    /*
        Here's the original Java:

        String[] decomp = {
            "\u0001",   "<",    "\u0002",
            "\u0001",   "=",    "\u0001",
            "A\u0001",  ">",    "~\u0002",      // Ensure A and ~ are not compared bitwise
            "\u00C0",   "=",    "A\u0300"       // Decomp should make these equal
        };

        String[] nodecomp = {
            "\u00C0",   ">",    "A\u0300"       // A-grave vs. A combining-grave
        };
    */

    static const UChar decomp[][CollationRegressionTest::MAX_TOKEN_LEN] =
    {
        {0x0001, 0},      {0x3c, 0}, {0x0002, 0},
        {0x0001, 0},      {0x3d, 0}, {0x0001, 0},
        {0x41, 0x0001, 0}, {0x3e, 0}, {0x7e, 0x0002, 0},
        {0x00c0, 0},      {0x3d, 0}, {0x41, 0x0300, 0}
    };


    UErrorCode status = U_ZERO_ERROR;
    RuleBasedCollator *c = (RuleBasedCollator *) en_us->clone();

    c->setStrength(Collator::IDENTICAL);

    c->setAttribute(UCOL_NORMALIZATION_MODE, UCOL_ON, status);
    compareArray(*c, decomp, ARRAY_LENGTH(decomp));

    delete c;
}

// @bug 4054736
//
// Full Decomposition mode not implemented
//
void CollationRegressionTest::Test4054736(/* char* par */)
{
    UErrorCode status = U_ZERO_ERROR;
    RuleBasedCollator *c = (RuleBasedCollator *) en_us->clone();

    c->setStrength(Collator::SECONDARY);
    c->setAttribute(UCOL_NORMALIZATION_MODE, UCOL_ON, status);

    static const UChar tests[][CollationRegressionTest::MAX_TOKEN_LEN] =
    {
        {0xFB4F, 0}, {0x3d, 0}, {0x05D0, 0x05DC}  // Alef-Lamed vs. Alef, Lamed
    };

    compareArray(*c, tests, ARRAY_LENGTH(tests));

    delete c;
}

// @bug 4058613
//
// Collator::createInstance() causes an ArrayIndexOutofBoundsException for Korean
//
void CollationRegressionTest::Test4058613(/* char* par */)
{
    // Creating a default collator doesn't work when Korean is the default
    // locale

    Locale oldDefault = Locale::getDefault();
    UErrorCode status = U_ZERO_ERROR;

    Locale::setDefault(Locale::getKorean(), status);

    if (U_FAILURE(status))
    {
        errln("Could not set default locale to Locale::KOREAN");
        return;
    }

    Collator *c = NULL;

    c = Collator::createInstance("en_US", status);

    if (c == NULL || U_FAILURE(status))
    {
        errln("Could not create a Korean collator");
        Locale::setDefault(oldDefault, status);
        delete c;
        return;
    }

    // Since the fix to this bug was to turn off decomposition for Korean collators,
    // ensure that's what we got
    if (c->getAttribute(UCOL_NORMALIZATION_MODE, status) != UCOL_OFF)
    {
      errln("Decomposition is not set to NO_DECOMPOSITION for Korean collator");
    }

    delete c;

    Locale::setDefault(oldDefault, status);
}

// @bug 4059820
//
// RuleBasedCollator.getRules does not return the exact pattern as input
// for expanding character sequences
//
void CollationRegressionTest::Test4059820(/* char* par */)
{
    UErrorCode status = U_ZERO_ERROR;

    RuleBasedCollator *c = NULL;
    UnicodeString rules = "< a < b , c/a < d < z";

    c = new RuleBasedCollator(rules, status);

    if (c == NULL || U_FAILURE(status))
    {
        errln("Failure building a collator.");
        delete c;
        return;
    }

    if ( c->getRules().indexOf("c/a") == -1)
    {
        errln("returned rules do not contain 'c/a'");
    }

    delete c;
}

// @bug 4060154
//
// MergeCollation::fixEntry broken for "& H < \u0131, \u0130, i, I"
//
void CollationRegressionTest::Test4060154(/* char* par */)
{
    UErrorCode status = U_ZERO_ERROR;
    UnicodeString rules;

    rules += "< g, G < h, H < i, I < j, J";
    rules +=  " & H < ";
    rules += (UChar)0x0131;
    rules += ", ";
    rules += (UChar)0x0130;
    rules += ", i, I";

    RuleBasedCollator *c = NULL;

    c = new RuleBasedCollator(rules, status);

    if (c == NULL || U_FAILURE(status))
    {
        errln("failure building collator.");
        delete c;
        return;
    }

    c->setAttribute(UCOL_NORMALIZATION_MODE, UCOL_ON, status);

 /*
    String[] tertiary = {
        "A",        "<",    "B",
        "H",        "<",    "\u0131",
        "H",        "<",    "I",
        "\u0131",   "<",    "\u0130",
        "\u0130",   "<",    "i",
        "\u0130",   ">",    "H",
    };
*/

    static const UChar tertiary[][CollationRegressionTest::MAX_TOKEN_LEN] =
    {
        {0x41, 0},    {0x3c, 0}, {0x42, 0},
        {0x48, 0},    {0x3c, 0}, {0x0131, 0},
        {0x48, 0},    {0x3c, 0}, {0x49, 0},
        {0x0131, 0}, {0x3c, 0}, {0x0130, 0},
        {0x0130, 0}, {0x3c, 0}, {0x69, 0},
        {0x0130, 0}, {0x3e, 0}, {0x48, 0}
    };

    c->setStrength(Collator::TERTIARY);
    compareArray(*c, tertiary, ARRAY_LENGTH(tertiary));

    /*
    String[] secondary = {
        "H",        "<",    "I",
        "\u0131",   "=",    "\u0130",
    };
*/
    static const UChar secondary[][CollationRegressionTest::MAX_TOKEN_LEN] =
    {
        {0x48, 0},    {0x3c, 0}, {0x49, 0},
        {0x0131, 0}, {0x3d, 0}, {0x0130, 0}
    };

    c->setStrength(Collator::PRIMARY);
    compareArray(*c, secondary, ARRAY_LENGTH(secondary));

    delete c;
}

// @bug 4062418
//
// Secondary/Tertiary comparison incorrect in French Secondary
//
void CollationRegressionTest::Test4062418(/* char* par */)
{
    UErrorCode status = U_ZERO_ERROR;

    RuleBasedCollator *c = NULL;

    c = (RuleBasedCollator *) Collator::createInstance(Locale::getCanadaFrench(), status);

    if (c == NULL || U_FAILURE(status))
    {
        errln("Failed to create collator for Locale::getCanadaFrench()");
        delete c;
        return;
    }

    c->setStrength(Collator::SECONDARY);

/*
    String[] tests = {
            "p\u00eache",    "<",    "p\u00e9ch\u00e9",    // Comparing accents from end, p\u00e9ch\u00e9 is greater
    };
*/
    static const UChar tests[][CollationRegressionTest::MAX_TOKEN_LEN] =
    {
        {0x70, 0x00EA, 0x63, 0x68, 0x65, 0}, {0x3c, 0}, {0x70, 0x00E9, 0x63, 0x68, 0x00E9, 0}
    };

    compareArray(*c, tests, ARRAY_LENGTH(tests));

    delete c;
}

// @bug 4065540
//
// Collator::compare() method broken if either string contains spaces
//
void CollationRegressionTest::Test4065540(/* char* par */)
{
    if (en_us->compare("abcd e", "abcd f") == 0)
    {
        errln("'abcd e' == 'abcd f'");
    }
}

// @bug 4066189
//
// Unicode characters need to be recursively decomposed to get the
// correct result. For example,
// u1EB1 -> \u0103 + \u0300 -> a + \u0306 + \u0300.
//
void CollationRegressionTest::Test4066189(/* char* par */)
{
    static const UChar chars1[] = {0x1EB1, 0};
    static const UChar chars2[] = {0x61, 0x0306, 0x0300, 0};
    const UnicodeString test1(chars1);
    const UnicodeString test2(chars2);
    UErrorCode status = U_ZERO_ERROR;

    // NOTE: The java code used en_us to create the
    // CollationElementIterator's. I'm pretty sure that
    // was wrong, so I've change the code to use c1 and c2
    RuleBasedCollator *c1 = (RuleBasedCollator *) en_us->clone();
    c1->setAttribute(UCOL_NORMALIZATION_MODE, UCOL_ON, status);
    CollationElementIterator *i1 = c1->createCollationElementIterator(test1);

    RuleBasedCollator *c2 = (RuleBasedCollator *) en_us->clone();
    c2->setAttribute(UCOL_NORMALIZATION_MODE, UCOL_OFF, status);
    CollationElementIterator *i2 = c2->createCollationElementIterator(test2);

    assertEqual(*i1, *i2);

    delete i2;
    delete c2;
    delete i1;
    delete c1;
}

// @bug 4066696
//
// French secondary collation checking at the end of compare iteration fails
//
void CollationRegressionTest::Test4066696(/* char* par */)
{
    UErrorCode status = U_ZERO_ERROR;
    RuleBasedCollator *c = NULL;

    c = (RuleBasedCollator *)Collator::createInstance(Locale::getCanadaFrench(), status);

    if (c == NULL || U_FAILURE(status))
    {
        errln("Failure creating collator for Locale::getCanadaFrench()");
        delete c;
        return;
    }

    c->setStrength(Collator::SECONDARY);

/*
    String[] tests = {
        "\u00e0",   "<",     "\u01fa",       // a-grave <  A-ring-acute
    };

  should be:

    String[] tests = {
        "\u00e0",   ">",     "\u01fa",       // a-grave <  A-ring-acute
    };

*/

    static const UChar tests[][CollationRegressionTest::MAX_TOKEN_LEN] =
    {
        {0x00E0, 0}, {0x3e, 0}, {0x01FA, 0}
    };

    compareArray(*c, tests, ARRAY_LENGTH(tests));

    delete c;
}

// @bug 4076676
//
// Bad canonicalization of same-class combining characters
//
void CollationRegressionTest::Test4076676(/* char* par */)
{
    // These combining characters are all in the same class, so they should not
    // be reordered, and they should compare as unequal.
    static const UChar s1[] = {0x41, 0x0301, 0x0302, 0x0300, 0};
    static const UChar s2[] = {0x41, 0x0302, 0x0300, 0x0301, 0};

    RuleBasedCollator *c = (RuleBasedCollator *) en_us->clone();
    c->setStrength(Collator::TERTIARY);

    if (c->compare(s1,s2) == 0)
    {
        errln("Same-class combining chars were reordered");
    }

    delete c;
}

// @bug 4079231
//
// RuleBasedCollator::operator==(NULL) throws NullPointerException
//
void CollationRegressionTest::Test4079231(/* char* par */)
{
    // I don't think there's any way to write this test
    // in C++. The following is equivalent to the Java,
    // but doesn't compile 'cause NULL can't be converted
    // to Collator&
    //
    // if (en_us->operator==(NULL))
    // {
    //     errln("en_us->operator==(NULL) returned TRUE");
    // }

 /*
   try {
        if (en_us->equals(null)) {
            errln("en_us->equals(null) returned true");
        }
    }
    catch (Exception e) {
        errln("en_us->equals(null) threw " + e.toString());
    }
*/
}

// @bug 4078588
//
// RuleBasedCollator breaks on "< a < bb" rule
//
void CollationRegressionTest::Test4078588(/* char *par */)
{
    UErrorCode status = U_ZERO_ERROR;
    RuleBasedCollator *rbc = new RuleBasedCollator((UnicodeString)"< a < bb", status);

    if (rbc == NULL || U_FAILURE(status))
    {
        errln("Failed to create RuleBasedCollator.");
        delete rbc;
        return;
    }

    Collator::EComparisonResult result = rbc->compare("a","bb");

    if (result != Collator::LESS)
    {
        errln((UnicodeString)"Compare(a,bb) returned " + (int)result
            + (UnicodeString)"; expected -1");
    }

    delete rbc;
}

// @bug 4081866
//
// Combining characters in different classes not reordered properly.
//
void CollationRegressionTest::Test4081866(/* char* par */)
{
    // These combining characters are all in different classes,
    // so they should be reordered and the strings should compare as equal.
    static const UChar s1[] = {0x41, 0x0300, 0x0316, 0x0327, 0x0315, 0};
    static const UChar s2[] = {0x41, 0x0327, 0x0316, 0x0315, 0x0300, 0};

    UErrorCode status = U_ZERO_ERROR;
    RuleBasedCollator *c = (RuleBasedCollator *) en_us->clone();
    c->setStrength(Collator::TERTIARY);

    // Now that the default collators are set to NO_DECOMPOSITION
    // (as a result of fixing bug 4114077), we must set it explicitly
    // when we're testing reordering behavior.  -- lwerner, 5/5/98
    c->setAttribute(UCOL_NORMALIZATION_MODE, UCOL_ON, status);

    if (c->compare(s1,s2) != 0)
    {
        errln("Combining chars were not reordered");
    }

    delete c;
}

// @bug 4087241
//
// string comparison errors in Scandinavian collators
//
void CollationRegressionTest::Test4087241(/* char* par */)
{
    UErrorCode status = U_ZERO_ERROR;
    Locale da_DK("da", "DK");
    RuleBasedCollator *c = NULL;

    c = (RuleBasedCollator *) Collator::createInstance(da_DK, status);

    if (c == NULL || U_FAILURE(status))
    {
        errln("Failed to create collator for da_DK locale");
        delete c;
        return;
    }

    c->setStrength(Collator::SECONDARY);

    static const UChar tests[][CollationRegressionTest::MAX_TOKEN_LEN] =
    {
        {0x7a, 0},          {0x3c, 0}, {0x00E6, 0},            // z        < ae
        {0x61, 0x0308, 0}, {0x3c, 0}, {0x61, 0x030A, 0},      // a-unlaut < a-ring
        {0x59, 0},          {0x3c, 0}, {0x75, 0x0308, 0},      // Y        < u-umlaut
    };

    compareArray(*c, tests, ARRAY_LENGTH(tests));

    delete c;
}

// @bug 4087243
//
// CollationKey takes ignorable strings into account when it shouldn't
//
void CollationRegressionTest::Test4087243(/* char* par */)
{
    RuleBasedCollator *c = (RuleBasedCollator *) en_us->clone();
    c->setStrength(Collator::TERTIARY);

    static const UChar tests[][CollationRegressionTest::MAX_TOKEN_LEN] =
    {
        {0x31, 0x32, 0x33, 0}, {0x3d, 0}, {0x31, 0x32, 0x33, 0x0001, 0}    // 1 2 3  =  1 2 3 ctrl-A
    };

    compareArray(*c, tests, ARRAY_LENGTH(tests));

    delete c;
}

// @bug 4092260
//
// Mu/micro conflict
// Micro symbol and greek lowercase letter Mu should sort identically
//
void CollationRegressionTest::Test4092260(/* char* par */)
{
    UErrorCode status = U_ZERO_ERROR;
    Locale el("el", "");
    Collator *c = NULL;

    c = Collator::createInstance(el, status);

    if (c == NULL || U_FAILURE(status))
    {
        errln("Failed to create collator for el locale.");
        delete c;
        return;
    }

    // These now have tertiary differences in UCA
    c->setAttribute(UCOL_STRENGTH, UCOL_SECONDARY, status);

    static const UChar tests[][CollationRegressionTest::MAX_TOKEN_LEN] =
    {
        {0x00B5, 0}, {0x3d, 0}, {0x03BC, 0}
    };

    compareArray(*c, tests, ARRAY_LENGTH(tests));

    delete c;
}

// @bug 4095316
//
void CollationRegressionTest::Test4095316(/* char* par */)
{
    UErrorCode status = U_ZERO_ERROR;
    Locale el_GR("el", "GR");
    Collator *c = Collator::createInstance(el_GR, status);

    if (c == NULL || U_FAILURE(status))
    {
        errln("Failed to create collator for el_GR locale");
        delete c;
        return;
    }
    // These now have tertiary differences in UCA
    //c->setStrength(Collator::TERTIARY);
    c->setAttribute(UCOL_STRENGTH, UCOL_SECONDARY, status);

    static const UChar tests[][CollationRegressionTest::MAX_TOKEN_LEN] =
    {
        {0x03D4, 0}, {0x3d, 0}, {0x03AB, 0}
    };

    compareArray(*c, tests, ARRAY_LENGTH(tests));

    delete c;
}

// @bug 4101940
//
void CollationRegressionTest::Test4101940(/* char* par */)
{
    UErrorCode status = U_ZERO_ERROR;
    RuleBasedCollator *c = NULL;
    UnicodeString rules = "< a < b";
    UnicodeString nothing = "";

    c = new RuleBasedCollator(rules, status);

    if (c == NULL || U_FAILURE(status))
    {
        errln("Failed to create RuleBasedCollator");
        delete c;
        return;
    }

    CollationElementIterator *i = c->createCollationElementIterator(nothing);
    i->reset();

    if (i->next(status) != CollationElementIterator::NULLORDER)
    {
        errln("next did not return NULLORDER");
    }

    delete i;
    delete c;
}

// @bug 4103436
//
// Collator::compare not handling spaces properly
//
void CollationRegressionTest::Test4103436(/* char* par */)
{
    RuleBasedCollator *c = (RuleBasedCollator *) en_us->clone();
    c->setStrength(Collator::TERTIARY);

    static const UChar tests[][CollationRegressionTest::MAX_TOKEN_LEN] =
    {
        {0x66, 0x69, 0x6c, 0x65, 0}, {0x3c, 0}, {0x66, 0x69, 0x6c, 0x65, 0x20, 0x61, 0x63, 0x63, 0x65, 0x73, 0x73, 0},
        {0x66, 0x69, 0x6c, 0x65, 0}, {0x3c, 0}, {0x66, 0x69, 0x6c, 0x65, 0x61, 0x63, 0x63, 0x65, 0x73, 0x73, 0}
    };

    compareArray(*c, tests, ARRAY_LENGTH(tests));

    delete c;
}

// @bug 4114076
//
// Collation not Unicode conformant with Hangul syllables
//
void CollationRegressionTest::Test4114076(/* char* par */)
{
    UErrorCode status = U_ZERO_ERROR;
    RuleBasedCollator *c = (RuleBasedCollator *) en_us->clone();
    c->setStrength(Collator::TERTIARY);

    //
    // With Canonical decomposition, Hangul syllables should get decomposed
    // into Jamo, but Jamo characters should not be decomposed into
    // conjoining Jamo
    //
    static const UChar test1[][CollationRegressionTest::MAX_TOKEN_LEN] =
    {
        {0xd4db, 0}, {0x3d, 0}, {0x1111, 0x1171, 0x11b6, 0}
    };

    c->setAttribute(UCOL_NORMALIZATION_MODE, UCOL_ON, status);
    compareArray(*c, test1, ARRAY_LENGTH(test1));

    // From UTR #15:
    // *In earlier versions of Unicode, jamo characters like ksf
    //  had compatibility mappings to kf + sf. These mappings were
    //  removed in Unicode 2.1.9 to ensure that Hangul syllables are maintained.)
    // That is, the following test is obsolete as of 2.1.9

//obsolete-    // With Full decomposition, it should go all the way down to
//obsolete-    // conjoining Jamo characters.
//obsolete-    //
//obsolete-    static const UChar test2[][CollationRegressionTest::MAX_TOKEN_LEN] =
//obsolete-    {
//obsolete-        {0xd4db, 0}, {0x3d, 0}, {0x1111, 0x116e, 0x1175, 0x11af, 0x11c2, 0}
//obsolete-    };
//obsolete-
//obsolete-    c->setDecomposition(Normalizer::DECOMP_COMPAT);
//obsolete-    compareArray(*c, test2, ARRAY_LENGTH(test2));

    delete c;
}


// @bug 4124632
//
// Collator::getCollationKey was hanging on certain character sequences
//
void CollationRegressionTest::Test4124632(/* char* par */)
{
    UErrorCode status = U_ZERO_ERROR;
    Collator *coll = NULL;

    coll = Collator::createInstance(Locale::getJapan(), status);

    if (coll == NULL || U_FAILURE(status))
    {
        errln("Failed to create collator for Locale::JAPAN");
        delete coll;
        return;
    }

    static const UChar test[] = {0x41, 0x0308, 0x62, 0x63, 0};
    CollationKey key;

    coll->getCollationKey(test, key, status);

    if (key.isBogus() || U_FAILURE(status))
    {
        errln("CollationKey creation failed.");
    }

    delete coll;
}

// @bug 4132736
//
// sort order of french words with multiple accents has errors
//
void CollationRegressionTest::Test4132736(/* char* par */)
{
    UErrorCode status = U_ZERO_ERROR;

    Collator *c = NULL;

    c = Collator::createInstance(Locale::getCanadaFrench(), status);
    c->setStrength(Collator::TERTIARY);

    if (c == NULL || U_FAILURE(status))
    {
        errln("Failed to create a collator for Locale::getCanadaFrench()");
        delete c;
        return;
    }

    static const UChar test1[][CollationRegressionTest::MAX_TOKEN_LEN] =
    {
        {0x65, 0x0300, 0x65, 0x0301, 0}, {0x3c, 0}, {0x65, 0x0301, 0x65, 0x0300, 0},
        {0x65, 0x0300, 0x0301, 0},       {0x3c, 0}, {0x65, 0x0301, 0x0300, 0}
    };

    compareArray(*c, test1, ARRAY_LENGTH(test1));

    delete c;
}

// @bug 4133509
//
// The sorting using java.text.CollationKey is not in the exact order
//
void CollationRegressionTest::Test4133509(/* char* par */)
{
    static const UChar test1[][CollationRegressionTest::MAX_TOKEN_LEN] =
    {
        {0x45, 0x78, 0x63, 0x65, 0x70, 0x74, 0x69, 0x6f, 0x6e, 0}, {0x3c, 0}, {0x45, 0x78, 0x63, 0x65, 0x70, 0x74, 0x69, 0x6f, 0x6e, 0x49, 0x6e, 0x49, 0x6e, 0x69, 0x74, 0x69, 0x61, 0x6c, 0x69, 0x7a, 0x65, 0x72, 0x45, 0x72, 0x72, 0x6f, 0x72, 0},
        {0x47, 0x72, 0x61, 0x70, 0x68, 0x69, 0x63, 0x73, 0},      {0x3c, 0}, {0x47, 0x72, 0x61, 0x70, 0x68, 0x69, 0x63, 0x73, 0x45, 0x6e, 0x76, 0x69, 0x72, 0x6f, 0x6e, 0x6d, 0x65, 0x6e, 0x74, 0},
        {0x53, 0x74, 0x72, 0x69, 0x6e, 0x67, 0},                  {0x3c, 0}, {0x53, 0x74, 0x72, 0x69, 0x6e, 0x67, 0x42, 0x75, 0x66, 0x66, 0x65, 0x72, 0}
    };

    compareArray(*en_us, test1, ARRAY_LENGTH(test1));
}

// @bug 4114077
//
// Collation with decomposition off doesn't work for Europe
//
void CollationRegressionTest::Test4114077(/* char* par */)
{
    // Ensure that we get the same results with decomposition off
    // as we do with it on....

    UErrorCode status = U_ZERO_ERROR;
    RuleBasedCollator *c = (RuleBasedCollator *) en_us->clone();
    c->setStrength(Collator::TERTIARY);

    static const UChar test1[][CollationRegressionTest::MAX_TOKEN_LEN] =
    {
        {0x00C0, 0},                     {0x3d, 0}, {0x41, 0x0300, 0},            // Should be equivalent
        {0x70, 0x00ea, 0x63, 0x68, 0x65, 0}, {0x3e, 0}, {0x70, 0x00e9, 0x63, 0x68, 0x00e9, 0},
        {0x0204, 0},                     {0x3d, 0}, {0x45, 0x030F, 0},
        {0x01fa, 0},                     {0x3d, 0}, {0x41, 0x030a, 0x0301, 0},    // a-ring-acute -> a-ring, acute
                                                //   -> a, ring, acute
        {0x41, 0x0300, 0x0316, 0},         {0x3c, 0}, {0x41, 0x0316, 0x0300, 0}        // No reordering --> unequal
    };

    c->setAttribute(UCOL_NORMALIZATION_MODE, UCOL_OFF, status);
    compareArray(*c, test1, ARRAY_LENGTH(test1));

    static const UChar test2[][CollationRegressionTest::MAX_TOKEN_LEN] =
    {
        {0x41, 0x0300, 0x0316, 0}, {0x3d, 0}, {0x41, 0x0316, 0x0300, 0}      // Reordering --> equal
    };

    c->setAttribute(UCOL_NORMALIZATION_MODE, UCOL_ON, status);
    compareArray(*c, test2, ARRAY_LENGTH(test2));

    delete c;
}

// @bug 4141640
//
// Support for Swedish gone in 1.1.6 (Can't create Swedish collator)
//
void CollationRegressionTest::Test4141640(/* char* par */)
{
    //
    // Rather than just creating a Swedish collator, we might as well
    // try to instantiate one for every locale available on the system
    // in order to prevent this sort of bug from cropping up in the future
    //
    UErrorCode status = U_ZERO_ERROR;
    int32_t i, localeCount;
    const Locale *locales = Locale::getAvailableLocales(localeCount);

    for (i = 0; i < localeCount; i += 1)
    {
        Collator *c = NULL;

        status = U_ZERO_ERROR;
        c = Collator::createInstance(locales[i], status);

        if (c == NULL || U_FAILURE(status))
        {
            UnicodeString msg, localeName;

            msg += "Could not create collator for locale ";
            msg += locales[i].getName();

            errln(msg);
        }

        delete c;
    }
}

// @bug 4139572
//
// getCollationKey throws exception for spanish text
// Cannot reproduce this bug on 1.2, however it DOES fail on 1.1.6
//
void CollationRegressionTest::Test4139572(/* char* par */)
{
    //
    // Code pasted straight from the bug report
    // (and then translated to C++ ;-)
    //
    // create spanish locale and collator
    UErrorCode status = U_ZERO_ERROR;
    Locale l("es", "es");
    Collator *col = NULL;

    col = Collator::createInstance(l, status);

    if (col == NULL || U_FAILURE(status))
    {
        errln("Failed to create a collator for es_es locale.");
        delete col;
        return;
    }

    CollationKey key;

    // this spanish phrase kills it!
    col->getCollationKey("Nombre De Objeto", key, status);

    if (key.isBogus() || U_FAILURE(status))
    {
        errln("Error creating CollationKey for \"Nombre De Ojbeto\"");
    }

    delete col;
}
/* HSYS : RuleBasedCollator::compare() performance enhancements
          compare() does not create CollationElementIterator() anymore.*/

class My4146160Collator : public RuleBasedCollator
{
public:
    My4146160Collator(RuleBasedCollator &rbc, UErrorCode &status);
    ~My4146160Collator();

    CollationElementIterator *createCollationElementIterator(const UnicodeString &text) const;

    CollationElementIterator *createCollationElementIterator(const CharacterIterator &text) const;

    static int32_t count;
};

int32_t My4146160Collator::count = 0;

My4146160Collator::My4146160Collator(RuleBasedCollator &rbc, UErrorCode &status)
  : RuleBasedCollator(rbc.getRules(), status)
{
}

My4146160Collator::~My4146160Collator()
{
}

CollationElementIterator *My4146160Collator::createCollationElementIterator(const UnicodeString &text) const
{
    count += 1;
    return RuleBasedCollator::createCollationElementIterator(text);
}

CollationElementIterator *My4146160Collator::createCollationElementIterator(const CharacterIterator &text) const
{
    count += 1;
    return RuleBasedCollator::createCollationElementIterator(text);
}

// @bug 4146160
//
// RuleBasedCollator doesn't use createCollationElementIterator internally
//
void CollationRegressionTest::Test4146160(/* char* par */)
{
#if 0
    //
    // Use a custom collator class whose createCollationElementIterator
    // methods increment a count....
    //
    UErrorCode status = U_ZERO_ERROR;
    CollationKey key;

    My4146160Collator::count = 0;
    My4146160Collator *mc = NULL;

    mc = new My4146160Collator(*en_us, status);

    if (mc == NULL || U_FAILURE(status))
    {
        errln("Failed to create a My4146160Collator.");
        delete mc;
        return;
    }

    mc->getCollationKey("1", key, status);

    if (key.isBogus() || U_FAILURE(status))
    {
        errln("Failure to get a CollationKey from a My4146160Collator.");
        delete mc;
        return;
    }

    if (My4146160Collator::count < 1)
    {
        errln("My4146160Collator::createCollationElementIterator not called for getCollationKey");
    }

    My4146160Collator::count = 0;
    mc->compare("1", "2");

    if (My4146160Collator::count < 1)
    {
        errln("My4146160Collator::createtCollationElementIterator not called for compare");
    }

    delete mc;
#endif
}

// Ticket 7189
//
// nextSortKeyPart incorrect for EO_S1 collation
static int32_t calcKeyIncremental(UCollator *coll, const UChar* text, int32_t len, uint8_t *keyBuf, int32_t /*keyBufLen*/, UErrorCode& status) {
    UCharIterator uiter;
    uint32_t state[2] = { 0, 0 };
    int32_t keyLen;
    int32_t count = 8;

    uiter_setString(&uiter, text, len);
    keyLen = 0;
    while (TRUE) {
        int32_t keyPartLen = ucol_nextSortKeyPart(coll, &uiter, state, &keyBuf[keyLen], count, &status);
        if (U_FAILURE(status)) {
            return -1;
        }
        if (keyPartLen == 0) {
            break;
        }
        keyLen += keyPartLen;
    }
    return keyLen;
}

void CollationRegressionTest::TestT7189() {
    UErrorCode status = U_ZERO_ERROR;
    UCollator *coll;
    uint32_t i;

    static const UChar text1[][CollationRegressionTest::MAX_TOKEN_LEN] = {
    // "Achter De Hoven"
        { 0x41, 0x63, 0x68, 0x74, 0x65, 0x72, 0x20, 0x44, 0x65, 0x20, 0x48, 0x6F, 0x76, 0x65, 0x6E, 0x00 },
        // "ABC"
        { 0x41, 0x42, 0x43, 0x00 },
        // "HELLO world!"
        { 0x48, 0x45, 0x4C, 0x4C, 0x4F, 0x20, 0x77, 0x6F, 0x72, 0x6C, 0x64, 0x21, 0x00 }
    };

    static const UChar text2[][CollationRegressionTest::MAX_TOKEN_LEN] = {
    // "Achter de Hoven"
        { 0x41, 0x63, 0x68, 0x74, 0x65, 0x72, 0x20, 0x64, 0x65, 0x20, 0x48, 0x6F, 0x76, 0x65, 0x6E, 0x00 },
        // "abc"
        { 0x61, 0x62, 0x63, 0x00 },
        // "hello world!"
        { 0x68, 0x65, 0x6C, 0x6C, 0x6F, 0x20, 0x77, 0x6F, 0x72, 0x6C, 0x64, 0x21, 0x00 }
    };

    // Open the collator
    coll = ucol_openFromShortString("EO_S1", FALSE, NULL, &status);
    if (U_FAILURE(status)) {
        errln("Failed to create a collator for short string EO_S1");
        return;
    }

    for (i = 0; i < sizeof(text1) / (CollationRegressionTest::MAX_TOKEN_LEN * sizeof(UChar)); i++) {
        uint8_t key1[100], key2[100];
        int32_t len1, len2;

        len1 = calcKeyIncremental(coll, text1[i], -1, key1, sizeof(key1), status);
        if (U_FAILURE(status)) {
            errln(UnicodeString("Failed to get a partial collation key for ") + text1[i]);
            break;
        }
        len2 = calcKeyIncremental(coll, text2[i], -1, key2, sizeof(key2), status);
        if (U_FAILURE(status)) {
            errln(UnicodeString("Failed to get a partial collation key for ") + text2[i]);
            break;
        }

        if (len1 == len2 && uprv_memcmp(key1, key2, len1) == 0) {
            errln(UnicodeString("Failed: Identical key\n") + "    text1: " + text1[i] + "\n" + "    text2: " + text2[i] + "\n" + "    key  : " + TestUtility::hex(key1, len1));
        } else {
            logln(UnicodeString("Keys produced -\n") + "    text1: " + text1[i] + "\n" + "    key1 : " + TestUtility::hex(key1, len1) + "\n" + "    text2: " + text2[i] + "\n" + "    key2 : "
                    + TestUtility::hex(key2, len2));
        }
    }
    ucol_close(coll);
}

void CollationRegressionTest::compareArray(Collator &c,
                                           const UChar tests[][CollationRegressionTest::MAX_TOKEN_LEN],
                                           int32_t testCount)
{
    int32_t i;
    Collator::EComparisonResult expectedResult = Collator::EQUAL;

    for (i = 0; i < testCount; i += 3)
    {
        UnicodeString source(tests[i]);
        UnicodeString comparison(tests[i + 1]);
        UnicodeString target(tests[i + 2]);

        if (comparison == "<")
        {
            expectedResult = Collator::LESS;
        }
        else if (comparison == ">")
        {
            expectedResult = Collator::GREATER;
        }
        else if (comparison == "=")
        {
            expectedResult = Collator::EQUAL;
        }
        else
        {
            UnicodeString bogus1("Bogus comparison string \"");
            UnicodeString bogus2("\"");
            errln(bogus1 + comparison + bogus2);
        }

        Collator::EComparisonResult compareResult = c.compare(source, target);

        CollationKey sourceKey, targetKey;
        UErrorCode status = U_ZERO_ERROR;

        c.getCollationKey(source, sourceKey, status);

        if (U_FAILURE(status))
        {
            errln("Couldn't get collationKey for source");
            continue;
        }

        c.getCollationKey(target, targetKey, status);

        if (U_FAILURE(status))
        {
            errln("Couldn't get collationKey for target");
            continue;
        }

        Collator::EComparisonResult keyResult = sourceKey.compareTo(targetKey);

        reportCResult( source, target, sourceKey, targetKey, compareResult, keyResult, compareResult, expectedResult );

    }
}

void CollationRegressionTest::assertEqual(CollationElementIterator &i1, CollationElementIterator &i2)
{
    int32_t c1, c2, count = 0;
    UErrorCode status = U_ZERO_ERROR;

    do
    {
        c1 = i1.next(status);
        c2 = i2.next(status);

        if (c1 != c2)
        {
            UnicodeString msg, msg1("    ");

            msg += msg1 + count;
            msg += ": strength(0x";
            appendHex(c1, 8, msg);
            msg += ") != strength(0x";
            appendHex(c2, 8, msg);
            msg += ")";

            errln(msg);
            break;
        }

        count += 1;
    }
    while (c1 != CollationElementIterator::NULLORDER);
}

void CollationRegressionTest::runIndexedTest(int32_t index, UBool exec, const char* &name, char* /* par */)
{
    if (exec)
    {
        logln("Collation Regression Tests: ");
    }

    if(en_us) {
      switch (index)
      {
          case  0: name = "Test4048446"; if (exec) Test4048446(/* par */); break;
          case  1: name = "Test4051866"; if (exec) Test4051866(/* par */); break;
          case  2: name = "Test4053636"; if (exec) Test4053636(/* par */); break;
          case  3: name = "Test4054238"; if (exec) Test4054238(/* par */); break;
          case  4: name = "Test4054734"; if (exec) Test4054734(/* par */); break;
          case  5: name = "Test4054736"; if (exec) Test4054736(/* par */); break;
          case  6: name = "Test4058613"; if (exec) Test4058613(/* par */); break;
          case  7: name = "Test4059820"; if (exec) Test4059820(/* par */); break;
          case  8: name = "Test4060154"; if (exec) Test4060154(/* par */); break;
          case  9: name = "Test4062418"; if (exec) Test4062418(/* par */); break;
          case 10: name = "Test4065540"; if (exec) Test4065540(/* par */); break;
          case 11: name = "Test4066189"; if (exec) Test4066189(/* par */); break;
          case 12: name = "Test4066696"; if (exec) Test4066696(/* par */); break;
          case 13: name = "Test4076676"; if (exec) Test4076676(/* par */); break;
          case 14: name = "Test4078588"; if (exec) Test4078588(/* par */); break;
          case 15: name = "Test4079231"; if (exec) Test4079231(/* par */); break;
          case 16: name = "Test4081866"; if (exec) Test4081866(/* par */); break;
          case 17: name = "Test4087241"; if (exec) Test4087241(/* par */); break;
          case 18: name = "Test4087243"; if (exec) Test4087243(/* par */); break;
          case 19: name = "Test4092260"; if (exec) Test4092260(/* par */); break;
          case 20: name = "Test4095316"; if (exec) Test4095316(/* par */); break;
          case 21: name = "Test4101940"; if (exec) Test4101940(/* par */); break;
          case 22: name = "Test4103436"; if (exec) Test4103436(/* par */); break;
          case 23: name = "Test4114076"; if (exec) Test4114076(/* par */); break;
          case 24: name = "Test4114077"; if (exec) Test4114077(/* par */); break;
          case 25: name = "Test4124632"; if (exec) Test4124632(/* par */); break;
          case 26: name = "Test4132736"; if (exec) Test4132736(/* par */); break;
          case 27: name = "Test4133509"; if (exec) Test4133509(/* par */); break;
          case 28: name = "Test4139572"; if (exec) Test4139572(/* par */); break;
          case 29: name = "Test4141640"; if (exec) Test4141640(/* par */); break;
          case 30: name = "Test4146160"; if (exec) Test4146160(/* par */); break;
		  case 31: name = "TestT7189";   if (exec) TestT7189(); break;
          default: name = ""; break;
      }
    } else {
      dataerrln("Class collator not instantiated");
      name = "";
    }
}

#endif /* #if !UCONFIG_NO_COLLATION */
