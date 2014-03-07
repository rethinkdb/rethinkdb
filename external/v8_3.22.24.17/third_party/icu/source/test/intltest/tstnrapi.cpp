/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2010, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

#include "unicode/utypes.h"
#include "unicode/uchar.h"
#include "unicode/normlzr.h"
#include "unicode/uniset.h"
#include "unicode/usetiter.h"
#include "unicode/schriter.h"
#include "tstnorm.h"

#if !UCONFIG_NO_NORMALIZATION

static UErrorCode status = U_ZERO_ERROR;

// test APIs that are not otherwise used - improve test coverage
void
BasicNormalizerTest::TestNormalizerAPI() {
    // instantiate a Normalizer from a CharacterIterator
    UnicodeString s=UnicodeString("a\\u0308\\uac00\\U0002f800", "").unescape();
    s.append(s); // make s a bit longer and more interesting
    StringCharacterIterator iter(s);
    Normalizer norm(iter, UNORM_NFC);
    if(norm.next()!=0xe4) {
        dataerrln("error in Normalizer(CharacterIterator).next()");
    }

    // test copy constructor
    Normalizer copy(norm);
    if(copy.next()!=0xac00) {
        dataerrln("error in Normalizer(Normalizer(CharacterIterator)).next()");
    }

    // test clone(), ==, and hashCode()
    Normalizer *clone=copy.clone();
    if(*clone!=copy) {
        errln("error in Normalizer(Normalizer(CharacterIterator)).clone()!=copy");
    }
    // clone must have the same hashCode()
    if(clone->hashCode()!=copy.hashCode()) {
        errln("error in Normalizer(Normalizer(CharacterIterator)).clone()->hashCode()!=copy.hashCode()");
    }
    if(clone->next()!=0x4e3d) {
        dataerrln("error in Normalizer(Normalizer(CharacterIterator)).clone()->next()");
    }
    // position changed, must change hashCode()
    if(clone->hashCode()==copy.hashCode()) {
        errln("error in Normalizer(Normalizer(CharacterIterator)).clone()->next().hashCode()==copy.hashCode()");
    }
    delete clone;
    clone=0;

    // test compose() and decompose()
    UnicodeString tel, nfkc, nfkd;
    tel=UnicodeString(1, (UChar32)0x2121, 10);
    tel.insert(1, (UChar)0x301);

    UErrorCode errorCode=U_ZERO_ERROR;
    Normalizer::compose(tel, TRUE, 0, nfkc, errorCode);
    Normalizer::decompose(tel, TRUE, 0, nfkd, errorCode);
    if(U_FAILURE(errorCode)) {
        dataerrln("error in Normalizer::(de)compose(): %s", u_errorName(errorCode));
    } else if(
        nfkc!=UnicodeString("TE\\u0139TELTELTELTELTELTELTELTELTEL", "").unescape() || 
        nfkd!=UnicodeString("TEL\\u0301TELTELTELTELTELTELTELTELTEL", "").unescape()
    ) {
        errln("error in Normalizer::(de)compose(): wrong result(s)");
    }

    // test setIndex()
    norm.setIndexOnly(3);
    if(norm.current()!=0x4e3d) {
        dataerrln("error in Normalizer(CharacterIterator).setIndex(3)");
    }

    // test setText(CharacterIterator) and getText()
    UnicodeString out, out2;
    errorCode=U_ZERO_ERROR;
    copy.setText(iter, errorCode);
    if(U_FAILURE(errorCode)) {
        errln("error Normalizer::setText() failed: %s", u_errorName(errorCode));
    } else {
        copy.getText(out);
        iter.getText(out2);
        if( out!=out2 ||
            copy.startIndex()!=iter.startIndex() ||
            copy.endIndex()!=iter.endIndex()
        ) {
            errln("error in Normalizer::setText() or Normalizer::getText()");
        }
    }

    // test setText(UChar *), getUMode() and setMode()
    errorCode=U_ZERO_ERROR;
    copy.setText(s.getBuffer()+1, s.length()-1, errorCode);
    copy.setMode(UNORM_NFD);
    if(copy.getUMode()!=UNORM_NFD) {
        errln("error in Normalizer::setMode() or Normalizer::getUMode()");
    }
    if(copy.next()!=0x308 || copy.next()!=0x1100) {
        dataerrln("error in Normalizer::setText(UChar *) or Normalizer::setMode()");
    }

    // test setText(UChar *, length=-1)
    errorCode=U_ZERO_ERROR;

    // NUL-terminate s
    s.append((UChar)0);         // append NUL
    s.truncate(s.length()-1);   // undo length change

    copy.setText(s.getBuffer()+1, -1, errorCode);
    if(copy.endIndex()!=s.length()-1) {
        errln("error in Normalizer::setText(UChar *, -1)");
    }

    // test setOption() and getOption()
    copy.setOption(0xaa0000, TRUE);
    copy.setOption(0x20000, FALSE);
    if(!copy.getOption(0x880000) || copy.getOption(0x20000)) {
        errln("error in Normalizer::setOption() or Normalizer::getOption()");
    }

    // test last()/previous() with an internal buffer overflow
    errorCode=U_ZERO_ERROR;
    copy.setText(UnicodeString(1000, (UChar32)0x308, 1000), errorCode);
    if(copy.last()!=0x308) {
        errln("error in Normalizer(1000*U+0308).last()");
    }

    // test UNORM_NONE
    norm.setMode(UNORM_NONE);
    if(norm.first()!=0x61 || norm.next()!=0x308 || norm.last()!=0x2f800) {
        errln("error in Normalizer(UNORM_NONE).first()/next()/last()");
    }
    Normalizer::normalize(s, UNORM_NONE, 0, out, status);
    if(out!=s) {
        errln("error in Normalizer::normalize(UNORM_NONE)");
    }

    // test that the same string can be used as source and destination
    s.setTo((UChar)0xe4);
    Normalizer::normalize(s, UNORM_NFD, 0, s, status);
    if(s.charAt(1)!=0x308) {
        dataerrln("error in Normalizer::normalize(UNORM_NFD, self)");
    }
    Normalizer::normalize(s, UNORM_NFC, 0, s, status);
    if(s.charAt(0)!=0xe4) {
        dataerrln("error in Normalizer::normalize(UNORM_NFC, self)");
    }
    Normalizer::decompose(s, FALSE, 0, s, status);
    if(s.charAt(1)!=0x308) {
        dataerrln("error in Normalizer::decompose(self)");
    }
    Normalizer::compose(s, FALSE, 0, s, status);
    if(s.charAt(0)!=0xe4) {
        dataerrln("error in Normalizer::compose(self)");
    }
    Normalizer::concatenate(s, s, s, UNORM_NFC, 0, status);
    if(s.charAt(1)!=0xe4) {
        dataerrln("error in Normalizer::decompose(self)");
    }
}

#endif
