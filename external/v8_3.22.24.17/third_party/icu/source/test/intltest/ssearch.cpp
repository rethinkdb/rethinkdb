/*
 **********************************************************************
 *   Copyright (C) 2005-2010, International Business Machines
 *   Corporation and others.  All Rights Reserved.
 **********************************************************************
 */


#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/unistr.h"
#include "unicode/putil.h"
#include "unicode/usearch.h"

#include "cmemory.h"
#include "unicode/coll.h"
#include "unicode/tblcoll.h"
#include "unicode/coleitr.h"
#include "unicode/ucoleitr.h"

#include "unicode/regex.h"        // TODO: make conditional on regexp being built.

#include "unicode/uniset.h"
#include "unicode/uset.h"
#include "unicode/ustring.h"
#include "hash.h"
#include "uhash.h"
#include "ucol_imp.h"

#include "intltest.h"
#include "ssearch.h"

#include "unicode/colldata.h"
#include "unicode/bmsearch.h"
#include "unicode/bms.h"

#include "xmlparser.h"
#include "ucbuf.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

char testId[100];

#define TEST_ASSERT(x) {if (!(x)) { \
    errln("Failure in file %s, line %d, test ID = \"%s\"", __FILE__, __LINE__, testId);}}

#define TEST_ASSERT_M(x, m) {if (!(x)) { \
    errln("Failure in file %s, line %d.   \"%s\"", __FILE__, __LINE__, m);return;}}

#define TEST_ASSERT_SUCCESS(errcode) {if (U_FAILURE(errcode)) { \
    dataerrln("Failure in file %s, line %d, test ID \"%s\", status = \"%s\"", \
          __FILE__, __LINE__, testId, u_errorName(errcode));}}

#define ARRAY_SIZE(array) (sizeof array / sizeof array[0])
#define NEW_ARRAY(type, count) (type *) uprv_malloc((count) * sizeof(type))
#define DELETE_ARRAY(array) uprv_free((void *) (array))

//---------------------------------------------------------------------------
//
//  Test class boilerplate
//
//---------------------------------------------------------------------------
SSearchTest::SSearchTest()
{
}

SSearchTest::~SSearchTest()
{
}

void SSearchTest::runIndexedTest( int32_t index, UBool exec, const char* &name, char *params )
{
    if (exec) logln("TestSuite SSearchTest: ");
    switch (index) {
#if !UCONFIG_NO_BREAK_ITERATION
       case 0: name = "searchTest";
            if (exec) searchTest();
            break;

        case 1: name = "offsetTest";
            if (exec) offsetTest();
            break;

        case 2: name = "monkeyTest";
            if (exec) monkeyTest(params);
            break;

        case 3: name = "bmMonkeyTest";
            if (exec) bmMonkeyTest(params);
            break;

        case 4: name = "boyerMooreTest";
            if (exec) boyerMooreTest();
            break;

        case 5: name = "goodSuffixTest";
            if (exec) goodSuffixTest();
            break;

        case 6: name = "searchTime";
            if (exec) searchTime();
            break;

        case 7: name = "bmsTest";
            if (exec) bmsTest();
            break;

        case 8: name = "bmSearchTest";
            if (exec) bmSearchTest();
            break;

        case 9: name = "udhrTest";
            if (exec) udhrTest();
            break;
        case 10: name = "stringListTest";
            if (exec) stringListTest();
            break;
#endif
        default: name = "";
            break; //needed to end loop
    }
}


#if !UCONFIG_NO_BREAK_ITERATION

#define PATH_BUFFER_SIZE 2048
const char *SSearchTest::getPath(char buffer[2048], const char *filename) {
    UErrorCode status = U_ZERO_ERROR;
    const char *testDataDirectory = IntlTest::getSourceTestData(status);

    if (U_FAILURE(status) || strlen(testDataDirectory) + strlen(filename) + 1 >= PATH_BUFFER_SIZE) {
        errln("ERROR: getPath() failed - %s", u_errorName(status));
        return NULL;
    }

    strcpy(buffer, testDataDirectory);
    strcat(buffer, filename);
    return buffer;
}


void SSearchTest::searchTest()
{
#if !UCONFIG_NO_REGULAR_EXPRESSIONS && !UCONFIG_NO_FILE_IO
    UErrorCode status = U_ZERO_ERROR;
    char path[PATH_BUFFER_SIZE];
    const char *testFilePath = getPath(path, "ssearch.xml");

    if (testFilePath == NULL) {
        return; /* Couldn't get path: error message already output. */
    }

    LocalPointer<UXMLParser> parser(UXMLParser::createParser(status));
    TEST_ASSERT_SUCCESS(status);
    LocalPointer<UXMLElement> root(parser->parseFile(testFilePath, status));
    TEST_ASSERT_SUCCESS(status);
    if (U_FAILURE(status)) {
        return;
    }

    const UnicodeString *debugTestCase = root->getAttribute("debug");
    if (debugTestCase != NULL) {
//       setenv("USEARCH_DEBUG", "1", 1);
    }


    const UXMLElement *testCase;
    int32_t tc = 0;

    while((testCase = root->nextChildElement(tc)) != NULL) {

        if (testCase->getTagName().compare("test-case") != 0) {
            errln("ssearch, unrecognized XML Element in test file");
            continue;
        }
        const UnicodeString *id       = testCase->getAttribute("id");
        *testId = 0;
        if (id != NULL) {
            id->extract(0, id->length(), testId,  sizeof(testId), US_INV);
        }

        // If debugging test case has been specified and this is not it, skip to next.
        if (id!=NULL && debugTestCase!=NULL && *id != *debugTestCase) {
            continue;
        }
        //
        //  Get the requested collation strength.
        //    Default is tertiary if the XML attribute is missing from the test case.
        //
        const UnicodeString *strength = testCase->getAttribute("strength");
        UColAttributeValue collatorStrength = UCOL_PRIMARY;
        if      (strength==NULL)          { collatorStrength = UCOL_TERTIARY;}
        else if (*strength=="PRIMARY")    { collatorStrength = UCOL_PRIMARY;}
        else if (*strength=="SECONDARY")  { collatorStrength = UCOL_SECONDARY;}
        else if (*strength=="TERTIARY")   { collatorStrength = UCOL_TERTIARY;}
        else if (*strength=="QUATERNARY") { collatorStrength = UCOL_QUATERNARY;}
        else if (*strength=="IDENTICAL")  { collatorStrength = UCOL_IDENTICAL;}
        else {
            // Bogus value supplied for strength.  Shouldn't happen, even from
            //  typos, if the  XML source has been validated.
            //  This assert is a little deceiving in that strength can be
            //   any of the allowed values, not just TERTIARY, but it will
            //   do the job of getting the error output.
            TEST_ASSERT(*strength=="TERTIARY")
        }

        //
        // Get the collator normalization flag.  Default is UCOL_OFF.
        //
        UColAttributeValue normalize = UCOL_OFF;
        const UnicodeString *norm = testCase->getAttribute("norm");
        TEST_ASSERT (norm==NULL || *norm=="ON" || *norm=="OFF");
        if (norm!=NULL && *norm=="ON") {
            normalize = UCOL_ON;
        }

        //
        // Get the alternate_handling flag. Default is UCOL_NON_IGNORABLE.
        //
        UColAttributeValue alternateHandling = UCOL_NON_IGNORABLE;
        const UnicodeString *alt = testCase->getAttribute("alternate_handling");
        TEST_ASSERT (alt == NULL || *alt == "SHIFTED" || *alt == "NON_IGNORABLE");
        if (alt != NULL && *alt == "SHIFTED") {
            alternateHandling = UCOL_SHIFTED;
        }

        const UnicodeString defLocale("en");
        char  clocale[100];
        const UnicodeString *locale   = testCase->getAttribute("locale");
        if (locale == NULL || locale->length()==0) {
            locale = &defLocale;
        };
        locale->extract(0, locale->length(), clocale, sizeof(clocale), NULL);


        UnicodeString  text;
        UnicodeString  target;
        UnicodeString  pattern;
        int32_t        expectedMatchStart = -1;
        int32_t        expectedMatchLimit = -1;
        const UXMLElement  *n;
        int32_t                nodeCount = 0;

        n = testCase->getChildElement("pattern");
        TEST_ASSERT(n != NULL);
        if (n==NULL) {
            continue;
        }
        text = n->getText(FALSE);
        text = text.unescape();
        pattern.append(text);
        nodeCount++;

        n = testCase->getChildElement("pre");
        if (n!=NULL) {
            text = n->getText(FALSE);
            text = text.unescape();
            target.append(text);
            nodeCount++;
        }

        n = testCase->getChildElement("m");
        if (n!=NULL) {
            expectedMatchStart = target.length();
            text = n->getText(FALSE);
            text = text.unescape();
            target.append(text);
            expectedMatchLimit = target.length();
            nodeCount++;
        }

        n = testCase->getChildElement("post");
        if (n!=NULL) {
            text = n->getText(FALSE);
            text = text.unescape();
            target.append(text);
            nodeCount++;
        }

        //  Check that there weren't extra things in the XML
        TEST_ASSERT(nodeCount == testCase->countChildren());

        // Open a collator and StringSearch based on the parameters
        //   obtained from the XML.
        //
        status = U_ZERO_ERROR;
        LocalUCollatorPointer collator(ucol_open(clocale, &status));
        ucol_setStrength(collator.getAlias(), collatorStrength);
        ucol_setAttribute(collator.getAlias(), UCOL_NORMALIZATION_MODE, normalize, &status);
        ucol_setAttribute(collator.getAlias(), UCOL_ALTERNATE_HANDLING, alternateHandling, &status);
        LocalUStringSearchPointer uss(usearch_openFromCollator(pattern.getBuffer(), pattern.length(),
                                                               target.getBuffer(), target.length(),
                                                               collator.getAlias(),
                                                               NULL,     // the break iterator
                                                               &status));

        TEST_ASSERT_SUCCESS(status);
        if (U_FAILURE(status)) {
            continue;
        }

        int32_t foundStart = 0;
        int32_t foundLimit = 0;
        UBool   foundMatch;

        //
        // Do the search, check the match result against the expected results.
        //
        foundMatch= usearch_search(uss.getAlias(), 0, &foundStart, &foundLimit, &status);
        TEST_ASSERT_SUCCESS(status);
        if ((foundMatch && expectedMatchStart<0) ||
            (foundStart != expectedMatchStart)   ||
            (foundLimit != expectedMatchLimit)) {
                TEST_ASSERT(FALSE);   //  ouput generic error position
                infoln("Found, expected match start = %d, %d \n"
                       "Found, expected match limit = %d, %d",
                foundStart, expectedMatchStart, foundLimit, expectedMatchLimit);
        }

        // In case there are other matches...
        // (should we only do this if the test case passed?)
        while (foundMatch) {
            expectedMatchStart = foundStart;
            expectedMatchLimit = foundLimit;

            foundMatch = usearch_search(uss.getAlias(), foundLimit, &foundStart, &foundLimit, &status);
        }

        uss.adoptInstead(usearch_openFromCollator(pattern.getBuffer(), pattern.length(),
            target.getBuffer(), target.length(),
            collator.getAlias(),
            NULL,
            &status));

        //
        // Do the backwards search, check the match result against the expected results.
        //
        foundMatch= usearch_searchBackwards(uss.getAlias(), target.length(), &foundStart, &foundLimit, &status);
        TEST_ASSERT_SUCCESS(status);
        if ((foundMatch && expectedMatchStart<0) ||
            (foundStart != expectedMatchStart)   ||
            (foundLimit != expectedMatchLimit)) {
                TEST_ASSERT(FALSE);   //  ouput generic error position
                infoln("Found, expected backwards match start = %d, %d \n"
                       "Found, expected backwards match limit = %d, %d",
                foundStart, expectedMatchStart, foundLimit, expectedMatchLimit);
        }
    }
#endif
}

struct UdhrTestCase
{
    const char *locale;
    const char *file;
};

void SSearchTest::udhrTest()
{
    UErrorCode status = U_ZERO_ERROR;
    char path[PATH_BUFFER_SIZE];
    const char *udhrPath = getPath(path, "udhr");

    if (udhrPath == NULL) {
        // couldn't get path: error message already output...
        return;
    }

    UdhrTestCase testCases[] = {
        {"en", "udhr_eng.txt"},
        {"de", "udhr_deu_1996.txt"},
        {"fr", "udhr_fra.txt"},
        {"ru", "udhr_rus.txt"},
        {"th", "udhr_tha.txt"},
        {"ja", "udhr_jpn.txt"},
        {"ko", "udhr_kor.txt"},
        {"zh", "udhr_cmn_hans.txt"},
        {"zh_Hant", "udhr_cmn_hant.txt"}
    };

    int32_t testCount = ARRAY_SIZE(testCases);

    for (int32_t t = 0; t < testCount; t += 1) {
        int32_t len = 0;
        char *resolvedFileName = NULL;
        const char *encoding = NULL;
        UCHARBUF *ucharBuf = NULL;

        ucbuf_resolveFileName(udhrPath, testCases[t].file, NULL, &len, &status);
        resolvedFileName = NEW_ARRAY(char, len);

        if(resolvedFileName == NULL){
            continue;
        }

        if(status == U_BUFFER_OVERFLOW_ERROR){
            status = U_ZERO_ERROR;
        }

        ucbuf_resolveFileName(udhrPath, testCases[t].file, resolvedFileName, &len, &status);
        ucharBuf = ucbuf_open(resolvedFileName, &encoding, TRUE, FALSE, &status);

        DELETE_ARRAY(resolvedFileName);

        if(U_FAILURE(status)){
            infoln("Could not open the input file %s. Test skipped\n", testCases[t].file);
            continue;
        }

        int32_t targetLen = 0;
        const UChar *target = ucbuf_getBuffer(ucharBuf, &targetLen, &status);

        /* The first line of the file contains the pattern */
        int32_t start = 0, end = 0, plen = 0;

        for(end = start; ; end += 1) {
            UChar ch = target[end];

            if (ch == 0x000A || ch == 0x000D || ch == 0x2028) {
                break;
            }
        }

        plen = end - start;

        UChar *pattern = NEW_ARRAY(UChar, plen);
        for (int32_t i = 0; i < plen; i += 1) {
            pattern[i] =  target[start++];
        }

        int32_t offset = 0;
        UCollator *coll = ucol_open(testCases[t].locale, &status);
        UCD *ucd = NULL;
        BMS *bms = NULL;

        if (U_FAILURE(status)) {
            errln("Could not open collator for %s", testCases[t].locale);
            goto delete_collator;
        }

        ucd = ucd_open(coll, &status);

        if (U_FAILURE(status)) {
            errln("Could not open CollData object for %s", testCases[t].locale);
            goto delete_ucd;
        }

        bms = bms_open(ucd, pattern, plen, target, targetLen, &status);

        if (U_FAILURE(status)) {
            errln("Could not open search object for %s", testCases[t].locale);
            goto delete_bms;
        }

        start = end = -1;
        while (bms_search(bms, offset, &start, &end)) {
            offset = end;
        }

        if (offset == 0) {
            errln("Could not find pattern - locale: %s, file: %s ", testCases[t].locale, testCases[t].file);
        }

delete_bms:
        bms_close(bms);

delete_ucd:
        ucd_close(ucd);

delete_collator:
        ucol_close(coll);

        DELETE_ARRAY(pattern);
        ucbuf_close(ucharBuf);
    }

    ucd_flushCache();
}

void SSearchTest::bmSearchTest()
{
#if !UCONFIG_NO_REGULAR_EXPRESSIONS
    UErrorCode status = U_ZERO_ERROR;
    char path[PATH_BUFFER_SIZE];
    const char *testFilePath = getPath(path, "ssearch.xml");

    if (testFilePath == NULL) {
        return; /* Couldn't get path: error message already output. */
    }

    UXMLParser  *parser = UXMLParser::createParser(status);
    TEST_ASSERT_SUCCESS(status);
    UXMLElement *root   = parser->parseFile(testFilePath, status);
    TEST_ASSERT_SUCCESS(status);
    if (U_FAILURE(status)) {
        return;
    }

    const UnicodeString *debugTestCase = root->getAttribute("debug");
    if (debugTestCase != NULL) {
//       setenv("USEARCH_DEBUG", "1", 1);
    }


    const UXMLElement *testCase;
    int32_t tc = 0;

    while((testCase = root->nextChildElement(tc)) != NULL) {

        if (testCase->getTagName().compare("test-case") != 0) {
            errln("ssearch, unrecognized XML Element in test file");
            continue;
        }
        const UnicodeString *id       = testCase->getAttribute("id");
        *testId = 0;
        if (id != NULL) {
            id->extract(0, id->length(), testId,  sizeof(testId), US_INV);
        }

        // If debugging test case has been specified and this is not it, skip to next.
        if (id!=NULL && debugTestCase!=NULL && *id != *debugTestCase) {
            continue;
        }
        //
        //  Get the requested collation strength.
        //    Default is tertiary if the XML attribute is missing from the test case.
        //
        const UnicodeString *strength = testCase->getAttribute("strength");
        UColAttributeValue collatorStrength = UCOL_PRIMARY;
        if      (strength==NULL)          { collatorStrength = UCOL_TERTIARY;}
        else if (*strength=="PRIMARY")    { collatorStrength = UCOL_PRIMARY;}
        else if (*strength=="SECONDARY")  { collatorStrength = UCOL_SECONDARY;}
        else if (*strength=="TERTIARY")   { collatorStrength = UCOL_TERTIARY;}
        else if (*strength=="QUATERNARY") { collatorStrength = UCOL_QUATERNARY;}
        else if (*strength=="IDENTICAL")  { collatorStrength = UCOL_IDENTICAL;}
        else {
            // Bogus value supplied for strength.  Shouldn't happen, even from
            //  typos, if the  XML source has been validated.
            //  This assert is a little deceiving in that strength can be
            //   any of the allowed values, not just TERTIARY, but it will
            //   do the job of getting the error output.
            TEST_ASSERT(*strength=="TERTIARY")
        }

        //
        // Get the collator normalization flag.  Default is UCOL_OFF.
        //
        UColAttributeValue normalize = UCOL_OFF;
        const UnicodeString *norm = testCase->getAttribute("norm");
        TEST_ASSERT (norm==NULL || *norm=="ON" || *norm=="OFF");
        if (norm!=NULL && *norm=="ON") {
            normalize = UCOL_ON;
        }

        //
        // Get the alternate_handling flag. Default is UCOL_NON_IGNORABLE.
        //
        UColAttributeValue alternateHandling = UCOL_NON_IGNORABLE;
        const UnicodeString *alt = testCase->getAttribute("alternate_handling");
        TEST_ASSERT (alt == NULL || *alt == "SHIFTED" || *alt == "NON_IGNORABLE");
        if (alt != NULL && *alt == "SHIFTED") {
            alternateHandling = UCOL_SHIFTED;
        }

        const UnicodeString defLocale("en");
        char  clocale[100];
        const UnicodeString *locale   = testCase->getAttribute("locale");
        if (locale == NULL || locale->length()==0) {
            locale = &defLocale;
        };
        locale->extract(0, locale->length(), clocale, sizeof(clocale), NULL);


        UnicodeString  text;
        UnicodeString  target;
        UnicodeString  pattern;
        int32_t        expectedMatchStart = -1;
        int32_t        expectedMatchLimit = -1;
        const UXMLElement  *n;
        int32_t                nodeCount = 0;

        n = testCase->getChildElement("pattern");
        TEST_ASSERT(n != NULL);
        if (n==NULL) {
            continue;
        }
        text = n->getText(FALSE);
        text = text.unescape();
        pattern.append(text);
        nodeCount++;

        n = testCase->getChildElement("pre");
        if (n!=NULL) {
            text = n->getText(FALSE);
            text = text.unescape();
            target.append(text);
            nodeCount++;
        }

        n = testCase->getChildElement("m");
        if (n!=NULL) {
            expectedMatchStart = target.length();
            text = n->getText(FALSE);
            text = text.unescape();
            target.append(text);
            expectedMatchLimit = target.length();
            nodeCount++;
        }

        n = testCase->getChildElement("post");
        if (n!=NULL) {
            text = n->getText(FALSE);
            text = text.unescape();
            target.append(text);
            nodeCount++;
        }

        //  Check that there weren't extra things in the XML
        TEST_ASSERT(nodeCount == testCase->countChildren());

        // Open a collator and StringSearch based on the parameters
        //   obtained from the XML.
        //
        status = U_ZERO_ERROR;
        UCollator *collator = ucol_open(clocale, &status);
        ucol_setStrength(collator, collatorStrength);
        ucol_setAttribute(collator, UCOL_NORMALIZATION_MODE, normalize, &status);
        ucol_setAttribute(collator, UCOL_ALTERNATE_HANDLING, alternateHandling, &status);
        UCD *ucd = ucd_open(collator, &status);
        BMS *bms = bms_open(ucd, pattern.getBuffer(), pattern.length(), target.getBuffer(), target.length(), &status);

        TEST_ASSERT_SUCCESS(status);
        if (U_FAILURE(status)) {
            bms_close(bms);
            ucd_close(ucd);
            ucol_close(collator);
            continue;
        }

        int32_t foundStart = 0;
        int32_t foundLimit = 0;
        UBool   foundMatch;

        //
        // Do the search, check the match result against the expected results.
        //
        foundMatch = bms_search(bms, 0, &foundStart, &foundLimit);
      //TEST_ASSERT_SUCCESS(status);
        if ((foundMatch && expectedMatchStart < 0) ||
            (foundStart != expectedMatchStart)     ||
            (foundLimit != expectedMatchLimit)) {
                TEST_ASSERT(FALSE);   //  ouput generic error position
                infoln("Found, expected match start = %d, %d \n"
                       "Found, expected match limit = %d, %d",
                foundStart, expectedMatchStart, foundLimit, expectedMatchLimit);
        }

        bms_close(bms);
        ucd_close(ucd);
        ucol_close(collator);
    }

    ucd_flushCache();
    delete root;
    delete parser;
#endif
}

struct Order
{
    int32_t order;
    int32_t lowOffset;
    int32_t highOffset;
};

class OrderList
{
public:
    OrderList();
    OrderList(UCollator *coll, const UnicodeString &string, int32_t stringOffset = 0);
    ~OrderList();

    int32_t size(void) const;
    void add(int32_t order, int32_t low, int32_t high);
    const Order *get(int32_t index) const;
    int32_t getLowOffset(int32_t index) const;
    int32_t getHighOffset(int32_t index) const;
    int32_t getOrder(int32_t index) const;
    void reverse(void);
    UBool compare(const OrderList &other) const;
    UBool matchesAt(int32_t offset, const OrderList &other) const;

private:
    Order *list;
    int32_t listMax;
    int32_t listSize;
};

OrderList::OrderList()
  : list(NULL),  listMax(16), listSize(0)
{
    list = new Order[listMax];
}

OrderList::OrderList(UCollator *coll, const UnicodeString &string, int32_t stringOffset)
    : list(NULL), listMax(16), listSize(0)
{
    UErrorCode status = U_ZERO_ERROR;
    UCollationElements *elems = ucol_openElements(coll, string.getBuffer(), string.length(), &status);
    uint32_t strengthMask = 0;
    int32_t order, low, high;

    switch (ucol_getStrength(coll))
    {
    default:
        strengthMask |= UCOL_TERTIARYORDERMASK;
        /* fall through */

    case UCOL_SECONDARY:
        strengthMask |= UCOL_SECONDARYORDERMASK;
        /* fall through */

    case UCOL_PRIMARY:
        strengthMask |= UCOL_PRIMARYORDERMASK;
    }

    list = new Order[listMax];

    ucol_setOffset(elems, stringOffset, &status);

    do {
        low   = ucol_getOffset(elems);
        order = ucol_next(elems, &status);
        high  = ucol_getOffset(elems);

        if (order != UCOL_NULLORDER) {
            order &= strengthMask;
        }

        if (order != UCOL_IGNORABLE) {
            add(order, low, high);
        }
    } while (order != UCOL_NULLORDER);

    ucol_closeElements(elems);
}

OrderList::~OrderList()
{
    delete[] list;
}

void OrderList::add(int32_t order, int32_t low, int32_t high)
{
    if (listSize >= listMax) {
        listMax *= 2;

        Order *newList = new Order[listMax];

        uprv_memcpy(newList, list, listSize * sizeof(Order));
        delete[] list;
        list = newList;
    }

    list[listSize].order      = order;
    list[listSize].lowOffset  = low;
    list[listSize].highOffset = high;

    listSize += 1;
}

const Order *OrderList::get(int32_t index) const
{
    if (index >= listSize) {
        return NULL;
    }

    return &list[index];
}

int32_t OrderList::getLowOffset(int32_t index) const
{
    const Order *order = get(index);

    if (order != NULL) {
        return order->lowOffset;
    }

    return -1;
}

int32_t OrderList::getHighOffset(int32_t index) const
{
    const Order *order = get(index);

    if (order != NULL) {
        return order->highOffset;
    }

    return -1;
}

int32_t OrderList::getOrder(int32_t index) const
{
    const Order *order = get(index);

    if (order != NULL) {
        return order->order;
    }

    return UCOL_NULLORDER;
}

int32_t OrderList::size() const
{
    return listSize;
}

void OrderList::reverse()
{
    for(int32_t f = 0, b = listSize - 1; f < b; f += 1, b -= 1) {
        Order swap = list[b];

        list[b] = list[f];
        list[f] = swap;
    }
}

UBool OrderList::compare(const OrderList &other) const
{
    if (listSize != other.listSize) {
        return FALSE;
    }

    for(int32_t i = 0; i < listSize; i += 1) {
        if (list[i].order  != other.list[i].order ||
            list[i].lowOffset != other.list[i].lowOffset ||
            list[i].highOffset != other.list[i].highOffset) {
                return FALSE;
        }
    }

    return TRUE;
}

UBool OrderList::matchesAt(int32_t offset, const OrderList &other) const
{
    // NOTE: sizes include the NULLORDER, which we don't want to compare.
    int32_t otherSize = other.size() - 1;

    if (listSize - 1 - offset < otherSize) {
        return FALSE;
    }

    for (int32_t i = offset, j = 0; j < otherSize; i += 1, j += 1) {
        if (getOrder(i) != other.getOrder(j)) {
            return FALSE;
        }
    }

    return TRUE;
}

static char *printOffsets(char *buffer, OrderList &list)
{
    int32_t size = list.size();
    char *s = buffer;

    for(int32_t i = 0; i < size; i += 1) {
        const Order *order = list.get(i);

        if (i != 0) {
            s += sprintf(s, ", ");
        }

        s += sprintf(s, "(%d, %d)", order->lowOffset, order->highOffset);
    }

    return buffer;
}

static char *printOrders(char *buffer, OrderList &list)
{
    int32_t size = list.size();
    char *s = buffer;

    for(int32_t i = 0; i < size; i += 1) {
        const Order *order = list.get(i);

        if (i != 0) {
            s += sprintf(s, ", ");
        }

        s += sprintf(s, "%8.8X", order->order);
    }

    return buffer;
}

void SSearchTest::offsetTest()
{
    static const UVersionInfo icu47 = { 4, 7, 0, 0 };
    const char *test[] = {
        // The sequence \u0FB3\u0F71\u0F71\u0F80 contains a discontiguous
        // contraction (\u0FB3\u0F71\u0F80) logically followed by \u0F71.
        "\\u1E33\\u0FB3\\u0F71\\u0F71\\u0F80\\uD835\\uDF6C\\u01B0",

        "\\ua191\\u16ef\\u2036\\u017a",

#if 0
        // This results in a complex interaction between contraction,
        // expansion and normalization that confuses the backwards offset fixups.
        "\\u0F7F\\u0F80\\u0F81\\u0F82\\u0F83\\u0F84\\u0F85",
#endif

        "\\u0F80\\u0F81\\u0F82\\u0F83\\u0F84\\u0F85",
        "\\u07E9\\u07EA\\u07F1\\u07F2\\u07F3",

        "\\u02FE\\u02FF"
        "\\u0300\\u0301\\u0302\\u0303\\u0304\\u0305\\u0306\\u0307\\u0308\\u0309\\u030A\\u030B\\u030C\\u030D\\u030E\\u030F"
        "\\u0310\\u0311\\u0312\\u0313\\u0314\\u0315\\u0316\\u0317\\u0318\\u0319\\u031A\\u031B\\u031C\\u031D\\u031E\\u031F"
        "\\u0320\\u0321\\u0322\\u0323\\u0324\\u0325\\u0326\\u0327\\u0328\\u0329\\u032A\\u032B\\u032C\\u032D\\u032E\\u032F"
        "\\u0330\\u0331\\u0332\\u0333\\u0334\\u0335\\u0336\\u0337\\u0338\\u0339\\u033A\\u033B\\u033C\\u033D\\u033E\\u033F"
        "\\u0340\\u0341\\u0342\\u0343\\u0344\\u0345\\u0346\\u0347\\u0348\\u0349\\u034A\\u034B\\u034C\\u034D\\u034E", // currently not working, see #8081

        "\\u02FE\\u02FF\\u0300\\u0301\\u0302\\u0303\\u0316\\u0317\\u0318", // currently not working, see #8081
        "a\\u02FF\\u0301\\u0316", // currently not working, see #8081
        "a\\u02FF\\u0316\\u0301",
        "a\\u0430\\u0301\\u0316",
        "a\\u0430\\u0316\\u0301",
        "abc\\u0E41\\u0301\\u0316",
        "abc\\u0E41\\u0316\\u0301",
        "\\u0E41\\u0301\\u0316",
        "\\u0E41\\u0316\\u0301",
        "a\\u0301\\u0316",
        "a\\u0316\\u0301",
        "\\uAC52\\uAC53",
        "\\u34CA\\u34CB",
        "\\u11ED\\u11EE",
        "\\u30C3\\u30D0",
        "p\\u00E9ch\\u00E9",
        "a\\u0301\\u0325",
        "a\\u0300\\u0325",
        "a\\u0325\\u0300",
        "A\\u0323\\u0300B",
        "A\\u0300\\u0323B",
        "A\\u0301\\u0323B",
        "A\\u0302\\u0301\\u0323B",
        "abc",
        "ab\\u0300c",
        "ab\\u0300\\u0323c",
        " \\uD800\\uDC00\\uDC00",
        "a\\uD800\\uDC00\\uDC00",
        "A\\u0301\\u0301",
        "A\\u0301\\u0323",
        "A\\u0301\\u0323B",
        "B\\u0301\\u0323C",
        "A\\u0300\\u0323B",
        "\\u0301A\\u0301\\u0301",
        "abcd\\r\\u0301",
        "p\\u00EAche",
        "pe\\u0302che",
    };

    int32_t testCount = ARRAY_SIZE(test);
    UErrorCode status = U_ZERO_ERROR;
    RuleBasedCollator *col = (RuleBasedCollator *) Collator::createInstance(Locale::getEnglish(), status);
    if (U_FAILURE(status)) {
        errcheckln(status, "Failed to create collator in offsetTest! - %s", u_errorName(status));
        return;
    }
    char buffer[4096];  // A bit of a hack... just happens to be long enough for all the test cases...
                        // We could allocate one that's the right size by (CE_count * 10) + 2
                        // 10 chars is enough room for 8 hex digits plus ", ". 2 extra chars for "[" and "]"

    col->setAttribute(UCOL_NORMALIZATION_MODE, UCOL_ON, status);

    for(int32_t i = 0; i < testCount; i += 1) {
        if (!isICUVersionAtLeast(icu47) && i>=4 && i<=6) {
            continue; // timebomb until ticket #8080 is resolved
        }
        UnicodeString ts = CharsToUnicodeString(test[i]);
        CollationElementIterator *iter = col->createCollationElementIterator(ts);
        OrderList forwardList;
        OrderList backwardList;
        int32_t order, low, high;

        do {
            low   = iter->getOffset();
            order = iter->next(status);
            high  = iter->getOffset();

            forwardList.add(order, low, high);
        } while (order != CollationElementIterator::NULLORDER);

        iter->reset();
        iter->setOffset(ts.length(), status);

        backwardList.add(CollationElementIterator::NULLORDER, iter->getOffset(), iter->getOffset());

        do {
            high  = iter->getOffset();
            order = iter->previous(status);
            low   = iter->getOffset();

            if (order == CollationElementIterator::NULLORDER) {
                break;
            }

            backwardList.add(order, low, high);
        } while (TRUE);

        backwardList.reverse();

        if (forwardList.compare(backwardList)) {
            logln("Works with \"%s\"", test[i]);
            logln("Forward offsets:  [%s]", printOffsets(buffer, forwardList));
//          logln("Backward offsets: [%s]", printOffsets(buffer, backwardList));

            logln("Forward CEs:  [%s]", printOrders(buffer, forwardList));
//          logln("Backward CEs: [%s]", printOrders(buffer, backwardList));

            logln();
        } else {
            errln("Fails with \"%s\"", test[i]);
            infoln("Forward offsets:  [%s]", printOffsets(buffer, forwardList));
            infoln("Backward offsets: [%s]", printOffsets(buffer, backwardList));

            infoln("Forward CEs:  [%s]", printOrders(buffer, forwardList));
            infoln("Backward CEs: [%s]", printOrders(buffer, backwardList));

            infoln();
        }
        delete iter;
    }
    delete col;
}

#if 0
static UnicodeString &escape(const UnicodeString &string, UnicodeString &buffer)
{
    for(int32_t i = 0; i < string.length(); i += 1) {
        UChar32 ch = string.char32At(i);

        if (ch >= 0x0020 && ch <= 0x007F) {
            if (ch == 0x005C) {
                buffer.append("\\\\");
            } else {
                buffer.append(ch);
            }
        } else {
            char cbuffer[12];

            if (ch <= 0xFFFFL) {
                sprintf(cbuffer, "\\u%4.4X", ch);
            } else {
                sprintf(cbuffer, "\\U%8.8X", ch);
            }

            buffer.append(cbuffer);
        }

        if (ch >= 0x10000L) {
            i += 1;
        }
    }

    return buffer;
}
#endif

#if 1

struct PCE
{
    uint64_t ce;
    int32_t  lowOffset;
    int32_t  highOffset;
};

class PCEList
{
public:
    PCEList(UCollator *coll, const UnicodeString &string);
    ~PCEList();

    int32_t size() const;

    const PCE *get(int32_t index) const;

    int32_t getLowOffset(int32_t index) const;
    int32_t getHighOffset(int32_t index) const;
    uint64_t getOrder(int32_t index) const;

    UBool matchesAt(int32_t offset, const PCEList &other) const;

    uint64_t operator[](int32_t index) const;

private:
    void add(uint64_t ce, int32_t low, int32_t high);

    PCE *list;
    int32_t listMax;
    int32_t listSize;
};

PCEList::PCEList(UCollator *coll, const UnicodeString &string)
{
    UErrorCode status = U_ZERO_ERROR;
    UCollationElements *elems = ucol_openElements(coll, string.getBuffer(), string.length(), &status);
    uint64_t order;
    int32_t low, high;

    list = new PCE[listMax];

    ucol_setOffset(elems, 0, &status);

    do {
        order = ucol_nextProcessed(elems, &low, &high, &status);
        add(order, low, high);
    } while (order != UCOL_PROCESSED_NULLORDER);

    ucol_closeElements(elems);
}

PCEList::~PCEList()
{
    delete[] list;
}

void PCEList::add(uint64_t order, int32_t low, int32_t high)
{
    if (listSize >= listMax) {
        listMax *= 2;

        PCE *newList = new PCE[listMax];

        uprv_memcpy(newList, list, listSize * sizeof(Order));
        delete[] list;
        list = newList;
    }

    list[listSize].ce         = order;
    list[listSize].lowOffset  = low;
    list[listSize].highOffset = high;

    listSize += 1;
}

const PCE *PCEList::get(int32_t index) const
{
    if (index >= listSize) {
        return NULL;
    }

    return &list[index];
}

int32_t PCEList::getLowOffset(int32_t index) const
{
    const PCE *pce = get(index);

    if (pce != NULL) {
        return pce->lowOffset;
    }

    return -1;
}

int32_t PCEList::getHighOffset(int32_t index) const
{
    const PCE *pce = get(index);

    if (pce != NULL) {
        return pce->highOffset;
    }

    return -1;
}

uint64_t PCEList::getOrder(int32_t index) const
{
    const PCE *pce = get(index);

    if (pce != NULL) {
        return pce->ce;
    }

    return UCOL_PROCESSED_NULLORDER;
}

int32_t PCEList::size() const
{
    return listSize;
}

UBool PCEList::matchesAt(int32_t offset, const PCEList &other) const
{
    // NOTE: sizes include the NULLORDER, which we don't want to compare.
    int32_t otherSize = other.size() - 1;

    if (listSize - 1 - offset < otherSize) {
        return FALSE;
    }

    for (int32_t i = offset, j = 0; j < otherSize; i += 1, j += 1) {
        if (getOrder(i) != other.getOrder(j)) {
            return FALSE;
        }
    }

    return TRUE;
}

uint64_t PCEList::operator[](int32_t index) const
{
    return getOrder(index);
}

void SSearchTest::boyerMooreTest()
{
    UErrorCode status = U_ZERO_ERROR;
    UCollator *coll = NULL;
    CollData *data = NULL;
    const CEList* ce = NULL;
    const CEList* ce1 = NULL;
    UnicodeString lp  = "fuss";
    UnicodeString sp = "fu\\u00DF";
    BoyerMooreSearch *longPattern = NULL;
    BoyerMooreSearch *shortPattern = NULL;
    UnicodeString targets[]  = {"fu\\u00DF", "fu\\u00DFball", "1fu\\u00DFball", "12fu\\u00DFball", "123fu\\u00DFball", "1234fu\\u00DFball",
                                "ffu\\u00DF", "fufu\\u00DF", "fusfu\\u00DF",
                                "fuss", "ffuss", "fufuss", "fusfuss", "1fuss", "12fuss", "123fuss", "1234fuss", "fu\\u00DF", "1fu\\u00DF", "12fu\\u00DF", "123fu\\u00DF", "1234fu\\u00DF"};
    int32_t start = -1, end = -1;

    coll = ucol_openFromShortString("LEN_S1", FALSE, NULL, &status);
    if (U_FAILURE(status)) {
        errcheckln(status, "Could not open collator. - %s", u_errorName(status));
        return;
    }

    data = CollData::open(coll, status);
    if (U_FAILURE(status)) {
        errln("Could not open CollData object.");
        goto close_data;
    }

    data->getDynamicClassID();
    if (U_FAILURE(status)) {
        errln("Could not get dynamic class ID of CollData.");
        goto close_patterns;
    }

    data->getStaticClassID();
    if (U_FAILURE(status)) {
        errln("Could not get static class ID of CollData.");
        goto close_patterns;
    }

    longPattern = new BoyerMooreSearch(data, lp.unescape(), NULL, status);
    shortPattern = new BoyerMooreSearch(data, sp.unescape(), NULL, status);
    if (U_FAILURE(status)) {
        errln("Could not create pattern objects.");
        goto close_patterns;
    }

    longPattern->getBadCharacterTable();
    shortPattern->getBadCharacterTable();
    if (U_FAILURE(status)) {
        errln("Could not get bad character table.");
        goto close_patterns;
    }

    longPattern->getGoodSuffixTable();
    shortPattern->getGoodSuffixTable();
    if (U_FAILURE(status)) {
        errln("Could not get good suffix table.");
        goto close_patterns;
    }

    longPattern->getDynamicClassID();
    shortPattern->getDynamicClassID();
    if (U_FAILURE(status)) {
        errln("Could not get dynamic class ID of BoyerMooreSearch.");
        goto close_patterns;
    }

    longPattern->getStaticClassID();
    shortPattern->getStaticClassID();
    if (U_FAILURE(status)) {
        errln("Could not get static class ID of BoyerMooreSearch.");
        goto close_patterns;
    }

    longPattern->getData();
    shortPattern->getData();
    if (U_FAILURE(status)) {
        errln("Could not get collate data.");
        goto close_patterns;
    }

    ce = longPattern->getPatternCEs();
    ce1 = shortPattern->getPatternCEs();
    if (U_FAILURE(status)) {
        errln("Could not get pattern CEs.");
        goto close_patterns;
    }

    ce->getDynamicClassID();
    ce1->getDynamicClassID();
    if (U_FAILURE(status)) {
        errln("Could not get dynamic class ID of CEList.");
        goto close_patterns;
    }

    ce->getStaticClassID();
    ce1->getStaticClassID();
    if (U_FAILURE(status)) {
        errln("Could not get static class ID of CEList.");
        goto close_patterns;
    }

    if(data->minLengthInChars(ce,0) != 3){
        errln("Minimal Length in Characters for 'data' with 'ce' was suppose to give 3.");
        goto close_patterns;
    }

    if(data->minLengthInChars(ce1,0) != 3){
        errln("Minimal Length in Characters for 'data' with 'ce1' was suppose to give 3.");
        goto close_patterns;
    }

    for (uint32_t t = 0; t < (sizeof(targets)/sizeof(targets[0])); t += 1) {
        UnicodeString target = targets[t].unescape();

        longPattern->setTargetString(&target, status);
        if (longPattern->search(0, start, end)) {
            logln("Test %d: found long pattern at [%d, %d].", t, start, end);
        } else {
            errln("Test %d: did not find long pattern.", t);
        }

        shortPattern->setTargetString(&target, status);
        if (shortPattern->search(0, start, end)) {
            logln("Test %d: found short pattern at [%d, %d].", t, start, end);
        } else {
            errln("Test %d: did not find short pattern.", t);
        }

        if(longPattern->empty()){
            errln("Test %d: Long pattern should not have been empty.");
        }

        if(shortPattern->empty()){
            errln("Test %d: Short pattern should not have been empty.");
        }
    }

close_patterns:
    delete shortPattern;
    delete longPattern;

close_data:
    CollData::close(data);
    ucol_close(coll);
}

void SSearchTest::bmsTest()
{
    UErrorCode status = U_ZERO_ERROR;
    UCollator *coll = NULL;
    UCD *data = NULL;
    UnicodeString lp  = "fuss";
    UnicodeString lpu = lp.unescape();
    UnicodeString sp  = "fu\\u00DF";
    UnicodeString spu = sp.unescape();
    BMS *longPattern = NULL;
    BMS *shortPattern = NULL;
    UnicodeString targets[]  = {"fu\\u00DF", "fu\\u00DFball", "1fu\\u00DFball", "12fu\\u00DFball", "123fu\\u00DFball", "1234fu\\u00DFball",
                                "ffu\\u00DF", "fufu\\u00DF", "fusfu\\u00DF",
                                "fuss", "ffuss", "fufuss", "fusfuss", "1fuss", "12fuss", "123fuss", "1234fuss", "fu\\u00DF", "1fu\\u00DF", "12fu\\u00DF", "123fu\\u00DF", "1234fu\\u00DF"};
    int32_t start = -1, end = -1;

    coll = ucol_openFromShortString("LEN_S1", FALSE, NULL, &status);
    if (U_FAILURE(status)) {
        errcheckln(status, "Could not open collator. - %s", u_errorName(status));
        return;
    }

    data = ucd_open(coll, &status);
    if (U_FAILURE(status)) {
        errln("Could not open CollData object.");
        goto close_data;
    }

    longPattern = bms_open(data, lpu.getBuffer(), lpu.length(), NULL, 0, &status);
    shortPattern = bms_open(data, spu.getBuffer(), spu.length(), NULL, 0, &status);
    if (U_FAILURE(status)) {
        errln("Couldn't open pattern objects.");
        goto close_patterns;
    }

    for (uint32_t t = 0; t < (sizeof(targets)/sizeof(targets[0])); t += 1) {
        UnicodeString target = targets[t].unescape();

        bms_setTargetString(longPattern, target.getBuffer(), target.length(), &status);
        if (bms_search(longPattern, 0, &start, &end)) {
            logln("Test %d: found long pattern at [%d, %d].", t, start, end);
        } else {
            errln("Test %d: did not find long pattern.", t);
        }

        bms_setTargetString(shortPattern, target.getBuffer(), target.length(), &status);
        if (bms_search(shortPattern, 0, &start, &end)) {
            logln("Test %d: found short pattern at [%d, %d].", t, start, end);
        } else {
            errln("Test %d: did not find short pattern.", t);
        }
    }

    /* Add better coverage for bms code. */
    if(bms_empty(longPattern)) {
        errln("FAIL: longgPattern is empty.");
    }

    if (!bms_getData(longPattern)) {
        errln("FAIL: bms_getData returned NULL.");
    }

    if (!ucd_getCollator(data)) {
        errln("FAIL: ucd_getCollator returned NULL.");
    }

close_patterns:
    bms_close(shortPattern);
    bms_close(longPattern);

close_data:
    ucd_close(data);
    ucd_freeCache();
    ucol_close(coll);
}

void SSearchTest::goodSuffixTest()
{
    UErrorCode status = U_ZERO_ERROR;
    UCollator *coll = NULL;
    CollData *data = NULL;
    UnicodeString pat = /*"gcagagag"*/ "fxeld";
    UnicodeString target = /*"gcatcgcagagagtatacagtacg"*/ "cloveldfxeld";
    BoyerMooreSearch *pattern = NULL;
    int32_t start = -1, end = -1;

    coll = ucol_open(NULL, &status);
    if (U_FAILURE(status)) {
        errcheckln(status, "Couldn't open collator. - %s", u_errorName(status));
        return;
    }

    data = CollData::open(coll, status);
    if (U_FAILURE(status)) {
        errln("Couldn't open CollData object.");
        goto close_data;
    }

    pattern = new BoyerMooreSearch(data, pat, &target, status);
    if (U_FAILURE(status)) {
        errln("Couldn't open pattern object.");
        goto close_pattern;
    }

    if (pattern->search(0, start, end)) {
        logln("Found pattern at [%d, %d].", start, end);
    } else {
        errln("Did not find pattern.");
    }

close_pattern:
    delete pattern;

close_data:
    CollData::close(data);
    ucol_close(coll);
}

//
//  searchTime()    A quick and dirty performance test for string search.
//                  Probably  doesn't really belong as part of intltest, but it
//                  does check that the search succeeds, and gets the right result,
//                  so it serves as a functionality test also.
//
//                  To run as a perf test, up the loop count, select by commenting
//                  and uncommenting in the code the operation to be measured,
//                  rebuild, and measure the running time of this test alone.
//
//                     time LD_LIBRARY_PATH=whatever  ./intltest  collate/SSearchTest/searchTime
//
void SSearchTest::searchTime() {
    static const char *longishText =
"Whylom, as olde stories tellen us,\n"
"Ther was a duk that highte Theseus:\n"
"Of Athenes he was lord and governour,\n"
"And in his tyme swich a conquerour,\n"
"That gretter was ther noon under the sonne.\n"
"Ful many a riche contree hadde he wonne;\n"
"What with his wisdom and his chivalrye,\n"
"He conquered al the regne of Femenye,\n"
"That whylom was y-cleped Scithia;\n"
"And weddede the quene Ipolita,\n"
"And broghte hir hoom with him in his contree\n"
"With muchel glorie and greet solempnitee,\n"
"And eek hir yonge suster Emelye.\n"
"And thus with victorie and with melodye\n"
"Lete I this noble duk to Athenes ryde,\n"
"And al his hoost, in armes, him bisyde.\n"
"And certes, if it nere to long to here,\n"
"I wolde han told yow fully the manere,\n"
"How wonnen was the regne of Femenye\n"
"By Theseus, and by his chivalrye;\n"
"And of the grete bataille for the nones\n"
"Bitwixen Athen's and Amazones;\n"
"And how asseged was Ipolita,\n"
"The faire hardy quene of Scithia;\n"
"And of the feste that was at hir weddinge,\n"
"And of the tempest at hir hoom-cominge;\n"
"But al that thing I moot as now forbere.\n"
"I have, God woot, a large feeld to ere,\n"
"And wayke been the oxen in my plough.\n"
"The remenant of the tale is long y-nough.\n"
"I wol nat letten eek noon of this route;\n"
"Lat every felawe telle his tale aboute,\n"
"And lat see now who shal the soper winne;\n"
"And ther I lefte, I wol ageyn biginne.\n"
"This duk, of whom I make mencioun,\n"
"When he was come almost unto the toun,\n"
"In al his wele and in his moste pryde,\n"
"He was war, as he caste his eye asyde,\n"
"Wher that ther kneled in the hye weye\n"
"A companye of ladies, tweye and tweye,\n"
"Ech after other, clad in clothes blake; \n"
"But swich a cry and swich a wo they make,\n"
"That in this world nis creature livinge,\n"
"That herde swich another weymentinge;\n"
"And of this cry they nolde never stenten,\n"
"Til they the reynes of his brydel henten.\n"
"'What folk ben ye, that at myn hoomcominge\n"
"Perturben so my feste with cryinge'?\n"
"Quod Theseus, 'have ye so greet envye\n"
"Of myn honour, that thus compleyne and crye? \n"
"Or who hath yow misboden, or offended?\n"
"And telleth me if it may been amended;\n"
"And why that ye ben clothed thus in blak'?\n"
"The eldest lady of hem alle spak,\n"
"When she hadde swowned with a deedly chere,\n"
"That it was routhe for to seen and here,\n"
"And seyde: 'Lord, to whom Fortune hath yiven\n"
"Victorie, and as a conquerour to liven,\n"
"Noght greveth us your glorie and your honour;\n"
"But we biseken mercy and socour.\n"
"Have mercy on our wo and our distresse.\n"
"Som drope of pitee, thurgh thy gentilesse,\n"
"Up-on us wrecched wommen lat thou falle.\n"
"For certes, lord, ther nis noon of us alle,\n"
"That she nath been a duchesse or a quene;\n"
"Now be we caitifs, as it is wel sene:\n"
"Thanked be Fortune, and hir false wheel,\n"
"That noon estat assureth to be weel.\n"
"And certes, lord, t'abyden your presence,\n"
"Here in the temple of the goddesse Clemence\n"
"We han ben waytinge al this fourtenight;\n"
"Now help us, lord, sith it is in thy might.\n"
"I wrecche, which that wepe and waille thus,\n"
"Was whylom wyf to king Capaneus,\n"
"That starf at Thebes, cursed be that day!\n"
"And alle we, that been in this array,\n"
"And maken al this lamentacioun,\n"
"We losten alle our housbondes at that toun,\n"
"Whyl that the sege ther-aboute lay.\n"
"And yet now th'olde Creon, weylaway!\n"
"The lord is now of Thebes the citee, \n"
"Fulfild of ire and of iniquitee,\n"
"He, for despyt, and for his tirannye,\n"
"To do the dede bodyes vileinye,\n"
"Of alle our lordes, whiche that ben slawe,\n"
"Hath alle the bodyes on an heep y-drawe,\n"
"And wol nat suffren hem, by noon assent,\n"
"Neither to been y-buried nor y-brent,\n"
"But maketh houndes ete hem in despyt. zet'\n";

#define TEST_BOYER_MOORE 1
const char *cPattern = "maketh houndes ete hem";
//const char *cPattern = "Whylom";
//const char *cPattern = "zet";
    const char *testId = "searchTime()";   // for error macros.
    UnicodeString target = longishText;
    UErrorCode status = U_ZERO_ERROR;


    LocalUCollatorPointer collator(ucol_open("en", &status));
    CollData *data = CollData::open(collator.getAlias(), status);
    if (U_FAILURE(status) || collator.isNull() || data == NULL) {
        errcheckln(status, "Unable to open UCollator or CollData. - %s", u_errorName(status));
        return;
    } 
    //ucol_setStrength(collator.getAlias(), collatorStrength);
    //ucol_setAttribute(collator.getAlias(), UCOL_NORMALIZATION_MODE, normalize, &status);
    UnicodeString uPattern = cPattern;
#ifndef TEST_BOYER_MOORE
    LocalUStringSearchPointer uss(usearch_openFromCollator(uPattern.getBuffer(), uPattern.length(),
                                                           target.getBuffer(), target.length(),
                                                           collator.getAlias(),
                                                           NULL,     // the break iterator
                                                           &status));
    TEST_ASSERT_SUCCESS(status);
#else
    BoyerMooreSearch bms(data, uPattern, &target, status);
    TEST_ASSERT_SUCCESS(status);
#endif

//  int32_t foundStart;
//  int32_t foundEnd;
    UBool   found;

    // Find the match position usgin strstr
    const char *pm = strstr(longishText, cPattern);
    TEST_ASSERT_M(pm!=NULL, "No pattern match with strstr");
    int32_t  refMatchPos = (int32_t)(pm - longishText);
    int32_t  icuMatchPos;
    int32_t  icuMatchEnd;
#ifndef TEST_BOYER_MOORE
    usearch_search(uss.getAlias(), 0, &icuMatchPos, &icuMatchEnd, &status);
    TEST_ASSERT_SUCCESS(status);
#else
    found = bms.search(0, icuMatchPos, icuMatchEnd);
#endif
    TEST_ASSERT_M(refMatchPos == icuMatchPos, "strstr and icu give different match positions.");

    int32_t i;
    int32_t j=0;

    // Try loopcounts around 100000 to some millions, depending on the operation,
    //   to get runtimes of at least several seconds.
    for (i=0; i<10000; i++) {
#ifndef TEST_BOYER_MOORE
        found = usearch_search(uss.getAlias(), 0, &icuMatchPos, &icuMatchEnd, &status);
#else
        found = bms.search(0, icuMatchPos, icuMatchEnd);
#endif
        //TEST_ASSERT_SUCCESS(status);
        //TEST_ASSERT(found);

        // usearch_setOffset(uss.getAlias(), 0, &status);
        // icuMatchPos = usearch_next(uss.getAlias(), &status);

         // The i+j stuff is to confuse the optimizer and get it to actually leave the
         //   call to strstr in place.
         //pm = strstr(longishText+j, cPattern);
         //j = (j + i)%5;
    }

    printf("%ld, %d\n", pm-longishText, j);
#ifdef TEST_BOYER_MOORE
    CollData::close(data);
#endif
}
#endif

//----------------------------------------------------------------------------------------
//
//   Random Numbers.  Similar to standard lib rand() and srand()
//                    Not using library to
//                      1.  Get same results on all platforms.
//                      2.  Get access to current seed, to more easily reproduce failures.
//
//---------------------------------------------------------------------------------------
static uint32_t m_seed = 1;

static uint32_t m_rand()
{
    m_seed = m_seed * 1103515245 + 12345;
    return (uint32_t)(m_seed/65536) % 32768;
}

class Monkey
{
public:
    virtual void append(UnicodeString &test, UnicodeString &alternate) = 0;

protected:
    Monkey();
    virtual ~Monkey();
};

Monkey::Monkey()
{
    // ook?
}

Monkey::~Monkey()
{
    // ook?
}

class SetMonkey : public Monkey
{
public:
    SetMonkey(const USet *theSet);
    ~SetMonkey();

    virtual void append(UnicodeString &test, UnicodeString &alternate);

private:
    const USet *set;
};

SetMonkey::SetMonkey(const USet *theSet)
    : Monkey(), set(theSet)
{
    // ook?
}

SetMonkey::~SetMonkey()
{
    //ook...
}

void SetMonkey::append(UnicodeString &test, UnicodeString &alternate)
{
    int32_t size = uset_size(set);
    int32_t index = m_rand() % size;
    UChar32 ch = uset_charAt(set, index);
    UnicodeString str(ch);

    test.append(str);
    alternate.append(str); // flip case, or some junk?
}

class StringSetMonkey : public Monkey
{
public:
    StringSetMonkey(const USet *theSet, UCollator *theCollator, CollData *theCollData);
    ~StringSetMonkey();

    void append(UnicodeString &testCase, UnicodeString &alternate);

private:
    UnicodeString &generateAlternative(const UnicodeString &testCase, UnicodeString &alternate);

    const USet *set;
    UCollator  *coll;
    CollData   *collData;
};

StringSetMonkey::StringSetMonkey(const USet *theSet, UCollator *theCollator, CollData *theCollData)
: Monkey(), set(theSet), coll(theCollator), collData(theCollData)
{
    // ook.
}

StringSetMonkey::~StringSetMonkey()
{
    // ook?
}

void StringSetMonkey::append(UnicodeString &testCase, UnicodeString &alternate)
{
    int32_t itemCount = uset_getItemCount(set), len = 0;
    int32_t index = m_rand() % itemCount;
    UChar32 rangeStart = 0, rangeEnd = 0;
    UChar buffer[16];
    UErrorCode err = U_ZERO_ERROR;

    len = uset_getItem(set, index, &rangeStart, &rangeEnd, buffer, 16, &err);

    if (len == 0) {
        int32_t offset = m_rand() % (rangeEnd - rangeStart + 1);
        UChar32 ch = rangeStart + offset;
        UnicodeString str(ch);

        testCase.append(str);
        generateAlternative(str, alternate);
    } else if (len > 0) {
        // should check that len < 16...
        UnicodeString str(buffer, len);

        testCase.append(str);
        generateAlternative(str, alternate);
    } else {
        // shouldn't happen...
    }
}

UnicodeString &StringSetMonkey::generateAlternative(const UnicodeString &testCase, UnicodeString &alternate)
{
    // find out shortest string for the longest sequence of ces.
    // needs to be refined to use dynamic programming, but will be roughly right
    UErrorCode status = U_ZERO_ERROR;
    CEList ceList(coll, testCase, status);
    UnicodeString alt;
    int32_t offset = 0;

    if (ceList.size() == 0) {
        return alternate.append(testCase);
    }

    while (offset < ceList.size()) {
        int32_t ce = ceList.get(offset);
        const StringList *strings = collData->getStringList(ce);

        if (strings == NULL) {
            return alternate.append(testCase);
        }

        int32_t stringCount = strings->size();
        int32_t tries = 0;

        // find random string that generates the same CEList
        const CEList *ceList2 = NULL;
        const UnicodeString *string = NULL;
              UBool matches = FALSE;

        do {
            int32_t s = m_rand() % stringCount;

            if (tries++ > stringCount) {
                alternate.append(testCase);
                return alternate;
            }

            string = strings->get(s);
            ceList2 = collData->getCEList(string);
            matches = ceList.matchesAt(offset, ceList2);

            if (! matches) {
                collData->freeCEList((CEList *) ceList2);
            }
        } while (! matches);

        alt.append(*string);
        offset += ceList2->size();
        collData->freeCEList(ceList2);
    }

    const CEList altCEs(coll, alt, status);

    if (ceList.matchesAt(0, &altCEs)) {
        return alternate.append(alt);
    }

    return alternate.append(testCase);
}

static void generateTestCase(UCollator *coll, Monkey *monkeys[], int32_t monkeyCount, UnicodeString &testCase, UnicodeString &alternate)
{
    int32_t pieces = (m_rand() % 4) + 1;
    UErrorCode status = U_ZERO_ERROR;
    UBool matches;

    do {
        testCase.remove();
        alternate.remove();
        monkeys[0]->append(testCase, alternate);

        for(int32_t piece = 0; piece < pieces; piece += 1) {
            int32_t monkey = m_rand() % monkeyCount;

            monkeys[monkey]->append(testCase, alternate);
        }

        const CEList ceTest(coll, testCase, status);
        const CEList ceAlt(coll, alternate, status);

        matches = ceTest.matchesAt(0, &ceAlt);
    } while (! matches);
}

//
//  Find the next acceptable boundary following the specified starting index
//     in the target text being searched.
//      TODO:  refine what is an acceptable boundary.  For the moment,
//             choose the next position not within a combining sequence.
//
#if 0
static int32_t nextBoundaryAfter(const UnicodeString &string, int32_t startIndex) {
    const UChar *text = string.getBuffer();
    int32_t textLen   = string.length();

    if (startIndex >= textLen) {
        return startIndex;
    }

    UChar32  c;
    int32_t  i = startIndex;

    U16_NEXT(text, i, textLen, c);

    // If we are on a control character, stop without looking for combining marks.
    //    Control characters do not combine.
    int32_t gcProperty = u_getIntPropertyValue(c, UCHAR_GRAPHEME_CLUSTER_BREAK);
    if (gcProperty==U_GCB_CONTROL || gcProperty==U_GCB_LF || gcProperty==U_GCB_CR) {
        return i;
    }

    // The initial character was not a control, and can thus accept trailing
    //   combining characters.  Advance over however many of them there are.
    int32_t  indexOfLastCharChecked;

    for (;;) {
        indexOfLastCharChecked = i;

        if (i>=textLen) {
            break;
        }

        U16_NEXT(text, i, textLen, c);
        gcProperty = u_getIntPropertyValue(c, UCHAR_GRAPHEME_CLUSTER_BREAK);

        if (gcProperty != U_GCB_EXTEND && gcProperty != U_GCB_SPACING_MARK) {
            break;
        }
    }

    return indexOfLastCharChecked;
}
#endif

#if 0
static UBool isInCombiningSequence(const UnicodeString &string, int32_t index) {
    const UChar *text = string.getBuffer();
    int32_t textLen   = string.length();

    if (index>=textLen || index<=0) {
        return FALSE;
    }

    // If the character at the current index is not a GRAPHEME_EXTEND
    //    then we can not be within a combining sequence.
    UChar32  c;
    U16_GET(text, 0, index, textLen, c);
    int32_t gcProperty = u_getIntPropertyValue(c, UCHAR_GRAPHEME_CLUSTER_BREAK);
    if (gcProperty != U_GCB_EXTEND && gcProperty != U_GCB_SPACING_MARK) {
        return FALSE;
    }

    // We are at a combining mark.  If the preceding character is anything
    //   except a CONTROL, CR or LF, we are in a combining sequence.
    U16_PREV(text, 0, index, c);
    gcProperty = u_getIntPropertyValue(c, UCHAR_GRAPHEME_CLUSTER_BREAK);

    return !(gcProperty==U_GCB_CONTROL || gcProperty==U_GCB_LF || gcProperty==U_GCB_CR);
}
#endif

static UBool simpleSearch(UCollator *coll, const UnicodeString &target, int32_t offset, const UnicodeString &pattern, int32_t &matchStart, int32_t &matchEnd)
{
    UErrorCode      status = U_ZERO_ERROR;
    OrderList       targetOrders(coll, target, offset);
    OrderList       patternOrders(coll, pattern);
    int32_t         targetSize  = targetOrders.size() - 1;
    int32_t         patternSize = patternOrders.size() - 1;
    UBreakIterator *charBreakIterator = ubrk_open(UBRK_CHARACTER, ucol_getLocaleByType(coll, ULOC_VALID_LOCALE, &status),
                                                  target.getBuffer(), target.length(), &status);

    if (patternSize == 0) {
        // Searching for an empty pattern always fails
        matchStart = matchEnd = -1;
        ubrk_close(charBreakIterator);
        return FALSE;
    }

    matchStart = matchEnd = -1;

    for(int32_t i = 0; i < targetSize; i += 1) {
        if (targetOrders.matchesAt(i, patternOrders)) {
            int32_t start    = targetOrders.getLowOffset(i);
            int32_t maxLimit = targetOrders.getLowOffset(i + patternSize);
            int32_t minLimit = targetOrders.getLowOffset(i + patternSize - 1);

            // if the low and high offsets of the first CE in
            // the match are the same, it means that the match
            // starts in the middle of an expansion - all but
            // the first CE of the expansion will have the offset
            // of the following character.
            if (start == targetOrders.getHighOffset(i)) {
                continue;
            }

            // Make sure match starts on a grapheme boundary
            if (! ubrk_isBoundary(charBreakIterator, start)) {
                continue;
            }

            // If the low and high offsets of the CE after the match
            // are the same, it means that the match ends in the middle
            // of an expansion sequence.
            if (maxLimit == targetOrders.getHighOffset(i + patternSize) &&
                targetOrders.getOrder(i + patternSize) != UCOL_NULLORDER) {
                continue;
            }

            int32_t mend = maxLimit;

            // Find the first grapheme break after the character index
            // of the last CE in the match. If it's after character index
            // that's after the last CE in the match, use that index
            // as the end of the match.
            if (minLimit < maxLimit) {
                int32_t nba = ubrk_following(charBreakIterator, minLimit);

                if (nba >= targetOrders.getHighOffset(i + patternSize - 1)) {
                    mend = nba;
                }
            }

            if (mend > maxLimit) {
                continue;
            }

            if (! ubrk_isBoundary(charBreakIterator, mend)) {
                continue;
            }

            matchStart = start;
            matchEnd   = mend;

            ubrk_close(charBreakIterator);
            return TRUE;
        }
    }

    ubrk_close(charBreakIterator);
    return FALSE;
}

#if !UCONFIG_NO_REGULAR_EXPRESSIONS
static int32_t  getIntParam(UnicodeString name, UnicodeString &params, int32_t defaultVal) {
    int32_t val = defaultVal;

    name.append(" *= *(-?\\d+)");

    UErrorCode status = U_ZERO_ERROR;
    RegexMatcher m(name, params, 0, status);

    if (m.find()) {
        // The param exists.  Convert the string to an int.
        char valString[100];
        int32_t paramLength = m.end(1, status) - m.start(1, status);

        if (paramLength >= (int32_t)(sizeof(valString)-1)) {
            paramLength = (int32_t)(sizeof(valString)-2);
        }

        params.extract(m.start(1, status), paramLength, valString, sizeof(valString));
        val = strtol(valString,  NULL, 10);

        // Delete this parameter from the params string.
        m.reset();
        params = m.replaceFirst("", status);
    }

  //U_ASSERT(U_SUCCESS(status));
    if (! U_SUCCESS(status)) {
        val = defaultVal;
    }

    return val;
}
#endif

#if !UCONFIG_NO_COLLATION
int32_t SSearchTest::monkeyTestCase(UCollator *coll, const UnicodeString &testCase, const UnicodeString &pattern, const UnicodeString &altPattern,
                                    const char *name, const char *strength, uint32_t seed)
{
    UErrorCode status = U_ZERO_ERROR;
    int32_t actualStart = -1, actualEnd = -1;
  //int32_t expectedStart = prefix.length(), expectedEnd = prefix.length() + altPattern.length();
    int32_t expectedStart = -1, expectedEnd = -1;
    int32_t notFoundCount = 0;
    LocalUStringSearchPointer uss(usearch_openFromCollator(pattern.getBuffer(), pattern.length(),
                                                           testCase.getBuffer(), testCase.length(),
                                                           coll,
                                                           NULL,     // the break iterator
                                                           &status));

    // **** TODO: find *all* matches, not just first one ****
    simpleSearch(coll, testCase, 0, pattern, expectedStart, expectedEnd);

    usearch_search(uss.getAlias(), 0, &actualStart, &actualEnd, &status);

    if (expectedStart >= 0 && (actualStart != expectedStart || actualEnd != expectedEnd)) {
        errln("Search for <pattern> in <%s> failed: expected [%d, %d], got [%d, %d]\n"
              "    strength=%s seed=%d",
              name, expectedStart, expectedEnd, actualStart, actualEnd, strength, seed);
    }

    if (expectedStart == -1 && actualStart == -1) {
        notFoundCount += 1;
    }

    // **** TODO: find *all* matches, not just first one ****
    simpleSearch(coll, testCase, 0, altPattern, expectedStart, expectedEnd);

    usearch_setPattern(uss.getAlias(), altPattern.getBuffer(), altPattern.length(), &status);

    usearch_search(uss.getAlias(), 0, &actualStart, &actualEnd, &status);

    if (expectedStart >= 0 && (actualStart != expectedStart || actualEnd != expectedEnd)) {
        errln("Search for <alt_pattern> in <%s> failed: expected [%d, %d], got [%d, %d]\n"
              "    strength=%s seed=%d",
              name, expectedStart, expectedEnd, actualStart, actualEnd, strength, seed);
    }

    if (expectedStart == -1 && actualStart == -1) {
        notFoundCount += 1;
    }

    return notFoundCount;
}

static void hexForUnicodeString(const UnicodeString &ustr, char * cbuf, int32_t cbuflen)
{
    int32_t ustri, ustrlen = ustr.length();

    for (ustri = 0; ustri < ustrlen; ++ustri) {
        if (cbuflen >= 9 /* format width for single code unit(5) + terminating ellipsis(3) + null(1) */) {
            int len = sprintf(cbuf, " %04X", ustr.charAt(ustri));
            cbuflen -= len;
            cbuf += len;
        } else {
            if (cbuflen >= 4 /* terminating ellipsis(3) + null(1) */) {
                sprintf(cbuf, "...");
            } else if (cbuflen >= 1) {
                cbuf = 0;
            }
            break;
        }
    }
}

int32_t SSearchTest::bmMonkeyTestCase(UCollator *coll, const UnicodeString &testCase, const UnicodeString &pattern, const UnicodeString &altPattern,
                                    BoyerMooreSearch *bms, BoyerMooreSearch *abms,
                                    const char *name, const char *strength, uint32_t seed)
{
    UErrorCode status = U_ZERO_ERROR;
    int32_t actualStart = -1, actualEnd = -1;
  //int32_t expectedStart = prefix.length(), expectedEnd = prefix.length() + altPattern.length();
    int32_t expectedStart = -1, expectedEnd = -1;
    int32_t notFoundCount = 0;
    char    hexbuf[128];

    // **** TODO: find *all* matches, not just first one ****
    simpleSearch(coll, testCase, 0, pattern, expectedStart, expectedEnd);

    bms->setTargetString(&testCase, status);
    bms->search(0, actualStart, actualEnd);

    if (expectedStart >= 0 && (actualStart != expectedStart || actualEnd != expectedEnd)) {
        hexForUnicodeString(pattern, hexbuf, sizeof(hexbuf));
        errln("Boyer-Moore Search for <pattern> in <%s> failed: expected [%d, %d], got [%d, %d]\n"
              "    strength=%s seed=%d <pattern>: %s",
              name, expectedStart, expectedEnd, actualStart, actualEnd, strength, seed, hexbuf);
    }

    if (expectedStart == -1 && actualStart == -1) {
        notFoundCount += 1;
    }

    // **** TODO: find *all* matches, not just first one ****
    simpleSearch(coll, testCase, 0, altPattern, expectedStart, expectedEnd);

    abms->setTargetString(&testCase, status);
    abms->search(0, actualStart, actualEnd);

    if (expectedStart >= 0 && (actualStart != expectedStart || actualEnd != expectedEnd)) {
        hexForUnicodeString(altPattern, hexbuf, sizeof(hexbuf));
        errln("Boyer-Moore Search for <alt_pattern> in <%s> failed: expected [%d, %d], got [%d, %d]\n"
              "    strength=%s seed=%d <pattern>: %s",
              name, expectedStart, expectedEnd, actualStart, actualEnd, strength, seed, hexbuf);
    }

    if (expectedStart == -1 && actualStart == -1) {
        notFoundCount += 1;
    }


    return notFoundCount;
}
#endif

void SSearchTest::monkeyTest(char *params)
{
    // ook!
    UErrorCode status = U_ZERO_ERROR;
  //UCollator *coll = ucol_open(NULL, &status);
    UCollator *coll = ucol_openFromShortString("S1", FALSE, NULL, &status);

    if (U_FAILURE(status)) {
        errcheckln(status, "Failed to create collator in MonkeyTest! - %s", u_errorName(status));
        return;
    }

    CollData  *monkeyData = CollData::open(coll, status);

    USet *expansions   = uset_openEmpty();
    USet *contractions = uset_openEmpty();

    ucol_getContractionsAndExpansions(coll, contractions, expansions, FALSE, &status);

    U_STRING_DECL(letter_pattern, "[[:letter:]-[:ideographic:]-[:hangul:]]", 39);
    U_STRING_INIT(letter_pattern, "[[:letter:]-[:ideographic:]-[:hangul:]]", 39);
    USet *letters = uset_openPattern(letter_pattern, 39, &status);
    SetMonkey letterMonkey(letters);
    StringSetMonkey contractionMonkey(contractions, coll, monkeyData);
    StringSetMonkey expansionMonkey(expansions, coll, monkeyData);
    UnicodeString testCase;
    UnicodeString alternate;
    UnicodeString pattern, altPattern;
    UnicodeString prefix, altPrefix;
    UnicodeString suffix, altSuffix;

    Monkey *monkeys[] = {
        &letterMonkey,
        &contractionMonkey,
        &expansionMonkey,
        &contractionMonkey,
        &expansionMonkey,
        &contractionMonkey,
        &expansionMonkey,
        &contractionMonkey,
        &expansionMonkey};
    int32_t monkeyCount = sizeof(monkeys) / sizeof(monkeys[0]);
    // int32_t nonMatchCount = 0;

    UCollationStrength strengths[] = {UCOL_PRIMARY, UCOL_SECONDARY, UCOL_TERTIARY};
    const char *strengthNames[] = {"primary", "secondary", "tertiary"};
    int32_t strengthCount = sizeof(strengths) / sizeof(strengths[0]);
    int32_t loopCount = quick? 1000 : 10000;
    int32_t firstStrength = 0;
    int32_t lastStrength  = strengthCount - 1; //*/ 0;

    if (params != NULL) {
#if !UCONFIG_NO_REGULAR_EXPRESSIONS
        UnicodeString p(params);

        loopCount = getIntParam("loop", p, loopCount);
        m_seed    = getIntParam("seed", p, m_seed);

        RegexMatcher m(" *strength *= *(primary|secondary|tertiary) *", p, 0, status);
        if (m.find()) {
            UnicodeString breakType = m.group(1, status);

            for (int32_t s = 0; s < strengthCount; s += 1) {
                if (breakType == strengthNames[s]) {
                    firstStrength = lastStrength = s;
                    break;
                }
            }

            m.reset();
            p = m.replaceFirst("", status);
        }

        if (RegexMatcher("\\S", p, 0, status).find()) {
            // Each option is stripped out of the option string as it is processed.
            // All options have been checked.  The option string should have been completely emptied..
            char buf[100];
            p.extract(buf, sizeof(buf), NULL, status);
            buf[sizeof(buf)-1] = 0;
            errln("Unrecognized or extra parameter:  %s\n", buf);
            return;
        }
#else
        infoln("SSearchTest built with UCONFIG_NO_REGULAR_EXPRESSIONS: ignoring parameters.");
#endif
    }

    for(int32_t s = firstStrength; s <= lastStrength; s += 1) {
        int32_t notFoundCount = 0;

        logln("Setting strength to %s.", strengthNames[s]);
        ucol_setStrength(coll, strengths[s]);

        // TODO: try alternate prefix and suffix too?
        // TODO: alterntaes are only equal at primary strength. Is this OK?
        for(int32_t t = 0; t < loopCount; t += 1) {
            uint32_t seed = m_seed;
            // int32_t  nmc = 0;

            generateTestCase(coll, monkeys, monkeyCount, pattern, altPattern);
            generateTestCase(coll, monkeys, monkeyCount, prefix,  altPrefix);
            generateTestCase(coll, monkeys, monkeyCount, suffix,  altSuffix);

            // pattern
            notFoundCount += monkeyTestCase(coll, pattern, pattern, altPattern, "pattern", strengthNames[s], seed);

            testCase.remove();
            testCase.append(prefix);
            testCase.append(/*alt*/pattern);

            // prefix + pattern
            notFoundCount += monkeyTestCase(coll, testCase, pattern, altPattern, "prefix + pattern", strengthNames[s], seed);

            testCase.append(suffix);

            // prefix + pattern + suffix
            notFoundCount += monkeyTestCase(coll, testCase, pattern, altPattern, "prefix + pattern + suffix", strengthNames[s], seed);

            testCase.remove();
            testCase.append(pattern);
            testCase.append(suffix);

            // pattern + suffix
            notFoundCount += monkeyTestCase(coll, testCase, pattern, altPattern, "pattern + suffix", strengthNames[s], seed);
        }

       logln("For strength %s the not found count is %d.", strengthNames[s], notFoundCount);
    }

    uset_close(contractions);
    uset_close(expansions);
    uset_close(letters);

    CollData::close(monkeyData);

    ucol_close(coll);
}

void SSearchTest::bmMonkeyTest(char *params)
{
    static const UVersionInfo icu47 = { 4, 7, 0, 0 }; // for timebomb
    static const UChar skipChars[] = { 0x0E40, 0x0E41, 0x0E42, 0x0E43, 0x0E44, 0xAAB5, 0xAAB6, 0xAAB9, 0xAABB, 0xAABC, 0 }; // for timebomb
    // ook!
    UErrorCode status = U_ZERO_ERROR;
    UCollator *coll = ucol_openFromShortString("LEN_S1", FALSE, NULL, &status);

    if (U_FAILURE(status)) {
        errcheckln(status, "Failed to create collator in MonkeyTest! - %s", u_errorName(status));
        return;
    }

    CollData  *monkeyData = CollData::open(coll, status);

    USet *expansions   = uset_openEmpty();
    USet *contractions = uset_openEmpty();

    ucol_getContractionsAndExpansions(coll, contractions, expansions, FALSE, &status);

    U_STRING_DECL(letter_pattern, "[[:letter:]-[:ideographic:]-[:hangul:]]", 39);
    U_STRING_INIT(letter_pattern, "[[:letter:]-[:ideographic:]-[:hangul:]]", 39);
    USet *letters = uset_openPattern(letter_pattern, 39, &status);
    SetMonkey letterMonkey(letters);
    StringSetMonkey contractionMonkey(contractions, coll, monkeyData);
    StringSetMonkey expansionMonkey(expansions, coll, monkeyData);
    UnicodeString testCase;
    UnicodeString alternate;
    UnicodeString pattern, altPattern;
    UnicodeString prefix, altPrefix;
    UnicodeString suffix, altSuffix;

    Monkey *monkeys[] = {
        &letterMonkey,
        &contractionMonkey,
        &expansionMonkey,
        &contractionMonkey,
        &expansionMonkey,
        &contractionMonkey,
        &expansionMonkey,
        &contractionMonkey,
        &expansionMonkey};
    int32_t monkeyCount = sizeof(monkeys) / sizeof(monkeys[0]);
    // int32_t nonMatchCount = 0;

    UCollationStrength strengths[] = {UCOL_PRIMARY, UCOL_SECONDARY, UCOL_TERTIARY};
    const char *strengthNames[] = {"primary", "secondary", "tertiary"};
    int32_t strengthCount = sizeof(strengths) / sizeof(strengths[0]);
    int32_t loopCount = quick? 1000 : 10000;
    int32_t firstStrength = 0;
    int32_t lastStrength  = strengthCount - 1; //*/ 0;

    if (params != NULL) {
#if !UCONFIG_NO_REGULAR_EXPRESSIONS
        UnicodeString p(params);

        loopCount = getIntParam("loop", p, loopCount);
        m_seed    = getIntParam("seed", p, m_seed);

        RegexMatcher m(" *strength *= *(primary|secondary|tertiary) *", p, 0, status);
        if (m.find()) {
            UnicodeString breakType = m.group(1, status);

            for (int32_t s = 0; s < strengthCount; s += 1) {
                if (breakType == strengthNames[s]) {
                    firstStrength = lastStrength = s;
                    break;
                }
            }

            m.reset();
            p = m.replaceFirst("", status);
        }

        if (RegexMatcher("\\S", p, 0, status).find()) {
            // Each option is stripped out of the option string as it is processed.
            // All options have been checked.  The option string should have been completely emptied..
            char buf[100];
            p.extract(buf, sizeof(buf), NULL, status);
            buf[sizeof(buf)-1] = 0;
            errln("Unrecognized or extra parameter:  %s\n", buf);
            return;
        }
#else
        infoln("SSearchTest built with UCONFIG_NO_REGULAR_EXPRESSIONS: ignoring parameters.");
#endif
    }

    for(int32_t s = firstStrength; s <= lastStrength; s += 1) {
        int32_t notFoundCount = 0;

        logln("Setting strength to %s.", strengthNames[s]);
        ucol_setStrength(coll, strengths[s]);

        CollData *data = CollData::open(coll, status);

        UnicodeString skipString(skipChars); // for timebomb
        UnicodeSet* skipSet = UnicodeSet::createFromAll(skipString); // for timebomb
        // TODO: try alternate prefix and suffix too?
        // TODO: alterntaes are only equal at primary strength. Is this OK?
        for(int32_t t = 0; t < loopCount; t += 1) {
            uint32_t seed = m_seed;
            // int32_t  nmc = 0;

            generateTestCase(coll, monkeys, monkeyCount, pattern, altPattern);
            generateTestCase(coll, monkeys, monkeyCount, prefix,  altPrefix);
            generateTestCase(coll, monkeys, monkeyCount, suffix,  altSuffix);
            
            if (!isICUVersionAtLeast(icu47) && skipSet->containsSome(pattern)) {
                continue; // timebomb until ticket #8080 is resolved
            }

            BoyerMooreSearch pat(data, pattern, NULL, status);
            BoyerMooreSearch alt(data, altPattern, NULL, status);

            // **** need a better way to deal with this ****
#if 0
            if (pat.empty() ||
                alt.empty()) {
                    continue;
            }
#endif

            // pattern
            notFoundCount += bmMonkeyTestCase(coll, pattern, pattern, altPattern, &pat, &alt, "pattern", strengthNames[s], seed);

            testCase.remove();
            testCase.append(prefix);
            testCase.append(/*alt*/pattern);

            // prefix + pattern
            notFoundCount += bmMonkeyTestCase(coll, testCase, pattern, altPattern, &pat, &alt, "prefix + pattern", strengthNames[s], seed);

            testCase.append(suffix);

            // prefix + pattern + suffix
            notFoundCount += bmMonkeyTestCase(coll, testCase, pattern, altPattern, &pat, &alt, "prefix + pattern + suffix", strengthNames[s], seed);

            testCase.remove();
            testCase.append(pattern);
            testCase.append(suffix);

            // pattern + suffix
            notFoundCount += bmMonkeyTestCase(coll, testCase, pattern, altPattern, &pat, &alt, "pattern + suffix", strengthNames[s], seed);
        }
        delete skipSet; // for timebomb

        CollData::close(data);

        logln("For strength %s the not found count is %d.", strengthNames[s], notFoundCount);
    }

    uset_close(contractions);
    uset_close(expansions);
    uset_close(letters);

    CollData::close(monkeyData);

    ucol_close(coll);
}

void SSearchTest::stringListTest(){
    UErrorCode status = U_ZERO_ERROR;
    StringList *sl = new StringList(status);
    if(U_FAILURE(status)){
        errln("ERROR: stringListTest: Could not start StringList");
    }

    const UChar chars[] = {
            0x0000
    };
    sl->add(chars, (int32_t) 0, status);
    if(U_FAILURE(status)){
        errln("ERROR: stringListTest: StringList::add");
    }

    if(sl->getDynamicClassID() != StringList::getStaticClassID()){
        errln("ERROR: stringListTest: getDynamicClassID and getStaticClassID does not match");
    }
    delete sl;
}

#endif

#endif
