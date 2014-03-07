/*
*******************************************************************************
* Copyright (C) 1997-2010, International Business Machines Corporation and    *
* others. All Rights Reserved.                                                *
*******************************************************************************
*
* File TIMEZONE.CPP
*
* Modification History:
*
*   Date        Name        Description
*   12/05/96    clhuang     Creation.
*   04/21/97    aliu        General clean-up and bug fixing.
*   05/08/97    aliu        Fixed Hashtable code per code review.
*   07/09/97    helena      Changed createInstance to createDefault.
*   07/29/97    aliu        Updated with all-new list of 96 UNIX-derived
*                           TimeZones.  Changed mechanism to load from static
*                           array rather than resource bundle.
*   07/07/1998  srl         Bugfixes from the Java side: UTC GMT CAT NST
*                           Added getDisplayName API
*                           going to add custom parsing.
*
*                           ISSUES:
*                               - should getDisplayName cache something?
*                               - should custom time zones be cached? [probably]
*  08/10/98     stephen     Brought getDisplayName() API in-line w/ conventions
*  08/19/98     stephen     Changed createTimeZone() to never return 0
*  09/02/98     stephen     Added getOffset(monthLen) and hasSameRules()
*  09/15/98     stephen     Added getStaticClassID()
*  02/22/99     stephen     Removed character literals for EBCDIC safety
*  05/04/99     stephen     Changed initDefault() for Mutex issues
*  07/12/99     helena      HPUX 11 CC Port.
*  12/03/99     aliu        Moved data out of static table into icudata.dll.
*                           Substantial rewrite of zone lookup, default zone, and
*                           available IDs code.  Misc. cleanup.
*********************************************************************************/

#include "unicode/utypeinfo.h"  // for 'typeid' to work

#include "unicode/utypes.h"
#include "unicode/ustring.h"

#ifdef U_DEBUG_TZ
# include <stdio.h>
# include "uresimp.h" // for debugging

static void debug_tz_loc(const char *f, int32_t l)
{
  fprintf(stderr, "%s:%d: ", f, l);
}

static void debug_tz_msg(const char *pat, ...)
{
  va_list ap;
  va_start(ap, pat);
  vfprintf(stderr, pat, ap);
  fflush(stderr);
}
static char gStrBuf[256];
#define U_DEBUG_TZ_STR(x) u_austrncpy(gStrBuf,x,sizeof(gStrBuf)-1)
// must use double parens, i.e.:  U_DEBUG_TZ_MSG(("four is: %d",4));
#define U_DEBUG_TZ_MSG(x) {debug_tz_loc(__FILE__,__LINE__);debug_tz_msg x;}
#else
#define U_DEBUG_TZ_MSG(x)
#endif

#if !UCONFIG_NO_FORMATTING

#include "unicode/simpletz.h"
#include "unicode/smpdtfmt.h"
#include "unicode/calendar.h"
#include "unicode/gregocal.h"
#include "unicode/ures.h"
#include "gregoimp.h"
#include "uresimp.h" // struct UResourceBundle
#include "olsontz.h"
#include "mutex.h"
#include "unicode/udata.h"
#include "ucln_in.h"
#include "cstring.h"
#include "cmemory.h"
#include "unicode/strenum.h"
#include "uassert.h"
#include "zonemeta.h"

#define kZONEINFO "zoneinfo64"
#define kREGIONS  "Regions"
#define kZONES    "Zones"
#define kRULES    "Rules"
#define kNAMES    "Names"
#define kTZVERSION  "TZVersion"
#define kLINKS    "links"
#define kMAX_CUSTOM_HOUR    23
#define kMAX_CUSTOM_MIN     59
#define kMAX_CUSTOM_SEC     59
#define MINUS 0x002D
#define PLUS 0x002B
#define ZERO_DIGIT 0x0030
#define COLON 0x003A

// Static data and constants

static const UChar         WORLD[] = {0x30, 0x30, 0x31, 0x00}; /* "001" */

static const UChar         GMT_ID[] = {0x47, 0x4D, 0x54, 0x00}; /* "GMT" */
static const UChar         Z_STR[] = {0x7A, 0x00}; /* "z" */
static const UChar         ZZZZ_STR[] = {0x7A, 0x7A, 0x7A, 0x7A, 0x00}; /* "zzzz" */
static const UChar         Z_UC_STR[] = {0x5A, 0x00}; /* "Z" */
static const UChar         ZZZZ_UC_STR[] = {0x5A, 0x5A, 0x5A, 0x5A, 0x00}; /* "ZZZZ" */
static const UChar         V_STR[] = {0x76, 0x00}; /* "v" */
static const UChar         VVVV_STR[] = {0x76, 0x76, 0x76, 0x76, 0x00}; /* "vvvv" */
static const UChar         V_UC_STR[] = {0x56, 0x00}; /* "V" */
static const UChar         VVVV_UC_STR[] = {0x56, 0x56, 0x56, 0x56, 0x00}; /* "VVVV" */
static const int32_t       GMT_ID_LENGTH = 3;

static UMTX                             LOCK;
static UMTX                             TZSET_LOCK;
static U_NAMESPACE_QUALIFIER TimeZone*  DEFAULT_ZONE = NULL;
static U_NAMESPACE_QUALIFIER TimeZone*  _GMT = NULL; // cf. TimeZone::GMT

static char TZDATA_VERSION[16];
static UBool TZDataVersionInitialized = FALSE;

U_CDECL_BEGIN
static UBool U_CALLCONV timeZone_cleanup(void)
{
    delete DEFAULT_ZONE;
    DEFAULT_ZONE = NULL;

    delete _GMT;
    _GMT = NULL;

    uprv_memset(TZDATA_VERSION, 0, sizeof(TZDATA_VERSION));
    TZDataVersionInitialized = FALSE;

    if (LOCK) {
        umtx_destroy(&LOCK);
        LOCK = NULL;
    }
    if (TZSET_LOCK) {
        umtx_destroy(&TZSET_LOCK);
        TZSET_LOCK = NULL;
    }

    return TRUE;
}
U_CDECL_END

U_NAMESPACE_BEGIN

/**
 * The Olson data is stored the "zoneinfo" resource bundle.
 * Sub-resources are organized into three ranges of data: Zones, final
 * rules, and country tables.  There is also a meta-data resource
 * which has 3 integers: The number of zones, rules, and countries,
 * respectively.  The country count includes the non-country 'Default'.
 */
static int32_t OLSON_ZONE_COUNT = 0;  // count of zones

/**
 * Given a pointer to an open "zoneinfo" resource, load up the Olson
 * meta-data. Return TRUE if successful.
 */
static UBool getOlsonMeta(const UResourceBundle* top) {
    if (OLSON_ZONE_COUNT == 0) {
        UErrorCode ec = U_ZERO_ERROR;
        UResourceBundle res;
        ures_initStackObject(&res);
        ures_getByKey(top, kZONES, &res, &ec);
        if(U_SUCCESS(ec)) {
            OLSON_ZONE_COUNT = ures_getSize(&res);
            U_DEBUG_TZ_MSG(("OZC%d\n",OLSON_ZONE_COUNT));
        }
        ures_close(&res);
    }
    return (OLSON_ZONE_COUNT > 0);
}

/**
 * Load up the Olson meta-data. Return TRUE if successful.
 */
static UBool getOlsonMeta() {
    if (OLSON_ZONE_COUNT == 0) {
        UErrorCode ec = U_ZERO_ERROR;
        UResourceBundle *top = ures_openDirect(0, kZONEINFO, &ec);
        if (U_SUCCESS(ec)) {
            getOlsonMeta(top);
        }
        ures_close(top);
    }
    return (OLSON_ZONE_COUNT > 0);
}

static int32_t findInStringArray(UResourceBundle* array, const UnicodeString& id, UErrorCode &status)
{
    UnicodeString copy;
    const UChar *u;
    int32_t len;
    
    int32_t start = 0;
    int32_t limit = ures_getSize(array);
    int32_t mid;
    int32_t lastMid = INT32_MAX;
    if(U_FAILURE(status) || (limit < 1)) { 
        return -1;
    }
    U_DEBUG_TZ_MSG(("fisa: Looking for %s, between %d and %d\n", U_DEBUG_TZ_STR(UnicodeString(id).getTerminatedBuffer()), start, limit));
    
    for (;;) {
        mid = (int32_t)((start + limit) / 2);
        if (lastMid == mid) {   /* Have we moved? */
            break;  /* We haven't moved, and it wasn't found. */
        }
        lastMid = mid;
        u = ures_getStringByIndex(array, mid, &len, &status);
        if (U_FAILURE(status)) {
            break;
        }
        U_DEBUG_TZ_MSG(("tz: compare to %s, %d .. [%d] .. %d\n", U_DEBUG_TZ_STR(u), start, mid, limit));
        copy.setTo(TRUE, u, len);
        int r = id.compare(copy);
        if(r==0) {
            U_DEBUG_TZ_MSG(("fisa: found at %d\n", mid));
            return mid;
        } else if(r<0) {
            limit = mid;
        } else {
            start = mid;
        }
    }
    U_DEBUG_TZ_MSG(("fisa: not found\n"));
    return -1;
}

/**
 * Fetch a specific zone by name.  Replaces the getByKey call. 
 * @param top Top timezone resource
 * @param id Time zone ID
 * @param oldbundle Bundle for reuse (or NULL).   see 'ures_open()'
 * @return the zone's bundle if found, or undefined if error.  Reuses oldbundle.
 */
static UResourceBundle* getZoneByName(const UResourceBundle* top, const UnicodeString& id, UResourceBundle *oldbundle, UErrorCode& status) {
    // load the Rules object
    UResourceBundle *tmp = ures_getByKey(top, kNAMES, NULL, &status);
    
    // search for the string
    int32_t idx = findInStringArray(tmp, id, status);
    
    if((idx == -1) && U_SUCCESS(status)) {
        // not found 
        status = U_MISSING_RESOURCE_ERROR;
        //ures_close(oldbundle);
        //oldbundle = NULL;
    } else {
        U_DEBUG_TZ_MSG(("gzbn: oldbundle= size %d, type %d, %s\n", ures_getSize(tmp), ures_getType(tmp), u_errorName(status)));
        tmp = ures_getByKey(top, kZONES, tmp, &status); // get Zones object from top
        U_DEBUG_TZ_MSG(("gzbn: loaded ZONES, size %d, type %d, path %s %s\n", ures_getSize(tmp), ures_getType(tmp), ures_getPath(tmp), u_errorName(status)));
        oldbundle = ures_getByIndex(tmp, idx, oldbundle, &status); // get nth Zone object
        U_DEBUG_TZ_MSG(("gzbn: loaded z#%d, size %d, type %d, path %s, %s\n", idx, ures_getSize(oldbundle), ures_getType(oldbundle), ures_getPath(oldbundle),  u_errorName(status)));
    }
    ures_close(tmp);
    if(U_FAILURE(status)) { 
        //ures_close(oldbundle);
        return NULL;
    } else {
        return oldbundle;
    }
}


UResourceBundle* TimeZone::loadRule(const UResourceBundle* top, const UnicodeString& ruleid, UResourceBundle* oldbundle, UErrorCode& status) {
    char key[64];
    ruleid.extract(0, sizeof(key)-1, key, (int32_t)sizeof(key)-1, US_INV);
    U_DEBUG_TZ_MSG(("loadRule(%s)\n", key));
    UResourceBundle *r = ures_getByKey(top, kRULES, oldbundle, &status);
    U_DEBUG_TZ_MSG(("loadRule(%s) -> kRULES [%s]\n", key, u_errorName(status)));
    r = ures_getByKey(r, key, r, &status);
    U_DEBUG_TZ_MSG(("loadRule(%s) -> item [%s]\n", key, u_errorName(status)));
    return r;
}

/**
 * Given an ID, open the appropriate resource for the given time zone.
 * Dereference aliases if necessary.
 * @param id zone id
 * @param res resource, which must be ready for use (initialized but not open)
 * @param ec input-output error code
 * @return top-level resource bundle
 */
static UResourceBundle* openOlsonResource(const UnicodeString& id,
                                          UResourceBundle& res,
                                          UErrorCode& ec)
{
#if U_DEBUG_TZ
    char buf[128];
    id.extract(0, sizeof(buf)-1, buf, sizeof(buf), "");
#endif
    UResourceBundle *top = ures_openDirect(0, kZONEINFO, &ec);
    U_DEBUG_TZ_MSG(("pre: res sz=%d\n", ures_getSize(&res)));
    /* &res = */ getZoneByName(top, id, &res, ec);
    // Dereference if this is an alias.  Docs say result should be 1
    // but it is 0 in 2.8 (?).
    U_DEBUG_TZ_MSG(("Loading zone '%s' (%s, size %d) - %s\n", buf, ures_getKey((UResourceBundle*)&res), ures_getSize(&res), u_errorName(ec)));
    if (ures_getType(&res) == URES_INT && getOlsonMeta(top)) {
        int32_t deref = ures_getInt(&res, &ec) + 0;
        U_DEBUG_TZ_MSG(("getInt: %s - type is %d\n", u_errorName(ec), ures_getType(&res)));
        UResourceBundle *ares = ures_getByKey(top, kZONES, NULL, &ec); // dereference Zones section
        ures_getByIndex(ares, deref, &res, &ec);
        ures_close(ares);
        U_DEBUG_TZ_MSG(("alias to #%d (%s) - %s\n", deref, "??", u_errorName(ec)));
    } else {
        U_DEBUG_TZ_MSG(("not an alias - size %d\n", ures_getSize(&res)));
    }
    U_DEBUG_TZ_MSG(("%s - final status is %s\n", buf, u_errorName(ec)));
    return top;
}

// -------------------------------------

const TimeZone* U_EXPORT2
TimeZone::getGMT(void)
{
    UBool needsInit;
    UMTX_CHECK(&LOCK, (_GMT == NULL), needsInit);   /* This is here to prevent race conditions. */

    // Initialize _GMT independently of other static data; it should
    // be valid even if we can't load the time zone UDataMemory.
    if (needsInit) {
        SimpleTimeZone *tmpGMT = new SimpleTimeZone(0, UnicodeString(TRUE, GMT_ID, GMT_ID_LENGTH));
        umtx_lock(&LOCK);
        if (_GMT == 0) {
            _GMT = tmpGMT;
            tmpGMT = NULL;
        }
        umtx_unlock(&LOCK);
        ucln_i18n_registerCleanup(UCLN_I18N_TIMEZONE, timeZone_cleanup);
        delete tmpGMT;
    }
    return _GMT;
}

// *****************************************************************************
// class TimeZone
// *****************************************************************************

UOBJECT_DEFINE_ABSTRACT_RTTI_IMPLEMENTATION(TimeZone)

TimeZone::TimeZone()
    :   UObject(), fID()
{
}

// -------------------------------------

TimeZone::TimeZone(const UnicodeString &id)
    :   UObject(), fID(id)
{
}

// -------------------------------------

TimeZone::~TimeZone()
{
}

// -------------------------------------

TimeZone::TimeZone(const TimeZone &source)
    :   UObject(source), fID(source.fID)
{
}

// -------------------------------------

TimeZone &
TimeZone::operator=(const TimeZone &right)
{
    if (this != &right) fID = right.fID;
    return *this;
}

// -------------------------------------

UBool
TimeZone::operator==(const TimeZone& that) const
{
    return typeid(*this) == typeid(that) &&
        fID == that.fID;
}

// -------------------------------------

TimeZone* U_EXPORT2
TimeZone::createTimeZone(const UnicodeString& ID)
{
    /* We first try to lookup the zone ID in our system list.  If this
     * fails, we try to parse it as a custom string GMT[+-]hh:mm.  If
     * all else fails, we return GMT, which is probably not what the
     * user wants, but at least is a functioning TimeZone object.
     *
     * We cannot return NULL, because that would break compatibility
     * with the JDK.
     */
    TimeZone* result = createSystemTimeZone(ID);

    if (result == 0) {
        U_DEBUG_TZ_MSG(("failed to load system time zone with id - falling to custom"));
        result = createCustomTimeZone(ID);
    }
    if (result == 0) {
        U_DEBUG_TZ_MSG(("failed to load time zone with id - falling to GMT"));
        const TimeZone* temptz = getGMT();
        if (temptz == NULL) {
            result = NULL;
        } else {
            result = temptz->clone();
        }
    }
    return result;
}

/**
 * Lookup the given name in our system zone table.  If found,
 * instantiate a new zone of that name and return it.  If not
 * found, return 0.
 */
TimeZone*
TimeZone::createSystemTimeZone(const UnicodeString& id) {
    TimeZone* z = 0;
    UErrorCode ec = U_ZERO_ERROR;
    UResourceBundle res;
    ures_initStackObject(&res);
    U_DEBUG_TZ_MSG(("pre-err=%s\n", u_errorName(ec)));
    UResourceBundle *top = openOlsonResource(id, res, ec);
    U_DEBUG_TZ_MSG(("post-err=%s\n", u_errorName(ec)));
    if (U_SUCCESS(ec)) {
        z = new OlsonTimeZone(top, &res, ec);
        if (z) {
          z->setID(id);
        } else {
          U_DEBUG_TZ_MSG(("cstz: olson time zone failed to initialize - err %s\n", u_errorName(ec)));
        }
    }
    ures_close(&res);
    ures_close(top);
    if (U_FAILURE(ec)) {
        U_DEBUG_TZ_MSG(("cstz: failed to create, err %s\n", u_errorName(ec)));
        delete z;
        z = 0;
    }
    return z;
}

// -------------------------------------

/**
 * Initialize DEFAULT_ZONE from the system default time zone.  The
 * caller should confirm that DEFAULT_ZONE is NULL before calling.
 * Upon return, DEFAULT_ZONE will not be NULL, unless operator new()
 * returns NULL.
 *
 * Must be called OUTSIDE mutex.
 */
void
TimeZone::initDefault()
{ 
    // We access system timezone data through TPlatformUtilities,
    // including tzset(), timezone, and tzname[].
    int32_t rawOffset = 0;
    const char *hostID;

    // First, try to create a system timezone, based
    // on the string ID in tzname[0].
    {
        // NOTE: Local mutex here. TimeZone mutex below
        // mutexed to avoid threading issues in the platform functions.
        // Some of the locale/timezone OS functions may not be thread safe,
        // so the intent is that any setting from anywhere within ICU
        // happens while the ICU mutex is held.
        // The operating system might actually use ICU to implement timezones.
        // So we may have ICU calling ICU here, like on AIX.
        // In order to prevent a double lock of a non-reentrant mutex in a
        // different part of ICU, we use TZSET_LOCK to allow only one instance
        // of ICU to query these thread unsafe OS functions at any given time.
        Mutex lock(&TZSET_LOCK);

        ucln_i18n_registerCleanup(UCLN_I18N_TIMEZONE, timeZone_cleanup);
        uprv_tzset(); // Initialize tz... system data
        
        // Get the timezone ID from the host.  This function should do
        // any required host-specific remapping; e.g., on Windows this
        // function maps the Date and Time control panel setting to an
        // ICU timezone ID.
        hostID = uprv_tzname(0);
        
        // Invert sign because UNIX semantics are backwards
        rawOffset = uprv_timezone() * -U_MILLIS_PER_SECOND;
    }

    UBool initialized;
    UMTX_CHECK(&LOCK, (DEFAULT_ZONE != NULL), initialized);
    if (initialized) {
        /* Hrmph? Either a race condition happened, or tzset initialized ICU. */
        return;
    }

    TimeZone* default_zone = NULL;

    /* Make sure that the string is NULL terminated to prevent BoundsChecker/Purify warnings. */
    UnicodeString hostStrID(hostID, -1, US_INV);
    hostStrID.append((UChar)0);
    hostStrID.truncate(hostStrID.length()-1);
    default_zone = createSystemTimeZone(hostStrID);

#ifdef U_WINDOWS
    // hostID points to a heap-allocated location on Windows.
    uprv_free(const_cast<char *>(hostID));
#endif

    int32_t hostIDLen = hostStrID.length();
    if (default_zone != NULL && rawOffset != default_zone->getRawOffset()
        && (3 <= hostIDLen && hostIDLen <= 4))
    {
        // Uh oh. This probably wasn't a good id.
        // It was probably an ambiguous abbreviation
        delete default_zone;
        default_zone = NULL;
    }

    // Construct a fixed standard zone with the host's ID
    // and raw offset.
    if (default_zone == NULL) {
        default_zone = new SimpleTimeZone(rawOffset, hostStrID);
    }

    // If we _still_ don't have a time zone, use GMT.
    if (default_zone == NULL) {
        const TimeZone* temptz = getGMT();
        // If we can't use GMT, get out.
        if (temptz == NULL) {
            return;
        }
        default_zone = temptz->clone();
    }

    // If DEFAULT_ZONE is still NULL, set it up.
    umtx_lock(&LOCK);
    if (DEFAULT_ZONE == NULL) {
        DEFAULT_ZONE = default_zone;
        default_zone = NULL;
        ucln_i18n_registerCleanup(UCLN_I18N_TIMEZONE, timeZone_cleanup);
    }
    umtx_unlock(&LOCK);
    
    delete default_zone;
}

// -------------------------------------

TimeZone* U_EXPORT2
TimeZone::createDefault()
{
    /* This is here to prevent race conditions. */
    UBool needsInit;
    UMTX_CHECK(&LOCK, (DEFAULT_ZONE == NULL), needsInit);
    if (needsInit) {
        initDefault();
    }

    Mutex lock(&LOCK); // In case adoptDefault is called
    return (DEFAULT_ZONE != NULL) ? DEFAULT_ZONE->clone() : NULL;
}

// -------------------------------------

void U_EXPORT2
TimeZone::adoptDefault(TimeZone* zone)
{
    if (zone != NULL)
    {
        TimeZone* old = NULL;

        umtx_lock(&LOCK);
        old = DEFAULT_ZONE;
        DEFAULT_ZONE = zone;
        umtx_unlock(&LOCK);

        delete old;
        ucln_i18n_registerCleanup(UCLN_I18N_TIMEZONE, timeZone_cleanup);
    }
}
// -------------------------------------

void U_EXPORT2
TimeZone::setDefault(const TimeZone& zone)
{
    adoptDefault(zone.clone());
}

//----------------------------------------------------------------------

/**
 * This is the default implementation for subclasses that do not
 * override this method.  This implementation calls through to the
 * 8-argument getOffset() method after suitable computations, and
 * correctly adjusts GMT millis to local millis when necessary.
 */
void TimeZone::getOffset(UDate date, UBool local, int32_t& rawOffset,
                         int32_t& dstOffset, UErrorCode& ec) const {
    if (U_FAILURE(ec)) {
        return;
    }

    rawOffset = getRawOffset();
    if (!local) {
        date += rawOffset; // now in local standard millis
    }

    // When local == TRUE, date might not be in local standard
    // millis.  getOffset taking 7 parameters used here assume
    // the given time in day is local standard time.
    // At STD->DST transition, there is a range of time which
    // does not exist.  When 'date' is in this time range
    // (and local == TRUE), this method interprets the specified
    // local time as DST.  At DST->STD transition, there is a
    // range of time which occurs twice.  In this case, this
    // method interprets the specified local time as STD.
    // To support the behavior above, we need to call getOffset
    // (with 7 args) twice when local == true and DST is
    // detected in the initial call.
    for (int32_t pass=0; ; ++pass) {
        int32_t year, month, dom, dow;
        double day = uprv_floor(date / U_MILLIS_PER_DAY);
        int32_t millis = (int32_t) (date - day * U_MILLIS_PER_DAY);

        Grego::dayToFields(day, year, month, dom, dow);

        dstOffset = getOffset(GregorianCalendar::AD, year, month, dom,
                              (uint8_t) dow, millis,
                              Grego::monthLength(year, month),
                              ec) - rawOffset;

        // Recompute if local==TRUE, dstOffset!=0.
        if (pass!=0 || !local || dstOffset == 0) {
            break;
        }
        // adjust to local standard millis
        date -= dstOffset;
    }
}

// -------------------------------------

// New available IDs API as of ICU 2.4.  Uses StringEnumeration API.

class TZEnumeration : public StringEnumeration {
private:

    // Map into to zones.  Our results are zone[map[i]] for
    // i=0..len-1, where zone[i] is the i-th Olson zone.  If map==NULL
    // then our results are zone[i] for i=0..len-1.  Len will be zero
    // iff the zone data could not be loaded.
    int32_t* map;
    int32_t  len;
    int32_t  pos;

    UBool getID(int32_t i) {
        UErrorCode ec = U_ZERO_ERROR;
        int32_t idLen = 0;
        const UChar* id = NULL;
        UResourceBundle *top = ures_openDirect(0, kZONEINFO, &ec);
        top = ures_getByKey(top, kNAMES, top, &ec); // dereference Zones section
        id = ures_getStringByIndex(top, i, &idLen, &ec);
        if(U_FAILURE(ec)) {
            unistr.truncate(0);
        }
        else {
            unistr.fastCopyFrom(UnicodeString(TRUE, id, idLen));
        }
        ures_close(top);
        return U_SUCCESS(ec);
    }

public:
    TZEnumeration() : map(NULL), len(0), pos(0) {
        if (getOlsonMeta()) {
            len = OLSON_ZONE_COUNT;
        }
    }

    TZEnumeration(int32_t rawOffset) : map(NULL), len(0), pos(0) {
        if (!getOlsonMeta()) {
            return;
        }

        // Allocate more space than we'll need.  The end of the array will
        // be blank.
        map = (int32_t*)uprv_malloc(OLSON_ZONE_COUNT * sizeof(int32_t));
        if (map == 0) {
            return;
        }

        uprv_memset(map, 0, sizeof(int32_t) * OLSON_ZONE_COUNT);

        UnicodeString s;
        for (int32_t i=0; i<OLSON_ZONE_COUNT; ++i) {
            if (getID(i)) {
                // This is VERY inefficient.
                TimeZone* z = TimeZone::createTimeZone(unistr);
                // Make sure we get back the ID we wanted (if the ID is
                // invalid we get back GMT).
                if (z != 0 && z->getID(s) == unistr &&
                    z->getRawOffset() == rawOffset) {
                    map[len++] = i;
                }
                delete z;
            }
        }
    }

    TZEnumeration(const char* country) : map(NULL), len(0), pos(0) {
        if (!getOlsonMeta()) {
            return;
        }

        UErrorCode ec = U_ZERO_ERROR;
        UResourceBundle *res = ures_openDirect(0, kZONEINFO, &ec);
        ures_getByKey(res, kREGIONS, res, &ec);
        if (U_SUCCESS(ec) && ures_getType(res) == URES_ARRAY) {
            UChar uCountry[] = {0, 0, 0, 0};
            if (country) {
                u_charsToUChars(country, uCountry, 2);
            } else {
                u_strcpy(uCountry, WORLD);
            }

            // count matches
            int32_t count = 0;
            int32_t i;
            const UChar *region;
            for (i = 0; i < ures_getSize(res); i++) {
                region = ures_getStringByIndex(res, i, NULL, &ec);
                if (U_FAILURE(ec)) {
                    break;
                }
                if (u_strcmp(uCountry, region) == 0) {
                    count++;
                }
            }

            if (count > 0) {
                map = (int32_t*)uprv_malloc(sizeof(int32_t) * count);
                if (map != NULL) {
                    int32_t idx = 0;
                    for (i = 0; i < ures_getSize(res); i++) {
                        region = ures_getStringByIndex(res, i, NULL, &ec);
                        if (U_FAILURE(ec)) {
                            break;
                        }
                        if (u_strcmp(uCountry, region) == 0) {
                            map[idx++] = i;
                        }
                    }
                    if (U_SUCCESS(ec)) {
                        len = count;
                    } else {
                        uprv_free(map);
                        map = NULL;
                    }
                } else {
                    U_DEBUG_TZ_MSG(("Failed to load tz for region %s: %s\n", country, u_errorName(ec)));
                }
            }
        }
        ures_close(res);
    }

  TZEnumeration(const TZEnumeration &other) : StringEnumeration(), map(NULL), len(0), pos(0) {
        if(other.len > 0) {
            if(other.map != NULL) {
                map = (int32_t *)uprv_malloc(other.len * sizeof(int32_t));
                if(map != NULL) {
                    len = other.len;
                    uprv_memcpy(map, other.map, len * sizeof(int32_t));
                    pos = other.pos;
                }
            } else {
                len = other.len;
                pos = other.pos;
            }
        }
    }

    virtual ~TZEnumeration() {
        uprv_free(map);
    }

    virtual StringEnumeration *clone() const {
        return new TZEnumeration(*this);
    }

    virtual int32_t count(UErrorCode& status) const {
        return U_FAILURE(status) ? 0 : len;
    }

    virtual const UnicodeString* snext(UErrorCode& status) {
        if (U_SUCCESS(status) && pos < len) {
            getID((map == 0) ? pos : map[pos]);
            ++pos;
            return &unistr;
        }
        return 0;
    }

    virtual void reset(UErrorCode& /*status*/) {
        pos = 0;
    }

public:
    static UClassID U_EXPORT2 getStaticClassID(void);
    virtual UClassID getDynamicClassID(void) const;
};

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(TZEnumeration)

StringEnumeration* U_EXPORT2
TimeZone::createEnumeration() {
    return new TZEnumeration();
}

StringEnumeration* U_EXPORT2
TimeZone::createEnumeration(int32_t rawOffset) {
    return new TZEnumeration(rawOffset);
}

StringEnumeration* U_EXPORT2
TimeZone::createEnumeration(const char* country) {
    return new TZEnumeration(country);
}

// ---------------------------------------

int32_t U_EXPORT2
TimeZone::countEquivalentIDs(const UnicodeString& id) {
    int32_t result = 0;
    UErrorCode ec = U_ZERO_ERROR;
    UResourceBundle res;
    ures_initStackObject(&res);
    U_DEBUG_TZ_MSG(("countEquivalentIDs..\n"));
    UResourceBundle *top = openOlsonResource(id, res, ec);
    if (U_SUCCESS(ec)) {
        UResourceBundle r;
        ures_initStackObject(&r);
        ures_getByKey(&res, kLINKS, &r, &ec);
        ures_getIntVector(&r, &result, &ec);
        ures_close(&r);
    }
    ures_close(&res);
    ures_close(top);
    return result;
}

// ---------------------------------------

const UnicodeString U_EXPORT2
TimeZone::getEquivalentID(const UnicodeString& id, int32_t index) {
    U_DEBUG_TZ_MSG(("gEI(%d)\n", index));
    UnicodeString result;
    UErrorCode ec = U_ZERO_ERROR;
    UResourceBundle res;
    ures_initStackObject(&res);
    UResourceBundle *top = openOlsonResource(id, res, ec);
    int32_t zone = -1;
    if (U_SUCCESS(ec)) {
        UResourceBundle r;
        ures_initStackObject(&r);
        int32_t size;
        ures_getByKey(&res, kLINKS, &r, &ec);
        const int32_t* v = ures_getIntVector(&r, &size, &ec);
        if (U_SUCCESS(ec)) {
            if (index >= 0 && index < size && getOlsonMeta()) {
                zone = v[index];
            }
        }
        ures_close(&r);
    }
    ures_close(&res);
    if (zone >= 0) {
        UResourceBundle *ares = ures_getByKey(top, kNAMES, NULL, &ec); // dereference Zones section
        if (U_SUCCESS(ec)) {
            int32_t idLen = 0;
            const UChar* id = ures_getStringByIndex(ares, zone, &idLen, &ec);
            result.fastCopyFrom(UnicodeString(TRUE, id, idLen));
            U_DEBUG_TZ_MSG(("gei(%d) -> %d, len%d, %s\n", index, zone, result.length(), u_errorName(ec)));
        }
        ures_close(ares);
    }
    ures_close(top);
#if defined(U_DEBUG_TZ)
    if(result.length() ==0) {
      U_DEBUG_TZ_MSG(("equiv [__, #%d] -> 0 (%s)\n", index, u_errorName(ec)));
    }
#endif
    return result;
}

// ---------------------------------------

// These two methods are used by ZoneMeta class only.

const UChar*
TimeZone::dereferOlsonLink(const UnicodeString& id) {
    const UChar *result = NULL;
    UErrorCode ec = U_ZERO_ERROR;
    UResourceBundle *rb = ures_openDirect(NULL, kZONEINFO, &ec);

    // resolve zone index by name
    UResourceBundle *names = ures_getByKey(rb, kNAMES, NULL, &ec);
    int32_t idx = findInStringArray(names, id, ec);
    result = ures_getStringByIndex(names, idx, NULL, &ec);

    // open the zone bundle by index
    ures_getByKey(rb, kZONES, rb, &ec);
    ures_getByIndex(rb, idx, rb, &ec); 

    if (U_SUCCESS(ec)) {
        if (ures_getType(rb) == URES_INT) {
            // this is a link - dereference the link
            int32_t deref = ures_getInt(rb, &ec);
            const UChar* tmp = ures_getStringByIndex(names, deref, NULL, &ec);
            if (U_SUCCESS(ec)) {
                result = tmp;
            }
        }
    }

    ures_close(names);
    ures_close(rb);

    return result;
}

const UChar*
TimeZone::getRegion(const UnicodeString& id) {
    const UChar *result = WORLD;
    UErrorCode ec = U_ZERO_ERROR;
    UResourceBundle *rb = ures_openDirect(NULL, kZONEINFO, &ec);

    // resolve zone index by name
    UResourceBundle *res = ures_getByKey(rb, kNAMES, NULL, &ec);
    int32_t idx = findInStringArray(res, id, ec);

    // get region mapping
    ures_getByKey(rb, kREGIONS, res, &ec);
    const UChar *tmp = ures_getStringByIndex(res, idx, NULL, &ec);
    if (U_SUCCESS(ec)) {
        result = tmp;
    }

    ures_close(res);
    ures_close(rb);

    return result;
}

// ---------------------------------------


UnicodeString&
TimeZone::getDisplayName(UnicodeString& result) const
{
    return getDisplayName(FALSE,LONG,Locale::getDefault(), result);
}

UnicodeString&
TimeZone::getDisplayName(const Locale& locale, UnicodeString& result) const
{
    return getDisplayName(FALSE, LONG, locale, result);
}

UnicodeString&
TimeZone::getDisplayName(UBool daylight, EDisplayType style, UnicodeString& result)  const
{
    return getDisplayName(daylight,style, Locale::getDefault(), result);
}
//--------------------------------------
int32_t 
TimeZone::getDSTSavings()const {
    if (useDaylightTime()) {
        return 3600000;
    }
    return 0;
}
//---------------------------------------
UnicodeString&
TimeZone::getDisplayName(UBool daylight, EDisplayType style, const Locale& locale, UnicodeString& result) const
{
    // SRL TODO: cache the SDF, just like java.
    UErrorCode status = U_ZERO_ERROR;
#ifdef U_DEBUG_TZ
    char buf[128];
    fID.extract(0, sizeof(buf)-1, buf, sizeof(buf), "");
#endif

    // select the proper format string
    UnicodeString pat;
    switch(style){
    case LONG:
        pat = ZZZZ_STR;
        break;
    case SHORT_GENERIC:
        pat = V_STR;
        break;
    case LONG_GENERIC:
        pat = VVVV_STR;
        break;
    case SHORT_GMT:
        pat = Z_UC_STR;
        break;
    case LONG_GMT:
        pat = ZZZZ_UC_STR;
        break;
    case SHORT_COMMONLY_USED:
        //pat = V_UC_STR;
        pat = Z_STR;
        break;
    case GENERIC_LOCATION:
        pat = VVVV_UC_STR;
        break;
    default: // SHORT
        //pat = Z_STR;
        pat = V_UC_STR;
        break;
    }

    SimpleDateFormat format(pat, locale, status);
    U_DEBUG_TZ_MSG(("getDisplayName(%s)\n", buf));
    if(!U_SUCCESS(status))
    {
#ifdef U_DEBUG_TZ
      char buf2[128];
      result.extract(0, sizeof(buf2)-1, buf2, sizeof(buf2), "");
      U_DEBUG_TZ_MSG(("getDisplayName(%s) -> %s\n", buf, buf2));
#endif
      return result.remove();
    }

    UDate d = Calendar::getNow();
    int32_t  rawOffset;
    int32_t  dstOffset;
    this->getOffset(d, FALSE, rawOffset, dstOffset, status);
    if (U_FAILURE(status)) {
        return result.remove();
    }
    
    if ((daylight && dstOffset != 0) || 
        (!daylight && dstOffset == 0) ||
        (style == SHORT_GENERIC) ||
        (style == LONG_GENERIC)
       ) {
        // Current time and the request (daylight / not daylight) agree.
        format.setTimeZone(*this);
        return format.format(d, result);
    }
    
    // Create a new SimpleTimeZone as a stand-in for this zone; the
    // stand-in will have no DST, or DST during July, but the same ID and offset,
    // and hence the same display name.
    // We don't cache these because they're small and cheap to create.
    UnicodeString tempID;
    getID(tempID);
    SimpleTimeZone *tz = NULL;
    if(daylight && useDaylightTime()){
        // The display name for daylight saving time was requested, but currently not in DST
        // Set a fixed date (July 1) in this Gregorian year
        GregorianCalendar cal(*this, status);
        if (U_FAILURE(status)) {
            return result.remove();
        }
        cal.set(UCAL_MONTH, UCAL_JULY);
        cal.set(UCAL_DATE, 1);
        
        // Get July 1 date
        d = cal.getTime(status);
        
        // Check if it is in DST
        if (cal.get(UCAL_DST_OFFSET, status) == 0) {
            // We need to create a fake time zone
            tz = new SimpleTimeZone(rawOffset, tempID,
                                UCAL_JUNE, 1, 0, 0,
                                UCAL_AUGUST, 1, 0, 0,
                                getDSTSavings(), status);
            if (U_FAILURE(status) || tz == NULL) {
                if (U_SUCCESS(status)) {
                    status = U_MEMORY_ALLOCATION_ERROR;
                }
                return result.remove();
            }
            format.adoptTimeZone(tz);
        } else {
            format.setTimeZone(*this);
        }
    } else {
        // The display name for standard time was requested, but currently in DST
        // or display name for daylight saving time was requested, but this zone no longer
        // observes DST.
        tz = new SimpleTimeZone(rawOffset, tempID);
        if (U_FAILURE(status) || tz == NULL) {
            if (U_SUCCESS(status)) {
                status = U_MEMORY_ALLOCATION_ERROR;
            }
            return result.remove();
        }
        format.adoptTimeZone(tz);
    }

    format.format(d, result, status);
    return  result;
}

/**
 * Parse a custom time zone identifier and return a corresponding zone.
 * @param id a string of the form GMT[+-]hh:mm, GMT[+-]hhmm, or
 * GMT[+-]hh.
 * @return a newly created SimpleTimeZone with the given offset and
 * no Daylight Savings Time, or null if the id cannot be parsed.
*/
TimeZone*
TimeZone::createCustomTimeZone(const UnicodeString& id)
{
    int32_t sign, hour, min, sec;
    if (parseCustomID(id, sign, hour, min, sec)) {
        UnicodeString customID;
        formatCustomID(hour, min, sec, (sign < 0), customID);
        int32_t offset = sign * ((hour * 60 + min) * 60 + sec) * 1000;
        return new SimpleTimeZone(offset, customID);
    }
    return NULL;
}

UnicodeString&
TimeZone::getCustomID(const UnicodeString& id, UnicodeString& normalized, UErrorCode& status) {
    normalized.remove();
    if (U_FAILURE(status)) {
        return normalized;
    }
    int32_t sign, hour, min, sec;
    if (parseCustomID(id, sign, hour, min, sec)) {
        formatCustomID(hour, min, sec, (sign < 0), normalized);
    }
    return normalized;
}

UBool
TimeZone::parseCustomID(const UnicodeString& id, int32_t& sign,
                        int32_t& hour, int32_t& min, int32_t& sec) {
    static const int32_t         kParseFailed = -99999;

    NumberFormat* numberFormat = 0;
    UnicodeString idUppercase = id;
    idUppercase.toUpper();

    if (id.length() > GMT_ID_LENGTH &&
        idUppercase.startsWith(GMT_ID))
    {
        ParsePosition pos(GMT_ID_LENGTH);
        sign = 1;
        hour = 0;
        min = 0;
        sec = 0;

        if (id[pos.getIndex()] == MINUS /*'-'*/) {
            sign = -1;
        } else if (id[pos.getIndex()] != PLUS /*'+'*/) {
            return FALSE;
        }
        pos.setIndex(pos.getIndex() + 1);

        UErrorCode success = U_ZERO_ERROR;
        numberFormat = NumberFormat::createInstance(success);
        if(U_FAILURE(success)){
            return FALSE;
        }
        numberFormat->setParseIntegerOnly(TRUE);

        // Look for either hh:mm, hhmm, or hh
        int32_t start = pos.getIndex();
        Formattable n(kParseFailed);
        numberFormat->parse(id, n, pos);
        if (pos.getIndex() == start) {
            delete numberFormat;
            return FALSE;
        }
        hour = n.getLong();

        if (pos.getIndex() < id.length()) {
            if (pos.getIndex() - start > 2
                || id[pos.getIndex()] != COLON) {
                delete numberFormat;
                return FALSE;
            }
            // hh:mm
            pos.setIndex(pos.getIndex() + 1);
            int32_t oldPos = pos.getIndex();
            n.setLong(kParseFailed);
            numberFormat->parse(id, n, pos);
            if ((pos.getIndex() - oldPos) != 2) {
                // must be 2 digits
                delete numberFormat;
                return FALSE;
            }
            min = n.getLong();
            if (pos.getIndex() < id.length()) {
                if (id[pos.getIndex()] != COLON) {
                    delete numberFormat;
                    return FALSE;
                }
                // [:ss]
                pos.setIndex(pos.getIndex() + 1);
                oldPos = pos.getIndex();
                n.setLong(kParseFailed);
                numberFormat->parse(id, n, pos);
                if (pos.getIndex() != id.length()
                        || (pos.getIndex() - oldPos) != 2) {
                    delete numberFormat;
                    return FALSE;
                }
                sec = n.getLong();
            }
        } else {
            // Supported formats are below -
            //
            // HHmmss
            // Hmmss
            // HHmm
            // Hmm
            // HH
            // H

            int32_t length = pos.getIndex() - start;
            if (length <= 0 || 6 < length) {
                // invalid length
                delete numberFormat;
                return FALSE;
            }
            switch (length) {
                case 1:
                case 2:
                    // already set to hour
                    break;
                case 3:
                case 4:
                    min = hour % 100;
                    hour /= 100;
                    break;
                case 5:
                case 6:
                    sec = hour % 100;
                    min = (hour/100) % 100;
                    hour /= 10000;
                    break;
            }
        }

        delete numberFormat;

        if (hour > kMAX_CUSTOM_HOUR || min > kMAX_CUSTOM_MIN || sec > kMAX_CUSTOM_SEC) {
            return FALSE;
        }
        return TRUE;
    }
    return FALSE;
}

UnicodeString&
TimeZone::formatCustomID(int32_t hour, int32_t min, int32_t sec,
                         UBool negative, UnicodeString& id) {
    // Create time zone ID - GMT[+|-]hhmm[ss]
    id.setTo(GMT_ID);
    if (hour | min | sec) {
        if (negative) {
            id += (UChar)MINUS;
        } else {
            id += (UChar)PLUS;
        }

        if (hour < 10) {
            id += (UChar)ZERO_DIGIT;
        } else {
            id += (UChar)(ZERO_DIGIT + hour/10);
        }
        id += (UChar)(ZERO_DIGIT + hour%10);
        id += (UChar)COLON;
        if (min < 10) {
            id += (UChar)ZERO_DIGIT;
        } else {
            id += (UChar)(ZERO_DIGIT + min/10);
        }
        id += (UChar)(ZERO_DIGIT + min%10);

        if (sec) {
            id += (UChar)COLON;
            if (sec < 10) {
                id += (UChar)ZERO_DIGIT;
            } else {
                id += (UChar)(ZERO_DIGIT + sec/10);
            }
            id += (UChar)(ZERO_DIGIT + sec%10);
        }
    }
    return id;
}


UBool 
TimeZone::hasSameRules(const TimeZone& other) const
{
    return (getRawOffset() == other.getRawOffset() && 
            useDaylightTime() == other.useDaylightTime());
}

const char*
TimeZone::getTZDataVersion(UErrorCode& status)
{
    /* This is here to prevent race conditions. */
    UBool needsInit;
    UMTX_CHECK(&LOCK, !TZDataVersionInitialized, needsInit);
    if (needsInit) {
        int32_t len = 0;
        UResourceBundle *bundle = ures_openDirect(NULL, kZONEINFO, &status);
        const UChar *tzver = ures_getStringByKey(bundle, kTZVERSION,
            &len, &status);

        if (U_SUCCESS(status)) {
            if (len >= (int32_t)sizeof(TZDATA_VERSION)) {
                // Ensure that there is always space for a trailing nul in TZDATA_VERSION
                len = sizeof(TZDATA_VERSION) - 1;
            }
            umtx_lock(&LOCK);
            if (!TZDataVersionInitialized) {
                u_UCharsToChars(tzver, TZDATA_VERSION, len);
                TZDataVersionInitialized = TRUE;
            }
            umtx_unlock(&LOCK);
            ucln_i18n_registerCleanup(UCLN_I18N_TIMEZONE, timeZone_cleanup);
        }

        ures_close(bundle);
    }
    if (U_FAILURE(status)) {
        return NULL;
    }
    return (const char*)TZDATA_VERSION;
}

UnicodeString&
TimeZone::getCanonicalID(const UnicodeString& id, UnicodeString& canonicalID, UErrorCode& status)
{
    UBool isSystemID = FALSE;
    return getCanonicalID(id, canonicalID, isSystemID, status);
}

UnicodeString&
TimeZone::getCanonicalID(const UnicodeString& id, UnicodeString& canonicalID, UBool& isSystemID,
                         UErrorCode& status)
{
    canonicalID.remove();
    isSystemID = FALSE;
    if (U_FAILURE(status)) {
        return canonicalID;
    }
    ZoneMeta::getCanonicalSystemID(id, canonicalID, status);
    if (U_SUCCESS(status)) {
        isSystemID = TRUE;
    } else {
        // Not a system ID
        status = U_ZERO_ERROR;
        getCustomID(id, canonicalID, status);
    }
    return canonicalID;
}

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */

//eof
