/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 2001-2010, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/
/************************************************************************
*   This test program is intended for testing Replaceable class.
*
*   Date        Name        Description
*   11/28/2001  hshih       Ported back from Java.
* 
************************************************************************/

#include "unicode/utypes.h"

#if !UCONFIG_NO_TRANSLITERATION

#include "ittrans.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "unicode/rep.h"
#include "reptest.h"

//---------------------------------------------
// runIndexedTest
//---------------------------------------------

    /**
     * This is a test class that simulates styled text.
     * It associates a style number (0..65535) with each character,
     * and maintains that style in the normal fashion:
     * When setting text from raw string or characters,<br>
     * Set the styles to the style of the first character replaced.<br>
     * If no characters are replaced, use the style of the previous character.<br>
     * If at start, use the following character<br>
     * Otherwise use NO_STYLE.
     */
class TestReplaceable : public Replaceable {
    UnicodeString chars;
    UnicodeString styles;
    
    static const UChar NO_STYLE;

    static const UChar NO_STYLE_MARK;

    /**
     * The address of this static class variable serves as this class's ID
     * for ICU "poor man's RTTI".
     */
    static const char fgClassID;

public:    
    TestReplaceable (const UnicodeString& text, 
                     const UnicodeString& newStyles) {
        chars = text;
        UnicodeString s;
        for (int i = 0; i < text.length(); ++i) {
            if (i < newStyles.length()) {
                s.append(newStyles.charAt(i));
            } else {
                if (text.charAt(i) == NO_STYLE_MARK) {
                    s.append(NO_STYLE);
                } else {
                    s.append((UChar)(i + 0x0031));
                }
            }
        }
        this->styles = s;
    }

    virtual Replaceable *clone() const {
        return new TestReplaceable(chars, styles);
    }

    ~TestReplaceable(void) {}

    UnicodeString getStyles() {
        return styles;
    }
    
    UnicodeString toString() {
        UnicodeString s = chars;
        s.append("{");
        s.append(styles);
        s.append("}");
        return s;
    }

    void extractBetween(int32_t start, int32_t limit, UnicodeString& result) const {
        chars.extractBetween(start, limit, result);
    }

    /**
     * ICU "poor man's RTTI", returns a UClassID for this class.
     *
     * @draft ICU 2.2
     */
    static inline UClassID getStaticClassID() { return (UClassID)&fgClassID; }

    /**
     * ICU "poor man's RTTI", returns a UClassID for the actual class.
     *
     * @draft ICU 2.2
     */
    virtual inline UClassID getDynamicClassID() const { return getStaticClassID(); }

protected:
    virtual int32_t getLength() const {
        return chars.length();
    }

    virtual UChar getCharAt(int32_t offset) const{
        return chars.charAt(offset);
    }

    virtual UChar32 getChar32At(int32_t offset) const{
        return chars.char32At(offset);
    }

    void fixStyles(int32_t start, int32_t limit, int32_t newLen) {
        UChar newStyle = NO_STYLE;
        if (start != limit && styles.charAt(start) != NO_STYLE) {
            newStyle = styles.charAt(start);
        } else if (start > 0 && getCharAt(start-1) != NO_STYLE_MARK) {
            newStyle = styles.charAt(start-1);
        } else if (limit < styles.length()) {
            newStyle = styles.charAt(limit);
        }
        // dumb implementation for now.
        UnicodeString s;
        for (int i = 0; i < newLen; ++i) {
            // this doesn't really handle an embedded NO_STYLE_MARK
            // in the middle of a long run of characters right -- but
            // that case shouldn't happen anyway
            if (getCharAt(start+i) == NO_STYLE_MARK) {
                s.append(NO_STYLE);
            } else {
                s.append(newStyle);
            }
        }
        styles.replaceBetween(start, limit, s);
    }

    virtual void handleReplaceBetween(int32_t start, int32_t limit, const UnicodeString& text) {
        UnicodeString s;
        this->extractBetween(start, limit, s);
        if (s == text) return; // NO ACTION!
        this->chars.replaceBetween(start, limit, text);
        fixStyles(start, limit, text.length());
    }
    

    virtual void copy(int32_t start, int32_t limit, int32_t dest) {
        chars.copy(start, limit, dest);
        styles.copy(start, limit, dest);
    }
};

const char TestReplaceable::fgClassID=0;

const UChar TestReplaceable::NO_STYLE  = 0x005F;

const UChar TestReplaceable::NO_STYLE_MARK = 0xFFFF;

void
ReplaceableTest::runIndexedTest(int32_t index, UBool exec,
                                      const char* &name, char* /*par*/) {
    switch (index) {
        TESTCASE(0,TestReplaceableClass);
        default: name = ""; break;
    }
}

/*
 * Dummy Replaceable implementation for better API/code coverage.
 */
class NoopReplaceable : public Replaceable {
public:
    virtual int32_t getLength() const {
        return 0;
    }

    virtual UChar getCharAt(int32_t /*offset*/) const{
        return 0xffff;
    }

    virtual UChar32 getChar32At(int32_t /*offset*/) const{
        return 0xffff;
    }

    void extractBetween(int32_t /*start*/, int32_t /*limit*/, UnicodeString& result) const {
        result.remove();
    }

    virtual void handleReplaceBetween(int32_t /*start*/, int32_t /*limit*/, const UnicodeString &/*text*/) {
        /* do nothing */
    }

    virtual void copy(int32_t /*start*/, int32_t /*limit*/, int32_t /*dest*/) {
        /* do nothing */
    }

    static inline UClassID getStaticClassID() { return (UClassID)&fgClassID; }
    virtual inline UClassID getDynamicClassID() const { return getStaticClassID(); }

private:
    static const char fgClassID;
};

const char NoopReplaceable::fgClassID=0;

void ReplaceableTest::TestReplaceableClass(void) {
    UChar rawTestArray[][6] = {
        {0x0041, 0x0042, 0x0043, 0x0044, 0x0000, 0x0000}, // ABCD
        {0x0061, 0x0062, 0x0063, 0x0064, 0x00DF, 0x0000}, // abcd\u00DF
        {0x0061, 0x0042, 0x0043, 0x0044, 0x0000, 0x0000}, // aBCD
        {0x0041, 0x0300, 0x0045, 0x0300, 0x0000, 0x0000}, // A\u0300E\u0300
        {0x00C0, 0x00C8, 0x0000, 0x0000, 0x0000, 0x0000}, // \u00C0\u00C8
        {0x0077, 0x0078, 0x0079, 0x0000, 0x0000, 0x0000}, /* "wxy" */
        {0x0077, 0x0078, 0x0079, 0x007A, 0x0000, 0x0000}, /* "wxyz" */
        {0x0077, 0x0078, 0x0079, 0x007A, 0x0075, 0x0000}, /* "wxyzu" */
        {0x0078, 0x0079, 0x007A, 0x0000, 0x0000, 0x0000}, /* "xyz" */
        {0x0077, 0x0078, 0x0079, 0x0000, 0x0000, 0x0000}, /* "wxy" */
        {0xFFFF, 0x0078, 0x0079, 0x0000, 0x0000, 0x0000}, /* "*xy" */
        {0xFFFF, 0x0078, 0x0079, 0x0000, 0x0000, 0x0000}, /* "*xy" */
    };
    check("Lower", rawTestArray[0], "1234");
    check("Upper", rawTestArray[1], "123455"); // must map 00DF to SS
    check("Title", rawTestArray[2], "1234");
    check("NFC",   rawTestArray[3], "13");
    check("NFD",   rawTestArray[4], "1122");
    check("*(x) > A $1 B", rawTestArray[5], "11223");
    check("*(x)(y) > A $2 B $1 C $2 D", rawTestArray[6], "113322334");
    check("*(x)(y)(z) > A $3 B $2 C $1 D", rawTestArray[7], "114433225");
    // Disabled for 2.4.  TODO Revisit in 2.6 or later.
    //check("*x > a", rawTestArray[8], "223"); // expect "123"?
    //check("*x > a", rawTestArray[9], "113"); // expect "123"?
    //check("*x > a", rawTestArray[10], "_33"); // expect "_23"?
    //check("*(x) > A $1 B", rawTestArray[11], "__223");

    // improve API/code coverage
    NoopReplaceable noop;
    Replaceable *p;
    if((p=noop.clone())!=NULL) {
        errln("Replaceable::clone() does not return NULL");
        delete p;
    }

    if(!noop.hasMetaData()) {
        errln("Replaceable::hasMetaData() does not return TRUE");
    }

    // try to call the compiler-provided
    // UMemory/UObject/Replaceable assignment operators
    NoopReplaceable noop2;
    noop2=noop;
    if((p=noop2.clone())!=NULL) {
        errln("noop2.Replaceable::clone() does not return NULL");
        delete p;
    }

    // try to call the compiler-provided
    // UMemory/UObject/Replaceable copy constructors
    NoopReplaceable noop3(noop);
    if((p=noop3.clone())!=NULL) {
        errln("noop3.Replaceable::clone() does not return NULL");
        delete p;
    }
}

void ReplaceableTest::check(const UnicodeString& transliteratorName, 
                            const UnicodeString& test, 
                            const UnicodeString& shouldProduceStyles) 
{
    UErrorCode status = U_ZERO_ERROR;
    TestReplaceable *tr = new TestReplaceable(test, "");
    UnicodeString expectedStyles = shouldProduceStyles;
    UnicodeString original = tr->toString();

    Transliterator* t;
    if (transliteratorName.charAt(0) == 0x2A /*'*'*/) {
        UnicodeString rules(transliteratorName);
        rules.remove(0,1);
        UParseError pe;
        t = Transliterator::createFromRules("test", rules, UTRANS_FORWARD,
                                            pe, status);

        // test clone()
        TestReplaceable *tr2 = (TestReplaceable *)tr->clone();
        if(tr2 != NULL) {
            delete tr;
            tr = tr2;
        }
    } else {
        t = Transliterator::createInstance(transliteratorName, UTRANS_FORWARD, status);
    }
    if (U_FAILURE(status)) {
        dataerrln("FAIL: failed to create the " + transliteratorName + " transliterator");
        delete tr;
        return;
    }
    t->transliterate(*tr);
    UnicodeString newStyles = tr->getStyles();
    if (newStyles != expectedStyles) {
        errln("FAIL Styles: " + transliteratorName + "{" + original + "} => "
            + tr->toString() + "; should be {" + expectedStyles + "}!");
    } else {
        log("OK: ");
        log(transliteratorName);
        log("(");
        log(original);
        log(") => ");
        logln(tr->toString());
    }
    delete tr;
    delete t;
}

#endif /* #if !UCONFIG_NO_TRANSLITERATION */
