/*
*******************************************************************************
* Copyright (C) 2007-2010, International Business Machines Corporation and
* others. All Rights Reserved.
*******************************************************************************
*
* File DTPTNGEN.H
*
*******************************************************************************
*/

#ifndef __DTPTNGEN_H__
#define __DTPTNGEN_H__

#include "unicode/datefmt.h"
#include "unicode/locid.h"
#include "unicode/udat.h"
#include "unicode/udatpg.h"

U_NAMESPACE_BEGIN

/**
 * \file
 * \brief C++ API: Date/Time Pattern Generator
 */


class Hashtable;
class FormatParser;
class DateTimeMatcher;
class DistanceInfo;
class PatternMap;
class PtnSkeleton;

/**
 * This class provides flexible generation of date format patterns, like "yy-MM-dd". 
 * The user can build up the generator by adding successive patterns. Once that 
 * is done, a query can be made using a "skeleton", which is a pattern which just
 * includes the desired fields and lengths. The generator will return the "best fit" 
 * pattern corresponding to that skeleton.
 * <p>The main method people will use is getBestPattern(String skeleton),
 * since normally this class is pre-built with data from a particular locale. 
 * However, generators can be built directly from other data as well.
 * <p><i>Issue: may be useful to also have a function that returns the list of 
 * fields in a pattern, in order, since we have that internally.
 * That would be useful for getting the UI order of field elements.</i>
 * @stable ICU 3.8
**/
class U_I18N_API DateTimePatternGenerator : public UObject {
public:
    /**
     * Construct a flexible generator according to default locale.
     * @param status  Output param set to success/failure code on exit,
     *               which must not indicate a failure before the function call.
     * @stable ICU 3.8
     */
    static DateTimePatternGenerator* U_EXPORT2 createInstance(UErrorCode& status);

    /**
     * Construct a flexible generator according to data for a given locale.
     * @param uLocale
     * @param status  Output param set to success/failure code on exit,
     *               which must not indicate a failure before the function call.
     * @stable ICU 3.8
     */
    static DateTimePatternGenerator* U_EXPORT2 createInstance(const Locale& uLocale, UErrorCode& status);

    /**
     * Create an empty generator, to be constructed with addPattern(...) etc.
     * @param status  Output param set to success/failure code on exit,
     *               which must not indicate a failure before the function call.
     * @stable ICU 3.8
     */
     static DateTimePatternGenerator* U_EXPORT2 createEmptyInstance(UErrorCode& status);
     
    /**
     * Destructor.
     * @stable ICU 3.8
     */
    virtual ~DateTimePatternGenerator();

    /**
     * Clone DateTimePatternGenerator object. Clients are responsible for 
     * deleting the DateTimePatternGenerator object cloned.
     * @stable ICU 3.8
     */
    DateTimePatternGenerator* clone() const;

     /**
      * Return true if another object is semantically equal to this one.
      *
      * @param other    the DateTimePatternGenerator object to be compared with.
      * @return         true if other is semantically equal to this.
      * @stable ICU 3.8
      */
    UBool operator==(const DateTimePatternGenerator& other) const;
    
    /**
     * Return true if another object is semantically unequal to this one.
     *
     * @param other    the DateTimePatternGenerator object to be compared with.
     * @return         true if other is semantically unequal to this.
     * @stable ICU 3.8
     */
    UBool operator!=(const DateTimePatternGenerator& other) const;

    /**
     * Utility to return a unique skeleton from a given pattern. For example,
     * both "MMM-dd" and "dd/MMM" produce the skeleton "MMMdd".
     *
     * @param pattern   Input pattern, such as "dd/MMM"
     * @param status  Output param set to success/failure code on exit,
     *                  which must not indicate a failure before the function call.
     * @return skeleton such as "MMMdd"
     * @stable ICU 3.8
     */
    UnicodeString getSkeleton(const UnicodeString& pattern, UErrorCode& status);

    /**
     * Utility to return a unique base skeleton from a given pattern. This is
     * the same as the skeleton, except that differences in length are minimized
     * so as to only preserve the difference between string and numeric form. So
     * for example, both "MMM-dd" and "d/MMM" produce the skeleton "MMMd"
     * (notice the single d).
     *
     * @param pattern  Input pattern, such as "dd/MMM"
     * @param status  Output param set to success/failure code on exit,
     *               which must not indicate a failure before the function call.
     * @return base skeleton, such as "Md"
     * @stable ICU 3.8
     */
    UnicodeString getBaseSkeleton(const UnicodeString& pattern, UErrorCode& status);

    /**
     * Adds a pattern to the generator. If the pattern has the same skeleton as
     * an existing pattern, and the override parameter is set, then the previous
     * value is overriden. Otherwise, the previous value is retained. In either
     * case, the conflicting status is set and previous vale is stored in 
     * conflicting pattern.
     * <p>
     * Note that single-field patterns (like "MMM") are automatically added, and
     * don't need to be added explicitly!
     *
     * @param pattern   Input pattern, such as "dd/MMM"
     * @param override  When existing values are to be overridden use true, 
     *                   otherwise use false.
     * @param conflictingPattern  Previous pattern with the same skeleton.
     * @param status  Output param set to success/failure code on exit,
     *               which must not indicate a failure before the function call.
     * @return conflicting status.  The value could be UDATPG_NO_CONFLICT, 
     *                             UDATPG_BASE_CONFLICT or UDATPG_CONFLICT.
     * @stable ICU 3.8
     */
    UDateTimePatternConflict addPattern(const UnicodeString& pattern, 
                                        UBool override, 
                                        UnicodeString& conflictingPattern,
                                        UErrorCode& status);

    /**
     * An AppendItem format is a pattern used to append a field if there is no
     * good match. For example, suppose that the input skeleton is "GyyyyMMMd",
     * and there is no matching pattern internally, but there is a pattern
     * matching "yyyyMMMd", say "d-MM-yyyy". Then that pattern is used, plus the
     * G. The way these two are conjoined is by using the AppendItemFormat for G
     * (era). So if that value is, say "{0}, {1}" then the final resulting
     * pattern is "d-MM-yyyy, G".
     * <p>
     * There are actually three available variables: {0} is the pattern so far,
     * {1} is the element we are adding, and {2} is the name of the element.
     * <p>
     * This reflects the way that the CLDR data is organized.
     *
     * @param field  such as UDATPG_ERA_FIELD.
     * @param value  pattern, such as "{0}, {1}"
     * @stable ICU 3.8
     */
    void setAppendItemFormat(UDateTimePatternField field, const UnicodeString& value);

    /**
     * Getter corresponding to setAppendItemFormat. Values below 0 or at or
     * above UDATPG_FIELD_COUNT are illegal arguments.
     *
     * @param  field  such as UDATPG_ERA_FIELD.
     * @return append pattern for field
     * @stable ICU 3.8
     */
    const UnicodeString& getAppendItemFormat(UDateTimePatternField field) const;

    /**
     * Sets the names of field, eg "era" in English for ERA. These are only
     * used if the corresponding AppendItemFormat is used, and if it contains a
     * {2} variable.
     * <p>
     * This reflects the way that the CLDR data is organized.
     *
     * @param field   such as UDATPG_ERA_FIELD.
     * @param value   name of the field
     * @stable ICU 3.8
     */
    void setAppendItemName(UDateTimePatternField field, const UnicodeString& value);

    /**
     * Getter corresponding to setAppendItemNames. Values below 0 or at or above
     * UDATPG_FIELD_COUNT are illegal arguments.
     *
     * @param field  such as UDATPG_ERA_FIELD.
     * @return name for field
     * @stable ICU 3.8
     */
    const UnicodeString& getAppendItemName(UDateTimePatternField field) const;

    /**
     * The date time format is a message format pattern used to compose date and
     * time patterns. The default value is "{0} {1}", where {0} will be replaced
     * by the date pattern and {1} will be replaced by the time pattern.
     * <p>
     * This is used when the input skeleton contains both date and time fields,
     * but there is not a close match among the added patterns. For example,
     * suppose that this object was created by adding "dd-MMM" and "hh:mm", and
     * its datetimeFormat is the default "{0} {1}". Then if the input skeleton
     * is "MMMdhmm", there is not an exact match, so the input skeleton is
     * broken up into two components "MMMd" and "hmm". There are close matches
     * for those two skeletons, so the result is put together with this pattern,
     * resulting in "d-MMM h:mm".
     *
     * @param dateTimeFormat
     *            message format pattern, here {0} will be replaced by the date
     *            pattern and {1} will be replaced by the time pattern.
     * @stable ICU 3.8
     */
    void setDateTimeFormat(const UnicodeString& dateTimeFormat);

    /**
     * Getter corresponding to setDateTimeFormat.
     * @return DateTimeFormat.
     * @stable ICU 3.8
     */
    const UnicodeString& getDateTimeFormat() const;

    /**
     * Return the best pattern matching the input skeleton. It is guaranteed to
     * have all of the fields in the skeleton.
     *
     * @param skeleton
     *            The skeleton is a pattern containing only the variable fields.
     *            For example, "MMMdd" and "mmhh" are skeletons.
     * @param status  Output param set to success/failure code on exit,
     *               which must not indicate a failure before the function call.
     * @return bestPattern
     *            The best pattern found from the given skeleton.
     * @stable ICU 3.8
     */
     UnicodeString getBestPattern(const UnicodeString& skeleton, UErrorCode& status);


    /**
     * Return the best pattern matching the input skeleton. It is guaranteed to
     * have all of the fields in the skeleton.
     *
     * @param skeleton
     *            The skeleton is a pattern containing only the variable fields.
     *            For example, "MMMdd" and "mmhh" are skeletons.
     * @param options
     *            Options for forcing the length of specified fields in the
     *            returned pattern to match those in the skeleton (when this
     *            would not happen otherwise). For default behavior, use
     *            UDATPG_MATCH_NO_OPTIONS.
     * @param status
     *            Output param set to success/failure code on exit,
     *            which must not indicate a failure before the function call.
     * @return bestPattern
     *            The best pattern found from the given skeleton.
     * @stable ICU 4.4
     */
     UnicodeString getBestPattern(const UnicodeString& skeleton,
                                  UDateTimePatternMatchOptions options,
                                  UErrorCode& status);


    /**
     * Adjusts the field types (width and subtype) of a pattern to match what is
     * in a skeleton. That is, if you supply a pattern like "d-M H:m", and a
     * skeleton of "MMMMddhhmm", then the input pattern is adjusted to be
     * "dd-MMMM hh:mm". This is used internally to get the best match for the
     * input skeleton, but can also be used externally.
     *
     * @param pattern Input pattern
     * @param skeleton
     *            The skeleton is a pattern containing only the variable fields.
     *            For example, "MMMdd" and "mmhh" are skeletons.
     * @param status  Output param set to success/failure code on exit,
     *               which must not indicate a failure before the function call.
     * @return pattern adjusted to match the skeleton fields widths and subtypes.
     * @stable ICU 3.8
     */
     UnicodeString replaceFieldTypes(const UnicodeString& pattern, 
                                     const UnicodeString& skeleton, 
                                     UErrorCode& status);

    /**
     * Adjusts the field types (width and subtype) of a pattern to match what is
     * in a skeleton. That is, if you supply a pattern like "d-M H:m", and a
     * skeleton of "MMMMddhhmm", then the input pattern is adjusted to be
     * "dd-MMMM hh:mm". This is used internally to get the best match for the
     * input skeleton, but can also be used externally.
     *
     * @param pattern Input pattern
     * @param skeleton
     *            The skeleton is a pattern containing only the variable fields.
     *            For example, "MMMdd" and "mmhh" are skeletons.
     * @param options
     *            Options controlling whether the length of specified fields in the
     *            pattern are adjusted to match those in the skeleton (when this
     *            would not happen otherwise). For default behavior, use
     *            UDATPG_MATCH_NO_OPTIONS.
     * @param status
     *            Output param set to success/failure code on exit,
     *            which must not indicate a failure before the function call.
     * @return pattern adjusted to match the skeleton fields widths and subtypes.
     * @stable ICU 4.4
     */
     UnicodeString replaceFieldTypes(const UnicodeString& pattern, 
                                     const UnicodeString& skeleton, 
                                     UDateTimePatternMatchOptions options,
                                     UErrorCode& status);

    /**
     * Return a list of all the skeletons (in canonical form) from this class.
     *
     * Call getPatternForSkeleton() to get the corresponding pattern.
     *
     * @param status  Output param set to success/failure code on exit,
     *               which must not indicate a failure before the function call.
     * @return StringEnumeration with the skeletons.
     *         The caller must delete the object.
     * @stable ICU 3.8
     */
     StringEnumeration* getSkeletons(UErrorCode& status) const;

     /**
      * Get the pattern corresponding to a given skeleton.
      * @param skeleton 
      * @return pattern corresponding to a given skeleton.
      * @stable ICU 3.8
      */
     const UnicodeString& getPatternForSkeleton(const UnicodeString& skeleton) const;
     
    /**
     * Return a list of all the base skeletons (in canonical form) from this class.
     *
     * @param status  Output param set to success/failure code on exit,
     *               which must not indicate a failure before the function call.
     * @return a StringEnumeration with the base skeletons.
     *         The caller must delete the object.
     * @stable ICU 3.8
     */
     StringEnumeration* getBaseSkeletons(UErrorCode& status) const;
     
     /**
      * Return a list of redundant patterns are those which if removed, make no 
      * difference in the resulting getBestPattern values. This method returns a 
      * list of them, to help check the consistency of the patterns used to build 
      * this generator.
      * 
      * @param status  Output param set to success/failure code on exit,
      *               which must not indicate a failure before the function call.
      * @return a StringEnumeration with the redundant pattern.
      *         The caller must delete the object.
      * @internal ICU 3.8
      */
     StringEnumeration* getRedundants(UErrorCode& status);
      
    /**
     * The decimal value is used in formatting fractions of seconds. If the
     * skeleton contains fractional seconds, then this is used with the
     * fractional seconds. For example, suppose that the input pattern is
     * "hhmmssSSSS", and the best matching pattern internally is "H:mm:ss", and
     * the decimal string is ",". Then the resulting pattern is modified to be
     * "H:mm:ss,SSSS"
     *
     * @param decimal 
     * @stable ICU 3.8
     */
    void setDecimal(const UnicodeString& decimal);

    /**
     * Getter corresponding to setDecimal.
     * @return UnicodeString corresponding to the decimal point
     * @stable ICU 3.8
     */
    const UnicodeString& getDecimal() const;

    /**
     * ICU "poor man's RTTI", returns a UClassID for the actual class.
     *
     * @stable ICU 3.8
     */
    virtual UClassID getDynamicClassID() const;

    /**
     * ICU "poor man's RTTI", returns a UClassID for this class.
     *
     * @stable ICU 3.8
     */
    static UClassID U_EXPORT2 getStaticClassID(void);

private:
    /**
     * Constructor.
     * @stable ICU 3.8
     */
    DateTimePatternGenerator(UErrorCode & status);

    /**
     * Constructor.
     * @stable ICU 3.8
     */
    DateTimePatternGenerator(const Locale& locale, UErrorCode & status);

    /**
     * Copy constructor.
     * @param other DateTimePatternGenerator to copy
     * @stable ICU 3.8
     */
    DateTimePatternGenerator(const DateTimePatternGenerator& other);

    /**
     * Default assignment operator.
     * @param other DateTimePatternGenerator to copy
     * @stable ICU 3.8
     */
    DateTimePatternGenerator& operator=(const DateTimePatternGenerator& other);

    Locale pLocale;  // pattern locale
    FormatParser *fp;
    DateTimeMatcher* dtMatcher;
    DistanceInfo *distanceInfo;
    PatternMap *patternMap;
    UnicodeString appendItemFormats[UDATPG_FIELD_COUNT];
    UnicodeString appendItemNames[UDATPG_FIELD_COUNT];
    UnicodeString dateTimeFormat;
    UnicodeString decimal;
    DateTimeMatcher *skipMatcher;
    Hashtable *fAvailableFormatKeyHash;
    UnicodeString hackPattern;
    UnicodeString emptyString;
    UChar fDefaultHourFormatChar;

    void initData(const Locale &locale, UErrorCode &status);
    void addCanonicalItems();
    void addICUPatterns(const Locale& locale, UErrorCode& status);
    void hackTimes(const UnicodeString& hackPattern, UErrorCode& status);
    void addCLDRData(const Locale& locale, UErrorCode& status);
    UDateTimePatternConflict addPatternWithSkeleton(const UnicodeString& pattern, const UnicodeString * skeletonToUse, UBool override, UnicodeString& conflictingPattern, UErrorCode& status);
    void initHashtable(UErrorCode& status);
    void setDateTimeFromCalendar(const Locale& locale, UErrorCode& status);
    void setDecimalSymbols(const Locale& locale, UErrorCode& status);
    UDateTimePatternField getAppendFormatNumber(const char* field) const;
    UDateTimePatternField getAppendNameNumber(const char* field) const;
    void getAppendName(UDateTimePatternField field, UnicodeString& value);
    int32_t getCanonicalIndex(const UnicodeString& field);
    const UnicodeString* getBestRaw(DateTimeMatcher& source, int32_t includeMask, DistanceInfo* missingFields, const PtnSkeleton** specifiedSkeletonPtr = 0);
    UnicodeString adjustFieldTypes(const UnicodeString& pattern, const PtnSkeleton* specifiedSkeleton, UBool fixFractionalSeconds, UDateTimePatternMatchOptions options = UDATPG_MATCH_NO_OPTIONS);
    UnicodeString getBestAppending(int32_t missingFields, UDateTimePatternMatchOptions options = UDATPG_MATCH_NO_OPTIONS);
    int32_t getTopBitNumber(int32_t foundMask);
    void setAvailableFormat(const UnicodeString &key, UErrorCode& status);
    UBool isAvailableFormatSet(const UnicodeString &key) const;
    void copyHashtable(Hashtable *other, UErrorCode &status);
    UBool isCanonicalItem(const UnicodeString& item) const;
} ;// end class DateTimePatternGenerator

U_NAMESPACE_END

#endif
