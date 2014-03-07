/*
*******************************************************************************
* Copyright (C) 1997-2010, International Business Machines Corporation
* and others. All Rights Reserved.
*******************************************************************************
*/

#include "unicode/utypeinfo.h"  // for 'typeid' to work

#include "unicode/rbnf.h"

#if U_HAVE_RBNF

#include "unicode/normlzr.h"
#include "unicode/tblcoll.h"
#include "unicode/uchar.h"
#include "unicode/ucol.h"
#include "unicode/uloc.h"
#include "unicode/unum.h"
#include "unicode/ures.h"
#include "unicode/ustring.h"
#include "unicode/utf16.h"
#include "unicode/udata.h"
#include "nfrs.h"

#include "cmemory.h"
#include "cstring.h"
#include "util.h"
#include "uresimp.h"

// debugging
// #define DEBUG

#ifdef DEBUG
#include "stdio.h"
#endif

#define U_ICUDATA_RBNF U_ICUDATA_NAME U_TREE_SEPARATOR_STRING "rbnf"

static const UChar gPercentPercent[] =
{
    0x25, 0x25, 0
}; /* "%%" */

// All urbnf objects are created through openRules, so we init all of the
// Unicode string constants required by rbnf, nfrs, or nfr here.
static const UChar gLenientParse[] =
{
    0x25, 0x25, 0x6C, 0x65, 0x6E, 0x69, 0x65, 0x6E, 0x74, 0x2D, 0x70, 0x61, 0x72, 0x73, 0x65, 0x3A, 0
}; /* "%%lenient-parse:" */
static const UChar gSemiColon = 0x003B;
static const UChar gSemiPercent[] =
{
    0x3B, 0x25, 0
}; /* ";%" */

#define kSomeNumberOfBitsDiv2 22
#define kHalfMaxDouble (double)(1 << kSomeNumberOfBitsDiv2)
#define kMaxDouble (kHalfMaxDouble * kHalfMaxDouble)

// Temporary workaround - when noParse is true, do noting in parse.
// TODO: We need a real fix - see #6895/#6896
static const char *NO_SPELLOUT_PARSE_LANGUAGES[] = { "ga", NULL };

U_NAMESPACE_BEGIN

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(RuleBasedNumberFormat)

/*
This is a utility class. It does not use ICU's RTTI.
If ICU's RTTI is needed again, you can uncomment the RTTI code and derive from UObject.
Please make sure that intltest passes on Windows in Release mode,
since the string pooling per compilation unit will mess up how RTTI works.
The RTTI code was also removed due to lack of code coverage.
*/
class LocalizationInfo : public UMemory {
protected:
    virtual ~LocalizationInfo() {};
    uint32_t refcount;
    
public:
    LocalizationInfo() : refcount(0) {}
    
    LocalizationInfo* ref(void) {
        ++refcount;
        return this;
    }
    
    LocalizationInfo* unref(void) {
        if (refcount && --refcount == 0) {
            delete this;
        }
        return NULL;
    }
    
    virtual UBool operator==(const LocalizationInfo* rhs) const;
    inline  UBool operator!=(const LocalizationInfo* rhs) const { return !operator==(rhs); }
    
    virtual int32_t getNumberOfRuleSets(void) const = 0;
    virtual const UChar* getRuleSetName(int32_t index) const = 0;
    virtual int32_t getNumberOfDisplayLocales(void) const = 0;
    virtual const UChar* getLocaleName(int32_t index) const = 0;
    virtual const UChar* getDisplayName(int32_t localeIndex, int32_t ruleIndex) const = 0;
    
    virtual int32_t indexForLocale(const UChar* locale) const;
    virtual int32_t indexForRuleSet(const UChar* ruleset) const;
    
//    virtual UClassID getDynamicClassID() const = 0;
//    static UClassID getStaticClassID(void);
};

//UOBJECT_DEFINE_ABSTRACT_RTTI_IMPLEMENTATION(LocalizationInfo)

// if both strings are NULL, this returns TRUE
static UBool 
streq(const UChar* lhs, const UChar* rhs) {
    if (rhs == lhs) {
        return TRUE;
    }
    if (lhs && rhs) {
        return u_strcmp(lhs, rhs) == 0;
    }
    return FALSE;
}

UBool
LocalizationInfo::operator==(const LocalizationInfo* rhs) const {
    if (rhs) {
        if (this == rhs) {
            return TRUE;
        }
        
        int32_t rsc = getNumberOfRuleSets();
        if (rsc == rhs->getNumberOfRuleSets()) {
            for (int i = 0; i < rsc; ++i) {
                if (!streq(getRuleSetName(i), rhs->getRuleSetName(i))) {
                    return FALSE;
                }
            }
            int32_t dlc = getNumberOfDisplayLocales();
            if (dlc == rhs->getNumberOfDisplayLocales()) {
                for (int i = 0; i < dlc; ++i) {
                    const UChar* locale = getLocaleName(i);
                    int32_t ix = rhs->indexForLocale(locale);
                    // if no locale, ix is -1, getLocaleName returns null, so streq returns false
                    if (!streq(locale, rhs->getLocaleName(ix))) {
                        return FALSE;
                    }
                    for (int j = 0; j < rsc; ++j) {
                        if (!streq(getDisplayName(i, j), rhs->getDisplayName(ix, j))) {
                            return FALSE;
                        }
                    }
                }
                return TRUE;
            }
        }
    }
    return FALSE;
}

int32_t
LocalizationInfo::indexForLocale(const UChar* locale) const {
    for (int i = 0; i < getNumberOfDisplayLocales(); ++i) {
        if (streq(locale, getLocaleName(i))) {
            return i;
        }
    }
    return -1;
}

int32_t
LocalizationInfo::indexForRuleSet(const UChar* ruleset) const {
    if (ruleset) {
        for (int i = 0; i < getNumberOfRuleSets(); ++i) {
            if (streq(ruleset, getRuleSetName(i))) {
                return i;
            }
        }
    }
    return -1;
}


typedef void (*Fn_Deleter)(void*);

class VArray {
    void** buf;
    int32_t cap;
    int32_t size;
    Fn_Deleter deleter;
public:
    VArray() : buf(NULL), cap(0), size(0), deleter(NULL) {}
    
    VArray(Fn_Deleter del) : buf(NULL), cap(0), size(0), deleter(del) {}
    
    ~VArray() {
        if (deleter) {
            for (int i = 0; i < size; ++i) {
                (*deleter)(buf[i]);
            }
        }
        uprv_free(buf); 
    }
    
    int32_t length() {
        return size;
    }
    
    void add(void* elem, UErrorCode& status) {
        if (U_SUCCESS(status)) {
            if (size == cap) {
                if (cap == 0) {
                    cap = 1;
                } else if (cap < 256) {
                    cap *= 2;
                } else {
                    cap += 256;
                }
                if (buf == NULL) {
                    buf = (void**)uprv_malloc(cap * sizeof(void*));
                } else {
                    buf = (void**)uprv_realloc(buf, cap * sizeof(void*));
                }
                if (buf == NULL) {
                    // if we couldn't realloc, we leak the memory we've already allocated, but we're in deep trouble anyway
                    status = U_MEMORY_ALLOCATION_ERROR;
                    return;
                }
                void* start = &buf[size];
                size_t count = (cap - size) * sizeof(void*);
                uprv_memset(start, 0, count); // fill with nulls, just because
            }
            buf[size++] = elem;
        }
    }
    
    void** release(void) {
        void** result = buf;
        buf = NULL;
        cap = 0;
        size = 0;
        return result;
    }
};

class LocDataParser;

class StringLocalizationInfo : public LocalizationInfo {
    UChar* info;
    UChar*** data;
    int32_t numRuleSets;
    int32_t numLocales;

friend class LocDataParser;

    StringLocalizationInfo(UChar* i, UChar*** d, int32_t numRS, int32_t numLocs)
        : info(i), data(d), numRuleSets(numRS), numLocales(numLocs)
    {
    }
    
public:
    static StringLocalizationInfo* create(const UnicodeString& info, UParseError& perror, UErrorCode& status);
    
    virtual ~StringLocalizationInfo();
    virtual int32_t getNumberOfRuleSets(void) const { return numRuleSets; }
    virtual const UChar* getRuleSetName(int32_t index) const;
    virtual int32_t getNumberOfDisplayLocales(void) const { return numLocales; }
    virtual const UChar* getLocaleName(int32_t index) const;
    virtual const UChar* getDisplayName(int32_t localeIndex, int32_t ruleIndex) const;
    
//    virtual UClassID getDynamicClassID() const;
//    static UClassID getStaticClassID(void);
    
private:
    void init(UErrorCode& status) const;
};


enum {
    OPEN_ANGLE = 0x003c, /* '<' */
    CLOSE_ANGLE = 0x003e, /* '>' */
    COMMA = 0x002c,
    TICK = 0x0027,
    QUOTE = 0x0022,
    SPACE = 0x0020
};

/**
 * Utility for parsing a localization string and returning a StringLocalizationInfo*.
 */
class LocDataParser {
    UChar* data;
    const UChar* e;
    UChar* p;
    UChar ch;
    UParseError& pe;
    UErrorCode& ec;
    
public:
    LocDataParser(UParseError& parseError, UErrorCode& status) 
        : data(NULL), e(NULL), p(NULL), ch(0xffff), pe(parseError), ec(status) {}
    ~LocDataParser() {}
    
    /*
    * On a successful parse, return a StringLocalizationInfo*, otherwise delete locData, set perror and status,
    * and return NULL.  The StringLocalizationInfo will adopt locData if it is created.
    */
    StringLocalizationInfo* parse(UChar* data, int32_t len);
    
private:
    
    void inc(void) { ++p; ch = 0xffff; }
    UBool checkInc(UChar c) { if (p < e && (ch == c || *p == c)) { inc(); return TRUE; } return FALSE; }
    UBool check(UChar c) { return p < e && (ch == c || *p == c); }
    void skipWhitespace(void) { while (p < e && uprv_isRuleWhiteSpace(ch != 0xffff ? ch : *p)) inc();}
    UBool inList(UChar c, const UChar* list) const {
        if (*list == SPACE && uprv_isRuleWhiteSpace(c)) return TRUE;
        while (*list && *list != c) ++list; return *list == c;
    }
    void parseError(const char* msg);
    
    StringLocalizationInfo* doParse(void);
        
    UChar** nextArray(int32_t& requiredLength);
    UChar*  nextString(void);
};

#ifdef DEBUG
#define ERROR(msg) parseError(msg); return NULL;
#else
#define ERROR(msg) parseError(NULL); return NULL;
#endif
        

static const UChar DQUOTE_STOPLIST[] = { 
    QUOTE, 0
};

static const UChar SQUOTE_STOPLIST[] = { 
    TICK, 0
};

static const UChar NOQUOTE_STOPLIST[] = { 
    SPACE, COMMA, CLOSE_ANGLE, OPEN_ANGLE, TICK, QUOTE, 0
};

static void
DeleteFn(void* p) {
  uprv_free(p);
}

StringLocalizationInfo*
LocDataParser::parse(UChar* _data, int32_t len) {
    if (U_FAILURE(ec)) {
        if (_data) uprv_free(_data);
        return NULL;
    }

    pe.line = 0;
    pe.offset = -1;
    pe.postContext[0] = 0;
    pe.preContext[0] = 0;

    if (_data == NULL) {
        ec = U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    }

    if (len <= 0) {
        ec = U_ILLEGAL_ARGUMENT_ERROR;
        uprv_free(_data);
        return NULL;
    }

    data = _data;
    e = data + len;
    p = _data;
    ch = 0xffff;

    return doParse();
}


StringLocalizationInfo*
LocDataParser::doParse(void) {
    skipWhitespace();
    if (!checkInc(OPEN_ANGLE)) {
        ERROR("Missing open angle");
    } else {
        VArray array(DeleteFn);
        UBool mightHaveNext = TRUE;
        int32_t requiredLength = -1;
        while (mightHaveNext) {
            mightHaveNext = FALSE;
            UChar** elem = nextArray(requiredLength);
            skipWhitespace();
            UBool haveComma = check(COMMA);
            if (elem) {
                array.add(elem, ec);
                if (haveComma) {
                    inc();
                    mightHaveNext = TRUE;
                }
            } else if (haveComma) {
                ERROR("Unexpected character");
            }
        }

        skipWhitespace();
        if (!checkInc(CLOSE_ANGLE)) {
            if (check(OPEN_ANGLE)) {
                ERROR("Missing comma in outer array");
            } else {
                ERROR("Missing close angle bracket in outer array");
            }
        }

        skipWhitespace();
        if (p != e) {
            ERROR("Extra text after close of localization data");
        }

        array.add(NULL, ec);
        if (U_SUCCESS(ec)) {
            int32_t numLocs = array.length() - 2; // subtract first, NULL
            UChar*** result = (UChar***)array.release();
            
            return new StringLocalizationInfo(data, result, requiredLength-2, numLocs); // subtract first, NULL
        }
    }
  
    ERROR("Unknown error");
}

UChar**
LocDataParser::nextArray(int32_t& requiredLength) {
    if (U_FAILURE(ec)) {
        return NULL;
    }
    
    skipWhitespace();
    if (!checkInc(OPEN_ANGLE)) {
        ERROR("Missing open angle");
    }

    VArray array;
    UBool mightHaveNext = TRUE;
    while (mightHaveNext) {
        mightHaveNext = FALSE;
        UChar* elem = nextString();
        skipWhitespace();
        UBool haveComma = check(COMMA);
        if (elem) {
            array.add(elem, ec);
            if (haveComma) {
                inc();
                mightHaveNext = TRUE;
            }
        } else if (haveComma) {
            ERROR("Unexpected comma");
        }
    }
    skipWhitespace();
    if (!checkInc(CLOSE_ANGLE)) {
        if (check(OPEN_ANGLE)) {
            ERROR("Missing close angle bracket in inner array");
        } else {
            ERROR("Missing comma in inner array");
        }
    }

    array.add(NULL, ec);
    if (U_SUCCESS(ec)) {
        if (requiredLength == -1) {
            requiredLength = array.length() + 1;
        } else if (array.length() != requiredLength) {
            ec = U_ILLEGAL_ARGUMENT_ERROR;
            ERROR("Array not of required length");
        }
        
        return (UChar**)array.release();
    }
    ERROR("Unknown Error");
}

UChar*
LocDataParser::nextString() {
    UChar* result = NULL;
    
    skipWhitespace();
    if (p < e) {
        const UChar* terminators;
        UChar c = *p;
        UBool haveQuote = c == QUOTE || c == TICK;
        if (haveQuote) {
            inc();
            terminators = c == QUOTE ? DQUOTE_STOPLIST : SQUOTE_STOPLIST;
        } else {
            terminators = NOQUOTE_STOPLIST;
        }
        UChar* start = p;
        while (p < e && !inList(*p, terminators)) ++p;
        if (p == e) {
            ERROR("Unexpected end of data");
        }
        
        UChar x = *p;
        if (p > start) {
            ch = x;
            *p = 0x0; // terminate by writing to data
            result = start; // just point into data
        }
        if (haveQuote) {
            if (x != c) {
                ERROR("Missing matching quote");
            } else if (p == start) {
                ERROR("Empty string");
            }
            inc();
        } else if (x == OPEN_ANGLE || x == TICK || x == QUOTE) {
            ERROR("Unexpected character in string");
        }
    }

    // ok for there to be no next string
    return result;
}

void
LocDataParser::parseError(const char* /*str*/) {
    if (!data) {
        return;
    }

    const UChar* start = p - U_PARSE_CONTEXT_LEN - 1;
    if (start < data) {
        start = data;
    }
    for (UChar* x = p; --x >= start;) {
        if (!*x) {
            start = x+1;
            break;
        }
    }
    const UChar* limit = p + U_PARSE_CONTEXT_LEN - 1;
    if (limit > e) {
        limit = e;
    }
    u_strncpy(pe.preContext, start, (int32_t)(p-start));
    pe.preContext[p-start] = 0;
    u_strncpy(pe.postContext, p, (int32_t)(limit-p));
    pe.postContext[limit-p] = 0;
    pe.offset = (int32_t)(p - data);
    
#ifdef DEBUG
    fprintf(stderr, "%s at or near character %d: ", str, p-data);

    UnicodeString msg;
    msg.append(start, p - start);
    msg.append((UChar)0x002f); /* SOLIDUS/SLASH */
    msg.append(p, limit-p);
    msg.append("'");
    
    char buf[128];
    int32_t len = msg.extract(0, msg.length(), buf, 128);
    if (len >= 128) {
        buf[127] = 0;
    } else {
        buf[len] = 0;
    }
    fprintf(stderr, "%s\n", buf);
    fflush(stderr);
#endif
    
    uprv_free(data);
    data = NULL;
    p = NULL;
    e = NULL;
    
    if (U_SUCCESS(ec)) {
        ec = U_PARSE_ERROR;
    }
}

//UOBJECT_DEFINE_RTTI_IMPLEMENTATION(StringLocalizationInfo)

StringLocalizationInfo* 
StringLocalizationInfo::create(const UnicodeString& info, UParseError& perror, UErrorCode& status) {
    if (U_FAILURE(status)) {
        return NULL;
    }
    
    int32_t len = info.length();
    if (len == 0) {
        return NULL; // no error;
    }
    
    UChar* p = (UChar*)uprv_malloc(len * sizeof(UChar));
    if (!p) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return NULL;
    }
    info.extract(p, len, status);
    if (!U_FAILURE(status)) {
        status = U_ZERO_ERROR; // clear warning about non-termination
    }
    
    LocDataParser parser(perror, status);
    return parser.parse(p, len);
}

StringLocalizationInfo::~StringLocalizationInfo() {
    for (UChar*** p = (UChar***)data; *p; ++p) {
        // remaining data is simply pointer into our unicode string data.
        if (*p) uprv_free(*p);
    }
    if (data) uprv_free(data);
    if (info) uprv_free(info);
}


const UChar*
StringLocalizationInfo::getRuleSetName(int32_t index) const {
    if (index >= 0 && index < getNumberOfRuleSets()) {
        return data[0][index];
    }
    return NULL;
}

const UChar*
StringLocalizationInfo::getLocaleName(int32_t index) const {
    if (index >= 0 && index < getNumberOfDisplayLocales()) {
        return data[index+1][0];
    }
    return NULL;
}

const UChar*
StringLocalizationInfo::getDisplayName(int32_t localeIndex, int32_t ruleIndex) const {
    if (localeIndex >= 0 && localeIndex < getNumberOfDisplayLocales() &&
        ruleIndex >= 0 && ruleIndex < getNumberOfRuleSets()) {
        return data[localeIndex+1][ruleIndex+1];
    }
    return NULL;
}

// ----------

RuleBasedNumberFormat::RuleBasedNumberFormat(const UnicodeString& description, 
                                             const UnicodeString& locs,
                                             const Locale& alocale, UParseError& perror, UErrorCode& status)
  : ruleSets(NULL)
  , defaultRuleSet(NULL)
  , locale(alocale)
  , collator(NULL)
  , decimalFormatSymbols(NULL)
  , lenient(FALSE)
  , lenientParseRules(NULL)
  , localizations(NULL)
  , noParse(FALSE) //TODO: to be removed after #6895
{
  LocalizationInfo* locinfo = StringLocalizationInfo::create(locs, perror, status);
  init(description, locinfo, perror, status);
}

RuleBasedNumberFormat::RuleBasedNumberFormat(const UnicodeString& description, 
                                             const UnicodeString& locs,
                                             UParseError& perror, UErrorCode& status)
  : ruleSets(NULL)
  , defaultRuleSet(NULL)
  , locale(Locale::getDefault())
  , collator(NULL)
  , decimalFormatSymbols(NULL)
  , lenient(FALSE)
  , lenientParseRules(NULL)
  , localizations(NULL)
  , noParse(FALSE) //TODO: to be removed after #6895
{
  LocalizationInfo* locinfo = StringLocalizationInfo::create(locs, perror, status);
  init(description, locinfo, perror, status);
}

RuleBasedNumberFormat::RuleBasedNumberFormat(const UnicodeString& description, 
                                             LocalizationInfo* info,
                                             const Locale& alocale, UParseError& perror, UErrorCode& status)
  : ruleSets(NULL)
  , defaultRuleSet(NULL)
  , locale(alocale)
  , collator(NULL)
  , decimalFormatSymbols(NULL)
  , lenient(FALSE)
  , lenientParseRules(NULL)
  , localizations(NULL)
  , noParse(FALSE) //TODO: to be removed after #6895
{
  init(description, info, perror, status);
}

RuleBasedNumberFormat::RuleBasedNumberFormat(const UnicodeString& description, 
                         UParseError& perror, 
                         UErrorCode& status) 
  : ruleSets(NULL)
  , defaultRuleSet(NULL)
  , locale(Locale::getDefault())
  , collator(NULL)
  , decimalFormatSymbols(NULL)
  , lenient(FALSE)
  , lenientParseRules(NULL)
  , localizations(NULL)
  , noParse(FALSE) //TODO: to be removed after #6895
{
    init(description, NULL, perror, status);
}

RuleBasedNumberFormat::RuleBasedNumberFormat(const UnicodeString& description, 
                         const Locale& aLocale,
                         UParseError& perror, 
                         UErrorCode& status) 
  : ruleSets(NULL)
  , defaultRuleSet(NULL)
  , locale(aLocale)
  , collator(NULL)
  , decimalFormatSymbols(NULL)
  , lenient(FALSE)
  , lenientParseRules(NULL)
  , localizations(NULL)
  , noParse(FALSE) //TODO: to be removed after #6895
{
    init(description, NULL, perror, status);
}

RuleBasedNumberFormat::RuleBasedNumberFormat(URBNFRuleSetTag tag, const Locale& alocale, UErrorCode& status)
  : ruleSets(NULL)
  , defaultRuleSet(NULL)
  , locale(alocale)
  , collator(NULL)
  , decimalFormatSymbols(NULL)
  , lenient(FALSE)
  , lenientParseRules(NULL)
  , localizations(NULL)
{
    if (U_FAILURE(status)) {
        return;
    }

    const char* rules_tag = "RBNFRules";
    const char* fmt_tag = "";
    switch (tag) {
    case URBNF_SPELLOUT: fmt_tag = "SpelloutRules"; break;
    case URBNF_ORDINAL: fmt_tag = "OrdinalRules"; break;
    case URBNF_DURATION: fmt_tag = "DurationRules"; break;
    case URBNF_NUMBERING_SYSTEM: fmt_tag = "NumberingSystemRules"; break;
    default: status = U_ILLEGAL_ARGUMENT_ERROR; return;
    }

    // TODO: read localization info from resource
    LocalizationInfo* locinfo = NULL;

    int32_t len = 0;
    UResourceBundle* nfrb = ures_open(U_ICUDATA_RBNF, locale.getName(), &status);
    if (U_SUCCESS(status)) {
        setLocaleIDs(ures_getLocaleByType(nfrb, ULOC_VALID_LOCALE, &status),
                     ures_getLocaleByType(nfrb, ULOC_ACTUAL_LOCALE, &status));

        UResourceBundle* rbnfRules = ures_getByKeyWithFallback(nfrb, rules_tag, NULL, &status);
        if (U_FAILURE(status)) {
            ures_close(nfrb);
        }
        UResourceBundle* ruleSets = ures_getByKeyWithFallback(rbnfRules, fmt_tag, NULL, &status);
        if (U_FAILURE(status)) {
            ures_close(rbnfRules);
            ures_close(nfrb);
            return;
        }
        
        UnicodeString desc;
        while (ures_hasNext(ruleSets)) {
           const UChar* currentString = ures_getNextString(ruleSets,&len,NULL,&status);
           desc.append(currentString);
        }
        UParseError perror;
        

        init (desc, locinfo, perror, status);

        //TODO: we need a real fix - see #6895 / #6896
        noParse = FALSE;
        if (tag == URBNF_SPELLOUT) {
            const char *lang = alocale.getLanguage();
            for (int32_t i = 0; NO_SPELLOUT_PARSE_LANGUAGES[i] != NULL; i++) {
                if (uprv_strcmp(lang, NO_SPELLOUT_PARSE_LANGUAGES[i]) == 0) {
                    noParse = TRUE;
                    break;
                }
            }
        }
        //TODO: end

        ures_close(ruleSets);
        ures_close(rbnfRules);
    }
    ures_close(nfrb);
}

RuleBasedNumberFormat::RuleBasedNumberFormat(const RuleBasedNumberFormat& rhs)
  : NumberFormat(rhs)
  , ruleSets(NULL)
  , defaultRuleSet(NULL)
  , locale(rhs.locale)
  , collator(NULL)
  , decimalFormatSymbols(NULL)
  , lenient(FALSE)
  , lenientParseRules(NULL)
  , localizations(NULL)
{
    this->operator=(rhs);
}

// --------

RuleBasedNumberFormat&
RuleBasedNumberFormat::operator=(const RuleBasedNumberFormat& rhs)
{
    UErrorCode status = U_ZERO_ERROR;
    dispose();
    locale = rhs.locale;
    lenient = rhs.lenient;

    UnicodeString rules = rhs.getRules();
    UParseError perror;
    init(rules, rhs.localizations ? rhs.localizations->ref() : NULL, perror, status);

    //TODO: remove below when we fix the parse bug - See #6895 / #6896
    noParse = rhs.noParse;

    return *this;
}

RuleBasedNumberFormat::~RuleBasedNumberFormat()
{
    dispose();
}

Format*
RuleBasedNumberFormat::clone(void) const
{
    RuleBasedNumberFormat * result = NULL;
    UnicodeString rules = getRules();
    UErrorCode status = U_ZERO_ERROR;
    UParseError perror;
    result = new RuleBasedNumberFormat(rules, localizations, locale, perror, status);
    /* test for NULL */
    if (result == 0) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return 0;
    }
    if (U_FAILURE(status)) {
        delete result;
        result = 0;
    } else {
        result->lenient = lenient;

        //TODO: remove below when we fix the parse bug - See #6895 / #6896
        result->noParse = noParse;
    }
    return result;
}

UBool
RuleBasedNumberFormat::operator==(const Format& other) const
{
    if (this == &other) {
        return TRUE;
    }

    if (typeid(*this) == typeid(other)) {
        const RuleBasedNumberFormat& rhs = (const RuleBasedNumberFormat&)other;
        if (locale == rhs.locale &&
            lenient == rhs.lenient &&
            (localizations == NULL 
                ? rhs.localizations == NULL 
                : (rhs.localizations == NULL 
                    ? FALSE
                    : *localizations == rhs.localizations))) {

            NFRuleSet** p = ruleSets;
            NFRuleSet** q = rhs.ruleSets;
            if (p == NULL) {
                return q == NULL;
            } else if (q == NULL) {
                return FALSE;
            }
            while (*p && *q && (**p == **q)) {
                ++p;
                ++q;
            }
            return *q == NULL && *p == NULL;
        }
    }

    return FALSE;
}

UnicodeString
RuleBasedNumberFormat::getRules() const
{
    UnicodeString result;
    if (ruleSets != NULL) {
        for (NFRuleSet** p = ruleSets; *p; ++p) {
            (*p)->appendRules(result);
        }
    }
    return result;
}

UnicodeString
RuleBasedNumberFormat::getRuleSetName(int32_t index) const
{
    if (localizations) {
      UnicodeString string(TRUE, localizations->getRuleSetName(index), (int32_t)-1);
      return string;
    } else if (ruleSets) {
        UnicodeString result;
        for (NFRuleSet** p = ruleSets; *p; ++p) {
            NFRuleSet* rs = *p;
            if (rs->isPublic()) {
                if (--index == -1) {
                    rs->getName(result);
                    return result;
                }
            }
        }
    }
    UnicodeString empty;
    return empty;
}

int32_t
RuleBasedNumberFormat::getNumberOfRuleSetNames() const
{
    int32_t result = 0;
    if (localizations) {
      result = localizations->getNumberOfRuleSets();
    } else if (ruleSets) {
        for (NFRuleSet** p = ruleSets; *p; ++p) {
            if ((**p).isPublic()) {
                ++result;
            }
        }
    }
    return result;
}

int32_t 
RuleBasedNumberFormat::getNumberOfRuleSetDisplayNameLocales(void) const {
    if (localizations) {
        return localizations->getNumberOfDisplayLocales();
    }
    return 0;
}

Locale 
RuleBasedNumberFormat::getRuleSetDisplayNameLocale(int32_t index, UErrorCode& status) const {
    if (U_FAILURE(status)) {
        return Locale("");
    }
    if (localizations && index >= 0 && index < localizations->getNumberOfDisplayLocales()) {
        UnicodeString name(TRUE, localizations->getLocaleName(index), -1);
        char buffer[64];
        int32_t cap = name.length() + 1;
        char* bp = buffer;
        if (cap > 64) {
            bp = (char *)uprv_malloc(cap);
            if (bp == NULL) {
                status = U_MEMORY_ALLOCATION_ERROR;
                return Locale("");
            }
        }
        name.extract(0, name.length(), bp, cap, UnicodeString::kInvariant);
        Locale retLocale(bp);
        if (bp != buffer) {
            uprv_free(bp);
        }
        return retLocale;
    }
    status = U_ILLEGAL_ARGUMENT_ERROR;
    Locale retLocale;
    return retLocale;
}

UnicodeString 
RuleBasedNumberFormat::getRuleSetDisplayName(int32_t index, const Locale& localeParam) {
    if (localizations && index >= 0 && index < localizations->getNumberOfRuleSets()) {
        UnicodeString localeName(localeParam.getBaseName(), -1, UnicodeString::kInvariant); 
        int32_t len = localeName.length();
        UChar* localeStr = localeName.getBuffer(len + 1);
        while (len >= 0) {
            localeStr[len] = 0;
            int32_t ix = localizations->indexForLocale(localeStr);
            if (ix >= 0) {
                UnicodeString name(TRUE, localizations->getDisplayName(ix, index), -1);
                return name;
            }
            
            // trim trailing portion, skipping over ommitted sections
            do { --len;} while (len > 0 && localeStr[len] != 0x005f); // underscore
            while (len > 0 && localeStr[len-1] == 0x005F) --len;
        }
        UnicodeString name(TRUE, localizations->getRuleSetName(index), -1);
        return name;
    }
    UnicodeString bogus;
    bogus.setToBogus();
    return bogus;
}

UnicodeString 
RuleBasedNumberFormat::getRuleSetDisplayName(const UnicodeString& ruleSetName, const Locale& localeParam) {
    if (localizations) {
        UnicodeString rsn(ruleSetName);
        int32_t ix = localizations->indexForRuleSet(rsn.getTerminatedBuffer());
        return getRuleSetDisplayName(ix, localeParam);
    }
    UnicodeString bogus;
    bogus.setToBogus();
    return bogus;
}

NFRuleSet*
RuleBasedNumberFormat::findRuleSet(const UnicodeString& name, UErrorCode& status) const
{
    if (U_SUCCESS(status) && ruleSets) {
        for (NFRuleSet** p = ruleSets; *p; ++p) {
            NFRuleSet* rs = *p;
            if (rs->isNamed(name)) {
                return rs;
            }
        }
        status = U_ILLEGAL_ARGUMENT_ERROR;
    }
    return NULL;
}

UnicodeString&
RuleBasedNumberFormat::format(int32_t number,
                              UnicodeString& toAppendTo,
                              FieldPosition& /* pos */) const
{
    if (defaultRuleSet) defaultRuleSet->format((int64_t)number, toAppendTo, toAppendTo.length());
    return toAppendTo;
}


UnicodeString&
RuleBasedNumberFormat::format(int64_t number,
                              UnicodeString& toAppendTo,
                              FieldPosition& /* pos */) const
{
    if (defaultRuleSet) defaultRuleSet->format(number, toAppendTo, toAppendTo.length());
    return toAppendTo;
}


UnicodeString&
RuleBasedNumberFormat::format(double number,
                              UnicodeString& toAppendTo,
                              FieldPosition& /* pos */) const
{
    if (defaultRuleSet) defaultRuleSet->format(number, toAppendTo, toAppendTo.length());
    return toAppendTo;
}


UnicodeString&
RuleBasedNumberFormat::format(int32_t number,
                              const UnicodeString& ruleSetName,
                              UnicodeString& toAppendTo,
                              FieldPosition& /* pos */,
                              UErrorCode& status) const
{
    // return format((int64_t)number, ruleSetName, toAppendTo, pos, status);
    if (U_SUCCESS(status)) {
        if (ruleSetName.indexOf(gPercentPercent) == 0) {
            // throw new IllegalArgumentException("Can't use internal rule set");
            status = U_ILLEGAL_ARGUMENT_ERROR;
        } else {
            NFRuleSet *rs = findRuleSet(ruleSetName, status);
            if (rs) {
                rs->format((int64_t)number, toAppendTo, toAppendTo.length());
            }
        }
    }
    return toAppendTo;
}


UnicodeString&
RuleBasedNumberFormat::format(int64_t number,
                              const UnicodeString& ruleSetName,
                              UnicodeString& toAppendTo,
                              FieldPosition& /* pos */,
                              UErrorCode& status) const
{
    if (U_SUCCESS(status)) {
        if (ruleSetName.indexOf(gPercentPercent) == 0) {
            // throw new IllegalArgumentException("Can't use internal rule set");
            status = U_ILLEGAL_ARGUMENT_ERROR;
        } else {
            NFRuleSet *rs = findRuleSet(ruleSetName, status);
            if (rs) {
                rs->format(number, toAppendTo, toAppendTo.length());
            }
        }
    }
    return toAppendTo;
}


// make linker happy
UnicodeString&
RuleBasedNumberFormat::format(const Formattable& obj,
                              UnicodeString& toAppendTo,
                              FieldPosition& pos,
                              UErrorCode& status) const
{
    return NumberFormat::format(obj, toAppendTo, pos, status);
}

UnicodeString&
RuleBasedNumberFormat::format(double number,
                              const UnicodeString& ruleSetName,
                              UnicodeString& toAppendTo,
                              FieldPosition& /* pos */,
                              UErrorCode& status) const
{
    if (U_SUCCESS(status)) {
        if (ruleSetName.indexOf(gPercentPercent) == 0) {
            // throw new IllegalArgumentException("Can't use internal rule set");
            status = U_ILLEGAL_ARGUMENT_ERROR;
        } else {
            NFRuleSet *rs = findRuleSet(ruleSetName, status);
            if (rs) {
                rs->format(number, toAppendTo, toAppendTo.length());
            }
        }
    }
    return toAppendTo;
}

void
RuleBasedNumberFormat::parse(const UnicodeString& text,
                             Formattable& result,
                             ParsePosition& parsePosition) const
{
    //TODO: We need a real fix.  See #6895 / #6896
    if (noParse) {
        // skip parsing
        parsePosition.setErrorIndex(0);
        return;
    }

    if (!ruleSets) {
        parsePosition.setErrorIndex(0);
        return;
    }

    UnicodeString workingText(text, parsePosition.getIndex());
    ParsePosition workingPos(0);

    ParsePosition high_pp(0);
    Formattable high_result;

    for (NFRuleSet** p = ruleSets; *p; ++p) {
        NFRuleSet *rp = *p;
        if (rp->isPublic() && rp->isParseable()) {
            ParsePosition working_pp(0);
            Formattable working_result;

            rp->parse(workingText, working_pp, kMaxDouble, working_result);
            if (working_pp.getIndex() > high_pp.getIndex()) {
                high_pp = working_pp;
                high_result = working_result;

                if (high_pp.getIndex() == workingText.length()) {
                    break;
                }
            }
        }
    }

    int32_t startIndex = parsePosition.getIndex();
    parsePosition.setIndex(startIndex + high_pp.getIndex());
    if (high_pp.getIndex() > 0) {
        parsePosition.setErrorIndex(-1);
    } else {
        int32_t errorIndex = (high_pp.getErrorIndex()>0)? high_pp.getErrorIndex(): 0;
        parsePosition.setErrorIndex(startIndex + errorIndex);
    }
    result = high_result;
    if (result.getType() == Formattable::kDouble) {
        int32_t r = (int32_t)result.getDouble();
        if ((double)r == result.getDouble()) {
            result.setLong(r);
        }
    }
}

#if !UCONFIG_NO_COLLATION

void
RuleBasedNumberFormat::setLenient(UBool enabled)
{
    lenient = enabled;
    if (!enabled && collator) {
        delete collator;
        collator = NULL;
    }
}

#endif

void 
RuleBasedNumberFormat::setDefaultRuleSet(const UnicodeString& ruleSetName, UErrorCode& status) {
    if (U_SUCCESS(status)) {
        if (ruleSetName.isEmpty()) {
          if (localizations) {
              UnicodeString name(TRUE, localizations->getRuleSetName(0), -1);
              defaultRuleSet = findRuleSet(name, status);
          } else {
            initDefaultRuleSet();
          }
        } else if (ruleSetName.startsWith(UNICODE_STRING_SIMPLE("%%"))) {
            status = U_ILLEGAL_ARGUMENT_ERROR;
        } else {
            NFRuleSet* result = findRuleSet(ruleSetName, status);
            if (result != NULL) {
                defaultRuleSet = result;
            }
        }
    }
}

UnicodeString
RuleBasedNumberFormat::getDefaultRuleSetName() const {
  UnicodeString result;
  if (defaultRuleSet && defaultRuleSet->isPublic()) {
    defaultRuleSet->getName(result);
  } else {
    result.setToBogus();
  }
  return result;
}

void 
RuleBasedNumberFormat::initDefaultRuleSet()
{
    defaultRuleSet = NULL;
    if (!ruleSets) {
      return;
    }

    const UnicodeString spellout = UNICODE_STRING_SIMPLE("%spellout-numbering");
    const UnicodeString ordinal = UNICODE_STRING_SIMPLE("%digits-ordinal");
    const UnicodeString duration = UNICODE_STRING_SIMPLE("%duration");

    NFRuleSet**p = &ruleSets[0];
    while (*p) {
        if ((*p)->isNamed(spellout) || (*p)->isNamed(ordinal) || (*p)->isNamed(duration)) {
            defaultRuleSet = *p;
            return;
        } else {
            ++p;
        }
    }

    defaultRuleSet = *--p;
    if (!defaultRuleSet->isPublic()) {
        while (p != ruleSets) {
            if ((*--p)->isPublic()) {
                defaultRuleSet = *p;
                break;
            }
        }
    }
}


void
RuleBasedNumberFormat::init(const UnicodeString& rules, LocalizationInfo* localizationInfos,
                            UParseError& pErr, UErrorCode& status)
{
    // TODO: implement UParseError
    uprv_memset(&pErr, 0, sizeof(UParseError));
    // Note: this can leave ruleSets == NULL, so remaining code should check
    if (U_FAILURE(status)) {
        return;
    }

    this->localizations = localizationInfos == NULL ? NULL : localizationInfos->ref();

    UnicodeString description(rules);
    if (!description.length()) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return;
    }

    // start by stripping the trailing whitespace from all the rules
    // (this is all the whitespace follwing each semicolon in the
    // description).  This allows us to look for rule-set boundaries
    // by searching for ";%" without having to worry about whitespace
    // between the ; and the %
    stripWhitespace(description);

    // check to see if there's a set of lenient-parse rules.  If there
    // is, pull them out into our temporary holding place for them,
    // and delete them from the description before the real desciption-
    // parsing code sees them
    int32_t lp = description.indexOf(gLenientParse);
    if (lp != -1) {
        // we've got to make sure we're not in the middle of a rule
        // (where "%%lenient-parse" would actually get treated as
        // rule text)
        if (lp == 0 || description.charAt(lp - 1) == gSemiColon) {
            // locate the beginning and end of the actual collation
            // rules (there may be whitespace between the name and
            // the first token in the description)
            int lpEnd = description.indexOf(gSemiPercent, lp);

            if (lpEnd == -1) {
                lpEnd = description.length() - 1;
            }
            int lpStart = lp + u_strlen(gLenientParse);
            while (uprv_isRuleWhiteSpace(description.charAt(lpStart))) {
                ++lpStart;
            }

            // copy out the lenient-parse rules and delete them
            // from the description
            lenientParseRules = new UnicodeString();
            /* test for NULL */
            if (lenientParseRules == 0) {
                status = U_MEMORY_ALLOCATION_ERROR;
                return;
            }
            lenientParseRules->setTo(description, lpStart, lpEnd - lpStart);

            description.remove(lp, lpEnd + 1 - lp);
        }
    }

    // pre-flight parsing the description and count the number of
    // rule sets (";%" marks the end of one rule set and the beginning
    // of the next)
    int numRuleSets = 0;
    for (int32_t p = description.indexOf(gSemiPercent); p != -1; p = description.indexOf(gSemiPercent, p)) {
        ++numRuleSets;
        ++p;
    }
    ++numRuleSets;

    // our rule list is an array of the appropriate size
    ruleSets = (NFRuleSet **)uprv_malloc((numRuleSets + 1) * sizeof(NFRuleSet *));
    /* test for NULL */
    if (ruleSets == 0) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return;
    }

    for (int i = 0; i <= numRuleSets; ++i) {
        ruleSets[i] = NULL;
    }

    // divide up the descriptions into individual rule-set descriptions
    // and store them in a temporary array.  At each step, we also
    // new up a rule set, but all this does is initialize its name
    // and remove it from its description.  We can't actually parse
    // the rest of the descriptions and finish initializing everything
    // because we have to know the names and locations of all the rule
    // sets before we can actually set everything up
    if(!numRuleSets) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }
    UnicodeString* ruleSetDescriptions = new UnicodeString[numRuleSets];
    if (ruleSetDescriptions == 0) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return;
    }

    {
        int curRuleSet = 0;
        int32_t start = 0;
        for (int32_t p = description.indexOf(gSemiPercent); p != -1; p = description.indexOf(gSemiPercent, start)) {
            ruleSetDescriptions[curRuleSet].setTo(description, start, p + 1 - start);
            ruleSets[curRuleSet] = new NFRuleSet(ruleSetDescriptions, curRuleSet, status);
            if (ruleSets[curRuleSet] == 0) {
                status = U_MEMORY_ALLOCATION_ERROR;
                goto cleanup;
            }
            ++curRuleSet;
            start = p + 1;
        }
        ruleSetDescriptions[curRuleSet].setTo(description, start, description.length() - start);
        ruleSets[curRuleSet] = new NFRuleSet(ruleSetDescriptions, curRuleSet, status);
        if (ruleSets[curRuleSet] == 0) {
            status = U_MEMORY_ALLOCATION_ERROR;
            goto cleanup;
        }
    }

    // now we can take note of the formatter's default rule set, which
    // is the last public rule set in the description (it's the last
    // rather than the first so that a user can create a new formatter
    // from an existing formatter and change its default behavior just
    // by appending more rule sets to the end)

    // {dlf} Initialization of a fraction rule set requires the default rule
    // set to be known.  For purposes of initialization, this is always the 
    // last public rule set, no matter what the localization data says.
    initDefaultRuleSet();

    // finally, we can go back through the temporary descriptions
    // list and finish seting up the substructure (and we throw
    // away the temporary descriptions as we go)
    {
        for (int i = 0; i < numRuleSets; i++) {
            ruleSets[i]->parseRules(ruleSetDescriptions[i], this, status);
        }
    }

    // Now that the rules are initialized, the 'real' default rule
    // set can be adjusted by the localization data.

    // The C code keeps the localization array as is, rather than building
    // a separate array of the public rule set names, so we have less work
    // to do here-- but we still need to check the names.
    
    if (localizationInfos) {
        // confirm the names, if any aren't in the rules, that's an error
        // it is ok if the rules contain public rule sets that are not in this list
        for (int32_t i = 0; i < localizationInfos->getNumberOfRuleSets(); ++i) {
            UnicodeString name(TRUE, localizationInfos->getRuleSetName(i), -1);
            NFRuleSet* rs = findRuleSet(name, status);
            if (rs == NULL) {
                break; // error
            }
            if (i == 0) {
                defaultRuleSet = rs;
            }
        }
    } else {
        defaultRuleSet = getDefaultRuleSet();
    }

cleanup:
    delete[] ruleSetDescriptions;
}

void
RuleBasedNumberFormat::stripWhitespace(UnicodeString& description)
{
    // iterate through the characters...
    UnicodeString result;

    int start = 0;
    while (start != -1 && start < description.length()) {
        // seek to the first non-whitespace character...
        while (start < description.length()
            && uprv_isRuleWhiteSpace(description.charAt(start))) {
            ++start;
        }

        // locate the next semicolon in the text and copy the text from
        // our current position up to that semicolon into the result
        int32_t p = description.indexOf(gSemiColon, start);
        if (p == -1) {
            // or if we don't find a semicolon, just copy the rest of
            // the string into the result
            result.append(description, start, description.length() - start);
            start = -1;
        }
        else if (p < description.length()) {
            result.append(description, start, p + 1 - start);
            start = p + 1;
        }

        // when we get here, we've seeked off the end of the sring, and
        // we terminate the loop (we continue until *start* is -1 rather
        // than until *p* is -1, because otherwise we'd miss the last
        // rule in the description)
        else {
            start = -1;
        }
    }

    description.setTo(result);
}


void
RuleBasedNumberFormat::dispose()
{
    if (ruleSets) {
        for (NFRuleSet** p = ruleSets; *p; ++p) {
            delete *p;
        }
        uprv_free(ruleSets);
        ruleSets = NULL;
    }

#if !UCONFIG_NO_COLLATION
    delete collator;
#endif
    collator = NULL;

    delete decimalFormatSymbols;
    decimalFormatSymbols = NULL;

    delete lenientParseRules;
    lenientParseRules = NULL;

    if (localizations) localizations = localizations->unref();
}


//-----------------------------------------------------------------------
// package-internal API
//-----------------------------------------------------------------------

/**
 * Returns the collator to use for lenient parsing.  The collator is lazily created:
 * this function creates it the first time it's called.
 * @return The collator to use for lenient parsing, or null if lenient parsing
 * is turned off.
*/
Collator*
RuleBasedNumberFormat::getCollator() const
{
#if !UCONFIG_NO_COLLATION
    if (!ruleSets) {
        return NULL;
    }

    // lazy-evaulate the collator
    if (collator == NULL && lenient) {
        // create a default collator based on the formatter's locale,
        // then pull out that collator's rules, append any additional
        // rules specified in the description, and create a _new_
        // collator based on the combinaiton of those rules

        UErrorCode status = U_ZERO_ERROR;

        Collator* temp = Collator::createInstance(locale, status);
        RuleBasedCollator* newCollator;
        if (U_SUCCESS(status) && (newCollator = dynamic_cast<RuleBasedCollator*>(temp)) != NULL) {
            if (lenientParseRules) {
                UnicodeString rules(newCollator->getRules());
                rules.append(*lenientParseRules);

                newCollator = new RuleBasedCollator(rules, status);
                // Exit if newCollator could not be created.
                if (newCollator == NULL) {
                	return NULL;
                }
            } else {
                temp = NULL;
            }
            if (U_SUCCESS(status)) {
                newCollator->setAttribute(UCOL_DECOMPOSITION_MODE, UCOL_ON, status);
                // cast away const
                ((RuleBasedNumberFormat*)this)->collator = newCollator;
            } else {
                delete newCollator;
            }
        }
        delete temp;
    }
#endif

    // if lenient-parse mode is off, this will be null
    // (see setLenientParseMode())
    return collator;
}


/**
 * Returns the DecimalFormatSymbols object that should be used by all DecimalFormat
 * instances owned by this formatter.  This object is lazily created: this function
 * creates it the first time it's called.
 * @return The DecimalFormatSymbols object that should be used by all DecimalFormat
 * instances owned by this formatter.
*/
DecimalFormatSymbols*
RuleBasedNumberFormat::getDecimalFormatSymbols() const
{
    // lazy-evaluate the DecimalFormatSymbols object.  This object
    // is shared by all DecimalFormat instances belonging to this
    // formatter
    if (decimalFormatSymbols == NULL) {
        UErrorCode status = U_ZERO_ERROR;
        DecimalFormatSymbols* temp = new DecimalFormatSymbols(locale, status);
        if (U_SUCCESS(status)) {
            ((RuleBasedNumberFormat*)this)->decimalFormatSymbols = temp;
        } else {
            delete temp;
        }
    }
    return decimalFormatSymbols;
}

U_NAMESPACE_END

/* U_HAVE_RBNF */
#endif
