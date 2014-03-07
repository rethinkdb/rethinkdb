/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2010, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

/**
 * IntlTestCollator is the medium level test class for everything in the directory "collate".
 */

/***********************************************************************
* Modification history
* Date        Name        Description
* 02/14/2001  synwee      Compare with cintltst and commented away tests 
*                         that are not run.
***********************************************************************/

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION && !UCONFIG_NO_FILE_IO

#include "unicode/uchar.h"
#include "unicode/tstdtmod.h"
#include "cstring.h"
#include "ucol_tok.h"
#include "tscoll.h"
#include "dadrcoll.h"

U_CDECL_BEGIN
static void U_CALLCONV deleteSeqElement(void *elem) {
  delete((SeqElement *)elem);
}
U_CDECL_END

DataDrivenCollatorTest::DataDrivenCollatorTest() 
: seq(StringCharacterIterator("")),
status(U_ZERO_ERROR),
sequences(status)
{
  driver = TestDataModule::getTestDataModule("DataDrivenCollationTest", *this, status);
  sequences.setDeleter(deleteSeqElement);
  UCA = (RuleBasedCollator*)Collator::createInstance("root", status);
}

DataDrivenCollatorTest::~DataDrivenCollatorTest() 
{
  delete driver;
  delete UCA;
}

void DataDrivenCollatorTest::runIndexedTest( int32_t index, UBool exec, const char* &name, char* /*par */)
{
  if(driver != NULL) {
    if (exec)
    {
        logln("TestSuite Collator: ");
    }
    const DataMap *info = NULL;
    TestData *testData = driver->createTestData(index, status);
    if(U_SUCCESS(status)) {
      name = testData->getName();
      if(testData->getInfo(info, status)) {
        log(info->getString("Description", status));
      }
      if(exec) {
        log(name);
          logln("---");
          logln("");
          processTest(testData);
      }
      delete testData;
    } else {
      name = "";
    }
  } else {
    dataerrln("collate/DataDrivenTest data not initialized!");
    name = "";
  }


}

UBool
DataDrivenCollatorTest::setTestSequence(const UnicodeString &setSequence, SeqElement &el) {
  seq.setText(setSequence);
  return getNextInSequence(el);
}

// Parses the sequence to be tested
UBool 
DataDrivenCollatorTest::getNextInSequence(SeqElement &el) {
  el.source.truncate(0);
  UBool quoted = FALSE;
  UBool quotedsingle = FALSE;
  UChar32 currChar = 0;

  while(currChar != CharacterIterator::DONE) {
    currChar= seq.next32PostInc();
    if(!quoted) {
      if(u_isWhitespace(currChar)) {
        continue;
      }
      switch(currChar) {
      case CharacterIterator::DONE:
        break;
      case 0x003C /* < */:
        el.relation = Collator::LESS;
        currChar = CharacterIterator::DONE;
        break;
      case 0x003D /* = */:
        el.relation = Collator::EQUAL;
        currChar = CharacterIterator::DONE;
        break;
      case 0x003E /* > */:
        el.relation = Collator::GREATER;
        currChar = CharacterIterator::DONE;
        break;
      case 0x0027 /* ' */: /* very basic quoting */
        quoted = TRUE;
        quotedsingle = FALSE;
        break;
      case 0x005c /* \ */: /* single quote */
        quoted = TRUE;
        quotedsingle = TRUE;
        break;
      default:
        el.source.append(currChar);
      }
    } else {
      if(currChar == CharacterIterator::DONE) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        errln("Quote in sequence not closed!");
        return FALSE;
      } else if(currChar == 0x0027) {
        quoted = FALSE;
      } else {
        el.source.append(currChar);
      }
      if(quotedsingle) {
        quoted = FALSE;
      }
    }
  }
  return seq.hasNext();
}

// Reads the options string and sets appropriate attributes in collator
void 
DataDrivenCollatorTest::processArguments(Collator *col, const UChar *start, int32_t optLen) {
  const UChar *end = start+optLen;
  UColAttribute attrib;
  UColAttributeValue value;

  if(optLen == 0) {
    return;
  }

  start = ucol_tok_getNextArgument(start, end, &attrib, &value, &status);
  while(start != NULL) {
    if(U_SUCCESS(status)) {
      col->setAttribute(attrib, value, status);
    }
    start = ucol_tok_getNextArgument(start, end, &attrib, &value, &status);
  }
}

void
DataDrivenCollatorTest::processTest(TestData *testData) {
  Collator *col = NULL;
  const UChar *arguments = NULL;
  int32_t argLen = 0;
  const DataMap *settings = NULL;
  const DataMap *currentCase = NULL;
  UErrorCode intStatus = U_ZERO_ERROR;
  UnicodeString testSetting;
  while(testData->nextSettings(settings, status)) {
    intStatus = U_ZERO_ERROR;
    // try to get a locale
    testSetting = settings->getString("TestLocale", intStatus);
    if(U_SUCCESS(intStatus)) {
      char localeName[256];
      testSetting.extract(0, testSetting.length(), localeName, "");
      col = Collator::createInstance(localeName, status);
      if(U_SUCCESS(status)) {
        logln("Testing collator for locale "+testSetting);
      } else {
        errln("Unable to instantiate collator for locale "+testSetting);
        return;
      }
    } else {
      // if no locale, try from rules
      intStatus = U_ZERO_ERROR;
      testSetting = settings->getString("Rules", intStatus);
      if(U_SUCCESS(intStatus)) {
        col = new RuleBasedCollator(testSetting, status);
        if(U_SUCCESS(status)) {
          logln("Testing collator for rules "+testSetting);
        } else {
          errln("Unable to instantiate collator for rules "+testSetting+" - "+u_errorName(status));
          return;
        }
      } else {
        errln("No collator definition!");
        return;
      }
    }
    
    int32_t cloneSize = 0;
    uint8_t* cloneBuf = NULL;
    RuleBasedCollator* clone = NULL;
    if(col != NULL){
      RuleBasedCollator* rbc = (RuleBasedCollator*)col;
      cloneSize = rbc->cloneBinary(NULL, 0, intStatus);
      intStatus = U_ZERO_ERROR;
      cloneBuf = (uint8_t*) malloc(cloneSize);
      cloneSize = rbc->cloneBinary(cloneBuf, cloneSize, intStatus);
      clone = new RuleBasedCollator(cloneBuf, cloneSize, UCA, intStatus);
      if(U_FAILURE(intStatus)){
          errln("Could not clone the RuleBasedCollator. Error: %s", u_errorName(intStatus));
          intStatus= U_ZERO_ERROR;
      }
      // get attributes
      testSetting = settings->getString("Arguments", intStatus);
      if(U_SUCCESS(intStatus)) {
        logln("Arguments: "+testSetting);
        argLen = testSetting.length();
        arguments = testSetting.getBuffer();
        processArguments(col, arguments, argLen);
        if(clone != NULL){
            processArguments(clone, arguments, argLen);
        }
        if(U_FAILURE(status)) {
          errln("Couldn't process arguments");
          break;
        }
      } else {
        intStatus = U_ZERO_ERROR;
      }
      // Start the processing
      while(testData->nextCase(currentCase, status)) {
        UnicodeString sequence = currentCase->getString("sequence", status);
        if(U_SUCCESS(status)) {
            processSequence(col, sequence);
            if(clone != NULL){
                processSequence(clone, sequence);
            }
        }
      }
    } else {
      errln("Couldn't instantiate a collator!");
    }
    delete clone;
    free(cloneBuf);
    delete col;
    col = NULL;
  }
}


void 
DataDrivenCollatorTest::processSequence(Collator* col, const UnicodeString &sequence) {
  Collator::EComparisonResult relation = Collator::EQUAL;
  UBool hasNext;
  SeqElement *source = NULL;
  SeqElement *target = NULL;
  int32_t j = 0;

  sequences.removeAllElements();

  target = new SeqElement(); 

  setTestSequence(sequence, *target);
  sequences.addElement(target, status);

  do {
    relation = Collator::EQUAL;
    target = new SeqElement(); 
    hasNext = getNextInSequence(*target);
    for(j = sequences.size(); j > 0; j--) {
      source = (SeqElement *)sequences.elementAt(j-1);
      if(relation == Collator::EQUAL && source->relation != Collator::EQUAL) {
        relation = source->relation;
      }
      doTest(col, source->source, target->source, relation);     
    }
    sequences.addElement(target, status);
    source = target;
  } while(hasNext);
}

#endif /* #if !UCONFIG_NO_COLLATION */
