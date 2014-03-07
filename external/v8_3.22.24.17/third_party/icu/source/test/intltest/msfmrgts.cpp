/***********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2009, International Business Machines Corporation
 * and others. All Rights Reserved.
 ***********************************************************************/
 
#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "msfmrgts.h"

#include "unicode/format.h"
#include "unicode/decimfmt.h"
#include "unicode/locid.h"
#include "unicode/msgfmt.h"
#include "unicode/numfmt.h"
#include "unicode/choicfmt.h"
#include "unicode/gregocal.h"
#include "putilimp.h"

// *****************************************************************************
// class MessageFormatRegressionTest
// *****************************************************************************

#define CASE(id,test) case id: name = #test; if (exec) { logln(#test "---"); logln((UnicodeString)""); test(); } break;

void 
MessageFormatRegressionTest::runIndexedTest( int32_t index, UBool exec, const char* &name, char* /*par*/ )
{
    // if (exec) logln((UnicodeString)"TestSuite MessageFormatRegressionTest");
    switch (index) {
        CASE(0,Test4074764)
        CASE(1,Test4058973)
        CASE(2,Test4031438)
        CASE(3,Test4052223)
        CASE(4,Test4104976)
        CASE(5,Test4106659)
        CASE(6,Test4106660)
        CASE(7,Test4111739)
        CASE(8,Test4114743)
        CASE(9,Test4116444)
        CASE(10,Test4114739)
        CASE(11,Test4113018)
        CASE(12,Test4106661)
        CASE(13,Test4094906)
        CASE(14,Test4118592)
        CASE(15,Test4118594)
        CASE(16,Test4105380)
        CASE(17,Test4120552)
        CASE(18,Test4142938)
        CASE(19,TestChoicePatternQuote)
        CASE(20,Test4112104)
        CASE(21,TestAPI)

        default: name = ""; break;
    }
}

UBool 
MessageFormatRegressionTest::failure(UErrorCode status, const char* msg, UBool possibleDataError)
{
    if(U_FAILURE(status)) {
        if (possibleDataError) {
            dataerrln(UnicodeString("FAIL: ") + msg + " failed, error " + u_errorName(status));
        } else {
            errln(UnicodeString("FAIL: ") + msg + " failed, error " + u_errorName(status));
        }
        return TRUE;
    }

    return FALSE;
}

/* @bug 4074764
 * Null exception when formatting pattern with MessageFormat
 * with no parameters.
 */
void MessageFormatRegressionTest::Test4074764() {
    UnicodeString pattern [] = {
        "Message without param",
        "Message with param:{0}",
        "Longer Message with param {0}"
    };
    //difference between the two param strings are that
    //in the first one, the param position is within the
    //length of the string without param while it is not so
    //in the other case.

    UErrorCode status = U_ZERO_ERROR;
    MessageFormat *messageFormatter = new MessageFormat("", status);

    failure(status, "couldn't create MessageFormat");

    //try {
        //Apply pattern with param and print the result
        messageFormatter->applyPattern(pattern[1], status);
        failure(status, "messageFormat->applyPattern");
        //Object[] params = {new UnicodeString("BUG"), new Date()};
        Formattable params [] = {
            Formattable(UnicodeString("BUG")), 
            Formattable(0, Formattable::kIsDate)
        };
        UnicodeString tempBuffer;
        FieldPosition pos(FieldPosition::DONT_CARE);
        tempBuffer = messageFormatter->format(params, 2, tempBuffer, pos, status);
        if( tempBuffer != "Message with param:BUG" || failure(status, "messageFormat->format"))
            errln("MessageFormat with one param test failed.");
        logln("Formatted with one extra param : " + tempBuffer);

        //Apply pattern without param and print the result
        messageFormatter->applyPattern(pattern[0], status);
        failure(status, "messageFormatter->applyPattern");
        
        // {sfb} how much does this apply in C++?
        // do we want to verify that the Formattable* array is not NULL,
        // or is that the user's responsibility?
        // additionally, what should be the item count?
        // for bug testing purposes, assume that something was set to
        // NULL by mistake, and that the length should be non-zero
        
        //tempBuffer = messageFormatter->format(NULL, 1, tempBuffer, FieldPosition(FieldPosition::DONT_CARE), status);
        tempBuffer.remove();
        tempBuffer = messageFormatter->format(NULL, 0, tempBuffer, pos, status);

        if( tempBuffer != "Message without param" || failure(status, "messageFormat->format"))
            errln("MessageFormat with no param test failed.");
        logln("Formatted with no params : " + tempBuffer);

        tempBuffer.remove();
        tempBuffer = messageFormatter->format(params, 2, tempBuffer, pos, status);
         if (tempBuffer != "Message without param" || failure(status, "messageFormat->format"))
            errln("Formatted with arguments > subsitution failed. result = " + tempBuffer);
         logln("Formatted with extra params : " + tempBuffer);
        //This statement gives an exception while formatting...
        //If we use pattern[1] for the message with param,
        //we get an NullPointerException in MessageFormat.java(617)
        //If we use pattern[2] for the message with param,
        //we get an StringArrayIndexOutOfBoundsException in MessageFormat.java(614)
        //Both are due to maxOffset not being reset to -1
        //in applyPattern() when the pattern does not
        //contain any param.
    /*} catch (Exception foo) {
        errln("Exception when formatting with no params.");
    }*/

    delete messageFormatter;
}

/* @bug 4058973
 * MessageFormat.toPattern has weird rounding behavior.
 */
void MessageFormatRegressionTest::Test4058973() 
{
    UErrorCode status = U_ZERO_ERROR;
    MessageFormat *fmt = new MessageFormat("{0,choice,0#no files|1#one file|1< {0,number,integer} files}", status);
    failure(status, "new MessageFormat");

    UnicodeString pat;
    pat = fmt->toPattern(pat);
    UnicodeString exp("{0,choice,0#no files|1#one file|1< {0,number,integer} files}");
    if (pat != exp) {
        errln("MessageFormat.toPattern failed");
        errln("Exp: " + exp);
        errln("Got: " + pat);
    }

    delete fmt;
}
/* @bug 4031438
 * More robust message formats.
 */
void MessageFormatRegressionTest::Test4031438() 
{
    UErrorCode status = U_ZERO_ERROR;
    
    UnicodeString pattern1("Impossible {1} has occurred -- status code is {0} and message is {2}.");
    UnicodeString pattern2("Double '' Quotes {0} test and quoted '{1}' test plus 'other {2} stuff'.");

    MessageFormat *messageFormatter = new MessageFormat("", status);
    failure(status, "new MessageFormat");
    
    const UBool possibleDataError = TRUE;

    //try {
        logln("Apply with pattern : " + pattern1);
        messageFormatter->applyPattern(pattern1, status);
        failure(status, "messageFormat->applyPattern");
        //Object[] params = {new Integer(7)};
        Formattable params []= {
            Formattable((int32_t)7)
        };
        UnicodeString tempBuffer;
        FieldPosition pos(FieldPosition::DONT_CARE);
        tempBuffer = messageFormatter->format(params, 1, tempBuffer, pos, status);
        if(tempBuffer != "Impossible {1} has occurred -- status code is 7 and message is {2}." || failure(status, "MessageFormat::format"))
            dataerrln("Tests arguments < substitution failed");
        logln("Formatted with 7 : " + tempBuffer);
        ParsePosition pp(0);
        int32_t count = 0;
        Formattable *objs = messageFormatter->parse(tempBuffer, pp, count);
        //if(objs[7/*params.length*/] != NULL)
        //    errln("Parse failed with more than expected arguments");

        NumberFormat *fmt = 0;
        UnicodeString temp, temp1;
        
        for (int i = 0; i < count; i++) {
            
            // convert to string if not already
            Formattable obj = objs[i];
            temp.remove();
            if(obj.getType() == Formattable::kString)
                temp = obj.getString(temp);
            else {
                fmt = NumberFormat::createInstance(status);
                switch (obj.getType()) {
                case Formattable::kLong: fmt->format(obj.getLong(), temp); break;
                case Formattable::kInt64: fmt->format(obj.getInt64(), temp); break;
                case Formattable::kDouble: fmt->format(obj.getDouble(), temp); break;
                default: break;
                }
            }

            // convert to string if not already
            Formattable obj1 = params[i];
            temp1.remove();
            if(obj1.getType() == Formattable::kString)
                temp1 = obj1.getString(temp1);
            else {
                fmt = NumberFormat::createInstance(status);
                switch (obj1.getType()) {
                case Formattable::kLong: fmt->format(obj1.getLong(), temp1); break;
                case Formattable::kInt64: fmt->format(obj1.getInt64(), temp1); break;
                case Formattable::kDouble: fmt->format(obj1.getDouble(), temp1); break;
                default: break;
                }
            }

            //if (objs[i] != NULL && objs[i].getString(temp1) != params[i].getString(temp2)) {
            if (temp != temp1) {
                errln("Parse failed on object " + objs[i].getString(temp1) + " at index : " + i);
            }       
        }

        delete fmt;
        delete [] objs;

        // {sfb} does this apply?  no way to really pass a null Formattable, 
        // only a null array

        /*tempBuffer = messageFormatter->format(null, tempBuffer, FieldPosition(FieldPosition::DONT_CARE), status);
        if (tempBuffer != "Impossible {1} has occurred -- status code is {0} and message is {2}." || failure(status, "messageFormat->format"))
            errln("Tests with no arguments failed");
        logln("Formatted with null : " + tempBuffer);*/
        logln("Apply with pattern : " + pattern2);
        messageFormatter->applyPattern(pattern2, status);
        failure(status, "messageFormatter->applyPattern", possibleDataError);
        tempBuffer.remove();
        tempBuffer = messageFormatter->format(params, 1, tempBuffer, pos, status);
        if (tempBuffer != "Double ' Quotes 7 test and quoted {1} test plus other {2} stuff.")
            dataerrln("quote format test (w/ params) failed. - %s", u_errorName(status));
        logln("Formatted with params : " + tempBuffer);
        
        /*tempBuffer = messageFormatter->format(null);
        if (!tempBuffer.equals("Double ' Quotes {0} test and quoted {1} test plus other {2} stuff."))
            errln("quote format test (w/ null) failed.");
        logln("Formatted with null : " + tempBuffer);
        logln("toPattern : " + messageFormatter.toPattern());*/
    /*} catch (Exception foo) {
        errln("Exception when formatting in bug 4031438. "+foo.getMessage());
    }*/
        delete messageFormatter;
}

void MessageFormatRegressionTest::Test4052223()
{

    ParsePosition pos(0);
    if (pos.getErrorIndex() != -1) {
        errln("ParsePosition.getErrorIndex initialization failed.");
    }

    UErrorCode status = U_ZERO_ERROR;
    MessageFormat *fmt = new MessageFormat("There are {0} apples growing on the {1} tree.", status);
    failure(status, "new MessageFormat");
    UnicodeString str("There is one apple growing on the peach tree.");
    
    int32_t count = 0;
    fmt->parse(str, pos, count);

    logln(UnicodeString("unparsable string , should fail at ") + pos.getErrorIndex());
    if (pos.getErrorIndex() == -1)
        errln("Bug 4052223 failed : parsing string " + str);
    pos.setErrorIndex(4);
    if (pos.getErrorIndex() != 4)
        errln(UnicodeString("setErrorIndex failed, got ") + pos.getErrorIndex() + " instead of 4");
    
    ChoiceFormat *f = new ChoiceFormat(
        "-1#are negative|0#are no or fraction|1#is one|1.0<is 1+|2#are two|2<are more than 2.", status);
    failure(status, "new ChoiceFormat");
    pos.setIndex(0); 
    pos.setErrorIndex(-1);
    Formattable obj;
    f->parse("are negative", obj, pos);
    if (pos.getErrorIndex() != -1 && obj.getDouble() == -1.0)
        errln(UnicodeString("Parse with \"are negative\" failed, at ") + pos.getErrorIndex());
    pos.setIndex(0); 
    pos.setErrorIndex(-1);
    f->parse("are no or fraction ", obj, pos);
    if (pos.getErrorIndex() != -1 && obj.getDouble() == 0.0)
        errln(UnicodeString("Parse with \"are no or fraction\" failed, at ") + pos.getErrorIndex());
    pos.setIndex(0); 
    pos.setErrorIndex(-1);
    f->parse("go postal", obj, pos);
    if (pos.getErrorIndex() == -1 && ! uprv_isNaN(obj.getDouble()))
        errln(UnicodeString("Parse with \"go postal\" failed, at ") + pos.getErrorIndex());
    
    delete fmt;
    delete f;
}
/* @bug 4104976
 * ChoiceFormat.equals(null) throws NullPointerException
 */

// {sfb} not really applicable in C++?? (kind of silly)

void MessageFormatRegressionTest::Test4104976()
{
    double limits [] = {1, 20};
    UnicodeString formats [] = {
        UnicodeString("xyz"), 
        UnicodeString("abc")
    };
    int32_t formats_length = (int32_t)(sizeof(formats)/sizeof(formats[0]));
    UErrorCode status = U_ZERO_ERROR;
    ChoiceFormat *cf = new ChoiceFormat(limits, formats, formats_length);
    failure(status, "new ChoiceFormat");
    //try {
        log("Compares to null is always false, returned : ");
        logln(cf == NULL ? "TRUE" : "FALSE");
    /*} catch (Exception foo) {
        errln("ChoiceFormat.equals(null) throws exception.");
    }*/

    delete cf;
}

/* @bug 4106659
 * ChoiceFormat.ctor(double[], String[]) doesn't check
 * whether lengths of input arrays are equal.
 */

// {sfb} again, not really applicable in C++

void MessageFormatRegressionTest::Test4106659()
{
    /*
    double limits [] = {
        1, 2, 3
    };
    UnicodeString formats [] = {
        "one", "two"
    };
    ChoiceFormat *cf = NULL;
    //try {
    //    cf = new ChoiceFormat(limits, formats, 3);
    //} catch (Exception foo) {
    //    logln("ChoiceFormat constructor should check for the array lengths");
    //    cf = null;
    //}
    //if (cf != null) 
    //    errln(cf->format(5));
    //
    delete cf;
    */
}

/* @bug 4106660
 * ChoiceFormat.ctor(double[], String[]) allows unordered double array.
 * This is not a bug, added javadoc to emphasize the use of limit
 * array must be in ascending order.
 */
void MessageFormatRegressionTest::Test4106660()
{
    double limits [] = {3, 1, 2};
    UnicodeString formats [] = {
        UnicodeString("Three"), 
            UnicodeString("One"), 
            UnicodeString("Two")
    };
    ChoiceFormat *cf = new ChoiceFormat(limits, formats, 3);
    double d = 5.0;
    UnicodeString str;
    FieldPosition pos(FieldPosition::DONT_CARE);
    str = cf->format(d, str, pos);
    if (str != "Two")
        errln( (UnicodeString) "format(" + d + ") = " + str);

    delete cf;
}

/* @bug 4111739
 * MessageFormat is incorrectly serialized/deserialized.
 */

// {sfb} doesn't apply in C++

void MessageFormatRegressionTest::Test4111739()
{
    /*MessageFormat format1 = null;
    MessageFormat format2 = null;
    ObjectOutputStream ostream = null;
    ByteArrayOutputStream baos = null;
    ObjectInputStream istream = null;

    try {
        baos = new ByteArrayOutputStream();
        ostream = new ObjectOutputStream(baos);
    } catch(IOException e) {
        errln("Unexpected exception : " + e.getMessage());
        return;
    }

    try {
        format1 = new MessageFormat("pattern{0}");
        ostream.writeObject(format1);
        ostream.flush();

        byte bytes[] = baos.toByteArray();

        istream = new ObjectInputStream(new ByteArrayInputStream(bytes));
        format2 = (MessageFormat)istream.readObject();
    } catch(Exception e) {
        errln("Unexpected exception : " + e.getMessage());
    }

    if (!format1.equals(format2)) {
        errln("MessageFormats before and after serialization are not" +
            " equal\nformat1 = " + format1 + "(" + format1.toPattern() + ")\nformat2 = " +
            format2 + "(" + format2.toPattern() + ")");
    } else {
        logln("Serialization for MessageFormat is OK.");
    }*/
}
/* @bug 4114743
 * MessageFormat.applyPattern allows illegal patterns.
 */
void MessageFormatRegressionTest::Test4114743()
{
    UnicodeString originalPattern("initial pattern");
    UErrorCode status = U_ZERO_ERROR;
    MessageFormat *mf = new MessageFormat(originalPattern, status);
    failure(status, "new MessageFormat");
    //try {
        UnicodeString illegalPattern("ab { '}' de");
        mf->applyPattern(illegalPattern, status);
        if( ! U_FAILURE(status))
            errln("illegal pattern: \"" + illegalPattern + "\"");
    /*} catch (IllegalArgumentException foo) {
        if (!originalPattern.equals(mf.toPattern()))
            errln("pattern after: \"" + mf.toPattern() + "\"");
    }*/
    delete mf;
}

/* @bug 4116444
 * MessageFormat.parse has different behavior in case of null.
 */
void MessageFormatRegressionTest::Test4116444()
{
    UnicodeString patterns [] = {
        (UnicodeString)"", 
        (UnicodeString)"one", 
        (UnicodeString) "{0,date,short}"
    };
    
    UErrorCode status = U_ZERO_ERROR;    
    MessageFormat *mf = new MessageFormat("", status);
    failure(status, "new MessageFormat");

    for (int i = 0; i < 3; i++) {
        UnicodeString pattern = patterns[i];
        mf->applyPattern(pattern, status);
        failure(status, "mf->applyPattern", TRUE);

        //try {
        int32_t count = 0;    
        ParsePosition pp(0);
        Formattable *array = mf->parse(UnicodeString(""), pp, count);
            logln("pattern: \"" + pattern + "\"");
            log(" parsedObjects: ");
            if (array != NULL) {
                log("{");
                for (int j = 0; j < count; j++) {
                    //if (array[j] != null)
                    UnicodeString dummy;
                    err("\"" + array[j].getString(dummy) + "\"");
                    //else
                     //   log("null");
                    if (j < count- 1) 
                        log(",");
                }
                log("}") ;
                delete[] array;
            } else {
                log("null");
            }
            logln("");
        /*} catch (Exception e) {
            errln("pattern: \"" + pattern + "\"");
            errln("  Exception: " + e.getMessage());
        }*/
    }

    delete mf;
}
/* @bug 4114739 (FIX and add javadoc)
 * MessageFormat.format has undocumented behavior about empty format objects.
 */

// {sfb} doesn't apply in C++?
void MessageFormatRegressionTest::Test4114739()
{

    UErrorCode status = U_ZERO_ERROR;    
    MessageFormat *mf = new MessageFormat("<{0}>", status);
    failure(status, "new MessageFormat");

    Formattable *objs1 = NULL;
    //Formattable objs2 [] = {};
    //Formattable *objs3 [] = {NULL};
    //try {
    UnicodeString pat;
    UnicodeString res;
        logln("pattern: \"" + mf->toPattern(pat) + "\"");
        log("format(null) : ");
        FieldPosition pos(FieldPosition::DONT_CARE);
        logln("\"" + mf->format(objs1, 0, res, pos, status) + "\"");
        failure(status, "mf->format");
        /*log("format({})   : ");
        logln("\"" + mf->format(objs2, 0, res, FieldPosition(FieldPosition::DONT_CARE), status) + "\"");
        failure(status, "mf->format");
        log("format({null}) :");
        logln("\"" + mf->format(objs3, 0, res, FieldPosition(FieldPosition::DONT_CARE), status) + "\"");
        failure(status, "mf->format");*/
    /*} catch (Exception e) {
        errln("Exception thrown for null argument tests.");
    }*/

    delete mf;
}

/* @bug 4113018
 * MessageFormat.applyPattern works wrong with illegal patterns.
 */
void MessageFormatRegressionTest::Test4113018()
{
    UnicodeString originalPattern("initial pattern");
    UErrorCode status = U_ZERO_ERROR;
    MessageFormat *mf = new MessageFormat(originalPattern, status);
    failure(status, "new messageFormat");
    UnicodeString illegalPattern("format: {0, xxxYYY}");
    UnicodeString pat;
    logln("pattern before: \"" + mf->toPattern(pat) + "\"");
    logln("illegal pattern: \"" + illegalPattern + "\"");
    //try {
        mf->applyPattern(illegalPattern, status);
        if( ! U_FAILURE(status))
            errln("Should have thrown IllegalArgumentException for pattern : " + illegalPattern);
    /*} catch (IllegalArgumentException e) {
        if (!originalPattern.equals(mf.toPattern()))
            errln("pattern after: \"" + mf.toPattern() + "\"");
    }*/
    delete mf;
}

/* @bug 4106661
 * ChoiceFormat is silent about the pattern usage in javadoc.
 */
void MessageFormatRegressionTest::Test4106661()
{
    UErrorCode status = U_ZERO_ERROR;
    ChoiceFormat *fmt = new ChoiceFormat(
      "-1#are negative| 0#are no or fraction | 1#is one |1.0<is 1+ |2#are two |2<are more than 2.", status);
    failure(status, "new ChoiceFormat");
    UnicodeString pat;
    logln("Formatter Pattern : " + fmt->toPattern(pat));

    FieldPosition bogus(FieldPosition::DONT_CARE);
    UnicodeString str;

    // Will this work for -inf?
    logln("Format with -INF : " + fmt->format(Formattable(-uprv_getInfinity()), str, bogus, status));
    failure(status, "fmt->format");
    str.remove();
    logln("Format with -1.0 : " + fmt->format(Formattable(-1.0), str, bogus, status));
    failure(status, "fmt->format");
    str.remove();
    logln("Format with -1.0 : " + fmt->format(Formattable(-1.0), str, bogus, status));
    failure(status, "fmt->format");
    str.remove();
    logln("Format with 0 : " + fmt->format(Formattable((int32_t)0), str, bogus, status));
    failure(status, "fmt->format");
    str.remove();
    logln("Format with 0.9 : " + fmt->format(Formattable(0.9), str, bogus, status));
    failure(status, "fmt->format");
    str.remove();
    logln("Format with 1.0 : " + fmt->format(Formattable(1.0), str, bogus, status));
    failure(status, "fmt->format");
    str.remove();
    logln("Format with 1.5 : " + fmt->format(Formattable(1.5), str, bogus, status));
    failure(status, "fmt->format");
    str.remove();
    logln("Format with 2 : " + fmt->format(Formattable((int32_t)2), str, bogus, status));
    failure(status, "fmt->format");
    str.remove();
    logln("Format with 2.1 : " + fmt->format(Formattable(2.1), str, bogus, status));
    failure(status, "fmt->format");
    str.remove();
    logln("Format with NaN : " + fmt->format(Formattable(uprv_getNaN()), str, bogus, status));
    failure(status, "fmt->format");
    str.remove();
    logln("Format with +INF : " + fmt->format(Formattable(uprv_getInfinity()), str, bogus, status));
    failure(status, "fmt->format");

    delete fmt;
}

/* @bug 4094906
 * ChoiceFormat should accept \u221E as eq. to INF.
 */
void MessageFormatRegressionTest::Test4094906()
{
    UErrorCode status = U_ZERO_ERROR;
    UnicodeString pattern("-");
    pattern += (UChar) 0x221E;
    pattern += "<are negative|0<are no or fraction|1#is one|1<is 1+|";
    pattern += (UChar) 0x221E;
    pattern += "<are many.";

    ChoiceFormat *fmt = new ChoiceFormat(pattern, status);
    failure(status, "new ChoiceFormat");
    UnicodeString pat;
    if (fmt->toPattern(pat) != pattern) {
        errln( (UnicodeString) "Formatter Pattern : " + pat);
        errln( (UnicodeString) "Expected Pattern  : " + pattern);
    }
    FieldPosition bogus(FieldPosition::DONT_CARE);
    UnicodeString str;

    // Will this work for -inf?
    logln("Format with -INF : " + fmt->format(Formattable(-uprv_getInfinity()), str, bogus, status));
    failure(status, "fmt->format");
    str.remove();
    logln("Format with -1.0 : " + fmt->format(Formattable(-1.0), str, bogus, status));
    failure(status, "fmt->format");
    str.remove();
    logln("Format with -1.0 : " + fmt->format(Formattable(-1.0), str, bogus, status));
    failure(status, "fmt->format");
    str.remove();
    logln("Format with 0 : " + fmt->format(Formattable((int32_t)0), str, bogus, status));
    failure(status, "fmt->format");
    str.remove();
    logln("Format with 0.9 : " + fmt->format(Formattable(0.9), str, bogus, status));
    failure(status, "fmt->format");
    str.remove();
    logln("Format with 1.0 : " + fmt->format(Formattable(1.0), str, bogus, status));
    failure(status, "fmt->format");
    str.remove();
    logln("Format with 1.5 : " + fmt->format(Formattable(1.5), str, bogus, status));
    failure(status, "fmt->format");
    str.remove();
    logln("Format with 2 : " + fmt->format(Formattable((int32_t)2), str, bogus, status));
    failure(status, "fmt->format");
    str.remove();
    logln("Format with 2.1 : " + fmt->format(Formattable(2.1), str, bogus, status));
    failure(status, "fmt->format");
    str.remove();
    logln("Format with NaN : " + fmt->format(Formattable(uprv_getNaN()), str, bogus, status));
    failure(status, "fmt->format");
    str.remove();
    logln("Format with +INF : " + fmt->format(Formattable(uprv_getInfinity()), str, bogus, status));
    failure(status, "fmt->format");

    delete fmt;
}

/* @bug 4118592
 * MessageFormat.parse fails with ChoiceFormat.
 */
void MessageFormatRegressionTest::Test4118592()
{
    UErrorCode status = U_ZERO_ERROR;
    MessageFormat *mf = new MessageFormat("", status);
    failure(status, "new messageFormat");
    UnicodeString pattern("{0,choice,1#YES|2#NO}");
    UnicodeString prefix("");
    Formattable *objs = 0;

    for (int i = 0; i < 5; i++) {
        UnicodeString formatted;
        formatted = prefix + "YES";
        mf->applyPattern(prefix + pattern, status);
        failure(status, "mf->applyPattern");
        prefix += "x";
        //Object[] objs = mf.parse(formatted, new ParsePosition(0));
        int32_t count = 0;
        ParsePosition pp(0);
        objs = mf->parse(formatted, pp, count);
        UnicodeString pat;
        logln(UnicodeString("") + i + ". pattern :\"" + mf->toPattern(pat) + "\"");
        log(" \"" + formatted + "\" parsed as ");
        if (objs == NULL) 
            logln("  null");
        else {
            UnicodeString temp;
            if(objs[0].getType() == Formattable::kString)
                logln((UnicodeString)"  " + objs[0].getString(temp));
            else
                logln((UnicodeString)"  " + (objs[0].getType() == Formattable::kLong ? objs[0].getLong() : objs[0].getDouble()));
            delete[] objs;

        }
    }

    delete mf;
}
/* @bug 4118594
 * MessageFormat.parse fails for some patterns.
 */
void MessageFormatRegressionTest::Test4118594()
{
    UErrorCode status = U_ZERO_ERROR;
    const UBool possibleDataError = TRUE;
    MessageFormat *mf = new MessageFormat("{0}, {0}, {0}", status);
    failure(status, "new MessageFormat");
    UnicodeString forParsing("x, y, z");
    //Object[] objs = mf.parse(forParsing, new ParsePosition(0));
    int32_t count = 0;
    ParsePosition pp(0);
    Formattable *objs = mf->parse(forParsing, pp, count);
    UnicodeString pat;
    logln("pattern: \"" + mf->toPattern(pat) + "\"");
    logln("text for parsing: \"" + forParsing + "\"");
    UnicodeString str;
    if (objs[0].getString(str) != "z")
        errln("argument0: \"" + objs[0].getString(str) + "\"");
    mf->applyPattern("{0,number,#.##}, {0,number,#.#}", status);
    failure(status, "mf->applyPattern", possibleDataError);
    //Object[] oldobjs = {new Double(3.1415)};
    Formattable oldobjs [] = {Formattable(3.1415)};
    UnicodeString result;
    FieldPosition pos(FieldPosition::DONT_CARE);
    result = mf->format( oldobjs, 1, result, pos, status );
    failure(status, "mf->format", possibleDataError);
    pat.remove();
    logln("pattern: \"" + mf->toPattern(pat) + "\"");
    logln("text for parsing: \"" + result + "\"");
    // result now equals "3.14, 3.1"
    if (result != "3.14, 3.1")
        dataerrln("result = " + result + " - " + u_errorName(status));
    //Object[] newobjs = mf.parse(result, new ParsePosition(0));
    int32_t count1 = 0;
    pp.setIndex(0);
    Formattable *newobjs = mf->parse(result, pp, count1);
    // newobjs now equals {new Double(3.1)}
    if (newobjs == NULL) {
        dataerrln("Error calling MessageFormat::parse");
    } else {
        if (newobjs[0].getDouble() != 3.1)
            errln( UnicodeString("newobjs[0] = ") + newobjs[0].getDouble());
    }

    delete [] objs;
    delete [] newobjs;
    delete mf;
}
/* @bug 4105380
 * When using ChoiceFormat, MessageFormat is not good for I18n.
 */
void MessageFormatRegressionTest::Test4105380()
{
    UnicodeString patternText1("The disk \"{1}\" contains {0}.");
    UnicodeString patternText2("There are {0} on the disk \"{1}\"");
    UErrorCode status = U_ZERO_ERROR;
    const UBool possibleDataError = TRUE;
    MessageFormat *form1 = new MessageFormat(patternText1, status);
    failure(status, "new MessageFormat");
    MessageFormat *form2 = new MessageFormat(patternText2, status);
    failure(status, "new MessageFormat");
    double filelimits [] = {0,1,2};
    UnicodeString filepart [] = {
        (UnicodeString)"no files",
            (UnicodeString)"one file",
            (UnicodeString)"{0,number} files"
    };
    ChoiceFormat *fileform = new ChoiceFormat(filelimits, filepart, 3);
    form1->setFormat(1, *fileform);
    form2->setFormat(0, *fileform);
    //Object[] testArgs = {new Long(12373), "MyDisk"};
    Formattable testArgs [] = {
        Formattable((int32_t)12373), 
            Formattable((UnicodeString)"MyDisk")
    };
    
    FieldPosition bogus(FieldPosition::DONT_CARE);

    UnicodeString result;
    logln(form1->format(testArgs, 2, result, bogus, status));
    failure(status, "form1->format", possibleDataError);
    result.remove();
    logln(form2->format(testArgs, 2, result, bogus, status));
    failure(status, "form1->format", possibleDataError);

    delete form1;
    delete form2;
    delete fileform;
}
/* @bug 4120552
 * MessageFormat.parse incorrectly sets errorIndex.
 */
void MessageFormatRegressionTest::Test4120552()
{
    UErrorCode status = U_ZERO_ERROR;
    MessageFormat *mf = new MessageFormat("pattern", status);
    failure(status, "new MessageFormat");
    UnicodeString texts[] = {
        (UnicodeString)"pattern", 
            (UnicodeString)"pat", 
            (UnicodeString)"1234"
    };
    UnicodeString pat;
    logln("pattern: \"" + mf->toPattern(pat) + "\"");
    for (int i = 0; i < 3; i++) {
        ParsePosition pp(0);
        //Object[] objs = mf.parse(texts[i], pp);
        int32_t count = 0;
        Formattable *objs = mf->parse(texts[i], pp, count);
        log("  text for parsing: \"" + texts[i] + "\"");
        if (objs == NULL) {
            logln("  (incorrectly formatted string)");
            if (pp.getErrorIndex() == -1)
                errln(UnicodeString("Incorrect error index: ") + pp.getErrorIndex());
        } else {
            logln("  (correctly formatted string)");
            delete[] objs;
        }
    }
    delete mf;
}

/**
 * @bug 4142938
 * MessageFormat handles single quotes in pattern wrong.
 * This is actually a problem in ChoiceFormat; it doesn't
 * understand single quotes.
 */
void MessageFormatRegressionTest::Test4142938() 
{
    UnicodeString pat = CharsToUnicodeString("''Vous'' {0,choice,0#n''|1#}avez s\\u00E9lectionn\\u00E9 "
        "{0,choice,0#aucun|1#{0}} client{0,choice,0#s|1#|2#s} "
        "personnel{0,choice,0#s|1#|2#s}.");
    UErrorCode status = U_ZERO_ERROR;
    MessageFormat *mf = new MessageFormat(pat, status);
    failure(status, "new MessageFormat");

    UnicodeString PREFIX [] = {
        CharsToUnicodeString("'Vous' n'avez s\\u00E9lectionn\\u00E9 aucun clients personnels."),
        CharsToUnicodeString("'Vous' avez s\\u00E9lectionn\\u00E9 "),
        CharsToUnicodeString("'Vous' avez s\\u00E9lectionn\\u00E9 ")
    };  
    UnicodeString SUFFIX [] = {
        UnicodeString(),
        UNICODE_STRING(" client personnel.", 18),
        UNICODE_STRING(" clients personnels.", 20)
    };

    for (int i=0; i<3; i++) {
        UnicodeString out;
        //out = mf->format(new Object[]{new Integer(i)});
        Formattable objs [] = {
            Formattable((int32_t)i)
        };
        FieldPosition pos(FieldPosition::DONT_CARE);
        out = mf->format(objs, 1, out, pos, status);
        if (!failure(status, "mf->format", TRUE)) {
            if (SUFFIX[i] == "") {
                if (out != PREFIX[i])
                    errln((UnicodeString)"" + i + ": Got \"" + out + "\"; Want \"" + PREFIX[i] + "\"");
            }
            else {
                if (!out.startsWith(PREFIX[i]) ||
                    !out.endsWith(SUFFIX[i]))
                    errln((UnicodeString)"" + i + ": Got \"" + out + "\"; Want \"" + PREFIX[i] + "\"...\"" +
                          SUFFIX[i] + "\"");
            }
        }
    }

    delete mf;
}

/**
 * @bug 4142938
 * Test the applyPattern and toPattern handling of single quotes
 * by ChoiceFormat.  (This is in here because this was a bug reported
 * against MessageFormat.)  The single quote is used to quote the
 * pattern characters '|', '#', '<', and '\u2264'.  Two quotes in a row
 * is a quote literal.
 */
void MessageFormatRegressionTest::TestChoicePatternQuote() 
{
    UnicodeString DATA [] = {
        // Pattern                  0 value           1 value
        // {sfb} hacked - changed \u2264 to = (copied from Character Map)
        (UnicodeString)"0#can''t|1#can",           (UnicodeString)"can't",          (UnicodeString)"can",
        (UnicodeString)"0#'pound(#)=''#'''|1#xyz", (UnicodeString)"pound(#)='#'",   (UnicodeString)"xyz",
        (UnicodeString)"0#'1<2 | 1=1'|1#''",  (UnicodeString)"1<2 | 1=1", (UnicodeString)"'",
    };
    for (int i=0; i<9; i+=3) {
        //try {
            UErrorCode status = U_ZERO_ERROR;
            ChoiceFormat *cf = new ChoiceFormat(DATA[i], status);
            failure(status, "new ChoiceFormat");
            for (int j=0; j<=1; ++j) {
                UnicodeString out;
                FieldPosition pos(FieldPosition::DONT_CARE);
                out = cf->format((double)j, out, pos);
                if (out != DATA[i+1+j])
                    errln("Fail: Pattern \"" + DATA[i] + "\" x "+j+" -> " +
                          out + "; want \"" + DATA[i+1+j] + '"');
            }
            UnicodeString pat;
            pat = cf->toPattern(pat);
            UnicodeString pat2;
            ChoiceFormat *cf2 = new ChoiceFormat(pat, status);
            pat2 = cf2->toPattern(pat2);
            if (pat != pat2)
                errln("Fail: Pattern \"" + DATA[i] + "\" x toPattern -> \"" + pat + '"');
            else
                logln("Ok: Pattern \"" + DATA[i] + "\" x toPattern -> \"" + pat + '"');
        /*}
        catch (IllegalArgumentException e) {
            errln("Fail: Pattern \"" + DATA[i] + "\" -> " + e);
        }*/
    
        delete cf;
        delete cf2;
    }
}

/**
 * @bug 4112104
 * MessageFormat.equals(null) throws a NullPointerException.  The JLS states
 * that it should return false.
 */
void MessageFormatRegressionTest::Test4112104() 
{
    UErrorCode status = U_ZERO_ERROR;
    MessageFormat *format = new MessageFormat("", status);
    failure(status, "new MessageFormat");
    //try {
        // This should NOT throw an exception
        if (format == NULL) {
            // It also should return false
            errln("MessageFormat.equals(null) returns false");
        }
    /*}
    catch (NullPointerException e) {
        errln("MessageFormat.equals(null) throws " + e);
    }*/
    delete format;
}

void MessageFormatRegressionTest::TestAPI() {
    UErrorCode status = U_ZERO_ERROR;
    MessageFormat *format = new MessageFormat("", status);
    failure(status, "new MessageFormat");
    
    // Test adoptFormat
    MessageFormat *fmt = new MessageFormat("",status);
    format->adoptFormat("",fmt,status);
    failure(status, "adoptFormat");

    // Test getFormat
    format->setFormat((int32_t)0,*fmt);
    format->getFormat("",status);
    failure(status, "getFormat");
    delete format;
}

#endif /* #if !UCONFIG_NO_FORMATTING */
