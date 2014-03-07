/*
*******************************************************************************
* Copyright (C) 1997-2010, International Business Machines Corporation and
* others. All Rights Reserved.
*******************************************************************************
*
* File TXTBDRY.CPP
*
* Modification History:
*
*   Date        Name        Description
*   02/18/97    aliu        Converted from OpenClass.  Added DONE.
*   01/13/2000  helena      Added UErrorCode parameter to createXXXInstance methods.
*****************************************************************************************
*/

// *****************************************************************************
// This file was generated from the java source file BreakIterator.java
// *****************************************************************************

#include "unicode/utypes.h"

#if !UCONFIG_NO_BREAK_ITERATION

#include "unicode/rbbi.h"
#include "unicode/brkiter.h"
#include "unicode/udata.h"
#include "unicode/ures.h"
#include "unicode/ustring.h"
#include "ucln_cmn.h"
#include "cstring.h"
#include "umutex.h"
#include "servloc.h"
#include "locbased.h"
#include "uresimp.h"
#include "uassert.h"
#include "ubrkimpl.h"

// *****************************************************************************
// class BreakIterator
// This class implements methods for finding the location of boundaries in text.
// Instances of BreakIterator maintain a current position and scan over text
// returning the index of characters where boundaries occur.
// *****************************************************************************

U_NAMESPACE_BEGIN

// -------------------------------------

BreakIterator*
BreakIterator::buildInstance(const Locale& loc, const char *type, int32_t kind, UErrorCode &status)
{
    char fnbuff[256];
    char ext[4]={'\0'};
    char actualLocale[ULOC_FULLNAME_CAPACITY];
    int32_t size;
    const UChar* brkfname = NULL;
    UResourceBundle brkRulesStack;
    UResourceBundle brkNameStack;
    UResourceBundle *brkRules = &brkRulesStack;
    UResourceBundle *brkName  = &brkNameStack;
    RuleBasedBreakIterator *result = NULL;

    if (U_FAILURE(status))
        return NULL;

    ures_initStackObject(brkRules);
    ures_initStackObject(brkName);

    // Get the locale
    UResourceBundle *b = ures_open(U_ICUDATA_BRKITR, loc.getName(), &status);
    /* this is a hack for now. Should be fixed when the data is fetched from
        brk_index.txt */
    if(status==U_USING_DEFAULT_WARNING){
        status=U_ZERO_ERROR;
        ures_openFillIn(b, U_ICUDATA_BRKITR, "", &status);
    }

    // Get the "boundaries" array.
    if (U_SUCCESS(status)) {
        brkRules = ures_getByKeyWithFallback(b, "boundaries", brkRules, &status);
        // Get the string object naming the rules file
        brkName = ures_getByKeyWithFallback(brkRules, type, brkName, &status);
        // Get the actual string
        brkfname = ures_getString(brkName, &size, &status);
        U_ASSERT((size_t)size<sizeof(fnbuff));
        if ((size_t)size>=sizeof(fnbuff)) {
            size=0;
            if (U_SUCCESS(status)) {
                status = U_BUFFER_OVERFLOW_ERROR;
            }
        }

        // Use the string if we found it
        if (U_SUCCESS(status) && brkfname) {
            uprv_strncpy(actualLocale,
                ures_getLocale(brkName, &status),
                sizeof(actualLocale)/sizeof(actualLocale[0]));

            UChar* extStart=u_strchr(brkfname, 0x002e);
            int len = 0;
            if(extStart!=NULL){
                len = (int)(extStart-brkfname);
                u_UCharsToChars(extStart+1, ext, sizeof(ext)); // nul terminates the buff
                u_UCharsToChars(brkfname, fnbuff, len);
            }
            fnbuff[len]=0; // nul terminate
        }
    }

    ures_close(brkRules);
    ures_close(brkName);

    UDataMemory* file = udata_open(U_ICUDATA_BRKITR, ext, fnbuff, &status);
    if (U_FAILURE(status)) {
        ures_close(b);
        return NULL;
    }

    // Create a RuleBasedBreakIterator
    result = new RuleBasedBreakIterator(file, status);

    // If there is a result, set the valid locale and actual locale, and the kind
    if (U_SUCCESS(status) && result != NULL) {
        U_LOCALE_BASED(locBased, *(BreakIterator*)result);
        locBased.setLocaleIDs(ures_getLocaleByType(b, ULOC_VALID_LOCALE, &status), actualLocale);
        result->setBreakType(kind);
    }

    ures_close(b);

    if (U_FAILURE(status) && result != NULL) {  // Sometimes redundant check, but simple
        delete result;
        return NULL;
    }

    if (result == NULL) {
        udata_close(file);
        if (U_SUCCESS(status)) {
            status = U_MEMORY_ALLOCATION_ERROR;
        }
    }

    return result;
}

// Creates a break iterator for word breaks.
BreakIterator* U_EXPORT2
BreakIterator::createWordInstance(const Locale& key, UErrorCode& status)
{
    return createInstance(key, UBRK_WORD, status);
}

// -------------------------------------

// Creates a break iterator  for line breaks.
BreakIterator* U_EXPORT2
BreakIterator::createLineInstance(const Locale& key, UErrorCode& status)
{
    return createInstance(key, UBRK_LINE, status);
}

// -------------------------------------

// Creates a break iterator  for character breaks.
BreakIterator* U_EXPORT2
BreakIterator::createCharacterInstance(const Locale& key, UErrorCode& status)
{
    return createInstance(key, UBRK_CHARACTER, status);
}

// -------------------------------------

// Creates a break iterator  for sentence breaks.
BreakIterator* U_EXPORT2
BreakIterator::createSentenceInstance(const Locale& key, UErrorCode& status)
{
    return createInstance(key, UBRK_SENTENCE, status);
}

// -------------------------------------

// Creates a break iterator for title casing breaks.
BreakIterator* U_EXPORT2
BreakIterator::createTitleInstance(const Locale& key, UErrorCode& status)
{
    return createInstance(key, UBRK_TITLE, status);
}

// -------------------------------------

// Gets all the available locales that has localized text boundary data.
const Locale* U_EXPORT2
BreakIterator::getAvailableLocales(int32_t& count)
{
    return Locale::getAvailableLocales(count);
}

// ------------------------------------------
//
// Default constructor and destructor
//
//-------------------------------------------

BreakIterator::BreakIterator()
{
    fBufferClone = FALSE;
    *validLocale = *actualLocale = 0;
}

BreakIterator::~BreakIterator()
{
}

// ------------------------------------------
//
// Registration
//
//-------------------------------------------
#if !UCONFIG_NO_SERVICE

// -------------------------------------

class ICUBreakIteratorFactory : public ICUResourceBundleFactory {
protected:
    virtual UObject* handleCreate(const Locale& loc, int32_t kind, const ICUService* /*service*/, UErrorCode& status) const {
        return BreakIterator::makeInstance(loc, kind, status);
    }
};

// -------------------------------------

class ICUBreakIteratorService : public ICULocaleService {
public:
    ICUBreakIteratorService()
        : ICULocaleService(UNICODE_STRING("Break Iterator", 14))
    {
        UErrorCode status = U_ZERO_ERROR;
        registerFactory(new ICUBreakIteratorFactory(), status);
    }

    virtual UObject* cloneInstance(UObject* instance) const {
        return ((BreakIterator*)instance)->clone();
    }

    virtual UObject* handleDefault(const ICUServiceKey& key, UnicodeString* /*actualID*/, UErrorCode& status) const {
        LocaleKey& lkey = (LocaleKey&)key;
        int32_t kind = lkey.kind();
        Locale loc;
        lkey.currentLocale(loc);
        return BreakIterator::makeInstance(loc, kind, status);
    }

    virtual UBool isDefault() const {
        return countFactories() == 1;
    }
};

// -------------------------------------

U_NAMESPACE_END

// defined in ucln_cmn.h

static U_NAMESPACE_QUALIFIER ICULocaleService* gService = NULL;

/**
 * Release all static memory held by breakiterator.
 */
U_CDECL_BEGIN
static UBool U_CALLCONV breakiterator_cleanup(void) {
#if !UCONFIG_NO_SERVICE
    if (gService) {
        delete gService;
        gService = NULL;
    }
#endif
    return TRUE;
}
U_CDECL_END
U_NAMESPACE_BEGIN

static ICULocaleService*
getService(void)
{
    UBool needsInit;
    UMTX_CHECK(NULL, (UBool)(gService == NULL), needsInit);

    if (needsInit) {
        ICULocaleService  *tService = new ICUBreakIteratorService();
        umtx_lock(NULL);
        if (gService == NULL) {
            gService = tService;
            tService = NULL;
            ucln_common_registerCleanup(UCLN_COMMON_BREAKITERATOR, breakiterator_cleanup);
        }
        umtx_unlock(NULL);
        delete tService;
    }
    return gService;
}

// -------------------------------------

static inline UBool
hasService(void)
{
    UBool retVal;
    UMTX_CHECK(NULL, gService != NULL, retVal);
    return retVal;
}

// -------------------------------------

URegistryKey U_EXPORT2
BreakIterator::registerInstance(BreakIterator* toAdopt, const Locale& locale, UBreakIteratorType kind, UErrorCode& status)
{
    ICULocaleService *service = getService();
    if (service == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return NULL;
    }
    return service->registerInstance(toAdopt, locale, kind, status);
}

// -------------------------------------

UBool U_EXPORT2
BreakIterator::unregister(URegistryKey key, UErrorCode& status)
{
    if (U_SUCCESS(status)) {
        if (hasService()) {
            return gService->unregister(key, status);
        }
        status = U_MEMORY_ALLOCATION_ERROR;
    }
    return FALSE;
}

// -------------------------------------

StringEnumeration* U_EXPORT2
BreakIterator::getAvailableLocales(void)
{
    ICULocaleService *service = getService();
    if (service == NULL) {
        return NULL;
    }
    return service->getAvailableLocales();
}
#endif /* UCONFIG_NO_SERVICE */

// -------------------------------------

BreakIterator*
BreakIterator::createInstance(const Locale& loc, int32_t kind, UErrorCode& status)
{
    if (U_FAILURE(status)) {
        return NULL;
    }

#if !UCONFIG_NO_SERVICE
    if (hasService()) {
        Locale actualLoc("");
        BreakIterator *result = (BreakIterator*)gService->get(loc, kind, &actualLoc, status);
        // TODO: The way the service code works in ICU 2.8 is that if
        // there is a real registered break iterator, the actualLoc
        // will be populated, but if the handleDefault path is taken
        // (because nothing is registered that can handle the
        // requested locale) then the actualLoc comes back empty.  In
        // that case, the returned object already has its actual/valid
        // locale data populated (by makeInstance, which is what
        // handleDefault calls), so we don't touch it.  YES, A COMMENT
        // THIS LONG is a sign of bad code -- so the action item is to
        // revisit this in ICU 3.0 and clean it up/fix it/remove it.
        if (U_SUCCESS(status) && (result != NULL) && *actualLoc.getName() != 0) {
            U_LOCALE_BASED(locBased, *result);
            locBased.setLocaleIDs(actualLoc.getName(), actualLoc.getName());
        }
        return result;
    }
    else
#endif
    {
        return makeInstance(loc, kind, status);
    }
}

// -------------------------------------

BreakIterator*
BreakIterator::makeInstance(const Locale& loc, int32_t kind, UErrorCode& status)
{

    if (U_FAILURE(status)) {
        return NULL;
    }

    BreakIterator *result = NULL;
    switch (kind) {
    case UBRK_CHARACTER:
        result = BreakIterator::buildInstance(loc, "grapheme", kind, status);
        break;
    case UBRK_WORD:
        result = BreakIterator::buildInstance(loc, "word", kind, status);
        break;
    case UBRK_LINE:
        result = BreakIterator::buildInstance(loc, "line", kind, status);
        break;
    case UBRK_SENTENCE:
        result = BreakIterator::buildInstance(loc, "sentence", kind, status);
        break;
    case UBRK_TITLE:
        result = BreakIterator::buildInstance(loc, "title", kind, status);
        break;
    default:
        status = U_ILLEGAL_ARGUMENT_ERROR;
    }

    if (U_FAILURE(status)) {
        return NULL;
    }

    return result;
}

Locale
BreakIterator::getLocale(ULocDataLocaleType type, UErrorCode& status) const {
    U_LOCALE_BASED(locBased, *this);
    return locBased.getLocale(type, status);
}

const char *
BreakIterator::getLocaleID(ULocDataLocaleType type, UErrorCode& status) const {
    U_LOCALE_BASED(locBased, *this);
    return locBased.getLocaleID(type, status);
}

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_BREAK_ITERATION */

//eof
