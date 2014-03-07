/*
*******************************************************************************
* Copyright (C) 2003-2008, International Business Machines Corporation and    *
* others. All Rights Reserved.                                                *
*******************************************************************************
*
* File BUDDHCAL.CPP
*
* Modification History:
*  05/13/2003    srl     copied from gregocal.cpp
*
*/

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "buddhcal.h"
#include "unicode/gregocal.h"
#include "umutex.h"
#include <float.h>

U_NAMESPACE_BEGIN

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(BuddhistCalendar)

static const int32_t kMaxEra = 0; // only 1 era

static const int32_t kBuddhistEraStart = -543;  // 544 BC (Gregorian)

static const int32_t kGregorianEpoch = 1970;    // used as the default value of EXTENDED_YEAR

BuddhistCalendar::BuddhistCalendar(const Locale& aLocale, UErrorCode& success)
:   GregorianCalendar(aLocale, success)
{
    setTimeInMillis(getNow(), success); // Call this again now that the vtable is set up properly.
}

BuddhistCalendar::~BuddhistCalendar()
{
}

BuddhistCalendar::BuddhistCalendar(const BuddhistCalendar& source)
: GregorianCalendar(source)
{
}

BuddhistCalendar& BuddhistCalendar::operator= ( const BuddhistCalendar& right)
{
    GregorianCalendar::operator=(right);
    return *this;
}

Calendar* BuddhistCalendar::clone(void) const
{
    return new BuddhistCalendar(*this);
}

const char *BuddhistCalendar::getType() const
{
    return "buddhist";
}

int32_t BuddhistCalendar::handleGetExtendedYear()
{
    // EXTENDED_YEAR in BuddhistCalendar is a Gregorian year.
    // The default value of EXTENDED_YEAR is 1970 (Buddhist 2513)
    int32_t year;
    if (newerField(UCAL_EXTENDED_YEAR, UCAL_YEAR) == UCAL_EXTENDED_YEAR) {
        year = internalGet(UCAL_EXTENDED_YEAR, kGregorianEpoch);
    } else {
        // extended year is a gregorian year, where 1 = 1AD,  0 = 1BC, -1 = 2BC, etc 
        year = internalGet(UCAL_YEAR, kGregorianEpoch - kBuddhistEraStart)
                + kBuddhistEraStart;
    }
    return year;
}

int32_t BuddhistCalendar::handleComputeMonthStart(int32_t eyear, int32_t month,

                                                  UBool useMonth) const
{
    return GregorianCalendar::handleComputeMonthStart(eyear, month, useMonth);
}

void BuddhistCalendar::handleComputeFields(int32_t julianDay, UErrorCode& status)
{
    GregorianCalendar::handleComputeFields(julianDay, status);
    int32_t y = internalGet(UCAL_EXTENDED_YEAR) - kBuddhistEraStart;
    internalSet(UCAL_ERA, 0);
    internalSet(UCAL_YEAR, y);
}

int32_t BuddhistCalendar::handleGetLimit(UCalendarDateFields field, ELimitType limitType) const
{
    if(field == UCAL_ERA) {
        return BE;
    } else {
        return GregorianCalendar::handleGetLimit(field,limitType);
    }
}

#if 0
void BuddhistCalendar::timeToFields(UDate theTime, UBool quick, UErrorCode& status)
{
    //Calendar::timeToFields(theTime, quick, status);

    int32_t era = internalGet(UCAL_ERA);
    int32_t year = internalGet(UCAL_YEAR);

    if(era == GregorianCalendar::BC) {
        year = 1-year;
        era = BuddhistCalendar::BE;
    } else if(era == GregorianCalendar::AD) {
        era = BuddhistCalendar::BE;
    } else {
        status = U_INTERNAL_PROGRAM_ERROR;
    }

    year = year - kBuddhistEraStart;

    internalSet(UCAL_ERA, era);
    internalSet(UCAL_YEAR, year);
}
#endif

// default century
const UDate     BuddhistCalendar::fgSystemDefaultCentury        = DBL_MIN;
const int32_t   BuddhistCalendar::fgSystemDefaultCenturyYear    = -1;

UDate           BuddhistCalendar::fgSystemDefaultCenturyStart       = DBL_MIN;
int32_t         BuddhistCalendar::fgSystemDefaultCenturyStartYear   = -1;


UBool BuddhistCalendar::haveDefaultCentury() const
{
    return TRUE;
}

UDate BuddhistCalendar::defaultCenturyStart() const
{
    return internalGetDefaultCenturyStart();
}

int32_t BuddhistCalendar::defaultCenturyStartYear() const
{
    return internalGetDefaultCenturyStartYear();
}

UDate
BuddhistCalendar::internalGetDefaultCenturyStart() const
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
BuddhistCalendar::internalGetDefaultCenturyStartYear() const
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
BuddhistCalendar::initializeSystemDefaultCentury()
{
    // initialize systemDefaultCentury and systemDefaultCenturyYear based
    // on the current time.  They'll be set to 80 years before
    // the current time.
    UErrorCode status = U_ZERO_ERROR;
    BuddhistCalendar calendar(Locale("@calendar=buddhist"),status);
    if (U_SUCCESS(status))
    {
        calendar.setTime(Calendar::getNow(), status);
        calendar.add(UCAL_YEAR, -80, status);
        UDate    newStart =  calendar.getTime(status);
        int32_t  newYear  =  calendar.get(UCAL_YEAR, status);
        umtx_lock(NULL);
        if (fgSystemDefaultCenturyStart == fgSystemDefaultCentury) {
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
