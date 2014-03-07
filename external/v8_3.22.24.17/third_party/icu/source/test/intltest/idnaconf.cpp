/*
 *******************************************************************************
 *
 *   Copyright (C) 2005-2009, International Business Machines
 *   Corporation and others.  All Rights Reserved.
 *
 *******************************************************************************
 *
 *   created on: 2005jun15
 *   created by: Raymond Yang
 */

#if !UCONFIG_NO_IDNA 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "unicode/utypes.h"
#include "unicode/ucnv.h"
#include "unicode/ustring.h"
#include "unicode/uidna.h"

#include "idnaconf.h"

static const UChar C_TAG[] = {0x3D, 0x3D, 0x3D, 0x3D, 0x3D, 0}; // =====
static const UChar C_NAMEZONE[] = {0x6E, 0x61, 0x6D, 0x65, 0x7A, 0x6F, 0x6E, 0x65, 0}; // namezone 
static const UChar C_NAMEBASE[] = {0x6E, 0x61, 0x6D, 0x65, 0x62, 0x61, 0x73, 0x65, 0}; // namebase 
static const UChar C_NAMEUTF8[] = {0x6E, 0x61, 0x6D, 0x65, 0x75, 0x74, 0x66, 0x38, 0}; // nameutf8 

static const UChar C_TYPE[] = {0x74, 0x79, 0x70, 0x65, 0}; // type
static const UChar C_TOASCII[]  =  {0x74, 0x6F, 0x61, 0x73, 0x63, 0x69, 0x69, 0};       // toascii
static const UChar C_TOUNICODE[] = {0x74, 0x6F, 0x75, 0x6E, 0x69, 0x63, 0x6F, 0x64, 0x65, 0}; // tounicode

static const UChar C_PASSFAIL[] = {0x70, 0x61, 0x73, 0x73, 0x66, 0x61, 0x69, 0x6C, 0}; // passfail
static const UChar C_PASS[] = {0x70, 0x61, 0x73, 0x73, 0}; // pass
static const UChar C_FAIL[] = {0x66, 0x61, 0x69, 0x6C, 0}; // fail

static const UChar C_DESC[] = {0x64, 0x65, 0x73, 0x63, 0}; // desc
static const UChar C_USESTD3ASCIIRULES[] = {0x55, 0x73, 0x65, 0x53, 0x54, 0x44, 
       0x33, 0x41, 0x53, 0x43, 0x49, 0x49, 0x52, 0x75, 0x6C, 0x65, 0x73, 0}; // UseSTD3ASCIIRules

IdnaConfTest::IdnaConfTest(){
    base = NULL;
    len = 0;
    curOffset = 0;

    type = option = passfail = -1;
    namebase.setToBogus();
    namezone.setToBogus();
}
IdnaConfTest::~IdnaConfTest(){
    delete [] base;
}

#if !UCONFIG_NO_IDNA
/* this function is modified from RBBITest::ReadAndConvertFile() 
 *
 */
UBool IdnaConfTest::ReadAndConvertFile(){
    
    char * source = NULL;
    size_t source_len;

    // read the test data file to memory
    FILE* f    = NULL;
    UErrorCode  status  = U_ZERO_ERROR;

    const char *path = IntlTest::getSourceTestData(status);
    if (U_FAILURE(status)) {
        errln("%s", u_errorName(status));
        return FALSE;
    }

    const char* name = "idna_conf.txt";     // test data file
    int t = strlen(path) + strlen(name) + 1;
    char* absolute_name = new char[t];
    strcpy(absolute_name, path);
    strcat(absolute_name, name);
    f = fopen(absolute_name, "rb");
    delete [] absolute_name;

    if (f == NULL){
        dataerrln("fopen error on %s", name);
        return FALSE;
    }

    fseek( f, 0, SEEK_END);
    if ((source_len = ftell(f)) <= 0){
        errln("Error reading test data file.");
        fclose(f);
        return FALSE;
    }

    source = new char[source_len];
    fseek(f, 0, SEEK_SET);
    if (fread(source, 1, source_len, f) != source_len) {
        errln("Error reading test data file.");
        delete [] source;
        fclose(f);
        return FALSE;
    }
    fclose(f);

    // convert the UTF-8 encoded stream to UTF-16 stream
    UConverter* conv = ucnv_open("utf-8", &status);
    int dest_len = ucnv_toUChars(conv,
                                NULL,           //  dest,
                                0,              //  destCapacity,
                                source,
                                source_len,
                                &status);
    if (status == U_BUFFER_OVERFLOW_ERROR) {
        // Buffer Overflow is expected from the preflight operation.
        status = U_ZERO_ERROR;
        UChar * dest = NULL;
        dest = new UChar[ dest_len + 1];
        ucnv_toUChars(conv, dest, dest_len + 1, source, source_len, &status);
        // Do not know the "if possible" behavior of ucnv_toUChars()
        // Do it by ourself.
        dest[dest_len] = 0; 
        len = dest_len;
        base = dest;
        delete [] source;
        ucnv_close(conv);
        return TRUE;    // The buffer will owned by caller.
    }
    errln("UConverter error: %s", u_errorName(status));
    delete [] source;
    ucnv_close(conv);
    return FALSE;
}

int IdnaConfTest::isNewlineMark(){
    static const UChar LF        = 0x0a;
    static const UChar CR        = 0x0d;
    UChar c = base[curOffset];
    // CR LF
    if ( c == CR && curOffset + 1 < len && base[curOffset + 1] == LF){
        return 2;
    }

    // CR or LF
    if ( c == CR || c == LF) {
        return 1;
    }

    return 0;
}

/* Read a logical line.
 *
 * All lines ending in a backslash (\) and immediately followed by a newline 
 * character are joined with the next line in the source file forming logical
 * lines from the physical lines.
 *
 */
UBool IdnaConfTest::ReadOneLine(UnicodeString& buf){
    if ( !(curOffset < len) ) return FALSE; // stream end

    static const UChar BACKSLASH = 0x5c;
    buf.remove();
    int t = 0;
    while (curOffset < len){
        if ((t = isNewlineMark())) {  // end of line
            curOffset += t;
            break;
        }
        UChar c = base[curOffset];
        if (c == BACKSLASH && curOffset < len -1){  // escaped new line mark
            if ((t = isNewlineMark())){
                curOffset += 1 + t;  // BACKSLAH and NewlineMark
                continue;
            }
        };
        buf.append(c);
        curOffset++;
    }
    return TRUE;
}

//
//===============================================================
//

/* Explain <xxxxx> tag to a native value
 *
 * Since <xxxxx> is always larger than the native value,
 * the operation will replace the tag directly in the buffer,
 * and, of course, will shift tail elements.
 */
void IdnaConfTest::ExplainCodePointTag(UnicodeString& buf){
    buf.append((UChar)0);    // add a terminal NULL
    UChar* bufBase = buf.getBuffer(buf.length());
    UChar* p = bufBase;
    while (*p != 0){
        if ( *p != 0x3C){    // <
            *bufBase++ = *p++;
        } else {
            p++;    // skip <
            UChar32 cp = 0;
            for ( ;*p != 0x3E; p++){   // >
                if (0x30 <= *p && *p <= 0x39){        // 0-9
                    cp = (cp * 16) + (*p - 0x30);
                } else if (0x61 <= *p && *p <= 0x66){ // a-f
                    cp = (cp * 16) + (*p - 0x61) + 10;
                } else if (0x41 <= *p && *p <= 0x46) {// A-F
                    cp = (cp * 16) + (*p - 0x41) + 10;
                }
                // no else. hope everything is good.
            }
            p++;    // skip >
            if (U_IS_BMP(cp)){
                *bufBase++ = cp;
            } else {
                *bufBase++ = U16_LEAD(cp);
                *bufBase++ = U16_TRAIL(cp);
            }
        }
    }
    *bufBase = 0;  // close our buffer
    buf.releaseBuffer();
}

void IdnaConfTest::Call(){
    if (type == -1 || option == -1 || passfail == -1 || namebase.isBogus() || namezone.isBogus()){
        errln("Incomplete record");
    } else {
        UErrorCode status = U_ZERO_ERROR;
        UChar result[200] = {0,};   // simple life
        const UChar *p = namebase.getTerminatedBuffer();
        const int p_len = namebase.length();

        if (type == 0 && option == 0){
            uidna_IDNToASCII(p, p_len, result, 200, UIDNA_USE_STD3_RULES, NULL, &status);
        } else if (type == 0 && option == 1){
            uidna_IDNToASCII(p, p_len, result, 200, UIDNA_ALLOW_UNASSIGNED, NULL, &status);
        } else if (type == 1 && option == 0){
            uidna_IDNToUnicode(p, p_len, result, 200, UIDNA_USE_STD3_RULES, NULL, &status);
        } else if (type == 1 && option == 1){
            uidna_IDNToUnicode(p, p_len, result, 200, UIDNA_ALLOW_UNASSIGNED, NULL, &status);
        }
        if (passfail == 0){
            if (U_FAILURE(status)){
                id.append(" should pass, but failed. - ");
                id.append(u_errorName(status));
                errcheckln(status, id);
            } else{
                if (namezone.compare(result, -1) == 0){
                    // expected
                    logln(UnicodeString("namebase: ") + prettify(namebase) + UnicodeString(" result: ") + prettify(result));
                } else {
                    id.append(" no error, but result is not as expected.");
                    errln(id);
                }
            }
        } else if (passfail == 1){
            if (U_FAILURE(status)){
                // expected
                // TODO: Uncomment this when U_IDNA_ZERO_LENGTH_LABEL_ERROR is added to u_errorName
                //logln("Got the expected error: " + UnicodeString(u_errorName(status)));
            } else{
                if (namebase.compare(result, -1) == 0){
                    // garbage in -> garbage out
                    logln(UnicodeString("ICU will not recognize malformed ACE-Prefixes or incorrect ACE-Prefixes. ") + UnicodeString("namebase: ") + prettify(namebase) + UnicodeString(" result: ") + prettify(result));
                } else {
                    id.append(" should fail, but not failed. ");
                    id.append(u_errorName(status));
                    errln(id);
                }
            }
        }
    }
    type = option = passfail = -1;
    namebase.setToBogus();
    namezone.setToBogus();
    id.remove();
    return;
}

void IdnaConfTest::Test(void){
    if (!ReadAndConvertFile())return;

    UnicodeString s;
    UnicodeString key;
    UnicodeString value;

    // skip everything before the first "=====" and "=====" itself
    do {
        if (!ReadOneLine(s)) {
            errln("End of file prematurely found");
            break;
        }
    }
    while (s.compare(C_TAG, -1) != 0);   //"====="

    while(ReadOneLine(s)){
        s.trim();
        key.remove();
        value.remove();
        if (s.compare(C_TAG, -1) == 0){   //"====="
            Call();
       } else {
            // explain      key:value
            int p = s.indexOf((UChar)0x3A);    // :
            key.setTo(s,0,p).trim();
            value.setTo(s,p+1).trim();
            if (key.compare(C_TYPE, -1) == 0){
                if (value.compare(C_TOASCII, -1) == 0) {
                    type = 0;
                } else if (value.compare(C_TOUNICODE, -1) == 0){
                    type = 1;
                }
            } else if (key.compare(C_PASSFAIL, -1) == 0){
                if (value.compare(C_PASS, -1) == 0){
                    passfail = 0;
                } else if (value.compare(C_FAIL, -1) == 0){
                    passfail = 1;
                }
            } else if (key.compare(C_DESC, -1) == 0){
                if (value.indexOf(C_USESTD3ASCIIRULES, u_strlen(C_USESTD3ASCIIRULES), 0) == -1){
                    option = 1; // not found
                } else {
                    option = 0;
                }
                id.setTo(value, 0, value.indexOf((UChar)0x20));    // space
            } else if (key.compare(C_NAMEZONE, -1) == 0){
                ExplainCodePointTag(value);
                namezone.setTo(value);
            } else if (key.compare(C_NAMEBASE, -1) == 0){
                ExplainCodePointTag(value);
                namebase.setTo(value);
            }
            // just skip other lines
        }
    }

    Call(); // for last record
}
#else
void IdnaConfTest::Test(void)
{
  // test nothing...
}
#endif

void IdnaConfTest::runIndexedTest( int32_t index, UBool exec, const char* &name, char* /*par*/){
    switch (index) {
        TESTCASE(0,Test);
        default: name = ""; break;
    }
}

#endif
