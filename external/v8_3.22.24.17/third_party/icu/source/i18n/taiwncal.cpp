/*
 *******************************************************************************
 * Copyright (C) 2003-2008, International Business Machines Corporation and    *
 * others. All Rights Reserved.                                                *
 *******************************************************************************
 *
 * File TAIWNCAL.CPP
 *
 * Modification History:
 *  05/13/2003    srl     copied from gregocal.cpp
 *  06/29/2007    srl     copied from buddhcal.cpp
 *  05/12/2008    jce     modified to use calendar=roc per CLDR
 *
 */

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "taiwncal.h"
#include "unicode/gregocal.h"
#include "umutex.h"
#include <float.h>

U_NAMESPACE_BEGIN

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(TaiwanCalendar)

static const int32_t kTaiwanEraStart = 1911;  // 1911 (Gregorian)

static const int32_t kGregorianEpoch = 1970; 

TaiwanCalendar::TaiwanCalendar(const Locale& aLocale, UErrorCode& success)
:   GregorianCalendar(aLocale, success)
{
    setTimeInMillis(getNow(), success); // Call this again now that the vtable is set up properly.
}

TaiwanCalendar::~TaiwanCalendar()
{
}

TaiwanCalendar::TaiwanCalendar(const TaiwanCalendar& source)
: GregorianCalendar(source)
{
}

TaiwanCalendar& TaiwanCalendar::operator= ( const TaiwanCalendar& right)
{
    GregorianCalendar::operator=(right);
    return *this;
}

Calendar* TaiwanCalendar::clone(void) const
{
    return new TaiwanCalendar(*this);
}

const char *TaiwanCalendar::getType() const
{
    return "roc";
}

int32_t TaiwanCalendar::handleGetExtendedYear()
{
    // EXTENDED_YEAR in TaiwanCalendar is a Gregorian year
    // The default value of EXTENDED_YEAR is 1970 (Minguo 59)
    int32_t year = kGregorianEpoch;

    if (newerField(UCAL_EXTENDED_YEAR, UCAL_YEAR) == UCAL_EXTENDED_YEAR
        && newerField(UCAL_EXTENDED_YEAR, UCAL_ERA) == UCAL_EXTENDED_YEAR) {
        year = internalGet(UCAL_EXTENDED_YEAR, kGregorianEpoch);
    } else {
        int32_t era = internalGet(UCAL_ERA, MINGUO);
        if(era == MINGUO) {
            year =     internalGet(UCAL_YEAR, 1) + kTaiwanEraStart;
        } else if(era == BEFORE_MINGUO) {
            year = 1 - internalGet(UCAL_YEAR, 1) + kTaiwanEraStart;
        }
    }
    return year;
}

void TaiwanCalendar::handleComputeFields(int32_t julianDay, UErrorCode& status)
{
    GregorianCalendar::handleComputeFields(julianDay, status);
    int32_t y = internalGet(UCAL_EXTENDED_YEAR) - kTaiwanEraStart;
    if(y>0) {
        internalSet(UCAL_ERA, MINGUO);
        internalSet(UCAL_YEAR, y);
    } else {
        internalSet(UCAL_ERA, BEFORE_MINGUO);
        internalSet(UCAL_YEAR, 1-y);
    }
}

int32_t TaiwanCalendar::handleGetLimit(UCalendarDateFields field, ELimitType limitType) const
{
    if(field == UCAL_ERA) {
        if(limitType == UCAL_LIMIT_MINIMUM || limitType == UCAL_LIMIT_GREATEST_MINIMUM) {
            return BEFORE_MINGUO;
        } else {
            return MINGUO;
        }
    } else {
        return GregorianCalendar::handleGetLimit(field,limitType);
    }
}

#if 0
void TaiwanCalendar::timeToFields(UDate theTime, UBool quick, UErrorCode& status)
{
    //Calendar::timeToFields(theTime, quick, status);

    int32_t era = internalGet(UCAL_ERA);
    int32_t year = internalGet(UCAL_YEAR);

    if(era == GregorianCalendar::BC) {
        year = 1-year;
        era = TaiwanCalendar::MINGUO;
    } else if(era == GregorianCalendar::AD) {
        era = TaiwanCalendar::MINGUO;
    } else {
        status = U_INTERNAL_PROGRAM_ERROR;
    }

    year = year - kTaiwanEraStart;

    internalSet(UCAL_ERA, era);
    internalSet(UCAL_YEAR, year);
}
#endif

// default century
const UDate     TaiwanCalendar::fgSystemDefaultCentury        = DBL_MIN;
const int32_t   TaiwanCalendar::fgSystemDefaultCenturyYear    = -1;

UDate           TaiwanCalendar::fgSystemDefaultCenturyStart       = DBL_MIN;
int32_t         TaiwanCalendar::fgSystemDefaultCenturyStartYear   = -1;


UBool TaiwanCalendar::haveDefaultCentury() const
{
    return TRUE;
}

UDate TaiwanCalendar::defaultCenturyStart() const
{
    return internalGetDefaultCenturyStart();
}

int32_t TaiwanCalendar::defaultCenturyStartYear() const
{
    return internalGetDefaultCenturyStartYear();
}

UDate
TaiwanCalendar::internalGetDefaultCenturyStart() const
{
    // lazy-evaluate systemDefaultCenturyStart
    UBool needsUpdate;
    UMTX_CHECK(NULL, (fgSystemDefaultCenturyStart == fgSystemDefaultCentury), needsUpdate);

    if (needsUpdate) {
        initializeSystemDefaultCentury();
    }

    // use defaultCenturyStart unless it's the flag value;
    // then use systemDefaultCenturyStart

    return fgSystemDefaultCenturyStart;
}

int32_t
TaiwanCalendar::internalGetDefaultCenturyStartYear() const
{
    // lazy-evaluate systemDefaultCenturyStartYear
    UBool needsUpdate;
    UMTX_CHECK(NULL, (fgSystemDefaultCenturyStart == fgSystemDefaultCentury), needsUpdate);

    if (needsUpdate) {
        initializeSystemDefaultCentury();
    }

    // use defaultCenturyStart unless it's the flag value;
    // then use systemDefaultCenturyStartYear

    return    fgSystemDefaultCenturyStartYear;
}

void
TaiwanCalendar::initializeSystemDefaultCentury()
{
    // initialize systemDefaultCentury and systemDefaultCenturyYear based
    // on the current time.  They'll be set to 80 years before
    // the current time.
    UErrorCode status = U_ZERO_ERROR;
    TaiwanCalendar calendar(Locale("@calendar=roc"),status);
    if (U_SUCCESS(status))
    {
        calendar.setTime(Calendar::getNow(), status);
        calendar.add(UCAL_YEAR, -80, status);
        UDate    newStart =  calendar.getTime(status);
        int32_t  newYear  =  calendar.get(UCAL_YEAR, status);
        umtx_lock(NULL);
        if (fgSystemDefaultCenturyStart == fgSystemDefaultCentury)
        {
            fgSystemDefaultCenturyStartYear = newYear;
            fgSystemDefaultCenturyStart = newStart;
        }
        umtx_unlock(NULL);
    }
    // We have no recourse upon failure unless we want to propagate the failure
    // out.
}


U_NAMESPACE_END

#endif
