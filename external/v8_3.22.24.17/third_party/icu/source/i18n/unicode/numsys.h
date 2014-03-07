/*
*******************************************************************************
* Copyright (C) 2010, International Business Machines Corporation and
* others. All Rights Reserved.
*******************************************************************************
*
*
* File NUMSYS.H
*
* Modification History:*
*   Date        Name        Description
*
********************************************************************************
*/

#ifndef NUMSYS
#define NUMSYS

/**
 * \def NUMSYS_NAME_CAPACITY
 * Size of a numbering system name.
 * @internal
 */
#define NUMSYS_NAME_CAPACITY 8

#include "unicode/utypes.h"

/**
 * \file
 * \brief C++ API: NumberingSystem object
 */

#if !UCONFIG_NO_FORMATTING


#include "unicode/format.h"
#include "unicode/uobject.h"

U_NAMESPACE_BEGIN

/**
 * Defines numbering systems. A numbering system describes the scheme by which 
 * numbers are to be presented to the end user.  In its simplest form, a numbering
 * system describes the set of digit characters that are to be used to display
 * numbers, such as Western digits, Thai digits, Arabic-Indic digits, etc. 
 * More complicated numbering systems are algorithmic in nature, and require use
 * of an RBNF formatter ( rule based number formatter ), in order to calculate
 * the characters to be displayed for a given number.  Examples of algorithmic
 * numbering systems include Roman numerals, Chinese numerals, and Hebrew numerals.
 * Formatting rules for many commonly used numbering systems are included in
 * the ICU package, based on the numbering system rules defined in CLDR.
 * Alternate numbering systems can be specified to a locale by using the
 * numbers locale keyword.
 */

class U_I18N_API NumberingSystem : public UObject {
public:

    /**
     * Default Constructor.
     *
     * @stable ICU 4.2
     */
    NumberingSystem();

    /**
     * Copy constructor.
     * @stable ICU 4.2
     */
    NumberingSystem(const NumberingSystem& other);

    /**
     * Destructor.
     * @stable ICU 4.2
     */
    virtual ~NumberingSystem();

    /**
     * Create the default numbering system associated with the specified locale.
     * @param inLocale The given locale.
     * @param status ICU status
     * @stable ICU 4.2
     */
    static NumberingSystem* U_EXPORT2 createInstance(const Locale & inLocale, UErrorCode& status);

    /**
     * Create the default numbering system associated with the default locale.
     * @stable ICU 4.2
     */
    static NumberingSystem* U_EXPORT2 createInstance(UErrorCode& status);

    /**
     * Create a numbering system using the specified radix, type, and description. 
     * @param radix         The radix (base) for this numbering system.
     * @param isAlgorithmic TRUE if the numbering system is algorithmic rather than numeric.
     * @param description   The string representing the set of digits used in a numeric system, or the name of the RBNF
     *                      ruleset to be used in an algorithmic system.
     * @param status ICU status
     * @stable ICU 4.2
     */
    static NumberingSystem* U_EXPORT2 createInstance(int32_t radix, UBool isAlgorithmic, const UnicodeString& description, UErrorCode& status );

    /**
     * Return a StringEnumeration over all the names of numbering systems known to ICU.
     * @stable ICU 4.2
     */

     static StringEnumeration * U_EXPORT2 getAvailableNames(UErrorCode& status);

    /**
     * Create a numbering system from one of the predefined numbering systems known to ICU.
     * @param name   The name of the numbering system.
     * @param status ICU status
     * @stable ICU 4.2
     */
    static NumberingSystem* U_EXPORT2 createInstanceByName(const char* name, UErrorCode& status);


    /**
     * Returns the radix of this numbering system.
     * @stable ICU 4.2
     */
    int32_t getRadix();

    /**
     * Returns the name of this numbering system if it was created using one of the predefined names
     * known to ICU.  Otherwise, returns NULL.
     * @draft ICU 4.6
     */
    const char * getName();

    /**
     * Returns the description string of this numbering system, which is either
     * the string of digits in the case of simple systems, or the ruleset name
     * in the case of algorithmic systems.
     * @stable ICU 4.2
     */
    virtual UnicodeString getDescription();



    /**
     * Returns TRUE if the given numbering system is algorithmic
     *
     * @return         TRUE if the numbering system is algorithmic.
     *                 Otherwise, return FALSE.
     * @stable ICU 4.2
     */
    UBool isAlgorithmic() const;

    /**
     * ICU "poor man's RTTI", returns a UClassID for this class.
     *
     * @stable ICU 4.2
     *
    */
    static UClassID U_EXPORT2 getStaticClassID(void);

    /**
     * ICU "poor man's RTTI", returns a UClassID for the actual class.
     *
     * @stable ICU 4.2
     */
    virtual UClassID getDynamicClassID() const;


private:
    UnicodeString   desc;
    int32_t         radix;
    UBool           algorithmic;
    char            name[NUMSYS_NAME_CAPACITY+1];

    void setRadix(int32_t radix);

    void setAlgorithmic(UBool algorithmic);

    void setDesc(UnicodeString desc);

    void setName(const char* name);

    static UBool isValidDigitString(const UnicodeString &str);

    UBool hasContiguousDecimalDigits() const;
};

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */

#endif // _NUMSYS
//eof
