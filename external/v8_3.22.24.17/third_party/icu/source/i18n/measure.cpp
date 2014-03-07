/*
**********************************************************************
* Copyright (c) 2004-2010, International Business Machines
* Corporation and others.  All Rights Reserved.
**********************************************************************
* Author: Alan Liu
* Created: April 26, 2004
* Since: ICU 3.0
**********************************************************************
*/
#include "unicode/utypeinfo.h"  // for 'typeid' to work

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/measure.h"
#include "unicode/measunit.h"

U_NAMESPACE_BEGIN

Measure::Measure() {}

Measure::Measure(const Formattable& _number, MeasureUnit* adoptedUnit,
                 UErrorCode& ec) :
    number(_number), unit(adoptedUnit) {
    if (U_SUCCESS(ec) &&
        (!number.isNumeric() || adoptedUnit == 0)) {
        ec = U_ILLEGAL_ARGUMENT_ERROR;
    }
}

Measure::Measure(const Measure& other) :
    UObject(other), unit(0) {
    *this = other;
}

Measure& Measure::operator=(const Measure& other) {
    if (this != &other) {
        delete unit;
        number = other.number;
        unit = (MeasureUnit*) other.unit->clone();
    }
    return *this;
}

Measure::~Measure() {
    delete unit;
}

UBool Measure::operator==(const UObject& other) const {
    const Measure* m = (const Measure*) &other;
    return typeid(*this) == typeid(other) &&
        number == m->getNumber() && 
        (unit != NULL && *unit == m->getUnit());
}

//----------------------------------------------------------------------
// MeasureUnit implementation

MeasureUnit:: MeasureUnit() {}

MeasureUnit::~MeasureUnit() {}

U_NAMESPACE_END

#endif // !UCONFIG_NO_FORMATTING
