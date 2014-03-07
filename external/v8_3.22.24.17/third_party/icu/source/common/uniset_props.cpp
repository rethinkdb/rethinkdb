/*
*******************************************************************************
*
*   Copyright (C) 1999-2010, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  uniset_props.cpp
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2004aug25
*   created by: Markus W. Scherer
*
*   Character property dependent functions moved here from uniset.cpp
*/

#include "unicode/utypes.h"
#include "unicode/uniset.h"
#include "unicode/parsepos.h"
#include "unicode/uchar.h"
#include "unicode/uscript.h"
#include "unicode/symtable.h"
#include "unicode/uset.h"
#include "unicode/locid.h"
#include "unicode/brkiter.h"
#include "uset_imp.h"
#include "ruleiter.h"
#include "cmemory.h"
#include "ucln_cmn.h"
#include "util.h"
#include "uvector.h"
#include "uprops.h"
#include "propname.h"
#include "normalizer2impl.h"
#include "ucase.h"
#include "ubidi_props.h"
#include "uinvchar.h"
#include "uprops.h"
#include "charstr.h"
#include "cstring.h"
#include "mutex.h"
#include "umutex.h"
#include "uassert.h"
#include "hash.h"

U_NAMESPACE_USE

#define LENGTHOF(array) (int32_t)(sizeof(array)/sizeof((array)[0]))

// initial storage. Must be >= 0
// *** same as in uniset.cpp ! ***
#define START_EXTRA 16

// Define UChar constants using hex for EBCDIC compatibility
// Used #define to reduce private static exports and memory access time.
#define SET_OPEN        ((UChar)0x005B) /*[*/
#define SET_CLOSE       ((UChar)0x005D) /*]*/
#define HYPHEN          ((UChar)0x002D) /*-*/
#define COMPLEMENT      ((UChar)0x005E) /*^*/
#define COLON           ((UChar)0x003A) /*:*/
#define BACKSLASH       ((UChar)0x005C) /*\*/
#define INTERSECTION    ((UChar)0x0026) /*&*/
#define UPPER_U         ((UChar)0x0055) /*U*/
#define LOWER_U         ((UChar)0x0075) /*u*/
#define OPEN_BRACE      ((UChar)123)    /*{*/
#define CLOSE_BRACE     ((UChar)125)    /*}*/
#define UPPER_P         ((UChar)0x0050) /*P*/
#define LOWER_P         ((UChar)0x0070) /*p*/
#define UPPER_N         ((UChar)78)     /*N*/
#define EQUALS          ((UChar)0x003D) /*=*/

//static const UChar POSIX_OPEN[]  = { SET_OPEN,COLON,0 };  // "[:"
static const UChar POSIX_CLOSE[] = { COLON,SET_CLOSE,0 };  // ":]"
//static const UChar PERL_OPEN[]   = { BACKSLASH,LOWER_P,0 }; // "\\p"
static const UChar PERL_CLOSE[]  = { CLOSE_BRACE,0 };    // "}"
//static const UChar NAME_OPEN[]   = { BACKSLASH,UPPER_N,0 };  // "\\N"
static const UChar HYPHEN_RIGHT_BRACE[] = {HYPHEN,SET_CLOSE,0}; /*-]*/

// Special property set IDs
static const char ANY[]   = "ANY";   // [\u0000-\U0010FFFF]
static const char ASCII[] = "ASCII"; // [\u0000-\u007F]
static const char ASSIGNED[] = "Assigned"; // [:^Cn:]

// Unicode name property alias
#define NAME_PROP "na"
#define NAME_PROP_LENGTH 2

/**
 * Delimiter string used in patterns to close a category reference:
 * ":]".  Example: "[:Lu:]".
 */
//static const UChar CATEGORY_CLOSE[] = {COLON, SET_CLOSE, 0x0000}; /* ":]" */

// Cached sets ------------------------------------------------------------- ***

U_CDECL_BEGIN
static UBool U_CALLCONV uset_cleanup();
U_CDECL_END

// Not a TriStateSingletonWrapper because we think the UnicodeSet constructor
// can only fail with an out-of-memory error
// if we have a correct pattern and the properties data is hardcoded and always available.
class UnicodeSetSingleton : public SimpleSingletonWrapper<UnicodeSet> {
public:
    UnicodeSetSingleton(SimpleSingleton &s, const char *pattern) :
            SimpleSingletonWrapper<UnicodeSet>(s), fPattern(pattern) {}
    UnicodeSet *getInstance(UErrorCode &errorCode) {
        return SimpleSingletonWrapper<UnicodeSet>::getInstance(createInstance, fPattern, errorCode);
    }
private:
    static void *createInstance(const void *context, UErrorCode &errorCode) {
        UnicodeString pattern((const char *)context, -1, US_INV);
        UnicodeSet *set=new UnicodeSet(pattern, errorCode);
        if(set==NULL) {
            errorCode=U_MEMORY_ALLOCATION_ERROR;
        }
        set->freeze();
        ucln_common_registerCleanup(UCLN_COMMON_USET, uset_cleanup);
        return set;
    }

    const char *fPattern;
};

U_CDECL_BEGIN

static UnicodeSet *INCLUSIONS[UPROPS_SRC_COUNT] = { NULL }; // cached getInclusions()

STATIC_SIMPLE_SINGLETON(uni32Singleton);

//----------------------------------------------------------------
// Inclusions list
//----------------------------------------------------------------

// USetAdder implementation
// Does not use uset.h to reduce code dependencies
static void U_CALLCONV
_set_add(USet *set, UChar32 c) {
    ((UnicodeSet *)set)->add(c);
}

static void U_CALLCONV
_set_addRange(USet *set, UChar32 start, UChar32 end) {
    ((UnicodeSet *)set)->add(start, end);
}

static void U_CALLCONV
_set_addString(USet *set, const UChar *str, int32_t length) {
    ((UnicodeSet *)set)->add(UnicodeString((UBool)(length<0), str, length));
}

/**
 * Cleanup function for UnicodeSet
 */
static UBool U_CALLCONV uset_cleanup(void) {
    int32_t i;

    for(i = UPROPS_SRC_NONE; i < UPROPS_SRC_COUNT; ++i) {
        if (INCLUSIONS[i] != NULL) {
            delete INCLUSIONS[i];
            INCLUSIONS[i] = NULL;
        }
    }
    UnicodeSetSingleton(uni32Singleton, NULL).deleteInstance();
    return TRUE;
}

U_CDECL_END

U_NAMESPACE_BEGIN

/*
Reduce excessive reallocation, and make it easier to detect initialization
problems.
Usually you don't see smaller sets than this for Unicode 5.0.
*/
#define DEFAULT_INCLUSION_CAPACITY 3072

const UnicodeSet* UnicodeSet::getInclusions(int32_t src, UErrorCode &status) {
    UBool needInit;
    UMTX_CHECK(NULL, (INCLUSIONS[src] == NULL), needInit);
    if (needInit) {
        UnicodeSet* incl = new UnicodeSet();
        USetAdder sa = {
            (USet *)incl,
            _set_add,
            _set_addRange,
            _set_addString,
            NULL, // don't need remove()
            NULL // don't need removeRange()
        };
        incl->ensureCapacity(DEFAULT_INCLUSION_CAPACITY, status);
        if (incl != NULL) {
            switch(src) {
            case UPROPS_SRC_CHAR:
                uchar_addPropertyStarts(&sa, &status);
                break;
            case UPROPS_SRC_PROPSVEC:
                upropsvec_addPropertyStarts(&sa, &status);
                break;
            case UPROPS_SRC_CHAR_AND_PROPSVEC:
                uchar_addPropertyStarts(&sa, &status);
                upropsvec_addPropertyStarts(&sa, &status);
                break;
#if !UCONFIG_NO_NORMALIZATION
            case UPROPS_SRC_CASE_AND_NORM: {
                const Normalizer2Impl *impl=Normalizer2Factory::getNFCImpl(status);
                if(U_SUCCESS(status)) {
                    impl->addPropertyStarts(&sa, status);
                }
                ucase_addPropertyStarts(ucase_getSingleton(), &sa, &status);
                break;
            }
            case UPROPS_SRC_NFC: {
                const Normalizer2Impl *impl=Normalizer2Factory::getNFCImpl(status);
                if(U_SUCCESS(status)) {
                    impl->addPropertyStarts(&sa, status);
                }
                break;
            }
            case UPROPS_SRC_NFKC: {
                const Normalizer2Impl *impl=Normalizer2Factory::getNFKCImpl(status);
                if(U_SUCCESS(status)) {
                    impl->addPropertyStarts(&sa, status);
                }
                break;
            }
            case UPROPS_SRC_NFKC_CF: {
                const Normalizer2Impl *impl=Normalizer2Factory::getNFKC_CFImpl(status);
                if(U_SUCCESS(status)) {
                    impl->addPropertyStarts(&sa, status);
                }
                break;
            }
            case UPROPS_SRC_NFC_CANON_ITER: {
                const Normalizer2Impl *impl=Normalizer2Factory::getNFCImpl(status);
                if(U_SUCCESS(status)) {
                    impl->addCanonIterPropertyStarts(&sa, status);
                }
                break;
            }
#endif
            case UPROPS_SRC_CASE:
                ucase_addPropertyStarts(ucase_getSingleton(), &sa, &status);
                break;
            case UPROPS_SRC_BIDI:
                ubidi_addPropertyStarts(ubidi_getSingleton(), &sa, &status);
                break;
            default:
                status = U_INTERNAL_PROGRAM_ERROR;
                break;
            }
            if (U_SUCCESS(status)) {
                // Compact for caching
                incl->compact();
                umtx_lock(NULL);
                if (INCLUSIONS[src] == NULL) {
                    INCLUSIONS[src] = incl;
                    incl = NULL;
                    ucln_common_registerCleanup(UCLN_COMMON_USET, uset_cleanup);
                }
                umtx_unlock(NULL);
            }
            delete incl;
        } else {
            status = U_MEMORY_ALLOCATION_ERROR;
        }
    }
    return INCLUSIONS[src];
}

// Cache some sets for other services -------------------------------------- ***

U_CFUNC UnicodeSet *
uniset_getUnicode32Instance(UErrorCode &errorCode) {
    return UnicodeSetSingleton(uni32Singleton, "[:age=3.2:]").getInstance(errorCode);
}

// helper functions for matching of pattern syntax pieces ------------------ ***
// these functions are parallel to the PERL_OPEN etc. strings above

// using these functions is not only faster than UnicodeString::compare() and
// caseCompare(), but they also make UnicodeSet work for simple patterns when
// no Unicode properties data is available - when caseCompare() fails

static inline UBool
isPerlOpen(const UnicodeString &pattern, int32_t pos) {
    UChar c;
    return pattern.charAt(pos)==BACKSLASH && ((c=pattern.charAt(pos+1))==LOWER_P || c==UPPER_P);
}

/*static inline UBool
isPerlClose(const UnicodeString &pattern, int32_t pos) {
    return pattern.charAt(pos)==CLOSE_BRACE;
}*/

static inline UBool
isNameOpen(const UnicodeString &pattern, int32_t pos) {
    return pattern.charAt(pos)==BACKSLASH && pattern.charAt(pos+1)==UPPER_N;
}

static inline UBool
isPOSIXOpen(const UnicodeString &pattern, int32_t pos) {
    return pattern.charAt(pos)==SET_OPEN && pattern.charAt(pos+1)==COLON;
}

/*static inline UBool
isPOSIXClose(const UnicodeString &pattern, int32_t pos) {
    return pattern.charAt(pos)==COLON && pattern.charAt(pos+1)==SET_CLOSE;
}*/

// TODO memory debugging provided inside uniset.cpp
// could be made available here but probably obsolete with use of modern
// memory leak checker tools
#define _dbgct(me)

//----------------------------------------------------------------
// Constructors &c
//----------------------------------------------------------------

/**
 * Constructs a set from the given pattern, optionally ignoring
 * white space.  See the class description for the syntax of the
 * pattern language.
 * @param pattern a string specifying what characters are in the set
 */
UnicodeSet::UnicodeSet(const UnicodeString& pattern,
                       UErrorCode& status) :
    len(0), capacity(START_EXTRA), list(0), bmpSet(0), buffer(0),
    bufferCapacity(0), patLen(0), pat(NULL), strings(NULL), stringSpan(NULL),
    fFlags(0)
{   
    if(U_SUCCESS(status)){
        list = (UChar32*) uprv_malloc(sizeof(UChar32) * capacity);
        /* test for NULL */
        if(list == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;  
        }else{
            allocateStrings(status);
            applyPattern(pattern, USET_IGNORE_SPACE, NULL, status);
        }
    }
    _dbgct(this);
}

/**
 * Constructs a set from the given pattern, optionally ignoring
 * white space.  See the class description for the syntax of the
 * pattern language.
 * @param pattern a string specifying what characters are in the set
 * @param options bitmask for options to apply to the pattern.
 * Valid options are USET_IGNORE_SPACE and USET_CASE_INSENSITIVE.
 */
UnicodeSet::UnicodeSet(const UnicodeString& pattern,
                       uint32_t options,
                       const SymbolTable* symbols,
                       UErrorCode& status) :
    len(0), capacity(START_EXTRA), list(0), bmpSet(0), buffer(0),
    bufferCapacity(0), patLen(0), pat(NULL), strings(NULL), stringSpan(NULL),
    fFlags(0)
{   
    if(U_SUCCESS(status)){
        list = (UChar32*) uprv_malloc(sizeof(UChar32) * capacity);
        /* test for NULL */
        if(list == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;  
        }else{
            allocateStrings(status);
            applyPattern(pattern, options, symbols, status);
        }
    }
    _dbgct(this);
}

UnicodeSet::UnicodeSet(const UnicodeString& pattern, ParsePosition& pos,
                       uint32_t options,
                       const SymbolTable* symbols,
                       UErrorCode& status) :
    len(0), capacity(START_EXTRA), list(0), bmpSet(0), buffer(0),
    bufferCapacity(0), patLen(0), pat(NULL), strings(NULL), stringSpan(NULL),
    fFlags(0)
{
    if(U_SUCCESS(status)){
        list = (UChar32*) uprv_malloc(sizeof(UChar32) * capacity);
        /* test for NULL */
        if(list == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;   
        }else{
            allocateStrings(status);
            applyPattern(pattern, pos, options, symbols, status);
        }
    }
    _dbgct(this);
}

//----------------------------------------------------------------
// Public API
//----------------------------------------------------------------

/**
 * Modifies this set to represent the set specified by the given
 * pattern, optionally ignoring white space.  See the class
 * description for the syntax of the pattern language.
 * @param pattern a string specifying what characters are in the set
 * @param ignoreSpaces if <code>true</code>, all spaces in the
 * pattern are ignored.  Spaces are those characters for which
 * <code>uprv_isRuleWhiteSpace()</code> is <code>true</code>.
 * Characters preceded by '\\' are escaped, losing any special
 * meaning they otherwise have.  Spaces may be included by
 * escaping them.
 * @exception <code>IllegalArgumentException</code> if the pattern
 * contains a syntax error.
 */
UnicodeSet& UnicodeSet::applyPattern(const UnicodeString& pattern,
                                     UErrorCode& status) {
    return applyPattern(pattern, USET_IGNORE_SPACE, NULL, status);
}


/**
 * Modifies this set to represent the set specified by the given
 * pattern, optionally ignoring white space.  See the class
 * description for the syntax of the pattern language.
 * @param pattern a string specifying what characters are in the set
 * @param options bitmask for options to apply to the pattern.
 * Valid options are USET_IGNORE_SPACE and USET_CASE_INSENSITIVE.
 */
UnicodeSet& UnicodeSet::applyPattern(const UnicodeString& pattern,
                                     uint32_t options,
                                     const SymbolTable* symbols,
                                     UErrorCode& status) {
    if (U_FAILURE(status) || isFrozen()) {
        return *this;
    }

    ParsePosition pos(0);
    applyPattern(pattern, pos, options, symbols, status);
    if (U_FAILURE(status)) return *this;

    int32_t i = pos.getIndex();

    if (options & USET_IGNORE_SPACE) {
        // Skip over trailing whitespace
        ICU_Utility::skipWhitespace(pattern, i, TRUE);
    }

    if (i != pattern.length()) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
    }
    return *this;
}

UnicodeSet& UnicodeSet::applyPattern(const UnicodeString& pattern,
                              ParsePosition& pos,
                              uint32_t options,
                              const SymbolTable* symbols,
                              UErrorCode& status) {
    if (U_FAILURE(status) || isFrozen()) {
        return *this;
    }
    // Need to build the pattern in a temporary string because
    // _applyPattern calls add() etc., which set pat to empty.
    UnicodeString rebuiltPat;
    RuleCharacterIterator chars(pattern, symbols, pos);
    applyPattern(chars, symbols, rebuiltPat, options, status);
    if (U_FAILURE(status)) return *this;
    if (chars.inVariable()) {
        // syntaxError(chars, "Extra chars in variable value");
        status = U_MALFORMED_SET;
        return *this;
    }
    setPattern(rebuiltPat);
    return *this;
}

/**
 * Return true if the given position, in the given pattern, appears
 * to be the start of a UnicodeSet pattern.
 */
UBool UnicodeSet::resemblesPattern(const UnicodeString& pattern, int32_t pos) {
    return ((pos+1) < pattern.length() &&
            pattern.charAt(pos) == (UChar)91/*[*/) ||
        resemblesPropertyPattern(pattern, pos);
}

//----------------------------------------------------------------
// Implementation: Pattern parsing
//----------------------------------------------------------------

/**
 * A small all-inline class to manage a UnicodeSet pointer.  Add
 * operator->() etc. as needed.
 */
class UnicodeSetPointer {
    UnicodeSet* p;
public:
    inline UnicodeSetPointer() : p(0) {}
    inline ~UnicodeSetPointer() { delete p; }
    inline UnicodeSet* pointer() { return p; }
    inline UBool allocate() {
        if (p == 0) {
            p = new UnicodeSet();
        }
        return p != 0;
    }
};

/**
 * Parse the pattern from the given RuleCharacterIterator.  The
 * iterator is advanced over the parsed pattern.
 * @param chars iterator over the pattern characters.  Upon return
 * it will be advanced to the first character after the parsed
 * pattern, or the end of the iteration if all characters are
 * parsed.
 * @param symbols symbol table to use to parse and dereference
 * variables, or null if none.
 * @param rebuiltPat the pattern that was parsed, rebuilt or
 * copied from the input pattern, as appropriate.
 * @param options a bit mask of zero or more of the following:
 * IGNORE_SPACE, CASE.
 */
void UnicodeSet::applyPattern(RuleCharacterIterator& chars,
                              const SymbolTable* symbols,
                              UnicodeString& rebuiltPat,
                              uint32_t options,
                              UErrorCode& ec) {
    if (U_FAILURE(ec)) return;

    // Syntax characters: [ ] ^ - & { }

    // Recognized special forms for chars, sets: c-c s-s s&s

    int32_t opts = RuleCharacterIterator::PARSE_VARIABLES |
                   RuleCharacterIterator::PARSE_ESCAPES;
    if ((options & USET_IGNORE_SPACE) != 0) {
        opts |= RuleCharacterIterator::SKIP_WHITESPACE;
    }

    UnicodeString patLocal, buf;
    UBool usePat = FALSE;
    UnicodeSetPointer scratch;
    RuleCharacterIterator::Pos backup;

    // mode: 0=before [, 1=between [...], 2=after ]
    // lastItem: 0=none, 1=char, 2=set
    int8_t lastItem = 0, mode = 0;
    UChar32 lastChar = 0;
    UChar op = 0;

    UBool invert = FALSE;

    clear();

    while (mode != 2 && !chars.atEnd()) {
        U_ASSERT((lastItem == 0 && op == 0) ||
                 (lastItem == 1 && (op == 0 || op == HYPHEN /*'-'*/)) ||
                 (lastItem == 2 && (op == 0 || op == HYPHEN /*'-'*/ ||
                                    op == INTERSECTION /*'&'*/)));

        UChar32 c = 0;
        UBool literal = FALSE;
        UnicodeSet* nested = 0; // alias - do not delete

        // -------- Check for property pattern

        // setMode: 0=none, 1=unicodeset, 2=propertypat, 3=preparsed
        int8_t setMode = 0;
        if (resemblesPropertyPattern(chars, opts)) {
            setMode = 2;
        }

        // -------- Parse '[' of opening delimiter OR nested set.
        // If there is a nested set, use `setMode' to define how
        // the set should be parsed.  If the '[' is part of the
        // opening delimiter for this pattern, parse special
        // strings "[", "[^", "[-", and "[^-".  Check for stand-in
        // characters representing a nested set in the symbol
        // table.

        else {
            // Prepare to backup if necessary
            chars.getPos(backup);
            c = chars.next(opts, literal, ec);
            if (U_FAILURE(ec)) return;

            if (c == 0x5B /*'['*/ && !literal) {
                if (mode == 1) {
                    chars.setPos(backup); // backup
                    setMode = 1;
                } else {
                    // Handle opening '[' delimiter
                    mode = 1;
                    patLocal.append((UChar) 0x5B /*'['*/);
                    chars.getPos(backup); // prepare to backup
                    c = chars.next(opts, literal, ec); 
                    if (U_FAILURE(ec)) return;
                    if (c == 0x5E /*'^'*/ && !literal) {
                        invert = TRUE;
                        patLocal.append((UChar) 0x5E /*'^'*/);
                        chars.getPos(backup); // prepare to backup
                        c = chars.next(opts, literal, ec);
                        if (U_FAILURE(ec)) return;
                    }
                    // Fall through to handle special leading '-';
                    // otherwise restart loop for nested [], \p{}, etc.
                    if (c == HYPHEN /*'-'*/) {
                        literal = TRUE;
                        // Fall through to handle literal '-' below
                    } else {
                        chars.setPos(backup); // backup
                        continue;
                    }
                }
            } else if (symbols != 0) {
                const UnicodeFunctor *m = symbols->lookupMatcher(c);
                if (m != 0) {
                    const UnicodeSet *ms = dynamic_cast<const UnicodeSet *>(m);
                    if (ms == NULL) {
                        ec = U_MALFORMED_SET;
                        return;
                    }
                    // casting away const, but `nested' won't be modified
                    // (important not to modify stored set)
                    nested = const_cast<UnicodeSet*>(ms);
                    setMode = 3;
                }
            }
        }

        // -------- Handle a nested set.  This either is inline in
        // the pattern or represented by a stand-in that has
        // previously been parsed and was looked up in the symbol
        // table.

        if (setMode != 0) {
            if (lastItem == 1) {
                if (op != 0) {
                    // syntaxError(chars, "Char expected after operator");
                    ec = U_MALFORMED_SET;
                    return;
                }
                add(lastChar, lastChar);
                _appendToPat(patLocal, lastChar, FALSE);
                lastItem = 0;
                op = 0;
            }

            if (op == HYPHEN /*'-'*/ || op == INTERSECTION /*'&'*/) {
                patLocal.append(op);
            }

            if (nested == 0) {
                // lazy allocation
                if (!scratch.allocate()) {
                    ec = U_MEMORY_ALLOCATION_ERROR;
                    return;
                }
                nested = scratch.pointer();
            }
            switch (setMode) {
            case 1:
                nested->applyPattern(chars, symbols, patLocal, options, ec);
                break;
            case 2:
                chars.skipIgnored(opts);
                nested->applyPropertyPattern(chars, patLocal, ec);
                if (U_FAILURE(ec)) return;
                break;
            case 3: // `nested' already parsed
                nested->_toPattern(patLocal, FALSE);
                break;
            }

            usePat = TRUE;

            if (mode == 0) {
                // Entire pattern is a category; leave parse loop
                *this = *nested;
                mode = 2;
                break;
            }

            switch (op) {
            case HYPHEN: /*'-'*/
                removeAll(*nested);
                break;
            case INTERSECTION: /*'&'*/
                retainAll(*nested);
                break;
            case 0:
                addAll(*nested);
                break;
            }

            op = 0;
            lastItem = 2;

            continue;
        }

        if (mode == 0) {
            // syntaxError(chars, "Missing '['");
            ec = U_MALFORMED_SET;
            return;
        }

        // -------- Parse special (syntax) characters.  If the
        // current character is not special, or if it is escaped,
        // then fall through and handle it below.

        if (!literal) {
            switch (c) {
            case 0x5D /*']'*/:
                if (lastItem == 1) {
                    add(lastChar, lastChar);
                    _appendToPat(patLocal, lastChar, FALSE);
                }
                // Treat final trailing '-' as a literal
                if (op == HYPHEN /*'-'*/) {
                    add(op, op);
                    patLocal.append(op);
                } else if (op == INTERSECTION /*'&'*/) {
                    // syntaxError(chars, "Trailing '&'");
                    ec = U_MALFORMED_SET;
                    return;
                }
                patLocal.append((UChar) 0x5D /*']'*/);
                mode = 2;
                continue;
            case HYPHEN /*'-'*/:
                if (op == 0) {
                    if (lastItem != 0) {
                        op = (UChar) c;
                        continue;
                    } else {
                        // Treat final trailing '-' as a literal
                        add(c, c);
                        c = chars.next(opts, literal, ec);
                        if (U_FAILURE(ec)) return;
                        if (c == 0x5D /*']'*/ && !literal) {
                            patLocal.append(HYPHEN_RIGHT_BRACE);
                            mode = 2;
                            continue;
                        }
                    }
                }
                // syntaxError(chars, "'-' not after char or set");
                ec = U_MALFORMED_SET;
                return;
            case INTERSECTION /*'&'*/:
                if (lastItem == 2 && op == 0) {
                    op = (UChar) c;
                    continue;
                }
                // syntaxError(chars, "'&' not after set");
                ec = U_MALFORMED_SET;
                return;
            case 0x5E /*'^'*/:
                // syntaxError(chars, "'^' not after '['");
                ec = U_MALFORMED_SET;
                return;
            case 0x7B /*'{'*/:
                if (op != 0) {
                    // syntaxError(chars, "Missing operand after operator");
                    ec = U_MALFORMED_SET;
                    return;
                }
                if (lastItem == 1) {
                    add(lastChar, lastChar);
                    _appendToPat(patLocal, lastChar, FALSE);
                }
                lastItem = 0;
                buf.truncate(0);
                {
                    UBool ok = FALSE;
                    while (!chars.atEnd()) {
                        c = chars.next(opts, literal, ec);
                        if (U_FAILURE(ec)) return;
                        if (c == 0x7D /*'}'*/ && !literal) {
                            ok = TRUE;
                            break;
                        }
                        buf.append(c);
                    }
                    if (buf.length() < 1 || !ok) {
                        // syntaxError(chars, "Invalid multicharacter string");
                        ec = U_MALFORMED_SET;
                        return;
                    }
                }
                // We have new string. Add it to set and continue;
                // we don't need to drop through to the further
                // processing
                add(buf);
                patLocal.append((UChar) 0x7B /*'{'*/);
                _appendToPat(patLocal, buf, FALSE);
                patLocal.append((UChar) 0x7D /*'}'*/);
                continue;
            case SymbolTable::SYMBOL_REF:
                //         symbols  nosymbols
                // [a-$]   error    error (ambiguous)
                // [a$]    anchor   anchor
                // [a-$x]  var "x"* literal '$'
                // [a-$.]  error    literal '$'
                // *We won't get here in the case of var "x"
                {
                    chars.getPos(backup);
                    c = chars.next(opts, literal, ec);
                    if (U_FAILURE(ec)) return;
                    UBool anchor = (c == 0x5D /*']'*/ && !literal);
                    if (symbols == 0 && !anchor) {
                        c = SymbolTable::SYMBOL_REF;
                        chars.setPos(backup);
                        break; // literal '$'
                    }
                    if (anchor && op == 0) {
                        if (lastItem == 1) {
                            add(lastChar, lastChar);
                            _appendToPat(patLocal, lastChar, FALSE);
                        }
                        add(U_ETHER);
                        usePat = TRUE;
                        patLocal.append((UChar) SymbolTable::SYMBOL_REF);
                        patLocal.append((UChar) 0x5D /*']'*/);
                        mode = 2;
                        continue;
                    }
                    // syntaxError(chars, "Unquoted '$'");
                    ec = U_MALFORMED_SET;
                    return;
                }
            default:
                break;
            }
        }

        // -------- Parse literal characters.  This includes both
        // escaped chars ("\u4E01") and non-syntax characters
        // ("a").

        switch (lastItem) {
        case 0:
            lastItem = 1;
            lastChar = c;
            break;
        case 1:
            if (op == HYPHEN /*'-'*/) {
                if (lastChar >= c) {
                    // Don't allow redundant (a-a) or empty (b-a) ranges;
                    // these are most likely typos.
                    // syntaxError(chars, "Invalid range");
                    ec = U_MALFORMED_SET;
                    return;
                }
                add(lastChar, c);
                _appendToPat(patLocal, lastChar, FALSE);
                patLocal.append(op);
                _appendToPat(patLocal, c, FALSE);
                lastItem = 0;
                op = 0;
            } else {
                add(lastChar, lastChar);
                _appendToPat(patLocal, lastChar, FALSE);
                lastChar = c;
            }
            break;
        case 2:
            if (op != 0) {
                // syntaxError(chars, "Set expected after operator");
                ec = U_MALFORMED_SET;
                return;
            }
            lastChar = c;
            lastItem = 1;
            break;
        }
    }

    if (mode != 2) {
        // syntaxError(chars, "Missing ']'");
        ec = U_MALFORMED_SET;
        return;
    }

    chars.skipIgnored(opts);

    /**
     * Handle global flags (invert, case insensitivity).  If this
     * pattern should be compiled case-insensitive, then we need
     * to close over case BEFORE COMPLEMENTING.  This makes
     * patterns like /[^abc]/i work.
     */
    if ((options & USET_CASE_INSENSITIVE) != 0) {
        closeOver(USET_CASE_INSENSITIVE);
    }
    else if ((options & USET_ADD_CASE_MAPPINGS) != 0) {
        closeOver(USET_ADD_CASE_MAPPINGS);
    }
    if (invert) {
        complement();
    }

    // Use the rebuilt pattern (patLocal) only if necessary.  Prefer the
    // generated pattern.
    if (usePat) {
        rebuiltPat.append(patLocal);
    } else {
        _generatePattern(rebuiltPat, FALSE);
    }
    if (isBogus() && U_SUCCESS(ec)) {
        // We likely ran out of memory. AHHH!
        ec = U_MEMORY_ALLOCATION_ERROR;
    }
}

//----------------------------------------------------------------
// Property set implementation
//----------------------------------------------------------------

static UBool numericValueFilter(UChar32 ch, void* context) {
    return u_getNumericValue(ch) == *(double*)context;
}

static UBool generalCategoryMaskFilter(UChar32 ch, void* context) {
    int32_t value = *(int32_t*)context;
    return (U_GET_GC_MASK((UChar32) ch) & value) != 0;
}

static UBool versionFilter(UChar32 ch, void* context) {
    static const UVersionInfo none = { 0, 0, 0, 0 };
    UVersionInfo v;
    u_charAge(ch, v);
    UVersionInfo* version = (UVersionInfo*)context;
    return uprv_memcmp(&v, &none, sizeof(v)) > 0 && uprv_memcmp(&v, version, sizeof(v)) <= 0;
}

typedef struct {
    UProperty prop;
    int32_t value;
} IntPropertyContext;

static UBool intPropertyFilter(UChar32 ch, void* context) {
    IntPropertyContext* c = (IntPropertyContext*)context;
    return u_getIntPropertyValue((UChar32) ch, c->prop) == c->value;
}

static UBool scriptExtensionsFilter(UChar32 ch, void* context) {
    return uscript_hasScript(ch, *(UScriptCode*)context);
}

/**
 * Generic filter-based scanning code for UCD property UnicodeSets.
 */
void UnicodeSet::applyFilter(UnicodeSet::Filter filter,
                             void* context,
                             int32_t src,
                             UErrorCode &status) {
    if (U_FAILURE(status)) return;

    // Logically, walk through all Unicode characters, noting the start
    // and end of each range for which filter.contain(c) is
    // true.  Add each range to a set.
    //
    // To improve performance, use an inclusions set which
    // encodes information about character ranges that are known
    // to have identical properties.
    // getInclusions(src) contains exactly the first characters of
    // same-value ranges for the given properties "source".
    const UnicodeSet* inclusions = getInclusions(src, status);
    if (U_FAILURE(status)) {
        return;
    }

    clear();

    UChar32 startHasProperty = -1;
    int32_t limitRange = inclusions->getRangeCount();

    for (int j=0; j<limitRange; ++j) {
        // get current range
        UChar32 start = inclusions->getRangeStart(j);
        UChar32 end = inclusions->getRangeEnd(j);

        // for all the code points in the range, process
        for (UChar32 ch = start; ch <= end; ++ch) {
            // only add to this UnicodeSet on inflection points --
            // where the hasProperty value changes to false
            if ((*filter)(ch, context)) {
                if (startHasProperty < 0) {
                    startHasProperty = ch;
                }
            } else if (startHasProperty >= 0) {
                add(startHasProperty, ch-1);
                startHasProperty = -1;
            }
        }
    }
    if (startHasProperty >= 0) {
        add((UChar32)startHasProperty, (UChar32)0x10FFFF);
    }
    if (isBogus() && U_SUCCESS(status)) {
        // We likely ran out of memory. AHHH!
        status = U_MEMORY_ALLOCATION_ERROR;
    }
}

static UBool mungeCharName(char* dst, const char* src, int32_t dstCapacity) {
    /* Note: we use ' ' in compiler code page */
    int32_t j = 0;
    char ch;
    --dstCapacity; /* make room for term. zero */
    while ((ch = *src++) != 0) {
        if (ch == ' ' && (j==0 || (j>0 && dst[j-1]==' '))) {
            continue;
        }
        if (j >= dstCapacity) return FALSE;
        dst[j++] = ch;
    }
    if (j > 0 && dst[j-1] == ' ') --j;
    dst[j] = 0;
    return TRUE;
}

//----------------------------------------------------------------
// Property set API
//----------------------------------------------------------------

#define FAIL(ec) {ec=U_ILLEGAL_ARGUMENT_ERROR; return *this;}

UnicodeSet&
UnicodeSet::applyIntPropertyValue(UProperty prop, int32_t value, UErrorCode& ec) {
    if (U_FAILURE(ec) || isFrozen()) return *this;

    if (prop == UCHAR_GENERAL_CATEGORY_MASK) {
        applyFilter(generalCategoryMaskFilter, &value, UPROPS_SRC_CHAR, ec);
    } else if (prop == UCHAR_SCRIPT_EXTENSIONS) {
        UScriptCode script = (UScriptCode)value;
        applyFilter(scriptExtensionsFilter, &script, UPROPS_SRC_PROPSVEC, ec);
    } else {
        IntPropertyContext c = {prop, value};
        applyFilter(intPropertyFilter, &c, uprops_getSource(prop), ec);
    }
    return *this;
}

UnicodeSet&
UnicodeSet::applyPropertyAlias(const UnicodeString& prop,
                               const UnicodeString& value,
                               UErrorCode& ec) {
    if (U_FAILURE(ec) || isFrozen()) return *this;

    // prop and value used to be converted to char * using the default
    // converter instead of the invariant conversion.
    // This should not be necessary because all Unicode property and value
    // names use only invariant characters.
    // If there are any variant characters, then we won't find them anyway.
    // Checking first avoids assertion failures in the conversion.
    if( !uprv_isInvariantUString(prop.getBuffer(), prop.length()) ||
        !uprv_isInvariantUString(value.getBuffer(), value.length())
    ) {
        FAIL(ec);
    }
    CharString pname, vname;
    pname.appendInvariantChars(prop, ec);
    vname.appendInvariantChars(value, ec);
    if (U_FAILURE(ec)) return *this;

    UProperty p;
    int32_t v;
    UBool mustNotBeEmpty = FALSE, invert = FALSE;

    if (value.length() > 0) {
        p = u_getPropertyEnum(pname.data());
        if (p == UCHAR_INVALID_CODE) FAIL(ec);

        // Treat gc as gcm
        if (p == UCHAR_GENERAL_CATEGORY) {
            p = UCHAR_GENERAL_CATEGORY_MASK;
        }

        if ((p >= UCHAR_BINARY_START && p < UCHAR_BINARY_LIMIT) ||
            (p >= UCHAR_INT_START && p < UCHAR_INT_LIMIT) ||
            (p >= UCHAR_MASK_START && p < UCHAR_MASK_LIMIT)) {
            v = u_getPropertyValueEnum(p, vname.data());
            if (v == UCHAR_INVALID_CODE) {
                // Handle numeric CCC
                if (p == UCHAR_CANONICAL_COMBINING_CLASS ||
                    p == UCHAR_TRAIL_CANONICAL_COMBINING_CLASS ||
                    p == UCHAR_LEAD_CANONICAL_COMBINING_CLASS) {
                    char* end;
                    double value = uprv_strtod(vname.data(), &end);
                    v = (int32_t) value;
                    if (v != value || v < 0 || *end != 0) {
                        // non-integral or negative value, or trailing junk
                        FAIL(ec);
                    }
                    // If the resultant set is empty then the numeric value
                    // was invalid.
                    mustNotBeEmpty = TRUE;
                } else {
                    FAIL(ec);
                }
            }
        }

        else {

            switch (p) {
            case UCHAR_NUMERIC_VALUE:
                {
                    char* end;
                    double value = uprv_strtod(vname.data(), &end);
                    if (*end != 0) {
                        FAIL(ec);
                    }
                    applyFilter(numericValueFilter, &value, UPROPS_SRC_CHAR, ec);
                    return *this;
                }
                break;
            case UCHAR_NAME:
            case UCHAR_UNICODE_1_NAME:
                {
                    // Must munge name, since u_charFromName() does not do
                    // 'loose' matching.
                    char buf[128]; // it suffices that this be > uprv_getMaxCharNameLength
                    if (!mungeCharName(buf, vname.data(), sizeof(buf))) FAIL(ec);
                    UCharNameChoice choice = (p == UCHAR_NAME) ?
                        U_EXTENDED_CHAR_NAME : U_UNICODE_10_CHAR_NAME;
                    UChar32 ch = u_charFromName(choice, buf, &ec);
                    if (U_SUCCESS(ec)) {
                        clear();
                        add(ch);
                        return *this;
                    } else {
                        FAIL(ec);
                    }
                }
                break;
            case UCHAR_AGE:
                {
                    // Must munge name, since u_versionFromString() does not do
                    // 'loose' matching.
                    char buf[128];
                    if (!mungeCharName(buf, vname.data(), sizeof(buf))) FAIL(ec);
                    UVersionInfo version;
                    u_versionFromString(version, buf);
                    applyFilter(versionFilter, &version, UPROPS_SRC_PROPSVEC, ec);
                    return *this;
                }
                break;
            case UCHAR_SCRIPT_EXTENSIONS:
                v = u_getPropertyValueEnum(UCHAR_SCRIPT, vname.data());
                if (v == UCHAR_INVALID_CODE) {
                    FAIL(ec);
                }
                // fall through to calling applyIntPropertyValue()
                break;
            default:
                // p is a non-binary, non-enumerated property that we
                // don't support (yet).
                FAIL(ec);
            }
        }
    }

    else {
        // value is empty.  Interpret as General Category, Script, or
        // Binary property.
        p = UCHAR_GENERAL_CATEGORY_MASK;
        v = u_getPropertyValueEnum(p, pname.data());
        if (v == UCHAR_INVALID_CODE) {
            p = UCHAR_SCRIPT;
            v = u_getPropertyValueEnum(p, pname.data());
            if (v == UCHAR_INVALID_CODE) {
                p = u_getPropertyEnum(pname.data());
                if (p >= UCHAR_BINARY_START && p < UCHAR_BINARY_LIMIT) {
                    v = 1;
                } else if (0 == uprv_comparePropertyNames(ANY, pname.data())) {
                    set(MIN_VALUE, MAX_VALUE);
                    return *this;
                } else if (0 == uprv_comparePropertyNames(ASCII, pname.data())) {
                    set(0, 0x7F);
                    return *this;
                } else if (0 == uprv_comparePropertyNames(ASSIGNED, pname.data())) {
                    // [:Assigned:]=[:^Cn:]
                    p = UCHAR_GENERAL_CATEGORY_MASK;
                    v = U_GC_CN_MASK;
                    invert = TRUE;
                } else {
                    FAIL(ec);
                }
            }
        }
    }

    applyIntPropertyValue(p, v, ec);
    if(invert) {
        complement();
    }

    if (U_SUCCESS(ec) && (mustNotBeEmpty && isEmpty())) {
        // mustNotBeEmpty is set to true if an empty set indicates
        // invalid input.
        ec = U_ILLEGAL_ARGUMENT_ERROR;
    }

    if (isBogus() && U_SUCCESS(ec)) {
        // We likely ran out of memory. AHHH!
        ec = U_MEMORY_ALLOCATION_ERROR;
    }
    return *this;
}

//----------------------------------------------------------------
// Property set patterns
//----------------------------------------------------------------

/**
 * Return true if the given position, in the given pattern, appears
 * to be the start of a property set pattern.
 */
UBool UnicodeSet::resemblesPropertyPattern(const UnicodeString& pattern,
                                           int32_t pos) {
    // Patterns are at least 5 characters long
    if ((pos+5) > pattern.length()) {
        return FALSE;
    }

    // Look for an opening [:, [:^, \p, or \P
    return isPOSIXOpen(pattern, pos) || isPerlOpen(pattern, pos) || isNameOpen(pattern, pos);
}

/**
 * Return true if the given iterator appears to point at a
 * property pattern.  Regardless of the result, return with the
 * iterator unchanged.
 * @param chars iterator over the pattern characters.  Upon return
 * it will be unchanged.
 * @param iterOpts RuleCharacterIterator options
 */
UBool UnicodeSet::resemblesPropertyPattern(RuleCharacterIterator& chars,
                                           int32_t iterOpts) {
    // NOTE: literal will always be FALSE, because we don't parse escapes.
    UBool result = FALSE, literal;
    UErrorCode ec = U_ZERO_ERROR;
    iterOpts &= ~RuleCharacterIterator::PARSE_ESCAPES;
    RuleCharacterIterator::Pos pos;
    chars.getPos(pos);
    UChar32 c = chars.next(iterOpts, literal, ec);
    if (c == 0x5B /*'['*/ || c == 0x5C /*'\\'*/) {
        UChar32 d = chars.next(iterOpts & ~RuleCharacterIterator::SKIP_WHITESPACE,
                               literal, ec);
        result = (c == 0x5B /*'['*/) ? (d == 0x3A /*':'*/) :
                 (d == 0x4E /*'N'*/ || d == 0x70 /*'p'*/ || d == 0x50 /*'P'*/);
    }
    chars.setPos(pos);
    return result && U_SUCCESS(ec);
}

/**
 * Parse the given property pattern at the given parse position.
 */
UnicodeSet& UnicodeSet::applyPropertyPattern(const UnicodeString& pattern,
                                             ParsePosition& ppos,
                                             UErrorCode &ec) {
    int32_t pos = ppos.getIndex();

    UBool posix = FALSE; // true for [:pat:], false for \p{pat} \P{pat} \N{pat}
    UBool isName = FALSE; // true for \N{pat}, o/w false
    UBool invert = FALSE;

    if (U_FAILURE(ec)) return *this;

    // Minimum length is 5 characters, e.g. \p{L}
    if ((pos+5) > pattern.length()) {
        FAIL(ec);
    }

    // On entry, ppos should point to one of the following locations:
    // Look for an opening [:, [:^, \p, or \P
    if (isPOSIXOpen(pattern, pos)) {
        posix = TRUE;
        pos += 2;
        pos = ICU_Utility::skipWhitespace(pattern, pos);
        if (pos < pattern.length() && pattern.charAt(pos) == COMPLEMENT) {
            ++pos;
            invert = TRUE;
        }
    } else if (isPerlOpen(pattern, pos) || isNameOpen(pattern, pos)) {
        UChar c = pattern.charAt(pos+1);
        invert = (c == UPPER_P);
        isName = (c == UPPER_N);
        pos += 2;
        pos = ICU_Utility::skipWhitespace(pattern, pos);
        if (pos == pattern.length() || pattern.charAt(pos++) != OPEN_BRACE) {
            // Syntax error; "\p" or "\P" not followed by "{"
            FAIL(ec);
        }
    } else {
        // Open delimiter not seen
        FAIL(ec);
    }

    // Look for the matching close delimiter, either :] or }
    int32_t close = pattern.indexOf(posix ? POSIX_CLOSE : PERL_CLOSE, pos);
    if (close < 0) {
        // Syntax error; close delimiter missing
        FAIL(ec);
    }

    // Look for an '=' sign.  If this is present, we will parse a
    // medium \p{gc=Cf} or long \p{GeneralCategory=Format}
    // pattern.
    int32_t equals = pattern.indexOf(EQUALS, pos);
    UnicodeString propName, valueName;
    if (equals >= 0 && equals < close && !isName) {
        // Equals seen; parse medium/long pattern
        pattern.extractBetween(pos, equals, propName);
        pattern.extractBetween(equals+1, close, valueName);
    }

    else {
        // Handle case where no '=' is seen, and \N{}
        pattern.extractBetween(pos, close, propName);
            
        // Handle \N{name}
        if (isName) {
            // This is a little inefficient since it means we have to
            // parse NAME_PROP back to UCHAR_NAME even though we already
            // know it's UCHAR_NAME.  If we refactor the API to
            // support args of (UProperty, char*) then we can remove
            // NAME_PROP and make this a little more efficient.
            valueName = propName;
            propName = UnicodeString(NAME_PROP, NAME_PROP_LENGTH, US_INV);
        }
    }

    applyPropertyAlias(propName, valueName, ec);

    if (U_SUCCESS(ec)) {
        if (invert) {
            complement();
        }
            
        // Move to the limit position after the close delimiter if the
        // parse succeeded.
        ppos.setIndex(close + (posix ? 2 : 1));
    }

    return *this;
}

/**
 * Parse a property pattern.
 * @param chars iterator over the pattern characters.  Upon return
 * it will be advanced to the first character after the parsed
 * pattern, or the end of the iteration if all characters are
 * parsed.
 * @param rebuiltPat the pattern that was parsed, rebuilt or
 * copied from the input pattern, as appropriate.
 */
void UnicodeSet::applyPropertyPattern(RuleCharacterIterator& chars,
                                      UnicodeString& rebuiltPat,
                                      UErrorCode& ec) {
    if (U_FAILURE(ec)) return;
    UnicodeString pattern;
    chars.lookahead(pattern);
    ParsePosition pos(0);
    applyPropertyPattern(pattern, pos, ec);
    if (U_FAILURE(ec)) return;
    if (pos.getIndex() == 0) {
        // syntaxError(chars, "Invalid property pattern");
        ec = U_MALFORMED_SET;
        return;
    }
    chars.jumpahead(pos.getIndex());
    rebuiltPat.append(pattern, 0, pos.getIndex());
}

//----------------------------------------------------------------
// Case folding API
//----------------------------------------------------------------

// add the result of a full case mapping to the set
// use str as a temporary string to avoid constructing one
static inline void
addCaseMapping(UnicodeSet &set, int32_t result, const UChar *full, UnicodeString &str) {
    if(result >= 0) {
        if(result > UCASE_MAX_STRING_LENGTH) {
            // add a single-code point case mapping
            set.add(result);
        } else {
            // add a string case mapping from full with length result
            str.setTo((UBool)FALSE, full, result);
            set.add(str);
        }
    }
    // result < 0: the code point mapped to itself, no need to add it
    // see ucase.h
}

UnicodeSet& UnicodeSet::closeOver(int32_t attribute) {
    if (isFrozen() || isBogus()) {
        return *this;
    }
    if (attribute & (USET_CASE_INSENSITIVE | USET_ADD_CASE_MAPPINGS)) {
        const UCaseProps *csp = ucase_getSingleton();
        {
            UnicodeSet foldSet(*this);
            UnicodeString str;
            USetAdder sa = {
                foldSet.toUSet(),
                _set_add,
                _set_addRange,
                _set_addString,
                NULL, // don't need remove()
                NULL // don't need removeRange()
            };

            // start with input set to guarantee inclusion
            // USET_CASE: remove strings because the strings will actually be reduced (folded);
            //            therefore, start with no strings and add only those needed
            if (attribute & USET_CASE_INSENSITIVE) {
                foldSet.strings->removeAllElements();
            }

            int32_t n = getRangeCount();
            UChar32 result;
            const UChar *full;
            int32_t locCache = 0;

            for (int32_t i=0; i<n; ++i) {
                UChar32 start = getRangeStart(i);
                UChar32 end   = getRangeEnd(i);

                if (attribute & USET_CASE_INSENSITIVE) {
                    // full case closure
                    for (UChar32 cp=start; cp<=end; ++cp) {
                        ucase_addCaseClosure(csp, cp, &sa);
                    }
                } else {
                    // add case mappings
                    // (does not add long s for regular s, or Kelvin for k, for example)
                    for (UChar32 cp=start; cp<=end; ++cp) {
                        result = ucase_toFullLower(csp, cp, NULL, NULL, &full, "", &locCache);
                        addCaseMapping(foldSet, result, full, str);

                        result = ucase_toFullTitle(csp, cp, NULL, NULL, &full, "", &locCache);
                        addCaseMapping(foldSet, result, full, str);

                        result = ucase_toFullUpper(csp, cp, NULL, NULL, &full, "", &locCache);
                        addCaseMapping(foldSet, result, full, str);

                        result = ucase_toFullFolding(csp, cp, &full, 0);
                        addCaseMapping(foldSet, result, full, str);
                    }
                }
            }
            if (strings != NULL && strings->size() > 0) {
                if (attribute & USET_CASE_INSENSITIVE) {
                    for (int32_t j=0; j<strings->size(); ++j) {
                        str = *(const UnicodeString *) strings->elementAt(j);
                        str.foldCase();
                        if(!ucase_addStringCaseClosure(csp, str.getBuffer(), str.length(), &sa)) {
                            foldSet.add(str); // does not map to code points: add the folded string itself
                        }
                    }
                } else {
                    Locale root("");
#if !UCONFIG_NO_BREAK_ITERATION
                    UErrorCode status = U_ZERO_ERROR;
                    BreakIterator *bi = BreakIterator::createWordInstance(root, status);
                    if (U_SUCCESS(status)) {
#endif
                        const UnicodeString *pStr;

                        for (int32_t j=0; j<strings->size(); ++j) {
                            pStr = (const UnicodeString *) strings->elementAt(j);
                            (str = *pStr).toLower(root);
                            foldSet.add(str);
#if !UCONFIG_NO_BREAK_ITERATION
                            (str = *pStr).toTitle(bi, root);
                            foldSet.add(str);
#endif
                            (str = *pStr).toUpper(root);
                            foldSet.add(str);
                            (str = *pStr).foldCase();
                            foldSet.add(str);
                        }
#if !UCONFIG_NO_BREAK_ITERATION
                    }
                    delete bi;
#endif
                }
            }
            *this = foldSet;
        }
    }
    return *this;
}

U_NAMESPACE_END
