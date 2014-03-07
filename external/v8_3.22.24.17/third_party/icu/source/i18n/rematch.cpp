/*
**************************************************************************
*   Copyright (C) 2002-2010 International Business Machines Corporation  *
*   and others. All rights reserved.                                     *
**************************************************************************
*/
//
//  file:  rematch.cpp
//
//         Contains the implementation of class RegexMatcher,
//         which is one of the main API classes for the ICU regular expression package.
//

#include "unicode/utypes.h"
#if !UCONFIG_NO_REGULAR_EXPRESSIONS

#include "unicode/regex.h"
#include "unicode/uniset.h"
#include "unicode/uchar.h"
#include "unicode/ustring.h"
#include "unicode/rbbi.h"
#include "uassert.h"
#include "cmemory.h"
#include "uvector.h"
#include "uvectr32.h"
#include "uvectr64.h"
#include "regeximp.h"
#include "regexst.h"
#include "regextxt.h"
#include "ucase.h"

// #include <malloc.h>        // Needed for heapcheck testing


// Find progress callback
// ----------------------
// Macro to inline test & call to ReportFindProgress().  Eliminates unnecessary function call.
//
#define REGEXFINDPROGRESS_INTERRUPT(pos, status)     \
    (fFindProgressCallbackFn != NULL) && (ReportFindProgress(pos, status) == FALSE)


// Smart Backtracking
// ------------------
// When a failure would go back to a LOOP_C instruction,
// strings, characters, and setrefs scan backwards for a valid start
// character themselves, pop the stack, and save state, emulating the
// LOOP_C's effect but assured that the next character of input is a
// possible matching character.
//
// Good idea in theory; unfortunately it only helps out a few specific
// cases and slows the engine down a little in the rest.

//#define REGEX_SMART_BACKTRACKING 1

U_NAMESPACE_BEGIN

// Default limit for the size of the back track stack, to avoid system
//    failures causedby heap exhaustion.  Units are in 32 bit words, not bytes.
// This value puts ICU's limits higher than most other regexp implementations,
//    which use recursion rather than the heap, and take more storage per
//    backtrack point.
//
static const int32_t DEFAULT_BACKTRACK_STACK_CAPACITY = 8000000;

// Time limit counter constant.
//   Time limits for expression evaluation are in terms of quanta of work by
//   the engine, each of which is 10,000 state saves.
//   This constant determines that state saves per tick number.
static const int32_t TIMER_INITIAL_VALUE = 10000;

//-----------------------------------------------------------------------------
//
//   Constructor and Destructor
//
//-----------------------------------------------------------------------------
RegexMatcher::RegexMatcher(const RegexPattern *pat)  { 
    fDeferredStatus = U_ZERO_ERROR;
    init(fDeferredStatus);
    if (U_FAILURE(fDeferredStatus)) {
        return;
    }
    if (pat==NULL) {
        fDeferredStatus = U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }
    fPattern = pat;
    init2(RegexStaticSets::gStaticSets->fEmptyText, fDeferredStatus);
}



RegexMatcher::RegexMatcher(const UnicodeString &regexp, const UnicodeString &input,
                           uint32_t flags, UErrorCode &status) {
    init(status);
    if (U_FAILURE(status)) {
        return;
    }
    UParseError    pe;
    fPatternOwned      = RegexPattern::compile(regexp, flags, pe, status);
    fPattern           = fPatternOwned;
    
    UText inputText = UTEXT_INITIALIZER;
    utext_openConstUnicodeString(&inputText, &input, &status);
    init2(&inputText, status);
    utext_close(&inputText);

    fInputUniStrMaybeMutable = TRUE;    
}


RegexMatcher::RegexMatcher(UText *regexp, UText *input,
                           uint32_t flags, UErrorCode &status) {
    init(status);
    if (U_FAILURE(status)) {
        return;
    }
    UParseError    pe;
    fPatternOwned      = RegexPattern::compile(regexp, flags, pe, status);
    if (U_FAILURE(status)) {
        return;
    }

    fPattern           = fPatternOwned;
    init2(input, status);
}


RegexMatcher::RegexMatcher(const UnicodeString &regexp, 
                           uint32_t flags, UErrorCode &status) {
    init(status);
    if (U_FAILURE(status)) {
        return;
    }
    UParseError    pe;
    fPatternOwned      = RegexPattern::compile(regexp, flags, pe, status);
    if (U_FAILURE(status)) {
        return;
    }
    fPattern           = fPatternOwned;
    init2(RegexStaticSets::gStaticSets->fEmptyText, status);
}

RegexMatcher::RegexMatcher(UText *regexp, 
                           uint32_t flags, UErrorCode &status) {
    init(status);
    if (U_FAILURE(status)) {
        return;
    }
    UParseError    pe;
    fPatternOwned      = RegexPattern::compile(regexp, flags, pe, status);
        if (U_FAILURE(status)) {
        return;
    }

    fPattern           = fPatternOwned;
    init2(RegexStaticSets::gStaticSets->fEmptyText, status);
}




RegexMatcher::~RegexMatcher() {
    delete fStack;
    if (fData != fSmallData) {
        uprv_free(fData);
        fData = NULL;
    }
    if (fPatternOwned) {
        delete fPatternOwned;
        fPatternOwned = NULL;
        fPattern = NULL;
    }
    
    if (fInput) {
        delete fInput;
    }
    if (fInputText) {
        utext_close(fInputText);
    }
    if (fAltInputText) {
        utext_close(fAltInputText);
    }
    
    #if UCONFIG_NO_BREAK_ITERATION==0
    delete fWordBreakItr;
    #endif
}

//
//   init()   common initialization for use by all constructors.
//            Initialize all fields, get the object into a consistent state.
//            This must be done even when the initial status shows an error,
//            so that the object is initialized sufficiently well for the destructor
//            to run safely.
//
void RegexMatcher::init(UErrorCode &status) {
    fPattern           = NULL;
    fPatternOwned      = NULL;
    fFrameSize         = 0;
    fRegionStart       = 0;
    fRegionLimit       = 0;
    fAnchorStart       = 0;
    fAnchorLimit       = 0;
    fLookStart         = 0;
    fLookLimit         = 0;
    fActiveStart       = 0;
    fActiveLimit       = 0;
    fTransparentBounds = FALSE;
    fAnchoringBounds   = TRUE;
    fMatch             = FALSE;
    fMatchStart        = 0;
    fMatchEnd          = 0;
    fLastMatchEnd      = -1;
    fAppendPosition    = 0;
    fHitEnd            = FALSE;
    fRequireEnd        = FALSE;
    fStack             = NULL;
    fFrame             = NULL;
    fTimeLimit         = 0;
    fTime              = 0;
    fTickCounter       = 0;
    fStackLimit        = DEFAULT_BACKTRACK_STACK_CAPACITY;
    fCallbackFn        = NULL;
    fCallbackContext   = NULL;
    fFindProgressCallbackFn      = NULL;
    fFindProgressCallbackContext = NULL;
    fTraceDebug        = FALSE;
    fDeferredStatus    = status;
    fData              = fSmallData;
    fWordBreakItr      = NULL;
    
    fStack             = new UVector64(status);
    fInputText         = NULL;
    fAltInputText      = NULL;
    fInput             = NULL;
    fInputLength       = 0;
    fInputUniStrMaybeMutable = FALSE;

    if (U_FAILURE(status)) {
        fDeferredStatus = status;
    }
}

//
//  init2()   Common initialization for use by RegexMatcher constructors, part 2.
//            This handles the common setup to be done after the Pattern is available.
//
void RegexMatcher::init2(UText *input, UErrorCode &status) {
    if (U_FAILURE(status)) {
        fDeferredStatus = status;
        return;
    }

    if (fPattern->fDataSize > (int32_t)(sizeof(fSmallData)/sizeof(fSmallData[0]))) {
        fData = (int64_t *)uprv_malloc(fPattern->fDataSize * sizeof(int64_t)); 
        if (fData == NULL) {
            status = fDeferredStatus = U_MEMORY_ALLOCATION_ERROR;
            return;
        }
    }

    reset(input);
    setStackLimit(DEFAULT_BACKTRACK_STACK_CAPACITY, status);
    if (U_FAILURE(status)) {
        fDeferredStatus = status;
        return;
    }
}


static const UChar BACKSLASH  = 0x5c;
static const UChar DOLLARSIGN = 0x24;
//--------------------------------------------------------------------------------
//
//    appendReplacement
//
//--------------------------------------------------------------------------------
RegexMatcher &RegexMatcher::appendReplacement(UnicodeString &dest,
                                              const UnicodeString &replacement,
                                              UErrorCode &status) {
    UText replacementText = UTEXT_INITIALIZER;
    
    utext_openConstUnicodeString(&replacementText, &replacement, &status);
    if (U_SUCCESS(status)) {        
        UText resultText = UTEXT_INITIALIZER;
        utext_openUnicodeString(&resultText, &dest, &status);
        
        if (U_SUCCESS(status)) {
            appendReplacement(&resultText, &replacementText, status);
            utext_close(&resultText);
        }
        utext_close(&replacementText);
    }
    
    return *this;
}

//
//    appendReplacement, UText mode
//
RegexMatcher &RegexMatcher::appendReplacement(UText *dest,
                                              UText *replacement,
                                              UErrorCode &status) {
    if (U_FAILURE(status)) {
        return *this;
    }
    if (U_FAILURE(fDeferredStatus)) {
        status = fDeferredStatus;
        return *this;
    }
    if (fMatch == FALSE) {
        status = U_REGEX_INVALID_STATE;
        return *this;
    }
    
    // Copy input string from the end of previous match to start of current match
    int64_t  destLen = utext_nativeLength(dest);
    if (fMatchStart > fAppendPosition) {
        if (UTEXT_FULL_TEXT_IN_CHUNK(fInputText, fInputLength)) {
            destLen += utext_replace(dest, destLen, destLen, fInputText->chunkContents+fAppendPosition, 
                                     (int32_t)(fMatchStart-fAppendPosition), &status);
        } else {
            int32_t len16;
            if (UTEXT_USES_U16(fInputText)) {
                len16 = (int32_t)(fMatchStart-fAppendPosition);
            } else {
                UErrorCode lengthStatus = U_ZERO_ERROR;
                len16 = utext_extract(fInputText, fAppendPosition, fMatchStart, NULL, 0, &lengthStatus);
            }
            UChar *inputChars = (UChar *)uprv_malloc(sizeof(UChar)*(len16+1));
            if (inputChars == NULL) {
                status = U_MEMORY_ALLOCATION_ERROR;
                return *this;
            }
            utext_extract(fInputText, fAppendPosition, fMatchStart, inputChars, len16+1, &status);
            destLen += utext_replace(dest, destLen, destLen, inputChars, len16, &status);
            uprv_free(inputChars);
        }
    }
    fAppendPosition = fMatchEnd;
    
    
    // scan the replacement text, looking for substitutions ($n) and \escapes.
    //  TODO:  optimize this loop by efficiently scanning for '$' or '\',
    //         move entire ranges not containing substitutions.
    UTEXT_SETNATIVEINDEX(replacement, 0);
    UChar32 c = UTEXT_NEXT32(replacement);
    while (c != U_SENTINEL) {
        if (c == BACKSLASH) {
            // Backslash Escape.  Copy the following char out without further checks.
            //                    Note:  Surrogate pairs don't need any special handling
            //                           The second half wont be a '$' or a '\', and
            //                           will move to the dest normally on the next
            //                           loop iteration.
            c = UTEXT_CURRENT32(replacement);
            if (c == U_SENTINEL) {
                break;
            }
            
            if (c==0x55/*U*/ || c==0x75/*u*/) {
                // We have a \udddd or \Udddddddd escape sequence.
                int32_t offset = 0;
                struct URegexUTextUnescapeCharContext context = U_REGEX_UTEXT_UNESCAPE_CONTEXT(replacement);
                UChar32 escapedChar = u_unescapeAt(uregex_utext_unescape_charAt, &offset, INT32_MAX, &context);
                if (escapedChar != (UChar32)0xFFFFFFFF) {
                    if (U_IS_BMP(escapedChar)) {
                        UChar c16 = (UChar)escapedChar;
                        destLen += utext_replace(dest, destLen, destLen, &c16, 1, &status);
                    } else {
                        UChar surrogate[2];
                        surrogate[0] = U16_LEAD(escapedChar);
                        surrogate[1] = U16_TRAIL(escapedChar);
                        if (U_SUCCESS(status)) {
                            destLen += utext_replace(dest, destLen, destLen, surrogate, 2, &status);
                        }
                    }
                    // TODO:  Report errors for mal-formed \u escapes?
                    //        As this is, the original sequence is output, which may be OK.
                    if (context.lastOffset == offset) {
                        UTEXT_PREVIOUS32(replacement);
                    } else if (context.lastOffset != offset-1) {
                        utext_moveIndex32(replacement, offset - context.lastOffset - 1);
                    }
                }
            } else {
                UTEXT_NEXT32(replacement);
                // Plain backslash escape.  Just put out the escaped character.
                if (U_IS_BMP(c)) {
                    UChar c16 = (UChar)c;
                    destLen += utext_replace(dest, destLen, destLen, &c16, 1, &status);
                } else {
                    UChar surrogate[2];
                    surrogate[0] = U16_LEAD(c);
                    surrogate[1] = U16_TRAIL(c);
                    if (U_SUCCESS(status)) {
                        destLen += utext_replace(dest, destLen, destLen, surrogate, 2, &status);
                    }
                }
            }
        } else if (c != DOLLARSIGN) {
            // Normal char, not a $.  Copy it out without further checks.
            if (U_IS_BMP(c)) {
                UChar c16 = (UChar)c;
                destLen += utext_replace(dest, destLen, destLen, &c16, 1, &status);
            } else {
                UChar surrogate[2];
                surrogate[0] = U16_LEAD(c);
                surrogate[1] = U16_TRAIL(c);
                if (U_SUCCESS(status)) {
                    destLen += utext_replace(dest, destLen, destLen, surrogate, 2, &status);
                }
            }
        } else {
            // We've got a $.  Pick up a capture group number if one follows.
            // Consume at most the number of digits necessary for the largest capture
            // number that is valid for this pattern.
            
            int32_t numDigits = 0;
            int32_t groupNum  = 0;
            UChar32 digitC;
            for (;;) {
                digitC = UTEXT_CURRENT32(replacement);
                if (digitC == U_SENTINEL) {
                    break;
                }
                if (u_isdigit(digitC) == FALSE) {
                    break;
                }
                UTEXT_NEXT32(replacement);
                groupNum=groupNum*10 + u_charDigitValue(digitC);
                numDigits++;
                if (numDigits >= fPattern->fMaxCaptureDigits) {
                    break;
                }
            }
            
            
            if (numDigits == 0) {
                // The $ didn't introduce a group number at all.
                // Treat it as just part of the substitution text.
                UChar c16 = DOLLARSIGN;
                destLen += utext_replace(dest, destLen, destLen, &c16, 1, &status);
            } else {
                // Finally, append the capture group data to the destination.
                destLen += appendGroup(groupNum, dest, status);
                if (U_FAILURE(status)) {
                    // Can fail if group number is out of range.
                    break;
                }
            }
        }
        
        if (U_FAILURE(status)) {
            break;
        } else {
            c = UTEXT_NEXT32(replacement);
        }
    }
    
    return *this;
}



//--------------------------------------------------------------------------------
//
//    appendTail     Intended to be used in conjunction with appendReplacement()
//                   To the destination string, append everything following
//                   the last match position from the input string.
//
//                   Note:  Match ranges do not affect appendTail or appendReplacement
//
//--------------------------------------------------------------------------------
UnicodeString &RegexMatcher::appendTail(UnicodeString &dest) {
    UErrorCode status = U_ZERO_ERROR;
    UText resultText = UTEXT_INITIALIZER;
    utext_openUnicodeString(&resultText, &dest, &status);
    
    if (U_SUCCESS(status)) {
        appendTail(&resultText, status);
        utext_close(&resultText);
    }
    
    return dest;
}

//
//   appendTail, UText mode
//
UText *RegexMatcher::appendTail(UText *dest, UErrorCode &status) {
    UBool bailOut = FALSE;
    if (U_FAILURE(status)) {
        bailOut = TRUE;
    }
    if (U_FAILURE(fDeferredStatus)) {
        status = fDeferredStatus;
        bailOut = TRUE;
    }
    
    if (bailOut) {
        //  dest must not be NULL
        if (dest) {
            utext_replace(dest, utext_nativeLength(dest), utext_nativeLength(dest), NULL, 0, &status);
            return dest;
        }
    }
    
    if (fInputLength > fAppendPosition) {
        if (UTEXT_FULL_TEXT_IN_CHUNK(fInputText, fInputLength)) {
            int64_t destLen = utext_nativeLength(dest);
            utext_replace(dest, destLen, destLen, fInputText->chunkContents+fAppendPosition, 
                          (int32_t)(fInputLength-fAppendPosition), &status);
        } else {
            int32_t len16;
            if (UTEXT_USES_U16(fInputText)) {
                len16 = (int32_t)(fInputLength-fAppendPosition);
            } else {
                len16 = utext_extract(fInputText, fAppendPosition, fInputLength, NULL, 0, &status);
                status = U_ZERO_ERROR; // buffer overflow
            }
            
            UChar *inputChars = (UChar *)uprv_malloc(sizeof(UChar)*(len16));
            if (inputChars == NULL) {
                fDeferredStatus = U_MEMORY_ALLOCATION_ERROR;
            } else {
                utext_extract(fInputText, fAppendPosition, fInputLength, inputChars, len16, &status); // unterminated 
                int64_t destLen = utext_nativeLength(dest);
                utext_replace(dest, destLen, destLen, inputChars, len16, &status);
                uprv_free(inputChars);
            }
        }
    }
    return dest;
}



//--------------------------------------------------------------------------------
//
//   end
//
//--------------------------------------------------------------------------------
int32_t RegexMatcher::end(UErrorCode &err) const {
    return end(0, err);
}

int64_t RegexMatcher::end64(UErrorCode &err) const {
    return end64(0, err);
}

int64_t RegexMatcher::end64(int32_t group, UErrorCode &err) const {
    if (U_FAILURE(err)) {
        return -1;
    }
    if (fMatch == FALSE) {
        err = U_REGEX_INVALID_STATE;
        return -1;
    }
    if (group < 0 || group > fPattern->fGroupMap->size()) {
        err = U_INDEX_OUTOFBOUNDS_ERROR;
        return -1;
    }
    int64_t e = -1;
    if (group == 0) {
        e = fMatchEnd; 
    } else {
        // Get the position within the stack frame of the variables for
        //    this capture group.
        int32_t groupOffset = fPattern->fGroupMap->elementAti(group-1);
        U_ASSERT(groupOffset < fPattern->fFrameSize);
        U_ASSERT(groupOffset >= 0);
        e = fFrame->fExtra[groupOffset + 1];
    }
    
        return e;
}

int32_t RegexMatcher::end(int32_t group, UErrorCode &err) const {
    return (int32_t)end64(group, err);
}


//--------------------------------------------------------------------------------
//
//   find()
//
//--------------------------------------------------------------------------------
UBool RegexMatcher::find() {
    // Start at the position of the last match end.  (Will be zero if the
    //   matcher has been reset.)
    //
    if (U_FAILURE(fDeferredStatus)) {
        return FALSE;
    }
    
    if (UTEXT_FULL_TEXT_IN_CHUNK(fInputText, fInputLength)) {
        return findUsingChunk();
    }

    int64_t startPos = fMatchEnd;
    if (startPos==0) {
        startPos = fActiveStart;
    }

    if (fMatch) {
        // Save the position of any previous successful match.
        fLastMatchEnd = fMatchEnd;

        if (fMatchStart == fMatchEnd) {
            // Previous match had zero length.  Move start position up one position
            //  to avoid sending find() into a loop on zero-length matches.
            if (startPos >= fActiveLimit) {
                fMatch = FALSE;
                fHitEnd = TRUE;
                return FALSE;
            }
            UTEXT_SETNATIVEINDEX(fInputText, startPos);
            UTEXT_NEXT32(fInputText);
            startPos = UTEXT_GETNATIVEINDEX(fInputText);
        }
    } else {
        if (fLastMatchEnd >= 0) {
            // A previous find() failed to match.  Don't try again.
            //   (without this test, a pattern with a zero-length match
            //    could match again at the end of an input string.)
            fHitEnd = TRUE;
            return FALSE;
        }
    }


    // Compute the position in the input string beyond which a match can not begin, because
    //   the minimum length match would extend past the end of the input.
    //   Note:  some patterns that cannot match anything will have fMinMatchLength==Max Int.
    //          Be aware of possible overflows if making changes here.
    int64_t testStartLimit;
    if (UTEXT_USES_U16(fInputText)) {
        testStartLimit = fActiveLimit - fPattern->fMinMatchLen;
        if (startPos > testStartLimit) {
            fMatch = FALSE;
            fHitEnd = TRUE;
            return FALSE;
        }
    } else {
        // For now, let the matcher discover that it can't match on its own
        // We don't know how long the match len is in native characters
        testStartLimit = fActiveLimit;
    }

    UChar32  c;
    U_ASSERT(startPos >= 0);

    switch (fPattern->fStartType) {
    case START_NO_INFO:
        // No optimization was found. 
        //  Try a match at each input position.
        for (;;) {
            MatchAt(startPos, FALSE, fDeferredStatus);
            if (U_FAILURE(fDeferredStatus)) {
                return FALSE;
            }
            if (fMatch) {
                return TRUE;
            }
            if (startPos >= testStartLimit) {
                fHitEnd = TRUE;
                return FALSE;
            }
            UTEXT_SETNATIVEINDEX(fInputText, startPos);
            UTEXT_NEXT32(fInputText);
            startPos = UTEXT_GETNATIVEINDEX(fInputText);
            // Note that it's perfectly OK for a pattern to have a zero-length
            //   match at the end of a string, so we must make sure that the loop
            //   runs with startPos == testStartLimit the last time through.
            if  (REGEXFINDPROGRESS_INTERRUPT(startPos, fDeferredStatus))
                return FALSE;
        }
        U_ASSERT(FALSE);

    case START_START:
        // Matches are only possible at the start of the input string
        //   (pattern begins with ^ or \A)
        if (startPos > fActiveStart) {
            fMatch = FALSE;
            return FALSE;
        }
        MatchAt(startPos, FALSE, fDeferredStatus);
        if (U_FAILURE(fDeferredStatus)) {
            return FALSE;
        }
        return fMatch;


    case START_SET:
        {
            // Match may start on any char from a pre-computed set.
            U_ASSERT(fPattern->fMinMatchLen > 0);
            int64_t pos;
            UTEXT_SETNATIVEINDEX(fInputText, startPos);
            for (;;) {
                c = UTEXT_NEXT32(fInputText);
                pos = UTEXT_GETNATIVEINDEX(fInputText);
                // c will be -1 (U_SENTINEL) at end of text, in which case we
                // skip this next block (so we don't have a negative array index)
                // and handle end of text in the following block.
                if (c >= 0 && ((c<256 && fPattern->fInitialChars8->contains(c)) ||
                              (c>=256 && fPattern->fInitialChars->contains(c)))) {
                    MatchAt(startPos, FALSE, fDeferredStatus);
                    if (U_FAILURE(fDeferredStatus)) {
                        return FALSE;
                    }
                    if (fMatch) {
                        return TRUE;
                    }
                    UTEXT_SETNATIVEINDEX(fInputText, pos);
                }
                if (startPos >= testStartLimit) {
                    fMatch = FALSE;
                    fHitEnd = TRUE;
                    return FALSE;
                }
                startPos = pos;
	            if  (REGEXFINDPROGRESS_INTERRUPT(startPos, fDeferredStatus))
                    return FALSE;
            }
        }
        U_ASSERT(FALSE);

    case START_STRING:
    case START_CHAR:
        {
            // Match starts on exactly one char.
            U_ASSERT(fPattern->fMinMatchLen > 0);
            UChar32 theChar = fPattern->fInitialChar;
            int64_t pos;
            UTEXT_SETNATIVEINDEX(fInputText, startPos);
            for (;;) {
                c = UTEXT_NEXT32(fInputText);
                pos = UTEXT_GETNATIVEINDEX(fInputText);
                if (c == theChar) {
                    MatchAt(startPos, FALSE, fDeferredStatus);
                    if (U_FAILURE(fDeferredStatus)) {
                        return FALSE;
                    }
                    if (fMatch) {
                        return TRUE;
                    }
                    UTEXT_SETNATIVEINDEX(fInputText, pos);
                }
                if (startPos >= testStartLimit) {
                    fMatch = FALSE;
                    fHitEnd = TRUE;
                    return FALSE;
                }
                startPos = pos;
	            if  (REGEXFINDPROGRESS_INTERRUPT(startPos, fDeferredStatus))
                    return FALSE;
           }
        }
        U_ASSERT(FALSE);

    case START_LINE:
        {
            UChar32  c;
            if (startPos == fAnchorStart) {
                MatchAt(startPos, FALSE, fDeferredStatus);
                if (U_FAILURE(fDeferredStatus)) {
                    return FALSE;
                }
                if (fMatch) {
                    return TRUE;
                }
                UTEXT_SETNATIVEINDEX(fInputText, startPos);
                c = UTEXT_NEXT32(fInputText);
                startPos = UTEXT_GETNATIVEINDEX(fInputText);
            } else {
                UTEXT_SETNATIVEINDEX(fInputText, startPos);
                c = UTEXT_PREVIOUS32(fInputText);
                UTEXT_SETNATIVEINDEX(fInputText, startPos);
            }

            if (fPattern->fFlags & UREGEX_UNIX_LINES) {
                for (;;) {
                    if (c == 0x0a) {
                            MatchAt(startPos, FALSE, fDeferredStatus);
                            if (U_FAILURE(fDeferredStatus)) {
                                return FALSE;
                            }
                            if (fMatch) {
                                return TRUE;
                            }
                            UTEXT_SETNATIVEINDEX(fInputText, startPos);
                    }
                    if (startPos >= testStartLimit) {
                        fMatch = FALSE;
                        fHitEnd = TRUE;
                        return FALSE;
                    }
                    c = UTEXT_NEXT32(fInputText);
                    startPos = UTEXT_GETNATIVEINDEX(fInputText);
                    // Note that it's perfectly OK for a pattern to have a zero-length
                    //   match at the end of a string, so we must make sure that the loop
                    //   runs with startPos == testStartLimit the last time through.
		            if  (REGEXFINDPROGRESS_INTERRUPT(startPos, fDeferredStatus))
                        return FALSE;
                }
            } else {
                for (;;) {
                    if (((c & 0x7f) <= 0x29) &&     // First quickly bypass as many chars as possible
                        ((c<=0x0d && c>=0x0a) || c==0x85 ||c==0x2028 || c==0x2029 )) {
                            if (c == 0x0d && startPos < fActiveLimit && UTEXT_CURRENT32(fInputText) == 0x0a) {
                                UTEXT_NEXT32(fInputText);
                                startPos = UTEXT_GETNATIVEINDEX(fInputText);
                            }
                            MatchAt(startPos, FALSE, fDeferredStatus);
                            if (U_FAILURE(fDeferredStatus)) {
                                return FALSE;
                            }
                            if (fMatch) {
                                return TRUE;
                            }
                            UTEXT_SETNATIVEINDEX(fInputText, startPos);
                    }
                    if (startPos >= testStartLimit) {
                        fMatch = FALSE;
                        fHitEnd = TRUE;
                        return FALSE;
                    }
                    c = UTEXT_NEXT32(fInputText);
                    startPos = UTEXT_GETNATIVEINDEX(fInputText);
                    // Note that it's perfectly OK for a pattern to have a zero-length
                    //   match at the end of a string, so we must make sure that the loop
                    //   runs with startPos == testStartLimit the last time through.
		            if  (REGEXFINDPROGRESS_INTERRUPT(startPos, fDeferredStatus))
                        return FALSE;
                }
            }
        }

    default:
        U_ASSERT(FALSE);
    }

    U_ASSERT(FALSE);
    return FALSE;
}



UBool RegexMatcher::find(int64_t start, UErrorCode &status) {
    if (U_FAILURE(status)) {
        return FALSE;
    }
    if (U_FAILURE(fDeferredStatus)) {
        status = fDeferredStatus;
        return FALSE;
    }
    this->reset();                        // Note:  Reset() is specified by Java Matcher documentation.
                                          //        This will reset the region to be the full input length.
    if (start < 0) {
        status = U_INDEX_OUTOFBOUNDS_ERROR;
        return FALSE;
    }
    
    int64_t nativeStart = start;
    if (nativeStart < fActiveStart || nativeStart > fActiveLimit) {
        status = U_INDEX_OUTOFBOUNDS_ERROR;
        return FALSE;
    }
    fMatchEnd = nativeStart;  
    return find();
}


//--------------------------------------------------------------------------------
//
//   findUsingChunk() -- like find(), but with the advance knowledge that the
//                       entire string is available in the UText's chunk buffer.
//
//--------------------------------------------------------------------------------
UBool RegexMatcher::findUsingChunk() {
    // Start at the position of the last match end.  (Will be zero if the
    //   matcher has been reset.
    //

    int32_t startPos = (int32_t)fMatchEnd;
    if (startPos==0) {
        startPos = (int32_t)fActiveStart;
    }
    
    const UChar *inputBuf = fInputText->chunkContents;

    if (fMatch) {
        // Save the position of any previous successful match.
        fLastMatchEnd = fMatchEnd;
        
        if (fMatchStart == fMatchEnd) {
            // Previous match had zero length.  Move start position up one position
            //  to avoid sending find() into a loop on zero-length matches.
            if (startPos >= fActiveLimit) {
                fMatch = FALSE;
                fHitEnd = TRUE;
                return FALSE;
            }
            U16_FWD_1(inputBuf, startPos, fInputLength);
        }
    } else {
        if (fLastMatchEnd >= 0) {
            // A previous find() failed to match.  Don't try again.
            //   (without this test, a pattern with a zero-length match
            //    could match again at the end of an input string.)
            fHitEnd = TRUE;
            return FALSE;
        }
    }
    
    
    // Compute the position in the input string beyond which a match can not begin, because
    //   the minimum length match would extend past the end of the input.
    //   Note:  some patterns that cannot match anything will have fMinMatchLength==Max Int.
    //          Be aware of possible overflows if making changes here.
    int32_t testLen  = (int32_t)(fActiveLimit - fPattern->fMinMatchLen);
    if (startPos > testLen) {
        fMatch = FALSE;
        fHitEnd = TRUE;
        return FALSE;
    }
    
    UChar32  c;
    U_ASSERT(startPos >= 0);
    
    switch (fPattern->fStartType) {
    case START_NO_INFO:
        // No optimization was found. 
        //  Try a match at each input position.
        for (;;) {
            MatchChunkAt(startPos, FALSE, fDeferredStatus);
            if (U_FAILURE(fDeferredStatus)) {
                return FALSE;
            }
            if (fMatch) {
                return TRUE;
            }
            if (startPos >= testLen) {
                fHitEnd = TRUE;
                return FALSE;
            }
            U16_FWD_1(inputBuf, startPos, fActiveLimit);
            // Note that it's perfectly OK for a pattern to have a zero-length
            //   match at the end of a string, so we must make sure that the loop
            //   runs with startPos == testLen the last time through.
            if  (REGEXFINDPROGRESS_INTERRUPT(startPos, fDeferredStatus))
                return FALSE;
        }
        U_ASSERT(FALSE);
        
    case START_START:
        // Matches are only possible at the start of the input string
        //   (pattern begins with ^ or \A)
        if (startPos > fActiveStart) {
            fMatch = FALSE;
            return FALSE;
        }
        MatchChunkAt(startPos, FALSE, fDeferredStatus);
        if (U_FAILURE(fDeferredStatus)) {
            return FALSE;
        }
        return fMatch;
        
        
    case START_SET:
    {
        // Match may start on any char from a pre-computed set.
        U_ASSERT(fPattern->fMinMatchLen > 0);
        for (;;) {
            int32_t pos = startPos;
            U16_NEXT(inputBuf, startPos, fActiveLimit, c);  // like c = inputBuf[startPos++];
            if ((c<256 && fPattern->fInitialChars8->contains(c)) ||
                (c>=256 && fPattern->fInitialChars->contains(c))) {
                MatchChunkAt(pos, FALSE, fDeferredStatus);
                if (U_FAILURE(fDeferredStatus)) {
                    return FALSE;
                }
                if (fMatch) {
                    return TRUE;
                }
            }
            if (pos >= testLen) {
                fMatch = FALSE;
                fHitEnd = TRUE;
                return FALSE;
            }
            if  (REGEXFINDPROGRESS_INTERRUPT(startPos, fDeferredStatus))
                return FALSE;
        }
    }
        U_ASSERT(FALSE);
        
    case START_STRING:
    case START_CHAR:
    {
        // Match starts on exactly one char.
        U_ASSERT(fPattern->fMinMatchLen > 0);
        UChar32 theChar = fPattern->fInitialChar;
        for (;;) {
            int32_t pos = startPos;
            U16_NEXT(inputBuf, startPos, fActiveLimit, c);  // like c = inputBuf[startPos++];
            if (c == theChar) {
                MatchChunkAt(pos, FALSE, fDeferredStatus);
                if (U_FAILURE(fDeferredStatus)) {
                    return FALSE;
                }
                if (fMatch) {
                    return TRUE;
                }
            }
            if (pos >= testLen) {
                fMatch = FALSE;
                fHitEnd = TRUE;
                return FALSE;
            }
            if  (REGEXFINDPROGRESS_INTERRUPT(startPos, fDeferredStatus))
                return FALSE;
        }
    }
        U_ASSERT(FALSE);
        
    case START_LINE:
    {
        UChar32  c;
        if (startPos == fAnchorStart) {
            MatchChunkAt(startPos, FALSE, fDeferredStatus);
            if (U_FAILURE(fDeferredStatus)) {
                return FALSE;
            }
            if (fMatch) {
                return TRUE;
            }
            U16_FWD_1(inputBuf, startPos, fActiveLimit);
        }
        
        if (fPattern->fFlags & UREGEX_UNIX_LINES) {
            for (;;) {
                c = inputBuf[startPos-1];
                if (c == 0x0a) {
                    MatchChunkAt(startPos, FALSE, fDeferredStatus);
                    if (U_FAILURE(fDeferredStatus)) {
                        return FALSE;
                    }
                    if (fMatch) {
                        return TRUE;
                    }
                }
                if (startPos >= testLen) {
                    fMatch = FALSE;
                    fHitEnd = TRUE;
                    return FALSE;
                }
                U16_FWD_1(inputBuf, startPos, fActiveLimit);
                // Note that it's perfectly OK for a pattern to have a zero-length
                //   match at the end of a string, so we must make sure that the loop
                //   runs with startPos == testLen the last time through.
	            if  (REGEXFINDPROGRESS_INTERRUPT(startPos, fDeferredStatus))
                    return FALSE;
            }
        } else {
            for (;;) {
                c = inputBuf[startPos-1];
                if (((c & 0x7f) <= 0x29) &&     // First quickly bypass as many chars as possible
                    ((c<=0x0d && c>=0x0a) || c==0x85 ||c==0x2028 || c==0x2029 )) {
                    if (c == 0x0d && startPos < fActiveLimit && inputBuf[startPos] == 0x0a) {
                        startPos++;
                    }
                    MatchChunkAt(startPos, FALSE, fDeferredStatus);
                    if (U_FAILURE(fDeferredStatus)) {
                        return FALSE;
                    }
                    if (fMatch) {
                        return TRUE;
                    }
                }
                if (startPos >= testLen) {
                    fMatch = FALSE;
                    fHitEnd = TRUE;
                    return FALSE;
                }
                U16_FWD_1(inputBuf, startPos, fActiveLimit);
                // Note that it's perfectly OK for a pattern to have a zero-length
                //   match at the end of a string, so we must make sure that the loop
                //   runs with startPos == testLen the last time through.
	            if  (REGEXFINDPROGRESS_INTERRUPT(startPos, fDeferredStatus))
                    return FALSE;
            }
        }
    }
        
    default:
        U_ASSERT(FALSE);
    }
    
    U_ASSERT(FALSE);
    return FALSE;
}



//--------------------------------------------------------------------------------
//
//  group()
//
//--------------------------------------------------------------------------------
UnicodeString RegexMatcher::group(UErrorCode &status) const {
    return group(0, status);
}

//  Return immutable shallow clone
UText *RegexMatcher::group(UText *dest, int64_t &group_len, UErrorCode &status) const {
    return group(0, dest, group_len, status);
}

//  Return immutable shallow clone
UText *RegexMatcher::group(int32_t groupNum, UText *dest, int64_t &group_len, UErrorCode &status) const {
    group_len = 0;
    UBool bailOut = FALSE;
    if (U_FAILURE(status)) {
        return dest;
    }
    if (U_FAILURE(fDeferredStatus)) {
        status = fDeferredStatus;
        bailOut = TRUE;
    }
    if (fMatch == FALSE) {
        status = U_REGEX_INVALID_STATE;
        bailOut = TRUE;
    }
    if (groupNum < 0 || groupNum > fPattern->fGroupMap->size()) {
        status = U_INDEX_OUTOFBOUNDS_ERROR;
        bailOut = TRUE;
    }
    
    if (bailOut) {
        return (dest) ? dest : utext_openUChars(NULL, NULL, 0, &status);
    }
    
    int64_t s, e;
    if (groupNum == 0) {
        s = fMatchStart;
        e = fMatchEnd;
    } else {
        int32_t groupOffset = fPattern->fGroupMap->elementAti(groupNum-1);
        U_ASSERT(groupOffset < fPattern->fFrameSize);
        U_ASSERT(groupOffset >= 0);
        s = fFrame->fExtra[groupOffset];
        e = fFrame->fExtra[groupOffset+1];
    }

    if (s < 0) {
        // A capture group wasn't part of the match
        return utext_clone(dest, fInputText, FALSE, TRUE, &status);
    }
    U_ASSERT(s <= e);
    group_len = e - s;
    
    dest = utext_clone(dest, fInputText, FALSE, TRUE, &status);
    if (dest)
        UTEXT_SETNATIVEINDEX(dest, s);
    return dest;
}

UnicodeString RegexMatcher::group(int32_t groupNum, UErrorCode &status) const {
    UnicodeString result;
    if (U_FAILURE(status)) {
        return result;
    }
    UText resultText = UTEXT_INITIALIZER;
    utext_openUnicodeString(&resultText, &result, &status);
    group(groupNum, &resultText, status);
    utext_close(&resultText);
    return result;
}


//  Return deep (mutable) clone
//		Technology Preview (as an API), but note that the UnicodeString API is implemented
//		using this function.
UText *RegexMatcher::group(int32_t groupNum, UText *dest, UErrorCode &status) const {
    UBool bailOut = FALSE;
    if (U_FAILURE(status)) {
        return dest;
    }
    if (U_FAILURE(fDeferredStatus)) {
        status = fDeferredStatus;
        bailOut = TRUE;
    }
    
    if (fMatch == FALSE) {
        status = U_REGEX_INVALID_STATE;
        bailOut = TRUE;
    }
    if (groupNum < 0 || groupNum > fPattern->fGroupMap->size()) {
        status = U_INDEX_OUTOFBOUNDS_ERROR;
        bailOut = TRUE;
    }
    
    if (bailOut) {
        if (dest) {
            utext_replace(dest, 0, utext_nativeLength(dest), NULL, 0, &status);
            return dest;
        } else {
            return utext_openUChars(NULL, NULL, 0, &status);
        }
    }
    
    int64_t s, e;
    if (groupNum == 0) {
        s = fMatchStart;
        e = fMatchEnd;
    } else {
        int32_t groupOffset = fPattern->fGroupMap->elementAti(groupNum-1);
        U_ASSERT(groupOffset < fPattern->fFrameSize);
        U_ASSERT(groupOffset >= 0);
        s = fFrame->fExtra[groupOffset];
        e = fFrame->fExtra[groupOffset+1];
    }
    
    if (s < 0) {
        // A capture group wasn't part of the match 
        if (dest) {
            utext_replace(dest, 0, utext_nativeLength(dest), NULL, 0, &status);
            return dest;
        } else {
            return utext_openUChars(NULL, NULL, 0, &status);
        }
    }
    U_ASSERT(s <= e);
    
    if (UTEXT_FULL_TEXT_IN_CHUNK(fInputText, fInputLength)) {
        U_ASSERT(e <= fInputLength);
        if (dest) {
            utext_replace(dest, 0, utext_nativeLength(dest), fInputText->chunkContents+s, (int32_t)(e-s), &status);
        } else {
            UText groupText = UTEXT_INITIALIZER;
            utext_openUChars(&groupText, fInputText->chunkContents+s, e-s, &status);
            dest = utext_clone(NULL, &groupText, TRUE, FALSE, &status);
            utext_close(&groupText);
        }
    } else {
        int32_t len16;
        if (UTEXT_USES_U16(fInputText)) {
            len16 = (int32_t)(e-s);
        } else {
            UErrorCode lengthStatus = U_ZERO_ERROR;
            len16 = utext_extract(fInputText, s, e, NULL, 0, &lengthStatus);
        }
        UChar *groupChars = (UChar *)uprv_malloc(sizeof(UChar)*(len16+1));
        if (groupChars == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return dest;
        }
        utext_extract(fInputText, s, e, groupChars, len16+1, &status);

        if (dest) {
            utext_replace(dest, 0, utext_nativeLength(dest), groupChars, len16, &status);
        } else {
            UText groupText = UTEXT_INITIALIZER;
            utext_openUChars(&groupText, groupChars, len16, &status);
            dest = utext_clone(NULL, &groupText, TRUE, FALSE, &status);
            utext_close(&groupText);
        }
        
        uprv_free(groupChars);
    }
    return dest;
}

//--------------------------------------------------------------------------------
//
//  appendGroup() -- currently internal only, appends a group to a UText rather
//                   than replacing its contents
//
//--------------------------------------------------------------------------------

int64_t RegexMatcher::appendGroup(int32_t groupNum, UText *dest, UErrorCode &status) const {
    if (U_FAILURE(status)) {
        return 0;
    }
    if (U_FAILURE(fDeferredStatus)) {
        status = fDeferredStatus;
        return 0;
    }
    int64_t destLen = utext_nativeLength(dest);
    
    if (fMatch == FALSE) {
        status = U_REGEX_INVALID_STATE;
        return utext_replace(dest, destLen, destLen, NULL, 0, &status);
    }
    if (groupNum < 0 || groupNum > fPattern->fGroupMap->size()) {
        status = U_INDEX_OUTOFBOUNDS_ERROR;
        return utext_replace(dest, destLen, destLen, NULL, 0, &status);
    }
    
    int64_t s, e;
    if (groupNum == 0) {
        s = fMatchStart;
        e = fMatchEnd;
    } else {
        int32_t groupOffset = fPattern->fGroupMap->elementAti(groupNum-1);
        U_ASSERT(groupOffset < fPattern->fFrameSize);
        U_ASSERT(groupOffset >= 0);
        s = fFrame->fExtra[groupOffset];
        e = fFrame->fExtra[groupOffset+1];
    }
    
    if (s < 0) {
        // A capture group wasn't part of the match 
        return utext_replace(dest, destLen, destLen, NULL, 0, &status);
    }
    U_ASSERT(s <= e);
    
    int64_t deltaLen;
    if (UTEXT_FULL_TEXT_IN_CHUNK(fInputText, fInputLength)) {
        U_ASSERT(e <= fInputLength);
        deltaLen = utext_replace(dest, destLen, destLen, fInputText->chunkContents+s, (int32_t)(e-s), &status);
    } else {
        int32_t len16;
        if (UTEXT_USES_U16(fInputText)) {
            len16 = (int32_t)(e-s);
        } else {
            UErrorCode lengthStatus = U_ZERO_ERROR;
            len16 = utext_extract(fInputText, s, e, NULL, 0, &lengthStatus);
        }
        UChar *groupChars = (UChar *)uprv_malloc(sizeof(UChar)*(len16+1));
        if (groupChars == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return 0;
        }
        utext_extract(fInputText, s, e, groupChars, len16+1, &status);
    
        deltaLen = utext_replace(dest, destLen, destLen, groupChars, len16, &status);
        uprv_free(groupChars);
    }
    return deltaLen;
}



//--------------------------------------------------------------------------------
//
//  groupCount()
//
//--------------------------------------------------------------------------------
int32_t RegexMatcher::groupCount() const {
    return fPattern->fGroupMap->size();
}



//--------------------------------------------------------------------------------
//
//  hasAnchoringBounds()
//
//--------------------------------------------------------------------------------
UBool RegexMatcher::hasAnchoringBounds() const {
    return fAnchoringBounds;
}


//--------------------------------------------------------------------------------
//
//  hasTransparentBounds()
//
//--------------------------------------------------------------------------------
UBool RegexMatcher::hasTransparentBounds() const {
    return fTransparentBounds;
}



//--------------------------------------------------------------------------------
//
//  hitEnd()
//
//--------------------------------------------------------------------------------
UBool RegexMatcher::hitEnd() const {
    return fHitEnd;
}


//--------------------------------------------------------------------------------
//
//  input()
//
//--------------------------------------------------------------------------------
const UnicodeString &RegexMatcher::input() const {
    if (!fInput) {
        UErrorCode status = U_ZERO_ERROR;
        int32_t len16;
        if (UTEXT_USES_U16(fInputText)) {
            len16 = (int32_t)fInputLength;
        } else {
            len16 = utext_extract(fInputText, 0, fInputLength, NULL, 0, &status);
            status = U_ZERO_ERROR; // overflow, length status
        }
        UnicodeString *result = new UnicodeString(len16, 0, 0);
        
        UChar *inputChars = result->getBuffer(len16);
        utext_extract(fInputText, 0, fInputLength, inputChars, len16, &status); // unterminated warning
        result->releaseBuffer(len16);
        
        (*(const UnicodeString **)&fInput) = result; // pointer assignment, rather than operator=
    }
    
    return *fInput;
}

//--------------------------------------------------------------------------------
//
//  inputText()
//
//--------------------------------------------------------------------------------
UText *RegexMatcher::inputText() const {
    return fInputText;
}


//--------------------------------------------------------------------------------
//
//  getInput() -- like inputText(), but makes a clone or copies into another UText
//
//--------------------------------------------------------------------------------
UText *RegexMatcher::getInput (UText *dest, UErrorCode &status) const {
    UBool bailOut = FALSE;
    if (U_FAILURE(status)) {
        return dest;
    }
    if (U_FAILURE(fDeferredStatus)) {
        status = fDeferredStatus;
        bailOut = TRUE;
    }
    
    if (bailOut) {
        if (dest) {
            utext_replace(dest, 0, utext_nativeLength(dest), NULL, 0, &status);
            return dest;
        } else {
            return utext_clone(NULL, fInputText, FALSE, TRUE, &status);
        }
    }
    
    if (dest) {
        if (UTEXT_FULL_TEXT_IN_CHUNK(fInputText, fInputLength)) {
            utext_replace(dest, 0, utext_nativeLength(dest), fInputText->chunkContents, (int32_t)fInputLength, &status);
        } else {
            int32_t input16Len;
            if (UTEXT_USES_U16(fInputText)) {
                input16Len = (int32_t)fInputLength;
            } else {
                UErrorCode lengthStatus = U_ZERO_ERROR;
                input16Len = utext_extract(fInputText, 0, fInputLength, NULL, 0, &lengthStatus); // buffer overflow error
            }
            UChar *inputChars = (UChar *)uprv_malloc(sizeof(UChar)*(input16Len));
            if (inputChars == NULL) {
                return dest;
            }
            
            status = U_ZERO_ERROR;
            utext_extract(fInputText, 0, fInputLength, inputChars, input16Len, &status); // not terminated warning
            status = U_ZERO_ERROR;
            utext_replace(dest, 0, utext_nativeLength(dest), inputChars, input16Len, &status);
            
            uprv_free(inputChars);
        }
        return dest;
    } else {
        return utext_clone(NULL, fInputText, FALSE, TRUE, &status);
    }
}


static UBool compat_SyncMutableUTextContents(UText *ut);
static UBool compat_SyncMutableUTextContents(UText *ut) {
    UBool retVal = FALSE;
    
    //  In the following test, we're really only interested in whether the UText should switch
    //  between heap and stack allocation.  If length hasn't changed, we won't, so the chunkContents
    //  will still point to the correct data.
    if (utext_nativeLength(ut) != ut->nativeIndexingLimit) {
        UnicodeString *us=(UnicodeString *)ut->context;
    
        // Update to the latest length.
        // For example, (utext_nativeLength(ut) != ut->nativeIndexingLimit).
        int32_t newLength = us->length();
    
        // Update the chunk description.
        // The buffer may have switched between stack- and heap-based.
        ut->chunkContents    = us->getBuffer();
        ut->chunkLength      = newLength;
        ut->chunkNativeLimit = newLength;
        ut->nativeIndexingLimit = newLength;
        retVal = TRUE;
    }

    return retVal;
}

//--------------------------------------------------------------------------------
//
//  lookingAt()
//
//--------------------------------------------------------------------------------
UBool RegexMatcher::lookingAt(UErrorCode &status) {
    if (U_FAILURE(status)) {
        return FALSE;
    }
    if (U_FAILURE(fDeferredStatus)) {
        status = fDeferredStatus;
        return FALSE;
    }
    
    if (fInputUniStrMaybeMutable) {
        if (compat_SyncMutableUTextContents(fInputText)) {
        fInputLength = utext_nativeLength(fInputText);
        reset();
        }
    }
    else {
        resetPreserveRegion();
    }
    if (UTEXT_FULL_TEXT_IN_CHUNK(fInputText, fInputLength)) {
        MatchChunkAt((int32_t)fActiveStart, FALSE, status);
    } else {
        MatchAt(fActiveStart, FALSE, status);
    }
    return fMatch;
}


UBool RegexMatcher::lookingAt(int64_t start, UErrorCode &status) {
    if (U_FAILURE(status)) {
        return FALSE;
    }
    if (U_FAILURE(fDeferredStatus)) {
        status = fDeferredStatus;
        return FALSE;
    }
    reset();
    
    if (start < 0) {
        status = U_INDEX_OUTOFBOUNDS_ERROR;
        return FALSE;
    }
    
    if (fInputUniStrMaybeMutable) {
        if (compat_SyncMutableUTextContents(fInputText)) {
        fInputLength = utext_nativeLength(fInputText);
        reset();
        }
    }

    int64_t nativeStart;
    nativeStart = start;
    if (nativeStart < fActiveStart || nativeStart > fActiveLimit) {
        status = U_INDEX_OUTOFBOUNDS_ERROR;
        return FALSE;
    }
    
    if (UTEXT_FULL_TEXT_IN_CHUNK(fInputText, fInputLength)) {
        MatchChunkAt((int32_t)nativeStart, FALSE, status);
    } else {
        MatchAt(nativeStart, FALSE, status);
    }
    return fMatch;
}



//--------------------------------------------------------------------------------
//
//  matches()
//
//--------------------------------------------------------------------------------
UBool RegexMatcher::matches(UErrorCode &status) {
    if (U_FAILURE(status)) {
        return FALSE;
    }
    if (U_FAILURE(fDeferredStatus)) {
        status = fDeferredStatus;
        return FALSE;
    }

    if (fInputUniStrMaybeMutable) {
        if (compat_SyncMutableUTextContents(fInputText)) {
        fInputLength = utext_nativeLength(fInputText);
        reset();
        }
    }
    else {
        resetPreserveRegion();
    }

    if (UTEXT_FULL_TEXT_IN_CHUNK(fInputText, fInputLength)) {
        MatchChunkAt((int32_t)fActiveStart, TRUE, status);
    } else {
        MatchAt(fActiveStart, TRUE, status);
    }
    return fMatch;
}


UBool RegexMatcher::matches(int64_t start, UErrorCode &status) {
    if (U_FAILURE(status)) {
        return FALSE;
    }
    if (U_FAILURE(fDeferredStatus)) {
        status = fDeferredStatus;
        return FALSE;
    }
    reset();
    
    if (start < 0) {
        status = U_INDEX_OUTOFBOUNDS_ERROR;
        return FALSE;
    }

    if (fInputUniStrMaybeMutable) {
        if (compat_SyncMutableUTextContents(fInputText)) {
        fInputLength = utext_nativeLength(fInputText);
        reset();
        }
    }

    int64_t nativeStart;
    nativeStart = start;
    if (nativeStart < fActiveStart || nativeStart > fActiveLimit) {
        status = U_INDEX_OUTOFBOUNDS_ERROR;
        return FALSE;
    }

    if (UTEXT_FULL_TEXT_IN_CHUNK(fInputText, fInputLength)) {
        MatchChunkAt((int32_t)nativeStart, TRUE, status);
    } else {
        MatchAt(nativeStart, TRUE, status);
    }
    return fMatch;
}



//--------------------------------------------------------------------------------
//
//    pattern
//
//--------------------------------------------------------------------------------
const RegexPattern &RegexMatcher::pattern() const {
    return *fPattern;
}



//--------------------------------------------------------------------------------
//
//    region
//
//--------------------------------------------------------------------------------
RegexMatcher &RegexMatcher::region(int64_t regionStart, int64_t regionLimit, int64_t startIndex, UErrorCode &status) {
    if (U_FAILURE(status)) {
        return *this;
    }
    
    if (regionStart>regionLimit || regionStart<0 || regionLimit<0) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
    }
        
    int64_t nativeStart = regionStart;
    int64_t nativeLimit = regionLimit;
    if (nativeStart > fInputLength || nativeLimit > fInputLength) {
      status = U_ILLEGAL_ARGUMENT_ERROR;
    }

    if (startIndex == -1)
      this->reset();
    else
      resetPreserveRegion();    
    
    fRegionStart = nativeStart;
    fRegionLimit = nativeLimit;
    fActiveStart = nativeStart;
    fActiveLimit = nativeLimit;

    if (startIndex != -1) {
      if (startIndex < fActiveStart || startIndex > fActiveLimit) {
          status = U_INDEX_OUTOFBOUNDS_ERROR;
      }
      fMatchEnd = startIndex;  
    }

    if (!fTransparentBounds) {
        fLookStart = nativeStart;
        fLookLimit = nativeLimit;
    }
    if (fAnchoringBounds) {
        fAnchorStart = nativeStart;
        fAnchorLimit = nativeLimit;
    }
    return *this;
}

RegexMatcher &RegexMatcher::region(int64_t start, int64_t limit, UErrorCode &status) {
  return region(start, limit, -1, status);
}

//--------------------------------------------------------------------------------
//
//    regionEnd
//
//--------------------------------------------------------------------------------
int32_t RegexMatcher::regionEnd() const {
    return (int32_t)fRegionLimit;
}

int64_t RegexMatcher::regionEnd64() const {
    return fRegionLimit;
}

//--------------------------------------------------------------------------------
//
//    regionStart
//
//--------------------------------------------------------------------------------
int32_t RegexMatcher::regionStart() const {
    return (int32_t)fRegionStart;
}

int64_t RegexMatcher::regionStart64() const {
    return fRegionStart;
}


//--------------------------------------------------------------------------------
//
//    replaceAll
//
//--------------------------------------------------------------------------------
UnicodeString RegexMatcher::replaceAll(const UnicodeString &replacement, UErrorCode &status) {
    UText replacementText = UTEXT_INITIALIZER;
    UText resultText = UTEXT_INITIALIZER;
    UnicodeString resultString;
    if (U_FAILURE(status)) {
        return resultString;
    }
    
    utext_openConstUnicodeString(&replacementText, &replacement, &status);
    utext_openUnicodeString(&resultText, &resultString, &status);
        
    replaceAll(&replacementText, &resultText, status);

    utext_close(&resultText);
    utext_close(&replacementText);
    
    return resultString;
}


//
//    replaceAll, UText mode
//
UText *RegexMatcher::replaceAll(UText *replacement, UText *dest, UErrorCode &status) {
    if (U_FAILURE(status)) {
        return dest;
    }
    if (U_FAILURE(fDeferredStatus)) {
        status = fDeferredStatus;
        return dest;
    }
    
    if (dest == NULL) {
        UnicodeString emptyString;
        UText empty = UTEXT_INITIALIZER;
        
        utext_openUnicodeString(&empty, &emptyString, &status);
        dest = utext_clone(NULL, &empty, TRUE, FALSE, &status);
        utext_close(&empty);
    }

    if (U_SUCCESS(status)) {
        reset();
        while (find()) {
            appendReplacement(dest, replacement, status);
            if (U_FAILURE(status)) {
                break;
            }
        }
        appendTail(dest, status);
    }
    
    return dest;
}


//--------------------------------------------------------------------------------
//
//    replaceFirst
//
//--------------------------------------------------------------------------------
UnicodeString RegexMatcher::replaceFirst(const UnicodeString &replacement, UErrorCode &status) {
    UText replacementText = UTEXT_INITIALIZER;
    UText resultText = UTEXT_INITIALIZER;
    UnicodeString resultString;
    
    utext_openConstUnicodeString(&replacementText, &replacement, &status);
    utext_openUnicodeString(&resultText, &resultString, &status);
    
    replaceFirst(&replacementText, &resultText, status);
    
    utext_close(&resultText);
    utext_close(&replacementText);
    
    return resultString;
}

//
//    replaceFirst, UText mode
//
UText *RegexMatcher::replaceFirst(UText *replacement, UText *dest, UErrorCode &status) {
    if (U_FAILURE(status)) {
        return dest;
    }
    if (U_FAILURE(fDeferredStatus)) {
        status = fDeferredStatus;
        return dest;
    }

    reset();
    if (!find()) {
        return getInput(dest, status);
    }
    
    if (dest == NULL) {
        UnicodeString emptyString;
        UText empty = UTEXT_INITIALIZER;
        
        utext_openUnicodeString(&empty, &emptyString, &status);
        dest = utext_clone(NULL, &empty, TRUE, FALSE, &status);
        utext_close(&empty);
    }
    
    appendReplacement(dest, replacement, status);
    appendTail(dest, status);
    
    return dest;
}


//--------------------------------------------------------------------------------
//
//     requireEnd
//
//--------------------------------------------------------------------------------
UBool RegexMatcher::requireEnd() const {
    return fRequireEnd;
}


//--------------------------------------------------------------------------------
//
//     reset
//
//--------------------------------------------------------------------------------
RegexMatcher &RegexMatcher::reset() {
    fRegionStart    = 0;
    fRegionLimit    = fInputLength;
    fActiveStart    = 0;
    fActiveLimit    = fInputLength;
    fAnchorStart    = 0;
    fAnchorLimit    = fInputLength;
    fLookStart      = 0;
    fLookLimit      = fInputLength;
    resetPreserveRegion();
    return *this;
}



void RegexMatcher::resetPreserveRegion() {
    fMatchStart     = 0;
    fMatchEnd       = 0;
    fLastMatchEnd   = -1;
    fAppendPosition = 0;
    fMatch          = FALSE;
    fHitEnd         = FALSE;
    fRequireEnd     = FALSE;
    fTime           = 0;
    fTickCounter    = TIMER_INITIAL_VALUE;
    //resetStack(); // more expensive than it looks...
}


RegexMatcher &RegexMatcher::reset(const UnicodeString &input) {
    fInputText = utext_openConstUnicodeString(fInputText, &input, &fDeferredStatus);
    if (fPattern->fNeedsAltInput) {
        fAltInputText = utext_clone(fAltInputText, fInputText, FALSE, TRUE, &fDeferredStatus);
    }
    fInputLength = utext_nativeLength(fInputText);
    
    reset();
    delete fInput;
    fInput = NULL;

    //  Do the following for any UnicodeString.
    //  This is for compatibility for those clients who modify the input string "live" during regex operations.
    fInputUniStrMaybeMutable = TRUE;    
    
    if (fWordBreakItr != NULL) {
#if UCONFIG_NO_BREAK_ITERATION==0
        UErrorCode status = U_ZERO_ERROR;
        fWordBreakItr->setText(fInputText, status);
#endif
    }
    return *this;
}


RegexMatcher &RegexMatcher::reset(UText *input) {
    if (fInputText != input) {
        fInputText = utext_clone(fInputText, input, FALSE, TRUE, &fDeferredStatus);
        if (fPattern->fNeedsAltInput) fAltInputText = utext_clone(fAltInputText, fInputText, FALSE, TRUE, &fDeferredStatus);
        fInputLength = utext_nativeLength(fInputText);
    
        delete fInput;
        fInput = NULL;
        
        if (fWordBreakItr != NULL) {
#if UCONFIG_NO_BREAK_ITERATION==0
            UErrorCode status = U_ZERO_ERROR;
            fWordBreakItr->setText(input, status);
#endif
        }
    }
    reset();
    fInputUniStrMaybeMutable = FALSE;

    return *this;
}

/*RegexMatcher &RegexMatcher::reset(const UChar *) {
    fDeferredStatus = U_INTERNAL_PROGRAM_ERROR;
    return *this;
}*/

RegexMatcher &RegexMatcher::reset(int64_t position, UErrorCode &status) {
    if (U_FAILURE(status)) {
        return *this;
    }
    reset();       // Reset also resets the region to be the entire string.
    
    if (position < 0 || position > fActiveLimit) {
        status = U_INDEX_OUTOFBOUNDS_ERROR;
        return *this;
    }
    fMatchEnd = position;
    return *this;
}





//--------------------------------------------------------------------------------
//
//    setTrace
//
//--------------------------------------------------------------------------------
void RegexMatcher::setTrace(UBool state) {
    fTraceDebug = state;
}



//---------------------------------------------------------------------
//
//   split
//
//---------------------------------------------------------------------
int32_t  RegexMatcher::split(const UnicodeString &input,
        UnicodeString    dest[],
        int32_t          destCapacity,
        UErrorCode      &status)
{
    UText inputText = UTEXT_INITIALIZER;
    utext_openConstUnicodeString(&inputText, &input, &status);
    if (U_FAILURE(status)) {
        return 0;
    }

    UText **destText = (UText **)uprv_malloc(sizeof(UText*)*destCapacity);
    if (destText == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return 0;
    }
    int32_t i;
    for (i = 0; i < destCapacity; i++) {
        destText[i] = utext_openUnicodeString(NULL, &dest[i], &status);
    }
    
    int32_t fieldCount = split(&inputText, destText, destCapacity, status);
    
    for (i = 0; i < destCapacity; i++) {
        utext_close(destText[i]);
    }

    uprv_free(destText);
    utext_close(&inputText);
    return fieldCount;
}

//
//   split, UText mode
//
int32_t  RegexMatcher::split(UText *input,
        UText           *dest[],
        int32_t          destCapacity,
        UErrorCode      &status)
{
    //
    // Check arguements for validity
    //
    if (U_FAILURE(status)) {
        return 0;
    };

    if (destCapacity < 1) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    //
    // Reset for the input text
    //
    reset(input);
    int64_t   nextOutputStringStart = 0;
    if (fActiveLimit == 0) {
        return 0;
    }

    //
    // Loop through the input text, searching for the delimiter pattern
    //
    int32_t i;
    int32_t numCaptureGroups = fPattern->fGroupMap->size();
    for (i=0; ; i++) {
        if (i>=destCapacity-1) {
            // There is one or zero output string left.
            // Fill the last output string with whatever is left from the input, then exit the loop.
            //  ( i will be == destCapacity if we filled the output array while processing
            //    capture groups of the delimiter expression, in which case we will discard the
            //    last capture group saved in favor of the unprocessed remainder of the
            //    input string.)
            i = destCapacity-1;
            if (fActiveLimit > nextOutputStringStart) {
                if (UTEXT_FULL_TEXT_IN_CHUNK(input, fInputLength)) {
                    if (dest[i]) {
                        utext_replace(dest[i], 0, utext_nativeLength(dest[i]), 
                                      input->chunkContents+nextOutputStringStart, 
                                      (int32_t)(fActiveLimit-nextOutputStringStart), &status);
                    } else {
                        UText remainingText = UTEXT_INITIALIZER;
                        utext_openUChars(&remainingText, input->chunkContents+nextOutputStringStart, 
                                         fActiveLimit-nextOutputStringStart, &status);
                        dest[i] = utext_clone(NULL, &remainingText, TRUE, FALSE, &status);
                        utext_close(&remainingText);
                    }
                } else {
                    UErrorCode lengthStatus = U_ZERO_ERROR;
                    int32_t remaining16Length = 
                        utext_extract(input, nextOutputStringStart, fActiveLimit, NULL, 0, &lengthStatus);
                    UChar *remainingChars = (UChar *)uprv_malloc(sizeof(UChar)*(remaining16Length+1));
                    if (remainingChars == NULL) {
                        status = U_MEMORY_ALLOCATION_ERROR;
                        break;
                    }

                    utext_extract(input, nextOutputStringStart, fActiveLimit, remainingChars, remaining16Length+1, &status);
                    if (dest[i]) {
                        utext_replace(dest[i], 0, utext_nativeLength(dest[i]), remainingChars, remaining16Length, &status);
                    } else {
                        UText remainingText = UTEXT_INITIALIZER;
                        utext_openUChars(&remainingText, remainingChars, remaining16Length, &status);
                        dest[i] = utext_clone(NULL, &remainingText, TRUE, FALSE, &status);
                        utext_close(&remainingText);
                    }
                    
                    uprv_free(remainingChars);
                }
            }
            break;
        }
        if (find()) {
            // We found another delimiter.  Move everything from where we started looking
            //  up until the start of the delimiter into the next output string.
            if (UTEXT_FULL_TEXT_IN_CHUNK(input, fInputLength)) {
                if (dest[i]) {
                    utext_replace(dest[i], 0, utext_nativeLength(dest[i]), 
                                  input->chunkContents+nextOutputStringStart, 
                                  (int32_t)(fMatchStart-nextOutputStringStart), &status);
                } else {
                    UText remainingText = UTEXT_INITIALIZER;
                    utext_openUChars(&remainingText, input->chunkContents+nextOutputStringStart, 
                                      fMatchStart-nextOutputStringStart, &status);
                    dest[i] = utext_clone(NULL, &remainingText, TRUE, FALSE, &status);
                    utext_close(&remainingText);
                }
            } else {
                UErrorCode lengthStatus = U_ZERO_ERROR;
                int32_t remaining16Length = utext_extract(input, nextOutputStringStart, fMatchStart, NULL, 0, &lengthStatus);
                UChar *remainingChars = (UChar *)uprv_malloc(sizeof(UChar)*(remaining16Length+1));
                if (remainingChars == NULL) {
                    status = U_MEMORY_ALLOCATION_ERROR;
                    break;
                }
                utext_extract(input, nextOutputStringStart, fMatchStart, remainingChars, remaining16Length+1, &status);
                if (dest[i]) {
                    utext_replace(dest[i], 0, utext_nativeLength(dest[i]), remainingChars, remaining16Length, &status);
                } else {
                    UText remainingText = UTEXT_INITIALIZER;
                    utext_openUChars(&remainingText, remainingChars, remaining16Length, &status);
                    dest[i] = utext_clone(NULL, &remainingText, TRUE, FALSE, &status);
                    utext_close(&remainingText);
                }
                
                uprv_free(remainingChars);
            }
            nextOutputStringStart = fMatchEnd;

            // If the delimiter pattern has capturing parentheses, the captured
            //  text goes out into the next n destination strings.
            int32_t groupNum;
            UBool lastGroupWasNullUText = FALSE;
            for (groupNum=1; groupNum<=numCaptureGroups; groupNum++) {
                if (i==destCapacity-1) {
                    break;
                }
                i++;
                lastGroupWasNullUText = (dest[i] == NULL ? TRUE : FALSE);
                dest[i] = group(groupNum, dest[i], status);
            }

            if (nextOutputStringStart == fActiveLimit) {
                // The delimiter was at the end of the string.  We're done.
                break;
            } else if (i == destCapacity-1) {
                // We're out of capture groups, and the rest of the string is more important
                if (lastGroupWasNullUText) {
                    utext_close(dest[i]);
                    dest[i] = NULL;
                }
            }

        }
        else
        {
            // We ran off the end of the input while looking for the next delimiter.
            // All the remaining text goes into the current output string.
            if (UTEXT_FULL_TEXT_IN_CHUNK(input, fInputLength)) {
                if (dest[i]) {
                    utext_replace(dest[i], 0, utext_nativeLength(dest[i]), 
                                  input->chunkContents+nextOutputStringStart, 
                                  (int32_t)(fActiveLimit-nextOutputStringStart), &status);
                } else {
                    UText remainingText = UTEXT_INITIALIZER;
                    utext_openUChars(&remainingText, input->chunkContents+nextOutputStringStart, 
                                     fActiveLimit-nextOutputStringStart, &status);
                    dest[i] = utext_clone(NULL, &remainingText, TRUE, FALSE, &status);
                    utext_close(&remainingText);
                }
            } else {
                UErrorCode lengthStatus = U_ZERO_ERROR;
                int32_t remaining16Length = utext_extract(input, nextOutputStringStart, fActiveLimit, NULL, 0, &lengthStatus);
                UChar *remainingChars = (UChar *)uprv_malloc(sizeof(UChar)*(remaining16Length+1));
                if (remainingChars == NULL) {
                    status = U_MEMORY_ALLOCATION_ERROR;
                    break;
                }
                
                utext_extract(input, nextOutputStringStart, fActiveLimit, remainingChars, remaining16Length+1, &status);
                if (dest[i]) {
                    utext_replace(dest[i], 0, utext_nativeLength(dest[i]), remainingChars, remaining16Length, &status);
                } else {
                    UText remainingText = UTEXT_INITIALIZER;
                    utext_openUChars(&remainingText, remainingChars, remaining16Length, &status);
                    dest[i] = utext_clone(NULL, &remainingText, TRUE, FALSE, &status);
                    utext_close(&remainingText);
                }
                
                uprv_free(remainingChars);
            }
            break;
        }
        if (U_FAILURE(status)) {
            break;
        }
    }   // end of for loop
    return i+1;
}


//--------------------------------------------------------------------------------
//
//     start
//
//--------------------------------------------------------------------------------
int32_t RegexMatcher::start(UErrorCode &status) const {
    return start(0, status);
}

int64_t RegexMatcher::start64(UErrorCode &status) const {
    return start64(0, status);
}

//--------------------------------------------------------------------------------
//
//     start(int32_t group, UErrorCode &status)
//
//--------------------------------------------------------------------------------

int64_t RegexMatcher::start64(int32_t group, UErrorCode &status) const {
    if (U_FAILURE(status)) {
        return -1;
    }
    if (U_FAILURE(fDeferredStatus)) {
        status = fDeferredStatus;
        return -1;
    }
    if (fMatch == FALSE) {
        status = U_REGEX_INVALID_STATE;
        return -1;
    }
    if (group < 0 || group > fPattern->fGroupMap->size()) {
        status = U_INDEX_OUTOFBOUNDS_ERROR;
        return -1;
    }
    int64_t s;
    if (group == 0) {
        s = fMatchStart; 
    } else {
        int32_t groupOffset = fPattern->fGroupMap->elementAti(group-1);
        U_ASSERT(groupOffset < fPattern->fFrameSize);
        U_ASSERT(groupOffset >= 0);
        s = fFrame->fExtra[groupOffset];
    }
    
    return s;
}


int32_t RegexMatcher::start(int32_t group, UErrorCode &status) const {
    return (int32_t)start64(group, status);
}

//--------------------------------------------------------------------------------
//
//     useAnchoringBounds
//
//--------------------------------------------------------------------------------
RegexMatcher &RegexMatcher::useAnchoringBounds(UBool b) {
    fAnchoringBounds = b;
    fAnchorStart = (fAnchoringBounds ? fRegionStart : 0);
    fAnchorLimit = (fAnchoringBounds ? fRegionLimit : fInputLength);
    return *this;
}


//--------------------------------------------------------------------------------
//
//     useTransparentBounds
//
//--------------------------------------------------------------------------------
RegexMatcher &RegexMatcher::useTransparentBounds(UBool b) {
    fTransparentBounds = b;
    fLookStart = (fTransparentBounds ? 0 : fRegionStart);
    fLookLimit = (fTransparentBounds ? fInputLength : fRegionLimit);
    return *this;
}

//--------------------------------------------------------------------------------
//
//     setTimeLimit
//
//--------------------------------------------------------------------------------
void RegexMatcher::setTimeLimit(int32_t limit, UErrorCode &status) {
    if (U_FAILURE(status)) {
        return;
    }
    if (U_FAILURE(fDeferredStatus)) {
        status = fDeferredStatus;
        return;
    }
    if (limit < 0) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }
    fTimeLimit = limit;
}


//--------------------------------------------------------------------------------
//
//     getTimeLimit
//
//--------------------------------------------------------------------------------
int32_t RegexMatcher::getTimeLimit() const {
    return fTimeLimit;
}


//--------------------------------------------------------------------------------
//
//     setStackLimit
//
//--------------------------------------------------------------------------------
void RegexMatcher::setStackLimit(int32_t limit, UErrorCode &status) {
    if (U_FAILURE(status)) {
        return;
    }
    if (U_FAILURE(fDeferredStatus)) {
        status = fDeferredStatus;
        return;
    }
    if (limit < 0) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }
    
    // Reset the matcher.  This is needed here in case there is a current match
    //    whose final stack frame (containing the match results, pointed to by fFrame) 
    //    would be lost by resizing to a smaller stack size.
    reset();
    
    if (limit == 0) {
        // Unlimited stack expansion
        fStack->setMaxCapacity(0);
    } else {
        // Change the units of the limit  from bytes to ints, and bump the size up
        //   to be big enough to hold at least one stack frame for the pattern, 
        //   if it isn't there already.
        int32_t adjustedLimit = limit / sizeof(int32_t);
        if (adjustedLimit < fPattern->fFrameSize) {
            adjustedLimit = fPattern->fFrameSize;
        }
        fStack->setMaxCapacity(adjustedLimit);
    }
    fStackLimit = limit;
}


//--------------------------------------------------------------------------------
//
//     getStackLimit
//
//--------------------------------------------------------------------------------
int32_t RegexMatcher::getStackLimit() const {
    return fStackLimit;
}


//--------------------------------------------------------------------------------
//
//     setMatchCallback
//
//--------------------------------------------------------------------------------
void RegexMatcher::setMatchCallback(URegexMatchCallback     *callback,
                                    const void              *context,
                                    UErrorCode              &status) {
    if (U_FAILURE(status)) {
        return;
    }
    fCallbackFn = callback;
    fCallbackContext = context;
}


//--------------------------------------------------------------------------------
//
//     getMatchCallback
//
//--------------------------------------------------------------------------------
void RegexMatcher::getMatchCallback(URegexMatchCallback   *&callback,
                                  const void              *&context,
                                  UErrorCode              &status) {
    if (U_FAILURE(status)) {
       return;
    }
    callback = fCallbackFn;
    context  = fCallbackContext;
}


//--------------------------------------------------------------------------------
//
//     setMatchCallback
//
//--------------------------------------------------------------------------------
void RegexMatcher::setFindProgressCallback(URegexFindProgressCallback      *callback,
                                                const void                      *context,
                                                UErrorCode                      &status) {
    if (U_FAILURE(status)) {
        return;
    }
    fFindProgressCallbackFn = callback;
    fFindProgressCallbackContext = context;
}


//--------------------------------------------------------------------------------
//
//     getMatchCallback
//
//--------------------------------------------------------------------------------
void RegexMatcher::getFindProgressCallback(URegexFindProgressCallback    *&callback,
                                                const void                    *&context,
                                                UErrorCode                    &status) {
    if (U_FAILURE(status)) {
       return;
    }
    callback = fFindProgressCallbackFn;
    context  = fFindProgressCallbackContext;
}


//================================================================================
//
//    Code following this point in this file is the internal
//    Match Engine Implementation.
//
//================================================================================


//--------------------------------------------------------------------------------
//
//   resetStack
//           Discard any previous contents of the state save stack, and initialize a
//           new stack frame to all -1.  The -1s are needed for capture group limits, 
//           where they indicate that a group has not yet matched anything.
//--------------------------------------------------------------------------------
REStackFrame *RegexMatcher::resetStack() {
    // Discard any previous contents of the state save stack, and initialize a
    //  new stack frame with all -1 data.  The -1s are needed for capture group limits,
    //  where they indicate that a group has not yet matched anything.
    fStack->removeAllElements();

    REStackFrame *iFrame = (REStackFrame *)fStack->reserveBlock(fPattern->fFrameSize, fDeferredStatus);
    int32_t i;
    for (i=0; i<fPattern->fFrameSize-RESTACKFRAME_HDRCOUNT; i++) {
        iFrame->fExtra[i] = -1;
    }
    return iFrame;
}



//--------------------------------------------------------------------------------
//
//   isWordBoundary 
//                     in perl, "xab..cd..", \b is true at positions 0,3,5,7
//                     For us,
//                       If the current char is a combining mark,
//                          \b is FALSE.
//                       Else Scan backwards to the first non-combining char.
//                            We are at a boundary if the this char and the original chars are
//                               opposite in membership in \w set
//
//          parameters:   pos   - the current position in the input buffer
//
//              TODO:  double-check edge cases at region boundaries.
//
//--------------------------------------------------------------------------------
UBool RegexMatcher::isWordBoundary(int64_t pos) {
    UBool isBoundary = FALSE;
    UBool cIsWord    = FALSE;
    
    if (pos >= fLookLimit) {
        fHitEnd = TRUE;
    } else {
        // Determine whether char c at current position is a member of the word set of chars.
        // If we're off the end of the string, behave as though we're not at a word char.
        UTEXT_SETNATIVEINDEX(fInputText, pos);
        UChar32  c = UTEXT_CURRENT32(fInputText);
        if (u_hasBinaryProperty(c, UCHAR_GRAPHEME_EXTEND) || u_charType(c) == U_FORMAT_CHAR) {
            // Current char is a combining one.  Not a boundary.
            return FALSE;
        }
        cIsWord = fPattern->fStaticSets[URX_ISWORD_SET]->contains(c);
    }
    
    // Back up until we come to a non-combining char, determine whether
    //  that char is a word char.
    UBool prevCIsWord = FALSE;
    for (;;) {
        if (UTEXT_GETNATIVEINDEX(fInputText) <= fLookStart) {
            break;
        }
        UChar32 prevChar = UTEXT_PREVIOUS32(fInputText);
        if (!(u_hasBinaryProperty(prevChar, UCHAR_GRAPHEME_EXTEND)
              || u_charType(prevChar) == U_FORMAT_CHAR)) {
            prevCIsWord = fPattern->fStaticSets[URX_ISWORD_SET]->contains(prevChar);
            break;
        }
    }
    isBoundary = cIsWord ^ prevCIsWord;
    return isBoundary;
}

UBool RegexMatcher::isChunkWordBoundary(int32_t pos) {
    UBool isBoundary = FALSE;
    UBool cIsWord    = FALSE;
    
    const UChar *inputBuf = fInputText->chunkContents;
    
    if (pos >= fLookLimit) {
        fHitEnd = TRUE;
    } else {
        // Determine whether char c at current position is a member of the word set of chars.
        // If we're off the end of the string, behave as though we're not at a word char.
        UChar32 c;
        U16_GET(inputBuf, fLookStart, pos, fLookLimit, c);
        if (u_hasBinaryProperty(c, UCHAR_GRAPHEME_EXTEND) || u_charType(c) == U_FORMAT_CHAR) {
            // Current char is a combining one.  Not a boundary.
            return FALSE;
        }
        cIsWord = fPattern->fStaticSets[URX_ISWORD_SET]->contains(c);
    }
    
    // Back up until we come to a non-combining char, determine whether
    //  that char is a word char.
    UBool prevCIsWord = FALSE;
    for (;;) {
        if (pos <= fLookStart) {
            break;
        }
        UChar32 prevChar;
        U16_PREV(inputBuf, fLookStart, pos, prevChar);
        if (!(u_hasBinaryProperty(prevChar, UCHAR_GRAPHEME_EXTEND)
              || u_charType(prevChar) == U_FORMAT_CHAR)) {
            prevCIsWord = fPattern->fStaticSets[URX_ISWORD_SET]->contains(prevChar);
            break;
        }
    }
    isBoundary = cIsWord ^ prevCIsWord;
    return isBoundary;
}

//--------------------------------------------------------------------------------
//
//   isUWordBoundary 
//
//         Test for a word boundary using RBBI word break.
//
//          parameters:   pos   - the current position in the input buffer
//
//--------------------------------------------------------------------------------
UBool RegexMatcher::isUWordBoundary(int64_t pos) {
    UBool       returnVal = FALSE;
#if UCONFIG_NO_BREAK_ITERATION==0
    
    // If we haven't yet created a break iterator for this matcher, do it now.
    if (fWordBreakItr == NULL) {
        fWordBreakItr = 
            (RuleBasedBreakIterator *)BreakIterator::createWordInstance(Locale::getEnglish(), fDeferredStatus);
        if (U_FAILURE(fDeferredStatus)) {
            return FALSE;
        }
        fWordBreakItr->setText(fInputText, fDeferredStatus);
    }

    if (pos >= fLookLimit) {
        fHitEnd = TRUE;
        returnVal = TRUE;   // With Unicode word rules, only positions within the interior of "real"
                            //    words are not boundaries.  All non-word chars stand by themselves,
                            //    with word boundaries on both sides.
    } else {
        if (!UTEXT_USES_U16(fInputText)) {
            // !!!: Would like a better way to do this!
            UErrorCode status = U_ZERO_ERROR;
            pos = utext_extract(fInputText, 0, pos, NULL, 0, &status);
        }
        returnVal = fWordBreakItr->isBoundary((int32_t)pos);
    }
#endif
    return   returnVal;
}

//--------------------------------------------------------------------------------
//
//   IncrementTime     This function is called once each TIMER_INITIAL_VALUE state
//                     saves. Increment the "time" counter, and call the
//                     user callback function if there is one installed.
//
//                     If the match operation needs to be aborted, either for a time-out
//                     or because the user callback asked for it, just set an error status.
//                     The engine will pick that up and stop in its outer loop.
//
//--------------------------------------------------------------------------------
void RegexMatcher::IncrementTime(UErrorCode &status) {
    fTickCounter = TIMER_INITIAL_VALUE;
    fTime++;
    if (fCallbackFn != NULL) {
        if ((*fCallbackFn)(fCallbackContext, fTime) == FALSE) {
            status = U_REGEX_STOPPED_BY_CALLER;
            return;
        }
    }
    if (fTimeLimit > 0 && fTime >= fTimeLimit) {
        status = U_REGEX_TIME_OUT;
    }
}

//--------------------------------------------------------------------------------
//
//   ReportFindProgress     This function is called once for each advance in the target
//                          string from the find() function, and calls the user progress callback
//                          function if there is one installed.
//                          
//                          NOTE:  
//
//                          If the match operation needs to be aborted because the user
//                          callback asked for it, just set an error status.
//                          The engine will pick that up and stop in its outer loop.
//
//--------------------------------------------------------------------------------
UBool RegexMatcher::ReportFindProgress(int64_t matchIndex, UErrorCode &status) {
    if (fFindProgressCallbackFn != NULL) {
        if ((*fFindProgressCallbackFn)(fFindProgressCallbackContext, matchIndex) == FALSE) {
            status = U_ZERO_ERROR /*U_REGEX_STOPPED_BY_CALLER*/;
            return FALSE;
        }
    }
    return TRUE;
}

//--------------------------------------------------------------------------------
//
//   StateSave
//       Make a new stack frame, initialized as a copy of the current stack frame.
//       Set the pattern index in the original stack frame from the operand value
//       in the opcode.  Execution of the engine continues with the state in
//       the newly created stack frame
//
//       Note that reserveBlock() may grow the stack, resulting in the
//       whole thing being relocated in memory.
//
//    Parameters:
//       fp           The top frame pointer when called.  At return, a new 
//                    fame will be present
//       savePatIdx   An index into the compiled pattern.  Goes into the original
//                    (not new) frame.  If execution ever back-tracks out of the
//                    new frame, this will be where we continue from in the pattern.
//    Return
//                    The new frame pointer.
//
//--------------------------------------------------------------------------------
inline REStackFrame *RegexMatcher::StateSave(REStackFrame *fp, int64_t savePatIdx, UErrorCode &status) {
    // push storage for a new frame. 
    int64_t *newFP = fStack->reserveBlock(fFrameSize, status);
    if (newFP == NULL) {
        // Failure on attempted stack expansion.
        //   Stack function set some other error code, change it to a more
        //   specific one for regular expressions.
        status = U_REGEX_STACK_OVERFLOW;
        // We need to return a writable stack frame, so just return the
        //    previous frame.  The match operation will stop quickly
        //    because of the error status, after which the frame will never
        //    be looked at again.
        return fp;
    }
    fp = (REStackFrame *)(newFP - fFrameSize);  // in case of realloc of stack.
    
    // New stack frame = copy of old top frame.
    int64_t *source = (int64_t *)fp;
    int64_t *dest   = newFP;
    for (;;) {
        *dest++ = *source++;
        if (source == newFP) {
            break;
        }
    }
    
    fTickCounter--;
    if (fTickCounter <= 0) {
       IncrementTime(status);    // Re-initializes fTickCounter
    }
    fp->fPatIdx = savePatIdx;
    return (REStackFrame *)newFP;
}


//--------------------------------------------------------------------------------
//
//   MatchAt      This is the actual matching engine.
//
//                  startIdx:    begin matching a this index.
//                  toEnd:       if true, match must extend to end of the input region
//
//--------------------------------------------------------------------------------
void RegexMatcher::MatchAt(int64_t startIdx, UBool toEnd, UErrorCode &status) {
    UBool       isMatch  = FALSE;      // True if the we have a match.
    
    int64_t     backSearchIndex = U_INT64_MAX; // used after greedy single-character matches for searching backwards

    int32_t     op;                    // Operation from the compiled pattern, split into
    int32_t     opType;                //    the opcode
    int32_t     opValue;               //    and the operand value.
        
    #ifdef REGEX_RUN_DEBUG
    if (fTraceDebug)
    {
        printf("MatchAt(startIdx=%ld)\n", startIdx);
        printf("Original Pattern: ");
        UChar32 c = utext_next32From(fPattern->fPattern, 0);
        while (c != U_SENTINEL) {
            if (c<32 || c>256) {
                c = '.';
            }
            REGEX_DUMP_DEBUG_PRINTF(("%c", c));
            
            c = UTEXT_NEXT32(fPattern->fPattern);
        }
        printf("\n");
        printf("Input String: ");
        c = utext_next32From(fInputText, 0);
        while (c != U_SENTINEL) {
            if (c<32 || c>256) {
                c = '.';
            }
            printf("%c", c);
            
            c = UTEXT_NEXT32(fInputText);
        }
        printf("\n");
        printf("\n");
    }
    #endif

    if (U_FAILURE(status)) {
        return;
    }

    //  Cache frequently referenced items from the compiled pattern
    //
    int64_t             *pat           = fPattern->fCompiledPat->getBuffer();

    const UChar         *litText       = fPattern->fLiteralText.getBuffer();
    UVector             *sets          = fPattern->fSets;

    fFrameSize = fPattern->fFrameSize;
    REStackFrame        *fp            = resetStack();

    fp->fPatIdx   = 0;
    fp->fInputIdx = startIdx;

    // Zero out the pattern's static data
    int32_t i;
    for (i = 0; i<fPattern->fDataSize; i++) {
        fData[i] = 0;
    }

    //
    //  Main loop for interpreting the compiled pattern.
    //  One iteration of the loop per pattern operation performed.
    //
    for (;;) {
#if 0
        if (_heapchk() != _HEAPOK) {
            fprintf(stderr, "Heap Trouble\n");
        }
#endif
        
        op      = (int32_t)pat[fp->fPatIdx];
        opType  = URX_TYPE(op);
        opValue = URX_VAL(op);
        #ifdef REGEX_RUN_DEBUG
        if (fTraceDebug) {
            UTEXT_SETNATIVEINDEX(fInputText, fp->fInputIdx);
            printf("inputIdx=%d   inputChar=%x   sp=%3d   activeLimit=%d  ", fp->fInputIdx,
                UTEXT_CURRENT32(fInputText), (int64_t *)fp-fStack->getBuffer(), fActiveLimit);
            fPattern->dumpOp(fp->fPatIdx);
        }
        #endif
        fp->fPatIdx++;
        
        switch (opType) {


        case URX_NOP:
            break;


        case URX_BACKTRACK:
            // Force a backtrack.  In some circumstances, the pattern compiler
            //   will notice that the pattern can't possibly match anything, and will
            //   emit one of these at that point.
            fp = (REStackFrame *)fStack->popFrame(fFrameSize);
            break;


        case URX_ONECHAR:
            if (fp->fInputIdx < fActiveLimit) {
                UTEXT_SETNATIVEINDEX(fInputText, fp->fInputIdx);
                UChar32 c = UTEXT_NEXT32(fInputText);
                if (c == opValue) {
                    fp->fInputIdx = UTEXT_GETNATIVEINDEX(fInputText);
                    break;
                }
            } else {
                fHitEnd = TRUE;
            }
                
            #ifdef REGEX_SMART_BACKTRACKING
            if (fp->fInputIdx > backSearchIndex && fStack->size() > fFrameSize) {
                REStackFrame *prevFrame = (REStackFrame *)fStack->peekFrame(fFrameSize);
                if (URX_LOOP_C == URX_TYPE(pat[prevFrame->fPatIdx]) && fp->fInputIdx <= prevFrame->fInputIdx) {
                    UBool success = FALSE;
                    UChar32 c = UTEXT_PREVIOUS32(fInputText);
                    while (UTEXT_GETNATIVEINDEX(fInputText) >= backSearchIndex) {
                        if (c == opValue) {
                            success = TRUE;
                            break;
                        } else if (c == U_SENTINEL) {
                            break;
                        }
                        c = UTEXT_PREVIOUS32(fInputText);
                    }
                    if (success) {
                        fHitEnd = FALSE;
                        fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                        fp->fInputIdx = UTEXT_GETNATIVEINDEX(fInputText);
                        if (fp->fInputIdx > backSearchIndex) {
                            fp = StateSave(fp, fp->fPatIdx, status);
                        }
                        fp->fPatIdx++; // Skip the LOOP_C, we just did that
                        break;
                    }
                }
            }
            #endif
            
            fp = (REStackFrame *)fStack->popFrame(fFrameSize);
            break;


        case URX_STRING:
            {
                // Test input against a literal string.
                // Strings require two slots in the compiled pattern, one for the
                //   offset to the string text, and one for the length.
                int32_t   stringStartIdx = opValue;
                int32_t   stringLen;

                op      = (int32_t)pat[fp->fPatIdx];     // Fetch the second operand
                fp->fPatIdx++;
                opType    = URX_TYPE(op);
                stringLen = URX_VAL(op);
                U_ASSERT(opType == URX_STRING_LEN);
                U_ASSERT(stringLen >= 2);
                                
                const UChar *patternChars = litText+stringStartIdx;
                const UChar *patternEnd = patternChars+stringLen;
                
                UTEXT_SETNATIVEINDEX(fInputText, fp->fInputIdx);
                UChar32 c;
                UBool success = TRUE;
                
                while (patternChars < patternEnd && success) {
                    c = UTEXT_NEXT32(fInputText);
                    
                    if (c != U_SENTINEL && UTEXT_GETNATIVEINDEX(fInputText) <= fActiveLimit) {
                        if (U_IS_BMP(c)) {
                            success = (*patternChars == c);
                            patternChars += 1;
                        } else if (patternChars+1 < patternEnd) {
                            success = (*patternChars == U16_LEAD(c) && *(patternChars+1) == U16_TRAIL(c));
                            patternChars += 2;
                        }
                    } else {
                        success = FALSE;
                        fHitEnd = TRUE;          //   TODO:  See ticket 6074
                    }
                }
                
                if (success) {
                    fp->fInputIdx = UTEXT_GETNATIVEINDEX(fInputText);
                } else {
                    #ifdef REGEX_SMART_BACKTRACKING
                    if (fp->fInputIdx > backSearchIndex && fStack->size()) {
                        REStackFrame *prevFrame = (REStackFrame *)fStack->peekFrame(fFrameSize);
                        if (URX_LOOP_C == URX_TYPE(pat[prevFrame->fPatIdx]) && fp->fInputIdx <= prevFrame->fInputIdx) {
                            // Reset to last start point
                            UTEXT_SETNATIVEINDEX(fInputText, fp->fInputIdx);
                            patternChars = litText+stringStartIdx;
                            
                            // Search backwards for a possible start
                            do {
                                c = UTEXT_PREVIOUS32(fInputText);
                                if (c == U_SENTINEL) {
                                    break;
                                } else if ((U_IS_BMP(c) && *patternChars == c) ||
                                    (*patternChars == U16_LEAD(c) && *(patternChars+1) == U16_TRAIL(c))) {
                                    success = TRUE;
                                    break;
                                }
                            } while (UTEXT_GETNATIVEINDEX(fInputText) >= backSearchIndex);
                            
                            // And try again
                            if (success) {
                                fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                                fp->fInputIdx = UTEXT_GETNATIVEINDEX(fInputText);
                                if (fp->fInputIdx > backSearchIndex) {
                                    fp = StateSave(fp, fp->fPatIdx, status);
                                }
                                fp->fPatIdx++; // Skip the LOOP_C, we just did that
                                break;
                            }
                        }
                    }
                    #endif
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                }
            }
            break;


        case URX_STATE_SAVE:
            fp = StateSave(fp, opValue, status);
            break;


        case URX_END:
            // The match loop will exit via this path on a successful match,
            //   when we reach the end of the pattern.
            if (toEnd && fp->fInputIdx != fActiveLimit) {
                // The pattern matched, but not to the end of input.  Try some more.
                fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                break;
            }
            isMatch = TRUE;
            goto  breakFromLoop;

        // Start and End Capture stack frame variables are laid out out like this:
            //  fp->fExtra[opValue]  - The start of a completed capture group
            //             opValue+1 - The end   of a completed capture group
            //             opValue+2 - the start of a capture group whose end
            //                          has not yet been reached (and might not ever be).
        case URX_START_CAPTURE:
            U_ASSERT(opValue >= 0 && opValue < fFrameSize-3);
            fp->fExtra[opValue+2] = fp->fInputIdx;
            break;


        case URX_END_CAPTURE:
            U_ASSERT(opValue >= 0 && opValue < fFrameSize-3);
            U_ASSERT(fp->fExtra[opValue+2] >= 0);            // Start pos for this group must be set.
            fp->fExtra[opValue]   = fp->fExtra[opValue+2];   // Tentative start becomes real.
            fp->fExtra[opValue+1] = fp->fInputIdx;           // End position
            U_ASSERT(fp->fExtra[opValue] <= fp->fExtra[opValue+1]);
            break;


        case URX_DOLLAR:                   //  $, test for End of line
                                           //     or for position before new line at end of input
            {
                if (fp->fInputIdx >= fAnchorLimit) {
                    // We really are at the end of input.  Success.
                    fHitEnd = TRUE;
                    fRequireEnd = TRUE;
                    break;
                }
                
                UTEXT_SETNATIVEINDEX(fInputText, fp->fInputIdx);
                
                // If we are positioned just before a new-line that is located at the
                //   end of input, succeed.
                UChar32 c = UTEXT_NEXT32(fInputText);
                if (UTEXT_GETNATIVEINDEX(fInputText) >= fAnchorLimit) {
                    if ((c>=0x0a && c<=0x0d) || c==0x85 || c==0x2028 || c==0x2029) {
                        // If not in the middle of a CR/LF sequence
                        if ( !(c==0x0a && fp->fInputIdx>fAnchorStart && (UTEXT_PREVIOUS32(fInputText), UTEXT_PREVIOUS32(fInputText))==0x0d)) {
                            // At new-line at end of input. Success
                            fHitEnd = TRUE;
                            fRequireEnd = TRUE;
                            
                            break;
                        }
                    }
                } else {
                    UChar32 nextC = UTEXT_NEXT32(fInputText);
                    if (c == 0x0d && nextC == 0x0a && UTEXT_GETNATIVEINDEX(fInputText) >= fAnchorLimit) {
                        fHitEnd = TRUE;
                        fRequireEnd = TRUE;
                        break;                         // At CR/LF at end of input.  Success
                    }
                }

                fp = (REStackFrame *)fStack->popFrame(fFrameSize);
            }
            break;


         case URX_DOLLAR_D:                   //  $, test for End of Line, in UNIX_LINES mode.
            if (fp->fInputIdx >= fAnchorLimit) {
                // Off the end of input.  Success.
                fHitEnd = TRUE;
                fRequireEnd = TRUE;
                break;
            } else {
                UTEXT_SETNATIVEINDEX(fInputText, fp->fInputIdx);
                UChar32 c = UTEXT_NEXT32(fInputText);
                // Either at the last character of input, or off the end.
                if (c == 0x0a && UTEXT_GETNATIVEINDEX(fInputText) == fAnchorLimit) {
                    fHitEnd = TRUE;
                    fRequireEnd = TRUE;
                    break;
                }
            }

            // Not at end of input.  Back-track out.
            fp = (REStackFrame *)fStack->popFrame(fFrameSize);
            break;


         case URX_DOLLAR_M:                //  $, test for End of line in multi-line mode
             {
                 if (fp->fInputIdx >= fAnchorLimit) {
                     // We really are at the end of input.  Success.
                     fHitEnd = TRUE;
                     fRequireEnd = TRUE;
                     break;
                 }
                 // If we are positioned just before a new-line, succeed.
                 // It makes no difference where the new-line is within the input.
                 UTEXT_SETNATIVEINDEX(fInputText, fp->fInputIdx);
                 UChar32 c = UTEXT_CURRENT32(fInputText);
                 if ((c>=0x0a && c<=0x0d) || c==0x85 ||c==0x2028 || c==0x2029) {
                     // At a line end, except for the odd chance of  being in the middle of a CR/LF sequence
                     //  In multi-line mode, hitting a new-line just before the end of input does not
                     //   set the hitEnd or requireEnd flags
                     if ( !(c==0x0a && fp->fInputIdx>fAnchorStart && UTEXT_PREVIOUS32(fInputText)==0x0d)) {
                        break;
                     }
                 }
                 // not at a new line.  Fail.
                 fp = (REStackFrame *)fStack->popFrame(fFrameSize);
             }
             break;


         case URX_DOLLAR_MD:                //  $, test for End of line in multi-line and UNIX_LINES mode
             {
                 if (fp->fInputIdx >= fAnchorLimit) {
                     // We really are at the end of input.  Success.
                     fHitEnd = TRUE;
                     fRequireEnd = TRUE;  // Java set requireEnd in this case, even though
                     break;               //   adding a new-line would not lose the match.
                 }
                 // If we are not positioned just before a new-line, the test fails; backtrack out.
                 // It makes no difference where the new-line is within the input.
                 UTEXT_SETNATIVEINDEX(fInputText, fp->fInputIdx);
                 if (UTEXT_CURRENT32(fInputText) != 0x0a) {
                     fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                 }
             }
             break;


       case URX_CARET:                    //  ^, test for start of line
            if (fp->fInputIdx != fAnchorStart) {
                fp = (REStackFrame *)fStack->popFrame(fFrameSize);
            }
            break;


       case URX_CARET_M:                   //  ^, test for start of line in mulit-line mode
           {
               if (fp->fInputIdx == fAnchorStart) {
                   // We are at the start input.  Success.
                   break;
               }
               // Check whether character just before the current pos is a new-line
               //   unless we are at the end of input
               UTEXT_SETNATIVEINDEX(fInputText, fp->fInputIdx);
               UChar32  c = UTEXT_PREVIOUS32(fInputText); 
               if ((fp->fInputIdx < fAnchorLimit) && 
                   ((c<=0x0d && c>=0x0a) || c==0x85 ||c==0x2028 || c==0x2029)) {
                   //  It's a new-line.  ^ is true.  Success.
                   //  TODO:  what should be done with positions between a CR and LF?
                   break;
               }
               // Not at the start of a line.  Fail.
               fp = (REStackFrame *)fStack->popFrame(fFrameSize);
           }
           break;


       case URX_CARET_M_UNIX:       //  ^, test for start of line in mulit-line + Unix-line mode
           {
               U_ASSERT(fp->fInputIdx >= fAnchorStart);
               if (fp->fInputIdx <= fAnchorStart) {
                   // We are at the start input.  Success.
                   break;
               }
               // Check whether character just before the current pos is a new-line
               U_ASSERT(fp->fInputIdx <= fAnchorLimit);
               UTEXT_SETNATIVEINDEX(fInputText, fp->fInputIdx);
               UChar32  c = UTEXT_PREVIOUS32(fInputText);
               if (c != 0x0a) {
                   // Not at the start of a line.  Back-track out.
                   fp = (REStackFrame *)fStack->popFrame(fFrameSize);
               }
           }
           break;

        case URX_BACKSLASH_B:          // Test for word boundaries
            {
                UBool success = isWordBoundary(fp->fInputIdx);
                success ^= (opValue != 0);     // flip sense for \B
                if (!success) {
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                }
            }
            break;


        case URX_BACKSLASH_BU:          // Test for word boundaries, Unicode-style
            {
                UBool success = isUWordBoundary(fp->fInputIdx);
                success ^= (opValue != 0);     // flip sense for \B
                if (!success) {
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                }
            }
            break;


        case URX_BACKSLASH_D:            // Test for decimal digit
            {
                if (fp->fInputIdx >= fActiveLimit) {
                    fHitEnd = TRUE;
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                    break;
                }

                UTEXT_SETNATIVEINDEX(fInputText, fp->fInputIdx);

                UChar32 c = UTEXT_NEXT32(fInputText);
                int8_t ctype = u_charType(c);     // TODO:  make a unicode set for this.  Will be faster.
                UBool success = (ctype == U_DECIMAL_DIGIT_NUMBER);
                success ^= (opValue != 0);        // flip sense for \D
                if (success) {
                    fp->fInputIdx = UTEXT_GETNATIVEINDEX(fInputText);
                } else {
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                }
            }
            break;


        case URX_BACKSLASH_G:          // Test for position at end of previous match
            if (!((fMatch && fp->fInputIdx==fMatchEnd) || (fMatch==FALSE && fp->fInputIdx==fActiveStart))) {
                fp = (REStackFrame *)fStack->popFrame(fFrameSize);
            }
            break;


        case URX_BACKSLASH_X:     
            //  Match a Grapheme, as defined by Unicode TR 29.
            //  Differs slightly from Perl, which consumes combining marks independently
            //    of context.
            {

                // Fail if at end of input
                if (fp->fInputIdx >= fActiveLimit) {
                    fHitEnd = TRUE;
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                    break;
                }
                
                UTEXT_SETNATIVEINDEX(fInputText, fp->fInputIdx);

                // Examine (and consume) the current char.
                //   Dispatch into a little state machine, based on the char.
                UChar32  c;
                c = UTEXT_NEXT32(fInputText);
                fp->fInputIdx = UTEXT_GETNATIVEINDEX(fInputText);
                UnicodeSet **sets = fPattern->fStaticSets;
                if (sets[URX_GC_NORMAL]->contains(c))  goto GC_Extend;
                if (sets[URX_GC_CONTROL]->contains(c)) goto GC_Control;
                if (sets[URX_GC_L]->contains(c))       goto GC_L;
                if (sets[URX_GC_LV]->contains(c))      goto GC_V;
                if (sets[URX_GC_LVT]->contains(c))     goto GC_T;
                if (sets[URX_GC_V]->contains(c))       goto GC_V;
                if (sets[URX_GC_T]->contains(c))       goto GC_T;
                goto GC_Extend;



GC_L:
                if (fp->fInputIdx >= fActiveLimit)         goto GC_Done;
                c = UTEXT_NEXT32(fInputText);
                fp->fInputIdx = UTEXT_GETNATIVEINDEX(fInputText);
                if (sets[URX_GC_L]->contains(c))       goto GC_L;
                if (sets[URX_GC_LV]->contains(c))      goto GC_V;
                if (sets[URX_GC_LVT]->contains(c))     goto GC_T;
                if (sets[URX_GC_V]->contains(c))       goto GC_V;
                UTEXT_PREVIOUS32(fInputText);
                fp->fInputIdx = UTEXT_GETNATIVEINDEX(fInputText);
                goto GC_Extend;

GC_V:
                if (fp->fInputIdx >= fActiveLimit)         goto GC_Done;
                c = UTEXT_NEXT32(fInputText);
                fp->fInputIdx = UTEXT_GETNATIVEINDEX(fInputText);
                if (sets[URX_GC_V]->contains(c))       goto GC_V;
                if (sets[URX_GC_T]->contains(c))       goto GC_T;
                UTEXT_PREVIOUS32(fInputText);
                fp->fInputIdx = UTEXT_GETNATIVEINDEX(fInputText);
                goto GC_Extend;

GC_T:
                if (fp->fInputIdx >= fActiveLimit)         goto GC_Done;
                c = UTEXT_NEXT32(fInputText);
                fp->fInputIdx = UTEXT_GETNATIVEINDEX(fInputText);
                if (sets[URX_GC_T]->contains(c))       goto GC_T;
                UTEXT_PREVIOUS32(fInputText);
                fp->fInputIdx = UTEXT_GETNATIVEINDEX(fInputText);
                goto GC_Extend;

GC_Extend:
                // Combining characters are consumed here
                for (;;) {
                    if (fp->fInputIdx >= fActiveLimit) {
                        break;
                    }
                    c = UTEXT_CURRENT32(fInputText);
                    if (sets[URX_GC_EXTEND]->contains(c) == FALSE) {
                        break;
                    }
                    UTEXT_NEXT32(fInputText);
                    fp->fInputIdx = UTEXT_GETNATIVEINDEX(fInputText);
                }
                goto GC_Done;

GC_Control:
                // Most control chars stand alone (don't combine with combining chars),  
                //   except for that CR/LF sequence is a single grapheme cluster.
                if (c == 0x0d && fp->fInputIdx < fActiveLimit && UTEXT_CURRENT32(fInputText) == 0x0a) {
                    c = UTEXT_NEXT32(fInputText);
                    fp->fInputIdx = UTEXT_GETNATIVEINDEX(fInputText);
                }

GC_Done:
                if (fp->fInputIdx >= fActiveLimit) {
                    fHitEnd = TRUE;
                }
                break;
            }
            



        case URX_BACKSLASH_Z:          // Test for end of Input
            if (fp->fInputIdx < fAnchorLimit) {
                fp = (REStackFrame *)fStack->popFrame(fFrameSize);
            } else {
                fHitEnd = TRUE;
                fRequireEnd = TRUE;
            }
            break;



        case URX_STATIC_SETREF:
            {
                // Test input character against one of the predefined sets
                //    (Word Characters, for example)
                // The high bit of the op value is a flag for the match polarity.
                //    0:   success if input char is in set.
                //    1:   success if input char is not in set.
                if (fp->fInputIdx >= fActiveLimit) {
                    fHitEnd = TRUE;
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                    break;
                }

                UBool success = ((opValue & URX_NEG_SET) == URX_NEG_SET);  
                opValue &= ~URX_NEG_SET;
                U_ASSERT(opValue > 0 && opValue < URX_LAST_SET);

                UTEXT_SETNATIVEINDEX(fInputText, fp->fInputIdx);
                UChar32 c = UTEXT_NEXT32(fInputText);
                if (c < 256) {
                    Regex8BitSet *s8 = &fPattern->fStaticSets8[opValue];
                    if (s8->contains(c)) {
                        success = !success;
                    }
                } else {
                    const UnicodeSet *s = fPattern->fStaticSets[opValue];
                    if (s->contains(c)) {
                        success = !success;
                    }
                }
                if (success) {
                    fp->fInputIdx = UTEXT_GETNATIVEINDEX(fInputText);
                } else {
                    // the character wasn't in the set.
                    #ifdef REGEX_SMART_BACKTRACKING
                    if (fp->fInputIdx > backSearchIndex && fStack->size() > fFrameSize) {
                        REStackFrame *prevFrame = (REStackFrame *)fStack->peekFrame(fFrameSize);
                        if (URX_LOOP_C == URX_TYPE(pat[prevFrame->fPatIdx]) && fp->fInputIdx <= prevFrame->fInputIdx) {
                            // Try to find it, backwards
                            UTEXT_PREVIOUS32(fInputText); // skip the first character we tried
                            success = ((opValue & URX_NEG_SET) == URX_NEG_SET); // reset
                            do {
                                c = UTEXT_PREVIOUS32(fInputText);
                                if (c == U_SENTINEL) {
                                    break;
                                } else if (c < 256) {
                                    Regex8BitSet *s8 = &fPattern->fStaticSets8[opValue];
                                    if (s8->contains(c)) {
                                        success = !success;
                                    }
                                } else {
                                    const UnicodeSet *s = fPattern->fStaticSets[opValue];
                                    if (s->contains(c)) {
                                        success = !success;
                                    }
                                }
                            } while (UTEXT_GETNATIVEINDEX(fInputText) >= backSearchIndex && !success);
                            
                            if (success && c != U_SENTINEL) {
                                fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                                fp->fInputIdx = UTEXT_GETNATIVEINDEX(fInputText);
                                if (fp->fInputIdx > backSearchIndex) {
                                    fp = StateSave(fp, fp->fPatIdx, status);
                                }
                                fp->fPatIdx++; // Skip the LOOP_C, we just did that
                                break;
                            }
                        }
                    }
                    #endif
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                }
            }
            break;
            

        case URX_STAT_SETREF_N:
            {
                // Test input character for NOT being a member of  one of 
                //    the predefined sets (Word Characters, for example)
                if (fp->fInputIdx >= fActiveLimit) {
                    fHitEnd = TRUE;
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                    break;
                }

                U_ASSERT(opValue > 0 && opValue < URX_LAST_SET);

                UTEXT_SETNATIVEINDEX(fInputText, fp->fInputIdx);
                
                UChar32 c = UTEXT_NEXT32(fInputText);
                if (c < 256) {
                    Regex8BitSet *s8 = &fPattern->fStaticSets8[opValue];
                    if (s8->contains(c) == FALSE) {
                        fp->fInputIdx = UTEXT_GETNATIVEINDEX(fInputText);
                        break;
                    }
                } else {
                    const UnicodeSet *s = fPattern->fStaticSets[opValue];
                    if (s->contains(c) == FALSE) {
                        fp->fInputIdx = UTEXT_GETNATIVEINDEX(fInputText);
                        break;
                    }
                }
                // the character wasn't in the set.
                #ifdef REGEX_SMART_BACKTRACKING
                if (fp->fInputIdx > backSearchIndex && fStack->size() > fFrameSize) {
                    REStackFrame *prevFrame = (REStackFrame *)fStack->peekFrame(fFrameSize);
                    if (URX_LOOP_C == URX_TYPE(pat[prevFrame->fPatIdx]) && fp->fInputIdx <= prevFrame->fInputIdx) {
                        // Try to find it, backwards
                        UTEXT_PREVIOUS32(fInputText); // skip the first character we tried
                        UBool success = FALSE;
                        do {
                            c = UTEXT_PREVIOUS32(fInputText);
                            if (c == U_SENTINEL) {
                                break;
                            } else if (c < 256) {
                                Regex8BitSet *s8 = &fPattern->fStaticSets8[opValue];
                                if (s8->contains(c) == FALSE) {
                                    success = TRUE;
                                    break;
                                }
                            } else {
                                const UnicodeSet *s = fPattern->fStaticSets[opValue];
                                if (s->contains(c) == FALSE) {
                                    success = TRUE;
                                    break;
                                }
                            }
                        } while (UTEXT_GETNATIVEINDEX(fInputText) >= backSearchIndex);
                        
                        if (success) {
                            fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                            fp->fInputIdx = UTEXT_GETNATIVEINDEX(fInputText);
                            if (fp->fInputIdx > backSearchIndex) {
                                fp = StateSave(fp, fp->fPatIdx, status);
                            }
                            fp->fPatIdx++; // Skip the LOOP_C, we just did that
                            break;
                        }
                    }
                }
                #endif
                fp = (REStackFrame *)fStack->popFrame(fFrameSize);
            }
            break;
            

        case URX_SETREF:
            if (fp->fInputIdx >= fActiveLimit) {
                fHitEnd = TRUE;
                fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                break;
            } else {
                UTEXT_SETNATIVEINDEX(fInputText, fp->fInputIdx);
                    
                // There is input left.  Pick up one char and test it for set membership.
                UChar32 c = UTEXT_NEXT32(fInputText);
                U_ASSERT(opValue > 0 && opValue < sets->size());
                if (c<256) {
                    Regex8BitSet *s8 = &fPattern->fSets8[opValue];
                    if (s8->contains(c)) {
                        fp->fInputIdx = UTEXT_GETNATIVEINDEX(fInputText);
                        break;
                    }
                } else {
                    UnicodeSet *s = (UnicodeSet *)sets->elementAt(opValue);
                    if (s->contains(c)) {
                        // The character is in the set.  A Match.
                        fp->fInputIdx = UTEXT_GETNATIVEINDEX(fInputText);
                        break;
                    }
                }
                
                // the character wasn't in the set.
                #ifdef REGEX_SMART_BACKTRACKING
                if (fp->fInputIdx > backSearchIndex && fStack->size() > fFrameSize) {
                    REStackFrame *prevFrame = (REStackFrame *)fStack->peekFrame(fFrameSize);
                    if (URX_LOOP_C == URX_TYPE(pat[prevFrame->fPatIdx]) && fp->fInputIdx <= prevFrame->fInputIdx) {
                        // Try to find it, backwards
                        UTEXT_PREVIOUS32(fInputText); // skip the first character we tried
                        UBool success = FALSE;
                        do {
                            c = UTEXT_PREVIOUS32(fInputText);
                            if (c == U_SENTINEL) {
                                break;
                            } else if (c < 256) {
                                Regex8BitSet *s8 = &fPattern->fSets8[opValue];
                                if (s8->contains(c)) {
                                    success = TRUE;
                                    break;
                                }
                            } else {
                                UnicodeSet *s = (UnicodeSet *)sets->elementAt(opValue);
                                if (s->contains(c)) {
                                    success = TRUE;
                                    break;
                                }
                            }
                        } while (UTEXT_GETNATIVEINDEX(fInputText) >= backSearchIndex);
                        
                        if (success) {
                            fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                            fp->fInputIdx = UTEXT_GETNATIVEINDEX(fInputText);
                            if (fp->fInputIdx > backSearchIndex) {
                                fp = StateSave(fp, fp->fPatIdx, status);
                            }
                            fp->fPatIdx++; // Skip the LOOP_C, we just did that
                            break;
                        }
                    }
                }
                #endif
                fp = (REStackFrame *)fStack->popFrame(fFrameSize);
            }
            break;


        case URX_DOTANY:
            {
                // . matches anything, but stops at end-of-line.
                if (fp->fInputIdx >= fActiveLimit) {
                    // At end of input.  Match failed.  Backtrack out.
                    fHitEnd = TRUE;
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                    break;
                }
                
                UTEXT_SETNATIVEINDEX(fInputText, fp->fInputIdx);
                
                // There is input left.  Advance over one char, unless we've hit end-of-line
                UChar32 c = UTEXT_NEXT32(fInputText);
                if (((c & 0x7f) <= 0x29) &&     // First quickly bypass as many chars as possible
                    ((c<=0x0d && c>=0x0a) || c==0x85 ||c==0x2028 || c==0x2029)) {
                    // End of line in normal mode.   . does not match.
                        fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                    break;
                }
                fp->fInputIdx = UTEXT_GETNATIVEINDEX(fInputText);
            }
            break;


        case URX_DOTANY_ALL:
            {
                // ., in dot-matches-all (including new lines) mode
                if (fp->fInputIdx >= fActiveLimit) {
                    // At end of input.  Match failed.  Backtrack out.
                    fHitEnd = TRUE;
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                    break;
                }
                
                UTEXT_SETNATIVEINDEX(fInputText, fp->fInputIdx);
                
                // There is input left.  Advance over one char, except if we are
                //   at a cr/lf, advance over both of them.
                UChar32 c; 
                c = UTEXT_NEXT32(fInputText);
                fp->fInputIdx = UTEXT_GETNATIVEINDEX(fInputText);
                if (c==0x0d && fp->fInputIdx < fActiveLimit) {
                    // In the case of a CR/LF, we need to advance over both.
                    UChar32 nextc = UTEXT_CURRENT32(fInputText);
                    if (nextc == 0x0a) {
                        UTEXT_NEXT32(fInputText);
                        fp->fInputIdx = UTEXT_GETNATIVEINDEX(fInputText);
                    }
                }
            }
            break;


        case URX_DOTANY_UNIX:
            {
                // '.' operator, matches all, but stops at end-of-line.
                //   UNIX_LINES mode, so 0x0a is the only recognized line ending.
                if (fp->fInputIdx >= fActiveLimit) {
                    // At end of input.  Match failed.  Backtrack out.
                    fHitEnd = TRUE;
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                    break;
                }

                UTEXT_SETNATIVEINDEX(fInputText, fp->fInputIdx);
                
                // There is input left.  Advance over one char, unless we've hit end-of-line
                UChar32 c = UTEXT_NEXT32(fInputText);
                if (c == 0x0a) {
                    // End of line in normal mode.   '.' does not match the \n
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                } else {
                    fp->fInputIdx = UTEXT_GETNATIVEINDEX(fInputText);
                }
            }
            break;


        case URX_JMP:
            fp->fPatIdx = opValue;
            break;

        case URX_FAIL:
            isMatch = FALSE;
            goto breakFromLoop;

        case URX_JMP_SAV:
            U_ASSERT(opValue < fPattern->fCompiledPat->size());
            fp = StateSave(fp, fp->fPatIdx, status);       // State save to loc following current
            fp->fPatIdx = opValue;                         // Then JMP.
            break;

        case URX_JMP_SAV_X:
            // This opcode is used with (x)+, when x can match a zero length string.
            // Same as JMP_SAV, except conditional on the match having made forward progress.
            // Destination of the JMP must be a URX_STO_INP_LOC, from which we get the
            //   data address of the input position at the start of the loop.
            {
                U_ASSERT(opValue > 0 && opValue < fPattern->fCompiledPat->size());
                int32_t  stoOp = (int32_t)pat[opValue-1];
                U_ASSERT(URX_TYPE(stoOp) == URX_STO_INP_LOC);
                int32_t  frameLoc = URX_VAL(stoOp);
                U_ASSERT(frameLoc >= 0 && frameLoc < fFrameSize);
                int64_t prevInputIdx = fp->fExtra[frameLoc];
                U_ASSERT(prevInputIdx <= fp->fInputIdx);
                if (prevInputIdx < fp->fInputIdx) {
                    // The match did make progress.  Repeat the loop.
                    fp = StateSave(fp, fp->fPatIdx, status);  // State save to loc following current
                    fp->fPatIdx = opValue;
                    fp->fExtra[frameLoc] = fp->fInputIdx;
                } 
                // If the input position did not advance, we do nothing here,
                //   execution will fall out of the loop.
            }
            break;

        case URX_CTR_INIT:
            {
                U_ASSERT(opValue >= 0 && opValue < fFrameSize-2);
                fp->fExtra[opValue] = 0;       //  Set the loop counter variable to zero

                // Pick up the three extra operands that CTR_INIT has, and
                //    skip the pattern location counter past 
                int32_t instrOperandLoc = (int32_t)fp->fPatIdx;
                fp->fPatIdx += 3;
                int32_t loopLoc  = URX_VAL(pat[instrOperandLoc]);
                int32_t minCount = (int32_t)pat[instrOperandLoc+1];
                int32_t maxCount = (int32_t)pat[instrOperandLoc+2];
                U_ASSERT(minCount>=0);
                U_ASSERT(maxCount>=minCount || maxCount==-1);
                U_ASSERT(loopLoc>fp->fPatIdx);

                if (minCount == 0) {
                    fp = StateSave(fp, loopLoc+1, status);
                }
                if (maxCount == 0) {
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                }
            }
            break;

        case URX_CTR_LOOP:
            {
                U_ASSERT(opValue>0 && opValue < fp->fPatIdx-2);
                int32_t initOp = (int32_t)pat[opValue];
                U_ASSERT(URX_TYPE(initOp) == URX_CTR_INIT);
                int64_t *pCounter = &fp->fExtra[URX_VAL(initOp)];
                int32_t minCount  = (int32_t)pat[opValue+2];
                int32_t maxCount  = (int32_t)pat[opValue+3];
                // Increment the counter.  Note: we DIDN'T worry about counter
                //   overflow, since the data comes from UnicodeStrings, which
                //   stores its length in an int32_t. Do we have to think about
                //   this now that we're using UText? Probably not, since the length
                //   in UChar32s is still an int32_t.
                (*pCounter)++;
                U_ASSERT(*pCounter > 0);
                if ((uint64_t)*pCounter >= (uint32_t)maxCount) {
                    U_ASSERT(*pCounter == maxCount || maxCount == -1);
                    break;
                }
                if (*pCounter >= minCount) {
                    fp = StateSave(fp, fp->fPatIdx, status);
                }
                fp->fPatIdx = opValue + 4;    // Loop back.
            }
            break;

        case URX_CTR_INIT_NG:
            {
                // Initialize a non-greedy loop
                U_ASSERT(opValue >= 0 && opValue < fFrameSize-2);
                fp->fExtra[opValue] = 0;       //  Set the loop counter variable to zero

                // Pick up the three extra operands that CTR_INIT has, and
                //    skip the pattern location counter past 
                int32_t instrOperandLoc = (int32_t)fp->fPatIdx;
                fp->fPatIdx += 3;
                int32_t loopLoc  = URX_VAL(pat[instrOperandLoc]);
                int32_t minCount = (int32_t)pat[instrOperandLoc+1];
                int32_t maxCount = (int32_t)pat[instrOperandLoc+2];
                U_ASSERT(minCount>=0);
                U_ASSERT(maxCount>=minCount || maxCount==-1);
                U_ASSERT(loopLoc>fp->fPatIdx);

                if (minCount == 0) {
                    if (maxCount != 0) {
                        fp = StateSave(fp, fp->fPatIdx, status);
                    }
                    fp->fPatIdx = loopLoc+1;   // Continue with stuff after repeated block
                } 
            }
            break;

        case URX_CTR_LOOP_NG:
            {
                // Non-greedy {min, max} loops
                U_ASSERT(opValue>0 && opValue < fp->fPatIdx-2);
                int32_t initOp = (int32_t)pat[opValue];
                U_ASSERT(URX_TYPE(initOp) == URX_CTR_INIT_NG);
                int64_t *pCounter = &fp->fExtra[URX_VAL(initOp)];
                int32_t minCount  = (int32_t)pat[opValue+2];
                int32_t maxCount  = (int32_t)pat[opValue+3];
                // Increment the counter.  Note: we DIDN'T worry about counter
                //   overflow, since the data comes from UnicodeStrings, which
                //   stores its length in an int32_t. Do we have to think about
                //   this now that we're using UText? Probably not, since the length
                //   in UChar32s is still an int32_t.
                (*pCounter)++;
                U_ASSERT(*pCounter > 0);

                if ((uint64_t)*pCounter >= (uint32_t)maxCount) {
                    // The loop has matched the maximum permitted number of times.
                    //   Break out of here with no action.  Matching will
                    //   continue with the following pattern.
                    U_ASSERT(*pCounter == maxCount || maxCount == -1);
                    break;
                }

                if (*pCounter < minCount) {
                    // We haven't met the minimum number of matches yet.
                    //   Loop back for another one.
                    fp->fPatIdx = opValue + 4;    // Loop back.
                } else {
                    // We do have the minimum number of matches.
                    //   Fall into the following pattern, but first do
                    //   a state save to the top of the loop, so that a failure
                    //   in the following pattern will try another iteration of the loop.
                    fp = StateSave(fp, opValue + 4, status);
                }
            }
            break;

        case URX_STO_SP:
            U_ASSERT(opValue >= 0 && opValue < fPattern->fDataSize);
            fData[opValue] = fStack->size();
            break;

        case URX_LD_SP:
            {
                U_ASSERT(opValue >= 0 && opValue < fPattern->fDataSize);
                int32_t newStackSize = (int32_t)fData[opValue];
                U_ASSERT(newStackSize <= fStack->size());
                int64_t *newFP = fStack->getBuffer() + newStackSize - fFrameSize;
                if (newFP == (int64_t *)fp) {
                    break;
                }
                int32_t i;
                for (i=0; i<fFrameSize; i++) {
                    newFP[i] = ((int64_t *)fp)[i];
                }
                fp = (REStackFrame *)newFP;
                fStack->setSize(newStackSize);
            }
            break;

        case URX_BACKREF:
        case URX_BACKREF_I:
            {
                U_ASSERT(opValue < fFrameSize);
                int64_t groupStartIdx = fp->fExtra[opValue];
                int64_t groupEndIdx   = fp->fExtra[opValue+1];
                U_ASSERT(groupStartIdx <= groupEndIdx);
                if (groupStartIdx < 0) {
                    // This capture group has not participated in the match thus far,
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);   // FAIL, no match.
                }
                
                if (groupEndIdx == groupStartIdx) {
                    //   The capture group match was of an empty string.
                    //   Verified by testing:  Perl matches succeed in this case, so
                    //   we do too.
                    break;
                }
                
                UTEXT_SETNATIVEINDEX(fAltInputText, groupStartIdx);
                UTEXT_SETNATIVEINDEX(fInputText, fp->fInputIdx);
                
                UBool haveMatch = (opType == URX_BACKREF ?
                    (0 == utext_compareNativeLimit(fAltInputText, groupEndIdx, fInputText, -1)) :
                    (0 == utext_caseCompareNativeLimit(fAltInputText, groupEndIdx, fInputText, -1, U_FOLD_CASE_DEFAULT, &status)));
                fp->fInputIdx = UTEXT_GETNATIVEINDEX(fInputText);
                
                if (fp->fInputIdx > fActiveLimit) {
                    fHitEnd = TRUE;
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);   // FAIL, no match.
                } else if (!haveMatch) {
                    if (fp->fInputIdx == fActiveLimit) {
                        fHitEnd = TRUE;
                    }
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);   // FAIL, no match.
                }
            }
            break;
                
        case URX_STO_INP_LOC:
            {
                U_ASSERT(opValue >= 0 && opValue < fFrameSize);
                fp->fExtra[opValue] = fp->fInputIdx;
            }
            break;

        case URX_JMPX:
            {
                int32_t instrOperandLoc = (int32_t)fp->fPatIdx;
                fp->fPatIdx += 1;
                int32_t dataLoc  = URX_VAL(pat[instrOperandLoc]);
                U_ASSERT(dataLoc >= 0 && dataLoc < fFrameSize);
                int64_t savedInputIdx = fp->fExtra[dataLoc];
                U_ASSERT(savedInputIdx <= fp->fInputIdx);
                if (savedInputIdx < fp->fInputIdx) {
                    fp->fPatIdx = opValue;                               // JMP
                } else {
                     fp = (REStackFrame *)fStack->popFrame(fFrameSize);   // FAIL, no progress in loop.
                }
            }
            break;

        case URX_LA_START:
            {
                // Entering a lookahead block.
                // Save Stack Ptr, Input Pos.
                U_ASSERT(opValue>=0 && opValue+1<fPattern->fDataSize);
                fData[opValue]   = fStack->size();
                fData[opValue+1] = fp->fInputIdx;
                fActiveStart     = fLookStart;          // Set the match region change for
                fActiveLimit     = fLookLimit;          //   transparent bounds.
            }
            break;

        case URX_LA_END:
            {
                // Leaving a look-ahead block.
                //  restore Stack Ptr, Input Pos to positions they had on entry to block.
                U_ASSERT(opValue>=0 && opValue+1<fPattern->fDataSize);
                int32_t stackSize = fStack->size();
                int32_t newStackSize =(int32_t)fData[opValue];
                U_ASSERT(stackSize >= newStackSize);
                if (stackSize > newStackSize) {
                    // Copy the current top frame back to the new (cut back) top frame.
                    //   This makes the capture groups from within the look-ahead
                    //   expression available.
                    int64_t *newFP = fStack->getBuffer() + newStackSize - fFrameSize;
                    int32_t i;
                    for (i=0; i<fFrameSize; i++) {
                        newFP[i] = ((int64_t *)fp)[i];
                    }
                    fp = (REStackFrame *)newFP;
                    fStack->setSize(newStackSize);
                }
                fp->fInputIdx = fData[opValue+1];

                // Restore the active region bounds in the input string; they may have
                //    been changed because of transparent bounds on a Region.
                fActiveStart = fRegionStart;
                fActiveLimit = fRegionLimit;
            }
            break;

        case URX_ONECHAR_I:
            if (fp->fInputIdx < fActiveLimit) {
                UTEXT_SETNATIVEINDEX(fInputText, fp->fInputIdx);

                UChar32 c = UTEXT_NEXT32(fInputText);
                if (u_foldCase(c, U_FOLD_CASE_DEFAULT) == opValue) {
                    fp->fInputIdx = UTEXT_GETNATIVEINDEX(fInputText);
                    break;
                }
            } else {
                fHitEnd = TRUE;
            }
            
            #ifdef REGEX_SMART_BACKTRACKING
            if (fp->fInputIdx > backSearchIndex && fStack->size() > fFrameSize) {
                REStackFrame *prevFrame = (REStackFrame *)fStack->peekFrame(fFrameSize);
                if (URX_LOOP_C == URX_TYPE(pat[prevFrame->fPatIdx]) && fp->fInputIdx <= prevFrame->fInputIdx) {
                    UBool success = FALSE;
                    UChar32 c = UTEXT_PREVIOUS32(fInputText);
                    while (UTEXT_GETNATIVEINDEX(fInputText) >= backSearchIndex) {
                        if (u_foldCase(c, U_FOLD_CASE_DEFAULT) == opValue) {
                            success = TRUE;
                            break;
                        } else if (c == U_SENTINEL) {
                            break;
                        }
                        c = UTEXT_PREVIOUS32(fInputText);
                    }
                    if (success) {
                        fHitEnd = FALSE;
                        fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                        fp->fInputIdx = UTEXT_GETNATIVEINDEX(fInputText);
                        if (fp->fInputIdx > backSearchIndex) {
                            fp = StateSave(fp, fp->fPatIdx, status);
                        }
                        fp->fPatIdx++; // Skip the LOOP_C, we just did that
                        break;
                    }
                }
            }
            #endif

            fp = (REStackFrame *)fStack->popFrame(fFrameSize);
            break;

        case URX_STRING_I:
            {
                // Test input against a literal string.
                // Strings require two slots in the compiled pattern, one for the
                //   offset to the string text, and one for the length.
                const UCaseProps *csp = ucase_getSingleton();
                {
                    int32_t stringStartIdx, stringLen;
                    stringStartIdx = opValue;

                    op      = (int32_t)pat[fp->fPatIdx];
                    fp->fPatIdx++;
                    opType  = URX_TYPE(op);
                    opValue = URX_VAL(op);
                    U_ASSERT(opType == URX_STRING_LEN);
                    stringLen = opValue;
                
                    const UChar *patternChars = litText+stringStartIdx;
                    const UChar *patternEnd = patternChars+stringLen;
                    
                    const UChar *foldChars = NULL;
                    int32_t foldOffset, foldLength;
                    UChar32 c;
                    
                    foldOffset = foldLength = 0;
                    UBool success = TRUE;
                    
                    UTEXT_SETNATIVEINDEX(fInputText, fp->fInputIdx);
                    while (patternChars < patternEnd && success) {
                        if(foldOffset < foldLength) {
                            U16_NEXT_UNSAFE(foldChars, foldOffset, c);
                        } else {
                            c = UTEXT_NEXT32(fInputText);
                            if (c != U_SENTINEL) {
                                foldLength = ucase_toFullFolding(csp, c, &foldChars, U_FOLD_CASE_DEFAULT);
                                if(foldLength >= 0) {
                                    if(foldLength <= UCASE_MAX_STRING_LENGTH) {   // !!!: Does not correctly handle chars that fold to 0-length strings
                                        foldOffset = 0;
                                        U16_NEXT_UNSAFE(foldChars, foldOffset, c);
                                    } else {
                                        c = foldLength;
                                        foldLength = foldOffset; // to avoid reading chars from the folding buffer
                                    }
                                }
                            }
                            
                            fp->fInputIdx = UTEXT_GETNATIVEINDEX(fInputText);
                        }
                        
                        success = FALSE;
                        if (c != U_SENTINEL && (fp->fInputIdx <= fActiveLimit)) {
                            if (U_IS_BMP(c)) {
                                success = (*patternChars == c);
                                patternChars += 1;
                            } else if (patternChars+1 < patternEnd) {
                                success = (*patternChars == U16_LEAD(c) && *(patternChars+1) == U16_TRAIL(c));
                                patternChars += 2;
                            }
                        } else {
                            fHitEnd = TRUE;          //   TODO:  See ticket 6074
                        }
                    }
                    
                    if (!success) {
                        #ifdef REGEX_SMART_BACKTRACKING
                        if (fp->fInputIdx > backSearchIndex && fStack->size()) {
                            REStackFrame *prevFrame = (REStackFrame *)fStack->peekFrame(fFrameSize);
                            if (URX_LOOP_C == URX_TYPE(pat[prevFrame->fPatIdx]) && fp->fInputIdx <= prevFrame->fInputIdx) {
                                // Reset to last start point
                                UTEXT_SETNATIVEINDEX(fInputText, fp->fInputIdx);
                                patternChars = litText+stringStartIdx;
                                
                                // Search backwards for a possible start
                                do {
                                    c = UTEXT_PREVIOUS32(fInputText);
                                    if (c == U_SENTINEL) {
                                        break;
                                    } else {
                                        foldLength = ucase_toFullFolding(csp, c, &foldChars, U_FOLD_CASE_DEFAULT);
                                        if(foldLength >= 0) {
                                            if(foldLength <= UCASE_MAX_STRING_LENGTH) {   // !!!: Does not correctly handle chars that fold to 0-length strings
                                                foldOffset = 0;
                                                U16_NEXT_UNSAFE(foldChars, foldOffset, c);
                                            } else {
                                                c = foldLength;
                                                foldLength = foldOffset; // to avoid reading chars from the folding buffer
                                            }
                                        }
                                        
                                        if ((U_IS_BMP(c) && *patternChars == c) ||
                                               (*patternChars == U16_LEAD(c) && *(patternChars+1) == U16_TRAIL(c))) {
                                            success = TRUE;
                                            break;
                                        }
                                    }
                                } while (UTEXT_GETNATIVEINDEX(fInputText) >= backSearchIndex);
                                
                                // And try again
                                if (success) {
                                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                                    fp->fInputIdx = UTEXT_GETNATIVEINDEX(fInputText);
                                    if (fp->fInputIdx > backSearchIndex) {
                                        fp = StateSave(fp, fp->fPatIdx, status);
                                    }
                                    fp->fPatIdx++; // Skip the LOOP_C, we just did that
                                    break;
                                }
                            }
                        }
                        #endif
                        fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                    }
                }
            }
            break;

        case URX_LB_START:
            {
                // Entering a look-behind block.
                // Save Stack Ptr, Input Pos.
                //   TODO:  implement transparent bounds.  Ticket #6067
                U_ASSERT(opValue>=0 && opValue+1<fPattern->fDataSize);
                fData[opValue]   = fStack->size();
                fData[opValue+1] = fp->fInputIdx;
                // Init the variable containing the start index for attempted matches.
                fData[opValue+2] = -1;
                // Save input string length, then reset to pin any matches to end at
                //   the current position.
                fData[opValue+3] = fActiveLimit;
                fActiveLimit     = fp->fInputIdx;
            }
            break;


        case URX_LB_CONT:
            {
                // Positive Look-Behind, at top of loop checking for matches of LB expression
                //    at all possible input starting positions.

                // Fetch the min and max possible match lengths.  They are the operands
                //   of this op in the pattern.
                int32_t minML = (int32_t)pat[fp->fPatIdx++];
                int32_t maxML = (int32_t)pat[fp->fPatIdx++];
                U_ASSERT(minML <= maxML);
                U_ASSERT(minML >= 0);

                // Fetch (from data) the last input index where a match was attempted.
                U_ASSERT(opValue>=0 && opValue+1<fPattern->fDataSize);
                int64_t  *lbStartIdx = &fData[opValue+2];
                if (*lbStartIdx < 0) {
                    // First time through loop.
                    *lbStartIdx = fp->fInputIdx - minML;
                } else {
                    // 2nd through nth time through the loop.
                    // Back up start position for match by one.
                    if (*lbStartIdx == 0) {
                        (*lbStartIdx)--;
                    } else {
                        UTEXT_SETNATIVEINDEX(fInputText, *lbStartIdx);
                        UTEXT_PREVIOUS32(fInputText);
                        *lbStartIdx = UTEXT_GETNATIVEINDEX(fInputText);
                    }
                }

                if (*lbStartIdx < 0 || *lbStartIdx < fp->fInputIdx - maxML) {
                    // We have tried all potential match starting points without
                    //  getting a match.  Backtrack out, and out of the
                    //   Look Behind altogether.
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                    int64_t restoreInputLen = fData[opValue+3];
                    U_ASSERT(restoreInputLen >= fActiveLimit);
                    U_ASSERT(restoreInputLen <= fInputLength);
                    fActiveLimit = restoreInputLen;
                    break;
                }

                //    Save state to this URX_LB_CONT op, so failure to match will repeat the loop.
                //      (successful match will fall off the end of the loop.)
                fp = StateSave(fp, fp->fPatIdx-3, status);
                fp->fInputIdx = *lbStartIdx;
            }
            break;

        case URX_LB_END:
            // End of a look-behind block, after a successful match.
            {
                U_ASSERT(opValue>=0 && opValue+1<fPattern->fDataSize);
                if (fp->fInputIdx != fActiveLimit) {
                    //  The look-behind expression matched, but the match did not
                    //    extend all the way to the point that we are looking behind from.
                    //  FAIL out of here, which will take us back to the LB_CONT, which
                    //     will retry the match starting at another position or fail
                    //     the look-behind altogether, whichever is appropriate.
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                    break;
                }

                // Look-behind match is good.  Restore the orignal input string length,
                //   which had been truncated to pin the end of the lookbehind match to the 
                //   position being looked-behind.
                int64_t originalInputLen = fData[opValue+3];
                U_ASSERT(originalInputLen >= fActiveLimit);
                U_ASSERT(originalInputLen <= fInputLength);
                fActiveLimit = originalInputLen;
            }
            break;


        case URX_LBN_CONT:
            {
                // Negative Look-Behind, at top of loop checking for matches of LB expression
                //    at all possible input starting positions.

                // Fetch the extra parameters of this op.
                int32_t minML       = (int32_t)pat[fp->fPatIdx++];
                int32_t maxML       = (int32_t)pat[fp->fPatIdx++];
                int32_t continueLoc = (int32_t)pat[fp->fPatIdx++];
                        continueLoc = URX_VAL(continueLoc);
                U_ASSERT(minML <= maxML);
                U_ASSERT(minML >= 0);
                U_ASSERT(continueLoc > fp->fPatIdx);

                // Fetch (from data) the last input index where a match was attempted.
                U_ASSERT(opValue>=0 && opValue+1<fPattern->fDataSize);
                int64_t  *lbStartIdx = &fData[opValue+2];
                if (*lbStartIdx < 0) {
                    // First time through loop.
                    *lbStartIdx = fp->fInputIdx - minML;
                } else {
                    // 2nd through nth time through the loop.
                    // Back up start position for match by one.
                    if (*lbStartIdx == 0) {
                        (*lbStartIdx)--;
                    } else {
                        UTEXT_SETNATIVEINDEX(fInputText, *lbStartIdx);
                        UTEXT_PREVIOUS32(fInputText);
                        *lbStartIdx = UTEXT_GETNATIVEINDEX(fInputText);
                    }
                }

                if (*lbStartIdx < 0 || *lbStartIdx < fp->fInputIdx - maxML) {
                    // We have tried all potential match starting points without
                    //  getting a match, which means that the negative lookbehind as
                    //  a whole has succeeded.  Jump forward to the continue location
                    int64_t restoreInputLen = fData[opValue+3];
                    U_ASSERT(restoreInputLen >= fActiveLimit);
                    U_ASSERT(restoreInputLen <= fInputLength);
                    fActiveLimit = restoreInputLen;
                    fp->fPatIdx = continueLoc;
                    break;
                }

                //    Save state to this URX_LB_CONT op, so failure to match will repeat the loop.
                //      (successful match will cause a FAIL out of the loop altogether.)
                fp = StateSave(fp, fp->fPatIdx-4, status);
                fp->fInputIdx = *lbStartIdx;
            }
            break;

        case URX_LBN_END:
            // End of a negative look-behind block, after a successful match.
            {
                U_ASSERT(opValue>=0 && opValue+1<fPattern->fDataSize);
                if (fp->fInputIdx != fActiveLimit) {
                    //  The look-behind expression matched, but the match did not
                    //    extend all the way to the point that we are looking behind from.
                    //  FAIL out of here, which will take us back to the LB_CONT, which
                    //     will retry the match starting at another position or succeed
                    //     the look-behind altogether, whichever is appropriate.
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                    break;
                }

                // Look-behind expression matched, which means look-behind test as
                //   a whole Fails
                
                //   Restore the orignal input string length, which had been truncated 
                //   inorder to pin the end of the lookbehind match  
                //   to the position being looked-behind.
                int64_t originalInputLen = fData[opValue+3];
                U_ASSERT(originalInputLen >= fActiveLimit);
                U_ASSERT(originalInputLen <= fInputLength);
                fActiveLimit = originalInputLen;

                // Restore original stack position, discarding any state saved
                //   by the successful pattern match.
                U_ASSERT(opValue>=0 && opValue+1<fPattern->fDataSize);
                int32_t newStackSize = (int32_t)fData[opValue];
                U_ASSERT(fStack->size() > newStackSize);
                fStack->setSize(newStackSize);
                
                //  FAIL, which will take control back to someplace 
                //  prior to entering the look-behind test.
                fp = (REStackFrame *)fStack->popFrame(fFrameSize);
            }
            break;


        case URX_LOOP_SR_I:
            // Loop Initialization for the optimized implementation of
            //     [some character set]*
            //   This op scans through all matching input.
            //   The following LOOP_C op emulates stack unwinding if the following pattern fails.
            {
                U_ASSERT(opValue > 0 && opValue < sets->size());
                Regex8BitSet *s8 = &fPattern->fSets8[opValue];
                UnicodeSet   *s  = (UnicodeSet *)sets->elementAt(opValue);

                // Loop through input, until either the input is exhausted or
                //   we reach a character that is not a member of the set.
                int64_t ix = fp->fInputIdx;
                UTEXT_SETNATIVEINDEX(fInputText, ix);
                for (;;) {
                    if (ix >= fActiveLimit) {
                        fHitEnd = TRUE;
                        break;
                    }
                    UChar32 c = UTEXT_NEXT32(fInputText);
                    if (c<256) {
                        if (s8->contains(c) == FALSE) {
                            break;
                        }
                    } else {
                        if (s->contains(c) == FALSE) {
                            break;
                        }
                    }
                    ix = UTEXT_GETNATIVEINDEX(fInputText);
                }

                // If there were no matching characters, skip over the loop altogether.
                //   The loop doesn't run at all, a * op always succeeds.
                if (ix == fp->fInputIdx) {
                    fp->fPatIdx++;   // skip the URX_LOOP_C op.
                    break;
                }

                // Peek ahead in the compiled pattern, to the URX_LOOP_C that
                //   must follow.  It's operand is the stack location
                //   that holds the starting input index for the match of this [set]*
                int32_t loopcOp = (int32_t)pat[fp->fPatIdx];
                U_ASSERT(URX_TYPE(loopcOp) == URX_LOOP_C);
                int32_t stackLoc = URX_VAL(loopcOp);
                U_ASSERT(stackLoc >= 0 && stackLoc < fFrameSize);
                fp->fExtra[stackLoc] = fp->fInputIdx;
                #ifdef REGEX_SMART_BACKTRACKING
                backSearchIndex = fp->fInputIdx;
                #endif
                fp->fInputIdx = ix;

                // Save State to the URX_LOOP_C op that follows this one,
                //   so that match failures in the following code will return to there.
                //   Then bump the pattern idx so the LOOP_C is skipped on the way out of here.
                fp = StateSave(fp, fp->fPatIdx, status);
                fp->fPatIdx++;
            }
            break;


        case URX_LOOP_DOT_I:
            // Loop Initialization for the optimized implementation of .*
            //   This op scans through all remaining input.
            //   The following LOOP_C op emulates stack unwinding if the following pattern fails.
            {
                // Loop through input until the input is exhausted (we reach an end-of-line)
                // In DOTALL mode, we can just go straight to the end of the input.
                int64_t ix;
                if ((opValue & 1) == 1) {
                    // Dot-matches-All mode.  Jump straight to the end of the string.
                    ix = fActiveLimit;
                    fHitEnd = TRUE;
                } else {
                    // NOT DOT ALL mode.  Line endings do not match '.'
                    // Scan forward until a line ending or end of input.
                    ix = fp->fInputIdx;
                    UTEXT_SETNATIVEINDEX(fInputText, ix);
                    for (;;) {
                        if (ix >= fActiveLimit) {
                            fHitEnd = TRUE;
                            break;
                        }
                        UChar32 c = UTEXT_NEXT32(fInputText);
                        if ((c & 0x7f) <= 0x29) {          // Fast filter of non-new-line-s
                            if ((c == 0x0a) ||             //  0x0a is newline in both modes.
                               (((opValue & 2) == 0) &&    // IF not UNIX_LINES mode
                                    (c<=0x0d && c>=0x0a)) || c==0x85 ||c==0x2028 || c==0x2029) {
                                //  char is a line ending.  Exit the scanning loop.
                                break;
                            }
                        }
                        ix = UTEXT_GETNATIVEINDEX(fInputText);
                    }
                }

                // If there were no matching characters, skip over the loop altogether.
                //   The loop doesn't run at all, a * op always succeeds.
                if (ix == fp->fInputIdx) {
                    fp->fPatIdx++;   // skip the URX_LOOP_C op.
                    break;
                }

                // Peek ahead in the compiled pattern, to the URX_LOOP_C that
                //   must follow.  It's operand is the stack location
                //   that holds the starting input index for the match of this .*
                int32_t loopcOp = (int32_t)pat[fp->fPatIdx];
                U_ASSERT(URX_TYPE(loopcOp) == URX_LOOP_C);
                int32_t stackLoc = URX_VAL(loopcOp);
                U_ASSERT(stackLoc >= 0 && stackLoc < fFrameSize);
                fp->fExtra[stackLoc] = fp->fInputIdx;
                #ifdef REGEX_SMART_BACKTRACKING
                backSearchIndex = fp->fInputIdx;
                #endif
                fp->fInputIdx = ix;

                // Save State to the URX_LOOP_C op that follows this one,
                //   so that match failures in the following code will return to there.
                //   Then bump the pattern idx so the LOOP_C is skipped on the way out of here.
                fp = StateSave(fp, fp->fPatIdx, status);
                fp->fPatIdx++;
            }
            break;


        case URX_LOOP_C:
            {
                U_ASSERT(opValue>=0 && opValue<fFrameSize);
                backSearchIndex = fp->fExtra[opValue];
                U_ASSERT(backSearchIndex <= fp->fInputIdx);
                if (backSearchIndex == fp->fInputIdx) {
                    // We've backed up the input idx to the point that the loop started.
                    // The loop is done.  Leave here without saving state.   
                    //  Subsequent failures won't come back here.
                    break;
                }
                // Set up for the next iteration of the loop, with input index
                //   backed up by one from the last time through,
                //   and a state save to this instruction in case the following code fails again.
                //   (We're going backwards because this loop emulates stack unwinding, not
                //    the initial scan forward.)
                U_ASSERT(fp->fInputIdx > 0);
                UTEXT_SETNATIVEINDEX(fInputText, fp->fInputIdx);
                UChar32 prevC = UTEXT_PREVIOUS32(fInputText);
                fp->fInputIdx = UTEXT_GETNATIVEINDEX(fInputText);
                
                UChar32 twoPrevC = UTEXT_PREVIOUS32(fInputText);
                if (prevC == 0x0a && 
                    fp->fInputIdx > backSearchIndex &&
                    twoPrevC == 0x0d) {
                    int32_t prevOp = (int32_t)pat[fp->fPatIdx-2];
                    if (URX_TYPE(prevOp) == URX_LOOP_DOT_I) {
                        // .*, stepping back over CRLF pair.
                        fp->fInputIdx = UTEXT_GETNATIVEINDEX(fInputText);
                    }
                }


                fp = StateSave(fp, fp->fPatIdx-1, status);
            }
            break;



        default:
            // Trouble.  The compiled pattern contains an entry with an
            //           unrecognized type tag.
            U_ASSERT(FALSE);
        }

        if (U_FAILURE(status)) {
            isMatch = FALSE;
            break;
        }
    }
    
breakFromLoop:
    fMatch = isMatch;
    if (isMatch) {
        fLastMatchEnd = fMatchEnd;
        fMatchStart   = startIdx;
        fMatchEnd     = fp->fInputIdx;
        if (fTraceDebug) {
            REGEX_RUN_DEBUG_PRINTF(("Match.  start=%d   end=%d\n\n", fMatchStart, fMatchEnd));
        }
    }
    else
    {
        if (fTraceDebug) {
            REGEX_RUN_DEBUG_PRINTF(("No match\n\n"));
        }
    }

    fFrame = fp;                // The active stack frame when the engine stopped.
                                //   Contains the capture group results that we need to
                                //    access later.
    return;
}


//--------------------------------------------------------------------------------
//
//   MatchChunkAt   This is the actual matching engine. Like MatchAt, but with the
//                  assumption that the entire string is available in the UText's
//                  chunk buffer. For now, that means we can use int32_t indexes,
//                  except for anything that needs to be saved (like group starts
//                  and ends).
//
//                  startIdx:    begin matching a this index.
//                  toEnd:       if true, match must extend to end of the input region
//
//--------------------------------------------------------------------------------
void RegexMatcher::MatchChunkAt(int32_t startIdx, UBool toEnd, UErrorCode &status) {
    UBool       isMatch  = FALSE;      // True if the we have a match.
    
    int32_t     backSearchIndex = INT32_MAX; // used after greedy single-character matches for searching backwards

    int32_t     op;                    // Operation from the compiled pattern, split into
    int32_t     opType;                //    the opcode
    int32_t     opValue;               //    and the operand value.
    
#ifdef REGEX_RUN_DEBUG
    if (fTraceDebug)
    {
        printf("MatchAt(startIdx=%ld)\n", startIdx);
        printf("Original Pattern: ");
        UChar32 c = utext_next32From(fPattern->fPattern, 0);
        while (c != U_SENTINEL) {
            if (c<32 || c>256) {
                c = '.';
            }
            REGEX_DUMP_DEBUG_PRINTF(("%c", c));
            
            c = UTEXT_NEXT32(fPattern->fPattern);
        }
        printf("\n");
        printf("Input String: ");
        c = utext_next32From(fInputText, 0);
        while (c != U_SENTINEL) {
            if (c<32 || c>256) {
                c = '.';
            }
            printf("%c", c);
            
            c = UTEXT_NEXT32(fInputText);
        }
        printf("\n");
        printf("\n");
    }
#endif
    
    if (U_FAILURE(status)) {
        return;
    }
    
    //  Cache frequently referenced items from the compiled pattern
    //
    int64_t             *pat           = fPattern->fCompiledPat->getBuffer();
    
    const UChar         *litText       = fPattern->fLiteralText.getBuffer();
    UVector             *sets          = fPattern->fSets;
    
    const UChar         *inputBuf      = fInputText->chunkContents;
    
    fFrameSize = fPattern->fFrameSize;
    REStackFrame        *fp            = resetStack();
    
    fp->fPatIdx   = 0;
    fp->fInputIdx = startIdx;
    
    // Zero out the pattern's static data
    int32_t i;
    for (i = 0; i<fPattern->fDataSize; i++) {
        fData[i] = 0;
    }
    
    //
    //  Main loop for interpreting the compiled pattern.
    //  One iteration of the loop per pattern operation performed.
    //
    for (;;) {
#if 0
        if (_heapchk() != _HEAPOK) {
            fprintf(stderr, "Heap Trouble\n");
        }
#endif
        
        op      = (int32_t)pat[fp->fPatIdx];
        opType  = URX_TYPE(op);
        opValue = URX_VAL(op);
#ifdef REGEX_RUN_DEBUG
        if (fTraceDebug) {
            UTEXT_SETNATIVEINDEX(fInputText, fp->fInputIdx);
            printf("inputIdx=%d   inputChar=%x   sp=%3d   activeLimit=%d  ", fp->fInputIdx,
                   UTEXT_CURRENT32(fInputText), (int64_t *)fp-fStack->getBuffer(), fActiveLimit);
            fPattern->dumpOp(fp->fPatIdx);
        }
#endif
        fp->fPatIdx++;
        
        switch (opType) {
                
                
        case URX_NOP:
            break;
            
            
        case URX_BACKTRACK:
            // Force a backtrack.  In some circumstances, the pattern compiler
            //   will notice that the pattern can't possibly match anything, and will
            //   emit one of these at that point.
            fp = (REStackFrame *)fStack->popFrame(fFrameSize);
            break;
            
            
        case URX_ONECHAR:
            if (fp->fInputIdx < fActiveLimit) {
                UChar32 c;
                U16_NEXT(inputBuf, fp->fInputIdx, fActiveLimit, c);
                if (c == opValue) {
                    break;
                }
            } else {
                fHitEnd = TRUE;
            }

            #ifdef REGEX_SMART_BACKTRACKING
            if (fp->fInputIdx > backSearchIndex && fStack->size() > fFrameSize) {
                REStackFrame *prevFrame = (REStackFrame *)fStack->peekFrame(fFrameSize);
                if (URX_LOOP_C == URX_TYPE(pat[prevFrame->fPatIdx]) && fp->fInputIdx <= prevFrame->fInputIdx) {
                    int64_t reverseIndex = fp->fInputIdx;
                    UChar32 c;
                    do {
                        U16_PREV(inputBuf, backSearchIndex, reverseIndex, c);
                        if (c == opValue) {
                            break;
                        }
                    } while (reverseIndex > backSearchIndex);
                    if (c == opValue) {
                        fHitEnd = FALSE;
                        fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                        fp->fInputIdx = reverseIndex;
                        if (fp->fInputIdx > backSearchIndex) {
                            fp = StateSave(fp, fp->fPatIdx, status);
                        }
                        fp->fPatIdx++; // Skip the LOOP_C, we just did that
                        break;
                    }
                }
            }
            #endif

            fp = (REStackFrame *)fStack->popFrame(fFrameSize);
            break;
            
            
        case URX_STRING:
            {
                // Test input against a literal string.
                // Strings require two slots in the compiled pattern, one for the
                //   offset to the string text, and one for the length.
                int32_t   stringStartIdx = opValue;
                int32_t   stringLen;
                
                op      = (int32_t)pat[fp->fPatIdx];     // Fetch the second operand
                fp->fPatIdx++;
                opType    = URX_TYPE(op);
                stringLen = URX_VAL(op);
                U_ASSERT(opType == URX_STRING_LEN);
                U_ASSERT(stringLen >= 2);
                
                if (fp->fInputIdx + stringLen > fActiveLimit) {
                    // No match.  String is longer than the remaining input text.
                    fHitEnd = TRUE;          //   TODO:  See ticket 6074
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                    break;
                }
                
                const UChar * pInp = inputBuf + fp->fInputIdx;
                const UChar * pPat = litText+stringStartIdx;
                const UChar * pEnd = pInp + stringLen;
                UBool success = FALSE;
                for(;;) {
                    if (*pInp == *pPat) {
                        pInp++;
                        pPat++;
                        if (pInp == pEnd) {
                            // Successful Match.
                            success = TRUE;
                            break;
                        }
                    } else {
                        // Match failed.
                        break;
                    }
                }
                
                if (success) {
                    fp->fInputIdx += stringLen;
                } else {
                    #ifdef REGEX_SMART_BACKTRACKING
                    if (fp->fInputIdx > backSearchIndex && fStack->size()) {
                        REStackFrame *prevFrame = (REStackFrame *)fStack->peekFrame(fFrameSize);
                        if (URX_LOOP_C == URX_TYPE(pat[prevFrame->fPatIdx]) && fp->fInputIdx <= prevFrame->fInputIdx) {
                            // Reset to last start point
                            int64_t reverseIndex = fp->fInputIdx;
                            UChar32 c;
                            pPat = litText+stringStartIdx;
                            
                            // Search backwards for a possible start
                            do {
                                U16_PREV(inputBuf, backSearchIndex, reverseIndex, c);
                                if ((U_IS_BMP(c) && *pPat == c) ||
                                    (*pPat == U16_LEAD(c) && *(pPat+1) == U16_TRAIL(c))) {
                                    success = TRUE;
                                    break;
                                }
                            } while (reverseIndex > backSearchIndex);
                            
                            // And try again
                            if (success) {
                                fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                                fp->fInputIdx = reverseIndex;
                                if (fp->fInputIdx > backSearchIndex) {
                                    fp = StateSave(fp, fp->fPatIdx, status);
                                }
                                fp->fPatIdx++; // Skip the LOOP_C, we just did that
                                break;
                            }
                        }
                    }
                    #endif
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                }
            }
            break;
            
            
        case URX_STATE_SAVE:
            fp = StateSave(fp, opValue, status);
            break;
            
            
        case URX_END:
            // The match loop will exit via this path on a successful match,
            //   when we reach the end of the pattern.
            if (toEnd && fp->fInputIdx != fActiveLimit) {
                // The pattern matched, but not to the end of input.  Try some more.
                fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                break;
            }
            isMatch = TRUE;
            goto  breakFromLoop;
            
            // Start and End Capture stack frame variables are laid out out like this:
            //  fp->fExtra[opValue]  - The start of a completed capture group
            //             opValue+1 - The end   of a completed capture group
            //             opValue+2 - the start of a capture group whose end
            //                          has not yet been reached (and might not ever be).
        case URX_START_CAPTURE:
            U_ASSERT(opValue >= 0 && opValue < fFrameSize-3);
            fp->fExtra[opValue+2] = fp->fInputIdx;
            break;
            
            
        case URX_END_CAPTURE:
            U_ASSERT(opValue >= 0 && opValue < fFrameSize-3);
            U_ASSERT(fp->fExtra[opValue+2] >= 0);            // Start pos for this group must be set.
            fp->fExtra[opValue]   = fp->fExtra[opValue+2];   // Tentative start becomes real.
            fp->fExtra[opValue+1] = fp->fInputIdx;           // End position
            U_ASSERT(fp->fExtra[opValue] <= fp->fExtra[opValue+1]);
            break;
            
            
        case URX_DOLLAR:                   //  $, test for End of line
            //     or for position before new line at end of input
            if (fp->fInputIdx < fAnchorLimit-2) {
                // We are no where near the end of input.  Fail.
                //   This is the common case.  Keep it first.
                fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                break;
            }
            if (fp->fInputIdx >= fAnchorLimit) {
                // We really are at the end of input.  Success.
                fHitEnd = TRUE;
                fRequireEnd = TRUE;
                break;
            }
            
            // If we are positioned just before a new-line that is located at the
            //   end of input, succeed.
            if (fp->fInputIdx == fAnchorLimit-1) {
                UChar32 c;
                U16_GET(inputBuf, fAnchorStart, fp->fInputIdx, fAnchorLimit, c);
                
                if ((c>=0x0a && c<=0x0d) || c==0x85 || c==0x2028 || c==0x2029) {
                    if ( !(c==0x0a && fp->fInputIdx>fAnchorStart && inputBuf[fp->fInputIdx-1]==0x0d)) {
                        // At new-line at end of input. Success
                        fHitEnd = TRUE;
                        fRequireEnd = TRUE;
                        break;
                    }
                }
            } else if (fp->fInputIdx == fAnchorLimit-2 &&
                inputBuf[fp->fInputIdx]==0x0d && inputBuf[fp->fInputIdx+1]==0x0a) {
                    fHitEnd = TRUE;
                    fRequireEnd = TRUE;
                    break;                         // At CR/LF at end of input.  Success
            }
            
            fp = (REStackFrame *)fStack->popFrame(fFrameSize);
            
            break;
            
            
        case URX_DOLLAR_D:                   //  $, test for End of Line, in UNIX_LINES mode.
            if (fp->fInputIdx >= fAnchorLimit-1) {
                // Either at the last character of input, or off the end.
                if (fp->fInputIdx == fAnchorLimit-1) {
                    // At last char of input.  Success if it's a new line.
                    if (inputBuf[fp->fInputIdx] == 0x0a) {
                        fHitEnd = TRUE;
                        fRequireEnd = TRUE;
                        break;
                    }
                } else {
                    // Off the end of input.  Success.
                    fHitEnd = TRUE;
                    fRequireEnd = TRUE;
                    break;
                }
            }
            
            // Not at end of input.  Back-track out.
            fp = (REStackFrame *)fStack->popFrame(fFrameSize);
            break;
            
            
        case URX_DOLLAR_M:                //  $, test for End of line in multi-line mode
            {
                if (fp->fInputIdx >= fAnchorLimit) {
                    // We really are at the end of input.  Success.
                    fHitEnd = TRUE;
                    fRequireEnd = TRUE;
                    break;
                }
                // If we are positioned just before a new-line, succeed.
                // It makes no difference where the new-line is within the input.
                UChar32 c = inputBuf[fp->fInputIdx];
                if ((c>=0x0a && c<=0x0d) || c==0x85 ||c==0x2028 || c==0x2029) {
                    // At a line end, except for the odd chance of  being in the middle of a CR/LF sequence
                    //  In multi-line mode, hitting a new-line just before the end of input does not
                    //   set the hitEnd or requireEnd flags
                    if ( !(c==0x0a && fp->fInputIdx>fAnchorStart && inputBuf[fp->fInputIdx-1]==0x0d)) {
                        break;
                    }
                }
                // not at a new line.  Fail.
                fp = (REStackFrame *)fStack->popFrame(fFrameSize);
            }
            break;
            
            
        case URX_DOLLAR_MD:                //  $, test for End of line in multi-line and UNIX_LINES mode
            {
                if (fp->fInputIdx >= fAnchorLimit) {
                    // We really are at the end of input.  Success.
                    fHitEnd = TRUE;
                    fRequireEnd = TRUE;  // Java set requireEnd in this case, even though
                    break;               //   adding a new-line would not lose the match.
                }
                // If we are not positioned just before a new-line, the test fails; backtrack out.
                // It makes no difference where the new-line is within the input.
                if (inputBuf[fp->fInputIdx] != 0x0a) {
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                }
            }
            break;
            
            
        case URX_CARET:                    //  ^, test for start of line
            if (fp->fInputIdx != fAnchorStart) {
                fp = (REStackFrame *)fStack->popFrame(fFrameSize);
            }
            break;
            
            
        case URX_CARET_M:                   //  ^, test for start of line in mulit-line mode
            {
                if (fp->fInputIdx == fAnchorStart) {
                    // We are at the start input.  Success.
                    break;
                }
                // Check whether character just before the current pos is a new-line
                //   unless we are at the end of input
                UChar  c = inputBuf[fp->fInputIdx - 1]; 
                if ((fp->fInputIdx < fAnchorLimit) && 
                    ((c<=0x0d && c>=0x0a) || c==0x85 ||c==0x2028 || c==0x2029)) {
                    //  It's a new-line.  ^ is true.  Success.
                    //  TODO:  what should be done with positions between a CR and LF?
                    break;
                }
                // Not at the start of a line.  Fail.
                fp = (REStackFrame *)fStack->popFrame(fFrameSize);
            }
            break;
            
            
        case URX_CARET_M_UNIX:       //  ^, test for start of line in mulit-line + Unix-line mode
            {
                U_ASSERT(fp->fInputIdx >= fAnchorStart);
                if (fp->fInputIdx <= fAnchorStart) {
                    // We are at the start input.  Success.
                    break;
                }
                // Check whether character just before the current pos is a new-line
                U_ASSERT(fp->fInputIdx <= fAnchorLimit);
                UChar  c = inputBuf[fp->fInputIdx - 1]; 
                if (c != 0x0a) {
                    // Not at the start of a line.  Back-track out.
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                }
            }
            break;
            
        case URX_BACKSLASH_B:          // Test for word boundaries
            {
                UBool success = isChunkWordBoundary((int32_t)fp->fInputIdx);
                success ^= (opValue != 0);     // flip sense for \B
                if (!success) {
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                }
            }
            break;
            
            
        case URX_BACKSLASH_BU:          // Test for word boundaries, Unicode-style
            {
                UBool success = isUWordBoundary(fp->fInputIdx);
                success ^= (opValue != 0);     // flip sense for \B
                if (!success) {
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                }
            }
            break;
            
            
        case URX_BACKSLASH_D:            // Test for decimal digit
            {
                if (fp->fInputIdx >= fActiveLimit) {
                    fHitEnd = TRUE;
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                    break;
                }
                
                UChar32 c;
                U16_NEXT(inputBuf, fp->fInputIdx, fActiveLimit, c);
                int8_t ctype = u_charType(c);     // TODO:  make a unicode set for this.  Will be faster.
                UBool success = (ctype == U_DECIMAL_DIGIT_NUMBER);
                success ^= (opValue != 0);        // flip sense for \D
                if (!success) {
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                }
            }
            break;
            
            
        case URX_BACKSLASH_G:          // Test for position at end of previous match
            if (!((fMatch && fp->fInputIdx==fMatchEnd) || (fMatch==FALSE && fp->fInputIdx==fActiveStart))) {
                fp = (REStackFrame *)fStack->popFrame(fFrameSize);
            }
            break;
            
            
        case URX_BACKSLASH_X:     
        //  Match a Grapheme, as defined by Unicode TR 29.
        //  Differs slightly from Perl, which consumes combining marks independently
        //    of context.
        {

            // Fail if at end of input
            if (fp->fInputIdx >= fActiveLimit) {
                fHitEnd = TRUE;
                fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                break;
            }

            // Examine (and consume) the current char.
            //   Dispatch into a little state machine, based on the char.
            UChar32  c;
            U16_NEXT(inputBuf, fp->fInputIdx, fActiveLimit, c);
            UnicodeSet **sets = fPattern->fStaticSets;
            if (sets[URX_GC_NORMAL]->contains(c))  goto GC_Extend;
            if (sets[URX_GC_CONTROL]->contains(c)) goto GC_Control;
            if (sets[URX_GC_L]->contains(c))       goto GC_L;
            if (sets[URX_GC_LV]->contains(c))      goto GC_V;
            if (sets[URX_GC_LVT]->contains(c))     goto GC_T;
            if (sets[URX_GC_V]->contains(c))       goto GC_V;
            if (sets[URX_GC_T]->contains(c))       goto GC_T;
            goto GC_Extend;



GC_L:
            if (fp->fInputIdx >= fActiveLimit)         goto GC_Done;
            U16_NEXT(inputBuf, fp->fInputIdx, fActiveLimit, c);
            if (sets[URX_GC_L]->contains(c))       goto GC_L;
            if (sets[URX_GC_LV]->contains(c))      goto GC_V;
            if (sets[URX_GC_LVT]->contains(c))     goto GC_T;
            if (sets[URX_GC_V]->contains(c))       goto GC_V;
            U16_PREV(inputBuf, 0, fp->fInputIdx, c);
            goto GC_Extend;

GC_V:
            if (fp->fInputIdx >= fActiveLimit)         goto GC_Done;
            U16_NEXT(inputBuf, fp->fInputIdx, fActiveLimit, c);
            if (sets[URX_GC_V]->contains(c))       goto GC_V;
            if (sets[URX_GC_T]->contains(c))       goto GC_T;
            U16_PREV(inputBuf, 0, fp->fInputIdx, c);
            goto GC_Extend;

GC_T:
            if (fp->fInputIdx >= fActiveLimit)         goto GC_Done;
            U16_NEXT(inputBuf, fp->fInputIdx, fActiveLimit, c);
            if (sets[URX_GC_T]->contains(c))       goto GC_T;
            U16_PREV(inputBuf, 0, fp->fInputIdx, c);
            goto GC_Extend;

GC_Extend:
            // Combining characters are consumed here
            for (;;) {
                if (fp->fInputIdx >= fActiveLimit) {
                    break;
                }
                U16_NEXT(inputBuf, fp->fInputIdx, fActiveLimit, c);
                if (sets[URX_GC_EXTEND]->contains(c) == FALSE) {
                    U16_BACK_1(inputBuf, 0, fp->fInputIdx);
                    break;
                }
            }
            goto GC_Done;

GC_Control:
            // Most control chars stand alone (don't combine with combining chars),  
            //   except for that CR/LF sequence is a single grapheme cluster.
            if (c == 0x0d && fp->fInputIdx < fActiveLimit && inputBuf[fp->fInputIdx] == 0x0a) {
                fp->fInputIdx++;
            }

GC_Done:
            if (fp->fInputIdx >= fActiveLimit) {
                fHitEnd = TRUE;
            }
            break;
        }
            
            
            
            
        case URX_BACKSLASH_Z:          // Test for end of Input
            if (fp->fInputIdx < fAnchorLimit) {
                fp = (REStackFrame *)fStack->popFrame(fFrameSize);
            } else {
                fHitEnd = TRUE;
                fRequireEnd = TRUE;
            }
            break;
            
            
            
        case URX_STATIC_SETREF:
            {
                // Test input character against one of the predefined sets
                //    (Word Characters, for example)
                // The high bit of the op value is a flag for the match polarity.
                //    0:   success if input char is in set.
                //    1:   success if input char is not in set.
                if (fp->fInputIdx >= fActiveLimit) {
                    fHitEnd = TRUE;
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                    break;
                }
                
                UBool success = ((opValue & URX_NEG_SET) == URX_NEG_SET);  
                opValue &= ~URX_NEG_SET;
                U_ASSERT(opValue > 0 && opValue < URX_LAST_SET);
                
                UChar32 c;
                U16_NEXT(inputBuf, fp->fInputIdx, fActiveLimit, c);
                if (c < 256) {
                    Regex8BitSet *s8 = &fPattern->fStaticSets8[opValue];
                    if (s8->contains(c)) {
                        success = !success;
                    }
                } else {
                    const UnicodeSet *s = fPattern->fStaticSets[opValue];
                    if (s->contains(c)) {
                        success = !success;
                    }
                }
                if (!success) {
                    #ifdef REGEX_SMART_BACKTRACKING
                    if (fp->fInputIdx > backSearchIndex && fStack->size() > fFrameSize) {
                        REStackFrame *prevFrame = (REStackFrame *)fStack->peekFrame(fFrameSize);
                        if (URX_LOOP_C == URX_TYPE(pat[prevFrame->fPatIdx]) && fp->fInputIdx <= prevFrame->fInputIdx) {
                            // Try to find it, backwards
                            int64_t reverseIndex = fp->fInputIdx;
                            U16_BACK_1(inputBuf, backSearchIndex, reverseIndex); // skip the first character we tried
                            success = ((opValue & URX_NEG_SET) == URX_NEG_SET); // reset
                            do {
                                U16_PREV(inputBuf, backSearchIndex, reverseIndex, c);
                                if (c < 256) {
                                    Regex8BitSet *s8 = &fPattern->fStaticSets8[opValue];
                                    if (s8->contains(c)) {
                                        success = !success;
                                    }
                                } else {
                                    const UnicodeSet *s = fPattern->fStaticSets[opValue];
                                    if (s->contains(c)) {
                                        success = !success;
                                    }
                                }
                            } while (reverseIndex > backSearchIndex && !success);
                            
                            if (success) {
                                fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                                fp->fInputIdx = reverseIndex;
                                if (fp->fInputIdx > backSearchIndex) {
                                    fp = StateSave(fp, fp->fPatIdx, status);
                                }
                                fp->fPatIdx++; // Skip the LOOP_C, we just did that
                                break;
                            }
                        }
                    }
                    #endif
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                }
            }
            break;
            
            
        case URX_STAT_SETREF_N:
            {
                // Test input character for NOT being a member of  one of 
                //    the predefined sets (Word Characters, for example)
                if (fp->fInputIdx >= fActiveLimit) {
                    fHitEnd = TRUE;
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                    break;
                }
                
                U_ASSERT(opValue > 0 && opValue < URX_LAST_SET);
                
                UChar32  c;
                U16_NEXT(inputBuf, fp->fInputIdx, fActiveLimit, c);
                if (c < 256) {
                    Regex8BitSet *s8 = &fPattern->fStaticSets8[opValue];
                    if (s8->contains(c) == FALSE) {
                        break;
                    }
                } else {
                    const UnicodeSet *s = fPattern->fStaticSets[opValue];
                    if (s->contains(c) == FALSE) {
                        break;
                    }
                }

                #ifdef REGEX_SMART_BACKTRACKING
                if (fp->fInputIdx > backSearchIndex && fStack->size() > fFrameSize) {
                    REStackFrame *prevFrame = (REStackFrame *)fStack->peekFrame(fFrameSize);
                    if (URX_LOOP_C == URX_TYPE(pat[prevFrame->fPatIdx]) && fp->fInputIdx <= prevFrame->fInputIdx) {
                        // Try to find it, backwards
                        int64_t reverseIndex = fp->fInputIdx;
                        U16_BACK_1(inputBuf, backSearchIndex, reverseIndex); // skip the first character we tried
                        UBool success = FALSE;
                        do {
                            U16_PREV(inputBuf, backSearchIndex, reverseIndex, c);
                            if (c < 256) {
                                Regex8BitSet *s8 = &fPattern->fStaticSets8[opValue];
                                if (s8->contains(c) == FALSE) {
                                    success = TRUE;
                                    break;
                                }
                            } else {
                                const UnicodeSet *s = fPattern->fStaticSets[opValue];
                                if (s->contains(c) == FALSE) {
                                    success = TRUE;
                                    break;
                                }
                            }
                        } while (reverseIndex > backSearchIndex);
                        
                        if (success) {
                            fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                            fp->fInputIdx = reverseIndex;
                            if (fp->fInputIdx > backSearchIndex) {
                                fp = StateSave(fp, fp->fPatIdx, status);
                            }
                            fp->fPatIdx++; // Skip the LOOP_C, we just did that
                            break;
                        }
                    }
                }
                #endif
                fp = (REStackFrame *)fStack->popFrame(fFrameSize);
            }
            break;
            
            
        case URX_SETREF:
            {
                if (fp->fInputIdx >= fActiveLimit) {
                    fHitEnd = TRUE;
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                    break;
                }
                
                U_ASSERT(opValue > 0 && opValue < sets->size());

                // There is input left.  Pick up one char and test it for set membership.
                UChar32  c;
                U16_NEXT(inputBuf, fp->fInputIdx, fActiveLimit, c);
                if (c<256) {
                    Regex8BitSet *s8 = &fPattern->fSets8[opValue];
                    if (s8->contains(c)) {
                        // The character is in the set.  A Match.
                        break;
                    }
                } else {
                    UnicodeSet *s = (UnicodeSet *)sets->elementAt(opValue);
                    if (s->contains(c)) {
                        // The character is in the set.  A Match.
                        break;
                    }
                }
                
                // the character wasn't in the set.
                #ifdef REGEX_SMART_BACKTRACKING
                if (fp->fInputIdx > backSearchIndex && fStack->size() > fFrameSize) {
                    REStackFrame *prevFrame = (REStackFrame *)fStack->peekFrame(fFrameSize);
                    if (URX_LOOP_C == URX_TYPE(pat[prevFrame->fPatIdx]) && fp->fInputIdx <= prevFrame->fInputIdx) {
                        // Try to find it, backwards
                        int64_t reverseIndex = fp->fInputIdx;
                        U16_BACK_1(inputBuf, backSearchIndex, reverseIndex); // skip the first character we tried
                        UBool success = FALSE;
                        do {
                            U16_PREV(inputBuf, backSearchIndex, reverseIndex, c);
                            if (c < 256) {
                                Regex8BitSet *s8 = &fPattern->fSets8[opValue];
                                if (s8->contains(c)) {
                                    success = TRUE;
                                    break;
                                }
                            } else {
                                UnicodeSet *s = (UnicodeSet *)sets->elementAt(opValue);
                                if (s->contains(c)) {
                                    success = TRUE;
                                    break;
                                }
                            }
                        } while (reverseIndex > backSearchIndex);
                        
                        if (success) {
                            fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                            fp->fInputIdx = reverseIndex;
                            if (fp->fInputIdx > reverseIndex) {
                                fp = StateSave(fp, fp->fPatIdx, status);
                            }
                            fp->fPatIdx++; // Skip the LOOP_C, we just did that
                            break;
                        }
                    }
                }
                #endif
                fp = (REStackFrame *)fStack->popFrame(fFrameSize);
            }
            break;
            
            
        case URX_DOTANY:
            {
                // . matches anything, but stops at end-of-line.
                if (fp->fInputIdx >= fActiveLimit) {
                    // At end of input.  Match failed.  Backtrack out.
                    fHitEnd = TRUE;
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                    break;
                }
                
                // There is input left.  Advance over one char, unless we've hit end-of-line
                UChar32  c;
                U16_NEXT(inputBuf, fp->fInputIdx, fActiveLimit, c);
                if (((c & 0x7f) <= 0x29) &&     // First quickly bypass as many chars as possible
                    ((c<=0x0d && c>=0x0a) || c==0x85 ||c==0x2028 || c==0x2029)) {
                    // End of line in normal mode.   . does not match.
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                    break;
                }
            }
            break;
            
            
        case URX_DOTANY_ALL:
            {
                // . in dot-matches-all (including new lines) mode
                if (fp->fInputIdx >= fActiveLimit) {
                    // At end of input.  Match failed.  Backtrack out.
                    fHitEnd = TRUE;
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                    break;
                }
                
                // There is input left.  Advance over one char, except if we are
                //   at a cr/lf, advance over both of them.
                UChar32 c; 
                U16_NEXT(inputBuf, fp->fInputIdx, fActiveLimit, c);
                if (c==0x0d && fp->fInputIdx < fActiveLimit) {
                    // In the case of a CR/LF, we need to advance over both.
                    if (inputBuf[fp->fInputIdx] == 0x0a) {
                        U16_FWD_1(inputBuf, fp->fInputIdx, fActiveLimit);
                    }
                }
            }
            break;
            
            
        case URX_DOTANY_UNIX:
            {
                // '.' operator, matches all, but stops at end-of-line.
                //   UNIX_LINES mode, so 0x0a is the only recognized line ending.
                if (fp->fInputIdx >= fActiveLimit) {
                    // At end of input.  Match failed.  Backtrack out.
                    fHitEnd = TRUE;
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                    break;
                }
                
                // There is input left.  Advance over one char, unless we've hit end-of-line
                UChar32 c; 
                U16_NEXT(inputBuf, fp->fInputIdx, fActiveLimit, c);
                if (c == 0x0a) {
                    // End of line in normal mode.   '.' does not match the \n
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                }
            }
            break;
            
            
        case URX_JMP:
            fp->fPatIdx = opValue;
            break;
            
        case URX_FAIL:
            isMatch = FALSE;
            goto breakFromLoop;
            
        case URX_JMP_SAV:
            U_ASSERT(opValue < fPattern->fCompiledPat->size());
            fp = StateSave(fp, fp->fPatIdx, status);       // State save to loc following current
            fp->fPatIdx = opValue;                         // Then JMP.
            break;
            
        case URX_JMP_SAV_X:
            // This opcode is used with (x)+, when x can match a zero length string.
            // Same as JMP_SAV, except conditional on the match having made forward progress.
            // Destination of the JMP must be a URX_STO_INP_LOC, from which we get the
            //   data address of the input position at the start of the loop.
            {
                U_ASSERT(opValue > 0 && opValue < fPattern->fCompiledPat->size());
                int32_t  stoOp = (int32_t)pat[opValue-1];
                U_ASSERT(URX_TYPE(stoOp) == URX_STO_INP_LOC);
                int32_t  frameLoc = URX_VAL(stoOp);
                U_ASSERT(frameLoc >= 0 && frameLoc < fFrameSize);
                int32_t prevInputIdx = (int32_t)fp->fExtra[frameLoc];
                U_ASSERT(prevInputIdx <= fp->fInputIdx);
                if (prevInputIdx < fp->fInputIdx) {
                    // The match did make progress.  Repeat the loop.
                    fp = StateSave(fp, fp->fPatIdx, status);  // State save to loc following current
                    fp->fPatIdx = opValue;
                    fp->fExtra[frameLoc] = fp->fInputIdx;
                } 
                // If the input position did not advance, we do nothing here,
                //   execution will fall out of the loop.
            }
            break;
            
        case URX_CTR_INIT:
            {
                U_ASSERT(opValue >= 0 && opValue < fFrameSize-2);
                fp->fExtra[opValue] = 0;       //  Set the loop counter variable to zero
                
                // Pick up the three extra operands that CTR_INIT has, and
                //    skip the pattern location counter past 
                int32_t instrOperandLoc = (int32_t)fp->fPatIdx;
                fp->fPatIdx += 3;
                int32_t loopLoc  = URX_VAL(pat[instrOperandLoc]);
                int32_t minCount = (int32_t)pat[instrOperandLoc+1];
                int32_t maxCount = (int32_t)pat[instrOperandLoc+2];
                U_ASSERT(minCount>=0);
                U_ASSERT(maxCount>=minCount || maxCount==-1);
                U_ASSERT(loopLoc>fp->fPatIdx);
                
                if (minCount == 0) {
                    fp = StateSave(fp, loopLoc+1, status);
                }
                if (maxCount == 0) {
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                }
            }
            break;
            
        case URX_CTR_LOOP:
            {
                U_ASSERT(opValue>0 && opValue < fp->fPatIdx-2);
                int32_t initOp = (int32_t)pat[opValue];
                U_ASSERT(URX_TYPE(initOp) == URX_CTR_INIT);
                int64_t *pCounter = &fp->fExtra[URX_VAL(initOp)];
                int32_t minCount  = (int32_t)pat[opValue+2];
                int32_t maxCount  = (int32_t)pat[opValue+3];
                // Increment the counter.  Note: we DIDN'T worry about counter
                //   overflow, since the data comes from UnicodeStrings, which
                //   stores its length in an int32_t. Do we have to think about
                //   this now that we're using UText? Probably not, since the length
                //   in UChar32s is still an int32_t.
                (*pCounter)++;
                U_ASSERT(*pCounter > 0);
                if ((uint64_t)*pCounter >= (uint32_t)maxCount) {
                    U_ASSERT(*pCounter == maxCount || maxCount == -1);
                    break;
                }
                if (*pCounter >= minCount) {
                    fp = StateSave(fp, fp->fPatIdx, status);
                }
                fp->fPatIdx = opValue + 4;    // Loop back.
            }
            break;
            
        case URX_CTR_INIT_NG:
            {
                // Initialize a non-greedy loop
                U_ASSERT(opValue >= 0 && opValue < fFrameSize-2);
                fp->fExtra[opValue] = 0;       //  Set the loop counter variable to zero
                
                // Pick up the three extra operands that CTR_INIT has, and
                //    skip the pattern location counter past 
                int32_t instrOperandLoc = (int32_t)fp->fPatIdx;
                fp->fPatIdx += 3;
                int32_t loopLoc  = URX_VAL(pat[instrOperandLoc]);
                int32_t minCount = (int32_t)pat[instrOperandLoc+1];
                int32_t maxCount = (int32_t)pat[instrOperandLoc+2];
                U_ASSERT(minCount>=0);
                U_ASSERT(maxCount>=minCount || maxCount==-1);
                U_ASSERT(loopLoc>fp->fPatIdx);
                
                if (minCount == 0) {
                    if (maxCount != 0) {
                        fp = StateSave(fp, fp->fPatIdx, status);
                    }
                    fp->fPatIdx = loopLoc+1;   // Continue with stuff after repeated block
                } 
            }
            break;
            
        case URX_CTR_LOOP_NG:
            {
                // Non-greedy {min, max} loops
                U_ASSERT(opValue>0 && opValue < fp->fPatIdx-2);
                int32_t initOp = (int32_t)pat[opValue];
                U_ASSERT(URX_TYPE(initOp) == URX_CTR_INIT_NG);
                int64_t *pCounter = &fp->fExtra[URX_VAL(initOp)];
                int32_t minCount  = (int32_t)pat[opValue+2];
                int32_t maxCount  = (int32_t)pat[opValue+3];
                // Increment the counter.  Note: we DIDN'T worry about counter
                //   overflow, since the data comes from UnicodeStrings, which
                //   stores its length in an int32_t. Do we have to think about
                //   this now that we're using UText? Probably not, since the length
                //   in UChar32s is still an int32_t.
                (*pCounter)++;
                U_ASSERT(*pCounter > 0);
                
                if ((uint64_t)*pCounter >= (uint32_t)maxCount) {
                    // The loop has matched the maximum permitted number of times.
                    //   Break out of here with no action.  Matching will
                    //   continue with the following pattern.
                    U_ASSERT(*pCounter == maxCount || maxCount == -1);
                    break;
                }
                
                if (*pCounter < minCount) {
                    // We haven't met the minimum number of matches yet.
                    //   Loop back for another one.
                    fp->fPatIdx = opValue + 4;    // Loop back.
                } else {
                    // We do have the minimum number of matches.
                    //   Fall into the following pattern, but first do
                    //   a state save to the top of the loop, so that a failure
                    //   in the following pattern will try another iteration of the loop.
                    fp = StateSave(fp, opValue + 4, status);
                }
            }
            break;
            
        case URX_STO_SP:
            U_ASSERT(opValue >= 0 && opValue < fPattern->fDataSize);
            fData[opValue] = fStack->size();
            break;
            
        case URX_LD_SP:
            {
                U_ASSERT(opValue >= 0 && opValue < fPattern->fDataSize);
                int32_t newStackSize = (int32_t)fData[opValue];
                U_ASSERT(newStackSize <= fStack->size());
                int64_t *newFP = fStack->getBuffer() + newStackSize - fFrameSize;
                if (newFP == (int64_t *)fp) {
                    break;
                }
                int32_t i;
                for (i=0; i<fFrameSize; i++) {
                    newFP[i] = ((int64_t *)fp)[i];
                }
                fp = (REStackFrame *)newFP;
                fStack->setSize(newStackSize);
            }
            break;
            
        case URX_BACKREF:
        case URX_BACKREF_I:
            {
                U_ASSERT(opValue < fFrameSize);
                int64_t groupStartIdx = fp->fExtra[opValue];
                int64_t groupEndIdx   = fp->fExtra[opValue+1];
                U_ASSERT(groupStartIdx <= groupEndIdx);
                int64_t len = groupEndIdx-groupStartIdx;
                if (groupStartIdx < 0) {
                    // This capture group has not participated in the match thus far,
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);   // FAIL, no match.
                }

                if (len == 0) {
                        //   The capture group match was of an empty string.
                        //   Verified by testing:  Perl matches succeed in this case, so
                        //   we do too.
                        break;
                    }

                UBool  haveMatch = FALSE;
                if (fp->fInputIdx + len <= fActiveLimit) {
                    if (opType == URX_BACKREF) {
                        if (u_strncmp(inputBuf+groupStartIdx, inputBuf+fp->fInputIdx, (int32_t)len) == 0) {
                            haveMatch = TRUE;
                        }
                    } else {
                        if (u_strncasecmp(inputBuf+groupStartIdx, inputBuf+fp->fInputIdx,
                                  (int32_t)len, U_FOLD_CASE_DEFAULT) == 0) {
                            haveMatch = TRUE;
                        }
                    }
                } else {
                    // TODO: probably need to do a partial string comparison, and only
                    //       set HitEnd if the available input matched.  Ticket #6074
                    fHitEnd = TRUE;
                }
                if (haveMatch) {
                    fp->fInputIdx += len;     // Match.  Advance current input position.
                } else {
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);   // FAIL, no match.
                }
            }
            break;
            
        case URX_STO_INP_LOC:
            {
                U_ASSERT(opValue >= 0 && opValue < fFrameSize);
                fp->fExtra[opValue] = fp->fInputIdx;
            }
            break;
            
        case URX_JMPX:
            {
                int32_t instrOperandLoc = (int32_t)fp->fPatIdx;
                fp->fPatIdx += 1;
                int32_t dataLoc  = URX_VAL(pat[instrOperandLoc]);
                U_ASSERT(dataLoc >= 0 && dataLoc < fFrameSize);
                int32_t savedInputIdx = (int32_t)fp->fExtra[dataLoc];
                U_ASSERT(savedInputIdx <= fp->fInputIdx);
                if (savedInputIdx < fp->fInputIdx) {
                    fp->fPatIdx = opValue;                               // JMP
                } else {
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);   // FAIL, no progress in loop.
                }
            }
            break;
            
        case URX_LA_START:
            {
                // Entering a lookahead block.
                // Save Stack Ptr, Input Pos.
                U_ASSERT(opValue>=0 && opValue+1<fPattern->fDataSize);
                fData[opValue]   = fStack->size();
                fData[opValue+1] = fp->fInputIdx;
                fActiveStart     = fLookStart;          // Set the match region change for
                fActiveLimit     = fLookLimit;          //   transparent bounds.
            }
            break;
            
        case URX_LA_END:
            {
                // Leaving a look-ahead block.
                //  restore Stack Ptr, Input Pos to positions they had on entry to block.
                U_ASSERT(opValue>=0 && opValue+1<fPattern->fDataSize);
                int32_t stackSize = fStack->size();
                int32_t newStackSize = (int32_t)fData[opValue];
                U_ASSERT(stackSize >= newStackSize);
                if (stackSize > newStackSize) {
                    // Copy the current top frame back to the new (cut back) top frame.
                    //   This makes the capture groups from within the look-ahead
                    //   expression available.
                    int64_t *newFP = fStack->getBuffer() + newStackSize - fFrameSize;
                    int32_t i;
                    for (i=0; i<fFrameSize; i++) {
                        newFP[i] = ((int64_t *)fp)[i];
                    }
                    fp = (REStackFrame *)newFP;
                    fStack->setSize(newStackSize);
                }
                fp->fInputIdx = fData[opValue+1];
                
                // Restore the active region bounds in the input string; they may have
                //    been changed because of transparent bounds on a Region.
                fActiveStart = fRegionStart;
                fActiveLimit = fRegionLimit;
            }
            break;
            
        case URX_ONECHAR_I:
            if (fp->fInputIdx < fActiveLimit) {
                UChar32 c; 
                U16_NEXT(inputBuf, fp->fInputIdx, fActiveLimit, c);
                if (u_foldCase(c, U_FOLD_CASE_DEFAULT) == opValue) {
                    break;
                }
            } else {
                fHitEnd = TRUE;
            }
            
            #ifdef REGEX_SMART_BACKTRACKING
            if (fp->fInputIdx > backSearchIndex && fStack->size() > fFrameSize) {
                REStackFrame *prevFrame = (REStackFrame *)fStack->peekFrame(fFrameSize);
                if (URX_LOOP_C == URX_TYPE(pat[prevFrame->fPatIdx]) && fp->fInputIdx <= prevFrame->fInputIdx) {
                    UBool success = FALSE;
                    int64_t reverseIndex = fp->fInputIdx;
                    UChar32 c;
                    while (reverseIndex > backSearchIndex) {
                        U16_PREV(inputBuf, backSearchIndex, reverseIndex, c);
                        if (u_foldCase(c, U_FOLD_CASE_DEFAULT) == opValue) {
                            success = TRUE;
                            break;
                        } else if (c == U_SENTINEL) {
                            break;
                        }
                    }
                    if (success) {
                        fHitEnd = FALSE;
                        fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                        fp->fInputIdx = reverseIndex;
                        if (fp->fInputIdx > backSearchIndex) {
                            fp = StateSave(fp, fp->fPatIdx, status);
                        }
                        fp->fPatIdx++; // Skip the LOOP_C, we just did that
                        break;
                    }
                }
            }
            #endif

            fp = (REStackFrame *)fStack->popFrame(fFrameSize);
            break;
            
        case URX_STRING_I:
            {
                // Test input against a literal string.
                // Strings require two slots in the compiled pattern, one for the
                //   offset to the string text, and one for the length.
                const UCaseProps *csp = ucase_getSingleton();
                {
                    int32_t stringStartIdx, stringLen;
                    stringStartIdx = opValue;
                    
                    op      = (int32_t)pat[fp->fPatIdx];
                    fp->fPatIdx++;
                    opType  = URX_TYPE(op);
                    opValue = URX_VAL(op);
                    U_ASSERT(opType == URX_STRING_LEN);
                    stringLen = opValue;
                    
                    const UChar *patternChars = litText+stringStartIdx;
                    const UChar *patternEnd = patternChars+stringLen;
                    
                    const UChar *foldChars = NULL;
                    int32_t foldOffset, foldLength;
                    UChar32 c;
                    UBool c_is_valid = FALSE;
                    
                    #ifdef REGEX_SMART_BACKTRACKING
                    int32_t originalInputIdx = fp->fInputIdx;
                    #endif
                    UBool success = TRUE;
                    
                    foldOffset = foldLength = 0;

                    while (patternChars < patternEnd && success) {
                        if (fp->fInputIdx < fActiveLimit) {  // don't read past end of string
                            if(foldOffset < foldLength) {
                                U16_NEXT_UNSAFE(foldChars, foldOffset, c);
                                c_is_valid = TRUE;
                            } else {
                                // test pre-condition of U16_NEXT: i < length
                                U_ASSERT(fp->fInputIdx < fActiveLimit);
                                U16_NEXT(inputBuf, fp->fInputIdx, fActiveLimit, c);
                                c_is_valid = TRUE;
                                foldLength = ucase_toFullFolding(csp, c, &foldChars, U_FOLD_CASE_DEFAULT);
                                if(foldLength >= 0) {
                                    if(foldLength <= UCASE_MAX_STRING_LENGTH) {   // !!!: Does not correctly handle chars that fold to 0-length strings
                                        foldOffset = 0;
                                        U16_NEXT_UNSAFE(foldChars, foldOffset, c);
                                    } else {
                                        c = foldLength;
                                        foldLength = foldOffset; // to avoid reading chars from the folding buffer
                                    }
                                }
                            }
                        } else {
                          c_is_valid = FALSE;
                        }
                        
                        if (fp->fInputIdx <= fActiveLimit && c_is_valid) {
                            if (U_IS_BMP(c)) {
                                success = (*patternChars == c);
                                patternChars += 1;
                            } else if (patternChars+1 < patternEnd) {
                                success = (*patternChars == U16_LEAD(c) && *(patternChars+1) == U16_TRAIL(c));
                                patternChars += 2;
                            }
                        } else {
                            success = FALSE;
                            fHitEnd = TRUE;          //   TODO:  See ticket 6074
                        }
                    }
                    
                    if (!success) {
                        #ifdef REGEX_SMART_BACKTRACKING
                        if (fp->fInputIdx > backSearchIndex && fStack->size()) {
                            REStackFrame *prevFrame = (REStackFrame *)fStack->peekFrame(fFrameSize);
                            if (URX_LOOP_C == URX_TYPE(pat[prevFrame->fPatIdx]) && fp->fInputIdx <= prevFrame->fInputIdx) {
                                // Reset to last start point
                                int64_t reverseIndex = originalInputIdx;
                                patternChars = litText+stringStartIdx;
                                
                                // Search backwards for a possible start
                                do {
                                    U16_PREV(inputBuf, backSearchIndex, reverseIndex, c);
                                    foldLength = ucase_toFullFolding(csp, c, &foldChars, U_FOLD_CASE_DEFAULT);
                                    if(foldLength >= 0) {
                                        if(foldLength <= UCASE_MAX_STRING_LENGTH) {   // !!!: Does not correctly handle chars that fold to 0-length strings
                                            foldOffset = 0;
                                            U16_NEXT_UNSAFE(foldChars, foldOffset, c);
                                        } else {
                                            c = foldLength;
                                            foldLength = foldOffset; // to avoid reading chars from the folding buffer
                                        }
                                    }
                                    
                                    if ((U_IS_BMP(c) && *patternChars == c) ||
                                           (*patternChars == U16_LEAD(c) && *(patternChars+1) == U16_TRAIL(c))) {
                                        success = TRUE;
                                        break;
                                    }
                                } while (reverseIndex > backSearchIndex);
                                
                                // And try again
                                if (success) {
                                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                                    fp->fInputIdx = reverseIndex;
                                    if (fp->fInputIdx > backSearchIndex) {
                                        fp = StateSave(fp, fp->fPatIdx, status);
                                    }
                                    fp->fPatIdx++; // Skip the LOOP_C, we just did that
                                    break;
                                }
                            }
                        }
                        #endif
                        fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                    }
                }
            }
            break;
            
        case URX_LB_START:
            {
                // Entering a look-behind block.
                // Save Stack Ptr, Input Pos.
                //   TODO:  implement transparent bounds.  Ticket #6067
                U_ASSERT(opValue>=0 && opValue+1<fPattern->fDataSize);
                fData[opValue]   = fStack->size();
                fData[opValue+1] = fp->fInputIdx;
                // Init the variable containing the start index for attempted matches.
                fData[opValue+2] = -1;
                // Save input string length, then reset to pin any matches to end at
                //   the current position.
                fData[opValue+3] = fActiveLimit;
                fActiveLimit     = fp->fInputIdx;
            }
            break;
            
            
        case URX_LB_CONT:
            {
                // Positive Look-Behind, at top of loop checking for matches of LB expression
                //    at all possible input starting positions.
                
                // Fetch the min and max possible match lengths.  They are the operands
                //   of this op in the pattern.
                int32_t minML = (int32_t)pat[fp->fPatIdx++];
                int32_t maxML = (int32_t)pat[fp->fPatIdx++];
                U_ASSERT(minML <= maxML);
                U_ASSERT(minML >= 0);
                
                // Fetch (from data) the last input index where a match was attempted.
                U_ASSERT(opValue>=0 && opValue+1<fPattern->fDataSize);
                int64_t  *lbStartIdx = &fData[opValue+2];
                if (*lbStartIdx < 0) {
                    // First time through loop.
                    *lbStartIdx = fp->fInputIdx - minML;
                } else {
                    // 2nd through nth time through the loop.
                    // Back up start position for match by one.
                    if (*lbStartIdx == 0) {
                        (*lbStartIdx)--;
                    } else {
                        U16_BACK_1(inputBuf, 0, *lbStartIdx);
                    }
                }
                
                if (*lbStartIdx < 0 || *lbStartIdx < fp->fInputIdx - maxML) {
                    // We have tried all potential match starting points without
                    //  getting a match.  Backtrack out, and out of the
                    //   Look Behind altogether.
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                    int64_t restoreInputLen = fData[opValue+3];
                    U_ASSERT(restoreInputLen >= fActiveLimit);
                    U_ASSERT(restoreInputLen <= fInputLength);
                    fActiveLimit = restoreInputLen;
                    break;
                }
                
                //    Save state to this URX_LB_CONT op, so failure to match will repeat the loop.
                //      (successful match will fall off the end of the loop.)
                fp = StateSave(fp, fp->fPatIdx-3, status);
                fp->fInputIdx =  *lbStartIdx;
            }
            break;
            
        case URX_LB_END:
            // End of a look-behind block, after a successful match.
            {
                U_ASSERT(opValue>=0 && opValue+1<fPattern->fDataSize);
                if (fp->fInputIdx != fActiveLimit) {
                    //  The look-behind expression matched, but the match did not
                    //    extend all the way to the point that we are looking behind from.
                    //  FAIL out of here, which will take us back to the LB_CONT, which
                    //     will retry the match starting at another position or fail
                    //     the look-behind altogether, whichever is appropriate.
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                    break;
                }
                
                // Look-behind match is good.  Restore the orignal input string length,
                //   which had been truncated to pin the end of the lookbehind match to the 
                //   position being looked-behind.
                int64_t originalInputLen = fData[opValue+3];
                U_ASSERT(originalInputLen >= fActiveLimit);
                U_ASSERT(originalInputLen <= fInputLength);
                fActiveLimit = originalInputLen;
            }
            break;
            
            
        case URX_LBN_CONT:
            {
                // Negative Look-Behind, at top of loop checking for matches of LB expression
                //    at all possible input starting positions.
                
                // Fetch the extra parameters of this op.
                int32_t minML       = (int32_t)pat[fp->fPatIdx++];
                int32_t maxML       = (int32_t)pat[fp->fPatIdx++];
                int32_t continueLoc = (int32_t)pat[fp->fPatIdx++];
                continueLoc = URX_VAL(continueLoc);
                U_ASSERT(minML <= maxML);
                U_ASSERT(minML >= 0);
                U_ASSERT(continueLoc > fp->fPatIdx);
                
                // Fetch (from data) the last input index where a match was attempted.
                U_ASSERT(opValue>=0 && opValue+1<fPattern->fDataSize);
                int64_t  *lbStartIdx = &fData[opValue+2];
                if (*lbStartIdx < 0) {
                    // First time through loop.
                    *lbStartIdx = fp->fInputIdx - minML;
                } else {
                    // 2nd through nth time through the loop.
                    // Back up start position for match by one.
                    if (*lbStartIdx == 0) {
                        (*lbStartIdx)--;   // Because U16_BACK is unsafe starting at 0.
                    } else {
                        U16_BACK_1(inputBuf, 0, *lbStartIdx);
                    }
                }
                
                if (*lbStartIdx < 0 || *lbStartIdx < fp->fInputIdx - maxML) {
                    // We have tried all potential match starting points without
                    //  getting a match, which means that the negative lookbehind as
                    //  a whole has succeeded.  Jump forward to the continue location
                    int64_t restoreInputLen = fData[opValue+3];
                    U_ASSERT(restoreInputLen >= fActiveLimit);
                    U_ASSERT(restoreInputLen <= fInputLength);
                    fActiveLimit = restoreInputLen;
                    fp->fPatIdx = continueLoc;
                    break;
                }
                
                //    Save state to this URX_LB_CONT op, so failure to match will repeat the loop.
                //      (successful match will cause a FAIL out of the loop altogether.)
                fp = StateSave(fp, fp->fPatIdx-4, status);
                fp->fInputIdx =  *lbStartIdx;
            }
            break;
            
        case URX_LBN_END:
            // End of a negative look-behind block, after a successful match.
            {
                U_ASSERT(opValue>=0 && opValue+1<fPattern->fDataSize);
                if (fp->fInputIdx != fActiveLimit) {
                    //  The look-behind expression matched, but the match did not
                    //    extend all the way to the point that we are looking behind from.
                    //  FAIL out of here, which will take us back to the LB_CONT, which
                    //     will retry the match starting at another position or succeed
                    //     the look-behind altogether, whichever is appropriate.
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                    break;
                }
                
                // Look-behind expression matched, which means look-behind test as
                //   a whole Fails
                
                //   Restore the orignal input string length, which had been truncated 
                //   inorder to pin the end of the lookbehind match  
                //   to the position being looked-behind.
                int64_t originalInputLen = fData[opValue+3];
                U_ASSERT(originalInputLen >= fActiveLimit);
                U_ASSERT(originalInputLen <= fInputLength);
                fActiveLimit = originalInputLen;
                
                // Restore original stack position, discarding any state saved
                //   by the successful pattern match.
                U_ASSERT(opValue>=0 && opValue+1<fPattern->fDataSize);
                int32_t newStackSize = (int32_t)fData[opValue];
                U_ASSERT(fStack->size() > newStackSize);
                fStack->setSize(newStackSize);
                
                //  FAIL, which will take control back to someplace 
                //  prior to entering the look-behind test.
                fp = (REStackFrame *)fStack->popFrame(fFrameSize);
            }
            break;
            
            
        case URX_LOOP_SR_I:
            // Loop Initialization for the optimized implementation of
            //     [some character set]*
            //   This op scans through all matching input.
            //   The following LOOP_C op emulates stack unwinding if the following pattern fails.
            {
                U_ASSERT(opValue > 0 && opValue < sets->size());
                Regex8BitSet *s8 = &fPattern->fSets8[opValue];
                UnicodeSet   *s  = (UnicodeSet *)sets->elementAt(opValue);
                
                // Loop through input, until either the input is exhausted or
                //   we reach a character that is not a member of the set.
                int32_t ix = (int32_t)fp->fInputIdx;
                for (;;) {
                    if (ix >= fActiveLimit) {
                        fHitEnd = TRUE;
                        break;
                    }
                    UChar32   c;
                    U16_NEXT(inputBuf, ix, fActiveLimit, c);
                    if (c<256) {
                        if (s8->contains(c) == FALSE) {
                            U16_BACK_1(inputBuf, 0, ix);
                            break;
                        }
                    } else {
                        if (s->contains(c) == FALSE) {
                            U16_BACK_1(inputBuf, 0, ix);
                            break;
                        }
                    }
                }
                
                // If there were no matching characters, skip over the loop altogether.
                //   The loop doesn't run at all, a * op always succeeds.
                if (ix == fp->fInputIdx) {
                    fp->fPatIdx++;   // skip the URX_LOOP_C op.
                    break;
                }
                
                // Peek ahead in the compiled pattern, to the URX_LOOP_C that
                //   must follow.  It's operand is the stack location
                //   that holds the starting input index for the match of this [set]*
                int32_t loopcOp = (int32_t)pat[fp->fPatIdx];
                U_ASSERT(URX_TYPE(loopcOp) == URX_LOOP_C);
                int32_t stackLoc = URX_VAL(loopcOp);
                U_ASSERT(stackLoc >= 0 && stackLoc < fFrameSize);
                fp->fExtra[stackLoc] = fp->fInputIdx;
                #ifdef REGEX_SMART_BACKTRACKING
                backSearchIndex = fp->fInputIdx;
                #endif
                fp->fInputIdx = ix;
                
                // Save State to the URX_LOOP_C op that follows this one,
                //   so that match failures in the following code will return to there.
                //   Then bump the pattern idx so the LOOP_C is skipped on the way out of here.
                fp = StateSave(fp, fp->fPatIdx, status);
                fp->fPatIdx++;
            }
            break;
            
            
        case URX_LOOP_DOT_I:
            // Loop Initialization for the optimized implementation of .*
            //   This op scans through all remaining input.
            //   The following LOOP_C op emulates stack unwinding if the following pattern fails.
            {
                // Loop through input until the input is exhausted (we reach an end-of-line)
                // In DOTALL mode, we can just go straight to the end of the input.
                int32_t ix;
                if ((opValue & 1) == 1) {
                    // Dot-matches-All mode.  Jump straight to the end of the string.
                    ix = (int32_t)fActiveLimit;
                    fHitEnd = TRUE;
                } else {
                    // NOT DOT ALL mode.  Line endings do not match '.'
                    // Scan forward until a line ending or end of input.
                    ix = (int32_t)fp->fInputIdx;
                    for (;;) {
                        if (ix >= fActiveLimit) {
                            fHitEnd = TRUE;
                            break;
                        }
                        UChar32   c;
                        U16_NEXT(inputBuf, ix, fActiveLimit, c);   // c = inputBuf[ix++]
                        if ((c & 0x7f) <= 0x29) {          // Fast filter of non-new-line-s
                            if ((c == 0x0a) ||             //  0x0a is newline in both modes.
                                (((opValue & 2) == 0) &&    // IF not UNIX_LINES mode
                                   ((c<=0x0d && c>=0x0a) || c==0x85 || c==0x2028 || c==0x2029))) {
                                //  char is a line ending.  Put the input pos back to the
                                //    line ending char, and exit the scanning loop.
                                U16_BACK_1(inputBuf, 0, ix);
                                break;
                            }
                        }
                    }
                }
                
                // If there were no matching characters, skip over the loop altogether.
                //   The loop doesn't run at all, a * op always succeeds.
                if (ix == fp->fInputIdx) {
                    fp->fPatIdx++;   // skip the URX_LOOP_C op.
                    break;
                }
                
                // Peek ahead in the compiled pattern, to the URX_LOOP_C that
                //   must follow.  It's operand is the stack location
                //   that holds the starting input index for the match of this .*
                int32_t loopcOp = (int32_t)pat[fp->fPatIdx];
                U_ASSERT(URX_TYPE(loopcOp) == URX_LOOP_C);
                int32_t stackLoc = URX_VAL(loopcOp);
                U_ASSERT(stackLoc >= 0 && stackLoc < fFrameSize);
                fp->fExtra[stackLoc] = fp->fInputIdx;
                #ifdef REGEX_SMART_BACKTRACKING
                backSearchIndex = fp->fInputIdx;
                #endif
                fp->fInputIdx = ix;
                
                // Save State to the URX_LOOP_C op that follows this one,
                //   so that match failures in the following code will return to there.
                //   Then bump the pattern idx so the LOOP_C is skipped on the way out of here.
                fp = StateSave(fp, fp->fPatIdx, status);
                fp->fPatIdx++;
            }
            break;
            
            
        case URX_LOOP_C:
            {
                U_ASSERT(opValue>=0 && opValue<fFrameSize);
                backSearchIndex = (int32_t)fp->fExtra[opValue];
                U_ASSERT(backSearchIndex <= fp->fInputIdx);
                if (backSearchIndex == fp->fInputIdx) {
                    // We've backed up the input idx to the point that the loop started.
                    // The loop is done.  Leave here without saving state.   
                    //  Subsequent failures won't come back here.
                    break;
                }
                // Set up for the next iteration of the loop, with input index
                //   backed up by one from the last time through,
                //   and a state save to this instruction in case the following code fails again.
                //   (We're going backwards because this loop emulates stack unwinding, not
                //    the initial scan forward.)
                U_ASSERT(fp->fInputIdx > 0);
                UChar32 prevC;
                U16_PREV(inputBuf, 0, fp->fInputIdx, prevC); // !!!: should this 0 be one of f*Limit?
                
                if (prevC == 0x0a && 
                    fp->fInputIdx > backSearchIndex &&
                    inputBuf[fp->fInputIdx-1] == 0x0d) {
                    int32_t prevOp = (int32_t)pat[fp->fPatIdx-2];
                    if (URX_TYPE(prevOp) == URX_LOOP_DOT_I) {
                        // .*, stepping back over CRLF pair.
                        U16_BACK_1(inputBuf, 0, fp->fInputIdx);
                    }
                }
                
                
                fp = StateSave(fp, fp->fPatIdx-1, status);
            }
            break;
            
            
            
        default:
            // Trouble.  The compiled pattern contains an entry with an
            //           unrecognized type tag.
            U_ASSERT(FALSE);
        }
        
        if (U_FAILURE(status)) {
            isMatch = FALSE;
            break;
        }
    }
    
breakFromLoop:
    fMatch = isMatch;
    if (isMatch) {
        fLastMatchEnd = fMatchEnd;
        fMatchStart   = startIdx;
        fMatchEnd     = fp->fInputIdx;
        if (fTraceDebug) {
            REGEX_RUN_DEBUG_PRINTF(("Match.  start=%d   end=%d\n\n", fMatchStart, fMatchEnd));
        }
    }
    else
    {
        if (fTraceDebug) {
            REGEX_RUN_DEBUG_PRINTF(("No match\n\n"));
        }
    }
    
    fFrame = fp;                // The active stack frame when the engine stopped.
    //   Contains the capture group results that we need to
    //    access later.

    return;
}


UOBJECT_DEFINE_RTTI_IMPLEMENTATION(RegexMatcher)

U_NAMESPACE_END

#endif  // !UCONFIG_NO_REGULAR_EXPRESSIONS
