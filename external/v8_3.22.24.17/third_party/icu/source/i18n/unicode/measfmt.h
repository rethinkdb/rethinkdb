/*
**********************************************************************
* Copyright (c) 2004-2006, International Business Machines
* Corporation and others.  All Rights Reserved.
**********************************************************************
* Author: Alan Liu
* Created: April 20, 2004
* Since: ICU 3.0
**********************************************************************
*/
#ifndef MEASUREFORMAT_H
#define MEASUREFORMAT_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/format.h"

/**
 * \file 
 * \brief C++ API: Formatter for measure objects.
 */

U_NAMESPACE_BEGIN

/**
 * 
 * A formatter for measure objects.  This is an abstract base class.
 *
 * <p>To format or parse a measure object, first create a formatter
 * object using a MeasureFormat factory method.  Then use that
 * object's format and parse methods.
 *
 * <p>This is an abstract class.
 *
 * @see Format
 * @author Alan Liu
 * @stable ICU 3.0
 */
class U_I18N_API MeasureFormat : public Format {

 public:

    /**
     * Return a formatter for CurrencyAmount objects in the given
     * locale.
     * @param locale desired locale
     * @param ec input-output error code
     * @return a formatter object, or NULL upon error
     * @stable ICU 3.0
     */
    static MeasureFormat* U_EXPORT2 createCurrencyFormat(const Locale& locale,
                                               UErrorCode& ec);

    /**
     * Return a formatter for CurrencyAmount objects in the default
     * locale.
     * @param ec input-output error code
     * @return a formatter object, or NULL upon error
     * @stable ICU 3.0
     */
    static MeasureFormat* U_EXPORT2 createCurrencyFormat(UErrorCode& ec);

 protected:

    /**
     * Default constructor.
     * @stable ICU 3.0
     */
    MeasureFormat();
};

U_NAMESPACE_END

#endif // #if !UCONFIG_NO_FORMATTING
#endif // #ifndef MEASUREFORMAT_H
