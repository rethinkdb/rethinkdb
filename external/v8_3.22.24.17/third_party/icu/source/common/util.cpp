/*
**********************************************************************
*   Copyright (c) 2001-2008, International Business Machines
*   Corporation and others.  All Rights Reserved.
**********************************************************************
*   Date        Name        Description
*   11/19/2001  aliu        Creation.
**********************************************************************
*/

#include "util.h"
#include "unicode/unimatch.h"
#include "unicode/uniset.h"

// Define UChar constants using hex for EBCDIC compatibility

static const UChar BACKSLASH  = 0x005C; /*\*/
static const UChar UPPER_U    = 0x0055; /*U*/
static const UChar LOWER_U    = 0x0075; /*u*/
static const UChar APOSTROPHE = 0x0027; // '\''
static const UChar SPACE      = 0x0020; // ' '

// "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"
static const UChar DIGITS[] = {
    48,49,50,51,52,53,54,55,56,57,
    65,66,67,68,69,70,71,72,73,74,
    75,76,77,78,79,80,81,82,83,84,
    85,86,87,88,89,90
};

U_NAMESPACE_BEGIN

UnicodeString& ICU_Utility::appendNumber(UnicodeString& result, int32_t n,
                                     int32_t radix, int32_t minDigits) {
    if (radix < 2 || radix > 36) {
        // Bogus radix
        return result.append((UChar)63/*?*/);
    }
    // Handle negatives
    if (n < 0) {
        n = -n;
        result.append((UChar)45/*-*/);
    }
    // First determine the number of digits
    int32_t nn = n;
    int32_t r = 1;
    while (nn >= radix) {
        nn /= radix;
        r *= radix;
        --minDigits;
    }
    // Now generate the digits
    while (--minDigits > 0) {
        result.append(DIGITS[0]);
    }
    while (r > 0) {
        int32_t digit = n / r;
        result.append(DIGITS[digit]);
        n -= digit * r;
        r /= radix;
    }
    return result;
}

/**
 * Return true if the character is NOT printable ASCII.
 */
UBool ICU_Utility::isUnprintable(UChar32 c) {
    return !(c >= 0x20 && c <= 0x7E);
}

/**
 * Escape unprintable characters using \uxxxx notation for U+0000 to
 * U+FFFF and \Uxxxxxxxx for U+10000 and above.  If the character is
 * printable ASCII, then do nothing and return FALSE.  Otherwise,
 * append the escaped notation and return TRUE.
 */
UBool ICU_Utility::escapeUnprintable(UnicodeString& result, UChar32 c) {
    if (isUnprintable(c)) {
        result.append(BACKSLASH);
        if (c & ~0xFFFF) {
            result.append(UPPER_U);
            result.append(DIGITS[0xF&(c>>28)]);
            result.append(DIGITS[0xF&(c>>24)]);
            result.append(DIGITS[0xF&(c>>20)]);
            result.append(DIGITS[0xF&(c>>16)]);
        } else {
            result.append(LOWER_U);
        }
        result.append(DIGITS[0xF&(c>>12)]);
        result.append(DIGITS[0xF&(c>>8)]);
        result.append(DIGITS[0xF&(c>>4)]);
        result.append(DIGITS[0xF&c]);
        return TRUE;
    }
    return FALSE;
}

/**
 * Returns the index of a character, ignoring quoted text.
 * For example, in the string "abc'hide'h", the 'h' in "hide" will not be
 * found by a search for 'h'.
 */
// FOR FUTURE USE.  DISABLE FOR NOW for coverage reasons.
/*
int32_t ICU_Utility::quotedIndexOf(const UnicodeString& text,
                               int32_t start, int32_t limit,
                               UChar charToFind) {
    for (int32_t i=start; i<limit; ++i) {
        UChar c = text.charAt(i);
        if (c == BACKSLASH) {
            ++i;
        } else if (c == APOSTROPHE) {
            while (++i < limit
                   && text.charAt(i) != APOSTROPHE) {}
        } else if (c == charToFind) {
            return i;
        }
    }
    return -1;
}
*/

/**
 * Skip over a sequence of zero or more white space characters at pos.
 * @param advance if true, advance pos to the first non-white-space
 * character at or after pos, or str.length(), if there is none.
 * Otherwise leave pos unchanged.
 * @return the index of the first non-white-space character at or
 * after pos, or str.length(), if there is none.
 */
int32_t ICU_Utility::skipWhitespace(const UnicodeString& str, int32_t& pos,
                                    UBool advance) {
    int32_t p = pos;
    while (p < str.length()) {
        UChar32 c = str.char32At(p);
        if (!uprv_isRuleWhiteSpace(c)) {
            break;
        }
        p += UTF_CHAR_LENGTH(c);
    }
    if (advance) {
        pos = p;
    }
    return p;
}

/**
 * Skip over whitespace in a Replaceable.  Whitespace is defined by
 * uprv_isRuleWhiteSpace().  Skipping may be done in the forward or
 * reverse direction.  In either case, the leftmost index will be
 * inclusive, and the rightmost index will be exclusive.  That is,
 * given a range defined as [start, limit), the call
 * skipWhitespace(text, start, limit) will advance start past leading
 * whitespace, whereas the call skipWhitespace(text, limit, start),
 * will back up limit past trailing whitespace.
 * @param text the text to be analyzed
 * @param pos either the start or limit of a range of 'text', to skip
 * leading or trailing whitespace, respectively
 * @param stop either the limit or start of a range of 'text', to skip
 * leading or trailing whitespace, respectively
 * @return the new start or limit, depending on what was passed in to
 * 'pos'
 */
//?FOR FUTURE USE.  DISABLE FOR NOW for coverage reasons.
//?int32_t ICU_Utility::skipWhitespace(const Replaceable& text,
//?                                    int32_t pos, int32_t stop) {
//?    UChar32 c;
//?    UBool isForward = (stop >= pos);
//?
//?    if (!isForward) {
//?        --pos; // pos is a limit, so back up by one
//?    }
//?    
//?    while (pos != stop &&
//?           uprv_isRuleWhiteSpace(c = text.char32At(pos))) {
//?        if (isForward) {
//?            pos += UTF_CHAR_LENGTH(c);
//?        } else {
//?            pos -= UTF_CHAR_LENGTH(c);
//?        }
//?    }
//?
//?    if (!isForward) {
//?        ++pos; // make pos back into a limit
//?    }
//?
//?    return pos;
//?}

/**
 * Parse a single non-whitespace character 'ch', optionally
 * preceded by whitespace.
 * @param id the string to be parsed
 * @param pos INPUT-OUTPUT parameter.  On input, pos[0] is the
 * offset of the first character to be parsed.  On output, pos[0]
 * is the index after the last parsed character.  If the parse
 * fails, pos[0] will be unchanged.
 * @param ch the non-whitespace character to be parsed.
 * @return true if 'ch' is seen preceded by zero or more
 * whitespace characters.
 */
UBool ICU_Utility::parseChar(const UnicodeString& id, int32_t& pos, UChar ch) {
    int32_t start = pos;
    skipWhitespace(id, pos, TRUE);
    if (pos == id.length() ||
        id.charAt(pos) != ch) {
        pos = start;
        return FALSE;
    }
    ++pos;
    return TRUE;
}

/**
 * Parse a pattern string within the given Replaceable and a parsing
 * pattern.  Characters are matched literally and case-sensitively
 * except for the following special characters:
 *
 * ~  zero or more uprv_isRuleWhiteSpace chars
 *
 * If end of pattern is reached with all matches along the way,
 * pos is advanced to the first unparsed index and returned.
 * Otherwise -1 is returned.
 * @param pat pattern that controls parsing
 * @param text text to be parsed, starting at index
 * @param index offset to first character to parse
 * @param limit offset after last character to parse
 * @return index after last parsed character, or -1 on parse failure.
 */
int32_t ICU_Utility::parsePattern(const UnicodeString& pat,
                                  const Replaceable& text,
                                  int32_t index,
                                  int32_t limit) {
    int32_t ipat = 0;

    // empty pattern matches immediately
    if (ipat == pat.length()) {
        return index;
    }

    UChar32 cpat = pat.char32At(ipat);

    while (index < limit) {
        UChar32 c = text.char32At(index);

        // parse \s*
        if (cpat == 126 /*~*/) {
            if (uprv_isRuleWhiteSpace(c)) {
                index += UTF_CHAR_LENGTH(c);
                continue;
            } else {
                if (++ipat == pat.length()) {
                    return index; // success; c unparsed
                }
                // fall thru; process c again with next cpat
            }
        }

        // parse literal
        else if (c == cpat) {
            index += UTF_CHAR_LENGTH(c);
            ipat += UTF_CHAR_LENGTH(cpat);
            if (ipat == pat.length()) {
                return index; // success; c parsed
            }
            // fall thru; get next cpat
        }

        // match failure of literal
        else {
            return -1;
        }

        cpat = pat.char32At(ipat);
    }

    return -1; // text ended before end of pat
}

/**
 * Append a character to a rule that is being built up.  To flush
 * the quoteBuf to rule, make one final call with isLiteral == TRUE.
 * If there is no final character, pass in (UChar32)-1 as c.
 * @param rule the string to append the character to
 * @param c the character to append, or (UChar32)-1 if none.
 * @param isLiteral if true, then the given character should not be
 * quoted or escaped.  Usually this means it is a syntactic element
 * such as > or $
 * @param escapeUnprintable if true, then unprintable characters
 * should be escaped using \uxxxx or \Uxxxxxxxx.  These escapes will
 * appear outside of quotes.
 * @param quoteBuf a buffer which is used to build up quoted
 * substrings.  The caller should initially supply an empty buffer,
 * and thereafter should not modify the buffer.  The buffer should be
 * cleared out by, at the end, calling this method with a literal
 * character.
 */
void ICU_Utility::appendToRule(UnicodeString& rule,
                               UChar32 c,
                               UBool isLiteral,
                               UBool escapeUnprintable,
                               UnicodeString& quoteBuf) {
    // If we are escaping unprintables, then escape them outside
    // quotes.  \u and \U are not recognized within quotes.  The same
    // logic applies to literals, but literals are never escaped.
    if (isLiteral ||
        (escapeUnprintable && ICU_Utility::isUnprintable(c))) {
        if (quoteBuf.length() > 0) {
            // We prefer backslash APOSTROPHE to double APOSTROPHE
            // (more readable, less similar to ") so if there are
            // double APOSTROPHEs at the ends, we pull them outside
            // of the quote.

            // If the first thing in the quoteBuf is APOSTROPHE
            // (doubled) then pull it out.
            while (quoteBuf.length() >= 2 &&
                   quoteBuf.charAt(0) == APOSTROPHE &&
                   quoteBuf.charAt(1) == APOSTROPHE) {
                rule.append(BACKSLASH).append(APOSTROPHE);
                quoteBuf.remove(0, 2);
            }
            // If the last thing in the quoteBuf is APOSTROPHE
            // (doubled) then remove and count it and add it after.
            int32_t trailingCount = 0;
            while (quoteBuf.length() >= 2 &&
                   quoteBuf.charAt(quoteBuf.length()-2) == APOSTROPHE &&
                   quoteBuf.charAt(quoteBuf.length()-1) == APOSTROPHE) {
                quoteBuf.truncate(quoteBuf.length()-2);
                ++trailingCount;
            }
            if (quoteBuf.length() > 0) {
                rule.append(APOSTROPHE);
                rule.append(quoteBuf);
                rule.append(APOSTROPHE);
                quoteBuf.truncate(0);
            }
            while (trailingCount-- > 0) {
                rule.append(BACKSLASH).append(APOSTROPHE);
            }
        }
        if (c != (UChar32)-1) {
            /* Since spaces are ignored during parsing, they are
             * emitted only for readability.  We emit one here
             * only if there isn't already one at the end of the
             * rule.
             */
            if (c == SPACE) {
                int32_t len = rule.length();
                if (len > 0 && rule.charAt(len-1) != c) {
                    rule.append(c);
                }
            } else if (!escapeUnprintable || !ICU_Utility::escapeUnprintable(rule, c)) {
                rule.append(c);
            }
        }
    }

    // Escape ' and '\' and don't begin a quote just for them
    else if (quoteBuf.length() == 0 &&
             (c == APOSTROPHE || c == BACKSLASH)) {
        rule.append(BACKSLASH);
        rule.append(c);
    }

    // Specials (printable ascii that isn't [0-9a-zA-Z]) and
    // whitespace need quoting.  Also append stuff to quotes if we are
    // building up a quoted substring already.
    else if (quoteBuf.length() > 0 ||
             (c >= 0x0021 && c <= 0x007E &&
              !((c >= 0x0030/*'0'*/ && c <= 0x0039/*'9'*/) ||
                (c >= 0x0041/*'A'*/ && c <= 0x005A/*'Z'*/) ||
                (c >= 0x0061/*'a'*/ && c <= 0x007A/*'z'*/))) ||
             uprv_isRuleWhiteSpace(c)) {
        quoteBuf.append(c);
        // Double ' within a quote
        if (c == APOSTROPHE) {
            quoteBuf.append(c);
        }
    }
    
    // Otherwise just append
    else {
        rule.append(c);
    }
}

void ICU_Utility::appendToRule(UnicodeString& rule,
                               const UnicodeString& text,
                               UBool isLiteral,
                               UBool escapeUnprintable,
                               UnicodeString& quoteBuf) {
    for (int32_t i=0; i<text.length(); ++i) {
        appendToRule(rule, text[i], isLiteral, escapeUnprintable, quoteBuf);
    }
}

/**
 * Given a matcher reference, which may be null, append its
 * pattern as a literal to the given rule.
 */
void ICU_Utility::appendToRule(UnicodeString& rule,
                               const UnicodeMatcher* matcher,
                               UBool escapeUnprintable,
                               UnicodeString& quoteBuf) {
    if (matcher != NULL) {
        UnicodeString pat;
        appendToRule(rule, matcher->toPattern(pat, escapeUnprintable),
                     TRUE, escapeUnprintable, quoteBuf);
    }
}

U_NAMESPACE_END

U_CAPI UBool U_EXPORT2
uprv_isRuleWhiteSpace(UChar32 c) {
    /* "white space" in the sense of ICU rule parsers
       This is a FIXED LIST that is NOT DEPENDENT ON UNICODE PROPERTIES.
       See UAX #31 Identifier and Pattern Syntax: http://www.unicode.org/reports/tr31/
       U+0009..U+000D, U+0020, U+0085, U+200E..U+200F, and U+2028..U+2029
       Equivalent to test for Pattern_White_Space Unicode property.
    */
    return (c >= 0x0009 && c <= 0x2029 &&
            (c <= 0x000D || c == 0x0020 || c == 0x0085 ||
             c == 0x200E || c == 0x200F || c >= 0x2028));
}

U_CAPI U_NAMESPACE_QUALIFIER UnicodeSet* U_EXPORT2
uprv_openRuleWhiteSpaceSet(UErrorCode* ec) {
    if(U_FAILURE(*ec)) {
        return NULL;
    }
    // create a set with the Pattern_White_Space characters,
    // without a pattern for fewer code dependencies
    U_NAMESPACE_QUALIFIER UnicodeSet *set=new U_NAMESPACE_QUALIFIER UnicodeSet(9, 0xd);
    // Check for new failure.
    if (set == NULL) {
        *ec = U_MEMORY_ALLOCATION_ERROR;
        return NULL;
    }
    set->UnicodeSet::add(0x20).add(0x85).add(0x200e, 0x200f).add(0x2028, 0x2029);
    return set;
}

//eof
