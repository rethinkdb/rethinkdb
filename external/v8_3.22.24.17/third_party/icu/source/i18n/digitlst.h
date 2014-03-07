/*
******************************************************************************
*
*   Copyright (C) 1997-2010, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
******************************************************************************
*
* File DIGITLST.H
*
* Modification History:
*
*   Date        Name        Description
*   02/25/97    aliu        Converted from java.
*   03/21/97    clhuang     Updated per C++ implementation.
*   04/15/97    aliu        Changed MAX_COUNT to DBL_DIG.  Changed Digit to char.
*   09/09/97    aliu        Adapted for exponential notation support.
*   08/02/98    stephen     Added nearest/even rounding
*   06/29/99    stephen     Made LONG_DIGITS a macro to satisfy SUN compiler
*   07/09/99    stephen     Removed kMaxCount (unused, for HP compiler)
******************************************************************************
*/
 
#ifndef DIGITLST_H
#define DIGITLST_H
 
#include "unicode/uobject.h"

#if !UCONFIG_NO_FORMATTING
#include "unicode/decimfmt.h"
#include <float.h>
#include "decContext.h"
#include "decNumber.h"
#include "cmemory.h"

// Decimal digits in a 64-bit int
#define INT64_DIGITS 19

typedef enum EDigitListValues {
    MAX_DBL_DIGITS = DBL_DIG,
    MAX_I64_DIGITS = INT64_DIGITS,
    MAX_DIGITS = MAX_I64_DIGITS,
    MAX_EXPONENT = DBL_DIG,
    DIGIT_PADDING = 3,
    DEFAULT_DIGITS = 40,   // Initial storage size, will grow as needed.

     // "+." + fDigits + "e" + fDecimalAt
    MAX_DEC_DIGITS = MAX_DIGITS + DIGIT_PADDING + MAX_EXPONENT
} EDigitListValues;

U_NAMESPACE_BEGIN

class CharString;

// Export an explicit template instantiation of the MaybeStackHeaderAndArray that
//    is used as a data member of DigitList.
//
//    MSVC requires this, even though it should not be necessary. 
//    No direct access to the MaybeStackHeaderAndArray leaks out of the i18n library.
//
//    Macintosh produces duplicate definition linker errors with the explicit template
//    instantiation.
//
#if !defined(U_DARWIN)
template class U_I18N_API MaybeStackHeaderAndArray<decNumber, char, DEFAULT_DIGITS>;
#endif


/**
 * Digit List is actually a Decimal Floating Point number.
 * The original implementation has been replaced by a thin wrapper onto a 
 * decimal number from the decNumber library.
 *
 * The original DigitList API has been retained, to minimize the impact of
 * the change on the rest of the ICU formatting code.
 *
 * The change to decNumber enables support for big decimal numbers, and
 * allows rounding computations to be done directly in decimal, avoiding
 * extra, and inaccurate, conversions to and from doubles.
 *
 * Original DigitList comments:
 *
 * Digit List utility class. Private to DecimalFormat.  Handles the transcoding
 * between numeric values and strings of characters.  Only handles
 * non-negative numbers.  The division of labor between DigitList and
 * DecimalFormat is that DigitList handles the radix 10 representation
 * issues; DecimalFormat handles the locale-specific issues such as
 * positive/negative, grouping, decimal point, currency, and so on.
 * <P>
 * A DigitList is really a representation of a floating point value.
 * It may be an integer value; we assume that a double has sufficient
 * precision to represent all digits of a long.
 * <P>
 * The DigitList representation consists of a string of characters,
 * which are the digits radix 10, from '0' to '9'.  It also has a radix
 * 10 exponent associated with it.  The value represented by a DigitList
 * object can be computed by mulitplying the fraction f, where 0 <= f < 1,
 * derived by placing all the digits of the list to the right of the
 * decimal point, by 10^exponent.
 *
 * --------
 *
 * DigitList vs. decimalNumber:
 *
 *    DigitList stores digits with the most significant first.
 *    decNumber stores digits with the least significant first.
 *
 *    DigitList, decimal point is before the most significant.
 *    decNumber, decimal point is after the least signficant digit.
 *
 *       digitList:    0.ddddd * 10 ^ exp
 *       decNumber:    ddddd. * 10 ^ exp
 *
 *       digitList exponent = decNumber exponent + digit count
 *
 *    digitList, digits are platform invariant chars, '0' - '9'
 *    decNumber, digits are binary, one per byte, 0 - 9.
 *
 *       (decNumber library is configurable in how digits are stored, ICU has configured
 *        it this way for convenience in replacing the old DigitList implementation.)
 */
class U_I18N_API DigitList : public UMemory { // Declare external to make compiler happy
public:

    DigitList();
    ~DigitList();

    /* copy constructor
     * @param DigitList The object to be copied.
     * @return the newly created object. 
     */
    DigitList(const DigitList&); // copy constructor

    /* assignment operator
     * @param DigitList The object to be copied.
     * @return the newly created object.
     */
    DigitList& operator=(const DigitList&);  // assignment operator

    /**
     * Return true if another object is semantically equal to this one.
     * @param other The DigitList to be compared for equality
     * @return true if another object is semantically equal to this one.
     * return false otherwise.
     */
    UBool operator==(const DigitList& other) const;

    int32_t  compare(const DigitList& other);


    inline UBool operator!=(const DigitList& other) const { return !operator==(other); };

    /**
     * Clears out the digits.
     * Use before appending them.
     * Typically, you set a series of digits with append, then at the point
     * you hit the decimal point, you set myDigitList.fDecimalAt = myDigitList.fCount;
     * then go on appending digits.
     */
    void clear(void);

    /**
     *  Remove, by rounding, any fractional part of the decimal number,
     *  leaving an integer value.
     */
    void toIntegralValue();
    
    /**
     * Appends digits to the list. 
     *    CAUTION:  this function is not recommended for new code.
     *              In the original DigitList implementation, decimal numbers were
     *              parsed by appending them to a digit list as they were encountered.
     *              With the revamped DigitList based on decNumber, append is very
     *              inefficient, and the interaction with the exponent value is confusing.
     *              Best avoided.
     *              TODO:  remove this function once all use has been replaced.
     *              TODO:  describe alternative to append()
     * @param digit The digit to be appended.
     */
    void append(char digit);

    /**
     * Utility routine to get the value of the digit list
     * Returns 0.0 if zero length.
     * @return the value of the digit list.
     */
    double getDouble(void) const;

    /**
     * Utility routine to get the value of the digit list
     * Make sure that fitsIntoLong() is called before calling this function.
     * Returns 0 if zero length.
     * @return the value of the digit list, return 0 if it is zero length
     */
    int32_t getLong(void) /*const*/;

    /**
     * Utility routine to get the value of the digit list
     * Make sure that fitsIntoInt64() is called before calling this function.
     * Returns 0 if zero length.
     * @return the value of the digit list, return 0 if it is zero length
     */
    int64_t getInt64(void) /*const*/;

    /**
     *  Utility routine to get the value of the digit list as a decimal string.
     */
    void getDecimal(CharString &str, UErrorCode &status);

    /**
     * Return true if the number represented by this object can fit into
     * a long.
     * @param ignoreNegativeZero True if negative zero is ignored.
     * @return true if the number represented by this object can fit into
     * a long, return false otherwise.
     */
    UBool fitsIntoLong(UBool ignoreNegativeZero) /*const*/;

    /**
     * Return true if the number represented by this object can fit into
     * an int64_t.
     * @param ignoreNegativeZero True if negative zero is ignored.
     * @return true if the number represented by this object can fit into
     * a long, return false otherwise.
     */
    UBool fitsIntoInt64(UBool ignoreNegativeZero) /*const*/;

    /**
     * Utility routine to set the value of the digit list from a double.
     * @param source The value to be set
     */
    void set(double source);

    /**
     * Utility routine to set the value of the digit list from a long.
     * If a non-zero maximumDigits is specified, no more than that number of
     * significant digits will be produced.
     * @param source The value to be set
     */
    void set(int32_t source);

    /**
     * Utility routine to set the value of the digit list from an int64.
     * If a non-zero maximumDigits is specified, no more than that number of
     * significant digits will be produced.
     * @param source The value to be set
     */
    void set(int64_t source);

   /**
     * Utility routine to set the value of the digit list from a decimal number
     * string.
     * @param source The value to be set.  The string must be nul-terminated.
     */
    void set(const StringPiece &source, UErrorCode &status);

    /**
     * Multiply    this = this * arg
     *    This digitlist will be expanded if necessary to accomodate the result.
     *  @param arg  the number to multiply by.
     */
    void mult(const DigitList &arg, UErrorCode &status);

    /**
     *   Divide    this = this / arg
     */
    void div(const DigitList &arg, UErrorCode &status);

    //  The following functions replace direct access to the original DigitList implmentation
    //  data structures.

    void setRoundingMode(DecimalFormat::ERoundingMode m); 

    /** Test a number for zero.
     * @return  TRUE if the number is zero
     */
    UBool isZero(void) const;

    /** Test for a Nan
     * @return  TRUE if the number is a NaN
     */
    UBool isNaN(void) const {return decNumberIsNaN(fDecNumber);};

    UBool isInfinite() const {return decNumberIsInfinite(fDecNumber);};

    /**  Reduce, or normalize.  Removes trailing zeroes, adjusts exponent appropriately. */
    void     reduce();

    /**  Remove trailing fraction zeros, adjust exponent accordingly. */
    void     trim();

    /** Set to zero */
    void     setToZero() {uprv_decNumberZero(fDecNumber);};

    /** get the number of digits in the decimal number */
    int32_t  digits() const {return fDecNumber->digits;};

    /**
     * Round the number to the given number of digits.
     * @param maximumDigits The maximum number of digits to be shown.
     * Upon return, count will be less than or equal to maximumDigits.
     */
    void round(int32_t maximumDigits);

    void roundFixedPoint(int32_t maximumFractionDigits);

    /** Ensure capacity for digits.  Grow the storage if it is currently less than
     *      the requested size.   Capacity is not reduced if it is already greater
     *      than requested.
     */
    void  ensureCapacity(int32_t  requestedSize, UErrorCode &status); 

    UBool    isPositive(void) const { return decNumberIsNegative(fDecNumber) == 0;};
    void     setPositive(UBool s); 

    void     setDecimalAt(int32_t d);
    int32_t  getDecimalAt();

    void     setCount(int32_t c);
    int32_t  getCount() const;
    
    /**
     * Set the digit in platform (invariant) format, from '0'..'9'
     * @param i index of digit
     * @param v digit value, from '0' to '9' in platform invariant format
     */
    void     setDigit(int32_t i, char v);

    /**
     * Get the digit in platform (invariant) format, from '0'..'9' inclusive
     * @param i index of digit
     * @return invariant format of the digit
     */
    char     getDigit(int32_t i);


    /**
     * Get the digit's value, as an integer from 0..9 inclusive.
     * Note that internally this value is a decNumberUnit, but ICU configures it to be a uint8_t.
     * @param i index of digit
     * @return value of that digit
     */
    uint8_t     getDigitValue(int32_t i);


private:
    /*
     * These data members are intentionally public and can be set directly.
     *<P>
     * The value represented is given by placing the decimal point before
     * fDigits[fDecimalAt].  If fDecimalAt is < 0, then leading zeros between
     * the decimal point and the first nonzero digit are implied.  If fDecimalAt
     * is > fCount, then trailing zeros between the fDigits[fCount-1] and the
     * decimal point are implied.
     * <P>
     * Equivalently, the represented value is given by f * 10^fDecimalAt.  Here
     * f is a value 0.1 <= f < 1 arrived at by placing the digits in fDigits to
     * the right of the decimal.
     * <P>
     * DigitList is normalized, so if it is non-zero, fDigits[0] is non-zero.  We
     * don't allow denormalized numbers because our exponent is effectively of
     * unlimited magnitude.  The fCount value contains the number of significant
     * digits present in fDigits[].
     * <P>
     * Zero is represented by any DigitList with fCount == 0 or with each fDigits[i]
     * for all i <= fCount == '0'.
     *
     * int32_t                         fDecimalAt;
     * int32_t                         fCount;
     * UBool                           fIsPositive;
     * char                            *fDigits;
     * DecimalFormat::ERoundingMode    fRoundingMode;
     */

private:

    decContext    fContext;
    decNumber     *fDecNumber;
    MaybeStackHeaderAndArray<decNumber, char, DEFAULT_DIGITS>  fStorage;

    /* Cached double value corresponding to this decimal number.
     * This is an optimization for the formatting implementation, which may
     * ask for the double value multiple times.
     */
    double        fDouble;
    UBool         fHaveDouble;



    UBool shouldRoundUp(int32_t maximumDigits) const;
};


U_NAMESPACE_END

#endif // #if !UCONFIG_NO_FORMATTING
#endif // _DIGITLST

//eof
