/*
*****************************************************************************
* Copyright (C) 2001-2010, International Business Machines orporation  
* and others. All Rights Reserved.
****************************************************************************/

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "srchtest.h"
#if !UCONFIG_NO_BREAK_ITERATION
#include "../cintltst/usrchdat.c"
#endif
#include "unicode/stsearch.h"
#include "unicode/ustring.h"
#include "unicode/schriter.h"
#include <string.h>
#include <stdio.h>

// private definitions -----------------------------------------------------

#define CASE(id,test)                 \
    case id:                          \
        name = #test;                 \
        if (exec) {                   \
            logln(#test "---");       \
            logln((UnicodeString)""); \
            if(areBroken) {           \
                  dataerrln(__FILE__ " cannot test - failed to create collator.");  \
            } else {                  \
                test();               \
            }                         \
        }                             \
        break;

// public contructors and destructors --------------------------------------

StringSearchTest::StringSearchTest() 
#if !UCONFIG_NO_BREAK_ITERATION
:
    m_en_wordbreaker_(NULL), m_en_characterbreaker_(NULL)
#endif
{
#if !UCONFIG_NO_BREAK_ITERATION
    UErrorCode    status = U_ZERO_ERROR;
    
    m_en_us_ = (RuleBasedCollator *)Collator::createInstance("en_US", status);
    m_fr_fr_ = (RuleBasedCollator *)Collator::createInstance("fr_FR", status);
    m_de_    = (RuleBasedCollator *)Collator::createInstance("de_DE", status);
    m_es_    = (RuleBasedCollator *)Collator::createInstance("es_ES", status);
    if(U_FAILURE(status)) {
      delete m_en_us_;
      delete m_fr_fr_;
      delete m_de_;
      delete m_es_;
      m_en_us_ = 0;
      m_fr_fr_ = 0;
      m_de_ = 0;
      m_es_ = 0;
      errln("Collator creation failed with %s", u_errorName(status));
      return;
    }

    
    UnicodeString rules;
    rules.setTo(((RuleBasedCollator *)m_de_)->getRules());
    UChar extrarules[128];
    u_unescape(EXTRACOLLATIONRULE, extrarules, 128);
    rules.append(extrarules, u_strlen(extrarules));
    delete m_de_;

    m_de_ = new RuleBasedCollator(rules, status);

    rules.setTo(((RuleBasedCollator *)m_es_)->getRules());
    rules.append(extrarules, u_strlen(extrarules));
        
    delete m_es_;

    m_es_ = new RuleBasedCollator(rules, status); 
        
#if !UCONFIG_NO_BREAK_ITERATION
    m_en_wordbreaker_      = BreakIterator::createWordInstance(
                                                    Locale::getEnglish(), status);
    m_en_characterbreaker_ = BreakIterator::createCharacterInstance(
                                                    Locale::getEnglish(), status);
#endif
#endif
}

StringSearchTest::~StringSearchTest() 
{
#if !UCONFIG_NO_BREAK_ITERATION
    delete m_en_us_;
    delete m_fr_fr_;
    delete m_de_;
    delete m_es_;
#if !UCONFIG_NO_BREAK_ITERATION
    delete m_en_wordbreaker_;
    delete m_en_characterbreaker_;
#endif
#endif
}

// public methods ----------------------------------------------------------

void StringSearchTest::runIndexedTest(int32_t index, UBool exec, 
                                      const char* &name, char* ) 
{
#if !UCONFIG_NO_BREAK_ITERATION
    UBool areBroken = FALSE;
    if (m_en_us_ == NULL && m_fr_fr_ == NULL && m_de_ == NULL &&
        m_es_ == NULL && m_en_wordbreaker_ == NULL &&
        m_en_characterbreaker_ == NULL && exec) {
        areBroken = TRUE;
    }

    switch (index) {
#if !UCONFIG_NO_FILE_IO
        CASE(0, TestOpenClose)
#endif
        CASE(1, TestInitialization)
        CASE(2, TestBasic)
        CASE(3, TestNormExact)
        CASE(4, TestStrength)
#if UCONFIG_NO_BREAK_ITERATION
    case 5:
        name = "TestBreakIterator";
        break;
#else
        CASE(5, TestBreakIterator)
#endif
        CASE(6, TestVariable)
        CASE(7, TestOverlap)
        CASE(8, TestCollator)
        CASE(9, TestPattern)
        CASE(10, TestText)
        CASE(11, TestCompositeBoundaries)
        CASE(12, TestGetSetOffset)
        CASE(13, TestGetSetAttribute)
        CASE(14, TestGetMatch)
        CASE(15, TestSetMatch)
        CASE(16, TestReset)
        CASE(17, TestSupplementary)
        CASE(18, TestContraction)
        CASE(19, TestIgnorable)
        CASE(20, TestCanonical)
        CASE(21, TestNormCanonical)
        CASE(22, TestStrengthCanonical)
#if UCONFIG_NO_BREAK_ITERATION
    case 23:
        name = "TestBreakIteratorCanonical";
        break;
#else
        CASE(23, TestBreakIteratorCanonical)
#endif
        CASE(24, TestVariableCanonical)
        CASE(25, TestOverlapCanonical)
        CASE(26, TestCollatorCanonical)
        CASE(27, TestPatternCanonical)
        CASE(28, TestTextCanonical)
        CASE(29, TestCompositeBoundariesCanonical)
        CASE(30, TestGetSetOffsetCanonical)
        CASE(31, TestSupplementaryCanonical)
        CASE(32, TestContractionCanonical)
        CASE(33, TestUClassID)
        CASE(34, TestSubclass)
        CASE(35, TestCoverage)
        CASE(36, TestDiacriticMatch)
        default: name = ""; break;
    }
#else
    name="";
#endif
}

#if !UCONFIG_NO_BREAK_ITERATION
// private methods ------------------------------------------------------

RuleBasedCollator * StringSearchTest::getCollator(const char *collator)
{
    if (collator == NULL) {
        return m_en_us_;
    }
    if (strcmp(collator, "fr") == 0) {
        return m_fr_fr_;
    }
    else if (strcmp(collator, "de") == 0) {
        return m_de_;
    }
    else if (strcmp(collator, "es") == 0) {
        return m_es_;
    }
    else {
        return m_en_us_;
    }
}
 
BreakIterator * StringSearchTest::getBreakIterator(const char *breaker)
{
#if UCONFIG_NO_BREAK_ITERATION
    return NULL;
#else
    if (breaker == NULL) {
        return NULL;
    }
    if (strcmp(breaker, "wordbreaker") == 0) {
        return m_en_wordbreaker_;
    }
    else {
        return m_en_characterbreaker_;
    }
#endif
}

char * StringSearchTest::toCharString(const UnicodeString &text)
{
    static char   result[1024];
           int    index  = 0;
           int    count  = 0;
           int    length = text.length();

    for (; count < length; count ++) {
        UChar ch = text[count];
        if (ch >= 0x20 && ch <= 0x7e) {
            result[index ++] = (char)ch;
        }
        else {
            sprintf(result+index, "\\u%04x", ch);
            index += 6; /* \uxxxx */
        }
    }
    result[index] = 0;

    return result;
}

Collator::ECollationStrength StringSearchTest::getECollationStrength(
                                    const UCollationStrength &strength) const
{
  switch (strength)
  {
  case UCOL_PRIMARY :
    return Collator::PRIMARY;
  case UCOL_SECONDARY :
    return Collator::SECONDARY;
  case UCOL_TERTIARY :
    return Collator::TERTIARY;
  default :
    return Collator::IDENTICAL;
  }
}

UBool StringSearchTest::assertEqualWithStringSearch(StringSearch *strsrch,
                                                    const SearchData *search)
{
    int           count       = 0;
    UErrorCode    status      = U_ZERO_ERROR;
    int32_t   matchindex  = search->offset[count];
    UnicodeString matchtext;
    
    strsrch->setAttribute(USEARCH_ELEMENT_COMPARISON, search->elemCompare, status);
    if (U_FAILURE(status)) {
        errln("Error setting USEARCH_ELEMENT_COMPARISON attribute %s", u_errorName(status));
        return FALSE;
    }   

    if (strsrch->getMatchedStart() != USEARCH_DONE ||
        strsrch->getMatchedLength() != 0) {
        errln("Error with the initialization of match start and length");
    }
    // start of following matches 
    while (U_SUCCESS(status) && matchindex >= 0) {
        int32_t matchlength = search->size[count];
        strsrch->next(status);
        if (matchindex != strsrch->getMatchedStart() || 
            matchlength != strsrch->getMatchedLength()) {
            char *str = toCharString(strsrch->getText());
            errln("Text: %s", str);
            str = toCharString(strsrch->getPattern());
            infoln("Pattern: %s", str);
            infoln("Error following match found at idx,len %d,%d; expected %d,%d", 
                    strsrch->getMatchedStart(), strsrch->getMatchedLength(),
                    matchindex, matchlength);
            return FALSE;
        }
        count ++;
        
        strsrch->getMatchedText(matchtext);

        if (U_FAILURE(status) ||
            strsrch->getText().compareBetween(matchindex, 
                                              matchindex + matchlength,
                                              matchtext, 0, 
                                              matchtext.length())) {
            errln("Error getting following matched text");
        }

        matchindex = search->offset[count];
    }
    strsrch->next(status);
    if (strsrch->getMatchedStart() != USEARCH_DONE ||
        strsrch->getMatchedLength() != 0) {
        char *str = toCharString(strsrch->getText());
            errln("Text: %s", str);
            str = toCharString(strsrch->getPattern());
            errln("Pattern: %s", str);
            errln("Error following match found at %d %d", 
                    strsrch->getMatchedStart(), strsrch->getMatchedLength());
            return FALSE;
    }
    // start of preceding matches 
    count = count == 0 ? 0 : count - 1;
    matchindex = search->offset[count];
    while (U_SUCCESS(status) && matchindex >= 0) {
        int32_t matchlength = search->size[count];
        strsrch->previous(status);
        if (matchindex != strsrch->getMatchedStart() || 
            matchlength != strsrch->getMatchedLength()) {
            char *str = toCharString(strsrch->getText());
            errln("Text: %s", str);
            str = toCharString(strsrch->getPattern());
            errln("Pattern: %s", str);
            errln("Error following match found at %d %d", 
                    strsrch->getMatchedStart(), strsrch->getMatchedLength());
            return FALSE;
        }
        
        strsrch->getMatchedText(matchtext);

        if (U_FAILURE(status) ||
            strsrch->getText().compareBetween(matchindex, 
                                              matchindex + matchlength,
                                              matchtext, 0, 
                                              matchtext.length())) {
            errln("Error getting following matched text");
        }

        matchindex = count > 0 ? search->offset[count - 1] : -1;
        count --;
    }
    strsrch->previous(status);
    if (strsrch->getMatchedStart() != USEARCH_DONE ||
        strsrch->getMatchedLength() != 0) {
        char *str = toCharString(strsrch->getText());
            errln("Text: %s", str);
            str = toCharString(strsrch->getPattern());
            errln("Pattern: %s", str);
            errln("Error following match found at %d %d", 
                    strsrch->getMatchedStart(), strsrch->getMatchedLength());
            return FALSE;
    }
    strsrch->setAttribute(USEARCH_ELEMENT_COMPARISON, USEARCH_STANDARD_ELEMENT_COMPARISON, status);
    return TRUE;
}
    
UBool StringSearchTest::assertEqual(const SearchData *search)
{
    UErrorCode     status   = U_ZERO_ERROR;
    
    Collator      *collator = getCollator(search->collator);
    BreakIterator *breaker  = getBreakIterator(search->breaker);
    StringSearch  *strsrch, *strsrch2;
    UChar          temp[128];
    
#if UCONFIG_NO_BREAK_ITERATION
    if(search->breaker) {
      return TRUE; /* skip test */
    }
#endif
    u_unescape(search->text, temp, 128);
    UnicodeString text;
    text.setTo(temp);
    u_unescape(search->pattern, temp, 128);
    UnicodeString  pattern;
    pattern.setTo(temp);

#if !UCONFIG_NO_BREAK_ITERATION
    if (breaker != NULL) {
        breaker->setText(text);
    }
#endif
    collator->setStrength(getECollationStrength(search->strength));
    strsrch = new StringSearch(pattern, text, (RuleBasedCollator *)collator, 
                               breaker, status);
    if (U_FAILURE(status)) {
        errln("Error opening string search %s", u_errorName(status));
        return FALSE;
    }   
    
    if (!assertEqualWithStringSearch(strsrch, search)) {
        collator->setStrength(getECollationStrength(UCOL_TERTIARY));
        delete strsrch;
        return FALSE;
    }


    strsrch2 = strsrch->clone();
    if( strsrch2 == strsrch || *strsrch2 != *strsrch ||
        !assertEqualWithStringSearch(strsrch2, search)
    ) {
        infoln("failure with StringSearch.clone()");
        collator->setStrength(getECollationStrength(UCOL_TERTIARY));
        delete strsrch;
        delete strsrch2;
        return FALSE;
    }
    delete strsrch2;

    collator->setStrength(getECollationStrength(UCOL_TERTIARY));
    delete strsrch;
    return TRUE;
}
 
UBool StringSearchTest::assertCanonicalEqual(const SearchData *search)
{
    UErrorCode     status   = U_ZERO_ERROR;
    Collator      *collator = getCollator(search->collator);
    BreakIterator *breaker  = getBreakIterator(search->breaker);
    StringSearch  *strsrch; 
    UChar          temp[128];
    UBool          result = TRUE;
    
#if UCONFIG_NO_BREAK_ITERATION
    if(search->breaker) {
      return TRUE; /* skip test */
    }
#endif

    u_unescape(search->text, temp, 128);
    UnicodeString text;
    text.setTo(temp);
    u_unescape(search->pattern, temp, 128);
    UnicodeString  pattern;
    pattern.setTo(temp);

#if !UCONFIG_NO_BREAK_ITERATION
    if (breaker != NULL) {
        breaker->setText(text);
    }
#endif
    collator->setStrength(getECollationStrength(search->strength));
    collator->setAttribute(UCOL_NORMALIZATION_MODE, UCOL_ON, status);
    strsrch = new StringSearch(pattern, text, (RuleBasedCollator *)collator, 
                               breaker, status);
    strsrch->setAttribute(USEARCH_CANONICAL_MATCH, USEARCH_ON, status);
    if (U_FAILURE(status)) {
        errln("Error opening string search %s", u_errorName(status));
        result = FALSE;
        goto bail;
    }   
    
    if (!assertEqualWithStringSearch(strsrch, search)) {
        result = FALSE;
        goto bail;
    }

bail:
    collator->setStrength(getECollationStrength(UCOL_TERTIARY));
    collator->setAttribute(UCOL_NORMALIZATION_MODE, UCOL_OFF, status);
    delete strsrch;

    return result;
}
   
UBool StringSearchTest::assertEqualWithAttribute(const SearchData *search, 
                                            USearchAttributeValue canonical,
                                            USearchAttributeValue overlap)
{
    UErrorCode     status   = U_ZERO_ERROR;
    Collator      *collator = getCollator(search->collator);
    BreakIterator *breaker  = getBreakIterator(search->breaker);
    StringSearch  *strsrch; 
    UChar          temp[128];
    

#if UCONFIG_NO_BREAK_ITERATION
    if(search->breaker) {
      return TRUE; /* skip test */
    }
#endif

    u_unescape(search->text, temp, 128);
    UnicodeString text;
    text.setTo(temp);
    u_unescape(search->pattern, temp, 128);
    UnicodeString  pattern;
    pattern.setTo(temp);

#if !UCONFIG_NO_BREAK_ITERATION
    if (breaker != NULL) {
        breaker->setText(text);
    }
#endif
    collator->setStrength(getECollationStrength(search->strength));
    strsrch = new StringSearch(pattern, text, (RuleBasedCollator *)collator, 
                               breaker, status);
    strsrch->setAttribute(USEARCH_CANONICAL_MATCH, canonical, status);
    strsrch->setAttribute(USEARCH_OVERLAP, overlap, status);
    
    if (U_FAILURE(status)) {
        errln("Error opening string search %s", u_errorName(status));
        return FALSE;
    }   
    
    if (!assertEqualWithStringSearch(strsrch, search)) {
        collator->setStrength(getECollationStrength(UCOL_TERTIARY));
        delete strsrch;
        return FALSE;
    }
    collator->setStrength(getECollationStrength(UCOL_TERTIARY));
    delete strsrch;
    return TRUE;
}

void StringSearchTest::TestOpenClose()
{
    UErrorCode               status    = U_ZERO_ERROR;
    StringSearch            *result;
    BreakIterator           *breakiter = m_en_wordbreaker_;
    UnicodeString            pattern;
    UnicodeString            text;
    UnicodeString            temp("a");
    StringCharacterIterator  chariter(text);

    /* testing null arguments */
    result = new StringSearch(pattern, text, NULL, NULL, status);
    if (U_SUCCESS(status)) {
        errln("Error: NULL arguments should produce an error");
    }
    delete result;

    chariter.setText(text);
    status = U_ZERO_ERROR;
    result = new StringSearch(pattern, chariter, NULL, NULL, status);
    if (U_SUCCESS(status)) {
        errln("Error: NULL arguments should produce an error");
    }
    delete result;

    text.append(0, 0x1);
    status = U_ZERO_ERROR;
    result = new StringSearch(pattern, text, NULL, NULL, status);
    if (U_SUCCESS(status)) {
        errln("Error: Empty pattern should produce an error");
    }
    delete result;

    chariter.setText(text);
    status = U_ZERO_ERROR;
    result = new StringSearch(pattern, chariter, NULL, NULL, status);
    if (U_SUCCESS(status)) {
        errln("Error: Empty pattern should produce an error");
    }
    delete result;

    text.remove();
    pattern.append(temp);
    status = U_ZERO_ERROR;
    result = new StringSearch(pattern, text, NULL, NULL, status);
    if (U_SUCCESS(status)) {
        errln("Error: Empty text should produce an error");
    }
    delete result;

    chariter.setText(text);
    status = U_ZERO_ERROR;
    result = new StringSearch(pattern, chariter, NULL, NULL, status);
    if (U_SUCCESS(status)) {
        errln("Error: Empty text should produce an error");
    }
    delete result;

    text.append(temp);
    status = U_ZERO_ERROR;
    result = new StringSearch(pattern, text, NULL, NULL, status);
    if (U_SUCCESS(status)) {
        errln("Error: NULL arguments should produce an error");
    }
    delete result;

    chariter.setText(text);
    status = U_ZERO_ERROR;
    result = new StringSearch(pattern, chariter, NULL, NULL, status);
    if (U_SUCCESS(status)) {
        errln("Error: NULL arguments should produce an error");
    }
    delete result;

    status = U_ZERO_ERROR;
    result = new StringSearch(pattern, text, m_en_us_, NULL, status);
    if (U_FAILURE(status)) {
        errln("Error: NULL break iterator is valid for opening search");
    }
    delete result;

    status = U_ZERO_ERROR;
    result = new StringSearch(pattern, chariter, m_en_us_, NULL, status);
    if (U_FAILURE(status)) {
        errln("Error: NULL break iterator is valid for opening search");
    }
    delete result;

    status = U_ZERO_ERROR;
    result = new StringSearch(pattern, text, Locale::getEnglish(), NULL, status);
    if (U_FAILURE(status) || result == NULL) {
        errln("Error: NULL break iterator is valid for opening search");
    }
    delete result;

    status = U_ZERO_ERROR;
    result = new StringSearch(pattern, chariter, Locale::getEnglish(), NULL, status);
    if (U_FAILURE(status)) {
        errln("Error: NULL break iterator is valid for opening search");
    }
    delete result;

    status = U_ZERO_ERROR;
    result = new StringSearch(pattern, text, m_en_us_, breakiter, status);
    if (U_FAILURE(status)) {
        errln("Error: Break iterator is valid for opening search");
    }
    delete result;

    status = U_ZERO_ERROR;
    result = new StringSearch(pattern, chariter, m_en_us_, NULL, status);
    if (U_FAILURE(status)) {
        errln("Error: Break iterator is valid for opening search");
    }
    delete result;
}
 
void StringSearchTest::TestInitialization()
{
    UErrorCode     status = U_ZERO_ERROR;
    UnicodeString  pattern;
    UnicodeString  text;
    UnicodeString  temp("a");
    StringSearch  *result;
    int count;

    /* simple test on the pattern ce construction */
    pattern.append(temp);
    pattern.append(temp);
    text.append(temp);
    text.append(temp);
    text.append(temp);
    result = new StringSearch(pattern, text, m_en_us_, NULL, status);
    if (U_FAILURE(status)) {
        errln("Error opening search %s", u_errorName(status));
    }
    StringSearch *copy = new StringSearch(*result);
    if (*(copy->getCollator()) != *(result->getCollator()) ||
        copy->getBreakIterator() != result->getBreakIterator() ||
        copy->getMatchedLength() != result->getMatchedLength() ||
        copy->getMatchedStart() != result->getMatchedStart() ||
        copy->getOffset() != result->getOffset() ||
        copy->getPattern() != result->getPattern() ||
        copy->getText() != result->getText() ||
        *(copy) != *(result))
    {
        errln("Error copying StringSearch");
    }
    delete copy;

    copy = (StringSearch *)result->safeClone();
    if (*(copy->getCollator()) != *(result->getCollator()) ||
        copy->getBreakIterator() != result->getBreakIterator() ||
        copy->getMatchedLength() != result->getMatchedLength() ||
        copy->getMatchedStart() != result->getMatchedStart() ||
        copy->getOffset() != result->getOffset() ||
        copy->getPattern() != result->getPattern() ||
        copy->getText() != result->getText() ||
        *(copy) != *(result)) {
        errln("Error copying StringSearch");
    }
    delete result;
    
    /* testing if an extremely large pattern will fail the initialization */
    for (count = 0; count < 512; count ++) {
        pattern.append(temp);
    }
    result = new StringSearch(pattern, text, m_en_us_, NULL, status);
    if (*result != *result) {
        errln("Error: string search object expected to match itself");
    }
    if (*result == *copy) {
        errln("Error: string search objects are not expected to match");
    }
    *copy  = *result;
    if (*(copy->getCollator()) != *(result->getCollator()) ||
        copy->getBreakIterator() != result->getBreakIterator() ||
        copy->getMatchedLength() != result->getMatchedLength() ||
        copy->getMatchedStart() != result->getMatchedStart() ||
        copy->getOffset() != result->getOffset() ||
        copy->getPattern() != result->getPattern() ||
        copy->getText() != result->getText() ||
        *(copy) != *(result)) {
        errln("Error copying StringSearch");
    }
    if (U_FAILURE(status)) {
        errln("Error opening search %s", u_errorName(status));
    }
    delete result;
    delete copy;
}

void StringSearchTest::TestBasic()
{
    int count = 0;
    while (BASIC[count].text != NULL) {
        //printf("count %d", count);
        if (!assertEqual(&BASIC[count])) {
            infoln("Error at test number %d", count);
        }
        count ++;
    }
}

void StringSearchTest::TestNormExact()
{
    int count = 0;
    UErrorCode status = U_ZERO_ERROR;
    m_en_us_->setAttribute(UCOL_NORMALIZATION_MODE, UCOL_ON, status);
    if (U_FAILURE(status)) {
        errln("Error setting collation normalization %s", 
              u_errorName(status));
    }
    while (BASIC[count].text != NULL) {
        if (!assertEqual(&BASIC[count])) {
            infoln("Error at test number %d", count);
        }
        count ++;
    }
    count = 0;
    while (NORMEXACT[count].text != NULL) {
        if (!assertEqual(&NORMEXACT[count])) {
            infoln("Error at test number %d", count);
        }
        count ++;
    }
    m_en_us_->setAttribute(UCOL_NORMALIZATION_MODE, UCOL_OFF, status);
    count = 0;
    while (NONNORMEXACT[count].text != NULL) {
        if (!assertEqual(&NONNORMEXACT[count])) {
            infoln("Error at test number %d", count);
        }
        count ++;
    }
}
 
void StringSearchTest::TestStrength()
{
    int count = 0;
    while (STRENGTH[count].text != NULL) {
        if (!assertEqual(&STRENGTH[count])) {
            infoln("Error at test number %d", count);
        }
        count ++;
    }
}
 
#if !UCONFIG_NO_BREAK_ITERATION

void StringSearchTest::TestBreakIterator()
{
    UChar temp[128];
    u_unescape(BREAKITERATOREXACT[0].text, temp, 128);
    UnicodeString text;
    text.setTo(temp, u_strlen(temp));
    u_unescape(BREAKITERATOREXACT[0].pattern, temp, 128);
    UnicodeString pattern;
    pattern.setTo(temp, u_strlen(temp));

    UErrorCode status = U_ZERO_ERROR;
    StringSearch *strsrch = new StringSearch(pattern, text, m_en_us_, NULL, 
                                             status);
    if (U_FAILURE(status)) {
        errln("Error opening string search %s", u_errorName(status));
    }
    
    strsrch->setBreakIterator(NULL, status);
    if (U_FAILURE(status) || strsrch->getBreakIterator() != NULL) {
        errln("Error usearch_getBreakIterator returned wrong object");
    }

    strsrch->setBreakIterator(m_en_characterbreaker_, status);
    if (U_FAILURE(status) || 
        strsrch->getBreakIterator() != m_en_characterbreaker_) {
        errln("Error usearch_getBreakIterator returned wrong object");
    }
    
    strsrch->setBreakIterator(m_en_wordbreaker_, status);
    if (U_FAILURE(status) || 
        strsrch->getBreakIterator() != m_en_wordbreaker_) {
        errln("Error usearch_getBreakIterator returned wrong object");
    }

    delete strsrch;

    int count = 0;
    while (count < 4) {
        // special purposes for tests numbers 0-3
        const SearchData        *search   = &(BREAKITERATOREXACT[count]);     
              RuleBasedCollator *collator = getCollator(search->collator);
              BreakIterator     *breaker  = getBreakIterator(search->breaker);
              StringSearch      *strsrch; 
    
        u_unescape(search->text, temp, 128);
        text.setTo(temp, u_strlen(temp));
        u_unescape(search->pattern, temp, 128);
        pattern.setTo(temp, u_strlen(temp));
        if (breaker != NULL) {
            breaker->setText(text);
        }
        collator->setStrength(getECollationStrength(search->strength));
        
        strsrch = new StringSearch(pattern, text, collator, breaker, status);
        if (U_FAILURE(status) || 
            strsrch->getBreakIterator() != breaker) {
            errln("Error setting break iterator");
            if (strsrch != NULL) {
                delete strsrch;
            }
        }
        if (!assertEqualWithStringSearch(strsrch, search)) {
            collator->setStrength(getECollationStrength(UCOL_TERTIARY));
            delete strsrch;
        }
        search   = &(BREAKITERATOREXACT[count + 1]);
        breaker  = getBreakIterator(search->breaker);
        if (breaker != NULL) {
            breaker->setText(text);
        }
        strsrch->setBreakIterator(breaker, status);
        if (U_FAILURE(status) || 
            strsrch->getBreakIterator() != breaker) {
            errln("Error setting break iterator");
            delete strsrch;
        }
        strsrch->reset();
        if (!assertEqualWithStringSearch(strsrch, search)) {
             infoln("Error at test number %d", count);
        }
        delete strsrch;
        count += 2;
    }
    count = 0;
    while (BREAKITERATOREXACT[count].text != NULL) {
         if (!assertEqual(&BREAKITERATOREXACT[count])) {
             infoln("Error at test number %d", count);
         }
         count ++;
    }
}

#endif
    
void StringSearchTest::TestVariable()
{
    int count = 0;
    UErrorCode status = U_ZERO_ERROR;
    m_en_us_->setAttribute(UCOL_ALTERNATE_HANDLING, UCOL_SHIFTED, status);
    if (U_FAILURE(status)) {
        errln("Error setting collation alternate attribute %s", 
              u_errorName(status));
    }
    while (VARIABLE[count].text != NULL) {
        logln("variable %d", count);
        if (!assertEqual(&VARIABLE[count])) {
            infoln("Error at test number %d", count);
        }
        count ++;
    }
    m_en_us_->setAttribute(UCOL_ALTERNATE_HANDLING, UCOL_NON_IGNORABLE, 
                           status);
}
  
void StringSearchTest::TestOverlap()
{
    int count = 0;
    while (OVERLAP[count].text != NULL) {
        if (!assertEqualWithAttribute(&OVERLAP[count], USEARCH_OFF, 
                                      USEARCH_ON)) {
            errln("Error at overlap test number %d", count);
        }
        count ++;
    }    
    count = 0;
    while (NONOVERLAP[count].text != NULL) {
        if (!assertEqual(&NONOVERLAP[count])) {
            errln("Error at non overlap test number %d", count);
        }
        count ++;
    }

    count = 0;
    while (count < 1) {
        const SearchData *search = &(OVERLAP[count]);     
              UChar       temp[128];
        u_unescape(search->text, temp, 128);
        UnicodeString text;
        text.setTo(temp, u_strlen(temp));
        u_unescape(search->pattern, temp, 128);
        UnicodeString pattern;
        pattern.setTo(temp, u_strlen(temp));

        RuleBasedCollator *collator = getCollator(search->collator);
        UErrorCode         status   = U_ZERO_ERROR;
        StringSearch      *strsrch  = new StringSearch(pattern, text, 
                                                       collator, NULL, 
                                                       status);

        strsrch->setAttribute(USEARCH_OVERLAP, USEARCH_ON, status);
        if (U_FAILURE(status) ||
            strsrch->getAttribute(USEARCH_OVERLAP) != USEARCH_ON) {
            errln("Error setting overlap option");
        }
        if (!assertEqualWithStringSearch(strsrch, search)) {
            delete strsrch;
            return;
        }
        
        search = &(NONOVERLAP[count]);
        strsrch->setAttribute(USEARCH_OVERLAP, USEARCH_OFF, status);
        if (U_FAILURE(status) ||
            strsrch->getAttribute(USEARCH_OVERLAP) != USEARCH_OFF) {
            errln("Error setting overlap option");
        }
        strsrch->reset();
        if (!assertEqualWithStringSearch(strsrch, search)) {
            delete strsrch;
            errln("Error at test number %d", count);
         }

        count ++;
        delete strsrch;
    }
}
 
void StringSearchTest::TestCollator()
{
    // test collator that thinks "o" and "p" are the same thing
    UChar         temp[128];
    u_unescape(COLLATOR[0].text, temp, 128);
    UnicodeString text;
    text.setTo(temp, u_strlen(temp));
    u_unescape(COLLATOR[0].pattern, temp, 128);
    UnicodeString pattern;
    pattern.setTo(temp, u_strlen(temp));

    UErrorCode    status = U_ZERO_ERROR;
    StringSearch *strsrch = new StringSearch(pattern, text, m_en_us_, NULL, 
                                             status);    
    if (U_FAILURE(status)) {
        errln("Error opening string search %s", u_errorName(status));
        delete strsrch;
        return;
    }
    if (!assertEqualWithStringSearch(strsrch, &COLLATOR[0])) {
        delete strsrch;
        return;
    }
    
    u_unescape(TESTCOLLATORRULE, temp, 128);
    UnicodeString rules;
    rules.setTo(temp, u_strlen(temp));
    RuleBasedCollator *tailored = new RuleBasedCollator(rules, status);
    tailored->setStrength(getECollationStrength(COLLATOR[1].strength)); 

    if (U_FAILURE(status)) {
        errln("Error opening rule based collator %s", u_errorName(status));
        delete strsrch;
        if (tailored != NULL) {
            delete tailored;
        }
        return;
    }

    strsrch->setCollator(tailored, status);
    if (U_FAILURE(status) || (*strsrch->getCollator()) != (*tailored)) {
        errln("Error setting rule based collator");
        delete strsrch;
        if (tailored != NULL) {
            delete tailored;
        }
    }
    strsrch->reset();
    if (!assertEqualWithStringSearch(strsrch, &COLLATOR[1])) {
        delete strsrch;
        if (tailored != NULL) {
            delete tailored;
        }
        return;
    }
        
    strsrch->setCollator(m_en_us_, status);
    strsrch->reset();
    if (U_FAILURE(status) || (*strsrch->getCollator()) != (*m_en_us_)) {
        errln("Error setting rule based collator");
        delete strsrch;
        if (tailored != NULL) {
            delete tailored;
        }
    }
    if (!assertEqualWithStringSearch(strsrch, &COLLATOR[0])) {
       errln("Error searching collator test");
    }
    delete strsrch;
    if (tailored != NULL) {
        delete tailored;
    }
}

void StringSearchTest::TestPattern()
{
          
    UChar temp[512];
    int templength;
    u_unescape(PATTERN[0].text, temp, 512);
    UnicodeString text;
    text.setTo(temp, u_strlen(temp));
    u_unescape(PATTERN[0].pattern, temp, 512);
    UnicodeString pattern;
    pattern.setTo(temp, u_strlen(temp));

    m_en_us_->setStrength(getECollationStrength(PATTERN[0].strength));
    UErrorCode    status = U_ZERO_ERROR;
    StringSearch *strsrch = new StringSearch(pattern, text, m_en_us_, NULL, 
                                             status);

    if (U_FAILURE(status)) {
        errln("Error opening string search %s", u_errorName(status));
        m_en_us_->setStrength(getECollationStrength(UCOL_TERTIARY));
        if (strsrch != NULL) {
            delete strsrch;
        }
        return;
    }
    if (strsrch->getPattern() != pattern) {
        errln("Error setting pattern");
    }
    if (!assertEqualWithStringSearch(strsrch, &PATTERN[0])) {
        m_en_us_->setStrength(getECollationStrength(UCOL_TERTIARY));
        if (strsrch != NULL) {
            delete strsrch;
        }
        return;
    }

    u_unescape(PATTERN[1].pattern, temp, 512);
    pattern.setTo(temp, u_strlen(temp));
    strsrch->setPattern(pattern, status);
    if (pattern != strsrch->getPattern()) {
        errln("Error setting pattern");
        m_en_us_->setStrength(getECollationStrength(UCOL_TERTIARY));
        if (strsrch != NULL) {
            delete strsrch;
        }
        return;
    }
    strsrch->reset();
    if (U_FAILURE(status)) {
        errln("Error setting pattern %s", u_errorName(status));
    }
    if (!assertEqualWithStringSearch(strsrch, &PATTERN[1])) {
        m_en_us_->setStrength(getECollationStrength(UCOL_TERTIARY));
        if (strsrch != NULL) {
            delete strsrch;
        }
        return;
    }

    u_unescape(PATTERN[0].pattern, temp, 512);
    pattern.setTo(temp, u_strlen(temp));
    strsrch->setPattern(pattern, status);
    if (pattern != strsrch->getPattern()) {
        errln("Error setting pattern");
        m_en_us_->setStrength(getECollationStrength(UCOL_TERTIARY));
        if (strsrch != NULL) {
            delete strsrch;
        }
        return;
    }
    strsrch->reset();
    if (U_FAILURE(status)) {
        errln("Error setting pattern %s", u_errorName(status));
    }
    if (!assertEqualWithStringSearch(strsrch, &PATTERN[0])) {
        m_en_us_->setStrength(getECollationStrength(UCOL_TERTIARY));
        if (strsrch != NULL) {
            delete strsrch;
        }
        return;
    }
    /* enormous pattern size to see if this crashes */
    for (templength = 0; templength != 512; templength ++) {
        temp[templength] = 0x61;
    }
    temp[511] = 0;
    pattern.setTo(temp, 511);
    strsrch->setPattern(pattern, status);
    if (U_FAILURE(status)) {
        errln("Error setting pattern with size 512, %s", u_errorName(status));
    }
    m_en_us_->setStrength(getECollationStrength(UCOL_TERTIARY));
    if (strsrch != NULL) {
        delete strsrch;
    }
}
 
void StringSearchTest::TestText()
{
    UChar temp[128];
    u_unescape(TEXT[0].text, temp, 128);
    UnicodeString text;
    text.setTo(temp, u_strlen(temp));
    u_unescape(TEXT[0].pattern, temp, 128);
    UnicodeString pattern;
    pattern.setTo(temp, u_strlen(temp));

    UErrorCode status = U_ZERO_ERROR;
    StringSearch *strsrch = new StringSearch(pattern, text, m_en_us_, NULL, 
                                             status);
    if (U_FAILURE(status)) {
        errln("Error opening string search %s", u_errorName(status));
        return;
    }
    if (text != strsrch->getText()) {
        errln("Error setting text");
    }
    if (!assertEqualWithStringSearch(strsrch, &TEXT[0])) {
        delete strsrch;
        return;
    }

    u_unescape(TEXT[1].text, temp, 128);
    text.setTo(temp, u_strlen(temp));
    strsrch->setText(text, status);
    if (text != strsrch->getText()) {
        errln("Error setting text");
        delete strsrch;
        return;
    }
    if (U_FAILURE(status)) {
        errln("Error setting text %s", u_errorName(status));
    }
    if (!assertEqualWithStringSearch(strsrch, &TEXT[1])) {
        delete strsrch;
        return;
    }

    u_unescape(TEXT[0].text, temp, 128);
    text.setTo(temp, u_strlen(temp));
    StringCharacterIterator chariter(text);
    strsrch->setText(chariter, status);
    if (text != strsrch->getText()) {
        errln("Error setting text");
        delete strsrch;
        return;
    }
    if (U_FAILURE(status)) {
        errln("Error setting pattern %s", u_errorName(status));
    }
    if (!assertEqualWithStringSearch(strsrch, &TEXT[0])) {
        errln("Error searching within set text");
    }
    delete strsrch;
}
 
void StringSearchTest::TestCompositeBoundaries()
{
    int count = 0;
    while (COMPOSITEBOUNDARIES[count].text != NULL) { 
        logln("composite %d", count);
        if (!assertEqual(&COMPOSITEBOUNDARIES[count])) {
            errln("Error at test number %d", count);
        }
        count ++;
    } 
}
 
void StringSearchTest::TestGetSetOffset()
{
    UErrorCode     status  = U_ZERO_ERROR;
    UnicodeString  pattern("1234567890123456");
    UnicodeString  text("12345678901234567890123456789012");
    StringSearch  *strsrch = new StringSearch(pattern, text, m_en_us_, 
                                              NULL, status);
    /* testing out of bounds error */
    strsrch->setOffset(-1, status);
    if (U_SUCCESS(status)) {
        errln("Error expecting set offset error");
    }
    strsrch->setOffset(128, status);
    if (U_SUCCESS(status)) {
        errln("Error expecting set offset error");
    }
    int index   = 0;
    while (BASIC[index].text != NULL) {
        UErrorCode  status      = U_ZERO_ERROR;
        SearchData  search      = BASIC[index ++];
        UChar       temp[128];
    
        u_unescape(search.text, temp, 128);
        text.setTo(temp, u_strlen(temp));
        u_unescape(search.pattern, temp, 128);
        pattern.setTo(temp, u_strlen(temp));
        strsrch->setText(text, status);
        strsrch->setPattern(pattern, status);
        strsrch->getCollator()->setStrength(getECollationStrength(
                                                          search.strength));
        strsrch->reset();

        int count = 0;
        int32_t matchindex  = search.offset[count];
        while (U_SUCCESS(status) && matchindex >= 0) {
            int32_t matchlength = search.size[count];
            strsrch->next(status);
            if (matchindex != strsrch->getMatchedStart() || 
                matchlength != strsrch->getMatchedLength()) {
                char *str = toCharString(strsrch->getText());
                errln("Text: %s", str);
                str = toCharString(strsrch->getPattern());
                errln("Pattern: %s", str);
                errln("Error match found at %d %d", 
                        strsrch->getMatchedStart(), 
                        strsrch->getMatchedLength());
                return;
            }
            matchindex = search.offset[count + 1] == -1 ? -1 : 
                         search.offset[count + 2];
            if (search.offset[count + 1] != -1) {
                strsrch->setOffset(search.offset[count + 1] + 1, status);
                if (strsrch->getOffset() != search.offset[count + 1] + 1) {
                    errln("Error setting offset\n");
                    return;
                }
            }
            
            count += 2;
        }
        strsrch->next(status);
        if (strsrch->getMatchedStart() != USEARCH_DONE) {
            char *str = toCharString(strsrch->getText());
            errln("Text: %s", str);
            str = toCharString(strsrch->getPattern());
            errln("Pattern: %s", str);
            errln("Error match found at %d %d", 
                        strsrch->getMatchedStart(), 
                        strsrch->getMatchedLength());
            return;
        }
    }
    strsrch->getCollator()->setStrength(getECollationStrength(
                                                             UCOL_TERTIARY));
    delete strsrch;
}
 
void StringSearchTest::TestGetSetAttribute()
{
    UErrorCode     status    = U_ZERO_ERROR;
    UnicodeString  pattern("pattern");
    UnicodeString  text("text");
    StringSearch  *strsrch = new StringSearch(pattern, text, m_en_us_, NULL, 
                                              status);
    if (U_FAILURE(status)) {
        errln("Error opening search %s", u_errorName(status));
        return;
    }

    strsrch->setAttribute(USEARCH_OVERLAP, USEARCH_DEFAULT, status);
    if (U_FAILURE(status) || 
        strsrch->getAttribute(USEARCH_OVERLAP) != USEARCH_OFF) {
        errln("Error setting overlap to the default");
    }
    strsrch->setAttribute(USEARCH_OVERLAP, USEARCH_ON, status);
    if (U_FAILURE(status) || 
        strsrch->getAttribute(USEARCH_OVERLAP) != USEARCH_ON) {
        errln("Error setting overlap true");
    }
    strsrch->setAttribute(USEARCH_OVERLAP, USEARCH_OFF, status);
    if (U_FAILURE(status) || 
        strsrch->getAttribute(USEARCH_OVERLAP) != USEARCH_OFF) {
        errln("Error setting overlap false");
    }
    strsrch->setAttribute(USEARCH_OVERLAP, USEARCH_ATTRIBUTE_VALUE_COUNT, 
                          status);
    if (U_SUCCESS(status)) {
        errln("Error setting overlap to illegal value");
    }
    status = U_ZERO_ERROR;
    strsrch->setAttribute(USEARCH_CANONICAL_MATCH, USEARCH_DEFAULT, status);
    if (U_FAILURE(status) || 
        strsrch->getAttribute(USEARCH_CANONICAL_MATCH) != USEARCH_OFF) {
        errln("Error setting canonical match to the default");
    }
    strsrch->setAttribute(USEARCH_CANONICAL_MATCH, USEARCH_ON, status);
    if (U_FAILURE(status) || 
        strsrch->getAttribute(USEARCH_CANONICAL_MATCH) != USEARCH_ON) {
        errln("Error setting canonical match true");
    }
    strsrch->setAttribute(USEARCH_CANONICAL_MATCH, USEARCH_OFF, status);
    if (U_FAILURE(status) || 
        strsrch->getAttribute(USEARCH_CANONICAL_MATCH) != USEARCH_OFF) {
        errln("Error setting canonical match false");
    }
    strsrch->setAttribute(USEARCH_CANONICAL_MATCH, 
                          USEARCH_ATTRIBUTE_VALUE_COUNT, status);
    if (U_SUCCESS(status)) {
        errln("Error setting canonical match to illegal value");
    }
    status = U_ZERO_ERROR;
    strsrch->setAttribute(USEARCH_ATTRIBUTE_COUNT, USEARCH_DEFAULT, status);
    if (U_SUCCESS(status)) {
        errln("Error setting illegal attribute success");
    }

    delete strsrch;
}
 
void StringSearchTest::TestGetMatch()
{
    UChar      temp[128];
    SearchData search = MATCH[0];
    u_unescape(search.text, temp, 128);
    UnicodeString text;
    text.setTo(temp, u_strlen(temp));
    u_unescape(search.pattern, temp, 128);
    UnicodeString pattern;
    pattern.setTo(temp, u_strlen(temp));

    UErrorCode    status  = U_ZERO_ERROR;
    StringSearch *strsrch = new StringSearch(pattern, text, m_en_us_, NULL, 
                                             status);
    if (U_FAILURE(status)) {
        errln("Error opening string search %s", u_errorName(status));
        if (strsrch != NULL) {
            delete strsrch;
        }
        return;
    }
    
    int           count      = 0;
    int32_t   matchindex = search.offset[count];
    UnicodeString matchtext;
    while (U_SUCCESS(status) && matchindex >= 0) {
        int32_t matchlength = search.size[count];
        strsrch->next(status);
        if (matchindex != strsrch->getMatchedStart() || 
            matchlength != strsrch->getMatchedLength()) {
            char *str = toCharString(strsrch->getText());
            errln("Text: %s", str);
            str = toCharString(strsrch->getPattern());
            errln("Pattern: %s", str);
            errln("Error match found at %d %d", strsrch->getMatchedStart(), 
                  strsrch->getMatchedLength());
            return;
        }
        count ++;
        
        status = U_ZERO_ERROR;
        strsrch->getMatchedText(matchtext);
        if (matchtext.length() != matchlength || U_FAILURE(status)){
            errln("Error getting match text");
        }
        matchindex = search.offset[count];
    }
    status = U_ZERO_ERROR;
    strsrch->next(status);
    if (strsrch->getMatchedStart()  != USEARCH_DONE || 
        strsrch->getMatchedLength() != 0) {
        errln("Error end of match not found");
    }
    status = U_ZERO_ERROR;
    strsrch->getMatchedText(matchtext);
    if (matchtext.length() != 0) {
        errln("Error getting null matches");
    }
    delete strsrch;
}
 
void StringSearchTest::TestSetMatch()
{
    int count = 0;
    while (MATCH[count].text != NULL) {
        SearchData     search = MATCH[count];
        UChar          temp[128];
        UErrorCode status = U_ZERO_ERROR;
        u_unescape(search.text, temp, 128);
        UnicodeString text;
        text.setTo(temp, u_strlen(temp));
        u_unescape(search.pattern, temp, 128);
        UnicodeString pattern;
        pattern.setTo(temp, u_strlen(temp));

        StringSearch *strsrch = new StringSearch(pattern, text, m_en_us_, 
                                                 NULL, status);
        if (U_FAILURE(status)) {
            errln("Error opening string search %s", u_errorName(status));
            if (strsrch != NULL) {
                delete strsrch;
            }
            return;
        }

        int size = 0;
        while (search.offset[size] != -1) {
            size ++;
        }

        if (strsrch->first(status) != search.offset[0] || U_FAILURE(status)) {
            errln("Error getting first match");
        }
        if (strsrch->last(status) != search.offset[size -1] ||
            U_FAILURE(status)) {
            errln("Error getting last match");
        }
        
        int index = 0;
        while (index < size) {
            if (index + 2 < size) {
                if (strsrch->following(search.offset[index + 2] - 1, status) 
                         != search.offset[index + 2] || U_FAILURE(status)) {
                    errln("Error getting following match at index %d", 
                          search.offset[index + 2] - 1);
                }
            }
            if (index + 1 < size) {
                if (strsrch->preceding(search.offset[index + 1] + 
                                                search.size[index + 1] + 1, 
                                       status) != search.offset[index + 1] ||
                    U_FAILURE(status)) {
                    errln("Error getting preceeding match at index %d", 
                          search.offset[index + 1] + 1);
                }
            }
            index += 2;
        }
        status = U_ZERO_ERROR;
        if (strsrch->following(text.length(), status) != USEARCH_DONE) {
            errln("Error expecting out of bounds match");
        }
        if (strsrch->preceding(0, status) != USEARCH_DONE) {
            errln("Error expecting out of bounds match");
        }
        count ++;
        delete strsrch;
    }
}
    
void StringSearchTest::TestReset()
{
    UErrorCode     status  = U_ZERO_ERROR;
    UnicodeString  text("fish fish");
    UnicodeString  pattern("s");
    StringSearch  *strsrch = new StringSearch(pattern, text, m_en_us_, NULL, 
                                              status);
    if (U_FAILURE(status)) {
        errln("Error opening string search %s", u_errorName(status));
        if (strsrch != NULL) {
            delete strsrch;
        }
        return;
    }
    strsrch->setAttribute(USEARCH_OVERLAP, USEARCH_ON, status);
    strsrch->setAttribute(USEARCH_CANONICAL_MATCH, USEARCH_ON, status);
    strsrch->setOffset(9, status);
    if (U_FAILURE(status)) {
        errln("Error setting attributes and offsets");
    }
    else {
        strsrch->reset();
        if (strsrch->getAttribute(USEARCH_OVERLAP) != USEARCH_OFF ||
            strsrch->getAttribute(USEARCH_CANONICAL_MATCH) != USEARCH_OFF ||
            strsrch->getOffset() != 0 || strsrch->getMatchedLength() != 0 ||
            strsrch->getMatchedStart() != USEARCH_DONE) {
            errln("Error resetting string search");
        }
        strsrch->previous(status);
        if (strsrch->getMatchedStart() != 7 ||
            strsrch->getMatchedLength() != 1) {
            errln("Error resetting string search\n");
        }
    }
    delete strsrch;
}
 
void StringSearchTest::TestSupplementary()
{
    int count = 0;
    while (SUPPLEMENTARY[count].text != NULL) {
        if (!assertEqual(&SUPPLEMENTARY[count])) {
            errln("Error at test number %d", count);
        }
        count ++;
    }
}
 
void StringSearchTest::TestContraction()
{
    UChar      temp[128];
    UErrorCode status = U_ZERO_ERROR;
    
    u_unescape(CONTRACTIONRULE, temp, 128);
    UnicodeString rules;
    rules.setTo(temp, u_strlen(temp));
    RuleBasedCollator *collator = new RuleBasedCollator(rules, 
        getECollationStrength(UCOL_TERTIARY), UCOL_ON, status); 
    if (U_FAILURE(status)) {
        errln("Error opening collator %s", u_errorName(status));
    }
    UnicodeString text("text");
    UnicodeString pattern("pattern");
    StringSearch *strsrch = new StringSearch(pattern, text, collator, NULL, 
                                             status);
    if (U_FAILURE(status)) {
        errln("Error opening string search %s", u_errorName(status));
    }   
    
    int count = 0;
    while (CONTRACTION[count].text != NULL) {
        u_unescape(CONTRACTION[count].text, temp, 128);
        text.setTo(temp, u_strlen(temp));
        u_unescape(CONTRACTION[count].pattern, temp, 128);
        pattern.setTo(temp, u_strlen(temp));
        strsrch->setText(text, status);
        strsrch->setPattern(pattern, status);
        if (!assertEqualWithStringSearch(strsrch, &CONTRACTION[count])) {
            errln("Error at test number %d", count);
        }
        count ++;
    }
    delete strsrch;
    delete collator;
}
    
void StringSearchTest::TestIgnorable()
{
    UChar temp[128];
    u_unescape(IGNORABLERULE, temp, 128);
    UnicodeString rules;
    rules.setTo(temp, u_strlen(temp));
    UErrorCode status = U_ZERO_ERROR;
    int        count  = 0;
    RuleBasedCollator *collator = new RuleBasedCollator(rules,  
                            getECollationStrength(IGNORABLE[count].strength), 
                            UCOL_ON, status);  
    if (U_FAILURE(status)) {
        errln("Error opening collator %s", u_errorName(status));
        return;
    }
    UnicodeString pattern("pattern");
    UnicodeString text("text");
    StringSearch *strsrch = new StringSearch(pattern, text, collator, NULL, 
                                             status);
    if (U_FAILURE(status)) {
        errln("Error opening string search %s", u_errorName(status));
        delete collator;
        return;
    }   
    
    while (IGNORABLE[count].text != NULL) {
        u_unescape(IGNORABLE[count].text, temp, 128);
        text.setTo(temp, u_strlen(temp));
        u_unescape(IGNORABLE[count].pattern, temp, 128);
        pattern.setTo(temp, u_strlen(temp));
        strsrch->setText(text, status);
        strsrch->setPattern(pattern, status);
        if (!assertEqualWithStringSearch(strsrch, &IGNORABLE[count])) {
            errln("Error at test number %d", count);
        }
        count ++;
    }
    delete strsrch;
    delete collator;
}

void StringSearchTest::TestDiacriticMatch()
{
	UChar temp[128];
    UErrorCode status = U_ZERO_ERROR;
    int        count  = 0;
    RuleBasedCollator* coll = NULL;
    StringSearch *strsrch = NULL;

    UnicodeString pattern("pattern");
    UnicodeString text("text");
    
    const SearchData *search; 
    
    search = &(DIACRITICMATCH[count]);
    while (search->text != NULL) {
   		coll = getCollator(search->collator);
    	coll->setStrength(getECollationStrength(search->strength));
    	strsrch = new StringSearch(pattern, text, coll, getBreakIterator(search->breaker), status);
    	if (U_FAILURE(status)) {
	        errln("Error opening string search %s", u_errorName(status));
	        return;
	    }  
        u_unescape(search->text, temp, 128);
        text.setTo(temp, u_strlen(temp));
        u_unescape(search->pattern, temp, 128);
        pattern.setTo(temp, u_strlen(temp));
        strsrch->setText(text, status);
        strsrch->setPattern(pattern, status);
        if (!assertEqualWithStringSearch(strsrch, search)) {
            errln("Error at test number %d", count);
        }
        search = &(DIACRITICMATCH[++count]);
        delete strsrch;
    }
    
}
 
void StringSearchTest::TestCanonical()
{
    int count = 0;
    while (BASICCANONICAL[count].text != NULL) {
        if (!assertCanonicalEqual(&BASICCANONICAL[count])) {
            errln("Error at test number %d", count);
        }
        count ++;
    }
}
 
void StringSearchTest::TestNormCanonical()
{
    UErrorCode status = U_ZERO_ERROR;
    m_en_us_->setAttribute(UCOL_NORMALIZATION_MODE, UCOL_ON, status);
    int count = 0;
    while (NORMCANONICAL[count].text != NULL) {
        if (!assertCanonicalEqual(&NORMCANONICAL[count])) {
            errln("Error at test number %d", count);
        }
        count ++;
    }
    m_en_us_->setAttribute(UCOL_NORMALIZATION_MODE, UCOL_OFF, status);
}
 
void StringSearchTest::TestStrengthCanonical()
{
    int count = 0;
    while (STRENGTHCANONICAL[count].text != NULL) {
        if (!assertCanonicalEqual(&STRENGTHCANONICAL[count])) {
            errln("Error at test number %d", count);
        }
        count ++;
    }
}
    
#if !UCONFIG_NO_BREAK_ITERATION

void StringSearchTest::TestBreakIteratorCanonical()
{
    UErrorCode status = U_ZERO_ERROR;
    int        count  = 0;

    while (count < 4) {
        // special purposes for tests numbers 0-3
              UChar           temp[128];
        const SearchData     *search   = &(BREAKITERATORCANONICAL[count]);     
    
        u_unescape(search->text, temp, 128);
        UnicodeString text;
        text.setTo(temp, u_strlen(temp));
        u_unescape(search->pattern, temp, 128);
        UnicodeString pattern;
        pattern.setTo(temp, u_strlen(temp));
        RuleBasedCollator *collator = getCollator(search->collator);
        collator->setStrength(getECollationStrength(search->strength));

        BreakIterator *breaker = getBreakIterator(search->breaker);
        StringSearch  *strsrch = new StringSearch(pattern, text, collator, 
                                                  breaker, status);
        if (U_FAILURE(status)) {
            errln("Error creating string search data");
            return;
        }
        strsrch->setAttribute(USEARCH_CANONICAL_MATCH, USEARCH_ON, status);
        if (U_FAILURE(status) || 
            strsrch->getBreakIterator() != breaker) {
            errln("Error setting break iterator");
            delete strsrch;
            return;
        }
        if (!assertEqualWithStringSearch(strsrch, search)) {
            collator->setStrength(getECollationStrength(UCOL_TERTIARY));
            delete strsrch;
            return;
        }
        search  = &(BREAKITERATOREXACT[count + 1]);
        breaker = getBreakIterator(search->breaker);
        if (breaker == NULL) {
            errln("Error creating BreakIterator");
            return;
        }
        breaker->setText(strsrch->getText());
        strsrch->setBreakIterator(breaker, status);
        if (U_FAILURE(status) || strsrch->getBreakIterator() != breaker) {
            errln("Error setting break iterator");
            delete strsrch;
            return;
        }
        strsrch->reset();
        strsrch->setAttribute(USEARCH_CANONICAL_MATCH, USEARCH_ON, status);
        if (!assertEqualWithStringSearch(strsrch, search)) {
             errln("Error at test number %d", count);
             return;
        }
        delete strsrch;
        count += 2;
    }
    count = 0;
    while (BREAKITERATORCANONICAL[count].text != NULL) {
         if (!assertEqual(&BREAKITERATORCANONICAL[count])) {
             errln("Error at test number %d", count);
             return;
         }
         count ++;
    }
}

#endif
    
void StringSearchTest::TestVariableCanonical()
{
    int count = 0;
    UErrorCode status = U_ZERO_ERROR;
    m_en_us_->setAttribute(UCOL_ALTERNATE_HANDLING, UCOL_SHIFTED, status);
    if (U_FAILURE(status)) {
        errln("Error setting collation alternate attribute %s", 
              u_errorName(status));
    }
    while (VARIABLE[count].text != NULL) {
        logln("variable %d", count);
        if (!assertCanonicalEqual(&VARIABLE[count])) {
            errln("Error at test number %d", count);
        }
        count ++;
    }
    m_en_us_->setAttribute(UCOL_ALTERNATE_HANDLING, UCOL_NON_IGNORABLE, 
                           status);
}
    
void StringSearchTest::TestOverlapCanonical()
{
    int count = 0;
    while (OVERLAPCANONICAL[count].text != NULL) {
        if (!assertEqualWithAttribute(&OVERLAPCANONICAL[count], USEARCH_ON, 
                                      USEARCH_ON)) {
            errln("Error at overlap test number %d", count);
        }
        count ++;
    }    
    count = 0;
    while (NONOVERLAP[count].text != NULL) {
        if (!assertCanonicalEqual(&NONOVERLAPCANONICAL[count])) {
            errln("Error at non overlap test number %d", count);
        }
        count ++;
    }

    count = 0;
    while (count < 1) {
              UChar       temp[128];
        const SearchData *search = &(OVERLAPCANONICAL[count]);     
              UErrorCode  status = U_ZERO_ERROR;
    
        u_unescape(search->text, temp, 128);
        UnicodeString text;
        text.setTo(temp, u_strlen(temp));
        u_unescape(search->pattern, temp, 128);
        UnicodeString pattern;
        pattern.setTo(temp, u_strlen(temp));
        RuleBasedCollator *collator = getCollator(search->collator);
        StringSearch *strsrch = new StringSearch(pattern, text, collator, 
                                                 NULL, status);
        strsrch->setAttribute(USEARCH_CANONICAL_MATCH, USEARCH_ON, status);
        strsrch->setAttribute(USEARCH_OVERLAP, USEARCH_ON, status);
        if (U_FAILURE(status) ||
            strsrch->getAttribute(USEARCH_OVERLAP) != USEARCH_ON) {
            errln("Error setting overlap option");
        }
        if (!assertEqualWithStringSearch(strsrch, search)) {
            delete strsrch;
            return;
        }
        search = &(NONOVERLAPCANONICAL[count]);
        strsrch->setAttribute(USEARCH_OVERLAP, USEARCH_OFF, status);
        if (U_FAILURE(status) ||
            strsrch->getAttribute(USEARCH_OVERLAP) != USEARCH_OFF) {
            errln("Error setting overlap option");
        }
        strsrch->reset();
        if (!assertEqualWithStringSearch(strsrch, search)) {
            delete strsrch;
            errln("Error at test number %d", count);
         }

        count ++;
        delete strsrch;
    }
}
    
void StringSearchTest::TestCollatorCanonical()
{
    /* test collator that thinks "o" and "p" are the same thing */
    UChar temp[128];
    u_unescape(COLLATORCANONICAL[0].text, temp, 128);
    UnicodeString text;
    text.setTo(temp, u_strlen(temp));
    u_unescape(COLLATORCANONICAL[0].pattern, temp, 128);
    UnicodeString pattern;
    pattern.setTo(temp, u_strlen(temp));

    UErrorCode    status  = U_ZERO_ERROR;
    StringSearch *strsrch = new StringSearch(pattern, text, m_en_us_, 
                                             NULL, status);
    strsrch->setAttribute(USEARCH_CANONICAL_MATCH, USEARCH_ON, status);
    if (U_FAILURE(status)) {
        errln("Error opening string search %s", u_errorName(status));
    }
    if (!assertEqualWithStringSearch(strsrch, &COLLATORCANONICAL[0])) {
        delete strsrch;
        return;
    }
    
    u_unescape(TESTCOLLATORRULE, temp, 128);
    UnicodeString rules;
    rules.setTo(temp, u_strlen(temp));
    RuleBasedCollator *tailored = new RuleBasedCollator(rules, 
        getECollationStrength(COLLATORCANONICAL[1].strength), 
        UCOL_ON, status);

    if (U_FAILURE(status)) {
        errln("Error opening rule based collator %s", u_errorName(status));
    }

    strsrch->setCollator(tailored, status);
    if (U_FAILURE(status) || *(strsrch->getCollator()) != *tailored) {
        errln("Error setting rule based collator");
    }
    strsrch->reset();
    strsrch->setAttribute(USEARCH_CANONICAL_MATCH, USEARCH_ON, status);
    if (!assertEqualWithStringSearch(strsrch, &COLLATORCANONICAL[1])) {
        delete strsrch;
        if (tailored != NULL) {
            delete tailored;
        }

        return;
    }
        
    strsrch->setCollator(m_en_us_, status);
    strsrch->reset();
    if (U_FAILURE(status) || *(strsrch->getCollator()) != *m_en_us_) {
        errln("Error setting rule based collator");
    }
    if (!assertEqualWithStringSearch(strsrch, &COLLATORCANONICAL[0])) {
    }
    delete strsrch;
    if (tailored != NULL) {
        delete tailored;
    }
}
    
void StringSearchTest::TestPatternCanonical()
{
    
    UChar temp[128];
    
    u_unescape(PATTERNCANONICAL[0].text, temp, 128);
    UnicodeString text;
    text.setTo(temp, u_strlen(temp));
    u_unescape(PATTERNCANONICAL[0].pattern, temp, 128);
    UnicodeString pattern;
    pattern.setTo(temp, u_strlen(temp));

    m_en_us_->setStrength(
                      getECollationStrength(PATTERNCANONICAL[0].strength));

    UErrorCode    status  = U_ZERO_ERROR;
    StringSearch *strsrch = new StringSearch(pattern, text, m_en_us_, NULL, 
                                             status);
    strsrch->setAttribute(USEARCH_CANONICAL_MATCH, USEARCH_ON, status);
    if (U_FAILURE(status)) {
        errln("Error opening string search %s", u_errorName(status));
        goto ENDTESTPATTERN;
    }
    if (pattern != strsrch->getPattern()) {
        errln("Error setting pattern");
    }
    if (!assertEqualWithStringSearch(strsrch, &PATTERNCANONICAL[0])) {
        goto ENDTESTPATTERN;
    }

    u_unescape(PATTERNCANONICAL[1].pattern, temp, 128);
    pattern.setTo(temp, u_strlen(temp));
    strsrch->setPattern(pattern, status);
    if (pattern != strsrch->getPattern()) {
        errln("Error setting pattern");
        goto ENDTESTPATTERN;
    }
    strsrch->reset();
    strsrch->setAttribute(USEARCH_CANONICAL_MATCH, USEARCH_ON, status);
    if (U_FAILURE(status)) {
        errln("Error setting pattern %s", u_errorName(status));
    }
    if (!assertEqualWithStringSearch(strsrch, &PATTERNCANONICAL[1])) {
        goto ENDTESTPATTERN;
    }

    u_unescape(PATTERNCANONICAL[0].pattern, temp, 128);
    pattern.setTo(temp, u_strlen(temp));
    strsrch->setPattern(pattern, status);
    if (pattern != strsrch->getPattern()) {
        errln("Error setting pattern");
        goto ENDTESTPATTERN;
    }
    strsrch->reset();
    strsrch->setAttribute(USEARCH_CANONICAL_MATCH, USEARCH_ON, status);
    if (U_FAILURE(status)) {
        errln("Error setting pattern %s", u_errorName(status));
    }
    if (!assertEqualWithStringSearch(strsrch, &PATTERNCANONICAL[0])) {
        goto ENDTESTPATTERN;
    }
ENDTESTPATTERN:
    m_en_us_->setStrength(getECollationStrength(UCOL_TERTIARY));
    if (strsrch != NULL) {
        delete strsrch;
    }
}
    
void StringSearchTest::TestTextCanonical()
{
    UChar temp[128];
    u_unescape(TEXTCANONICAL[0].text, temp, 128);
    UnicodeString text;
    text.setTo(temp, u_strlen(temp));
    u_unescape(TEXTCANONICAL[0].pattern, temp, 128);
    UnicodeString pattern;
    pattern.setTo(temp, u_strlen(temp));

    UErrorCode    status  = U_ZERO_ERROR;
    StringSearch *strsrch = new StringSearch(pattern, text, m_en_us_, NULL, 
                                             status);
    strsrch->setAttribute(USEARCH_CANONICAL_MATCH, USEARCH_ON, status);

    if (U_FAILURE(status)) {
        errln("Error opening string search %s", u_errorName(status));
        goto ENDTESTPATTERN;
    }
    if (text != strsrch->getText()) {
        errln("Error setting text");
    }
    if (!assertEqualWithStringSearch(strsrch, &TEXTCANONICAL[0])) {
        goto ENDTESTPATTERN;
    }

    u_unescape(TEXTCANONICAL[1].text, temp, 128);
    text.setTo(temp, u_strlen(temp));
    strsrch->setText(text, status);
    if (text != strsrch->getText()) {
        errln("Error setting text");
        goto ENDTESTPATTERN;
    }
    if (U_FAILURE(status)) {
        errln("Error setting text %s", u_errorName(status));
    }
    if (!assertEqualWithStringSearch(strsrch, &TEXTCANONICAL[1])) {
        goto ENDTESTPATTERN;
    }

    u_unescape(TEXTCANONICAL[0].text, temp, 128);
    text.setTo(temp, u_strlen(temp));
    strsrch->setText(text, status);
    if (text != strsrch->getText()) {
        errln("Error setting text");
        goto ENDTESTPATTERN;
    }
    if (U_FAILURE(status)) {
        errln("Error setting pattern %s", u_errorName(status));
    }
    if (!assertEqualWithStringSearch(strsrch, &TEXTCANONICAL[0])) {
        goto ENDTESTPATTERN;
    }
ENDTESTPATTERN:
    if (strsrch != NULL) {
        delete strsrch;
    }
}
    
void StringSearchTest::TestCompositeBoundariesCanonical()
{
    int count = 0;
    while (COMPOSITEBOUNDARIESCANONICAL[count].text != NULL) { 
        logln("composite %d", count);
        if (!assertCanonicalEqual(&COMPOSITEBOUNDARIESCANONICAL[count])) {
            errln("Error at test number %d", count);
        }
        count ++;
    } 
}
    
void StringSearchTest::TestGetSetOffsetCanonical()
{
    
    UErrorCode     status  = U_ZERO_ERROR;
    UnicodeString  text("text");
    UnicodeString  pattern("pattern");
    StringSearch  *strsrch = new StringSearch(pattern, text, m_en_us_, NULL, 
                                              status);
    Collator *collator = strsrch->getCollator();

    collator->setAttribute(UCOL_NORMALIZATION_MODE, UCOL_ON, status);

    strsrch->setAttribute(USEARCH_CANONICAL_MATCH, USEARCH_ON, status);
    /* testing out of bounds error */
    strsrch->setOffset(-1, status);
    if (U_SUCCESS(status)) {
        errln("Error expecting set offset error");
    }
    strsrch->setOffset(128, status);
    if (U_SUCCESS(status)) {
        errln("Error expecting set offset error");
    }
    int   index   = 0;
    UChar temp[128];
    while (BASICCANONICAL[index].text != NULL) {
        SearchData  search      = BASICCANONICAL[index ++];
        if (BASICCANONICAL[index].text == NULL) {
            /* skip the last one */
            break;
        }
        
        u_unescape(search.text, temp, 128);
        text.setTo(temp, u_strlen(temp));
        u_unescape(search.pattern, temp, 128);
        pattern.setTo(temp, u_strlen(temp));

        UErrorCode  status      = U_ZERO_ERROR;
        strsrch->setText(text, status);

        strsrch->setPattern(pattern, status);

        int         count       = 0;
        int32_t matchindex  = search.offset[count];
        while (U_SUCCESS(status) && matchindex >= 0) {
            int32_t matchlength = search.size[count];
            strsrch->next(status);
            if (matchindex != strsrch->getMatchedStart() || 
                matchlength != strsrch->getMatchedLength()) {
                char *str = toCharString(strsrch->getText());
                errln("Text: %s", str);
                str = toCharString(strsrch->getPattern());
                errln("Pattern: %s", str);
                errln("Error match found at %d %d", 
                      strsrch->getMatchedStart(), 
                      strsrch->getMatchedLength());
                goto bail;
            }
            matchindex = search.offset[count + 1] == -1 ? -1 : 
                         search.offset[count + 2];
            if (search.offset[count + 1] != -1) {
                strsrch->setOffset(search.offset[count + 1] + 1, status);
                if (strsrch->getOffset() != search.offset[count + 1] + 1) {
                    errln("Error setting offset");
                    goto bail;
                }
            }
            
            count += 2;
        }
        strsrch->next(status);
        if (strsrch->getMatchedStart() != USEARCH_DONE) {
            char *str = toCharString(strsrch->getText());
            errln("Text: %s", str);
            str = toCharString(strsrch->getPattern());
            errln("Pattern: %s", str);
            errln("Error match found at %d %d", strsrch->getMatchedStart(), 
                   strsrch->getMatchedLength());
            goto bail;
        }
    }

bail:
    collator->setAttribute(UCOL_NORMALIZATION_MODE, UCOL_OFF, status);
    delete strsrch;
}
    
void StringSearchTest::TestSupplementaryCanonical()
{
    int count = 0;
    while (SUPPLEMENTARYCANONICAL[count].text != NULL) {
        if (!assertCanonicalEqual(&SUPPLEMENTARYCANONICAL[count])) {
            errln("Error at test number %d", count);
        }
        count ++;
    }
}
    
void StringSearchTest::TestContractionCanonical()
{
    UChar          temp[128];
    
    u_unescape(CONTRACTIONRULE, temp, 128);
    UnicodeString rules;
    rules.setTo(temp, u_strlen(temp));

    UErrorCode         status   = U_ZERO_ERROR;
    RuleBasedCollator *collator = new RuleBasedCollator(rules, 
        getECollationStrength(UCOL_TERTIARY), UCOL_ON, status); 
    if (U_FAILURE(status)) {
        errln("Error opening collator %s", u_errorName(status));
    }
    UnicodeString text("text");
    UnicodeString pattern("pattern");
    StringSearch *strsrch = new StringSearch(pattern, text, collator, NULL, 
                                             status);
    strsrch->setAttribute(USEARCH_CANONICAL_MATCH, USEARCH_ON, status);
    if (U_FAILURE(status)) {
        errln("Error opening string search %s", u_errorName(status));
    }   
    
    int count = 0;
    while (CONTRACTIONCANONICAL[count].text != NULL) {
        u_unescape(CONTRACTIONCANONICAL[count].text, temp, 128);
        text.setTo(temp, u_strlen(temp));
        u_unescape(CONTRACTIONCANONICAL[count].pattern, temp, 128);
        pattern.setTo(temp, u_strlen(temp));
        strsrch->setText(text, status);
        strsrch->setPattern(pattern, status);
        if (!assertEqualWithStringSearch(strsrch, 
                                             &CONTRACTIONCANONICAL[count])) {
            errln("Error at test number %d", count);
        }
        count ++;
    }
    delete strsrch;
    delete collator;
}

void StringSearchTest::TestUClassID()
{
    char id = *((char *)StringSearch::getStaticClassID());
    if (id != 0) {
        errln("Static class id for StringSearch should be 0");
    }
    UErrorCode     status    = U_ZERO_ERROR;
    UnicodeString  text("text");
    UnicodeString  pattern("pattern");
    StringSearch  *strsrch = new StringSearch(pattern, text, m_en_us_, NULL, 
                                              status);
    id = *((char *)strsrch->getDynamicClassID());
    if (id != 0) {
        errln("Dynamic class id for StringSearch should be 0");
    }
    delete strsrch;
}

class TestSearch : public SearchIterator
{
public:
    TestSearch(const TestSearch &obj);
    TestSearch(const UnicodeString &text, 
               BreakIterator *breakiter,
               const UnicodeString &pattern);
    ~TestSearch();

    void        setOffset(int32_t position, UErrorCode &status);
    int32_t     getOffset() const;
    SearchIterator* safeClone() const;


    /**
     * ICU "poor man's RTTI", returns a UClassID for the actual class.
     *
     * @draft ICU 2.2
     */
    virtual inline UClassID getDynamicClassID() const { return getStaticClassID(); }

    /**
     * ICU "poor man's RTTI", returns a UClassID for this class.
     *
     * @draft ICU 2.2
     */
    static inline UClassID getStaticClassID() { return (UClassID)&fgClassID; }

    UBool operator!=(const TestSearch &that) const;

    UnicodeString m_pattern_;

protected:
    int32_t      handleNext(int32_t position, UErrorCode &status);
    int32_t      handlePrev(int32_t position, UErrorCode &status);
    TestSearch & operator=(const TestSearch &that);

private:

    /**
     * The address of this static class variable serves as this class's ID
     * for ICU "poor man's RTTI".
     */
    static const char fgClassID;
    uint32_t m_offset_;
};

const char TestSearch::fgClassID=0;

TestSearch::TestSearch(const TestSearch &obj) : SearchIterator(obj)
{
    m_offset_ = obj.m_offset_;
    m_pattern_ = obj.m_pattern_;
}

TestSearch::TestSearch(const UnicodeString &text, 
                       BreakIterator *breakiter,
                       const UnicodeString &pattern) : SearchIterator()
{
    m_breakiterator_ = breakiter;
    m_pattern_ = pattern;
    m_text_ = text;
    m_offset_ = 0;
    m_pattern_ = pattern;
}

TestSearch::~TestSearch()
{
}


void TestSearch::setOffset(int32_t position, UErrorCode &status)
{
    if (position >= 0 && position <= m_text_.length()) {
        m_offset_ = position;
    }
    else {
        status = U_INDEX_OUTOFBOUNDS_ERROR;
    }
}

int32_t TestSearch::getOffset() const
{
    return m_offset_;
}

SearchIterator * TestSearch::safeClone() const 
{
    return new TestSearch(m_text_, m_breakiterator_, m_pattern_);
}

UBool TestSearch::operator!=(const TestSearch &that) const
{
    if (SearchIterator::operator !=(that)) {
        return FALSE;
    }
    return m_offset_ != that.m_offset_ || m_pattern_ != that.m_pattern_;
}

int32_t TestSearch::handleNext(int32_t start, UErrorCode &status)
{
  if(U_SUCCESS(status)) {
    int match = m_text_.indexOf(m_pattern_, start);
    if (match < 0) {
        m_offset_ = m_text_.length();
        setMatchStart(m_offset_);
        setMatchLength(0);
        return USEARCH_DONE;
    }
    setMatchStart(match);
    m_offset_ = match;
    setMatchLength(m_pattern_.length());
    return match;
  } else {
    return USEARCH_DONE;
  }
}

int32_t TestSearch::handlePrev(int32_t start, UErrorCode &status)
{
  if(U_SUCCESS(status)) {
    int match = m_text_.lastIndexOf(m_pattern_, 0, start);
    if (match < 0) {
        m_offset_ = 0;
        setMatchStart(m_offset_);
        setMatchLength(0);
        return USEARCH_DONE;
    }
    setMatchStart(match);
    m_offset_ = match;
    setMatchLength(m_pattern_.length());
    return match;
  } else {
    return USEARCH_DONE;
  }
}

TestSearch & TestSearch::operator=(const TestSearch &that)
{
    SearchIterator::operator=(that);
    m_offset_ = that.m_offset_;
    m_pattern_ = that.m_pattern_;
    return *this;
}

void StringSearchTest::TestSubclass()
{
    UnicodeString text("abc abcd abc");
    UnicodeString pattern("abc");
    TestSearch search(text, NULL, pattern);
    TestSearch search2(search);
    int expected[] = {0, 4, 9};
    UErrorCode status = U_ZERO_ERROR; 
    int i;
    StringCharacterIterator chariter(text);

    search.setText(text, status);
    if (search.getText() != search2.getText()) {
        errln("Error setting text");
    }

    search.setText(chariter, status);
    if (search.getText() != search2.getText()) {
        errln("Error setting text");
    }

    search.reset();
    // comparing constructors
 
    for (i = 0; i < (int)(sizeof(expected) / sizeof(expected[0])); i ++) {
        if (search.next(status) != expected[i]) {
            errln("Error getting next match");
        }
        if (search.getMatchedLength() != search.m_pattern_.length()) {
            errln("Error getting next match length");
        }
    }
    if (search.next(status) != USEARCH_DONE) {
        errln("Error should have reached the end of the iteration");
    }
    for (i = sizeof(expected) / sizeof(expected[0]) - 1; i >= 0; i --) {
        if (search.previous(status) != expected[i]) {
            errln("Error getting previous match");
        }
        if (search.getMatchedLength() != search.m_pattern_.length()) {
            errln("Error getting previous match length");
        }
    }
    if (search.previous(status) != USEARCH_DONE) {
        errln("Error should have reached the start of the iteration");
    }
}

class StubSearchIterator:public SearchIterator{
public:
    StubSearchIterator(){}
    virtual void setOffset(int32_t , UErrorCode &) {};
    virtual int32_t getOffset(void) const {return 0;};
    virtual SearchIterator* safeClone(void) const {return NULL;};
    virtual int32_t handleNext(int32_t , UErrorCode &){return 0;};
    virtual int32_t handlePrev(int32_t , UErrorCode &) {return 0;};
    virtual UClassID getDynamicClassID() const {
        static char classID = 0;
        return (UClassID)&classID; 
    }
};

void StringSearchTest::TestCoverage(){
    StubSearchIterator stub1, stub2;
    UErrorCode status = U_ZERO_ERROR;

    if (stub1 != stub2){
        errln("new StubSearchIterator should be equal");
    }

    stub2.setText(UnicodeString("ABC"), status);
    if (U_FAILURE(status)) {
        errln("Error: SearchIterator::SetText");
    }

    stub1 = stub2;
    if (stub1 != stub2){
        errln("SearchIterator::operator =  assigned object should be equal");
    }
}

#endif /* !UCONFIG_NO_BREAK_ITERATION */

#endif /* #if !UCONFIG_NO_COLLATION */
