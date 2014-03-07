/*
**********************************************************************
*   Copyright (C) 2001-2009, International Business Machines
*   Corporation and others.  All Rights Reserved.
**********************************************************************
*   Date        Name        Description
*   05/23/00    aliu        Creation.
**********************************************************************
*/

#include "unicode/unistr.h"
#include "testutil.h"

static const UChar HEX[16]={48,49,50,51,52,53,54,55,56,57,65,66,67,68,69,70};

UnicodeString &TestUtility::appendHex(UnicodeString &buf, UChar32 ch) {
    if (ch >= 0x10000) {
        if (ch >= 0x100000) {
            buf.append(HEX[0xF&(ch>>20)]);
        }
        buf.append(HEX[0xF&(ch>>16)]);
    }
    buf.append(HEX[0xF&(ch>>12)]);
    buf.append(HEX[0xF&(ch>>8)]);
    buf.append(HEX[0xF&(ch>>4)]);
    buf.append(HEX[0xF&ch]);
    return buf;
}

UnicodeString TestUtility::hex(UChar32 ch) {
    UnicodeString buf;
    appendHex(buf, ch);
    return buf;
}

UnicodeString TestUtility::hex(const UnicodeString& s) {
    return hex(s, 44 /*,*/);
}

UnicodeString TestUtility::hex(const UnicodeString& s, UChar sep) {
    UnicodeString result;
    if (s.isEmpty()) return result;
    UChar32 c;
    for (int32_t i = 0; i < s.length(); i += U16_LENGTH(c)) {
        c = s.char32At(i);
        if (i > 0) {
            result.append(sep);
        }
        appendHex(result, c);
    }
    return result;
}

UnicodeString TestUtility::hex(const uint8_t* bytes, int32_t len) {
	UnicodeString buf;
	for (int32_t i = 0; i < len; ++i) {
		buf.append(HEX[0x0F & (bytes[i] >> 4)]);
		buf.append(HEX[0x0F & bytes[i]]);
	}
	return buf;
}
