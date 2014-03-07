/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2007, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

#ifndef _DATEFORMATREGRESSIONTEST_
#define _DATEFORMATREGRESSIONTEST_

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/unistr.h"
#include "unicode/smpdtfmt.h" 
#include "caltztst.h"

/** 
 * Performs regression test for DateFormat
 **/
class DateFormatRegressionTest: public CalendarTimeZoneTest {
    // IntlTest override
    void runIndexedTest( int32_t index, UBool exec, const char* &name, char* par );
public:

    void Test4029195(void);
    void Test4052408(void);
    void Test4056591(void);
    void Test4059917(void);
    void aux917( SimpleDateFormat *fmt, UnicodeString& str );
    void Test4060212(void);
    void Test4061287(void);
    void Test4065240(void);
    void Test4071441(void);
    void Test4073003(void);
    void Test4089106(void);
    void Test4100302(void);
    void Test4101483(void);
    void Test4103340(void);
    void Test4103341(void);
    void Test4104136(void);
    void Test4104522(void);
    void Test4106807(void);
    void Test4108407(void); 
    void Test4134203(void);
    void Test4151631(void);
    void Test4151706(void);
    void Test4162071(void);
    void Test4182066(void);
    void Test4210209(void);
    void Test714(void);
    void Test1684(void);
    void Test5554(void);
 };

#endif /* #if !UCONFIG_NO_FORMATTING */
 
#endif // _DATEFORMATREGRESSIONTEST_
//eof
