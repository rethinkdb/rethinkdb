/*
**********************************************************************
*   Copyright (C) 1999-2009, International Business Machines
*   Corporation and others.  All Rights Reserved.
**********************************************************************
*   Date        Name        Description
*   11/17/99    aliu        Creation.
**********************************************************************
*/
#ifndef CPDTRANS_H
#define CPDTRANS_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_TRANSLITERATION

#include "unicode/translit.h"

U_NAMESPACE_BEGIN

class U_COMMON_API UVector;
class TransliteratorRegistry;

/**
 * A transliterator that is composed of two or more other
 * transliterator objects linked together.  For example, if one
 * transliterator transliterates from script A to script B, and
 * another transliterates from script B to script C, the two may be
 * combined to form a new transliterator from A to C.
 *
 * <p>Composed transliterators may not behave as expected.  For
 * example, inverses may not combine to form the identity
 * transliterator.  See the class documentation for {@link
 * Transliterator} for details.
 *
 * @author Alan Liu
 * @internal Use transliterator factory methods instead since this class will be removed in that release.
 */
class U_I18N_API CompoundTransliterator : public Transliterator {

    Transliterator** trans;

    int32_t count;

    int32_t numAnonymousRBTs;

public:

    /**
     * Constructs a new compound transliterator given an array of
     * transliterators.  The array of transliterators may be of any
     * length, including zero or one, however, useful compound
     * transliterators have at least two components.
     * @param transliterators array of <code>Transliterator</code>
     * objects
     * @param transliteratorCount The number of
     * <code>Transliterator</code> objects in transliterators.
     * @param adoptedFilter the filter.  Any character for which
     * <tt>filter.contains()</tt> returns <tt>false</tt> will not be
     * altered by this transliterator.  If <tt>filter</tt> is
     * <tt>null</tt> then no filtering is applied.
     * @internal Use transliterator factory methods instead since this class will be removed in that release.
     */
    CompoundTransliterator(Transliterator* const transliterators[],
                           int32_t transliteratorCount,
                           UnicodeFilter* adoptedFilter = 0);

    /**
     * Constructs a new compound transliterator.
     * @param id compound ID
     * @param dir either UTRANS_FORWARD or UTRANS_REVERSE
     * @param adoptedFilter a global filter for this compound transliterator
     * or NULL
     * @internal Use transliterator factory methods instead since this class will be removed in that release.
     */
    CompoundTransliterator(const UnicodeString& id,
                           UTransDirection dir,
                           UnicodeFilter* adoptedFilter,
                           UParseError& parseError,
                           UErrorCode& status);

    /**
     * Constructs a new compound transliterator in the FORWARD
     * direction with a NULL filter.
     * @internal Use transliterator factory methods instead since this class will be removed in that release.
     */
    CompoundTransliterator(const UnicodeString& id,
                           UParseError& parseError,
                           UErrorCode& status);
    /**
     * Destructor.
     * @internal Use transliterator factory methods instead since this class will be removed in that release.
     */
    virtual ~CompoundTransliterator();

    /**
     * Copy constructor.
     * @internal Use transliterator factory methods instead since this class will be removed in that release.
     */
    CompoundTransliterator(const CompoundTransliterator&);

    /**
     * Transliterator API.
     * @internal Use transliterator factory methods instead since this class will be removed in that release.
     */
    virtual Transliterator* clone(void) const;

    /**
     * Returns the number of transliterators in this chain.
     * @return number of transliterators in this chain.
     * @internal Use transliterator factory methods instead since this class will be removed in that release.
     */
    virtual int32_t getCount(void) const;

    /**
     * Returns the transliterator at the given index in this chain.
     * @param idx index into chain, from 0 to <code>getCount() - 1</code>
     * @return transliterator at the given index
     * @internal Use transliterator factory methods instead since this class will be removed in that release.
     */
    virtual const Transliterator& getTransliterator(int32_t idx) const;

    /**
     * Sets the transliterators.
     * @internal Use transliterator factory methods instead since this class will be removed in that release.
     */
    void setTransliterators(Transliterator* const transliterators[],
                            int32_t count);

    /**
     * Adopts the transliterators.
     * @internal Use transliterator factory methods instead since this class will be removed in that release.
     */
    void adoptTransliterators(Transliterator* adoptedTransliterators[],
                              int32_t count);

    /**
     * Override Transliterator:
     * Create a rule string that can be passed to createFromRules()
     * to recreate this transliterator.
     * @param result the string to receive the rules.  Previous
     * contents will be deleted.
     * @param escapeUnprintable if TRUE then convert unprintable
     * character to their hex escape representations, \uxxxx or
     * \Uxxxxxxxx.  Unprintable characters are those other than
     * U+000A, U+0020..U+007E.
     * @internal Use transliterator factory methods instead since this class will be removed in that release.
     */
    virtual UnicodeString& toRules(UnicodeString& result,
                                   UBool escapeUnprintable) const;

 protected:
    /**
     * Implement Transliterator framework
     */
    virtual void handleGetSourceSet(UnicodeSet& result) const;

 public:
    /**
     * Override Transliterator framework
     */
    virtual UnicodeSet& getTargetSet(UnicodeSet& result) const;

protected:
    /**
     * Implements {@link Transliterator#handleTransliterate}.
     * @internal Use transliterator factory methods instead since this class will be removed in that release.
     */
    virtual void handleTransliterate(Replaceable& text, UTransPosition& idx,
                                     UBool incremental) const;

public:

    /**
     * ICU "poor man's RTTI", returns a UClassID for the actual class.
     *
     * @draft ICU 2.2
     */
    virtual UClassID getDynamicClassID() const;

    /**
     * ICU "poor man's RTTI", returns a UClassID for this class.
     *
     * @draft ICU 2.2
     */
    static UClassID U_EXPORT2 getStaticClassID();

    /* @internal */
    static const UChar PASS_STRING[];

private:

    friend class Transliterator;
    friend class TransliteratorAlias; // to access private ct

    /**
     * Assignment operator.
     * @internal Use transliterator factory methods instead since this class will be removed in that release.
     */
    CompoundTransliterator& operator=(const CompoundTransliterator&);

    /**
     * Private constructor for Transliterator.
     */
    CompoundTransliterator(const UnicodeString& ID,
                           UVector& list,
                           UnicodeFilter* adoptedFilter,
                           int32_t numAnonymousRBTs,
                           UParseError& parseError,
                           UErrorCode& status);
    
    CompoundTransliterator(UVector& list,
                           UParseError& parseError,
                           UErrorCode& status);

    CompoundTransliterator(UVector& list,
                           int32_t anonymousRBTs,
                           UParseError& parseError,
                           UErrorCode& status);

    void init(const UnicodeString& id,
              UTransDirection direction,
              UBool fixReverseID,
              UErrorCode& status);

    void init(UVector& list,
              UTransDirection direction,
              UBool fixReverseID,
              UErrorCode& status);

    /**
     * Return the IDs of the given list of transliterators, concatenated
     * with ';' delimiting them.  Equivalent to the perlish expression
     * join(';', map($_.getID(), transliterators).
     */
    UnicodeString joinIDs(Transliterator* const transliterators[],
                          int32_t transCount);

    void freeTransliterators(void);

    void computeMaximumContextLength(void);
};

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_TRANSLITERATION */

#endif
