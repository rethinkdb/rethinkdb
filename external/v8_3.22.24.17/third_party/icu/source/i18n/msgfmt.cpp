/********************************************************************
 * COPYRIGHT:
 * Copyright (c) 1997-2010, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************
 *
 * File MSGFMT.CPP
 *
 * Modification History:
 *
 *   Date        Name        Description
 *   02/19/97    aliu        Converted from java.
 *   03/20/97    helena      Finished first cut of implementation.
 *   04/10/97    aliu        Made to work on AIX.  Added stoi to replace wtoi.
 *   06/11/97    helena      Fixed addPattern to take the pattern correctly.
 *   06/17/97    helena      Fixed the getPattern to return the correct pattern.
 *   07/09/97    helena      Made ParsePosition into a class.
 *   02/22/99    stephen     Removed character literals for EBCDIC safety
 *   11/01/09    kirtig      Added SelectFormat
 ********************************************************************/

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/msgfmt.h"
#include "unicode/decimfmt.h"
#include "unicode/datefmt.h"
#include "unicode/smpdtfmt.h"
#include "unicode/choicfmt.h"
#include "unicode/plurfmt.h"
#include "unicode/selfmt.h"
#include "unicode/ustring.h"
#include "unicode/ucnv_err.h"
#include "unicode/uchar.h"
#include "unicode/umsg.h"
#include "unicode/rbnf.h"
#include "cmemory.h"
#include "msgfmt_impl.h"
#include "util.h"
#include "uassert.h"
#include "ustrfmt.h"
#include "uvector.h"

// *****************************************************************************
// class MessageFormat
// *****************************************************************************

#define COMMA             ((UChar)0x002C)
#define SINGLE_QUOTE      ((UChar)0x0027)
#define LEFT_CURLY_BRACE  ((UChar)0x007B)
#define RIGHT_CURLY_BRACE ((UChar)0x007D)

//---------------------------------------
// static data

static const UChar ID_EMPTY[]     = {
    0 /* empty string, used for default so that null can mark end of list */
};

static const UChar ID_NUMBER[]    = {
    0x6E, 0x75, 0x6D, 0x62, 0x65, 0x72, 0  /* "number" */
};
static const UChar ID_DATE[]      = {
    0x64, 0x61, 0x74, 0x65, 0              /* "date" */
};
static const UChar ID_TIME[]      = {
    0x74, 0x69, 0x6D, 0x65, 0              /* "time" */
};
static const UChar ID_CHOICE[]    = {
    0x63, 0x68, 0x6F, 0x69, 0x63, 0x65, 0  /* "choice" */
};
static const UChar ID_SPELLOUT[]  = {
    0x73, 0x70, 0x65, 0x6c, 0x6c, 0x6f, 0x75, 0x74, 0 /* "spellout" */
};
static const UChar ID_ORDINAL[]   = {
    0x6f, 0x72, 0x64, 0x69, 0x6e, 0x61, 0x6c, 0 /* "ordinal" */
};
static const UChar ID_DURATION[]  = {
    0x64, 0x75, 0x72, 0x61, 0x74, 0x69, 0x6f, 0x6e, 0 /* "duration" */
};
static const UChar ID_PLURAL[]  = {
    0x70, 0x6c, 0x75, 0x72, 0x61, 0x6c, 0  /* "plural" */
};
static const UChar ID_SELECT[]  = {
    0x73, 0x65, 0x6C, 0x65, 0x63, 0x74, 0  /* "select" */
};

// MessageFormat Type List  Number, Date, Time or Choice
static const UChar * const TYPE_IDS[] = {
    ID_EMPTY,
    ID_NUMBER,
    ID_DATE,
    ID_TIME,
    ID_CHOICE,
    ID_SPELLOUT,
    ID_ORDINAL,
    ID_DURATION,
    ID_PLURAL,
    ID_SELECT,
    NULL,
};

static const UChar ID_CURRENCY[]  = {
    0x63, 0x75, 0x72, 0x72, 0x65, 0x6E, 0x63, 0x79, 0  /* "currency" */
};
static const UChar ID_PERCENT[]   = {
    0x70, 0x65, 0x72, 0x63, 0x65, 0x6E, 0x74, 0        /* "percent" */
};
static const UChar ID_INTEGER[]   = {
    0x69, 0x6E, 0x74, 0x65, 0x67, 0x65, 0x72, 0        /* "integer" */
};

// NumberFormat modifier list, default, currency, percent or integer
static const UChar * const NUMBER_STYLE_IDS[] = {
    ID_EMPTY,
    ID_CURRENCY,
    ID_PERCENT,
    ID_INTEGER,
    NULL,
};

static const UChar ID_SHORT[]     = {
    0x73, 0x68, 0x6F, 0x72, 0x74, 0        /* "short" */
};
static const UChar ID_MEDIUM[]    = {
    0x6D, 0x65, 0x64, 0x69, 0x75, 0x6D, 0  /* "medium" */
};
static const UChar ID_LONG[]      = {
    0x6C, 0x6F, 0x6E, 0x67, 0              /* "long" */
};
static const UChar ID_FULL[]      = {
    0x66, 0x75, 0x6C, 0x6C, 0              /* "full" */
};

// DateFormat modifier list, default, short, medium, long or full
static const UChar * const DATE_STYLE_IDS[] = {
    ID_EMPTY,
    ID_SHORT,
    ID_MEDIUM,
    ID_LONG,
    ID_FULL,
    NULL,
};

static const U_NAMESPACE_QUALIFIER DateFormat::EStyle DATE_STYLES[] = {
    U_NAMESPACE_QUALIFIER DateFormat::kDefault,
    U_NAMESPACE_QUALIFIER DateFormat::kShort,
    U_NAMESPACE_QUALIFIER DateFormat::kMedium,
    U_NAMESPACE_QUALIFIER DateFormat::kLong,
    U_NAMESPACE_QUALIFIER DateFormat::kFull,
};

static const int32_t DEFAULT_INITIAL_CAPACITY = 10;

U_NAMESPACE_BEGIN

// -------------------------------------
UOBJECT_DEFINE_RTTI_IMPLEMENTATION(MessageFormat)
UOBJECT_DEFINE_RTTI_IMPLEMENTATION(FormatNameEnumeration)

//--------------------------------------------------------------------

/**
 * Convert a string to an unsigned decimal, ignoring rule whitespace.
 * @return a non-negative number if successful, or a negative number
 *         upon failure.
 */
static int32_t stou(const UnicodeString& string) {
    int32_t n = 0;
    int32_t count = 0;
    UChar32 c;
    for (int32_t i=0; i<string.length(); i+=U16_LENGTH(c)) {
        c = string.char32At(i);
        if (uprv_isRuleWhiteSpace(c)) {
            continue;
        }
        int32_t d = u_digit(c, 10);
        if (d < 0 || ++count > 10) {
            return -1;
        }
        n = 10*n + d;
    }
    return n;
}

/**
 * Convert an integer value to a string and append the result to
 * the given UnicodeString.
 */
static UnicodeString& itos(int32_t i, UnicodeString& appendTo) {
    UChar temp[16];
    uprv_itou(temp,16,i,10,0); // 10 == radix
    appendTo.append(temp);
    return appendTo;
}

/*
 * A structure representing one subformat of this MessageFormat.
 * Each subformat has a Format object, an offset into the plain
 * pattern text fPattern, and an argument number.  The argument
 * number corresponds to the array of arguments to be formatted.
 * @internal
 */
class MessageFormat::Subformat : public UMemory {
public:
    /**
     * @internal
     */
    Format* format; // formatter
    /**
     * @internal
     */
    int32_t offset; // offset into fPattern
    /**
     * @internal
     */
    // TODO (claireho) or save the number to argName and use itos to convert to number.=> we need this number
    int32_t argNum;    // 0-based argument number
    /**
     * @internal
     */
    UnicodeString* argName; // argument name or number

    /**
     * Clone that.format and assign it to this.format
     * Do NOT delete this.format
     * @internal
     */
    Subformat& operator=(const Subformat& that) {
        if (this != &that) {
            format = that.format ? that.format->clone() : NULL;
            offset = that.offset;
            argNum = that.argNum;
            argName = (that.argNum==-1) ? new UnicodeString(*that.argName): NULL;
        }
        return *this;
    }

    /**
     * @internal
     */
    UBool operator==(const Subformat& that) const {
        // Do cheap comparisons first
        return offset == that.offset &&
               argNum == that.argNum &&
               ((argName == that.argName) ||
                (*argName == *that.argName)) &&
               ((format == that.format) || // handles NULL
                (*format == *that.format));
    }

    /**
     * @internal
     */
    UBool operator!=(const Subformat& that) const {
        return !operator==(that);
    }
};

// -------------------------------------
// Creates a MessageFormat instance based on the pattern.

MessageFormat::MessageFormat(const UnicodeString& pattern,
                             UErrorCode& success)
: fLocale(Locale::getDefault()),  // Uses the default locale
  formatAliases(NULL),
  formatAliasesCapacity(0),
  idStart(UCHAR_ID_START),
  idContinue(UCHAR_ID_CONTINUE),
  subformats(NULL),
  subformatCount(0),
  subformatCapacity(0),
  argTypes(NULL),
  argTypeCount(0),
  argTypeCapacity(0),
  isArgNumeric(TRUE),
  defaultNumberFormat(NULL),
  defaultDateFormat(NULL)
{
    if (!allocateSubformats(DEFAULT_INITIAL_CAPACITY) ||
        !allocateArgTypes(DEFAULT_INITIAL_CAPACITY)) {
        success = U_MEMORY_ALLOCATION_ERROR;
        return;
    }
    applyPattern(pattern, success);
    setLocaleIDs(fLocale.getName(), fLocale.getName());
}

MessageFormat::MessageFormat(const UnicodeString& pattern,
                             const Locale& newLocale,
                             UErrorCode& success)
: fLocale(newLocale),
  formatAliases(NULL),
  formatAliasesCapacity(0),
  idStart(UCHAR_ID_START),
  idContinue(UCHAR_ID_CONTINUE),
  subformats(NULL),
  subformatCount(0),
  subformatCapacity(0),
  argTypes(NULL),
  argTypeCount(0),
  argTypeCapacity(0),
  isArgNumeric(TRUE),
  defaultNumberFormat(NULL),
  defaultDateFormat(NULL)
{
    if (!allocateSubformats(DEFAULT_INITIAL_CAPACITY) ||
        !allocateArgTypes(DEFAULT_INITIAL_CAPACITY)) {
        success = U_MEMORY_ALLOCATION_ERROR;
        return;
    }
    applyPattern(pattern, success);
    setLocaleIDs(fLocale.getName(), fLocale.getName());
}

MessageFormat::MessageFormat(const UnicodeString& pattern,
                             const Locale& newLocale,
                             UParseError& parseError,
                             UErrorCode& success)
: fLocale(newLocale),
  formatAliases(NULL),
  formatAliasesCapacity(0),
  idStart(UCHAR_ID_START),
  idContinue(UCHAR_ID_CONTINUE),
  subformats(NULL),
  subformatCount(0),
  subformatCapacity(0),
  argTypes(NULL),
  argTypeCount(0),
  argTypeCapacity(0),
  isArgNumeric(TRUE),
  defaultNumberFormat(NULL),
  defaultDateFormat(NULL)
{
    if (!allocateSubformats(DEFAULT_INITIAL_CAPACITY) ||
        !allocateArgTypes(DEFAULT_INITIAL_CAPACITY)) {
        success = U_MEMORY_ALLOCATION_ERROR;
        return;
    }
    applyPattern(pattern, parseError, success);
    setLocaleIDs(fLocale.getName(), fLocale.getName());
}

MessageFormat::MessageFormat(const MessageFormat& that)
: Format(that),
  formatAliases(NULL),
  formatAliasesCapacity(0),
  idStart(UCHAR_ID_START),
  idContinue(UCHAR_ID_CONTINUE),
  subformats(NULL),
  subformatCount(0),
  subformatCapacity(0),
  argTypes(NULL),
  argTypeCount(0),
  argTypeCapacity(0),
  isArgNumeric(TRUE),
  defaultNumberFormat(NULL),
  defaultDateFormat(NULL)
{
    *this = that;
}

MessageFormat::~MessageFormat()
{
    int32_t idx;
    for (idx = 0; idx < subformatCount; idx++) {
        delete subformats[idx].format;
        delete subformats[idx].argName;
    }
    uprv_free(subformats);
    subformats = NULL;
    subformatCount = subformatCapacity = 0;

    uprv_free(argTypes);
    argTypes = NULL;
    argTypeCount = argTypeCapacity = 0;

    uprv_free(formatAliases);

    delete defaultNumberFormat;
    delete defaultDateFormat;
}

//--------------------------------------------------------------------
// Variable-size array management

/**
 * Allocate subformats[] to at least the given capacity and return
 * TRUE if successful.  If not, leave subformats[] unchanged.
 *
 * If subformats is NULL, allocate it.  If it is not NULL, enlarge it
 * if necessary to be at least as large as specified.
 */
UBool MessageFormat::allocateSubformats(int32_t capacity) {
    if (subformats == NULL) {
        subformats = (Subformat*) uprv_malloc(sizeof(*subformats) * capacity);
        subformatCapacity = capacity;
        subformatCount = 0;
        if (subformats == NULL) {
            subformatCapacity = 0;
            return FALSE;
        }
    } else if (subformatCapacity < capacity) {
        if (capacity < 2*subformatCapacity) {
            capacity = 2*subformatCapacity;
        }
        Subformat* a = (Subformat*)
            uprv_realloc(subformats, sizeof(*subformats) * capacity);
        if (a == NULL) {
            return FALSE; // request failed
        }
        subformats = a;
        subformatCapacity = capacity;
    }
    return TRUE;
}

/**
 * Allocate argTypes[] to at least the given capacity and return
 * TRUE if successful.  If not, leave argTypes[] unchanged.
 *
 * If argTypes is NULL, allocate it.  If it is not NULL, enlarge it
 * if necessary to be at least as large as specified.
 */
UBool MessageFormat::allocateArgTypes(int32_t capacity) {
    if (argTypes == NULL) {
        argTypes = (Formattable::Type*) uprv_malloc(sizeof(*argTypes) * capacity);
        argTypeCount = 0;
        argTypeCapacity = capacity;
        if (argTypes == NULL) {
            argTypeCapacity = 0;
            return FALSE;
        }
        for (int32_t i=0; i<capacity; ++i) {
            argTypes[i] = Formattable::kString;
        }
    } else if (argTypeCapacity < capacity) {
        if (capacity < 2*argTypeCapacity) {
            capacity = 2*argTypeCapacity;
        }
        Formattable::Type* a = (Formattable::Type*)
            uprv_realloc(argTypes, sizeof(*argTypes) * capacity);
        if (a == NULL) {
            return FALSE; // request failed
        }
        for (int32_t i=argTypeCapacity; i<capacity; ++i) {
            a[i] = Formattable::kString;
        }
        argTypes = a;
        argTypeCapacity = capacity;
    }
    return TRUE;
}

// -------------------------------------
// assignment operator

const MessageFormat&
MessageFormat::operator=(const MessageFormat& that)
{
    // Reallocate the arrays BEFORE changing this object
    if (this != &that &&
        allocateSubformats(that.subformatCount) &&
        allocateArgTypes(that.argTypeCount)) {

        // Calls the super class for assignment first.
        Format::operator=(that);

        fPattern = that.fPattern;
        setLocale(that.fLocale);
        isArgNumeric = that.isArgNumeric;
        int32_t j;
        for (j=0; j<subformatCount; ++j) {
            delete subformats[j].format;
        }
        subformatCount = 0;

        for (j=0; j<that.subformatCount; ++j) {
            // Subformat::operator= does NOT delete this.format
            subformats[j] = that.subformats[j];
        }
        subformatCount = that.subformatCount;

        for (j=0; j<that.argTypeCount; ++j) {
            argTypes[j] = that.argTypes[j];
        }
        argTypeCount = that.argTypeCount;
    }
    return *this;
}

UBool
MessageFormat::operator==(const Format& rhs) const
{
    if (this == &rhs) return TRUE;

    MessageFormat& that = (MessageFormat&)rhs;

    // Check class ID before checking MessageFormat members
    if (!Format::operator==(rhs) ||
        fPattern != that.fPattern ||
        fLocale != that.fLocale ||
        isArgNumeric != that.isArgNumeric) {
        return FALSE;
    }

    int32_t j;
    for (j=0; j<subformatCount; ++j) {
        if (subformats[j] != that.subformats[j]) {
            return FALSE;
        }
    }

    return TRUE;
}

// -------------------------------------
// Creates a copy of this MessageFormat, the caller owns the copy.

Format*
MessageFormat::clone() const
{
    return new MessageFormat(*this);
}

// -------------------------------------
// Sets the locale of this MessageFormat object to theLocale.

void
MessageFormat::setLocale(const Locale& theLocale)
{
    if (fLocale != theLocale) {
        delete defaultNumberFormat;
        defaultNumberFormat = NULL;
        delete defaultDateFormat;
        defaultDateFormat = NULL;
    }
    fLocale = theLocale;
    setLocaleIDs(fLocale.getName(), fLocale.getName());
}

// -------------------------------------
// Gets the locale of this MessageFormat object.

const Locale&
MessageFormat::getLocale() const
{
    return fLocale;
}




void
MessageFormat::applyPattern(const UnicodeString& newPattern,
                            UErrorCode& status)
{
    UParseError parseError;
    applyPattern(newPattern,parseError,status);
}


// -------------------------------------
// Applies the new pattern and returns an error if the pattern
// is not correct.
void
MessageFormat::applyPattern(const UnicodeString& pattern,
                            UParseError& parseError,
                            UErrorCode& ec)
{
    if(U_FAILURE(ec)) {
        return;
    }
    // The pattern is broken up into segments.  Each time a subformat
    // is encountered, 4 segments are recorded.  For example, consider
    // the pattern:
    //  "There {0,choice,0.0#are no files|1.0#is one file|1.0<are {0, number} files} on disk {1}."
    // The first set of segments is:
    //  segments[0] = "There "
    //  segments[1] = "0"
    //  segments[2] = "choice"
    //  segments[3] = "0.0#are no files|1.0#is one file|1.0<are {0, number} files"

    // During parsing, the plain text is accumulated into segments[0].
    // Segments 1..3 are used to parse each subpattern.  Each time a
    // subpattern is parsed, it creates a format object that is stored
    // in the subformats array, together with an offset and argument
    // number.  The offset into the plain text stored in
    // segments[0].

    // Quotes in segment 0 are handled normally.  They are removed.
    // Quotes may not occur in segments 1 or 2.
    // Quotes in segment 3 are parsed and _copied_.  This makes
    //  subformat patterns work, e.g., {1,number,'#'.##} passes
    //  the pattern "'#'.##" to DecimalFormat.

    UnicodeString segments[4];
    int32_t part = 0; // segment we are in, 0..3
    // Record the highest argument number in the pattern.  (In the
    // subpattern {3,number} the argument number is 3.)
    int32_t formatNumber = 0;
    UBool inQuote = FALSE;
    int32_t braceStack = 0;
    // Clear error struct
    parseError.offset = -1;
    parseError.preContext[0] = parseError.postContext[0] = (UChar)0;
    int32_t patLen = pattern.length();
    int32_t i;

    for (i=0; i<subformatCount; ++i) {
        delete subformats[i].format;
    }
    subformatCount = 0;
    argTypeCount = 0;

    for (i=0; i<patLen; ++i) {
        UChar ch = pattern[i];
        if (part == 0) {
            // In segment 0, recognize and remove quotes
            if (ch == SINGLE_QUOTE) {
                if (i+1 < patLen && pattern[i+1] == SINGLE_QUOTE) {
                    segments[0] += ch;
                    ++i;
                } else {
                    inQuote = !inQuote;
                }
            } else if (ch == LEFT_CURLY_BRACE && !inQuote) {
                // The only way we get from segment 0 to 1 is via an
                // unquoted '{'.
                part = 1;
            } else {
                segments[0] += ch;
            }
        } else if (inQuote) {
            // In segments 1..3, recognize quoted matter, and copy it
            // into the segment, together with the quotes.  This takes
            // care of '' as well.
            segments[part] += ch;
            if (ch == SINGLE_QUOTE) {
                inQuote = FALSE;
            }
        } else {
            // We have an unquoted character in segment 1..3
            switch (ch) {
            case COMMA:
                // Commas bump us to the next segment, except for segment 3,
                // which can contain commas.  See example above.
                if (part < 3)
                    part += 1;
                else
                    segments[3] += ch;
                break;
            case LEFT_CURLY_BRACE:
                // Handle '{' within segment 3.  The initial '{'
                // before segment 1 is handled above.
                if (part != 3) {
                    ec = U_PATTERN_SYNTAX_ERROR;
                    goto SYNTAX_ERROR;
                }
                ++braceStack;
                segments[part] += ch;
                break;
            case RIGHT_CURLY_BRACE:
                if (braceStack == 0) {
                    makeFormat(formatNumber, segments, parseError,ec);
                    if (U_FAILURE(ec)){
                        goto SYNTAX_ERROR;
                    }
                    formatNumber++;

                    segments[1].remove();
                    segments[2].remove();
                    segments[3].remove();
                    part = 0;
                } else {
                    --braceStack;
                    segments[part] += ch;
                }
                break;
            case SINGLE_QUOTE:
                inQuote = TRUE;
                // fall through (copy quote chars in segments 1..3)
            default:
                segments[part] += ch;
                break;
            }
        }
    }
    if (braceStack != 0 || part != 0) {
        // Unmatched braces in the pattern
        ec = U_UNMATCHED_BRACES;
        goto SYNTAX_ERROR;
    }
    fPattern = segments[0];
    return;

 SYNTAX_ERROR:
    syntaxError(pattern, i, parseError);
    for (i=0; i<subformatCount; ++i) {
        delete subformats[i].format;
    }
    argTypeCount = subformatCount = 0;
}
// -------------------------------------
// Converts this MessageFormat instance to a pattern.

UnicodeString&
MessageFormat::toPattern(UnicodeString& appendTo) const {
    // later, make this more extensible
    int32_t lastOffset = 0;
    int32_t i;
    for (i=0; i<subformatCount; ++i) {
        copyAndFixQuotes(fPattern, lastOffset, subformats[i].offset, appendTo);
        lastOffset = subformats[i].offset;
        appendTo += LEFT_CURLY_BRACE;
        if (isArgNumeric) {
            itos(subformats[i].argNum, appendTo);
        }
        else {
            appendTo += *subformats[i].argName;
        }
        Format* fmt = subformats[i].format;
        DecimalFormat* decfmt;
        SimpleDateFormat* sdtfmt;
        ChoiceFormat* chcfmt;
        PluralFormat* plfmt;
        SelectFormat* selfmt;
        if (fmt == NULL) {
            // do nothing, string format
        }
        else if ((decfmt = dynamic_cast<DecimalFormat*>(fmt)) != NULL) {
            UErrorCode ec = U_ZERO_ERROR;
            NumberFormat& formatAlias = *decfmt;
            NumberFormat *defaultTemplate = NumberFormat::createInstance(fLocale, ec);
            NumberFormat *currencyTemplate = NumberFormat::createCurrencyInstance(fLocale, ec);
            NumberFormat *percentTemplate = NumberFormat::createPercentInstance(fLocale, ec);
            NumberFormat *integerTemplate = createIntegerFormat(fLocale, ec);

            appendTo += COMMA;
            appendTo += ID_NUMBER;
            if (formatAlias != *defaultTemplate) {
                appendTo += COMMA;
                if (formatAlias == *currencyTemplate) {
                    appendTo += ID_CURRENCY;
                }
                else if (formatAlias == *percentTemplate) {
                    appendTo += ID_PERCENT;
                }
                else if (formatAlias == *integerTemplate) {
                    appendTo += ID_INTEGER;
                }
                else {
                    UnicodeString buffer;
                    appendTo += decfmt->toPattern(buffer);
                }
            }

            delete defaultTemplate;
            delete currencyTemplate;
            delete percentTemplate;
            delete integerTemplate;
        }
        else if ((sdtfmt = dynamic_cast<SimpleDateFormat*>(fmt)) != NULL) {
            DateFormat& formatAlias = *sdtfmt;
            DateFormat *defaultDateTemplate = DateFormat::createDateInstance(DateFormat::kDefault, fLocale);
            DateFormat *shortDateTemplate = DateFormat::createDateInstance(DateFormat::kShort, fLocale);
            DateFormat *longDateTemplate = DateFormat::createDateInstance(DateFormat::kLong, fLocale);
            DateFormat *fullDateTemplate = DateFormat::createDateInstance(DateFormat::kFull, fLocale);
            DateFormat *defaultTimeTemplate = DateFormat::createTimeInstance(DateFormat::kDefault, fLocale);
            DateFormat *shortTimeTemplate = DateFormat::createTimeInstance(DateFormat::kShort, fLocale);
            DateFormat *longTimeTemplate = DateFormat::createTimeInstance(DateFormat::kLong, fLocale);
            DateFormat *fullTimeTemplate = DateFormat::createTimeInstance(DateFormat::kFull, fLocale);


            appendTo += COMMA;
            if (formatAlias == *defaultDateTemplate) {
                // default is medium. no need to handle medium separately.
                appendTo += ID_DATE;
            }
            else if (formatAlias == *shortDateTemplate) {
                appendTo += ID_DATE;
                appendTo += COMMA;
                appendTo += ID_SHORT;
            }
            else if (formatAlias == *longDateTemplate) {
                appendTo += ID_DATE;
                appendTo += COMMA;
                appendTo += ID_LONG;
            }
            else if (formatAlias == *fullDateTemplate) {
                appendTo += ID_DATE;
                appendTo += COMMA;
                appendTo += ID_FULL;
            }
            else if (formatAlias == *defaultTimeTemplate) {
                // default is medium. no need to handle medium separately.
                appendTo += ID_TIME;
            }
            else if (formatAlias == *shortTimeTemplate) {
                appendTo += ID_TIME;
                appendTo += COMMA;
                appendTo += ID_SHORT;
            }
            else if (formatAlias == *longTimeTemplate) {
                appendTo += ID_TIME;
                appendTo += COMMA;
                appendTo += ID_LONG;
            }
            else if (formatAlias == *fullTimeTemplate) {
                appendTo += ID_TIME;
                appendTo += COMMA;
                appendTo += ID_FULL;
            }
            else {
                UnicodeString buffer;
                appendTo += ID_DATE;
                appendTo += COMMA;
                appendTo += sdtfmt->toPattern(buffer);
            }

            delete defaultDateTemplate;
            delete shortDateTemplate;
            delete longDateTemplate;
            delete fullDateTemplate;
            delete defaultTimeTemplate;
            delete shortTimeTemplate;
            delete longTimeTemplate;
            delete fullTimeTemplate;
            // {sfb} there should be a more efficient way to do this!
        }
        else if ((chcfmt = dynamic_cast<ChoiceFormat*>(fmt)) != NULL) {
            UnicodeString buffer;
            appendTo += COMMA;
            appendTo += ID_CHOICE;
            appendTo += COMMA;
            appendTo += ((ChoiceFormat*)fmt)->toPattern(buffer);
        }
        else if ((plfmt = dynamic_cast<PluralFormat*>(fmt)) != NULL) {
            UnicodeString buffer;
            appendTo += plfmt->toPattern(buffer);
        }
        else if ((selfmt = dynamic_cast<SelectFormat*>(fmt)) != NULL) {
            UnicodeString buffer;
            appendTo += ((SelectFormat*)fmt)->toPattern(buffer);
        }
        else {
            //appendTo += ", unknown";
        }
        appendTo += RIGHT_CURLY_BRACE;
    }
    copyAndFixQuotes(fPattern, lastOffset, fPattern.length(), appendTo);
    return appendTo;
}

// -------------------------------------
// Adopts the new formats array and updates the array count.
// This MessageFormat instance owns the new formats.

void
MessageFormat::adoptFormats(Format** newFormats,
                            int32_t count) {
    if (newFormats == NULL || count < 0) {
        return;
    }

    int32_t i;
    if (allocateSubformats(count)) {
        for (i=0; i<subformatCount; ++i) {
            delete subformats[i].format;
        }
        for (i=0; i<count; ++i) {
            subformats[i].format = newFormats[i];
        }
        subformatCount = count;
    } else {
        // An adopt method must always take ownership.  Delete
        // the incoming format objects and return unchanged.
        for (i=0; i<count; ++i) {
            delete newFormats[i];
        }
    }

    // TODO: What about the .offset and .argNum fields?
}

// -------------------------------------
// Sets the new formats array and updates the array count.
// This MessageFormat instance maks a copy of the new formats.

void
MessageFormat::setFormats(const Format** newFormats,
                          int32_t count) {
    if (newFormats == NULL || count < 0) {
        return;
    }

    if (allocateSubformats(count)) {
        int32_t i;
        for (i=0; i<subformatCount; ++i) {
            delete subformats[i].format;
        }
        subformatCount = 0;

        for (i=0; i<count; ++i) {
            subformats[i].format = newFormats[i] ? newFormats[i]->clone() : NULL;
        }
        subformatCount = count;
    }

    // TODO: What about the .offset and .arg fields?
}

// -------------------------------------
// Adopt a single format by format number.
// Do nothing if the format number is not less than the array count.

void
MessageFormat::adoptFormat(int32_t n, Format *newFormat) {
    if (n < 0 || n >= subformatCount) {
        delete newFormat;
    } else {
        delete subformats[n].format;
        subformats[n].format = newFormat;
    }
}

// -------------------------------------
// Adopt a single format by format name.
// Do nothing if there is no match of formatName.
void
MessageFormat::adoptFormat(const UnicodeString& formatName,
                           Format* formatToAdopt,
                           UErrorCode& status) {
    if (isArgNumeric ) {
        int32_t argumentNumber = stou(formatName);
        if (argumentNumber<0) {
            status = U_ARGUMENT_TYPE_MISMATCH;
            return;
        }
        adoptFormat(argumentNumber, formatToAdopt);
        return;
    }
    for (int32_t i=0; i<subformatCount; ++i) {
        if (formatName==*subformats[i].argName) {
            delete subformats[i].format;
            if ( formatToAdopt== NULL) {
                // This should never happen -- but we'll be nice if it does
                subformats[i].format = NULL;
            } else {
                subformats[i].format = formatToAdopt;
            }
        }
    }
}

// -------------------------------------
// Set a single format.
// Do nothing if the variable is not less than the array count.

void
MessageFormat::setFormat(int32_t n, const Format& newFormat) {
    if (n >= 0 && n < subformatCount) {
        delete subformats[n].format;
        if (&newFormat == NULL) {
            // This should never happen -- but we'll be nice if it does
            subformats[n].format = NULL;
        } else {
            subformats[n].format = newFormat.clone();
        }
    }
}

// -------------------------------------
// Get a single format by format name.
// Do nothing if the variable is not less than the array count.
Format *
MessageFormat::getFormat(const UnicodeString& formatName, UErrorCode& status) {

    if (U_FAILURE(status)) return NULL;

    if (isArgNumeric ) {
        int32_t argumentNumber = stou(formatName);
        if (argumentNumber<0) {
            status = U_ARGUMENT_TYPE_MISMATCH;
            return NULL;
        }
        if (argumentNumber < 0 || argumentNumber >= subformatCount) {
            return subformats[argumentNumber].format;
        }
        else {
            return NULL;
        }
    }

    for (int32_t i=0; i<subformatCount; ++i) {
        if (formatName==*subformats[i].argName)
        {
            return subformats[i].format;
        }
    }
    return NULL;
}

// -------------------------------------
// Set a single format by format name
// Do nothing if the variable is not less than the array count.
void
MessageFormat::setFormat(const UnicodeString& formatName,
                         const Format& newFormat,
                         UErrorCode& status) {
    if (isArgNumeric) {
        status = U_ARGUMENT_TYPE_MISMATCH;
        return;
    }
    for (int32_t i=0; i<subformatCount; ++i) {
        if (formatName==*subformats[i].argName)
        {
            delete subformats[i].format;
            if (&newFormat == NULL) {
                // This should never happen -- but we'll be nice if it does
                subformats[i].format = NULL;
            } else {
                subformats[i].format = newFormat.clone();
            }
            break;
        }
    }
}

// -------------------------------------
// Gets the format array.

const Format**
MessageFormat::getFormats(int32_t& cnt) const
{
    // This old API returns an array (which we hold) of Format*
    // pointers.  The array is valid up to the next call to any
    // method on this object.  We construct and resize an array
    // on demand that contains aliases to the subformats[i].format
    // pointers.
    MessageFormat* t = (MessageFormat*) this;
    cnt = 0;
    if (formatAliases == NULL) {
        t->formatAliasesCapacity = (subformatCount<10) ? 10 : subformatCount;
        Format** a = (Format**)
            uprv_malloc(sizeof(Format*) * formatAliasesCapacity);
        if (a == NULL) {
            return NULL;
        }
        t->formatAliases = a;
    } else if (subformatCount > formatAliasesCapacity) {
        Format** a = (Format**)
            uprv_realloc(formatAliases, sizeof(Format*) * subformatCount);
        if (a == NULL) {
            return NULL;
        }
        t->formatAliases = a;
        t->formatAliasesCapacity = subformatCount;
    }
    for (int32_t i=0; i<subformatCount; ++i) {
        t->formatAliases[i] = subformats[i].format;
    }
    cnt = subformatCount;
    return (const Format**)formatAliases;
}


StringEnumeration*
MessageFormat::getFormatNames(UErrorCode& status) {
    if (U_FAILURE(status))  return NULL;

    if (isArgNumeric) {
        status = U_ARGUMENT_TYPE_MISMATCH;
        return NULL;
    }
    UVector *fFormatNames = new UVector(status);
    if (U_FAILURE(status)) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return NULL;
    }
    for (int32_t i=0; i<subformatCount; ++i) {
        fFormatNames->addElement(new UnicodeString(*subformats[i].argName), status);
    }

    StringEnumeration* nameEnumerator = new FormatNameEnumeration(fFormatNames, status);
    return nameEnumerator;
}

// -------------------------------------
// Formats the source Formattable array and copy into the result buffer.
// Ignore the FieldPosition result for error checking.

UnicodeString&
MessageFormat::format(const Formattable* source,
                      int32_t cnt,
                      UnicodeString& appendTo,
                      FieldPosition& ignore,
                      UErrorCode& success) const
{
    if (U_FAILURE(success))
        return appendTo;

    return format(source, cnt, appendTo, ignore, 0, success);
}

// -------------------------------------
// Internally creates a MessageFormat instance based on the
// pattern and formats the arguments Formattable array and
// copy into the appendTo buffer.

UnicodeString&
MessageFormat::format(  const UnicodeString& pattern,
                        const Formattable* arguments,
                        int32_t cnt,
                        UnicodeString& appendTo,
                        UErrorCode& success)
{
    MessageFormat temp(pattern, success);
    FieldPosition ignore(0);
    temp.format(arguments, cnt, appendTo, ignore, success);
    return appendTo;
}

// -------------------------------------
// Formats the source Formattable object and copy into the
// appendTo buffer.  The Formattable object must be an array
// of Formattable instances, returns error otherwise.

UnicodeString&
MessageFormat::format(const Formattable& source,
                      UnicodeString& appendTo,
                      FieldPosition& ignore,
                      UErrorCode& success) const
{
    int32_t cnt;

    if (U_FAILURE(success))
        return appendTo;
    if (source.getType() != Formattable::kArray) {
        success = U_ILLEGAL_ARGUMENT_ERROR;
        return appendTo;
    }
    const Formattable* tmpPtr = source.getArray(cnt);

    return format(tmpPtr, cnt, appendTo, ignore, 0, success);
}


UnicodeString&
MessageFormat::format(const UnicodeString* argumentNames,
                      const Formattable* arguments,
                      int32_t count,
                      UnicodeString& appendTo,
                      UErrorCode& success) const {
    FieldPosition ignore(0);
    return format(arguments, argumentNames, count, appendTo, ignore, 0, success);
}

UnicodeString&
MessageFormat::format(const Formattable* arguments,
                      int32_t cnt,
                      UnicodeString& appendTo,
                      FieldPosition& status,
                      int32_t recursionProtection,
                      UErrorCode& success) const
{
    return format(arguments, NULL, cnt, appendTo, status, recursionProtection, success);
}

// -------------------------------------
// Formats the arguments Formattable array and copy into the appendTo buffer.
// Ignore the FieldPosition result for error checking.

UnicodeString&
MessageFormat::format(const Formattable* arguments,
                      const UnicodeString *argumentNames,
                      int32_t cnt,
                      UnicodeString& appendTo,
                      FieldPosition& status,
                      int32_t recursionProtection,
                      UErrorCode& success) const
{
    int32_t lastOffset = 0;
    int32_t argumentNumber=0;
    if (cnt < 0 || (cnt && arguments == NULL)) {
        success = U_ILLEGAL_ARGUMENT_ERROR;
        return appendTo;
    }

    if ( !isArgNumeric && argumentNames== NULL ) {
        success = U_ILLEGAL_ARGUMENT_ERROR;
        return appendTo;
    }

    const Formattable *obj=NULL;
    for (int32_t i=0; i<subformatCount; ++i) {
        // Append the prefix of current format element.
        appendTo.append(fPattern, lastOffset, subformats[i].offset - lastOffset);
        lastOffset = subformats[i].offset;
        obj = NULL;
        if (isArgNumeric) {
            argumentNumber = subformats[i].argNum;

            // Checks the scope of the argument number.
            if (argumentNumber >= cnt) {
                appendTo += LEFT_CURLY_BRACE;
                itos(argumentNumber, appendTo);
                appendTo += RIGHT_CURLY_BRACE;
                continue;
            }
            obj = arguments+argumentNumber;
        }
        else {
            for (int32_t j=0; j<cnt; ++j) {
                if (argumentNames[j]== *subformats[i].argName ) {
                    obj = arguments+j;
                    break;
                }
            }
            if (obj == NULL ) {
                appendTo += LEFT_CURLY_BRACE;
                appendTo += *subformats[i].argName;
                appendTo += RIGHT_CURLY_BRACE;
                continue;

            }
        }
        Formattable::Type type = obj->getType();

        // Recursively calling the format process only if the current
        // format argument refers to either of the following:
        // a ChoiceFormat object, a PluralFormat object, a SelectFormat object.
        Format* fmt = subformats[i].format;
        if (fmt != NULL) {
            UnicodeString argNum;
            fmt->format(*obj, argNum, success);

            // Needs to reprocess the ChoiceFormat and PluralFormat and SelectFormat option by using the
            // MessageFormat pattern application.
            if ((dynamic_cast<ChoiceFormat*>(fmt) != NULL ||
                 dynamic_cast<PluralFormat*>(fmt) != NULL ||
                 dynamic_cast<SelectFormat*>(fmt) != NULL) &&
                argNum.indexOf(LEFT_CURLY_BRACE) >= 0
            ) {
                MessageFormat temp(argNum, fLocale, success);
                // TODO: Implement recursion protection
                if ( isArgNumeric ) {
                    temp.format(arguments, NULL, cnt, appendTo, status, recursionProtection, success);
                }
                else {
                    temp.format(arguments, argumentNames, cnt, appendTo, status, recursionProtection, success);
                }
                if (U_FAILURE(success)) {
                    return appendTo;
                }
            }
            else {
                appendTo += argNum;
            }
        }
        // If the obj data type is a number, use a NumberFormat instance.
        else if ((type == Formattable::kDouble) ||
                 (type == Formattable::kLong) ||
                 (type == Formattable::kInt64)) {

            const NumberFormat* nf = getDefaultNumberFormat(success);
            if (nf == NULL) {
                return appendTo;
            }
            if (type == Formattable::kDouble) {
                nf->format(obj->getDouble(), appendTo);
            } else if (type == Formattable::kLong) {
                nf->format(obj->getLong(), appendTo);
            } else {
                nf->format(obj->getInt64(), appendTo);
            }
        }
        // If the obj data type is a Date instance, use a DateFormat instance.
        else if (type == Formattable::kDate) {
            const DateFormat* df = getDefaultDateFormat(success);
            if (df == NULL) {
                return appendTo;
            }
            df->format(obj->getDate(), appendTo);
        }
        else if (type == Formattable::kString) {
            appendTo += obj->getString();
        }
        else {
            success = U_ILLEGAL_ARGUMENT_ERROR;
            return appendTo;
        }
    }
    // Appends the rest of the pattern characters after the real last offset.
    appendTo.append(fPattern, lastOffset, 0x7fffffff);
    return appendTo;
}


// -------------------------------------
// Parses the source pattern and returns the Formattable objects array,
// the array count and the ending parse position.  The caller of this method
// owns the array.

Formattable*
MessageFormat::parse(const UnicodeString& source,
                     ParsePosition& pos,
                     int32_t& count) const
{
    // Allocate at least one element.  Allocating an array of length
    // zero causes problems on some platforms (e.g. Win32).
    Formattable *resultArray = new Formattable[argTypeCount ? argTypeCount : 1];
    int32_t patternOffset = 0;
    int32_t sourceOffset = pos.getIndex();
    ParsePosition tempPos(0);
    count = 0; // {sfb} reset to zero
    int32_t len;
    // If resultArray could not be created, exit out.
    // Avoid crossing initialization of variables above.
    if (resultArray == NULL) {
        goto PARSE_ERROR;
    }
    for (int32_t i = 0; i < subformatCount; ++i) {
        // match up to format
        len = subformats[i].offset - patternOffset;
        if (len == 0 ||
            fPattern.compare(patternOffset, len, source, sourceOffset, len) == 0) {
            sourceOffset += len;
            patternOffset += len;
        }
        else {
            goto PARSE_ERROR;
        }

        // now use format
        Format* fmt = subformats[i].format;
        int32_t argNum = subformats[i].argNum;
        if (fmt == NULL) {   // string format
            // if at end, use longest possible match
            // otherwise uses first match to intervening string
            // does NOT recursively try all possibilities
            int32_t tempLength = (i+1<subformatCount) ?
                subformats[i+1].offset : fPattern.length();

            int32_t next;
            if (patternOffset >= tempLength) {
                next = source.length();
            }
            else {
                UnicodeString buffer;
                fPattern.extract(patternOffset,tempLength - patternOffset, buffer);
                next = source.indexOf(buffer, sourceOffset);
            }

            if (next < 0) {
                goto PARSE_ERROR;
            }
            else {
                UnicodeString buffer;
                source.extract(sourceOffset,next - sourceOffset, buffer);
                UnicodeString strValue = buffer;
                UnicodeString temp(LEFT_CURLY_BRACE);
                // {sfb} check this later
                if (isArgNumeric) {
                    itos(argNum, temp);
                }
                else {
                    temp+=(*subformats[i].argName);
                }
                temp += RIGHT_CURLY_BRACE;
                if (strValue != temp) {
                    source.extract(sourceOffset,next - sourceOffset, buffer);
                    resultArray[argNum].setString(buffer);
                    // {sfb} not sure about this
                    if ((argNum + 1) > count) {
                        count = argNum + 1;
                    }
                }
                sourceOffset = next;
            }
        }
        else {
            tempPos.setIndex(sourceOffset);
            fmt->parseObject(source, resultArray[argNum], tempPos);
            if (tempPos.getIndex() == sourceOffset) {
                goto PARSE_ERROR;
            }

            if ((argNum + 1) > count) {
                count = argNum + 1;
            }
            sourceOffset = tempPos.getIndex(); // update
        }
    }
    len = fPattern.length() - patternOffset;
    if (len == 0 ||
        fPattern.compare(patternOffset, len, source, sourceOffset, len) == 0) {
        pos.setIndex(sourceOffset + len);
        return resultArray;
    }
    // else fall through...

 PARSE_ERROR:
    pos.setErrorIndex(sourceOffset);
    delete [] resultArray;
    count = 0;
    return NULL; // leave index as is to signal error
}

// -------------------------------------
// Parses the source string and returns the array of
// Formattable objects and the array count.  The caller
// owns the returned array.

Formattable*
MessageFormat::parse(const UnicodeString& source,
                     int32_t& cnt,
                     UErrorCode& success) const
{
    if (!isArgNumeric ) {
        success = U_ARGUMENT_TYPE_MISMATCH;
        return NULL;
    }
    ParsePosition status(0);
    // Calls the actual implementation method and starts
    // from zero offset of the source text.
    Formattable* result = parse(source, status, cnt);
    if (status.getIndex() == 0) {
        success = U_MESSAGE_PARSE_ERROR;
        delete[] result;
        return NULL;
    }
    return result;
}

// -------------------------------------
// Parses the source text and copy into the result buffer.

void
MessageFormat::parseObject( const UnicodeString& source,
                            Formattable& result,
                            ParsePosition& status) const
{
    int32_t cnt = 0;
    Formattable* tmpResult = parse(source, status, cnt);
    if (tmpResult != NULL)
        result.adoptArray(tmpResult, cnt);
}

UnicodeString
MessageFormat::autoQuoteApostrophe(const UnicodeString& pattern, UErrorCode& status) {
  UnicodeString result;
  if (U_SUCCESS(status)) {
    int32_t plen = pattern.length();
    const UChar* pat = pattern.getBuffer();
    int32_t blen = plen * 2 + 1; // space for null termination, convenience
    UChar* buf = result.getBuffer(blen);
    if (buf == NULL) {
      status = U_MEMORY_ALLOCATION_ERROR;
    } else {
      int32_t len = umsg_autoQuoteApostrophe(pat, plen, buf, blen, &status);
      result.releaseBuffer(U_SUCCESS(status) ? len : 0);
    }
  }
  if (U_FAILURE(status)) {
    result.setToBogus();
  }
  return result;
}

// -------------------------------------

static Format* makeRBNF(URBNFRuleSetTag tag, const Locale& locale, const UnicodeString& defaultRuleSet, UErrorCode& ec) {
    RuleBasedNumberFormat* fmt = new RuleBasedNumberFormat(tag, locale, ec);
    if (fmt == NULL) {
        ec = U_MEMORY_ALLOCATION_ERROR;
    } else if (U_SUCCESS(ec) && defaultRuleSet.length() > 0) {
        UErrorCode localStatus = U_ZERO_ERROR; // ignore unrecognized default rule set
        fmt->setDefaultRuleSet(defaultRuleSet, localStatus);
    }
    return fmt;
}

/**
 * Reads the segments[] array (see applyPattern()) and parses the
 * segments[1..3] into a Format* object.  Stores the format object in
 * the subformats[] array.  Updates the argTypes[] array type
 * information for the corresponding argument.
 *
 * @param formatNumber index into subformats[] for this format
 * @param segments array of strings with the parsed pattern segments
 * @param parseError parse error data (output param)
 * @param ec error code
 */
void
MessageFormat::makeFormat(int32_t formatNumber,
                          UnicodeString* segments,
                          UParseError& parseError,
                          UErrorCode& ec) {
    if (U_FAILURE(ec)) {
        return;
    }

    // Parse the argument number
    int32_t argumentNumber = stou(segments[1]); // always unlocalized!
    UnicodeString argumentName;
    if (argumentNumber < 0) {
        if ( (isArgNumeric==TRUE) && (formatNumber !=0) ) {
            ec = U_INVALID_FORMAT_ERROR;
            return;
        }
        isArgNumeric = FALSE;
        argumentNumber=formatNumber;
    }
    if (!isArgNumeric) {
        if ( !isLegalArgName(segments[1]) ) {
            ec = U_INVALID_FORMAT_ERROR;
            return;
        }
        argumentName = segments[1];
    }

    // Parse the format, recording the argument type and creating a
    // new Format object (except for string arguments).
    Formattable::Type argType;
    Format *fmt = NULL;
    int32_t typeID, styleID;
    DateFormat::EStyle style;
    UnicodeString unquotedPattern, quotedPattern;
    UBool inQuote = FALSE;

    switch (typeID = findKeyword(segments[2], TYPE_IDS)) {

    case 0: // string
        argType = Formattable::kString;
        break;

    case 1: // number
        argType = Formattable::kDouble;

        switch (findKeyword(segments[3], NUMBER_STYLE_IDS)) {
        case 0: // default
            fmt = NumberFormat::createInstance(fLocale, ec);
            break;
        case 1: // currency
            fmt = NumberFormat::createCurrencyInstance(fLocale, ec);
            break;
        case 2: // percent
            fmt = NumberFormat::createPercentInstance(fLocale, ec);
            break;
        case 3: // integer
            argType = Formattable::kLong;
            fmt = createIntegerFormat(fLocale, ec);
            break;
        default: // pattern
            fmt = NumberFormat::createInstance(fLocale, ec);
            if (fmt) {
                DecimalFormat* decfmt = dynamic_cast<DecimalFormat*>(fmt);
                if (decfmt != NULL) {
                    decfmt->applyPattern(segments[3],parseError,ec);
                }
            }
            break;
        }
        break;

    case 2: // date
    case 3: // time
        argType = Formattable::kDate;
        styleID = findKeyword(segments[3], DATE_STYLE_IDS);
        style = (styleID >= 0) ? DATE_STYLES[styleID] : DateFormat::kDefault;

        if (typeID == 2) {
            fmt = DateFormat::createDateInstance(style, fLocale);
        } else {
            fmt = DateFormat::createTimeInstance(style, fLocale);
        }

        if (styleID < 0 && fmt != NULL) {
            SimpleDateFormat* sdtfmt = dynamic_cast<SimpleDateFormat*>(fmt);
            if (sdtfmt != NULL) {
                sdtfmt->applyPattern(segments[3]);
            }
        }
        break;

    case 4: // choice
        argType = Formattable::kDouble;

        fmt = new ChoiceFormat(segments[3], parseError, ec);
        break;

    case 5: // spellout
        argType = Formattable::kDouble;
        fmt = makeRBNF(URBNF_SPELLOUT, fLocale, segments[3], ec);
        break;
    case 6: // ordinal
        argType = Formattable::kDouble;
        fmt = makeRBNF(URBNF_ORDINAL, fLocale, segments[3], ec);
        break;
    case 7: // duration
        argType = Formattable::kDouble;
        fmt = makeRBNF(URBNF_DURATION, fLocale, segments[3], ec);
        break;
    case 8: // plural
    case 9: // Select
        if(typeID == 8)
            argType = Formattable::kDouble;
        else
            argType = Formattable::kString;
        quotedPattern = segments[3];
        for (int32_t i = 0; i < quotedPattern.length(); ++i) {
            UChar ch = quotedPattern.charAt(i);
            if (ch == SINGLE_QUOTE) {
                if (i+1 < quotedPattern.length() && quotedPattern.charAt(i+1)==SINGLE_QUOTE) {
                    unquotedPattern+=ch;
                    ++i;
                }
                else {
                    inQuote = !inQuote;
                }
            }
            else {
                unquotedPattern += ch;
            }
        }
        if(typeID == 8)
            fmt = new PluralFormat(fLocale, unquotedPattern, ec);
        else
            fmt = new SelectFormat(unquotedPattern, ec);
        break;
    default:
        argType = Formattable::kString;
        ec = U_ILLEGAL_ARGUMENT_ERROR;
        break;
    }

    if (fmt==NULL && argType!=Formattable::kString && U_SUCCESS(ec)) {
        ec = U_MEMORY_ALLOCATION_ERROR;
    }

    if (!allocateSubformats(formatNumber+1) ||
        !allocateArgTypes(argumentNumber+1)) {
        ec = U_MEMORY_ALLOCATION_ERROR;
    }

    if (U_FAILURE(ec)) {
        delete fmt;
        return;
    }

    // Parse succeeded; record results in our arrays
    subformats[formatNumber].format = fmt;
    subformats[formatNumber].offset = segments[0].length();
    if (isArgNumeric) {
        subformats[formatNumber].argName = NULL;
        subformats[formatNumber].argNum = argumentNumber;
    }
    else {
        subformats[formatNumber].argName = new UnicodeString(argumentName);
        subformats[formatNumber].argNum = -1;
    }
    subformatCount = formatNumber+1;

    // Careful here: argumentNumber may in general arrive out of
    // sequence, e.g., "There was {2} on {0,date} (see {1,number})."
    argTypes[argumentNumber] = argType;
    if (argumentNumber+1 > argTypeCount) {
        argTypeCount = argumentNumber+1;
    }
}

// -------------------------------------
// Finds the string, s, in the string array, list.
int32_t MessageFormat::findKeyword(const UnicodeString& s,
                                   const UChar * const *list)
{
    if (s.length() == 0)
        return 0; // default

    UnicodeString buffer = s;
    // Trims the space characters and turns all characters
    // in s to lower case.
    buffer.trim().toLower("");
    for (int32_t i = 0; list[i]; ++i) {
        if (!buffer.compare(list[i], u_strlen(list[i]))) {
            return i;
        }
    }
    return -1;
}

// -------------------------------------
// Checks the range of the source text to quote the special
// characters, { and ' and copy to target buffer.

void
MessageFormat::copyAndFixQuotes(const UnicodeString& source,
                                int32_t start,
                                int32_t end,
                                UnicodeString& appendTo)
{
    UBool gotLB = FALSE;

    for (int32_t i = start; i < end; ++i) {
        UChar ch = source[i];
        if (ch == LEFT_CURLY_BRACE) {
            appendTo += SINGLE_QUOTE;
            appendTo += LEFT_CURLY_BRACE;
            appendTo += SINGLE_QUOTE;
            gotLB = TRUE;
        }
        else if (ch == RIGHT_CURLY_BRACE) {
            if(gotLB) {
                appendTo += RIGHT_CURLY_BRACE;
                gotLB = FALSE;
            }
            else {
                // orig code.
                appendTo += SINGLE_QUOTE;
                appendTo += RIGHT_CURLY_BRACE;
                appendTo += SINGLE_QUOTE;
            }
        }
        else if (ch == SINGLE_QUOTE) {
            appendTo += SINGLE_QUOTE;
            appendTo += SINGLE_QUOTE;
        }
        else {
            appendTo += ch;
        }
    }
}

/**
 * Convenience method that ought to be in NumberFormat
 */
NumberFormat*
MessageFormat::createIntegerFormat(const Locale& locale, UErrorCode& status) const {
    NumberFormat *temp = NumberFormat::createInstance(locale, status);
    DecimalFormat *temp2;
    if (temp != NULL && (temp2 = dynamic_cast<DecimalFormat*>(temp)) != NULL) {
        temp2->setMaximumFractionDigits(0);
        temp2->setDecimalSeparatorAlwaysShown(FALSE);
        temp2->setParseIntegerOnly(TRUE);
    }

    return temp;
}

/**
 * Return the default number format.  Used to format a numeric
 * argument when subformats[i].format is NULL.  Returns NULL
 * on failure.
 *
 * Semantically const but may modify *this.
 */
const NumberFormat* MessageFormat::getDefaultNumberFormat(UErrorCode& ec) const {
    if (defaultNumberFormat == NULL) {
        MessageFormat* t = (MessageFormat*) this;
        t->defaultNumberFormat = NumberFormat::createInstance(fLocale, ec);
        if (U_FAILURE(ec)) {
            delete t->defaultNumberFormat;
            t->defaultNumberFormat = NULL;
        } else if (t->defaultNumberFormat == NULL) {
            ec = U_MEMORY_ALLOCATION_ERROR;
        }
    }
    return defaultNumberFormat;
}

/**
 * Return the default date format.  Used to format a date
 * argument when subformats[i].format is NULL.  Returns NULL
 * on failure.
 *
 * Semantically const but may modify *this.
 */
const DateFormat* MessageFormat::getDefaultDateFormat(UErrorCode& ec) const {
    if (defaultDateFormat == NULL) {
        MessageFormat* t = (MessageFormat*) this;
        t->defaultDateFormat = DateFormat::createDateTimeInstance(DateFormat::kShort, DateFormat::kShort, fLocale);
        if (t->defaultDateFormat == NULL) {
            ec = U_MEMORY_ALLOCATION_ERROR;
        }
    }
    return defaultDateFormat;
}

UBool
MessageFormat::usesNamedArguments() const {
    return !isArgNumeric;
}

UBool
MessageFormat::isLegalArgName(const UnicodeString& argName) const {
    if(!u_hasBinaryProperty(argName.charAt(0), idStart)) {
        return FALSE;
    }
    for (int32_t i=1; i<argName.length(); ++i) {
        if(!u_hasBinaryProperty(argName.charAt(i), idContinue)) {
            return FALSE;
        }
    }
    return TRUE;
}

int32_t
MessageFormat::getArgTypeCount() const {
        return argTypeCount;
}

FormatNameEnumeration::FormatNameEnumeration(UVector *fNameList, UErrorCode& /*status*/) {
    pos=0;
    fFormatNames = fNameList;
}

const UnicodeString*
FormatNameEnumeration::snext(UErrorCode& status) {
    if (U_SUCCESS(status) && pos < fFormatNames->size()) {
        return (const UnicodeString*)fFormatNames->elementAt(pos++);
    }
    return NULL;
}

void
FormatNameEnumeration::reset(UErrorCode& /*status*/) {
    pos=0;
}

int32_t
FormatNameEnumeration::count(UErrorCode& /*status*/) const {
       return (fFormatNames==NULL) ? 0 : fFormatNames->size();
}

FormatNameEnumeration::~FormatNameEnumeration() {
    UnicodeString *s;
    for (int32_t i=0; i<fFormatNames->size(); ++i) {
        if ((s=(UnicodeString *)fFormatNames->elementAt(i))!=NULL) {
            delete s;
        }
    }
    delete fFormatNames;
}
U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */

//eof
