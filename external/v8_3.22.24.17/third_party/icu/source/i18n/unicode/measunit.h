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
#ifndef __MEASUREUNIT_H__
#define __MEASUREUNIT_H__

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/fmtable.h"

/**
 * \file 
 * \brief C++ API: A unit for measuring a quantity.
 */
 
U_NAMESPACE_BEGIN

/**
 * A unit such as length, mass, volume, currency, etc.  A unit is
 * coupled with a numeric amount to produce a Measure.
 *
 * <p>This is an abstract class.
 *
 * @author Alan Liu
 * @stable ICU 3.0
 */
class U_I18N_API MeasureUnit: public UObject {
 public:
    /**
     * Return a polymorphic clone of this object.  The result will
     * have the same class as returned by getDynamicClassID().
     * @stable ICU 3.0
     */
    virtual UObject* clone() const = 0;

    /**
     * Destructor
     * @stable ICU 3.0
     */
    virtual ~MeasureUnit();
    
    /**
     * Equality operator.  Return true if this object is equal
     * to the given object.
     * @stable ICU 3.0
     */
    virtual UBool operator==(const UObject& other) const = 0;

 protected:
    /**
     * Default constructor.
     * @stable ICU 3.0
     */
    MeasureUnit();
};

U_NAMESPACE_END

// NOTE: There is no measunit.cpp. For implementation, see measure.cpp. [alan]

#endif // !UCONFIG_NO_FORMATTING
#endif // __MEASUREUNIT_H__
