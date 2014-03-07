/*
**********************************************************************
* Copyright (c) 2004-2006, International Business Machines
* Corporation and others.  All Rights Reserved.
**********************************************************************
* Author: Alan Liu
* Created: April 26, 2004
* Since: ICU 3.0
**********************************************************************
*/
#ifndef __CURRENCYAMOUNT_H__
#define __CURRENCYAMOUNT_H__

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/measure.h"
#include "unicode/currunit.h"

/**
 * \file 
 * \brief C++ API: Currency Amount Object.
 */
 
U_NAMESPACE_BEGIN

/**
 *
 * A currency together with a numeric amount, such as 200 USD.
 *
 * @author Alan Liu
 * @stable ICU 3.0
 */
class U_I18N_API CurrencyAmount: public Measure {
 public:
    /**
     * Construct an object with the given numeric amount and the given
     * ISO currency code.
     * @param amount a numeric object; amount.isNumeric() must be TRUE
     * @param isoCode the 3-letter ISO 4217 currency code; must not be
     * NULL and must have length 3
     * @param ec input-output error code. If the amount or the isoCode
     * is invalid, then this will be set to a failing value.
     * @stable ICU 3.0
     */
    CurrencyAmount(const Formattable& amount, const UChar* isoCode,
                   UErrorCode &ec);

    /**
     * Construct an object with the given numeric amount and the given
     * ISO currency code.
     * @param amount the amount of the given currency
     * @param isoCode the 3-letter ISO 4217 currency code; must not be
     * NULL and must have length 3
     * @param ec input-output error code. If the isoCode is invalid,
     * then this will be set to a failing value.
     * @stable ICU 3.0
     */
    CurrencyAmount(double amount, const UChar* isoCode,
                   UErrorCode &ec);

    /**
     * Copy constructor
     * @stable ICU 3.0
     */
    CurrencyAmount(const CurrencyAmount& other);
 
    /**
     * Assignment operator
     * @stable ICU 3.0
     */
    CurrencyAmount& operator=(const CurrencyAmount& other);

    /**
     * Return a polymorphic clone of this object.  The result will
     * have the same class as returned by getDynamicClassID().
     * @stable ICU 3.0
     */
    virtual UObject* clone() const;

    /**
     * Destructor
     * @stable ICU 3.0
     */
    virtual ~CurrencyAmount();
    
    /**
     * Returns a unique class ID for this object POLYMORPHICALLY.
     * This method implements a simple form of RTTI used by ICU.
     * @return The class ID for this object. All objects of a given
     * class have the same class ID.  Objects of other classes have
     * different class IDs.
     * @stable ICU 3.0
     */
    virtual UClassID getDynamicClassID() const;

    /**
     * Returns the class ID for this class. This is used to compare to
     * the return value of getDynamicClassID().
     * @return The class ID for all objects of this class.
     * @stable ICU 3.0
     */
    static UClassID U_EXPORT2 getStaticClassID();

    /**
     * Return the currency unit object of this object.
     * @stable ICU 3.0
     */
    inline const CurrencyUnit& getCurrency() const;

    /**
     * Return the ISO currency code of this object.
     * @stable ICU 3.0
     */
    inline const UChar* getISOCurrency() const;
};

inline const CurrencyUnit& CurrencyAmount::getCurrency() const {
    return (const CurrencyUnit&) getUnit();
}

inline const UChar* CurrencyAmount::getISOCurrency() const {
    return getCurrency().getISOCurrency();
}

U_NAMESPACE_END

#endif // !UCONFIG_NO_FORMATTING
#endif // __CURRENCYAMOUNT_H__
