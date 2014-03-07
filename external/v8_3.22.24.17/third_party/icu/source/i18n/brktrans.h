/*
**********************************************************************
*   Copyright (C) 2008-2009, International Business Machines
*   Corporation and others.  All Rights Reserved.
**********************************************************************
*   Date        Name        Description
*   05/11/2008  Andy Heninger  Ported from Java
**********************************************************************
*/
#ifndef BRKTRANS_H
#define BRKTRANS_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_TRANSLITERATION && !UCONFIG_NO_BREAK_ITERATION

#include "unicode/translit.h"


U_NAMESPACE_BEGIN

class UVector32;

/**
 * A transliterator that pInserts the specified characters at word breaks.
 * To restrict it to particular characters, use a filter.
 * TODO: this is an internal class, and only temporary. 
 * Remove it once we have \b notation in Transliterator.
 */
class BreakTransliterator : public Transliterator {
public:

    BreakTransliterator(const UnicodeString &ID, 
                        UnicodeFilter *adoptedFilter,
                        BreakIterator *bi, 
                        const UnicodeString &insertion);
    /**
     * Constructs a transliterator.
     * @param adoptedFilter    the filter for this transliterator.
     */
    BreakTransliterator(UnicodeFilter* adoptedFilter = 0);

    /**
     * Destructor.
     */
    virtual ~BreakTransliterator();

    /**
     * Copy constructor.
     */
    BreakTransliterator(const BreakTransliterator&);

    /**
     * Transliterator API.
     * @return    A copy of the object.
     */
    virtual Transliterator* clone(void) const;

    virtual const UnicodeString &getInsertion() const;

    virtual void setInsertion(const UnicodeString &insertion);

    /**
      *  Return the break iterator used by this transliterator.
      *  Caution, this is the live break iterator; it must not be used while
      *     there is any possibility that this transliterator is using it.
      */
    virtual BreakIterator *getBreakIterator();


    /**
     * ICU "poor man's RTTI", returns a UClassID for the actual class.
     */
    virtual UClassID getDynamicClassID() const;

    /**
     * ICU "poor man's RTTI", returns a UClassID for this class.
     */
    U_I18N_API static UClassID U_EXPORT2 getStaticClassID();

 protected:

    /**
     * Implements {@link Transliterator#handleTransliterate}.
     * @param text          the buffer holding transliterated and
     *                      untransliterated text
     * @param offset        the start and limit of the text, the position
     *                      of the cursor, and the start and limit of transliteration.
     * @param incremental   if true, assume more text may be coming after
     *                      pos.contextLimit. Otherwise, assume the text is complete.
     */
    virtual void handleTransliterate(Replaceable& text, UTransPosition& offset,
                                     UBool isIncremental) const;

 private:
     BreakIterator     *bi;
     UnicodeString      fInsertion;
     UVector32         *boundaries;
     UnicodeString      sText;  // text from handleTransliterate().

     static UnicodeString replaceableAsString(Replaceable &r);

    /**
     * Assignment operator.
     */
    BreakTransliterator& operator=(const BreakTransliterator&);
};

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_TRANSLITERATION */

#endif
