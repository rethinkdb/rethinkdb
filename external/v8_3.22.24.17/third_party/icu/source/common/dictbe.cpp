/**
 *******************************************************************************
 * Copyright (C) 2006-2008, International Business Machines Corporation and others. *
 * All Rights Reserved.                                                        *
 *******************************************************************************
 */

#include "unicode/utypes.h"

#if !UCONFIG_NO_BREAK_ITERATION

#include "brkeng.h"
#include "dictbe.h"
#include "unicode/uniset.h"
#include "unicode/chariter.h"
#include "unicode/ubrk.h"
#include "uvector.h"
#include "triedict.h"
#include "uassert.h"
#include "unicode/normlzr.h"
#include "cmemory.h"

U_NAMESPACE_BEGIN

/*
 ******************************************************************
 */

/*DictionaryBreakEngine::DictionaryBreakEngine() {
    fTypes = 0;
}*/

DictionaryBreakEngine::DictionaryBreakEngine(uint32_t breakTypes) {
    fTypes = breakTypes;
}

DictionaryBreakEngine::~DictionaryBreakEngine() {
}

UBool
DictionaryBreakEngine::handles(UChar32 c, int32_t breakType) const {
    return (breakType >= 0 && breakType < 32 && (((uint32_t)1 << breakType) & fTypes)
            && fSet.contains(c));
}

int32_t
DictionaryBreakEngine::findBreaks( UText *text,
                                 int32_t startPos,
                                 int32_t endPos,
                                 UBool reverse,
                                 int32_t breakType,
                                 UStack &foundBreaks ) const {
    int32_t result = 0;

    // Find the span of characters included in the set.
    int32_t start = (int32_t)utext_getNativeIndex(text);
    int32_t current;
    int32_t rangeStart;
    int32_t rangeEnd;
    UChar32 c = utext_current32(text);
    if (reverse) {
        UBool   isDict = fSet.contains(c);
        while((current = (int32_t)utext_getNativeIndex(text)) > startPos && isDict) {
            c = utext_previous32(text);
            isDict = fSet.contains(c);
        }
        rangeStart = (current < startPos) ? startPos : current+(isDict ? 0 : 1);
        rangeEnd = start + 1;
    }
    else {
        while((current = (int32_t)utext_getNativeIndex(text)) < endPos && fSet.contains(c)) {
            utext_next32(text);         // TODO:  recast loop for postincrement
            c = utext_current32(text);
        }
        rangeStart = start;
        rangeEnd = current;
    }
    if (breakType >= 0 && breakType < 32 && (((uint32_t)1 << breakType) & fTypes)) {
        result = divideUpDictionaryRange(text, rangeStart, rangeEnd, foundBreaks);
        utext_setNativeIndex(text, current);
    }
    
    return result;
}

void
DictionaryBreakEngine::setCharacters( const UnicodeSet &set ) {
    fSet = set;
    // Compact for caching
    fSet.compact();
}

/*void
DictionaryBreakEngine::setBreakTypes( uint32_t breakTypes ) {
    fTypes = breakTypes;
}*/

/*
 ******************************************************************
 */


// Helper class for improving readability of the Thai word break
// algorithm. The implementation is completely inline.

// List size, limited by the maximum number of words in the dictionary
// that form a nested sequence.
#define POSSIBLE_WORD_LIST_MAX 20

class PossibleWord {
 private:
  // list of word candidate lengths, in increasing length order
  int32_t   lengths[POSSIBLE_WORD_LIST_MAX];
  int       count;      // Count of candidates
  int32_t   prefix;     // The longest match with a dictionary word
  int32_t   offset;     // Offset in the text of these candidates
  int       mark;       // The preferred candidate's offset
  int       current;    // The candidate we're currently looking at

 public:
  PossibleWord();
  ~PossibleWord();
  
  // Fill the list of candidates if needed, select the longest, and return the number found
  int       candidates( UText *text, const TrieWordDictionary *dict, int32_t rangeEnd );
  
  // Select the currently marked candidate, point after it in the text, and invalidate self
  int32_t   acceptMarked( UText *text );
  
  // Back up from the current candidate to the next shorter one; return TRUE if that exists
  // and point the text after it
  UBool     backUp( UText *text );
  
  // Return the longest prefix this candidate location shares with a dictionary word
  int32_t   longestPrefix();
  
  // Mark the current candidate as the one we like
  void      markCurrent();
};

inline
PossibleWord::PossibleWord() {
    offset = -1;
}

inline
PossibleWord::~PossibleWord() {
}

inline int
PossibleWord::candidates( UText *text, const TrieWordDictionary *dict, int32_t rangeEnd ) {
    // TODO: If getIndex is too slow, use offset < 0 and add discardAll()
    int32_t start = (int32_t)utext_getNativeIndex(text);
    if (start != offset) {
        offset = start;
        prefix = dict->matches(text, rangeEnd-start, lengths, count, sizeof(lengths)/sizeof(lengths[0]));
        // Dictionary leaves text after longest prefix, not longest word. Back up.
        if (count <= 0) {
            utext_setNativeIndex(text, start);
        }
    }
    if (count > 0) {
        utext_setNativeIndex(text, start+lengths[count-1]);
    }
    current = count-1;
    mark = current;
    return count;
}

inline int32_t
PossibleWord::acceptMarked( UText *text ) {
    utext_setNativeIndex(text, offset + lengths[mark]);
    return lengths[mark];
}

inline UBool
PossibleWord::backUp( UText *text ) {
    if (current > 0) {
        utext_setNativeIndex(text, offset + lengths[--current]);
        return TRUE;
    }
    return FALSE;
}

inline int32_t
PossibleWord::longestPrefix() {
    return prefix;
}

inline void
PossibleWord::markCurrent() {
    mark = current;
}

// How many words in a row are "good enough"?
#define THAI_LOOKAHEAD 3

// Will not combine a non-word with a preceding dictionary word longer than this
#define THAI_ROOT_COMBINE_THRESHOLD 3

// Will not combine a non-word that shares at least this much prefix with a
// dictionary word, with a preceding word
#define THAI_PREFIX_COMBINE_THRESHOLD 3

// Ellision character
#define THAI_PAIYANNOI 0x0E2F

// Repeat character
#define THAI_MAIYAMOK 0x0E46

// Minimum word size
#define THAI_MIN_WORD 2

// Minimum number of characters for two words
#define THAI_MIN_WORD_SPAN (THAI_MIN_WORD * 2)

ThaiBreakEngine::ThaiBreakEngine(const TrieWordDictionary *adoptDictionary, UErrorCode &status)
    : DictionaryBreakEngine((1<<UBRK_WORD) | (1<<UBRK_LINE)),
      fDictionary(adoptDictionary)
{
    fThaiWordSet.applyPattern(UNICODE_STRING_SIMPLE("[[:Thai:]&[:LineBreak=SA:]]"), status);
    if (U_SUCCESS(status)) {
        setCharacters(fThaiWordSet);
    }
    fMarkSet.applyPattern(UNICODE_STRING_SIMPLE("[[:Thai:]&[:LineBreak=SA:]&[:M:]]"), status);
    fMarkSet.add(0x0020);
    fEndWordSet = fThaiWordSet;
    fEndWordSet.remove(0x0E31);             // MAI HAN-AKAT
    fEndWordSet.remove(0x0E40, 0x0E44);     // SARA E through SARA AI MAIMALAI
    fBeginWordSet.add(0x0E01, 0x0E2E);      // KO KAI through HO NOKHUK
    fBeginWordSet.add(0x0E40, 0x0E44);      // SARA E through SARA AI MAIMALAI
    fSuffixSet.add(THAI_PAIYANNOI);
    fSuffixSet.add(THAI_MAIYAMOK);

    // Compact for caching.
    fMarkSet.compact();
    fEndWordSet.compact();
    fBeginWordSet.compact();
    fSuffixSet.compact();
}

ThaiBreakEngine::~ThaiBreakEngine() {
    delete fDictionary;
}

int32_t
ThaiBreakEngine::divideUpDictionaryRange( UText *text,
                                                int32_t rangeStart,
                                                int32_t rangeEnd,
                                                UStack &foundBreaks ) const {
    if ((rangeEnd - rangeStart) < THAI_MIN_WORD_SPAN) {
        return 0;       // Not enough characters for two words
    }

    uint32_t wordsFound = 0;
    int32_t wordLength;
    int32_t current;
    UErrorCode status = U_ZERO_ERROR;
    PossibleWord words[THAI_LOOKAHEAD];
    UChar32 uc;
    
    utext_setNativeIndex(text, rangeStart);
    
    while (U_SUCCESS(status) && (current = (int32_t)utext_getNativeIndex(text)) < rangeEnd) {
        wordLength = 0;

        // Look for candidate words at the current position
        int candidates = words[wordsFound%THAI_LOOKAHEAD].candidates(text, fDictionary, rangeEnd);
        
        // If we found exactly one, use that
        if (candidates == 1) {
            wordLength = words[wordsFound%THAI_LOOKAHEAD].acceptMarked(text);
            wordsFound += 1;
        }
        
        // If there was more than one, see which one can take us forward the most words
        else if (candidates > 1) {
            // If we're already at the end of the range, we're done
            if ((int32_t)utext_getNativeIndex(text) >= rangeEnd) {
                goto foundBest;
            }
            do {
                int wordsMatched = 1;
                if (words[(wordsFound+1)%THAI_LOOKAHEAD].candidates(text, fDictionary, rangeEnd) > 0) {
                    if (wordsMatched < 2) {
                        // Followed by another dictionary word; mark first word as a good candidate
                        words[wordsFound%THAI_LOOKAHEAD].markCurrent();
                        wordsMatched = 2;
                    }
                    
                    // If we're already at the end of the range, we're done
                    if ((int32_t)utext_getNativeIndex(text) >= rangeEnd) {
                        goto foundBest;
                    }
                    
                    // See if any of the possible second words is followed by a third word
                    do {
                        // If we find a third word, stop right away
                        if (words[(wordsFound+2)%THAI_LOOKAHEAD].candidates(text, fDictionary, rangeEnd)) {
                            words[wordsFound%THAI_LOOKAHEAD].markCurrent();
                            goto foundBest;
                        }
                    }
                    while (words[(wordsFound+1)%THAI_LOOKAHEAD].backUp(text));
                }
            }
            while (words[wordsFound%THAI_LOOKAHEAD].backUp(text));
foundBest:
            wordLength = words[wordsFound%THAI_LOOKAHEAD].acceptMarked(text);
            wordsFound += 1;
        }
        
        // We come here after having either found a word or not. We look ahead to the
        // next word. If it's not a dictionary word, we will combine it withe the word we
        // just found (if there is one), but only if the preceding word does not exceed
        // the threshold.
        // The text iterator should now be positioned at the end of the word we found.
        if ((int32_t)utext_getNativeIndex(text) < rangeEnd && wordLength < THAI_ROOT_COMBINE_THRESHOLD) {
            // if it is a dictionary word, do nothing. If it isn't, then if there is
            // no preceding word, or the non-word shares less than the minimum threshold
            // of characters with a dictionary word, then scan to resynchronize
            if (words[wordsFound%THAI_LOOKAHEAD].candidates(text, fDictionary, rangeEnd) <= 0
                  && (wordLength == 0
                      || words[wordsFound%THAI_LOOKAHEAD].longestPrefix() < THAI_PREFIX_COMBINE_THRESHOLD)) {
                // Look for a plausible word boundary
                //TODO: This section will need a rework for UText.
                int32_t remaining = rangeEnd - (current+wordLength);
                UChar32 pc = utext_current32(text);
                int32_t chars = 0;
                for (;;) {
                    utext_next32(text);
                    uc = utext_current32(text);
                    // TODO: Here we're counting on the fact that the SA languages are all
                    // in the BMP. This should get fixed with the UText rework.
                    chars += 1;
                    if (--remaining <= 0) {
                        break;
                    }
                    if (fEndWordSet.contains(pc) && fBeginWordSet.contains(uc)) {
                        // Maybe. See if it's in the dictionary.
                        // NOTE: In the original Apple code, checked that the next
                        // two characters after uc were not 0x0E4C THANTHAKHAT before
                        // checking the dictionary. That is just a performance filter,
                        // but it's not clear it's faster than checking the trie.
                        int candidates = words[(wordsFound+1)%THAI_LOOKAHEAD].candidates(text, fDictionary, rangeEnd);
                        utext_setNativeIndex(text, current+wordLength+chars);
                        if (candidates > 0) {
                            break;
                        }
                    }
                    pc = uc;
                }
                
                // Bump the word count if there wasn't already one
                if (wordLength <= 0) {
                    wordsFound += 1;
                }
                
                // Update the length with the passed-over characters
                wordLength += chars;
            }
            else {
                // Back up to where we were for next iteration
                utext_setNativeIndex(text, current+wordLength);
            }
        }
        
        // Never stop before a combining mark.
        int32_t currPos;
        while ((currPos = (int32_t)utext_getNativeIndex(text)) < rangeEnd && fMarkSet.contains(utext_current32(text))) {
            utext_next32(text);
            wordLength += (int32_t)utext_getNativeIndex(text) - currPos;
        }
        
        // Look ahead for possible suffixes if a dictionary word does not follow.
        // We do this in code rather than using a rule so that the heuristic
        // resynch continues to function. For example, one of the suffix characters
        // could be a typo in the middle of a word.
        if ((int32_t)utext_getNativeIndex(text) < rangeEnd && wordLength > 0) {
            if (words[wordsFound%THAI_LOOKAHEAD].candidates(text, fDictionary, rangeEnd) <= 0
                && fSuffixSet.contains(uc = utext_current32(text))) {
                if (uc == THAI_PAIYANNOI) {
                    if (!fSuffixSet.contains(utext_previous32(text))) {
                        // Skip over previous end and PAIYANNOI
                        utext_next32(text);
                        utext_next32(text);
                        wordLength += 1;            // Add PAIYANNOI to word
                        uc = utext_current32(text);     // Fetch next character
                    }
                    else {
                        // Restore prior position
                        utext_next32(text);
                    }
                }
                if (uc == THAI_MAIYAMOK) {
                    if (utext_previous32(text) != THAI_MAIYAMOK) {
                        // Skip over previous end and MAIYAMOK
                        utext_next32(text);
                        utext_next32(text);
                        wordLength += 1;            // Add MAIYAMOK to word
                    }
                    else {
                        // Restore prior position
                        utext_next32(text);
                    }
                }
            }
            else {
                utext_setNativeIndex(text, current+wordLength);
            }
        }
        
        // Did we find a word on this iteration? If so, push it on the break stack
        if (wordLength > 0) {
            foundBreaks.push((current+wordLength), status);
        }
    }
    
    // Don't return a break for the end of the dictionary range if there is one there.
    if (foundBreaks.peeki() >= rangeEnd) {
        (void) foundBreaks.popi();
        wordsFound -= 1;
    }

    return wordsFound;
}

/*
 ******************************************************************
 * CjkBreakEngine
 */
static const uint32_t kuint32max = 0xFFFFFFFF;
CjkBreakEngine::CjkBreakEngine(const TrieWordDictionary *adoptDictionary, LanguageType type, UErrorCode &status)
: DictionaryBreakEngine(1<<UBRK_WORD), fDictionary(adoptDictionary){
    if (!adoptDictionary->getValued()) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }

    // Korean dictionary only includes Hangul syllables
    fHangulWordSet.applyPattern(UNICODE_STRING_SIMPLE("[\\uac00-\\ud7a3]"), status);
    fHanWordSet.applyPattern(UNICODE_STRING_SIMPLE("[:Han:]"), status);
    fKatakanaWordSet.applyPattern(UNICODE_STRING_SIMPLE("[[:Katakana:]\\uff9e\\uff9f]"), status);
    fHiraganaWordSet.applyPattern(UNICODE_STRING_SIMPLE("[:Hiragana:]"), status);

    if (U_SUCCESS(status)) {
        // handle Korean and Japanese/Chinese using different dictionaries
        if (type == kKorean) {
            setCharacters(fHangulWordSet);
        } else { //Chinese and Japanese
            UnicodeSet cjSet;
            cjSet.addAll(fHanWordSet);
            cjSet.addAll(fKatakanaWordSet);
            cjSet.addAll(fHiraganaWordSet);
            cjSet.add(UNICODE_STRING_SIMPLE("\\uff70\\u30fc"));
            setCharacters(cjSet);
        }
    }
}

CjkBreakEngine::~CjkBreakEngine(){
    delete fDictionary;
}

// The katakanaCost values below are based on the length frequencies of all
// katakana phrases in the dictionary
static const int kMaxKatakanaLength = 8;
static const int kMaxKatakanaGroupLength = 20;
static const uint32_t maxSnlp = 255;

static inline uint32_t getKatakanaCost(int wordLength){
    //TODO: fill array with actual values from dictionary!
    static const uint32_t katakanaCost[kMaxKatakanaLength + 1]
                                       = {8192, 984, 408, 240, 204, 252, 300, 372, 480};
    return (wordLength > kMaxKatakanaLength) ? 8192 : katakanaCost[wordLength];
}

static inline bool isKatakana(uint16_t value) {
    return (value >= 0x30A1u && value <= 0x30FEu && value != 0x30FBu) ||
            (value >= 0xFF66u && value <= 0xFF9fu);
}

// A very simple helper class to streamline the buffer handling in
// divideUpDictionaryRange. 
template<class T, size_t N>
class AutoBuffer {
 public:
  AutoBuffer(size_t size) : buffer(stackBuffer), capacity(N) {
    if (size > N) {
      buffer = reinterpret_cast<T*>(uprv_malloc(sizeof(T)*size));
      capacity = size;
    }
  }
  ~AutoBuffer() {
    if (buffer != stackBuffer)
      uprv_free(buffer);
  }
#if 0
  T* operator& () {
    return buffer;
  }
#endif
  T* elems() {
    return buffer;
  }
  const T& operator[] (size_t i) const {
    return buffer[i];
  }
  T& operator[] (size_t i) {
    return buffer[i];
  }

  // resize without copy
  void resize(size_t size) {
    if (size <= capacity)
      return;
    if (buffer != stackBuffer)
      uprv_free(buffer);
    buffer = reinterpret_cast<T*>(uprv_malloc(sizeof(T)*size));
    capacity = size;
  }
 private:
  T stackBuffer[N];
  T* buffer;
  AutoBuffer();
  size_t capacity;
};


/*
 * @param text A UText representing the text
 * @param rangeStart The start of the range of dictionary characters
 * @param rangeEnd The end of the range of dictionary characters
 * @param foundBreaks Output of C array of int32_t break positions, or 0
 * @return The number of breaks found
 */
int32_t 
CjkBreakEngine::divideUpDictionaryRange( UText *text,
        int32_t rangeStart,
        int32_t rangeEnd,
        UStack &foundBreaks ) const {
    if (rangeStart >= rangeEnd) {
        return 0;
    }

    const size_t defaultInputLength = 80;
    size_t inputLength = rangeEnd - rangeStart;
    AutoBuffer<UChar, defaultInputLength> charString(inputLength);

    // Normalize the input string and put it in normalizedText.
    // The map from the indices of the normalized input to the raw
    // input is kept in charPositions.
    UErrorCode status = U_ZERO_ERROR;
    utext_extract(text, rangeStart, rangeEnd, charString.elems(), inputLength, &status);
    if (U_FAILURE(status))
        return 0;

    UnicodeString inputString(charString.elems(), inputLength);
    UNormalizationMode norm_mode = UNORM_NFKC;
    UBool isNormalized =
        Normalizer::quickCheck(inputString, norm_mode, status) == UNORM_YES ||
        Normalizer::isNormalized(inputString, norm_mode, status);

    AutoBuffer<int32_t, defaultInputLength> charPositions(inputLength + 1);
    int numChars = 0;
    UText normalizedText = UTEXT_INITIALIZER;
    // Needs to be declared here because normalizedText holds onto its buffer.
    UnicodeString normalizedString;
    if (isNormalized) {
        int32_t index = 0;
        charPositions[0] = 0;
        while(index < inputString.length()) {
            index = inputString.moveIndex32(index, 1);
            charPositions[++numChars] = index;
        }
        utext_openUnicodeString(&normalizedText, &inputString, &status);
    }
    else {
        Normalizer::normalize(inputString, norm_mode, 0, normalizedString, status);
        if (U_FAILURE(status))
            return 0;
        charPositions.resize(normalizedString.length() + 1);
        Normalizer normalizer(charString.elems(), inputLength, norm_mode);
        int32_t index = 0;
        charPositions[0] = 0;
        while(index < normalizer.endIndex()){
            UChar32 uc = normalizer.next();
            charPositions[++numChars] = index = normalizer.getIndex();
        }
        utext_openUnicodeString(&normalizedText, &normalizedString, &status);
    }

    if (U_FAILURE(status))
        return 0;

    // From this point on, all the indices refer to the indices of
    // the normalized input string.

    // bestSnlp[i] is the snlp of the best segmentation of the first i
    // characters in the range to be matched.
    AutoBuffer<uint32_t, defaultInputLength> bestSnlp(numChars + 1);
    bestSnlp[0] = 0;
    for(int i=1; i<=numChars; i++){
        bestSnlp[i] = kuint32max;
    }

    // prev[i] is the index of the last CJK character in the previous word in 
    // the best segmentation of the first i characters.
    AutoBuffer<int, defaultInputLength> prev(numChars + 1);
    for(int i=0; i<=numChars; i++){
        prev[i] = -1;
    }

    const size_t maxWordSize = 20;
    AutoBuffer<uint16_t, maxWordSize> values(numChars);
    AutoBuffer<int32_t, maxWordSize> lengths(numChars);

    // Dynamic programming to find the best segmentation.
    bool is_prev_katakana = false;
    for (int i = 0; i < numChars; ++i) {
        //utext_setNativeIndex(text, rangeStart + i);
        utext_setNativeIndex(&normalizedText, i);
        if (bestSnlp[i] == kuint32max)
            continue;

        int count;
        // limit maximum word length matched to size of current substring
        int maxSearchLength = (i + maxWordSize < (size_t) numChars)? maxWordSize: numChars - i; 

        fDictionary->matches(&normalizedText, maxSearchLength, lengths.elems(), count, maxSearchLength, values.elems());

        // if there are no single character matches found in the dictionary 
        // starting with this charcter, treat character as a 1-character word 
        // with the highest value possible, i.e. the least likely to occur.
        // Exclude Korean characters from this treatment, as they should be left
        // together by default.
        if((count == 0 || lengths[0] != 1) &&
                !fHangulWordSet.contains(utext_current32(&normalizedText))){
            values[count] = maxSnlp;
            lengths[count++] = 1;
        }

        for (int j = 0; j < count; j++){
            //U_ASSERT(values[j] >= 0 && values[j] <= maxSnlp);
            uint32_t newSnlp = bestSnlp[i] + values[j];
            if (newSnlp < bestSnlp[lengths[j] + i]) {
                bestSnlp[lengths[j] + i] = newSnlp;
                prev[lengths[j] + i] = i;
            }
        }

        // In Japanese,
        // Katakana word in single character is pretty rare. So we apply
        // the following heuristic to Katakana: any continuous run of Katakana
        // characters is considered a candidate word with a default cost
        // specified in the katakanaCost table according to its length.
        //utext_setNativeIndex(text, rangeStart + i);
        utext_setNativeIndex(&normalizedText, i);
        bool is_katakana = isKatakana(utext_current32(&normalizedText));
        if (!is_prev_katakana && is_katakana) {
            int j = i + 1;
            utext_next32(&normalizedText);
            // Find the end of the continuous run of Katakana characters
            while (j < numChars && (j - i) < kMaxKatakanaGroupLength &&
                    isKatakana(utext_current32(&normalizedText))) {
                utext_next32(&normalizedText);
                ++j;
            }
            if ((j - i) < kMaxKatakanaGroupLength) {
                uint32_t newSnlp = bestSnlp[i] + getKatakanaCost(j - i);
                if (newSnlp < bestSnlp[j]) {
                    bestSnlp[j] = newSnlp;
                    prev[j] = i;
                }
            }
        }
        is_prev_katakana = is_katakana;
    }

    // Start pushing the optimal offset index into t_boundary (t for tentative).
    // prev[numChars] is guaranteed to be meaningful.
    // We'll first push in the reverse order, i.e.,
    // t_boundary[0] = numChars, and afterwards do a swap.
    AutoBuffer<int, maxWordSize> t_boundary(numChars + 1);

    int numBreaks = 0;
    // No segmentation found, set boundary to end of range
    if (bestSnlp[numChars] == kuint32max) {
        t_boundary[numBreaks++] = numChars;
    } else {
        for (int i = numChars; i > 0; i = prev[i]){
            t_boundary[numBreaks++] = i;
    
        }
        U_ASSERT(prev[t_boundary[numBreaks-1]] == 0);
    }

    // Reverse offset index in t_boundary.
    // Don't add a break for the start of the dictionary range if there is one
    // there already.
    if (foundBreaks.size() == 0 || foundBreaks.peeki() < rangeStart) {
        t_boundary[numBreaks++] = 0;
    }

    // Now that we're done, convert positions in t_bdry[] (indices in 
    // the normalized input string) back to indices in the raw input string
    // while reversing t_bdry and pushing values to foundBreaks.
    for (int i = numBreaks-1; i >= 0; i--) {
        foundBreaks.push(charPositions[t_boundary[i]] + rangeStart, status);
    }

    utext_close(&normalizedText);
    return numBreaks;
}

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_BREAK_ITERATION */
