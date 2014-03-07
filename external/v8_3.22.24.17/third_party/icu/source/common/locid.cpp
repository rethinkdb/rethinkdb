/*
 **********************************************************************
 *   Copyright (C) 1997-2010, International Business Machines
 *   Corporation and others.  All Rights Reserved.
 **********************************************************************
*
* File locid.cpp
*
* Created by: Richard Gillam
*
* Modification History:
*
*   Date        Name        Description
*   02/11/97    aliu        Changed gLocPath to fgDataDirectory and added
*                           methods to get and set it.
*   04/02/97    aliu        Made operator!= inline; fixed return value
*                           of getName().
*   04/15/97    aliu        Cleanup for AIX/Win32.
*   04/24/97    aliu        Numerous changes per code review.
*   08/18/98    stephen     Changed getDisplayName()
*                           Added SIMPLIFIED_CHINESE, TRADITIONAL_CHINESE
*                           Added getISOCountries(), getISOLanguages(),
*                           getLanguagesForCountry()
*   03/16/99    bertrand    rehaul.
*   07/21/99    stephen     Added U_CFUNC setDefault
*   11/09/99    weiv        Added const char * getName() const;
*   04/12/00    srl         removing unicodestring api's and cached hash code
*   08/10/01    grhoten     Change the static Locales to accessor functions
******************************************************************************
*/


#include "unicode/locid.h"
#include "unicode/uloc.h"
#include "umutex.h"
#include "uassert.h"
#include "cmemory.h"
#include "cstring.h"
#include "uhash.h"
#include "ucln_cmn.h"

#define LENGTHOF(array) (int32_t)(sizeof(array)/sizeof((array)[0]))

typedef enum ELocalePos {
    eENGLISH,
    eFRENCH,
    eGERMAN,
    eITALIAN,
    eJAPANESE,
    eKOREAN,
    eCHINESE,

    eFRANCE,
    eGERMANY,
    eITALY,
    eJAPAN,
    eKOREA,
    eCHINA,      /* Alias for PRC */
    eTAIWAN,
    eUK,
    eUS,
    eCANADA,
    eCANADA_FRENCH,
    eROOT,


    //eDEFAULT,
    eMAX_LOCALES
} ELocalePos;

U_CFUNC int32_t locale_getKeywords(const char *localeID,
            char prev,
            char *keywords, int32_t keywordCapacity,
            char *values, int32_t valuesCapacity, int32_t *valLen,
            UBool valuesToo,
            UErrorCode *status);

static U_NAMESPACE_QUALIFIER Locale *gLocaleCache         = NULL;
static U_NAMESPACE_QUALIFIER Locale *gDefaultLocale       = NULL;
static UHashtable                   *gDefaultLocalesHashT = NULL;

U_CDECL_BEGIN
//
// Deleter function for Locales owned by the default Locale hash table/
//
static void U_CALLCONV
deleteLocale(void *obj) {
    delete (U_NAMESPACE_QUALIFIER Locale *) obj;
}

static UBool U_CALLCONV locale_cleanup(void)
{
    U_NAMESPACE_USE

    if (gLocaleCache) {
        delete [] gLocaleCache;
        gLocaleCache = NULL;
    }

    if (gDefaultLocalesHashT) {
        uhash_close(gDefaultLocalesHashT);   // Automatically deletes all elements, using deleter func.
        gDefaultLocalesHashT = NULL;
    }
    else if (gDefaultLocale) {
        // The cache wasn't created, and only one default locale was created.
        delete gDefaultLocale;
    }
    gDefaultLocale = NULL;

    return TRUE;
}
U_CDECL_END

U_NAMESPACE_BEGIN
//
//  locale_set_default_internal.
//
void locale_set_default_internal(const char *id)
{
    UErrorCode   status = U_ZERO_ERROR;
    UBool canonicalize = FALSE;

    // If given a NULL string for the locale id, grab the default
    //   name from the system.
    //   (Different from most other locale APIs, where a null name means use
    //    the current ICU default locale.)
    if (id == NULL) {
        umtx_lock(NULL);
        id = uprv_getDefaultLocaleID();
        umtx_unlock(NULL);
        canonicalize = TRUE; // always canonicalize host ID
    }

    // put the locale id into a canonical form,
    //   in preparation for looking up this locale in the hash table of
    //   already-created locale objects.
    //
    status = U_ZERO_ERROR;
    char localeNameBuf[512];

    if (canonicalize) {
        uloc_canonicalize(id, localeNameBuf, sizeof(localeNameBuf)-1, &status);
    } else {
        uloc_getName(id, localeNameBuf, sizeof(localeNameBuf)-1, &status);
    }
    localeNameBuf[sizeof(localeNameBuf)-1] = 0;  // Force null termination in event of
                                                 //   a long name filling the buffer.
                                                 //   (long names are truncated.)

    // Lazy creation of the hash table itself, if needed.
    UBool isOnlyLocale;
    UMTX_CHECK(NULL, (gDefaultLocale == NULL), isOnlyLocale);
    if (isOnlyLocale) {
        // We haven't seen this locale id before.
        // Create a new Locale object for it.
        Locale *newFirstDefault = new Locale(Locale::eBOGUS);
        if (newFirstDefault == NULL) {
            // No way to report errors from here.
            return;
        }
        newFirstDefault->init(localeNameBuf, FALSE);
        umtx_lock(NULL);
        if (gDefaultLocale == NULL) {
            gDefaultLocale = newFirstDefault;  // Assignment to gDefaultLocale must happen inside mutex
            newFirstDefault = NULL;
            ucln_common_registerCleanup(UCLN_COMMON_LOCALE, locale_cleanup);
        }
        // Else some other thread raced us through here, and set the new Locale.
        // Use the hash table next.
        umtx_unlock(NULL);
        if (newFirstDefault == NULL) {
            // We were successful in setting the locale, and we were the first one to set it.
            return;
        }
        // else start using the hash table.
    }

    // Lazy creation of the hash table itself, if needed.
    UBool hashTableNeedsInit;
    UMTX_CHECK(NULL, (gDefaultLocalesHashT == NULL), hashTableNeedsInit);
    if (hashTableNeedsInit) {
        status = U_ZERO_ERROR;
        UHashtable *tHashTable = uhash_open(uhash_hashChars, uhash_compareChars, NULL, &status);
        if (U_FAILURE(status)) {
            return;
        }
        uhash_setValueDeleter(tHashTable, deleteLocale);
        umtx_lock(NULL);
        if (gDefaultLocalesHashT == NULL) {
            gDefaultLocalesHashT = tHashTable;
            ucln_common_registerCleanup(UCLN_COMMON_LOCALE, locale_cleanup);
        } else {
            uhash_close(tHashTable);
            hashTableNeedsInit = FALSE;
        }
        umtx_unlock(NULL);
    }

    // Hash table lookup, key is the locale full name
    umtx_lock(NULL);
    Locale *newDefault = (Locale *)uhash_get(gDefaultLocalesHashT, localeNameBuf);
    if (newDefault != NULL) {
        // We have the requested locale in the hash table already.
        // Just set it as default.  Inside the mutex lock, for those troublesome processors.
        gDefaultLocale = newDefault;
        umtx_unlock(NULL);
    } else {
        umtx_unlock(NULL);
        // We haven't seen this locale id before.
        // Create a new Locale object for it.
        newDefault = new Locale(Locale::eBOGUS);
        if (newDefault == NULL) {
            // No way to report errors from here.
            return;
        }
        newDefault->init(localeNameBuf, FALSE);

        // Add newly created Locale to the hash table of default Locales
        const char *key = newDefault->getName();
        U_ASSERT(uprv_strcmp(key, localeNameBuf) == 0);
        umtx_lock(NULL);
        Locale *hashTableVal = (Locale *)uhash_get(gDefaultLocalesHashT, key);
        if (hashTableVal == NULL) {
            if (hashTableNeedsInit) {
                // This is the second request to set the locale.
                // Cache the first one.
                uhash_put(gDefaultLocalesHashT, (void *)gDefaultLocale->getName(), gDefaultLocale, &status);
            }
            uhash_put(gDefaultLocalesHashT, (void *)key, newDefault, &status);
            gDefaultLocale = newDefault;
            // ignore errors from hash table insert.  (Couldn't do anything anyway)
            // We can still set the default Locale,
            //  it just wont be cached, and will eventually leak.
        } else {
            // Some other thread raced us through here, and got the new Locale
            //   into the hash table before us.  Use that one.
            gDefaultLocale = hashTableVal;  // Assignment to gDefaultLocale must happen inside mutex
            delete newDefault;
        }
        umtx_unlock(NULL);
    }
}
U_NAMESPACE_END

/* sfb 07/21/99 */
U_CFUNC void
locale_set_default(const char *id)
{
    U_NAMESPACE_USE
    locale_set_default_internal(id);
}
/* end */

U_CFUNC const char *
locale_get_default(void)
{
    U_NAMESPACE_USE

    return Locale::getDefault().getName();
}


U_NAMESPACE_BEGIN

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(Locale)

/*Character separating the posix id fields*/
// '_'
// In the platform codepage.
#define SEP_CHAR '_'

Locale::~Locale()
{
    /*if fullName is on the heap, we free it*/
    if (fullName != fullNameBuffer)
    {
        uprv_free(fullName);
        fullName = NULL;
    }
    if (baseName && baseName != baseNameBuffer) {
        uprv_free(baseName);
        baseName = NULL;
    }
}

Locale::Locale()
    : UObject(), fullName(fullNameBuffer), baseName(NULL)
{
    init(NULL, FALSE);
}

/*
 * Internal constructor to allow construction of a locale object with
 *   NO side effects.   (Default constructor tries to get
 *   the default locale.)
 */
Locale::Locale(Locale::ELocaleType)
    : UObject(), fullName(fullNameBuffer), baseName(NULL)
{
    setToBogus();
}


Locale::Locale( const   char * newLanguage,
                const   char * newCountry,
                const   char * newVariant,
                const   char * newKeywords)
    : UObject(), fullName(fullNameBuffer), baseName(NULL)
{
    if( (newLanguage==NULL) && (newCountry == NULL) && (newVariant == NULL) )
    {
        init(NULL, FALSE); /* shortcut */
    }
    else
    {
        MaybeStackArray<char, ULOC_FULLNAME_CAPACITY> togo;
        int32_t size = 0;
        int32_t lsize = 0;
        int32_t csize = 0;
        int32_t vsize = 0;
        int32_t ksize = 0;
        char    *p;

        // Calculate the size of the resulting string.

        // Language
        if ( newLanguage != NULL )
        {
            lsize = (int32_t)uprv_strlen(newLanguage);
            size = lsize;
        }

        // _Country
        if ( newCountry != NULL )
        {
            csize = (int32_t)uprv_strlen(newCountry);
            size += csize;
        }

        // _Variant
        if ( newVariant != NULL )
        {
            // remove leading _'s
            while(newVariant[0] == SEP_CHAR)
            {
                newVariant++;
            }

            // remove trailing _'s
            vsize = (int32_t)uprv_strlen(newVariant);
            while( (vsize>1) && (newVariant[vsize-1] == SEP_CHAR) )
            {
                vsize--;
            }
        }

        if( vsize > 0 )
        {
            size += vsize;
        }

        // Separator rules:
        if ( vsize > 0 )
        {
            size += 2;  // at least: __v
        }
        else if ( csize > 0 )
        {
            size += 1;  // at least: _v
        }

        if ( newKeywords != NULL)
        {
            ksize = (int32_t)uprv_strlen(newKeywords);
            size += ksize + 1;
        }


        //  NOW we have the full locale string..

        /*if the whole string is longer than our internal limit, we need
        to go to the heap for temporary buffers*/
        if (size >= togo.getCapacity())
        {
            // If togo_heap could not be created, initialize with default settings.
            if (togo.resize(size+1) == NULL) {
                init(NULL, FALSE);
            }
        }

        togo[0] = 0;

        // Now, copy it back.
        p = togo.getAlias();
        if ( lsize != 0 )
        {
            uprv_strcpy(p, newLanguage);
            p += lsize;
        }

        if ( ( vsize != 0 ) || (csize != 0) )  // at least:  __v
        {                                      //            ^
            *p++ = SEP_CHAR;
        }

        if ( csize != 0 )
        {
            uprv_strcpy(p, newCountry);
            p += csize;
        }

        if ( vsize != 0)
        {
            *p++ = SEP_CHAR; // at least: __v

            uprv_strncpy(p, newVariant, vsize);  // Must use strncpy because
            p += vsize;                          // of trimming (above).
            *p = 0; // terminate
        }

        if ( ksize != 0)
        {
            if (uprv_strchr(newKeywords, '=')) {
                *p++ = '@'; /* keyword parsing */
            }
            else {
                *p++ = '_'; /* Variant parsing with a script */
                if ( vsize == 0) {
                    *p++ = '_'; /* No country found */
                }
            }
            uprv_strcpy(p, newKeywords);
            p += ksize;
        }

        // Parse it, because for example 'language' might really be a complete
        // string.
        init(togo.getAlias(), FALSE);
    }
}

Locale::Locale(const Locale &other)
    : UObject(other), fullName(fullNameBuffer), baseName(NULL)
{
    *this = other;
}

Locale &Locale::operator=(const Locale &other)
{
    if (this == &other) {
        return *this;
    }

    if (&other == NULL) {
        this->setToBogus();
        return *this;
    }

    /* Free our current storage */
    if(fullName != fullNameBuffer) {
        uprv_free(fullName);
        fullName = fullNameBuffer;
    }

    /* Allocate the full name if necessary */
    if(other.fullName != other.fullNameBuffer) {
        fullName = (char *)uprv_malloc(sizeof(char)*(uprv_strlen(other.fullName)+1));
        if (fullName == NULL) {
            return *this;
        }
    }
    /* Copy the full name */
    uprv_strcpy(fullName, other.fullName);

    /* baseName is the cached result of getBaseName.  if 'other' has a
       baseName and it fits in baseNameBuffer, then copy it. otherwise set
       it to NULL, and let the user lazy-create it (in getBaseName) if they
       want it. */
    if(baseName && baseName != baseNameBuffer) {
        uprv_free(baseName);
    }
    baseName = NULL;

    if(other.baseName == other.baseNameBuffer) {
        uprv_strcpy(baseNameBuffer, other.baseNameBuffer);
        baseName = baseNameBuffer;
    }

    /* Copy the language and country fields */
    uprv_strcpy(language, other.language);
    uprv_strcpy(script, other.script);
    uprv_strcpy(country, other.country);

    /* The variantBegin is an offset, just copy it */
    variantBegin = other.variantBegin;
    fIsBogus = other.fIsBogus;
    return *this;
}

Locale *
Locale::clone() const {
    return new Locale(*this);
}

UBool
Locale::operator==( const   Locale& other) const
{
    return (uprv_strcmp(other.fullName, fullName) == 0);
}

/*This function initializes a Locale from a C locale ID*/
Locale& Locale::init(const char* localeID, UBool canonicalize)
{
    fIsBogus = FALSE;
    /* Free our current storage */
    if(fullName != fullNameBuffer) {
        uprv_free(fullName);
        fullName = fullNameBuffer;
    }

    if(baseName && baseName != baseNameBuffer) {
        uprv_free(baseName);
        baseName = NULL;
    }

    // not a loop:
    // just an easy way to have a common error-exit
    // without goto and without another function
    do {
        char *separator;
        char *field[5] = {0};
        int32_t fieldLen[5] = {0};
        int32_t fieldIdx;
        int32_t variantField;
        int32_t length;
        UErrorCode err;

        if(localeID == NULL) {
            // not an error, just set the default locale
            return *this = getDefault();
        }

        /* preset all fields to empty */
        language[0] = script[0] = country[0] = 0;

        // "canonicalize" the locale ID to ICU/Java format
        err = U_ZERO_ERROR;
        length = canonicalize ?
            uloc_canonicalize(localeID, fullName, sizeof(fullNameBuffer), &err) :
            uloc_getName(localeID, fullName, sizeof(fullNameBuffer), &err);

        if(err == U_BUFFER_OVERFLOW_ERROR || length >= (int32_t)sizeof(fullNameBuffer)) {
            /*Go to heap for the fullName if necessary*/
            fullName = (char *)uprv_malloc(sizeof(char)*(length + 1));
            if(fullName == 0) {
                fullName = fullNameBuffer;
                break; // error: out of memory
            }
            err = U_ZERO_ERROR;
            length = canonicalize ?
                uloc_canonicalize(localeID, fullName, length+1, &err) :
                uloc_getName(localeID, fullName, length+1, &err);
        }
        if(U_FAILURE(err) || err == U_STRING_NOT_TERMINATED_WARNING) {
            /* should never occur */
            break;
        }

        variantBegin = length;

        /* after uloc_getName/canonicalize() we know that only '_' are separators */
        separator = field[0] = fullName;
        fieldIdx = 1;
        while ((separator = uprv_strchr(field[fieldIdx-1], SEP_CHAR)) && fieldIdx < (int32_t)(sizeof(field)/sizeof(field[0]))-1) {
            field[fieldIdx] = separator + 1;
            fieldLen[fieldIdx-1] = (int32_t)(separator - field[fieldIdx-1]);
            fieldIdx++;
        }
        // variant may contain @foo or .foo POSIX cruft; remove it
        separator = uprv_strchr(field[fieldIdx-1], '@');
        char* sep2 = uprv_strchr(field[fieldIdx-1], '.');
        if (separator!=NULL || sep2!=NULL) {
            if (separator==NULL || (sep2!=NULL && separator > sep2)) {
                separator = sep2;
            }
            fieldLen[fieldIdx-1] = (int32_t)(separator - field[fieldIdx-1]);
        } else {
            fieldLen[fieldIdx-1] = length - (int32_t)(field[fieldIdx-1] - fullName);
        }

        if (fieldLen[0] >= (int32_t)(sizeof(language))
            || (fieldLen[1] == 4 && fieldLen[2] >= (int32_t)(sizeof(country)))
            || (fieldLen[1] != 4 && fieldLen[1] >= (int32_t)(sizeof(country))))
        {
            break; // error: one of the fields is too long
        }

        variantField = 2; /* Usually the 2nd one, except when a script is used. */
        if (fieldLen[0] > 0) {
            /* We have a language */
            uprv_memcpy(language, fullName, fieldLen[0]);
            language[fieldLen[0]] = 0;
        }
        if (fieldLen[1] == 4) {
            /* We have at least a script */
            uprv_memcpy(script, field[1], fieldLen[1]);
            script[fieldLen[1]] = 0;
            variantField = 3;
            if (fieldLen[2] > 0) {
                /* We have a country */
                uprv_memcpy(country, field[2], fieldLen[2]);
                country[fieldLen[2]] = 0;
            }
        }
        else if (fieldLen[1] > 0) {
            /* We have a country and no script */
            uprv_memcpy(country, field[1], fieldLen[1]);
            country[fieldLen[1]] = 0;
        }
        if (variantField > 0 && fieldLen[variantField] > 0) {
            /* We have a variant */
            variantBegin = (int32_t)(field[variantField] - fullName);
        }

        // successful end of init()
        return *this;
    } while(0); /*loop doesn't iterate*/

    // when an error occurs, then set this object to "bogus" (there is no UErrorCode here)
    setToBogus();

    return *this;
}

int32_t
Locale::hashCode() const
{
    UHashTok hashKey;
    hashKey.pointer = fullName;
    return uhash_hashChars(hashKey);
}

void
Locale::setToBogus() {
    /* Free our current storage */
    if(fullName != fullNameBuffer) {
        uprv_free(fullName);
        fullName = fullNameBuffer;
    }
    if(baseName && baseName != baseNameBuffer) {
        uprv_free(baseName);
        baseName = NULL;
    }
    *fullNameBuffer = 0;
    *language = 0;
    *script = 0;
    *country = 0;
    fIsBogus = TRUE;
}

const Locale& U_EXPORT2
Locale::getDefault()
{
    const Locale *retLocale;
    UMTX_CHECK(NULL, gDefaultLocale, retLocale);
    if (retLocale == NULL) {
        locale_set_default_internal(NULL);
        umtx_lock(NULL);
        // Need a mutex  in case some other thread set a new
        // default inbetween when we set and when we get the new default.  For
        // processors with weak memory coherency, we might not otherwise see all
        // of the newly created new default locale.
        retLocale = gDefaultLocale;
        umtx_unlock(NULL);
    }
    return *retLocale;
}



void U_EXPORT2
Locale::setDefault( const   Locale&     newLocale,
                            UErrorCode&  status)
{
    if (U_FAILURE(status)) {
        return;
    }

    /* Set the default from the full name string of the supplied locale.
     * This is a convenient way to access the default locale caching mechanisms.
     */
    const char *localeID = newLocale.getName();
    locale_set_default_internal(localeID);
}

Locale U_EXPORT2
Locale::createFromName (const char *name)
{
    if (name) {
        Locale l("");
        l.init(name, FALSE);
        return l;
    }
    else {
        return getDefault();
    }
}

Locale U_EXPORT2
Locale::createCanonical(const char* name) {
    Locale loc("");
    loc.init(name, TRUE);
    return loc;
}

const char *
Locale::getISO3Language() const
{
    return uloc_getISO3Language(fullName);
}


const char *
Locale::getISO3Country() const
{
    return uloc_getISO3Country(fullName);
}

/**
 * Return the LCID value as specified in the "LocaleID" resource for this
 * locale.  The LocaleID must be expressed as a hexadecimal number, from
 * one to four digits.  If the LocaleID resource is not present, or is
 * in an incorrect format, 0 is returned.  The LocaleID is for use in
 * Windows (it is an LCID), but is available on all platforms.
 */
uint32_t
Locale::getLCID() const
{
    return uloc_getLCID(fullName);
}

const char* const* U_EXPORT2 Locale::getISOCountries()
{
    return uloc_getISOCountries();
}

const char* const* U_EXPORT2 Locale::getISOLanguages()
{
    return uloc_getISOLanguages();
}

// Set the locale's data based on a posix id.
void Locale::setFromPOSIXID(const char *posixID)
{
    init(posixID, TRUE);
}

const Locale & U_EXPORT2
Locale::getRoot(void)
{
    return getLocale(eROOT);
}

const Locale & U_EXPORT2
Locale::getEnglish(void)
{
    return getLocale(eENGLISH);
}

const Locale & U_EXPORT2
Locale::getFrench(void)
{
    return getLocale(eFRENCH);
}

const Locale & U_EXPORT2
Locale::getGerman(void)
{
    return getLocale(eGERMAN);
}

const Locale & U_EXPORT2
Locale::getItalian(void)
{
    return getLocale(eITALIAN);
}

const Locale & U_EXPORT2
Locale::getJapanese(void)
{
    return getLocale(eJAPANESE);
}

const Locale & U_EXPORT2
Locale::getKorean(void)
{
    return getLocale(eKOREAN);
}

const Locale & U_EXPORT2
Locale::getChinese(void)
{
    return getLocale(eCHINESE);
}

const Locale & U_EXPORT2
Locale::getSimplifiedChinese(void)
{
    return getLocale(eCHINA);
}

const Locale & U_EXPORT2
Locale::getTraditionalChinese(void)
{
    return getLocale(eTAIWAN);
}


const Locale & U_EXPORT2
Locale::getFrance(void)
{
    return getLocale(eFRANCE);
}

const Locale & U_EXPORT2
Locale::getGermany(void)
{
    return getLocale(eGERMANY);
}

const Locale & U_EXPORT2
Locale::getItaly(void)
{
    return getLocale(eITALY);
}

const Locale & U_EXPORT2
Locale::getJapan(void)
{
    return getLocale(eJAPAN);
}

const Locale & U_EXPORT2
Locale::getKorea(void)
{
    return getLocale(eKOREA);
}

const Locale & U_EXPORT2
Locale::getChina(void)
{
    return getLocale(eCHINA);
}

const Locale & U_EXPORT2
Locale::getPRC(void)
{
    return getLocale(eCHINA);
}

const Locale & U_EXPORT2
Locale::getTaiwan(void)
{
    return getLocale(eTAIWAN);
}

const Locale & U_EXPORT2
Locale::getUK(void)
{
    return getLocale(eUK);
}

const Locale & U_EXPORT2
Locale::getUS(void)
{
    return getLocale(eUS);
}

const Locale & U_EXPORT2
Locale::getCanada(void)
{
    return getLocale(eCANADA);
}

const Locale & U_EXPORT2
Locale::getCanadaFrench(void)
{
    return getLocale(eCANADA_FRENCH);
}

const Locale &
Locale::getLocale(int locid)
{
    Locale *localeCache = getLocaleCache();
    U_ASSERT((locid < eMAX_LOCALES)&&(locid>=0));
    if (localeCache == NULL) {
        // Failure allocating the locale cache.
        //   The best we can do is return a NULL reference.
        locid = 0;
    }
    return localeCache[locid]; /*operating on NULL*/
}

/*
This function is defined this way in order to get around static
initialization and static destruction.
 */
Locale *
Locale::getLocaleCache(void)
{
    umtx_lock(NULL);
    UBool needInit = (gLocaleCache == NULL);
    umtx_unlock(NULL);

    if (needInit) {
        Locale *tLocaleCache = new Locale[(int)eMAX_LOCALES];
        if (tLocaleCache == NULL) {
            return NULL;
        }
	tLocaleCache[eROOT]          = Locale("");
        tLocaleCache[eENGLISH]       = Locale("en");
        tLocaleCache[eFRENCH]        = Locale("fr");
        tLocaleCache[eGERMAN]        = Locale("de");
        tLocaleCache[eITALIAN]       = Locale("it");
        tLocaleCache[eJAPANESE]      = Locale("ja");
        tLocaleCache[eKOREAN]        = Locale("ko");
        tLocaleCache[eCHINESE]       = Locale("zh");
        tLocaleCache[eFRANCE]        = Locale("fr", "FR");
        tLocaleCache[eGERMANY]       = Locale("de", "DE");
        tLocaleCache[eITALY]         = Locale("it", "IT");
        tLocaleCache[eJAPAN]         = Locale("ja", "JP");
        tLocaleCache[eKOREA]         = Locale("ko", "KR");
        tLocaleCache[eCHINA]         = Locale("zh", "CN");
        tLocaleCache[eTAIWAN]        = Locale("zh", "TW");
        tLocaleCache[eUK]            = Locale("en", "GB");
        tLocaleCache[eUS]            = Locale("en", "US");
        tLocaleCache[eCANADA]        = Locale("en", "CA");
        tLocaleCache[eCANADA_FRENCH] = Locale("fr", "CA");

        umtx_lock(NULL);
        if (gLocaleCache == NULL) {
            gLocaleCache = tLocaleCache;
            tLocaleCache = NULL;
            ucln_common_registerCleanup(UCLN_COMMON_LOCALE, locale_cleanup);
        }
        umtx_unlock(NULL);
        if (tLocaleCache) {
            delete [] tLocaleCache;  // Fancy array delete will destruct each member.
        }
    }
    return gLocaleCache;
}

class KeywordEnumeration : public StringEnumeration {
private:
    char *keywords;
    char *current;
    int32_t length;
    UnicodeString currUSKey;
    static const char fgClassID;/* Warning this is used beyond the typical RTTI usage. */

public:
    static UClassID U_EXPORT2 getStaticClassID(void) { return (UClassID)&fgClassID; }
    virtual UClassID getDynamicClassID(void) const { return getStaticClassID(); }
public:
    KeywordEnumeration(const char *keys, int32_t keywordLen, int32_t currentIndex, UErrorCode &status)
        : keywords((char *)&fgClassID), current((char *)&fgClassID), length(0) {
        if(U_SUCCESS(status) && keywordLen != 0) {
            if(keys == NULL || keywordLen < 0) {
                status = U_ILLEGAL_ARGUMENT_ERROR;
            } else {
                keywords = (char *)uprv_malloc(keywordLen+1);
                if (keywords == NULL) {
                    status = U_MEMORY_ALLOCATION_ERROR;
                }
                else {
                    uprv_memcpy(keywords, keys, keywordLen);
                    keywords[keywordLen] = 0;
                    current = keywords + currentIndex;
                    length = keywordLen;
                }
            }
        }
    }

    virtual ~KeywordEnumeration() {
        uprv_free(keywords);
    }

    virtual StringEnumeration * clone() const
    {
        UErrorCode status = U_ZERO_ERROR;
        return new KeywordEnumeration(keywords, length, (int32_t)(current - keywords), status);
    }

    virtual int32_t count(UErrorCode &/*status*/) const {
        char *kw = keywords;
        int32_t result = 0;
        while(*kw) {
            result++;
            kw += uprv_strlen(kw)+1;
        }
        return result;
    }

    virtual const char* next(int32_t* resultLength, UErrorCode& status) {
        const char* result;
        int32_t len;
        if(U_SUCCESS(status) && *current != 0) {
            result = current;
            len = (int32_t)uprv_strlen(current);
            current += len+1;
            if(resultLength != NULL) {
                *resultLength = len;
            }
        } else {
            if(resultLength != NULL) {
                *resultLength = 0;
            }
            result = NULL;
        }
        return result;
    }

    virtual const UnicodeString* snext(UErrorCode& status) {
        int32_t resultLength = 0;
        const char *s = next(&resultLength, status);
        return setChars(s, resultLength, status);
    }

    virtual void reset(UErrorCode& /*status*/) {
        current = keywords;
    }
};

const char KeywordEnumeration::fgClassID = '\0';

StringEnumeration *
Locale::createKeywords(UErrorCode &status) const
{
    char keywords[256];
    int32_t keywordCapacity = 256;
    StringEnumeration *result = NULL;

    const char* variantStart = uprv_strchr(fullName, '@');
    const char* assignment = uprv_strchr(fullName, '=');
    if(variantStart) {
        if(assignment > variantStart) {
            int32_t keyLen = locale_getKeywords(variantStart+1, '@', keywords, keywordCapacity, NULL, 0, NULL, FALSE, &status);
            if(keyLen) {
                result = new KeywordEnumeration(keywords, keyLen, 0, status);
            }
        } else {
            status = U_INVALID_FORMAT_ERROR;
        }
    }
    return result;
}

int32_t
Locale::getKeywordValue(const char* keywordName, char *buffer, int32_t bufLen, UErrorCode &status) const
{
    return uloc_getKeywordValue(fullName, keywordName, buffer, bufLen, &status);
}

void
Locale::setKeywordValue(const char* keywordName, const char* keywordValue, UErrorCode &status)
{
    uloc_setKeywordValue(keywordName, keywordValue, fullName, ULOC_FULLNAME_CAPACITY, &status);
}

const char *
Locale::getBaseName() const
{
    // lazy init
    UErrorCode status = U_ZERO_ERROR;
    // semantically const
    if(baseName == 0) {
        ((Locale *)this)->baseName = ((Locale *)this)->baseNameBuffer;
        int32_t baseNameSize = uloc_getBaseName(fullName, baseName, ULOC_FULLNAME_CAPACITY, &status);
        if(baseNameSize >= ULOC_FULLNAME_CAPACITY) {
            ((Locale *)this)->baseName = (char *)uprv_malloc(sizeof(char) * baseNameSize + 1);
            if (baseName == NULL) {
                return baseName;
            }
            uloc_getBaseName(fullName, baseName, baseNameSize+1, &status);
        }
        baseName[baseNameSize] = 0;

        // the computation of variantBegin leaves it equal to the length
        // of fullName if there is no variant.  It should instead be
        // the length of the baseName.  Patch around this for now.
        if (variantBegin == (int32_t)uprv_strlen(fullName)) {
          ((Locale*)this)->variantBegin = baseNameSize;
        }
    }
    return baseName;
}

//eof
U_NAMESPACE_END
