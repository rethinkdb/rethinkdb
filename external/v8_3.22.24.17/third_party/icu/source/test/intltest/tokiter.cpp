/*
**********************************************************************
* Copyright (c) 2004, International Business Machines
* Corporation and others.  All Rights Reserved.
**********************************************************************
* Author: Alan Liu
* Created: March 22 2004
* Since: ICU 3.0
**********************************************************************
*/
#include "tokiter.h"
#include "textfile.h"
#include "util.h"
#include "uprops.h"

TokenIterator::TokenIterator(TextFile* r) {
    reader = r;
    done = haveLine = FALSE;
    pos = lastpos = -1;
}

TokenIterator::~TokenIterator() {
}

UBool TokenIterator::next(UnicodeString& token, UErrorCode& ec) {
    if (done || U_FAILURE(ec)) {
        return FALSE;
    }
    token.truncate(0);
    for (;;) {
        if (!haveLine) {
            if (!reader->readLineSkippingComments(line, ec)) {
                done = TRUE;
                return FALSE;
            }
            haveLine = TRUE;
            pos = 0;
        }
        lastpos = pos;
        if (!nextToken(token, ec)) {
            haveLine = FALSE;
            if (U_FAILURE(ec)) return FALSE;
            continue;
        }
        return TRUE;
    }
}

int32_t TokenIterator::getLineNumber() const {
    return reader->getLineNumber();
}

/**
 * Read the next token from 'this->line' and append it to 'token'.
 * Tokens are separated by rule white space.  Tokens may also be
 * delimited by double or single quotes.  The closing quote must match
 * the opening quote.  If a '#' is encountered, the rest of the line
 * is ignored, unless it is backslash-escaped or within quotes.
 * @param token the token is appended to this StringBuffer
 * @param ec input-output error code
 * @return TRUE if a valid token is found, or FALSE if the end
 * of the line is reached or an error occurs
 */
UBool TokenIterator::nextToken(UnicodeString& token, UErrorCode& ec) {
    ICU_Utility::skipWhitespace(line, pos, TRUE);
    if (pos == line.length()) {
        return FALSE;
    }
    UChar c = line.charAt(pos++);
    UChar quote = 0;
    switch (c) {
    case 34/*'"'*/:
    case 39/*'\\'*/:
        quote = c;
        break;
    case 35/*'#'*/:
        return FALSE;
    default:
        token.append(c);
        break;
    }
    while (pos < line.length()) {
        c = line.charAt(pos); // 16-bit ok
        if (c == 92/*'\\'*/) {
            UChar32 c32 = line.unescapeAt(pos);
            if (c32 < 0) {
                ec = U_MALFORMED_UNICODE_ESCAPE;
                return FALSE;
            }
            token.append(c32);
        } else if ((quote != 0 && c == quote) ||
                   (quote == 0 && uprv_isRuleWhiteSpace(c))) {
            ++pos;
            return TRUE;
        } else if (quote == 0 && c == '#') {
            return TRUE; // do NOT increment
        } else {
            token.append(c);
            ++pos;
        }
    }
    if (quote != 0) {
        ec = U_UNTERMINATED_QUOTE;
        return FALSE;
    }
    return TRUE;
}
