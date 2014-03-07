/*
**********************************************************************
*   Copyright (C) 1997-2010, International Business Machines
*   Corporation and others.  All Rights Reserved.
**********************************************************************
*
* File DIGITLST.CPP
*
* Modification History:
*
*   Date        Name        Description
*   03/21/97    clhuang     Converted from java.
*   03/21/97    clhuang     Implemented with new APIs.
*   03/27/97    helena      Updated to pass the simple test after code review.
*   03/31/97    aliu        Moved isLONG_MIN to here, and fixed it.
*   04/15/97    aliu        Changed MAX_COUNT to DBL_DIG.  Changed Digit to char.
*                           Reworked representation by replacing fDecimalAt
*                           with fExponent.
*   04/16/97    aliu        Rewrote set() and getDouble() to use sprintf/atof
*                           to do digit conversion.
*   09/09/97    aliu        Modified for exponential notation support.
*   08/02/98    stephen     Added nearest/even rounding
*                            Fixed bug in fitsIntoLong
******************************************************************************
*/

#include "digitlst.h"

#if !UCONFIG_NO_FORMATTING
#include "unicode/putil.h"
#include "charstr.h"
#include "cmemory.h"
#include "cstring.h"
#include "putilimp.h"
#include "uassert.h"
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <limits>

// ***************************************************************************
// class DigitList
//    A wrapper onto decNumber.
//    Used to be standalone.
// ***************************************************************************

/**
 * This is the zero digit.  The base for the digits returned by getDigit()
 * Note that it is the platform invariant digit, and is not Unicode.
 */
#define kZero '0'

static char gDecimal = 0;

/* Only for 32 bit numbers. Ignore the negative sign. */
static const char LONG_MIN_REP[] = "2147483648";
static const char I64_MIN_REP[] = "9223372036854775808";


U_NAMESPACE_BEGIN

static void
loadDecimalChar() {
    if (gDecimal == 0) {
        char rep[MAX_DIGITS];
        // For machines that decide to change the decimal on you,
        // and try to be too smart with localization.
        // This normally should be just a '.'.
        sprintf(rep, "%+1.1f", 1.0);
        gDecimal = rep[2];
    }
}

// -------------------------------------
// default constructor

DigitList::DigitList()
{
    uprv_decContextDefault(&fContext, DEC_INIT_BASE);
    fContext.traps  = 0;
    uprv_decContextSetRounding(&fContext, DEC_ROUND_HALF_EVEN);
    fContext.digits = fStorage.getCapacity();

    fDecNumber = fStorage.getAlias();
    uprv_decNumberZero(fDecNumber);

    fDouble = 0.0;
    fHaveDouble = TRUE;
}

// -------------------------------------

DigitList::~DigitList()
{
}

// -------------------------------------
// copy constructor

DigitList::DigitList(const DigitList &other)
{
    fDecNumber = fStorage.getAlias();
    *this = other;
}


// -------------------------------------
// assignment operator

DigitList&
DigitList::operator=(const DigitList& other)
{
    if (this != &other)
    {
        uprv_memcpy(&fContext, &other.fContext, sizeof(decContext));

        if (other.fStorage.getCapacity() > fStorage.getCapacity()) {
            fDecNumber = fStorage.resize(other.fStorage.getCapacity());
        }
        // Always reset the fContext.digits, even if fDecNumber was not reallocated,
        // because above we copied fContext from other.fContext.
        fContext.digits = fStorage.getCapacity();
        uprv_decNumberCopy(fDecNumber, other.fDecNumber);

        fDouble = other.fDouble;
        fHaveDouble = other.fHaveDouble;
    }
    return *this;
}

// -------------------------------------
//    operator ==  (does not exactly match the old DigitList function)

UBool
DigitList::operator==(const DigitList& that) const
{
    if (this == &that) {
        return TRUE;
    }
    decNumber n;  // Has space for only a none digit value.
    decContext c;
    uprv_decContextDefault(&c, DEC_INIT_BASE);
    c.digits = 1;
    c.traps = 0;

    uprv_decNumberCompare(&n, this->fDecNumber, that.fDecNumber, &c);
    UBool result = decNumberIsZero(&n);
    return result;
}

// -------------------------------------
//      comparison function.   Returns 
//         Not Comparable :  -2
//                      < :  -1
//                     == :   0
//                      > :  +1
int32_t DigitList::compare(const DigitList &other) {
    decNumber   result;
    int32_t     savedDigits = fContext.digits;
    fContext.digits = 1;
    uprv_decNumberCompare(&result, this->fDecNumber, other.fDecNumber, &fContext);
    fContext.digits = savedDigits;
    if (decNumberIsZero(&result)) {
        return 0;
    } else if (decNumberIsSpecial(&result)) {
        return -2;
    } else if (result.bits & DECNEG) {
        return -1;
    } else {
        return 1;
    }
}


// -------------------------------------
//  Reduce - remove trailing zero digits.
void
DigitList::reduce() {
    uprv_decNumberReduce(fDecNumber, fDecNumber, &fContext);
}


// -------------------------------------
//  trim - remove trailing fraction zero digits.
void
DigitList::trim() {
    uprv_decNumberTrim(fDecNumber);
}

// -------------------------------------
// Resets the digit list; sets all the digits to zero.

void
DigitList::clear()
{
    uprv_decNumberZero(fDecNumber);
    uprv_decContextSetRounding(&fContext, DEC_ROUND_HALF_EVEN);
    fDouble = 0.0;
    fHaveDouble = TRUE;
}


/**
 * Formats a int64_t number into a base 10 string representation, and NULL terminates it.
 * @param number The number to format
 * @param outputStr The string to output to.  Must be at least MAX_DIGITS+2 in length (21),
 *                  to hold the longest int64_t value.
 * @return the number of digits written, not including the sign.
 */
static int32_t
formatBase10(int64_t number, char *outputStr) {
    // The number is output backwards, starting with the LSD.
    // Fill the buffer from the far end.  After the number is complete,
    // slide the string contents to the front.

    const int32_t MAX_IDX = MAX_DIGITS+2;
    int32_t destIdx = MAX_IDX;
    outputStr[--destIdx] = 0; 

    int64_t  n = number;
    if (number < 0) {   // Negative numbers are slightly larger than a postive
        outputStr[--destIdx] = (char)(-(n % 10) + kZero);
        n /= -10;
    }
    do { 
        outputStr[--destIdx] = (char)(n % 10 + kZero);
        n /= 10;
    } while (n > 0);
    
    if (number < 0) {
        outputStr[--destIdx] = '-';
    }

    // Slide the number to the start of the output str
    U_ASSERT(destIdx >= 0);
    int32_t length = MAX_IDX - destIdx;
    uprv_memmove(outputStr, outputStr+MAX_IDX-length, length);

    return length;
}


// -------------------------------------

void 
DigitList::setRoundingMode(DecimalFormat::ERoundingMode m) {
    enum rounding r;

    switch (m) {
      case  DecimalFormat::kRoundCeiling:  r = DEC_ROUND_CEILING;   break;
      case  DecimalFormat::kRoundFloor:    r = DEC_ROUND_FLOOR;     break;
      case  DecimalFormat::kRoundDown:     r = DEC_ROUND_DOWN;      break;
      case  DecimalFormat::kRoundUp:       r = DEC_ROUND_UP;        break;
      case  DecimalFormat::kRoundHalfEven: r = DEC_ROUND_HALF_EVEN; break;
      case  DecimalFormat::kRoundHalfDown: r = DEC_ROUND_HALF_DOWN; break;
      case  DecimalFormat::kRoundHalfUp:   r = DEC_ROUND_HALF_UP;   break;
      default:
         // TODO: how to report the problem?
         // Leave existing mode unchanged.
         r = uprv_decContextGetRounding(&fContext);
    }
    uprv_decContextSetRounding(&fContext, r);
  
}


// -------------------------------------

void  
DigitList::setPositive(UBool s) {
    if (s) {
        fDecNumber->bits &= ~DECNEG; 
    } else {
        fDecNumber->bits |= DECNEG;
    }
    fHaveDouble = FALSE;
}
// -------------------------------------

void     
DigitList::setDecimalAt(int32_t d) {
    U_ASSERT((fDecNumber->bits & DECSPECIAL) == 0);  // Not Infinity or NaN
    U_ASSERT(d-1>-999999999);
    U_ASSERT(d-1< 999999999);
    int32_t adjustedDigits = fDecNumber->digits;
    if (decNumberIsZero(fDecNumber)) {
        // Account for difference in how zero is represented between DigitList & decNumber.
        adjustedDigits = 0;
    }
    fDecNumber->exponent = d - adjustedDigits;
    fHaveDouble = FALSE;
}

int32_t  
DigitList::getDecimalAt() {
    U_ASSERT((fDecNumber->bits & DECSPECIAL) == 0);  // Not Infinity or NaN
    if (decNumberIsZero(fDecNumber) || ((fDecNumber->bits & DECSPECIAL) != 0)) {
        return fDecNumber->exponent;  // Exponent should be zero for these cases.
    }
    return fDecNumber->exponent + fDecNumber->digits;
}

void     
DigitList::setCount(int32_t c)  {
    U_ASSERT(c <= fContext.digits);
    if (c == 0) {
        // For a value of zero, DigitList sets all fields to zero, while
        // decNumber keeps one digit (with that digit being a zero)
        c = 1;
        fDecNumber->lsu[0] = 0;
    }
    fDecNumber->digits = c;
    fHaveDouble = FALSE;
}

int32_t  
DigitList::getCount() const {
    if (decNumberIsZero(fDecNumber) && fDecNumber->exponent==0) {
       // The extra test for exponent==0 is needed because parsing sometimes appends
       // zero digits.  It's bogus, decimalFormatter parsing needs to be cleaned up.
       return 0;
    } else {
       return fDecNumber->digits;
    }
}
    
void     
DigitList::setDigit(int32_t i, char v) {
    int32_t count = fDecNumber->digits;
    U_ASSERT(i<count);
    U_ASSERT(v>='0' && v<='9');
    v &= 0x0f;
    fDecNumber->lsu[count-i-1] = v;
    fHaveDouble = FALSE;
}

char     
DigitList::getDigit(int32_t i) {
    int32_t count = fDecNumber->digits;
    U_ASSERT(i<count);
    return fDecNumber->lsu[count-i-1] + '0';
}

// copied from DigitList::getDigit()
uint8_t
DigitList::getDigitValue(int32_t i) {
    int32_t count = fDecNumber->digits;
    U_ASSERT(i<count);
    return fDecNumber->lsu[count-i-1];
}

// -------------------------------------
// Appends the digit to the digit list if it's not out of scope.
// Ignores the digit, otherwise.
// 
// This function is horribly inefficient to implement with decNumber because
// the digits are stored least significant first, which requires moving all
// existing digits down one to make space for the new one to be appended.
//
void
DigitList::append(char digit)
{
    U_ASSERT(digit>='0' && digit<='9');
    // Ignore digits which exceed the precision we can represent
    //    And don't fix for larger precision.  Fix callers instead.
    if (decNumberIsZero(fDecNumber)) {
        // Zero needs to be special cased because of the difference in the way
        // that the old DigitList and decNumber represent it.
        // digit cout was zero for digitList, is one for decNumber
        fDecNumber->lsu[0] = digit & 0x0f;
        fDecNumber->digits = 1;
        fDecNumber->exponent--;     // To match the old digit list implementation.
    } else {
        int32_t nDigits = fDecNumber->digits;
        if (nDigits < fContext.digits) {
            int i;
            for (i=nDigits; i>0; i--) {
                fDecNumber->lsu[i] = fDecNumber->lsu[i-1];
            }
            fDecNumber->lsu[0] = digit & 0x0f;
            fDecNumber->digits++;
            // DigitList emulation - appending doesn't change the magnitude of existing
            //                       digits.  With decNumber's decimal being after the
            //                       least signficant digit, we need to adjust the exponent.
            fDecNumber->exponent--;
        }
    }
    fHaveDouble = FALSE;
}

// -------------------------------------

/**
 * Currently, getDouble() depends on atof() to do its conversion.
 *
 * WARNING!!
 * This is an extremely costly function. ~1/2 of the conversion time
 * can be linked to this function.
 */
double
DigitList::getDouble() const
{
    // TODO:  fix thread safety.  Can probably be finessed some by analyzing
    //        what public const functions can see which DigitLists.
    //        Like precompute fDouble for DigitLists coming in from a parse
    //        or from a Formattable::set(), but not for any others.
    if (fHaveDouble) {
        return fDouble;
    }
    DigitList *nonConstThis = const_cast<DigitList *>(this);

    if (isZero()) {
        nonConstThis->fDouble = 0.0;
        if (decNumberIsNegative(fDecNumber)) {
            nonConstThis->fDouble /= -1;
        }
    } else if (isInfinite()) {
        if (std::numeric_limits<double>::has_infinity) {
            nonConstThis->fDouble = std::numeric_limits<double>::infinity();
        } else {
            nonConstThis->fDouble = std::numeric_limits<double>::max();
        }
        if (!isPositive()) {
            nonConstThis->fDouble = -fDouble;
        } 
    } else {
        MaybeStackArray<char, MAX_DBL_DIGITS+18> s;
           // Note:  14 is a  magic constant from the decNumber library documentation,
           //        the max number of extra characters beyond the number of digits 
           //        needed to represent the number in string form.  Add a few more
           //        for the additional digits we retain.

        // Round down to appx. double precision, if the number is longer than that.
        // Copy the number first, so that we don't modify the original.
        if (getCount() > MAX_DBL_DIGITS + 3) {
            DigitList numToConvert(*this);
            numToConvert.reduce();    // Removes any trailing zeros, so that digit count is good.
            numToConvert.round(MAX_DBL_DIGITS+3);
            uprv_decNumberToString(numToConvert.fDecNumber, s);
            // TODO:  how many extra digits should be included for an accurate conversion?
        } else {
            uprv_decNumberToString(this->fDecNumber, s);
        }
        U_ASSERT(uprv_strlen(&s[0]) < MAX_DBL_DIGITS+18);
        
        loadDecimalChar();
        if (gDecimal != '.') {
            char *decimalPt = strchr(s, '.');
            if (decimalPt != NULL) {
                *decimalPt = gDecimal;
            }
        }
        char *end = NULL;
        nonConstThis->fDouble = uprv_strtod(s, &end);
    }
    nonConstThis->fHaveDouble = TRUE;
    return fDouble;
}

// -------------------------------------

/**
 *  convert this number to an int32_t.   Round if there is a fractional part.
 *  Return zero if the number cannot be represented.
 */
int32_t DigitList::getLong() /*const*/
{
    int32_t result = 0;
    if (fDecNumber->digits + fDecNumber->exponent > 10) {
        // Overflow, absolute value too big.
        return result;
    }
    if (fDecNumber->exponent != 0) {
        // Force to an integer, with zero exponent, rounding if necessary.
        //   (decNumberToInt32 will only work if the exponent is exactly zero.)
        DigitList copy(*this);
        DigitList zero;
        uprv_decNumberQuantize(copy.fDecNumber, copy.fDecNumber, zero.fDecNumber, &fContext);
        result = uprv_decNumberToInt32(copy.fDecNumber, &fContext);
    } else {
        result = uprv_decNumberToInt32(fDecNumber, &fContext);
    }
    return result;
}


/**
 *  convert this number to an int64_t.   Round if there is a fractional part.
 *  Return zero if the number cannot be represented.
 */
int64_t DigitList::getInt64() /*const*/ {
    // Round if non-integer.   (Truncate or round?)
    // Return 0 if out of range.
    // Range of in64_t is -9223372036854775808 to 9223372036854775807  (19 digits)
    //
    if (fDecNumber->digits + fDecNumber->exponent > 19) {
        // Overflow, absolute value too big.
        return 0;
    }
    decNumber *workingNum = fDecNumber;

    if (fDecNumber->exponent != 0) {
        // Force to an integer, with zero exponent, rounding if necessary.
        DigitList copy(*this);
        DigitList zero;
        uprv_decNumberQuantize(copy.fDecNumber, copy.fDecNumber, zero.fDecNumber, &fContext);
        workingNum = copy.fDecNumber;
    }

    uint64_t value = 0;
    int32_t numDigits = workingNum->digits;
    for (int i = numDigits-1; i>=0 ; --i) {
        int v = workingNum->lsu[i];
        value = value * (uint64_t)10 + (uint64_t)v;
    }
    if (decNumberIsNegative(workingNum)) {
        value = ~value;
        value += 1;
    }
    int64_t svalue = (int64_t)value;

    // Check overflow.  It's convenient that the MSD is 9 only on overflow, the amount of
    //                  overflow can't wrap too far.  The test will also fail -0, but
    //                  that does no harm; the right answer is 0.
    if (numDigits == 19) {
        if (( decNumberIsNegative(fDecNumber) && svalue>0) ||
            (!decNumberIsNegative(fDecNumber) && svalue<0)) {
            svalue = 0;
        }
    }
        
    return svalue;
}


/**
 *  Return a string form of this number.
 *     Format is as defined by the decNumber library, for interchange of
 *     decimal numbers.
 */
void DigitList::getDecimal(CharString &str, UErrorCode &status) {
    if (U_FAILURE(status)) {
        return;
    }

    // A decimal number in string form can, worst case, be 14 characters longer
    //  than the number of digits.  So says the decNumber library doc.
    int32_t maxLength = fDecNumber->digits + 14;
    int32_t capacity = 0;
    char *buffer = str.clear().getAppendBuffer(maxLength, 0, capacity, status);
    if (U_FAILURE(status)) {
        return;    // Memory allocation error on growing the string.
    }
    U_ASSERT(capacity >= maxLength);
    uprv_decNumberToString(this->fDecNumber, buffer);
    U_ASSERT((int32_t)uprv_strlen(buffer) <= maxLength);
    str.append(buffer, -1, status);
}

/**
 * Return true if this is an integer value that can be held
 * by an int32_t type.
 */
UBool
DigitList::fitsIntoLong(UBool ignoreNegativeZero) /*const*/
{
    if (decNumberIsSpecial(this->fDecNumber)) {
        // NaN or Infinity.  Does not fit in int32.
        return FALSE;
    }
    uprv_decNumberTrim(this->fDecNumber);
    if (fDecNumber->exponent < 0) {
        // Number contains fraction digits.
        return FALSE;
    }
    if (decNumberIsZero(this->fDecNumber) && !ignoreNegativeZero &&
        (fDecNumber->bits & DECNEG) != 0) {
        // Negative Zero, not ingored.  Cannot represent as a long.
        return FALSE;
    }
    if (fDecNumber->digits + fDecNumber->exponent < 10) {
        // The number is 9 or fewer digits.
        // The max and min int32 are 10 digts, so this number fits.
        // This is the common case.
        return TRUE;
    }

    // TODO:  Should cache these constants; construction is relatively costly.
    //        But not of huge consequence; they're only needed for 10 digit ints.
    UErrorCode status = U_ZERO_ERROR;
    DigitList min32; min32.set("-2147483648", status);
    if (this->compare(min32) < 0) {
        return FALSE;
    }
    DigitList max32; max32.set("2147483647", status);
    if (this->compare(max32) > 0) {
        return FALSE;
    }
    if (U_FAILURE(status)) {
        return FALSE;
    }
    return true;
}



/**
 * Return true if the number represented by this object can fit into
 * a long.
 */
UBool
DigitList::fitsIntoInt64(UBool ignoreNegativeZero) /*const*/
{
    if (decNumberIsSpecial(this->fDecNumber)) {
        // NaN or Infinity.  Does not fit in int32.
        return FALSE;
    }
    uprv_decNumberTrim(this->fDecNumber);
    if (fDecNumber->exponent < 0) {
        // Number contains fraction digits.
        return FALSE;
    }
    if (decNumberIsZero(this->fDecNumber) && !ignoreNegativeZero &&
        (fDecNumber->bits & DECNEG) != 0) {
        // Negative Zero, not ingored.  Cannot represent as a long.
        return FALSE;
    }
    if (fDecNumber->digits + fDecNumber->exponent < 19) {
        // The number is 18 or fewer digits.
        // The max and min int64 are 19 digts, so this number fits.
        // This is the common case.
        return TRUE;
    }

    // TODO:  Should cache these constants; construction is relatively costly.
    //        But not of huge consequence; they're only needed for 19 digit ints.
    UErrorCode status = U_ZERO_ERROR;
    DigitList min64; min64.set("-9223372036854775808", status);
    if (this->compare(min64) < 0) {
        return FALSE;
    }
    DigitList max64; max64.set("9223372036854775807", status);
    if (this->compare(max64) > 0) {
        return FALSE;
    }
    if (U_FAILURE(status)) {
        return FALSE;
    }
    return true;
}


// -------------------------------------

void
DigitList::set(int32_t source)
{
    set((int64_t)source);
    fDouble = source;
    fHaveDouble = TRUE;
}

// -------------------------------------
/**
 * @param maximumDigits The maximum digits to be generated.  If zero,
 * there is no maximum -- generate all digits.
 */
void
DigitList::set(int64_t source)
{
    char str[MAX_DIGITS+2];   // Leave room for sign and trailing nul.
    formatBase10(source, str);
    U_ASSERT(uprv_strlen(str) < sizeof(str));

    uprv_decNumberFromString(fDecNumber, str, &fContext);
    fDouble = (double)source;
    fHaveDouble = TRUE;
}


// -------------------------------------
/**
 * Set the DigitList from a decimal number string.
 *
 * The incoming string _must_ be nul terminated, even though it is arriving
 * as a StringPiece because that is what the decNumber library wants.
 * We can get away with this for an internal function; it would not
 * be acceptable for a public API.
 */
void
DigitList::set(const StringPiece &source, UErrorCode &status) {
    if (U_FAILURE(status)) {
        return;
    }

    // Figure out a max number of digits to use during the conversion, and
    // resize the number up if necessary.
    int32_t numDigits = source.length();
    if (numDigits > fContext.digits) {
        // fContext.digits == fStorage.getCapacity()
        decNumber *t = fStorage.resize(numDigits, fStorage.getCapacity());
        if (t == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return;
        }
        fDecNumber = t;
        fContext.digits = numDigits;
    }

    fContext.status = 0;
    uprv_decNumberFromString(fDecNumber, source.data(), &fContext);
    if ((fContext.status & DEC_Conversion_syntax) != 0) {
        status = U_DECIMAL_NUMBER_SYNTAX_ERROR;
    }
    fHaveDouble = FALSE;
}   

/**
 * Set the digit list to a representation of the given double value.
 * This method supports both fixed-point and exponential notation.
 * @param source Value to be converted.
 */
void
DigitList::set(double source)
{
    // for now, simple implementation; later, do proper IEEE stuff
    char rep[MAX_DIGITS + 8]; // Extra space for '+', '.', e+NNN, and '\0' (actually +8 is enough)

    // Generate a representation of the form /[+-][0-9]+e[+-][0-9]+/
    sprintf(rep, "%+1.*e", MAX_DBL_DIGITS - 1, source);
    U_ASSERT(uprv_strlen(rep) < sizeof(rep));

    // uprv_decNumberFromString() will parse the string expecting '.' as a
    // decimal separator, however sprintf() can use ',' in certain locales.
    // Overwrite a different decimal separator with '.' here before proceeding.
    loadDecimalChar();
    if (gDecimal != '.') {
        char *decimalPt = strchr(rep, gDecimal);
        if (decimalPt != NULL) {
            *decimalPt = '.';
        }
    }

    // Create a decNumber from the string.
    uprv_decNumberFromString(fDecNumber, rep, &fContext);
    uprv_decNumberTrim(fDecNumber);
    fDouble = source;
    fHaveDouble = TRUE;
}

// -------------------------------------

/*
 * Multiply
 *      The number will be expanded if need be to retain full precision.
 *      In practice, for formatting, multiply is by 10, 100 or 1000, so more digits
 *      will not be required for this use.
 */
void
DigitList::mult(const DigitList &other, UErrorCode &status) {
    fContext.status = 0;
    int32_t requiredDigits = this->digits() + other.digits();
    if (requiredDigits > fContext.digits) {
        reduce();    // Remove any trailing zeros
        int32_t requiredDigits = this->digits() + other.digits();
        ensureCapacity(requiredDigits, status);
    }
    uprv_decNumberMultiply(fDecNumber, fDecNumber, other.fDecNumber, &fContext);
    fHaveDouble = FALSE;
}

// -------------------------------------

/*
 * Divide
 *      The number will _not_ be expanded for inexact results.
 *      TODO:  probably should expand some, for rounding increments that
 *             could add a few digits, e.g. .25, but not expand arbitrarily.
 */
void
DigitList::div(const DigitList &other, UErrorCode &status) {
    if (U_FAILURE(status)) {
        return;
    }
    uprv_decNumberDivide(fDecNumber, fDecNumber, other.fDecNumber, &fContext);
    fHaveDouble = FALSE;
}

// -------------------------------------

/*
 * ensureCapacity.   Grow the digit storage for the number if it's less than the requested
 *         amount.  Never reduce it.  Available size is kept in fContext.digits.
 */
void
DigitList::ensureCapacity(int32_t requestedCapacity, UErrorCode &status) {
    if (U_FAILURE(status)) {
        return;
    }
    if (requestedCapacity <= 0) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }
    if (requestedCapacity > DEC_MAX_DIGITS) {
        // Don't report an error for requesting too much.
        // Arithemetic Results will be rounded to what can be supported.
        //   At 999,999,999 max digits, exceeding the limit is not too likely!
        requestedCapacity = DEC_MAX_DIGITS;
    }
    if (requestedCapacity > fContext.digits) {
        decNumber *newBuffer = fStorage.resize(requestedCapacity, fStorage.getCapacity());
        if (newBuffer == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return;
        }
        fContext.digits = requestedCapacity;
        fDecNumber = newBuffer;
    }
}

// -------------------------------------

/**
 * Round the representation to the given number of digits.
 * @param maximumDigits The maximum number of digits to be shown.
 * Upon return, count will be less than or equal to maximumDigits.
 */
void
DigitList::round(int32_t maximumDigits)
{
    int32_t savedDigits  = fContext.digits;
    fContext.digits = maximumDigits;
    uprv_decNumberPlus(fDecNumber, fDecNumber, &fContext);
    fContext.digits = savedDigits;
    uprv_decNumberTrim(fDecNumber);
    fHaveDouble = FALSE;
}


void
DigitList::roundFixedPoint(int32_t maximumFractionDigits) {
    trim();        // Remove trailing zeros.
    if (fDecNumber->exponent >= -maximumFractionDigits) {
        return;
    }
    decNumber scale;   // Dummy decimal number, but with the desired number of
    uprv_decNumberZero(&scale);    //    fraction digits.
    scale.exponent = -maximumFractionDigits;
    scale.lsu[0] = 1;
    
    uprv_decNumberQuantize(fDecNumber, fDecNumber, &scale, &fContext);
    trim();
    fHaveDouble = FALSE;
}

// -------------------------------------

void
DigitList::toIntegralValue() {
    uprv_decNumberToIntegralValue(fDecNumber, fDecNumber, &fContext);
}


// -------------------------------------
UBool
DigitList::isZero() const
{
    return decNumberIsZero(fDecNumber);
}


U_NAMESPACE_END
#endif // #if !UCONFIG_NO_FORMATTING

//eof
