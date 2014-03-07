/********************************************************************
 * COPYRIGHT:
 * Copyright (c) 1997-2010, International Business Machines Corporation and
 * others. All Rights Reserved.
 * Copyright (C) 2010 , Yahoo! Inc.
 ********************************************************************
 *
 * File SELFMT.CPP
 *
 * Modification History:
 *
 *   Date        Name        Description
 *   11/11/09    kirtig      Finished first cut of implementation.
 *   11/16/09    kirtig      Improved version
 ********************************************************************/

#include "unicode/utypeinfo.h"  // for 'typeid' to work

#include "unicode/utypes.h"
#include "unicode/ustring.h"
#include "unicode/ucnv_err.h"
#include "unicode/uchar.h"
#include "unicode/umsg.h"
#include "unicode/rbnf.h"
#include "cmemory.h"
#include "util.h"
#include "uassert.h"
#include "ustrfmt.h"
#include "uvector.h"

#include "unicode/selfmt.h"
#include "selfmtimpl.h"

#if !UCONFIG_NO_FORMATTING

U_NAMESPACE_BEGIN

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(SelectFormat)

#define MAX_KEYWORD_SIZE 30
static const UChar SELECT_KEYWORD_OTHER[] = {LOW_O, LOW_T, LOW_H, LOW_E, LOW_R, 0};

SelectFormat::SelectFormat(const UnicodeString& pat, UErrorCode& status) : parsedValuesHash(NULL) {
   if (U_FAILURE(status)) {
      return;
   }
   initHashTable(status);
   applyPattern(pat, status);
}

SelectFormat::SelectFormat(const SelectFormat& other) : Format(other), parsedValuesHash(NULL) {
   UErrorCode status = U_ZERO_ERROR;
   pattern = other.pattern;
   copyHashtable(other.parsedValuesHash, status);
}

SelectFormat::~SelectFormat() {
  cleanHashTable();
}

void SelectFormat::initHashTable(UErrorCode &status) {
  if (U_FAILURE(status)) {
    return;
  }
  // has inited
  if (parsedValuesHash != NULL) {
    return;
  }

  parsedValuesHash = new Hashtable(TRUE, status);
  if (U_FAILURE(status)) {
    cleanHashTable();
    return;
  } else {
    if (parsedValuesHash == NULL) {
      status = U_MEMORY_ALLOCATION_ERROR;
      return;
    }
  }
  // to use hashtable->equals(), must set Value Compartor.
  parsedValuesHash->setValueComparator(uhash_compareCaselessUnicodeString);
}

void SelectFormat::cleanHashTable() {
  if (parsedValuesHash != NULL) {
    delete parsedValuesHash;
    parsedValuesHash = NULL;
  }
}

void
SelectFormat::applyPattern(const UnicodeString& newPattern, UErrorCode& status) {
    if (U_FAILURE(status)) {
      return;
    }

    pattern = newPattern;
    enum State{ startState, keywordState, pastKeywordState, phraseState};

    //Initialization
    UnicodeString keyword ;
    UnicodeString phrase ;
    UnicodeString* ptrPhrase ;
    int32_t braceCount = 0;

    if (parsedValuesHash == NULL) {
      initHashTable(status);
      if (U_FAILURE(status)) {
        return;
      }
    }
    parsedValuesHash->removeAll();
    parsedValuesHash->setValueDeleter(uhash_deleteUnicodeString);

    //Process the state machine
    State state = startState;
    for (int32_t i = 0; i < pattern.length(); ++i) {
        //Get the character and check its type
        UChar ch = pattern.charAt(i);
        CharacterClass type = classifyCharacter(ch);

        //Allow any character in phrase but nowhere else
        if ( type == tOther ) {
            if ( state == phraseState ){
                phrase += ch;
                continue;
            }else {
                status = U_PATTERN_SYNTAX_ERROR;
                cleanHashTable();
                return;
            }
        }

        //Process the state machine
        switch (state) {
            //At the start of pattern
            case startState:
                switch (type) {
                    case tSpace:
                        break;
                    case tStartKeyword:
                        state = keywordState;
                        keyword += ch;
                        break;
                    //If anything else is encountered, it's a syntax error
                    default:
                        status = U_PATTERN_SYNTAX_ERROR;
                        cleanHashTable();
                        return;
                }//end of switch(type)
                break;

            //Handle the keyword state
            case keywordState:
                switch (type) {
                    case tSpace:
                        state = pastKeywordState;
                        break;
                    case tStartKeyword:
                    case tContinueKeyword:
                        keyword += ch;
                        break;
                    case tLeftBrace:
                        state = phraseState;
                        break;
                    //If anything else is encountered, it's a syntax error
                    default:
                        status = U_PATTERN_SYNTAX_ERROR;
                        cleanHashTable();
                        return;
                }//end of switch(type)
                break;

            //Handle the pastkeyword state
            case pastKeywordState:
                switch (type) {
                    case tSpace:
                        break;
                    case tLeftBrace:
                        state = phraseState;
                        break;
                    //If anything else is encountered, it's a syntax error
                    default:
                        status = U_PATTERN_SYNTAX_ERROR;
                        cleanHashTable();
                        return;
                }//end of switch(type)
                break;

            //Handle the phrase state
            case phraseState:
                switch (type) {
                    case tLeftBrace:
                        braceCount++;
                        phrase += ch;
                        break;
                    case tRightBrace:
                        //Matching keyword, phrase pair found
                        if (braceCount == 0){
                            //Check validity of keyword
                            if (parsedValuesHash->get(keyword) != NULL) {
                                status = U_DUPLICATE_KEYWORD;
                                cleanHashTable();
                                return;
                            }
                            if (keyword.length() == 0) {
                                status = U_PATTERN_SYNTAX_ERROR;
                                cleanHashTable();
                                return;
                            }

                            //Store the keyword, phrase pair in hashTable
                            ptrPhrase = new UnicodeString(phrase);
                            parsedValuesHash->put( keyword, ptrPhrase, status);

                            //Reinitialize
                            keyword.remove();
                            phrase.remove();
                            ptrPhrase = NULL;
                            state = startState;
                        }

                        if (braceCount > 0){
                            braceCount-- ;
                            phrase += ch;
                        }
                        break;
                    default:
                        phrase += ch;
                }//end of switch(type)
                break;

            //Handle the  default case of switch(state)
            default:
                status = U_PATTERN_SYNTAX_ERROR;
                cleanHashTable();
                return;

        }//end of switch(state)
    }

    //Check if the state machine is back to startState
    if ( state != startState){
        status = U_PATTERN_SYNTAX_ERROR;
        cleanHashTable();
        return;
    }

    //Check if "other" keyword is present
    if ( !checkSufficientDefinition() ) {
        status = U_DEFAULT_KEYWORD_MISSING;
        cleanHashTable();
    }
    return;
}

UnicodeString&
SelectFormat::format(const Formattable& obj,
                   UnicodeString& appendTo,
                   FieldPosition& pos,
                   UErrorCode& status) const
{
    switch (obj.getType())
    {
    case Formattable::kString:
        return format(obj.getString(), appendTo, pos, status);
    default:
        if( U_SUCCESS(status) ){
            status = U_ILLEGAL_ARGUMENT_ERROR;
        }
        return appendTo;
    }
}

UnicodeString&
SelectFormat::format(const UnicodeString& keyword,
                     UnicodeString& appendTo,
                     FieldPosition& /*pos */,
                     UErrorCode& status) const {

    if (U_FAILURE(status)) return appendTo;

    if (parsedValuesHash == NULL) {
        status = U_INVALID_FORMAT_ERROR;
        return appendTo;
    }

    //Check for the validity of the keyword
    if ( !checkValidKeyword(keyword) ){
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return appendTo;
    }

    UnicodeString *selectedPattern = (UnicodeString *)parsedValuesHash->get(keyword);
    if (selectedPattern == NULL) {
        selectedPattern = (UnicodeString *)parsedValuesHash->get(SELECT_KEYWORD_OTHER);
    }

    return appendTo += *selectedPattern;
}

UnicodeString&
SelectFormat::toPattern(UnicodeString& appendTo) {
    return appendTo += pattern;
}

SelectFormat::CharacterClass
SelectFormat::classifyCharacter(UChar ch) const{
    if ((ch >= CAP_A) && (ch <= CAP_Z)) {
        return tStartKeyword;
    }
    if ((ch >= LOW_A) && (ch <= LOW_Z)) {
        return tStartKeyword;
    }
    if ((ch >= U_ZERO) && (ch <= U_NINE)) {
        return tContinueKeyword;
    }
    if ( uprv_isRuleWhiteSpace(ch) ){
        return tSpace;
    }
    switch (ch) {
        case LEFTBRACE:
            return tLeftBrace;
        case RIGHTBRACE:
            return tRightBrace;
        case HYPHEN:
        case LOWLINE:
            return tContinueKeyword;
        default :
            return tOther;
    }
}

UBool
SelectFormat::checkSufficientDefinition() {
    // Check that at least the default rule is defined.
    return (parsedValuesHash != NULL &&
           parsedValuesHash->get(SELECT_KEYWORD_OTHER) != NULL) ;
}

UBool
SelectFormat::checkValidKeyword(const UnicodeString& argKeyword ) const{
    int32_t len = argKeyword.length();
    if (len < 1){
        return FALSE;
    }
    CharacterClass type = classifyCharacter(argKeyword.charAt(0));
    if( type != tStartKeyword ){
        return FALSE;
    }

    for (int32_t i = 0; i < argKeyword.length(); ++i) {
        type = classifyCharacter(argKeyword.charAt(i));
        if( type != tStartKeyword && type != tContinueKeyword ){
            return FALSE;
        }
    }
    return TRUE;
}

Format* SelectFormat::clone() const
{
    return new SelectFormat(*this);
}

SelectFormat&
SelectFormat::operator=(const SelectFormat& other) {
    if (this != &other) {
        UErrorCode status = U_ZERO_ERROR;
        pattern = other.pattern;
        copyHashtable(other.parsedValuesHash, status);
    }
    return *this;
}

UBool
SelectFormat::operator==(const Format& other) const {
    if( this == &other){
        return TRUE;
    }
    if (typeid(*this) != typeid(other)) {
        return  FALSE;
    }
    SelectFormat* fmt = (SelectFormat*)&other;
    Hashtable* hashOther = fmt->parsedValuesHash;
    if ( parsedValuesHash == NULL && hashOther == NULL)
        return TRUE;
    if ( parsedValuesHash == NULL || hashOther == NULL)
        return FALSE;
    return parsedValuesHash->equals(*hashOther);
}

UBool
SelectFormat::operator!=(const Format& other) const {
    return  !operator==(other);
}

void
SelectFormat::parseObject(const UnicodeString& /*source*/,
                        Formattable& /*result*/,
                        ParsePosition& pos) const
{
    // TODO: not yet supported in icu4j and icu4c
    pos.setErrorIndex(pos.getIndex());
}

void
SelectFormat::copyHashtable(Hashtable *other, UErrorCode& status) {
    if (U_FAILURE(status)) {
      return;
    }
    if (other == NULL) {
      cleanHashTable();
      return;
    }
    if (parsedValuesHash == NULL) {
      initHashTable(status);
      if (U_FAILURE(status)) {
        return;
      }
    }

    parsedValuesHash->removeAll();
    parsedValuesHash->setValueDeleter(uhash_deleteUnicodeString);

    int32_t pos = -1;
    const UHashElement* elem = NULL;

    // walk through the hash table and create a deep clone
    while ((elem = other->nextElement(pos)) != NULL){
        const UHashTok otherKeyTok = elem->key;
        UnicodeString* otherKey = (UnicodeString*)otherKeyTok.pointer;
        const UHashTok otherKeyToVal = elem->value;
        UnicodeString* otherValue = (UnicodeString*)otherKeyToVal.pointer;
        parsedValuesHash->put(*otherKey, new UnicodeString(*otherValue), status);
        if (U_FAILURE(status)){
            cleanHashTable();
            return;
        }
    }
}

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */

//eof
