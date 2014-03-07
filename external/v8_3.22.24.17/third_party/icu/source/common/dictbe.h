/**
 **********************************************************************************
 * Copyright (C) 2006-2010, International Business Machines Corporation and others.
 * All Rights Reserved.                                                       
 **********************************************************************************
 */

#ifndef DICTBE_H
#define DICTBE_H

#include "unicode/utypes.h"
#include "unicode/uniset.h"
#include "unicode/utext.h"

#include "brkeng.h"

U_NAMESPACE_BEGIN

class TrieWordDictionary;

/*******************************************************************
 * DictionaryBreakEngine
 */

/**
 * <p>DictionaryBreakEngine is a kind of LanguageBreakEngine that uses a
 * dictionary to determine language-specific breaks.</p>
 *
 * <p>After it is constructed a DictionaryBreakEngine may be shared between
 * threads without synchronization.</p>
 */
class DictionaryBreakEngine : public LanguageBreakEngine {
 private:
    /**
     * The set of characters handled by this engine
     * @internal
     */

  UnicodeSet    fSet;

    /**
     * The set of break types handled by this engine
     * @internal
     */

  uint32_t      fTypes;

  /**
   * <p>Default constructor.</p>
   *
   */
  DictionaryBreakEngine();

 public:

  /**
   * <p>Constructor setting the break types handled.</p>
   *
   * @param breakTypes A bitmap of types handled by the engine.
   */
  DictionaryBreakEngine( uint32_t breakTypes );

  /**
   * <p>Virtual destructor.</p>
   */
  virtual ~DictionaryBreakEngine();

  /**
   * <p>Indicate whether this engine handles a particular character for
   * a particular kind of break.</p>
   *
   * @param c A character which begins a run that the engine might handle
   * @param breakType The type of text break which the caller wants to determine
   * @return TRUE if this engine handles the particular character and break
   * type.
   */
  virtual UBool handles( UChar32 c, int32_t breakType ) const;

  /**
   * <p>Find any breaks within a run in the supplied text.</p>
   *
   * @param text A UText representing the text. The iterator is left at
   * the end of the run of characters which the engine is capable of handling 
   * that starts from the first (or last) character in the range.
   * @param startPos The start of the run within the supplied text.
   * @param endPos The end of the run within the supplied text.
   * @param reverse Whether the caller is looking for breaks in a reverse
   * direction.
   * @param breakType The type of break desired, or -1.
   * @param foundBreaks An allocated C array of the breaks found, if any
   * @return The number of breaks found.
   */
  virtual int32_t findBreaks( UText *text,
                              int32_t startPos,
                              int32_t endPos,
                              UBool reverse,
                              int32_t breakType,
                              UStack &foundBreaks ) const;

 protected:

 /**
  * <p>Set the character set handled by this engine.</p>
  *
  * @param set A UnicodeSet of the set of characters handled by the engine
  */
  virtual void setCharacters( const UnicodeSet &set );

 /**
  * <p>Set the break types handled by this engine.</p>
  *
  * @param breakTypes A bitmap of types handled by the engine.
  */
//  virtual void setBreakTypes( uint32_t breakTypes );

 /**
  * <p>Divide up a range of known dictionary characters handled by this break engine.</p>
  *
  * @param text A UText representing the text
  * @param rangeStart The start of the range of dictionary characters
  * @param rangeEnd The end of the range of dictionary characters
  * @param foundBreaks Output of C array of int32_t break positions, or 0
  * @return The number of breaks found
  */
  virtual int32_t divideUpDictionaryRange( UText *text,
                                           int32_t rangeStart,
                                           int32_t rangeEnd,
                                           UStack &foundBreaks ) const = 0;

};

/*******************************************************************
 * ThaiBreakEngine
 */

/**
 * <p>ThaiBreakEngine is a kind of DictionaryBreakEngine that uses a
 * TrieWordDictionary and heuristics to determine Thai-specific breaks.</p>
 *
 * <p>After it is constructed a ThaiBreakEngine may be shared between
 * threads without synchronization.</p>
 */
class ThaiBreakEngine : public DictionaryBreakEngine {
 private:
    /**
     * The set of characters handled by this engine
     * @internal
     */

  UnicodeSet                fThaiWordSet;
  UnicodeSet                fEndWordSet;
  UnicodeSet                fBeginWordSet;
  UnicodeSet                fSuffixSet;
  UnicodeSet                fMarkSet;
  const TrieWordDictionary  *fDictionary;

 public:

  /**
   * <p>Default constructor.</p>
   *
   * @param adoptDictionary A TrieWordDictionary to adopt. Deleted when the
   * engine is deleted.
   */
  ThaiBreakEngine(const TrieWordDictionary *adoptDictionary, UErrorCode &status);

  /**
   * <p>Virtual destructor.</p>
   */
  virtual ~ThaiBreakEngine();

 protected:
 /**
  * <p>Divide up a range of known dictionary characters handled by this break engine.</p>
  *
  * @param text A UText representing the text
  * @param rangeStart The start of the range of dictionary characters
  * @param rangeEnd The end of the range of dictionary characters
  * @param foundBreaks Output of C array of int32_t break positions, or 0
  * @return The number of breaks found
  */
  virtual int32_t divideUpDictionaryRange( UText *text,
                                           int32_t rangeStart,
                                           int32_t rangeEnd,
                                           UStack &foundBreaks ) const;

};

/*******************************************************************
 * CjkBreakEngine
 */

//indicates language/script that the CjkBreakEngine will handle
enum LanguageType {
    kKorean,
    kChineseJapanese
};

/**
 * <p>CjkBreakEngine is a kind of DictionaryBreakEngine that uses a
 * TrieWordDictionary with costs associated with each word and
 * Viterbi decoding to determine CJK-specific breaks.</p>
 */
class CjkBreakEngine : public DictionaryBreakEngine {
 protected:
    /**
     * The set of characters handled by this engine
     * @internal
     */
  UnicodeSet                fHangulWordSet;
  UnicodeSet                fHanWordSet;
  UnicodeSet                fKatakanaWordSet;
  UnicodeSet                fHiraganaWordSet;

  const TrieWordDictionary  *fDictionary;

 public:

    /**
     * <p>Default constructor.</p>
     *
     * @param adoptDictionary A TrieWordDictionary to adopt. Deleted when the
     * engine is deleted. The TrieWordDictionary must contain costs for each word
     * in order for the dictionary to work properly.
     */
  CjkBreakEngine(const TrieWordDictionary *adoptDictionary, LanguageType type, UErrorCode &status);

    /**
     * <p>Virtual destructor.</p>
     */
  virtual ~CjkBreakEngine();

 protected:
    /**
     * <p>Divide up a range of known dictionary characters handled by this break engine.</p>
     *
     * @param text A UText representing the text
     * @param rangeStart The start of the range of dictionary characters
     * @param rangeEnd The end of the range of dictionary characters
     * @param foundBreaks Output of C array of int32_t break positions, or 0
     * @return The number of breaks found
     */
  virtual int32_t divideUpDictionaryRange( UText *text,
          int32_t rangeStart,
          int32_t rangeEnd,
          UStack &foundBreaks ) const;

};

U_NAMESPACE_END

    /* DICTBE_H */
#endif
