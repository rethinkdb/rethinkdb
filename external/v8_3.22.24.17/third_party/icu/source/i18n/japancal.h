/*
 ********************************************************************************
 * Copyright (C) 2003-2008, International Business Machines Corporation
 * and others. All Rights Reserved.
 ********************************************************************************
 *
 * File JAPANCAL.H
 *
 * Modification History:
 *
 *   Date        Name        Description
 *   05/13/2003  srl         copied from gregocal.h
 ********************************************************************************
 */

#ifndef JAPANCAL_H
#define JAPANCAL_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/calendar.h"
#include "unicode/gregocal.h"

U_NAMESPACE_BEGIN

/**
 * Concrete class which provides the Japanese calendar.
 * <P>
 * <code>JapaneseCalendar</code> is a subclass of <code>GregorianCalendar</code>
 * that numbers years and eras based on the reigns of the Japanese emperors.
 * The Japanese calendar is identical to the Gregorian calendar in all respects
 * except for the year and era.  The ascension of each  emperor to the throne
 * begins a new era, and the years of that era are numbered starting with the
 * year of ascension as year 1.
 * <p>
 * Note that in the year of an imperial ascension, there are two possible sets
 * of year and era values: that for the old era and for the new.  For example, a
 * new era began on January 7, 1989 AD.  Strictly speaking, the first six days
 * of that year were in the Showa era, e.g. "January 6, 64 Showa", while the rest
 * of the year was in the Heisei era, e.g. "January 7, 1 Heisei".  This class
 * handles this distinction correctly when computing dates.  However, in lenient
 * mode either form of date is acceptable as input. 
 * <p>
 * In modern times, eras have started on January 8, 1868 AD, Gregorian (Meiji),
 * July 30, 1912 (Taisho), December 25, 1926 (Showa), and January 7, 1989 (Heisei).  Constants
 * for these eras, suitable for use in the <code>UCAL_ERA</code> field, are provided
 * in this class.  Note that the <em>number</em> used for each era is more or
 * less arbitrary.  Currently, the era starting in 1053 AD is era #0; however this
 * may change in the future as we add more historical data.  Use the predefined
 * constants rather than using actual, absolute numbers.
 * <p>
 * @internal
 */
class JapaneseCalendar : public GregorianCalendar {
public:

    /**
     * Useful constants for JapaneseCalendar.  
     * @internal
     */
    U_I18N_API static uint32_t U_EXPORT2 getCurrentEra(void); // the current era

    /**
     * Constructs a JapaneseCalendar based on the current time in the default time zone
     * with the given locale.
     *
     * @param aLocale  The given locale.
     * @param success  Indicates the status of JapaneseCalendar object construction.
     *                 Returns U_ZERO_ERROR if constructed successfully.
     * @stable ICU 2.0
     */
    JapaneseCalendar(const Locale& aLocale, UErrorCode& success);


    /**
     * Destructor
     * @internal
     */
    virtual ~JapaneseCalendar();

    /**
     * Copy constructor
     * @param source    the object to be copied.
     * @internal
     */
    JapaneseCalendar(const JapaneseCalendar& source);

    /**
     * Default assignment operator
     * @param right    the object to be copied.
     * @internal
     */
    JapaneseCalendar& operator=(const JapaneseCalendar& right);

    /**
     * Create and return a polymorphic copy of this calendar.
     * @return    return a polymorphic copy of this calendar.
     * @internal
     */
    virtual Calendar* clone(void) const;

    /**
     * Return the extended year defined by the current fields.  In the 
     * Japanese calendar case, this is equal to the equivalent extended Gregorian year.
     * @internal
     */
    virtual int32_t handleGetExtendedYear();

    /**
     * Return the maximum value that this field could have, given the current date.
     * @internal
     */
    virtual int32_t getActualMaximum(UCalendarDateFields field, UErrorCode& status) const;


public:
    /**
     * Override Calendar Returns a unique class ID POLYMORPHICALLY. Pure virtual
     * override. This method is to implement a simple version of RTTI, since not all C++
     * compilers support genuine RTTI. Polymorphic operator==() and clone() methods call
     * this method.
     *
     * @return   The class ID for this object. All objects of a given class have the
     *           same class ID. Objects of other classes have different class IDs.
     * @internal
     */
    virtual UClassID getDynamicClassID(void) const;

    /**
     * Return the class ID for this class. This is useful only for comparing to a return
     * value from getDynamicClassID(). For example:
     *
     *      Base* polymorphic_pointer = createPolymorphicObject();
     *      if (polymorphic_pointer->getDynamicClassID() ==
     *          Derived::getStaticClassID()) ...
     *
     * @return   The class ID for all objects of this class.
     * @internal
     */
    U_I18N_API static UClassID U_EXPORT2 getStaticClassID(void);

    /**
     * return the calendar type, "japanese".
     *
     * @return calendar type
     * @internal
     */
    virtual const char * getType() const;

    /**
     * @return FALSE - no default century in Japanese
     * @internal 
     */
    virtual UBool haveDefaultCentury() const;

    /**
     * Not used - no default century.
     * @internal
     */
    virtual UDate defaultCenturyStart() const;
    /**
     * Not used - no default century.
     * @internal
     */
    virtual int32_t defaultCenturyStartYear() const;

private:
    JapaneseCalendar(); // default constructor not implemented

protected:
    /** 
     * Calculate the era for internal computation
     * @internal
     */
    virtual int32_t internalGetEra() const;

    /**
     * Compute fields from the JD
     * @internal
     */
    virtual void handleComputeFields(int32_t julianDay, UErrorCode& status);

    /**
     * Calculate the limit for a specified type of limit and field
     * @internal
     */
    virtual int32_t handleGetLimit(UCalendarDateFields field, ELimitType limitType) const;

    /***
     * Called by computeJulianDay.  Returns the default month (0-based) for the year,
     * taking year and era into account.  Will return the first month of the given era, if 
     * the current year is an ascension year.
     * @param eyear the extended year
     * @internal
     */
    virtual int32_t getDefaultMonthInYear(int32_t eyear);

    /***
     * Called by computeJulianDay.  Returns the default day (1-based) for the month,
     * taking currently-set year and era into account.  Will return the first day of the given
     * era, if the current month is an ascension year and month.
     * @param eyear the extended year
     * @param mon the month in the year
     * @internal
     */
    virtual int32_t getDefaultDayInMonth(int32_t eyear, int32_t month);
};

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */

#endif
//eof

