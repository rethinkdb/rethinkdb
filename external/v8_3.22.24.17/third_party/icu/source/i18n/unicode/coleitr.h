/*
 ******************************************************************************
 *   Copyright (C) 1997-2008, International Business Machines
 *   Corporation and others.  All Rights Reserved.
 ******************************************************************************
 */

/**
 * \file 
 * \brief C++ API: Collation Element Iterator.
 */

/**
* File coleitr.h
*
* 
*
* Created by: Helena Shih
*
* Modification History:
*
*  Date       Name        Description
*
*  8/18/97    helena      Added internal API documentation.
* 08/03/98    erm         Synched with 1.2 version CollationElementIterator.java
* 12/10/99    aliu        Ported Thai collation support from Java.
* 01/25/01    swquek      Modified into a C++ wrapper calling C APIs (ucoliter.h)
* 02/19/01    swquek      Removed CollationElementsIterator() since it is 
*                         private constructor and no calls are made to it
*/

#ifndef COLEITR_H
#define COLEITR_H

#include "unicode/utypes.h"

 
#if !UCONFIG_NO_COLLATION

#include "unicode/uobject.h"
#include "unicode/tblcoll.h"
#include "unicode/ucoleitr.h"

/** 
 * The UCollationElements struct.
 * For usage in C programs.
 * @stable ICU 2.0
 */
typedef struct UCollationElements UCollationElements;

U_NAMESPACE_BEGIN

/**
* The CollationElementIterator class is used as an iterator to walk through     
* each character of an international string. Use the iterator to return the
* ordering priority of the positioned character. The ordering priority of a 
* character, which we refer to as a key, defines how a character is collated in 
* the given collation object.
* For example, consider the following in Spanish:
* <pre>
*        "ca" -> the first key is key('c') and second key is key('a').
*        "cha" -> the first key is key('ch') and second key is key('a').</pre>
* And in German,
* <pre> \htmlonly       "&#x00E6;b"-> the first key is key('a'), the second key is key('e'), and
*        the third key is key('b'). \endhtmlonly </pre>
* The key of a character, is an integer composed of primary order(short),
* secondary order(char), and tertiary order(char). Java strictly defines the 
* size and signedness of its primitive data types. Therefore, the static
* functions primaryOrder(), secondaryOrder(), and tertiaryOrder() return 
* int32_t to ensure the correctness of the key value.
* <p>Example of the iterator usage: (without error checking)
* <pre>
* \code
*   void CollationElementIterator_Example()
*   {
*       UnicodeString str = "This is a test";
*       UErrorCode success = U_ZERO_ERROR;
*       RuleBasedCollator* rbc =
*           (RuleBasedCollator*) RuleBasedCollator::createInstance(success);
*       CollationElementIterator* c =
*           rbc->createCollationElementIterator( str );
*       int32_t order = c->next(success);
*       c->reset();
*       order = c->previous(success);
*       delete c;
*       delete rbc;
*   }
* \endcode
* </pre>
* <p>
* CollationElementIterator::next returns the collation order of the next
* character based on the comparison level of the collator. 
* CollationElementIterator::previous returns the collation order of the 
* previous character based on the comparison level of the collator. 
* The Collation Element Iterator moves only in one direction between calls to
* CollationElementIterator::reset. That is, CollationElementIterator::next() 
* and CollationElementIterator::previous can not be inter-used. Whenever 
* CollationElementIterator::previous is to be called after 
* CollationElementIterator::next() or vice versa, 
* CollationElementIterator::reset has to be called first to reset the status, 
* shifting pointers to either the end or the start of the string. Hence at the 
* next call of CollationElementIterator::previous or 
* CollationElementIterator::next(), the first or last collation order will be 
* returned. 
* If a change of direction is done without a CollationElementIterator::reset(), 
* the result is undefined.
* The result of a forward iterate (CollationElementIterator::next) and 
* reversed result of the backward iterate (CollationElementIterator::previous) 
* on the same string are equivalent, if collation orders with the value 
* UCOL_IGNORABLE are ignored.
* Character based on the comparison level of the collator.  A collation order 
* consists of primary order, secondary order and tertiary order.  The data 
* type of the collation order is <strong>t_int32</strong>. 
*
* Note, CollationElementIterator should not be subclassed.
* @see     Collator
* @see     RuleBasedCollator
* @version 1.8 Jan 16 2001
*/
class U_I18N_API CollationElementIterator : public UObject {
public: 

    // CollationElementIterator public data member ------------------------------

    enum {
        /**
         * NULLORDER indicates that an error has occured while processing
         * @stable ICU 2.0
         */
        NULLORDER = (int32_t)0xffffffff
    };

    // CollationElementIterator public constructor/destructor -------------------

    /**
    * Copy constructor.
    *
    * @param other    the object to be copied from
    * @stable ICU 2.0
    */
    CollationElementIterator(const CollationElementIterator& other);

    /** 
    * Destructor
    * @stable ICU 2.0
    */
    virtual ~CollationElementIterator();

    // CollationElementIterator public methods ----------------------------------

    /**
    * Returns true if "other" is the same as "this"
    *
    * @param other    the object to be compared
    * @return         true if "other" is the same as "this"
    * @stable ICU 2.0
    */
    UBool operator==(const CollationElementIterator& other) const;

    /**
    * Returns true if "other" is not the same as "this".
    *
    * @param other    the object to be compared
    * @return         true if "other" is not the same as "this"
    * @stable ICU 2.0
    */
    UBool operator!=(const CollationElementIterator& other) const;

    /**
    * Resets the cursor to the beginning of the string.
    * @stable ICU 2.0
    */
    void reset(void);

    /**
    * Gets the ordering priority of the next character in the string.
    * @param status the error code status.
    * @return the next character's ordering. otherwise returns NULLORDER if an 
    *         error has occured or if the end of string has been reached
    * @stable ICU 2.0
    */
    int32_t next(UErrorCode& status);

    /**
    * Get the ordering priority of the previous collation element in the string.
    * @param status the error code status.
    * @return the previous element's ordering. otherwise returns NULLORDER if an 
    *         error has occured or if the start of string has been reached
    * @stable ICU 2.0
    */
    int32_t previous(UErrorCode& status);

    /**
    * Gets the primary order of a collation order.
    * @param order the collation order
    * @return the primary order of a collation order.
    * @stable ICU 2.0
    */
    static inline int32_t primaryOrder(int32_t order);

    /**
    * Gets the secondary order of a collation order.
    * @param order the collation order
    * @return the secondary order of a collation order.
    * @stable ICU 2.0
    */
    static inline int32_t secondaryOrder(int32_t order);

    /**
    * Gets the tertiary order of a collation order.
    * @param order the collation order
    * @return the tertiary order of a collation order.
    * @stable ICU 2.0
    */
    static inline int32_t tertiaryOrder(int32_t order);

    /**
    * Return the maximum length of any expansion sequences that end with the 
    * specified comparison order.
    * @param order a collation order returned by previous or next.
    * @return maximum size of the expansion sequences ending with the collation 
    *         element or 1 if collation element does not occur at the end of any 
    *         expansion sequence
    * @stable ICU 2.0
    */
    int32_t getMaxExpansion(int32_t order) const;

    /**
    * Gets the comparison order in the desired strength. Ignore the other
    * differences.
    * @param order The order value
    * @stable ICU 2.0
    */
    int32_t strengthOrder(int32_t order) const;

    /**
    * Sets the source string.
    * @param str the source string.
    * @param status the error code status.
    * @stable ICU 2.0
    */
    void setText(const UnicodeString& str, UErrorCode& status);

    /**
    * Sets the source string.
    * @param str the source character iterator.
    * @param status the error code status.
    * @stable ICU 2.0
    */
    void setText(CharacterIterator& str, UErrorCode& status);

    /**
    * Checks if a comparison order is ignorable.
    * @param order the collation order.
    * @return TRUE if a character is ignorable, FALSE otherwise.
    * @stable ICU 2.0
    */
    static inline UBool isIgnorable(int32_t order);

    /**
    * Gets the offset of the currently processed character in the source string.
    * @return the offset of the character.
    * @stable ICU 2.0
    */
    int32_t getOffset(void) const;

    /**
    * Sets the offset of the currently processed character in the source string.
    * @param newOffset the new offset.
    * @param status the error code status.
    * @return the offset of the character.
    * @stable ICU 2.0
    */
    void setOffset(int32_t newOffset, UErrorCode& status);

    /**
    * ICU "poor man's RTTI", returns a UClassID for the actual class.
    *
    * @stable ICU 2.2
    */
    virtual UClassID getDynamicClassID() const;

    /**
    * ICU "poor man's RTTI", returns a UClassID for this class.
    *
    * @stable ICU 2.2
    */
    static UClassID U_EXPORT2 getStaticClassID();

protected:
  
    // CollationElementIterator protected constructors --------------------------
    /**
    * @stable ICU 2.0
    */
    friend class RuleBasedCollator;

    /**
    * CollationElementIterator constructor. This takes the source string and the 
    * collation object. The cursor will walk thru the source string based on the 
    * predefined collation rules. If the source string is empty, NULLORDER will 
    * be returned on the calls to next().
    * @param sourceText    the source string.
    * @param order         the collation object.
    * @param status        the error code status.
    * @stable ICU 2.0
    */
    CollationElementIterator(const UnicodeString& sourceText,
        const RuleBasedCollator* order, UErrorCode& status);

    /**
    * CollationElementIterator constructor. This takes the source string and the 
    * collation object.  The cursor will walk thru the source string based on the 
    * predefined collation rules.  If the source string is empty, NULLORDER will 
    * be returned on the calls to next().
    * @param sourceText    the source string.
    * @param order         the collation object.
    * @param status        the error code status.
    * @stable ICU 2.0
    */
    CollationElementIterator(const CharacterIterator& sourceText,
        const RuleBasedCollator* order, UErrorCode& status);

    // CollationElementIterator protected methods -------------------------------

    /**
    * Assignment operator
    *
    * @param other    the object to be copied
    * @stable ICU 2.0
    */
    const CollationElementIterator&
        operator=(const CollationElementIterator& other);

private:
    CollationElementIterator(); // default constructor not implemented

    // CollationElementIterator private data members ----------------------------

    /**
    * Data wrapper for collation elements
    */
    UCollationElements *m_data_;

    /**
    * Indicates if m_data_ belongs to this object.
    */
    UBool isDataOwned_;

};

// CollationElementIterator inline method defination --------------------------

/**
* Get the primary order of a collation order.
* @param order the collation order
* @return the primary order of a collation order.
*/
inline int32_t CollationElementIterator::primaryOrder(int32_t order)
{
    order &= RuleBasedCollator::PRIMARYORDERMASK;
    return (order >> RuleBasedCollator::PRIMARYORDERSHIFT);
}

/**
* Get the secondary order of a collation order.
* @param order the collation order
* @return the secondary order of a collation order.
*/
inline int32_t CollationElementIterator::secondaryOrder(int32_t order)
{
    order = order & RuleBasedCollator::SECONDARYORDERMASK;
    return (order >> RuleBasedCollator::SECONDARYORDERSHIFT);
}

/**
* Get the tertiary order of a collation order.
* @param order the collation order
* @return the tertiary order of a collation order.
*/
inline int32_t CollationElementIterator::tertiaryOrder(int32_t order)
{
    return (order &= RuleBasedCollator::TERTIARYORDERMASK);
}

inline int32_t CollationElementIterator::getMaxExpansion(int32_t order) const
{
    return ucol_getMaxExpansion(m_data_, (uint32_t)order);
}

inline UBool CollationElementIterator::isIgnorable(int32_t order)
{
    return (primaryOrder(order) == RuleBasedCollator::PRIMIGNORABLE);
}

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_COLLATION */

#endif
