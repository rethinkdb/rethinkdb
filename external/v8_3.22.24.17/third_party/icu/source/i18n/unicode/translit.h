/*
**********************************************************************
* Copyright (C) 1999-2008, International Business Machines
* Corporation and others. All Rights Reserved.
**********************************************************************
*   Date        Name        Description
*   11/17/99    aliu        Creation.
**********************************************************************
*/
#ifndef TRANSLIT_H
#define TRANSLIT_H

#include "unicode/utypes.h"

/**
 * \file 
 * \brief C++ API: Tranforms text from one format to another.
 */
 
#if !UCONFIG_NO_TRANSLITERATION

#include "unicode/uobject.h"
#include "unicode/unistr.h"
#include "unicode/parseerr.h"
#include "unicode/utrans.h" // UTransPosition, UTransDirection
#include "unicode/strenum.h"

U_NAMESPACE_BEGIN

class UnicodeFilter;
class UnicodeSet;
class CompoundTransliterator;
class TransliteratorParser;
class NormalizationTransliterator;
class TransliteratorIDParser;

/**
 *
 * <code>Transliterator</code> is an abstract class that
 * transliterates text from one format to another.  The most common
 * kind of transliterator is a script, or alphabet, transliterator.
 * For example, a Russian to Latin transliterator changes Russian text
 * written in Cyrillic characters to phonetically equivalent Latin
 * characters.  It does not <em>translate</em> Russian to English!
 * Transliteration, unlike translation, operates on characters, without
 * reference to the meanings of words and sentences.
 *
 * <p>Although script conversion is its most common use, a
 * transliterator can actually perform a more general class of tasks.
 * In fact, <code>Transliterator</code> defines a very general API
 * which specifies only that a segment of the input text is replaced
 * by new text.  The particulars of this conversion are determined
 * entirely by subclasses of <code>Transliterator</code>.
 *
 * <p><b>Transliterators are stateless</b>
 *
 * <p><code>Transliterator</code> objects are <em>stateless</em>; they
 * retain no information between calls to
 * <code>transliterate()</code>.  (However, this does <em>not</em>
 * mean that threads may share transliterators without synchronizing
 * them.  Transliterators are not immutable, so they must be
 * synchronized when shared between threads.)  This might seem to
 * limit the complexity of the transliteration operation.  In
 * practice, subclasses perform complex transliterations by delaying
 * the replacement of text until it is known that no other
 * replacements are possible.  In other words, although the
 * <code>Transliterator</code> objects are stateless, the source text
 * itself embodies all the needed information, and delayed operation
 * allows arbitrary complexity.
 *
 * <p><b>Batch transliteration</b>
 *
 * <p>The simplest way to perform transliteration is all at once, on a
 * string of existing text.  This is referred to as <em>batch</em>
 * transliteration.  For example, given a string <code>input</code>
 * and a transliterator <code>t</code>, the call
 *
 * \htmlonly<blockquote>\endhtmlonly<code>String result = t.transliterate(input);
 * </code>\htmlonly</blockquote>\endhtmlonly
 *
 * will transliterate it and return the result.  Other methods allow
 * the client to specify a substring to be transliterated and to use
 * {@link Replaceable } objects instead of strings, in order to
 * preserve out-of-band information (such as text styles).
 *
 * <p><b>Keyboard transliteration</b>
 *
 * <p>Somewhat more involved is <em>keyboard</em>, or incremental
 * transliteration.  This is the transliteration of text that is
 * arriving from some source (typically the user's keyboard) one
 * character at a time, or in some other piecemeal fashion.
 *
 * <p>In keyboard transliteration, a <code>Replaceable</code> buffer
 * stores the text.  As text is inserted, as much as possible is
 * transliterated on the fly.  This means a GUI that displays the
 * contents of the buffer may show text being modified as each new
 * character arrives.
 *
 * <p>Consider the simple <code>RuleBasedTransliterator</code>:
 *
 * \htmlonly<blockquote>\endhtmlonly<code>
 * th&gt;{theta}<br>
 * t&gt;{tau}
 * </code>\htmlonly</blockquote>\endhtmlonly
 *
 * When the user types 't', nothing will happen, since the
 * transliterator is waiting to see if the next character is 'h'.  To
 * remedy this, we introduce the notion of a cursor, marked by a '|'
 * in the output string:
 *
 * \htmlonly<blockquote>\endhtmlonly<code>
 * t&gt;|{tau}<br>
 * {tau}h&gt;{theta}
 * </code>\htmlonly</blockquote>\endhtmlonly
 *
 * Now when the user types 't', tau appears, and if the next character
 * is 'h', the tau changes to a theta.  This is accomplished by
 * maintaining a cursor position (independent of the insertion point,
 * and invisible in the GUI) across calls to
 * <code>transliterate()</code>.  Typically, the cursor will
 * be coincident with the insertion point, but in a case like the one
 * above, it will precede the insertion point.
 *
 * <p>Keyboard transliteration methods maintain a set of three indices
 * that are updated with each call to
 * <code>transliterate()</code>, including the cursor, start,
 * and limit.  Since these indices are changed by the method, they are
 * passed in an <code>int[]</code> array. The <code>START</code> index
 * marks the beginning of the substring that the transliterator will
 * look at.  It is advanced as text becomes committed (but it is not
 * the committed index; that's the <code>CURSOR</code>).  The
 * <code>CURSOR</code> index, described above, marks the point at
 * which the transliterator last stopped, either because it reached
 * the end, or because it required more characters to disambiguate
 * between possible inputs.  The <code>CURSOR</code> can also be
 * explicitly set by rules in a <code>RuleBasedTransliterator</code>.
 * Any characters before the <code>CURSOR</code> index are frozen;
 * future keyboard transliteration calls within this input sequence
 * will not change them.  New text is inserted at the
 * <code>LIMIT</code> index, which marks the end of the substring that
 * the transliterator looks at.
 *
 * <p>Because keyboard transliteration assumes that more characters
 * are to arrive, it is conservative in its operation.  It only
 * transliterates when it can do so unambiguously.  Otherwise it waits
 * for more characters to arrive.  When the client code knows that no
 * more characters are forthcoming, perhaps because the user has
 * performed some input termination operation, then it should call
 * <code>finishTransliteration()</code> to complete any
 * pending transliterations.
 *
 * <p><b>Inverses</b>
 *
 * <p>Pairs of transliterators may be inverses of one another.  For
 * example, if transliterator <b>A</b> transliterates characters by
 * incrementing their Unicode value (so "abc" -> "def"), and
 * transliterator <b>B</b> decrements character values, then <b>A</b>
 * is an inverse of <b>B</b> and vice versa.  If we compose <b>A</b>
 * with <b>B</b> in a compound transliterator, the result is the
 * indentity transliterator, that is, a transliterator that does not
 * change its input text.
 *
 * The <code>Transliterator</code> method <code>getInverse()</code>
 * returns a transliterator's inverse, if one exists, or
 * <code>null</code> otherwise.  However, the result of
 * <code>getInverse()</code> usually will <em>not</em> be a true
 * mathematical inverse.  This is because true inverse transliterators
 * are difficult to formulate.  For example, consider two
 * transliterators: <b>AB</b>, which transliterates the character 'A'
 * to 'B', and <b>BA</b>, which transliterates 'B' to 'A'.  It might
 * seem that these are exact inverses, since
 *
 * \htmlonly<blockquote>\endhtmlonly"A" x <b>AB</b> -> "B"<br>
 * "B" x <b>BA</b> -> "A"\htmlonly</blockquote>\endhtmlonly
 *
 * where 'x' represents transliteration.  However,
 *
 * \htmlonly<blockquote>\endhtmlonly"ABCD" x <b>AB</b> -> "BBCD"<br>
 * "BBCD" x <b>BA</b> -> "AACD"\htmlonly</blockquote>\endhtmlonly
 *
 * so <b>AB</b> composed with <b>BA</b> is not the
 * identity. Nonetheless, <b>BA</b> may be usefully considered to be
 * <b>AB</b>'s inverse, and it is on this basis that
 * <b>AB</b><code>.getInverse()</code> could legitimately return
 * <b>BA</b>.
 *
 * <p><b>IDs and display names</b>
 *
 * <p>A transliterator is designated by a short identifier string or
 * <em>ID</em>.  IDs follow the format <em>source-destination</em>,
 * where <em>source</em> describes the entity being replaced, and
 * <em>destination</em> describes the entity replacing
 * <em>source</em>.  The entities may be the names of scripts,
 * particular sequences of characters, or whatever else it is that the
 * transliterator converts to or from.  For example, a transliterator
 * from Russian to Latin might be named "Russian-Latin".  A
 * transliterator from keyboard escape sequences to Latin-1 characters
 * might be named "KeyboardEscape-Latin1".  By convention, system
 * entity names are in English, with the initial letters of words
 * capitalized; user entity names may follow any format so long as
 * they do not contain dashes.
 *
 * <p>In addition to programmatic IDs, transliterator objects have
 * display names for presentation in user interfaces, returned by
 * {@link #getDisplayName }.
 *
 * <p><b>Factory methods and registration</b>
 *
 * <p>In general, client code should use the factory method
 * {@link #createInstance } to obtain an instance of a
 * transliterator given its ID.  Valid IDs may be enumerated using
 * <code>getAvailableIDs()</code>.  Since transliterators are mutable,
 * multiple calls to {@link #createInstance } with the same ID will
 * return distinct objects.
 *
 * <p>In addition to the system transliterators registered at startup,
 * user transliterators may be registered by calling
 * <code>registerInstance()</code> at run time.  A registered instance
 * acts a template; future calls to {@link #createInstance } with the ID
 * of the registered object return clones of that object.  Thus any
 * object passed to <tt>registerInstance()</tt> must implement
 * <tt>clone()</tt> propertly.  To register a transliterator subclass
 * without instantiating it (until it is needed), users may call
 * {@link #registerFactory }.  In this case, the objects are
 * instantiated by invoking the zero-argument public constructor of
 * the class.
 *
 * <p><b>Subclassing</b>
 *
 * Subclasses must implement the abstract method
 * <code>handleTransliterate()</code>.  <p>Subclasses should override
 * the <code>transliterate()</code> method taking a
 * <code>Replaceable</code> and the <code>transliterate()</code>
 * method taking a <code>String</code> and <code>StringBuffer</code>
 * if the performance of these methods can be improved over the
 * performance obtained by the default implementations in this class.
 *
 * @author Alan Liu
 * @stable ICU 2.0
 */
class U_I18N_API Transliterator : public UObject {

private:

    /**
     * Programmatic name, e.g., "Latin-Arabic".
     */
    UnicodeString ID;

    /**
     * This transliterator's filter.  Any character for which
     * <tt>filter.contains()</tt> returns <tt>false</tt> will not be
     * altered by this transliterator.  If <tt>filter</tt> is
     * <tt>null</tt> then no filtering is applied.
     */
    UnicodeFilter* filter;

    int32_t maximumContextLength;

 public:

    /**
     * A context integer or pointer for a factory function, passed by
     * value.
     * @stable ICU 2.4
     */
    union Token {
        /**
         * This token, interpreted as a 32-bit integer.
         * @stable ICU 2.4
         */
        int32_t integer;
        /**
         * This token, interpreted as a native pointer.
         * @stable ICU 2.4
         */
        void*   pointer;
    };

    /**
     * Return a token containing an integer.
     * @return a token containing an integer.
     * @internal
     */
    inline static Token integerToken(int32_t);

    /**
     * Return a token containing a pointer.
     * @return a token containing a pointer.
     * @internal
     */
    inline static Token pointerToken(void*);

    /**
     * A function that creates and returns a Transliterator.  When
     * invoked, it will be passed the ID string that is being
     * instantiated, together with the context pointer that was passed
     * in when the factory function was first registered.  Many
     * factory functions will ignore both parameters, however,
     * functions that are registered to more than one ID may use the
     * ID or the context parameter to parameterize the transliterator
     * they create.
     * @param ID      the string identifier for this transliterator
     * @param context a context pointer that will be stored and
     *                later passed to the factory function when an ID matching
     *                the registration ID is being instantiated with this factory.
     * @stable ICU 2.4
     */
    typedef Transliterator* (U_EXPORT2 *Factory)(const UnicodeString& ID, Token context);

protected:

    /**
     * Default constructor.
     * @param ID the string identifier for this transliterator
     * @param adoptedFilter the filter.  Any character for which
     * <tt>filter.contains()</tt> returns <tt>false</tt> will not be
     * altered by this transliterator.  If <tt>filter</tt> is
     * <tt>null</tt> then no filtering is applied.
     * @stable ICU 2.4
     */
    Transliterator(const UnicodeString& ID, UnicodeFilter* adoptedFilter);

    /**
     * Copy constructor.
     * @stable ICU 2.4
     */
    Transliterator(const Transliterator&);

    /**
     * Assignment operator.
     * @stable ICU 2.4
     */
    Transliterator& operator=(const Transliterator&);

    /**
     * Create a transliterator from a basic ID.  This is an ID
     * containing only the forward direction source, target, and
     * variant.
     * @param id a basic ID of the form S-T or S-T/V.
     * @param canon canonical ID to assign to the object, or
     * NULL to leave the ID unchanged
     * @return a newly created Transliterator or null if the ID is
     * invalid.
     * @stable ICU 2.4
     */
    static Transliterator* createBasicInstance(const UnicodeString& id,
                                               const UnicodeString* canon);

    friend class TransliteratorParser; // for parseID()
    friend class TransliteratorIDParser; // for createBasicInstance()
    friend class TransliteratorAlias; // for setID()

public:

    /**
     * Destructor.
     * @stable ICU 2.0
     */
    virtual ~Transliterator();

    /**
     * Implements Cloneable.
     * All subclasses are encouraged to implement this method if it is
     * possible and reasonable to do so.  Subclasses that are to be
     * registered with the system using <tt>registerInstance()</tt>
     * are required to implement this method.  If a subclass does not
     * implement clone() properly and is registered with the system
     * using registerInstance(), then the default clone() implementation
     * will return null, and calls to createInstance() will fail.
     *
     * @return a copy of the object.
     * @see #registerInstance
     * @stable ICU 2.0
     */
    virtual Transliterator* clone() const;

    /**
     * Transliterates a segment of a string, with optional filtering.
     *
     * @param text the string to be transliterated
     * @param start the beginning index, inclusive; <code>0 <= start
     * <= limit</code>.
     * @param limit the ending index, exclusive; <code>start <= limit
     * <= text.length()</code>.
     * @return The new limit index.  The text previously occupying <code>[start,
     * limit)</code> has been transliterated, possibly to a string of a different
     * length, at <code>[start, </code><em>new-limit</em><code>)</code>, where
     * <em>new-limit</em> is the return value. If the input offsets are out of bounds,
     * the returned value is -1 and the input string remains unchanged.
     * @stable ICU 2.0
     */
    virtual int32_t transliterate(Replaceable& text,
                                  int32_t start, int32_t limit) const;

    /**
     * Transliterates an entire string in place. Convenience method.
     * @param text the string to be transliterated
     * @stable ICU 2.0
     */
    virtual void transliterate(Replaceable& text) const;

    /**
     * Transliterates the portion of the text buffer that can be
     * transliterated unambiguosly after new text has been inserted,
     * typically as a result of a keyboard event.  The new text in
     * <code>insertion</code> will be inserted into <code>text</code>
     * at <code>index.limit</code>, advancing
     * <code>index.limit</code> by <code>insertion.length()</code>.
     * Then the transliterator will try to transliterate characters of
     * <code>text</code> between <code>index.cursor</code> and
     * <code>index.limit</code>.  Characters before
     * <code>index.cursor</code> will not be changed.
     *
     * <p>Upon return, values in <code>index</code> will be updated.
     * <code>index.start</code> will be advanced to the first
     * character that future calls to this method will read.
     * <code>index.cursor</code> and <code>index.limit</code> will
     * be adjusted to delimit the range of text that future calls to
     * this method may change.
     *
     * <p>Typical usage of this method begins with an initial call
     * with <code>index.start</code> and <code>index.limit</code>
     * set to indicate the portion of <code>text</code> to be
     * transliterated, and <code>index.cursor == index.start</code>.
     * Thereafter, <code>index</code> can be used without
     * modification in future calls, provided that all changes to
     * <code>text</code> are made via this method.
     *
     * <p>This method assumes that future calls may be made that will
     * insert new text into the buffer.  As a result, it only performs
     * unambiguous transliterations.  After the last call to this
     * method, there may be untransliterated text that is waiting for
     * more input to resolve an ambiguity.  In order to perform these
     * pending transliterations, clients should call {@link
     * #finishTransliteration } after the last call to this
     * method has been made.
     *
     * @param text the buffer holding transliterated and untransliterated text
     * @param index an array of three integers.
     *
     * <ul><li><code>index.start</code>: the beginning index,
     * inclusive; <code>0 <= index.start <= index.limit</code>.
     *
     * <li><code>index.limit</code>: the ending index, exclusive;
     * <code>index.start <= index.limit <= text.length()</code>.
     * <code>insertion</code> is inserted at
     * <code>index.limit</code>.
     *
     * <li><code>index.cursor</code>: the next character to be
     * considered for transliteration; <code>index.start <=
     * index.cursor <= index.limit</code>.  Characters before
     * <code>index.cursor</code> will not be changed by future calls
     * to this method.</ul>
     *
     * @param insertion text to be inserted and possibly
     * transliterated into the translation buffer at
     * <code>index.limit</code>.  If <code>null</code> then no text
     * is inserted.
     * @param status    Output param to filled in with a success or an error.
     * @see #handleTransliterate
     * @exception IllegalArgumentException if <code>index</code>
     * is invalid
     * @see UTransPosition
     * @stable ICU 2.0
     */
    virtual void transliterate(Replaceable& text, UTransPosition& index,
                               const UnicodeString& insertion,
                               UErrorCode& status) const;

    /**
     * Transliterates the portion of the text buffer that can be
     * transliterated unambiguosly after a new character has been
     * inserted, typically as a result of a keyboard event.  This is a
     * convenience method.
     * @param text the buffer holding transliterated and
     * untransliterated text
     * @param index an array of three integers.
     * @param insertion text to be inserted and possibly
     * transliterated into the translation buffer at
     * <code>index.limit</code>.
     * @param status    Output param to filled in with a success or an error.
     * @see #transliterate(Replaceable&, UTransPosition&, const UnicodeString&, UErrorCode&) const
     * @stable ICU 2.0
     */
    virtual void transliterate(Replaceable& text, UTransPosition& index,
                               UChar32 insertion,
                               UErrorCode& status) const;

    /**
     * Transliterates the portion of the text buffer that can be
     * transliterated unambiguosly.  This is a convenience method; see
     * {@link
     * #transliterate(Replaceable&, UTransPosition&, const UnicodeString&, UErrorCode&) const }
     * for details.
     * @param text the buffer holding transliterated and
     * untransliterated text
     * @param index an array of three integers.  See {@link
     * #transliterate(Replaceable&, UTransPosition&, const UnicodeString&, UErrorCode&) const }.
     * @param status    Output param to filled in with a success or an error.
     * @see #transliterate(Replaceable, int[], String)
     * @stable ICU 2.0
     */
    virtual void transliterate(Replaceable& text, UTransPosition& index,
                               UErrorCode& status) const;

    /**
     * Finishes any pending transliterations that were waiting for
     * more characters.  Clients should call this method as the last
     * call after a sequence of one or more calls to
     * <code>transliterate()</code>.
     * @param text the buffer holding transliterated and
     * untransliterated text.
     * @param index the array of indices previously passed to {@link
     * #transliterate }
     * @stable ICU 2.0
     */
    virtual void finishTransliteration(Replaceable& text,
                                       UTransPosition& index) const;

private:

    /**
     * This internal method does incremental transliteration.  If the
     * 'insertion' is non-null then we append it to 'text' before
     * proceeding.  This method calls through to the pure virtual
     * framework method handleTransliterate() to do the actual
     * work.
     * @param text the buffer holding transliterated and
     * untransliterated text
     * @param index an array of three integers.  See {@link
     * #transliterate(Replaceable, int[], String)}.
     * @param insertion text to be inserted and possibly
     * transliterated into the translation buffer at
     * <code>index.limit</code>.
     * @param status    Output param to filled in with a success or an error.
     */
    void _transliterate(Replaceable& text,
                        UTransPosition& index,
                        const UnicodeString* insertion,
                        UErrorCode &status) const;

protected:

    /**
     * Abstract method that concrete subclasses define to implement
     * their transliteration algorithm.  This method handles both
     * incremental and non-incremental transliteration.  Let
     * <code>originalStart</code> refer to the value of
     * <code>pos.start</code> upon entry.
     *
     * <ul>
     *  <li>If <code>incremental</code> is false, then this method
     *  should transliterate all characters between
     *  <code>pos.start</code> and <code>pos.limit</code>. Upon return
     *  <code>pos.start</code> must == <code> pos.limit</code>.</li>
     *
     *  <li>If <code>incremental</code> is true, then this method
     *  should transliterate all characters between
     *  <code>pos.start</code> and <code>pos.limit</code> that can be
     *  unambiguously transliterated, regardless of future insertions
     *  of text at <code>pos.limit</code>.  Upon return,
     *  <code>pos.start</code> should be in the range
     *  [<code>originalStart</code>, <code>pos.limit</code>).
     *  <code>pos.start</code> should be positioned such that
     *  characters [<code>originalStart</code>, <code>
     *  pos.start</code>) will not be changed in the future by this
     *  transliterator and characters [<code>pos.start</code>,
     *  <code>pos.limit</code>) are unchanged.</li>
     * </ul>
     *
     * <p>Implementations of this method should also obey the
     * following invariants:</p>
     *
     * <ul>
     *  <li> <code>pos.limit</code> and <code>pos.contextLimit</code>
     *  should be updated to reflect changes in length of the text
     *  between <code>pos.start</code> and <code>pos.limit</code>. The
     *  difference <code> pos.contextLimit - pos.limit</code> should
     *  not change.</li>
     *
     *  <li><code>pos.contextStart</code> should not change.</li>
     *
     *  <li>Upon return, neither <code>pos.start</code> nor
     *  <code>pos.limit</code> should be less than
     *  <code>originalStart</code>.</li>
     *
     *  <li>Text before <code>originalStart</code> and text after
     *  <code>pos.limit</code> should not change.</li>
     *
     *  <li>Text before <code>pos.contextStart</code> and text after
     *  <code> pos.contextLimit</code> should be ignored.</li>
     * </ul>
     *
     * <p>Subclasses may safely assume that all characters in
     * [<code>pos.start</code>, <code>pos.limit</code>) are filtered.
     * In other words, the filter has already been applied by the time
     * this method is called.  See
     * <code>filteredTransliterate()</code>.
     *
     * <p>This method is <b>not</b> for public consumption.  Calling
     * this method directly will transliterate
     * [<code>pos.start</code>, <code>pos.limit</code>) without
     * applying the filter. End user code should call <code>
     * transliterate()</code> instead of this method. Subclass code
     * and wrapping transliterators should call
     * <code>filteredTransliterate()</code> instead of this method.<p>
     *
     * @param text the buffer holding transliterated and
     * untransliterated text
     *
     * @param pos the indices indicating the start, limit, context
     * start, and context limit of the text.
     *
     * @param incremental if true, assume more text may be inserted at
     * <code>pos.limit</code> and act accordingly.  Otherwise,
     * transliterate all text between <code>pos.start</code> and
     * <code>pos.limit</code> and move <code>pos.start</code> up to
     * <code>pos.limit</code>.
     *
     * @see #transliterate
     * @stable ICU 2.4
     */
    virtual void handleTransliterate(Replaceable& text,
                                     UTransPosition& pos,
                                     UBool incremental) const = 0;

public:
    /**
     * Transliterate a substring of text, as specified by index, taking filters
     * into account.  This method is for subclasses that need to delegate to
     * another transliterator, such as CompoundTransliterator.
     * @param text the text to be transliterated
     * @param index the position indices
     * @param incremental if TRUE, then assume more characters may be inserted
     * at index.limit, and postpone processing to accomodate future incoming
     * characters
     * @stable ICU 2.4
     */
    virtual void filteredTransliterate(Replaceable& text,
                                       UTransPosition& index,
                                       UBool incremental) const;

private:

    /**
     * Top-level transliteration method, handling filtering, incremental and
     * non-incremental transliteration, and rollback.  All transliteration
     * public API methods eventually call this method with a rollback argument
     * of TRUE.  Other entities may call this method but rollback should be
     * FALSE.
     *
     * <p>If this transliterator has a filter, break up the input text into runs
     * of unfiltered characters.  Pass each run to
     * <subclass>.handleTransliterate().
     *
     * <p>In incremental mode, if rollback is TRUE, perform a special
     * incremental procedure in which several passes are made over the input
     * text, adding one character at a time, and committing successful
     * transliterations as they occur.  Unsuccessful transliterations are rolled
     * back and retried with additional characters to give correct results.
     *
     * @param text the text to be transliterated
     * @param index the position indices
     * @param incremental if TRUE, then assume more characters may be inserted
     * at index.limit, and postpone processing to accomodate future incoming
     * characters
     * @param rollback if TRUE and if incremental is TRUE, then perform special
     * incremental processing, as described above, and undo partial
     * transliterations where necessary.  If incremental is FALSE then this
     * parameter is ignored.
     */
    virtual void filteredTransliterate(Replaceable& text,
                                       UTransPosition& index,
                                       UBool incremental,
                                       UBool rollback) const;

public:

    /**
     * Returns the length of the longest context required by this transliterator.
     * This is <em>preceding</em> context.  The default implementation supplied
     * by <code>Transliterator</code> returns zero; subclasses
     * that use preceding context should override this method to return the
     * correct value.  For example, if a transliterator translates "ddd" (where
     * d is any digit) to "555" when preceded by "(ddd)", then the preceding
     * context length is 5, the length of "(ddd)".
     *
     * @return The maximum number of preceding context characters this
     * transliterator needs to examine
     * @stable ICU 2.0
     */
    int32_t getMaximumContextLength(void) const;

protected:

    /**
     * Method for subclasses to use to set the maximum context length.
     * @param maxContextLength the new value to be set.
     * @see #getMaximumContextLength
     * @stable ICU 2.4
     */
    void setMaximumContextLength(int32_t maxContextLength);

public:

    /**
     * Returns a programmatic identifier for this transliterator.
     * If this identifier is passed to <code>createInstance()</code>, it
     * will return this object, if it has been registered.
     * @return a programmatic identifier for this transliterator.
     * @see #registerInstance
     * @see #registerFactory
     * @see #getAvailableIDs
     * @stable ICU 2.0
     */
    virtual const UnicodeString& getID(void) const;

    /**
     * Returns a name for this transliterator that is appropriate for
     * display to the user in the default locale.  See {@link
     * #getDisplayName } for details.
     * @param ID     the string identifier for this transliterator
     * @param result Output param to receive the display name
     * @return       A reference to 'result'.
     * @stable ICU 2.0
     */
    static UnicodeString& U_EXPORT2 getDisplayName(const UnicodeString& ID,
                                         UnicodeString& result);

    /**
     * Returns a name for this transliterator that is appropriate for
     * display to the user in the given locale.  This name is taken
     * from the locale resource data in the standard manner of the
     * <code>java.text</code> package.
     *
     * <p>If no localized names exist in the system resource bundles,
     * a name is synthesized using a localized
     * <code>MessageFormat</code> pattern from the resource data.  The
     * arguments to this pattern are an integer followed by one or two
     * strings.  The integer is the number of strings, either 1 or 2.
     * The strings are formed by splitting the ID for this
     * transliterator at the first '-'.  If there is no '-', then the
     * entire ID forms the only string.
     * @param ID       the string identifier for this transliterator
     * @param inLocale the Locale in which the display name should be
     *                 localized.
     * @param result   Output param to receive the display name
     * @return         A reference to 'result'.
     * @stable ICU 2.0
     */
    static UnicodeString& U_EXPORT2 getDisplayName(const UnicodeString& ID,
                                         const Locale& inLocale,
                                         UnicodeString& result);

    /**
     * Returns the filter used by this transliterator, or <tt>NULL</tt>
     * if this transliterator uses no filter.
     * @return the filter used by this transliterator, or <tt>NULL</tt>
     *         if this transliterator uses no filter.
     * @stable ICU 2.0
     */
    const UnicodeFilter* getFilter(void) const;

    /**
     * Returns the filter used by this transliterator, or <tt>NULL</tt> if this
     * transliterator uses no filter.  The caller must eventually delete the
     * result.  After this call, this transliterator's filter is set to
     * <tt>NULL</tt>.
     * @return the filter used by this transliterator, or <tt>NULL</tt> if this
     *         transliterator uses no filter.
     * @stable ICU 2.4
     */
    UnicodeFilter* orphanFilter(void);

    /**
     * Changes the filter used by this transliterator.  If the filter
     * is set to <tt>null</tt> then no filtering will occur.
     *
     * <p>Callers must take care if a transliterator is in use by
     * multiple threads.  The filter should not be changed by one
     * thread while another thread may be transliterating.
     * @param adoptedFilter the new filter to be adopted.
     * @stable ICU 2.0
     */
    void adoptFilter(UnicodeFilter* adoptedFilter);

    /**
     * Returns this transliterator's inverse.  See the class
     * documentation for details.  This implementation simply inverts
     * the two entities in the ID and attempts to retrieve the
     * resulting transliterator.  That is, if <code>getID()</code>
     * returns "A-B", then this method will return the result of
     * <code>createInstance("B-A")</code>, or <code>null</code> if that
     * call fails.
     *
     * <p>Subclasses with knowledge of their inverse may wish to
     * override this method.
     *
     * @param status Output param to filled in with a success or an error.
     * @return a transliterator that is an inverse, not necessarily
     * exact, of this transliterator, or <code>null</code> if no such
     * transliterator is registered.
     * @see #registerInstance
     * @stable ICU 2.0
     */
    Transliterator* createInverse(UErrorCode& status) const;

    /**
     * Returns a <code>Transliterator</code> object given its ID.
     * The ID must be either a system transliterator ID or a ID registered
     * using <code>registerInstance()</code>.
     *
     * @param ID a valid ID, as enumerated by <code>getAvailableIDs()</code>
     * @param dir        either FORWARD or REVERSE.
     * @param parseError Struct to recieve information on position
     *                   of error if an error is encountered
     * @param status     Output param to filled in with a success or an error.
     * @return A <code>Transliterator</code> object with the given ID
     * @see #registerInstance
     * @see #getAvailableIDs
     * @see #getID
     * @stable ICU 2.0
     */
    static Transliterator* U_EXPORT2 createInstance(const UnicodeString& ID,
                                          UTransDirection dir,
                                          UParseError& parseError,
                                          UErrorCode& status);

    /**
     * Returns a <code>Transliterator</code> object given its ID.
     * The ID must be either a system transliterator ID or a ID registered
     * using <code>registerInstance()</code>.
     * @param ID a valid ID, as enumerated by <code>getAvailableIDs()</code>
     * @param dir        either FORWARD or REVERSE.
     * @param status     Output param to filled in with a success or an error.
     * @return A <code>Transliterator</code> object with the given ID
     * @stable ICU 2.0
     */
    static Transliterator* U_EXPORT2 createInstance(const UnicodeString& ID,
                                          UTransDirection dir,
                                          UErrorCode& status);

    /**
     * Returns a <code>Transliterator</code> object constructed from
     * the given rule string.  This will be a RuleBasedTransliterator,
     * if the rule string contains only rules, or a
     * CompoundTransliterator, if it contains ID blocks, or a
     * NullTransliterator, if it contains ID blocks which parse as
     * empty for the given direction.
     * @param ID            the id for the transliterator.
     * @param rules         rules, separated by ';'
     * @param dir           either FORWARD or REVERSE.
     * @param parseError    Struct to recieve information on position
     *                      of error if an error is encountered
     * @param status        Output param set to success/failure code.
     * @stable ICU 2.0
     */
    static Transliterator* U_EXPORT2 createFromRules(const UnicodeString& ID,
                                           const UnicodeString& rules,
                                           UTransDirection dir,
                                           UParseError& parseError,
                                           UErrorCode& status);

    /**
     * Create a rule string that can be passed to createFromRules()
     * to recreate this transliterator.
     * @param result the string to receive the rules.  Previous
     * contents will be deleted.
     * @param escapeUnprintable if TRUE then convert unprintable
     * character to their hex escape representations, \\uxxxx or
     * \\Uxxxxxxxx.  Unprintable characters are those other than
     * U+000A, U+0020..U+007E.
     * @stable ICU 2.0
     */
    virtual UnicodeString& toRules(UnicodeString& result,
                                   UBool escapeUnprintable) const;

    /**
     * Return the number of elements that make up this transliterator.
     * For example, if the transliterator "NFD;Jamo-Latin;Latin-Greek"
     * were created, the return value of this method would be 3.
     *
     * <p>If this transliterator is not composed of other
     * transliterators, then this method returns 1.
     * @return the number of transliterators that compose this
     * transliterator, or 1 if this transliterator is not composed of
     * multiple transliterators
     * @stable ICU 3.0
     */
    int32_t countElements() const;

    /**
     * Return an element that makes up this transliterator.  For
     * example, if the transliterator "NFD;Jamo-Latin;Latin-Greek"
     * were created, the return value of this method would be one
     * of the three transliterator objects that make up that
     * transliterator: [NFD, Jamo-Latin, Latin-Greek].
     *
     * <p>If this transliterator is not composed of other
     * transliterators, then this method will return a reference to
     * this transliterator when given the index 0.
     * @param index a value from 0..countElements()-1 indicating the
     * transliterator to return
     * @param ec input-output error code
     * @return one of the transliterators that makes up this
     * transliterator, if this transliterator is made up of multiple
     * transliterators, otherwise a reference to this object if given
     * an index of 0
     * @stable ICU 3.0
     */
    const Transliterator& getElement(int32_t index, UErrorCode& ec) const;

    /**
     * Returns the set of all characters that may be modified in the
     * input text by this Transliterator.  This incorporates this
     * object's current filter; if the filter is changed, the return
     * value of this function will change.  The default implementation
     * returns an empty set.  Some subclasses may override {@link
     * #handleGetSourceSet } to return a more precise result.  The
     * return result is approximate in any case and is intended for
     * use by tests, tools, or utilities.
     * @param result receives result set; previous contents lost
     * @return a reference to result
     * @see #getTargetSet
     * @see #handleGetSourceSet
     * @stable ICU 2.4
     */
    UnicodeSet& getSourceSet(UnicodeSet& result) const;

    /**
     * Framework method that returns the set of all characters that
     * may be modified in the input text by this Transliterator,
     * ignoring the effect of this object's filter.  The base class
     * implementation returns the empty set.  Subclasses that wish to
     * implement this should override this method.
     * @return the set of characters that this transliterator may
     * modify.  The set may be modified, so subclasses should return a
     * newly-created object.
     * @param result receives result set; previous contents lost
     * @see #getSourceSet
     * @see #getTargetSet
     * @stable ICU 2.4
     */
    virtual void handleGetSourceSet(UnicodeSet& result) const;

    /**
     * Returns the set of all characters that may be generated as
     * replacement text by this transliterator.  The default
     * implementation returns the empty set.  Some subclasses may
     * override this method to return a more precise result.  The
     * return result is approximate in any case and is intended for
     * use by tests, tools, or utilities requiring such
     * meta-information.
     * @param result receives result set; previous contents lost
     * @return a reference to result
     * @see #getTargetSet
     * @stable ICU 2.4
     */
    virtual UnicodeSet& getTargetSet(UnicodeSet& result) const;

public:

    /**
     * Registers a factory function that creates transliterators of
     * a given ID.
     * @param id the ID being registered
     * @param factory a function pointer that will be copied and
     * called later when the given ID is passed to createInstance()
     * @param context a context pointer that will be stored and
     * later passed to the factory function when an ID matching
     * the registration ID is being instantiated with this factory.
     * @stable ICU 2.0
     */
    static void U_EXPORT2 registerFactory(const UnicodeString& id,
                                Factory factory,
                                Token context);

    /**
     * Registers an instance <tt>obj</tt> of a subclass of
     * <code>Transliterator</code> with the system.  When
     * <tt>createInstance()</tt> is called with an ID string that is
     * equal to <tt>obj->getID()</tt>, then <tt>obj->clone()</tt> is
     * returned.
     *
     * After this call the Transliterator class owns the adoptedObj
     * and will delete it.
     *
     * @param adoptedObj an instance of subclass of
     * <code>Transliterator</code> that defines <tt>clone()</tt>
     * @see #createInstance
     * @see #registerFactory
     * @see #unregister
     * @stable ICU 2.0
     */
    static void U_EXPORT2 registerInstance(Transliterator* adoptedObj);

    /**
     * Registers an ID string as an alias of another ID string.
     * That is, after calling this function, <tt>createInstance(aliasID)</tt>
     * will return the same thing as <tt>createInstance(realID)</tt>.
     * This is generally used to create shorter, more mnemonic aliases
     * for long compound IDs.
     *
     * @param aliasID The new ID being registered.
     * @param realID The ID that the new ID is to be an alias for.
     * This can be a compound ID and can include filters and should
     * refer to transliterators that have already been registered with
     * the framework, although this isn't checked.
     * @stable ICU 3.6
     */
     static void U_EXPORT2 registerAlias(const UnicodeString& aliasID,
                                         const UnicodeString& realID);

protected:

    /**
     * @internal
     * @param id the ID being registered
     * @param factory a function pointer that will be copied and
     * called later when the given ID is passed to createInstance()
     * @param context a context pointer that will be stored and
     * later passed to the factory function when an ID matching
     * the registration ID is being instantiated with this factory.
     */
    static void _registerFactory(const UnicodeString& id,
                                 Factory factory,
                                 Token context);

    /**
     * @internal
     */
    static void _registerInstance(Transliterator* adoptedObj);

    /**
     * @internal
     */
    static void _registerAlias(const UnicodeString& aliasID, const UnicodeString& realID);

    /**
     * Register two targets as being inverses of one another.  For
     * example, calling registerSpecialInverse("NFC", "NFD", true) causes
     * Transliterator to form the following inverse relationships:
     *
     * <pre>NFC => NFD
     * Any-NFC => Any-NFD
     * NFD => NFC
     * Any-NFD => Any-NFC</pre>
     *
     * (Without the special inverse registration, the inverse of NFC
     * would be NFC-Any.)  Note that NFD is shorthand for Any-NFD, but
     * that the presence or absence of "Any-" is preserved.
     *
     * <p>The relationship is symmetrical; registering (a, b) is
     * equivalent to registering (b, a).
     *
     * <p>The relevant IDs must still be registered separately as
     * factories or classes.
     *
     * <p>Only the targets are specified.  Special inverses always
     * have the form Any-Target1 <=> Any-Target2.  The target should
     * have canonical casing (the casing desired to be produced when
     * an inverse is formed) and should contain no whitespace or other
     * extraneous characters.
     *
     * @param target the target against which to register the inverse
     * @param inverseTarget the inverse of target, that is
     * Any-target.getInverse() => Any-inverseTarget
     * @param bidirectional if true, register the reverse relation
     * as well, that is, Any-inverseTarget.getInverse() => Any-target
     * @internal
     */
    static void _registerSpecialInverse(const UnicodeString& target,
                                        const UnicodeString& inverseTarget,
                                        UBool bidirectional);

public:

    /**
     * Unregisters a transliterator or class.  This may be either
     * a system transliterator or a user transliterator or class.
     * Any attempt to construct an unregistered transliterator based
     * on its ID will fail.
     *
     * @param ID the ID of the transliterator or class
     * @return the <code>Object</code> that was registered with
     * <code>ID</code>, or <code>null</code> if none was
     * @see #registerInstance
     * @see #registerFactory
     * @stable ICU 2.0
     */
    static void U_EXPORT2 unregister(const UnicodeString& ID);

public:

    /**
     * Return a StringEnumeration over the IDs available at the time of the
     * call, including user-registered IDs.
     * @param ec input-output error code
     * @return a newly-created StringEnumeration over the transliterators
     * available at the time of the call. The caller should delete this object
     * when done using it.
     * @stable ICU 3.0
     */
    static StringEnumeration* U_EXPORT2 getAvailableIDs(UErrorCode& ec);

    /**
     * Return the number of registered source specifiers.
     * @return the number of registered source specifiers.
     * @stable ICU 2.0
     */
    static int32_t U_EXPORT2 countAvailableSources(void);

    /**
     * Return a registered source specifier.
     * @param index which specifier to return, from 0 to n-1, where
     * n = countAvailableSources()
     * @param result fill-in paramter to receive the source specifier.
     * If index is out of range, result will be empty.
     * @return reference to result
     * @stable ICU 2.0
     */
    static UnicodeString& U_EXPORT2 getAvailableSource(int32_t index,
                                             UnicodeString& result);

    /**
     * Return the number of registered target specifiers for a given
     * source specifier.
     * @param source the given source specifier.
     * @return the number of registered target specifiers for a given
     *         source specifier.
     * @stable ICU 2.0
     */
    static int32_t U_EXPORT2 countAvailableTargets(const UnicodeString& source);

    /**
     * Return a registered target specifier for a given source.
     * @param index which specifier to return, from 0 to n-1, where
     * n = countAvailableTargets(source)
     * @param source the source specifier
     * @param result fill-in paramter to receive the target specifier.
     * If source is invalid or if index is out of range, result will
     * be empty.
     * @return reference to result
     * @stable ICU 2.0
     */
    static UnicodeString& U_EXPORT2 getAvailableTarget(int32_t index,
                                             const UnicodeString& source,
                                             UnicodeString& result);

    /**
     * Return the number of registered variant specifiers for a given
     * source-target pair.
     * @param source    the source specifiers.
     * @param target    the target specifiers.
     * @stable ICU 2.0
     */
    static int32_t U_EXPORT2 countAvailableVariants(const UnicodeString& source,
                                          const UnicodeString& target);

    /**
     * Return a registered variant specifier for a given source-target
     * pair.
     * @param index which specifier to return, from 0 to n-1, where
     * n = countAvailableVariants(source, target)
     * @param source the source specifier
     * @param target the target specifier
     * @param result fill-in paramter to receive the variant
     * specifier.  If source is invalid or if target is invalid or if
     * index is out of range, result will be empty.
     * @return reference to result
     * @stable ICU 2.0
     */
    static UnicodeString& U_EXPORT2 getAvailableVariant(int32_t index,
                                              const UnicodeString& source,
                                              const UnicodeString& target,
                                              UnicodeString& result);

protected:

    /**
     * Non-mutexed internal method
     * @internal
     */
    static int32_t _countAvailableSources(void);

    /**
     * Non-mutexed internal method
     * @internal
     */
    static UnicodeString& _getAvailableSource(int32_t index,
                                              UnicodeString& result);

    /**
     * Non-mutexed internal method
     * @internal
     */
    static int32_t _countAvailableTargets(const UnicodeString& source);

    /**
     * Non-mutexed internal method
     * @internal
     */
    static UnicodeString& _getAvailableTarget(int32_t index,
                                              const UnicodeString& source,
                                              UnicodeString& result);

    /**
     * Non-mutexed internal method
     * @internal
     */
    static int32_t _countAvailableVariants(const UnicodeString& source,
                                           const UnicodeString& target);

    /**
     * Non-mutexed internal method
     * @internal
     */
    static UnicodeString& _getAvailableVariant(int32_t index,
                                               const UnicodeString& source,
                                               const UnicodeString& target,
                                               UnicodeString& result);

protected:

    /**
     * Set the ID of this transliterators.  Subclasses shouldn't do
     * this, unless the underlying script behavior has changed.
     * @param id the new id t to be set.
     * @stable ICU 2.4
     */
    void setID(const UnicodeString& id);

public:

    /**
     * Return the class ID for this class.  This is useful only for
     * comparing to a return value from getDynamicClassID().
     * Note that Transliterator is an abstract base class, and therefor
     * no fully constructed object will  have a dynamic
     * UCLassID that equals the UClassID returned from
     * TRansliterator::getStaticClassID().
     * @return       The class ID for class Transliterator.
     * @stable ICU 2.0
     */
    static UClassID U_EXPORT2 getStaticClassID(void);

    /**
     * Returns a unique class ID <b>polymorphically</b>.  This method
     * is to implement a simple version of RTTI, since not all C++
     * compilers support genuine RTTI.  Polymorphic operator==() and
     * clone() methods call this method.
     *
     * <p>Concrete subclasses of Transliterator must use the
     *    UOBJECT_DEFINE_RTTI_IMPLEMENTATION macro from
     *    uobject.h to provide the RTTI functions.
     *
     * @return The class ID for this object. All objects of a given
     * class have the same class ID.  Objects of other classes have
     * different class IDs.
     * @stable ICU 2.0
     */
    virtual UClassID getDynamicClassID(void) const = 0;

private:
    static UBool initializeRegistry(UErrorCode &status);

public:
    /**
     * Return the number of IDs currently registered with the system.
     * To retrieve the actual IDs, call getAvailableID(i) with
     * i from 0 to countAvailableIDs() - 1.
     * @return the number of IDs currently registered with the system.
     * @obsolete ICU 3.4 use getAvailableIDs() instead
     */
    static int32_t U_EXPORT2 countAvailableIDs(void);

    /**
     * Return the index-th available ID.  index must be between 0
     * and countAvailableIDs() - 1, inclusive.  If index is out of
     * range, the result of getAvailableID(0) is returned.
     * @param index the given ID index.
     * @return      the index-th available ID.  index must be between 0
     *              and countAvailableIDs() - 1, inclusive.  If index is out of
     *              range, the result of getAvailableID(0) is returned.
     * @obsolete ICU 3.4 use getAvailableIDs() instead; this function
     * is not thread safe, since it returns a reference to storage that
     * may become invalid if another thread calls unregister
     */
    static const UnicodeString& U_EXPORT2 getAvailableID(int32_t index);
};

inline int32_t Transliterator::getMaximumContextLength(void) const {
    return maximumContextLength;
}

inline void Transliterator::setID(const UnicodeString& id) {
    ID = id;
    // NUL-terminate the ID string, which is a non-aliased copy.
    ID.append((UChar)0);
    ID.truncate(ID.length()-1);
}

inline Transliterator::Token Transliterator::integerToken(int32_t i) {
    Token t;
    t.integer = i;
    return t;
}

inline Transliterator::Token Transliterator::pointerToken(void* p) {
    Token t;
    t.pointer = p;
    return t;
}

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_TRANSLITERATION */

#endif
