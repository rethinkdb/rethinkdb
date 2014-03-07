/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 2002-2010, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

/* Created by weiv 05/09/2002 */

#include <stdarg.h>

#include "unicode/tstdtmod.h"
#include "cmemory.h"
#include <stdio.h>

TestLog::~TestLog() {}

IcuTestErrorCode::~IcuTestErrorCode() {
    // Safe because our handleFailure() does not throw exceptions.
    if(isFailure()) { handleFailure(); }
}

UBool IcuTestErrorCode::logIfFailureAndReset(const char *fmt, ...) {
    if(isFailure()) {
        char buffer[4000];
        va_list ap;
        va_start(ap, fmt);
        vsprintf(buffer, fmt, ap);
        va_end(ap);
        UnicodeString msg(testName, -1, US_INV);
        msg.append(UNICODE_STRING_SIMPLE(" failure: ")).append(UnicodeString(errorName(), -1, US_INV));
        msg.append(UNICODE_STRING_SIMPLE(" - ")).append(UnicodeString(buffer, -1, US_INV));
        testClass.errln(msg);
        reset();
        return TRUE;
    } else {
        reset();
        return FALSE;
    }
}

UBool IcuTestErrorCode::logDataIfFailureAndReset(const char *fmt, ...) {
    if(isFailure()) {
        char buffer[4000];
        va_list ap;
        va_start(ap, fmt);
        vsprintf(buffer, fmt, ap);
        va_end(ap);
        UnicodeString msg(testName, -1, US_INV);
        msg.append(UNICODE_STRING_SIMPLE(" failure: ")).append(UnicodeString(errorName(), -1, US_INV));
        msg.append(UNICODE_STRING_SIMPLE(" - ")).append(UnicodeString(buffer, -1, US_INV));
        testClass.dataerrln(msg);
        reset();
        return TRUE;
    } else {
        reset();
        return FALSE;
    }
}

void IcuTestErrorCode::handleFailure() const {
    // testClass.errln("%s failure - %s", testName, errorName());
    UnicodeString msg(testName, -1, US_INV);
    msg.append(UNICODE_STRING_SIMPLE(" failure: ")).append(UnicodeString(errorName(), -1, US_INV));

    if (get() == U_MISSING_RESOURCE_ERROR) {
        testClass.dataerrln(msg);
    } else {
        testClass.errln(msg);
    }
}

TestDataModule *TestDataModule::getTestDataModule(const char* name, TestLog& log, UErrorCode &status)
{
  if(U_FAILURE(status)) {
    return NULL;
  }
  TestDataModule *result = NULL;

  // TODO: probe for resource bundle and then for XML.
  // According to that, construct an appropriate driver object

  result = new RBTestDataModule(name, log, status);
  if(U_SUCCESS(status)) {
    return result;
  } else {
    delete result;
    return NULL;
  }
}

TestDataModule::TestDataModule(const char* name, TestLog& log, UErrorCode& /*status*/)
: testName(name),
fInfo(NULL),
fLog(log)
{
}

TestDataModule::~TestDataModule() {
  if(fInfo != NULL) {
    delete fInfo;
  }
}

const char * TestDataModule::getName() const
{
  return testName;
}



RBTestDataModule::~RBTestDataModule()
{
  ures_close(fTestData);
  ures_close(fModuleBundle);
  ures_close(fInfoRB);
  uprv_free(tdpath);
}

RBTestDataModule::RBTestDataModule(const char* name, TestLog& log, UErrorCode& status) 
: TestDataModule(name, log, status),
  fModuleBundle(NULL),
  fTestData(NULL),
  fInfoRB(NULL),
  tdpath(NULL)
{
  fNumberOfTests = 0;
  fDataTestValid = TRUE;
  fModuleBundle = getTestBundle(name, status);
  if(fDataTestValid) {
    fTestData = ures_getByKey(fModuleBundle, "TestData", NULL, &status);
    fNumberOfTests = ures_getSize(fTestData);
    fInfoRB = ures_getByKey(fModuleBundle, "Info", NULL, &status);
    if(status != U_ZERO_ERROR) {
      log.errln(UNICODE_STRING_SIMPLE("Unable to initalize test data - missing mandatory description resources!"));
      fDataTestValid = FALSE;
    } else {
      fInfo = new RBDataMap(fInfoRB, status);
    }
  }
}

UBool RBTestDataModule::getInfo(const DataMap *& info, UErrorCode &/*status*/) const
{
    info = fInfo;
    if(fInfo) {
        return TRUE;
    } else {
        return FALSE;
    }
}

TestData* RBTestDataModule::createTestData(int32_t index, UErrorCode &status) const 
{
  TestData *result = NULL;
  UErrorCode intStatus = U_ZERO_ERROR;

  if(fDataTestValid == TRUE) {
    // Both of these resources get adopted by a TestData object.
    UResourceBundle *DataFillIn = ures_getByIndex(fTestData, index, NULL, &status); 
    UResourceBundle *headers = ures_getByKey(fInfoRB, "Headers", NULL, &intStatus);
  
    if(U_SUCCESS(status)) {
      result = new RBTestData(DataFillIn, headers, status);

      if(U_SUCCESS(status)) {
        return result;
      } else {
        delete result;
      }
    } else {
      ures_close(DataFillIn);
      ures_close(headers);
    }
  } else {
    status = U_MISSING_RESOURCE_ERROR;
  }
  return NULL;
}

TestData* RBTestDataModule::createTestData(const char* name, UErrorCode &status) const
{
  TestData *result = NULL;
  UErrorCode intStatus = U_ZERO_ERROR;

  if(fDataTestValid == TRUE) {
    // Both of these resources get adopted by a TestData object.
    UResourceBundle *DataFillIn = ures_getByKey(fTestData, name, NULL, &status); 
    UResourceBundle *headers = ures_getByKey(fInfoRB, "Headers", NULL, &intStatus);
   
    if(U_SUCCESS(status)) {
      result = new RBTestData(DataFillIn, headers, status);
      if(U_SUCCESS(status)) {
        return result;
      } else {
        delete result;
      }
    } else {
      ures_close(DataFillIn);
      ures_close(headers);
    }
  } else {
    status = U_MISSING_RESOURCE_ERROR;
  }
  return NULL;
}



//Get test data from ResourceBundles
UResourceBundle* 
RBTestDataModule::getTestBundle(const char* bundleName, UErrorCode &status) 
{
  if(U_SUCCESS(status)) {
    UResourceBundle *testBundle = NULL;
    const char* icu_data = fLog.getTestDataPath(status);
    if (testBundle == NULL) {
        testBundle = ures_openDirect(icu_data, bundleName, &status);
        if (status != U_ZERO_ERROR) {
            fLog.dataerrln(UNICODE_STRING_SIMPLE("Could not load test data from resourcebundle: ") + UnicodeString(bundleName, -1, US_INV));
            fDataTestValid = FALSE;
        }
    }
    return testBundle;
  } else {
    return NULL;
  }
}

