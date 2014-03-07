/*
**********************************************************************
* Copyright (c) 2003-2007, International Business Machines
* Corporation and others.  All Rights Reserved.
**********************************************************************
* Author: Alan Liu
* Created: September 24 2003
* Since: ICU 2.8
**********************************************************************
*/
#include "ruleiter.h"
#include "unicode/parsepos.h"
#include "unicode/unistr.h"
#include "unicode/symtable.h"
#include "util.h"

/* \U87654321 or \ud800\udc00 */
#define MAX_U_NOTATION_LEN 12

U_NAMESPACE_BEGIN

RuleCharacterIterator::RuleCharacterIterator(const UnicodeString& theText, const SymbolTable* theSym,
                      ParsePosition& thePos) :
    text(theText),
    pos(thePos),
    sym(theSym),
    buf(0),
    bufPos(0)
{}

UBool RuleCharacterIterator::atEnd() const {
    return buf == 0 && pos.getIndex() == text.length();
}

UChar32 RuleCharacterIterator::next(int32_t options, UBool& isEscaped, UErrorCode& ec) {
    if (U_FAILURE(ec)) return DONE;

    UChar32 c = DONE;
    isEscaped = FALSE;

    for (;;) {
        c = _current();
        _advance(UTF_CHAR_LENGTH(c));

        if (c == SymbolTable::SYMBOL_REF && buf == 0 &&
            (options & PARSE_VARIABLES) != 0 && sym != 0) {
            UnicodeString name = sym->parseReference(text, pos, text.length());
            // If name is empty there was an isolated SYMBOL_REF;
            // return it.  Caller must be prepared for this.
            if (name.length() == 0) {
                break;
            }
            bufPos = 0;
            buf = sym->lookup(name);
            if (buf == 0) {
                ec = U_UNDEFINED_VARIABLE;
                return DONE;
            }
            // Handle empty variable value
            if (buf->length() == 0) {
                buf = 0;
            }
            continue;
        }

        if ((options & SKIP_WHITESPACE) != 0 &&
            uprv_isRuleWhiteSpace(c)) {
            continue;
        }

        if (c == 0x5C /*'\\'*/ && (options & PARSE_ESCAPES) != 0) {
            UnicodeString tempEscape;
            int32_t offset = 0;
            c = lookahead(tempEscape, MAX_U_NOTATION_LEN).unescapeAt(offset);
            jumpahead(offset);
            isEscaped = TRUE;
            if (c < 0) {
                ec = U_MALFORMED_UNICODE_ESCAPE;
                return DONE;
            }
        }

        break;
    }

    return c;
}

void RuleCharacterIterator::getPos(RuleCharacterIterator::Pos& p) const {
    p.buf = buf;
    p.pos = pos.getIndex();
    p.bufPos = bufPos;
}

void RuleCharacterIterator::setPos(const RuleCharacterIterator::Pos& p) {
    buf = p.buf;
    pos.setIndex(p.pos);
    bufPos = p.bufPos;
}

void RuleCharacterIterator::skipIgnored(int32_t options) {
    if ((options & SKIP_WHITESPACE) != 0) {
        for (;;) {
            UChar32 a = _current();
            if (!uprv_isRuleWhiteSpace(a)) break;
            _advance(UTF_CHAR_LENGTH(a));
        }
    }
}

UnicodeString& RuleCharacterIterator::lookahead(UnicodeString& result, int32_t maxLookAhead) const {
    if (maxLookAhead < 0) {
        maxLookAhead = 0x7FFFFFFF;
    }
    if (buf != 0) {
        buf->extract(bufPos, maxLookAhead, result);
    } else {
        text.extract(pos.getIndex(), maxLookAhead, result);
    }
    return result;
}

void RuleCharacterIterator::jumpahead(int32_t count) {
    _advance(count);
}

/*
UnicodeString& RuleCharacterIterator::toString(UnicodeString& result) const {
    int32_t b = pos.getIndex();
    text.extract(0, b, result);
    return result.append((UChar) 0x7C).append(text, b, 0x7FFFFFFF); // Insert '|' at index
}
*/

UChar32 RuleCharacterIterator::_current() const {
    if (buf != 0) {
        return buf->char32At(bufPos);
    } else {
        int i = pos.getIndex();
        return (i < text.length()) ? text.char32At(i) : (UChar32)DONE;
    }
}

void RuleCharacterIterator::_advance(int32_t count) {
    if (buf != 0) {
        bufPos += count;
        if (bufPos == buf->length()) {
            buf = 0;
        }
    } else {
        pos.setIndex(pos.getIndex() + count);
        if (pos.getIndex() > text.length()) {
            pos.setIndex(text.length());
        }
    }
}

U_NAMESPACE_END

//eof
