/*
*******************************************************************************
*
*   Copyright (C) 1999-2006, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  umsg.cpp
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
* This is a C wrapper to MessageFormat C++ API.
*
*   Change history:
*
*   08/5/2001  Ram         Added C wrappers for C++ API. Changed implementation of old API's
*                          Removed pattern parser.
* 
*/

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/umsg.h"
#include "unicode/ustring.h"
#include "unicode/fmtable.h"
#include "unicode/msgfmt.h"
#include "unicode/unistr.h"
#include "cpputils.h"
#include "uassert.h"
#include "ustr_imp.h"

U_NAMESPACE_USE

U_CAPI int32_t
u_formatMessage(const char  *locale,
                const UChar *pattern,
                int32_t     patternLength,
                UChar       *result,
                int32_t     resultLength,
                UErrorCode  *status,
                ...)
{
    va_list    ap;
    int32_t actLen;        
    //argument checking defered to subsequent method calls
    // start vararg processing
    va_start(ap, status);

    actLen = u_vformatMessage(locale,pattern,patternLength,result,resultLength,ap,status);
    // end vararg processing
    va_end(ap);

    return actLen;
}

U_CAPI int32_t U_EXPORT2
u_vformatMessage(   const char  *locale,
                    const UChar *pattern,
                    int32_t     patternLength,
                    UChar       *result,
                    int32_t     resultLength,
                    va_list     ap,
                    UErrorCode  *status)

{
    //argument checking defered to subsequent method calls
    UMessageFormat *fmt = umsg_open(pattern,patternLength,locale,NULL,status);
    int32_t retVal = umsg_vformat(fmt,result,resultLength,ap,status);
    umsg_close(fmt);
    return retVal;
}

U_CAPI int32_t
u_formatMessageWithError(const char *locale,
                        const UChar *pattern,
                        int32_t     patternLength,
                        UChar       *result,
                        int32_t     resultLength,
                        UParseError *parseError,
                        UErrorCode  *status,
                        ...)
{
    va_list    ap;
    int32_t actLen;
    //argument checking defered to subsequent method calls
    // start vararg processing
    va_start(ap, status);

    actLen = u_vformatMessageWithError(locale,pattern,patternLength,result,resultLength,parseError,ap,status);

    // end vararg processing
    va_end(ap);
    return actLen;
}

U_CAPI int32_t U_EXPORT2
u_vformatMessageWithError(  const char  *locale,
                            const UChar *pattern,
                            int32_t     patternLength,
                            UChar       *result,
                            int32_t     resultLength,
                            UParseError *parseError,
                            va_list     ap,
                            UErrorCode  *status)

{
    //argument checking defered to subsequent method calls
    UMessageFormat *fmt = umsg_open(pattern,patternLength,locale,parseError,status);
    int32_t retVal = umsg_vformat(fmt,result,resultLength,ap,status);
    umsg_close(fmt);
    return retVal;
}


// For parse, do the reverse of format:
//  1. Call through to the C++ APIs
//  2. Just assume the user passed in enough arguments.
//  3. Iterate through each formattable returned, and assign to the arguments
U_CAPI void
u_parseMessage( const char   *locale,
                const UChar  *pattern,
                int32_t      patternLength,
                const UChar  *source,
                int32_t      sourceLength,
                UErrorCode   *status,
                ...)
{
    va_list    ap;
    //argument checking defered to subsequent method calls

    // start vararg processing
    va_start(ap, status);

    u_vparseMessage(locale,pattern,patternLength,source,sourceLength,ap,status);
    // end vararg processing
    va_end(ap);
}

U_CAPI void U_EXPORT2
u_vparseMessage(const char  *locale,
                const UChar *pattern,
                int32_t     patternLength,
                const UChar *source,
                int32_t     sourceLength,
                va_list     ap,
                UErrorCode  *status)
{
    //argument checking defered to subsequent method calls
    UMessageFormat *fmt = umsg_open(pattern,patternLength,locale,NULL,status);
    int32_t count = 0;
    umsg_vparse(fmt,source,sourceLength,&count,ap,status);
    umsg_close(fmt);
}

U_CAPI void
u_parseMessageWithError(const char  *locale,
                        const UChar *pattern,
                        int32_t     patternLength,
                        const UChar *source,
                        int32_t     sourceLength,
                        UParseError *error,
                        UErrorCode  *status,
                        ...)
{
    va_list    ap;

    //argument checking defered to subsequent method calls

    // start vararg processing
    va_start(ap, status);

    u_vparseMessageWithError(locale,pattern,patternLength,source,sourceLength,ap,error,status);
    // end vararg processing
    va_end(ap);
}
U_CAPI void U_EXPORT2
u_vparseMessageWithError(const char  *locale,
                         const UChar *pattern,
                         int32_t     patternLength,
                         const UChar *source,
                         int32_t     sourceLength,
                         va_list     ap,
                         UParseError *error,
                         UErrorCode* status)
{
    //argument checking defered to subsequent method calls
    UMessageFormat *fmt = umsg_open(pattern,patternLength,locale,error,status);
    int32_t count = 0;
    umsg_vparse(fmt,source,sourceLength,&count,ap,status);
    umsg_close(fmt);
}
//////////////////////////////////////////////////////////////////////////////////
//
//  Message format C API
//
/////////////////////////////////////////////////////////////////////////////////


U_CAPI UMessageFormat* U_EXPORT2
umsg_open(  const UChar     *pattern,
            int32_t         patternLength,
            const  char     *locale,
            UParseError     *parseError,
            UErrorCode      *status)
{
    //check arguments
    if(status==NULL || U_FAILURE(*status))
    {
      return 0;
    }
    if(pattern==NULL||patternLength<-1){
        *status=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    UParseError tErr;
   
    if(parseError==NULL)
    {
        parseError = &tErr;
    }
        
    UMessageFormat* retVal = 0;

    int32_t len = (patternLength == -1 ? u_strlen(pattern) : patternLength);
    
    UnicodeString patString((patternLength == -1 ? TRUE:FALSE), pattern,len);

    retVal = (UMessageFormat*) new MessageFormat(patString,Locale(locale),*parseError,*status);
    
    if(retVal == 0) {
        *status = U_MEMORY_ALLOCATION_ERROR;
        return 0;
    }
    return retVal;
}

U_CAPI void U_EXPORT2
umsg_close(UMessageFormat* format)
{
    //check arguments
    if(format==NULL){
        return;
    }
    delete (MessageFormat*) format;
}

U_CAPI UMessageFormat U_EXPORT2
umsg_clone(const UMessageFormat *fmt,
           UErrorCode *status)
{
    //check arguments
    if(status==NULL || U_FAILURE(*status)){
        return NULL;
    }
    if(fmt==NULL){
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    }
    UMessageFormat retVal = (UMessageFormat)((MessageFormat*)fmt)->clone();
    if(retVal == 0) {
        *status = U_MEMORY_ALLOCATION_ERROR;
        return 0;
    }
    return retVal;    
}

U_CAPI void  U_EXPORT2
umsg_setLocale(UMessageFormat *fmt, const char* locale)
{
    //check arguments
    if(fmt==NULL){
        return;
    }
    ((MessageFormat*)fmt)->setLocale(Locale(locale));   
}

U_CAPI const char*  U_EXPORT2
umsg_getLocale(const UMessageFormat *fmt)
{
    //check arguments
    if(fmt==NULL){
        return "";
    }
    return ((const MessageFormat*)fmt)->getLocale().getName();
}

U_CAPI void  U_EXPORT2
umsg_applyPattern(UMessageFormat *fmt,
                           const UChar* pattern,
                           int32_t patternLength,
                           UParseError* parseError,
                           UErrorCode* status)
{
    //check arguments
    UParseError tErr;
    if(status ==NULL||U_FAILURE(*status)){
        return ;
    }
    if(fmt==NULL||pattern==NULL||patternLength<-1){
        *status=U_ILLEGAL_ARGUMENT_ERROR;
        return ;
    }

    if(parseError==NULL){
      parseError = &tErr;
    }
    if(patternLength<-1){
        patternLength=u_strlen(pattern);
    }

    ((MessageFormat*)fmt)->applyPattern(UnicodeString(pattern,patternLength),*parseError,*status);  
}

U_CAPI int32_t  U_EXPORT2
umsg_toPattern(const UMessageFormat *fmt,
               UChar* result, 
               int32_t resultLength,
               UErrorCode* status)
{
    //check arguments
    if(status ==NULL||U_FAILURE(*status)){
        return -1;
    }
    if(fmt==NULL||resultLength<0 || (resultLength>0 && result==0)){
        *status=U_ILLEGAL_ARGUMENT_ERROR;
        return -1;
    }


    UnicodeString res;
    if(!(result==NULL && resultLength==0)) {
        // NULL destination for pure preflighting: empty dummy string
        // otherwise, alias the destination buffer
        res.setTo(result, 0, resultLength);
    }
    ((const MessageFormat*)fmt)->toPattern(res);
    return res.extract(result, resultLength, *status);
}

U_CAPI int32_t
umsg_format(    const UMessageFormat *fmt,
                UChar          *result,
                int32_t        resultLength,
                UErrorCode     *status,
                ...)
{
    va_list    ap;
    int32_t actLen;  
    //argument checking defered to last method call umsg_vformat which
    //saves time when arguments are valid and we dont care when arguments are not
    //since we return an error anyway

    
    // start vararg processing
    va_start(ap, status);

    actLen = umsg_vformat(fmt,result,resultLength,ap,status);

    // end vararg processing
    va_end(ap);

    return actLen;
}

U_NAMESPACE_BEGIN
/**
 * This class isolates our access to private internal methods of
 * MessageFormat.  It is never instantiated; it exists only for C++
 * access management.
 */
class MessageFormatAdapter {
public:
    static const Formattable::Type* getArgTypeList(const MessageFormat& m,
                                                   int32_t& count);
};
const Formattable::Type*
MessageFormatAdapter::getArgTypeList(const MessageFormat& m,
                                     int32_t& count) {
    return m.getArgTypeList(count);
}
U_NAMESPACE_END

U_CAPI int32_t U_EXPORT2
umsg_vformat(   const UMessageFormat *fmt,
                UChar          *result,
                int32_t        resultLength,
                va_list        ap,
                UErrorCode     *status)
{
    //check arguments
    if(status==0 || U_FAILURE(*status))
    {
        return -1;
    }
    if(fmt==NULL||resultLength<0 || (resultLength>0 && result==0)) {
        *status=U_ILLEGAL_ARGUMENT_ERROR;
        return -1;
    }

    int32_t count =0;
    const Formattable::Type* argTypes =
        MessageFormatAdapter::getArgTypeList(*(const MessageFormat*)fmt, count);
    // Allocate at least one element.  Allocating an array of length
    // zero causes problems on some platforms (e.g. Win32).
    Formattable* args = new Formattable[count ? count : 1];

    // iterate through the vararg list, and get the arguments out
    for(int32_t i = 0; i < count; ++i) {
        
        UChar *stringVal;
        double tDouble=0;
        int32_t tInt =0;
        int64_t tInt64 = 0;
        UDate tempDate = 0;
        switch(argTypes[i]) {
        case Formattable::kDate:
            tempDate = va_arg(ap, UDate);
            args[i].setDate(tempDate);
            break;
            
        case Formattable::kDouble:
            tDouble =va_arg(ap, double);
            args[i].setDouble(tDouble);
            break;
            
        case Formattable::kLong:
            tInt = va_arg(ap, int32_t);
            args[i].setLong(tInt);
            break;

        case Formattable::kInt64:
            tInt64 = va_arg(ap, int64_t);
            args[i].setInt64(tInt64);
            break;
            
        case Formattable::kString:
            // For some reason, a temporary is needed
            stringVal = va_arg(ap, UChar*);
            if(stringVal){
                args[i].setString(stringVal);
            }else{
                *status=U_ILLEGAL_ARGUMENT_ERROR;
            }
            break;
            
        case Formattable::kArray:
            // throw away this argument
            // this is highly platform-dependent, and probably won't work
            // so, if you try to skip arguments in the list (and not use them)
            // you'll probably crash
            va_arg(ap, int);
            break;

        case Formattable::kObject:
            // This will never happen because MessageFormat doesn't
            // support kObject.  When MessageFormat is changed to
            // understand MeasureFormats, modify this code to do the
            // right thing. [alan]
            U_ASSERT(FALSE);
            break;
        }
    }
    UnicodeString resultStr;
    FieldPosition fieldPosition(0);
    
    /* format the message */
    ((const MessageFormat*)fmt)->format(args,count,resultStr,fieldPosition,*status);

    delete[] args;

    if(U_FAILURE(*status)){
        return -1;
    }

    return resultStr.extract(result, resultLength, *status);
}

U_CAPI void
umsg_parse( const UMessageFormat *fmt,
            const UChar    *source,
            int32_t        sourceLength,
            int32_t        *count,
            UErrorCode     *status,
            ...)
{
    va_list    ap;
    //argument checking defered to last method call umsg_vparse which
    //saves time when arguments are valid and we dont care when arguments are not
    //since we return an error anyway

    // start vararg processing
    va_start(ap, status);

    umsg_vparse(fmt,source,sourceLength,count,ap,status);

    // end vararg processing
    va_end(ap);
}

U_CAPI void U_EXPORT2
umsg_vparse(const UMessageFormat *fmt,
            const UChar    *source,
            int32_t        sourceLength,
            int32_t        *count,
            va_list        ap,
            UErrorCode     *status)
{
    //check arguments
    if(status==NULL||U_FAILURE(*status))
    {
        return;
    }
    if(fmt==NULL||source==NULL || sourceLength<-1 || count==NULL){
        *status=U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }
    if(sourceLength==-1){
        sourceLength=u_strlen(source);
    }

    UnicodeString srcString(source,sourceLength);
    Formattable *args = ((const MessageFormat*)fmt)->parse(source,*count,*status);
    UDate *aDate;
    double *aDouble;
    UChar *aString;
    int32_t* aInt;
    int64_t* aInt64;
    UnicodeString temp;
    int len =0;
    // assign formattables to varargs
    for(int32_t i = 0; i < *count; i++) {
        switch(args[i].getType()) {

        case Formattable::kDate:
            aDate = va_arg(ap, UDate*);
            if(aDate){
                *aDate = args[i].getDate();
            }else{
                *status=U_ILLEGAL_ARGUMENT_ERROR;
            }
            break;

        case Formattable::kDouble:
            aDouble = va_arg(ap, double*);
            if(aDouble){
                *aDouble = args[i].getDouble();
            }else{
                *status=U_ILLEGAL_ARGUMENT_ERROR;
            }
            break;

        case Formattable::kLong:
            aInt = va_arg(ap, int32_t*);
            if(aInt){
                *aInt = (int32_t) args[i].getLong();
            }else{
                *status=U_ILLEGAL_ARGUMENT_ERROR;
            }
            break;

        case Formattable::kInt64:
            aInt64 = va_arg(ap, int64_t*);
            if(aInt64){
                *aInt64 = args[i].getInt64();
            }else{
                *status=U_ILLEGAL_ARGUMENT_ERROR;
            }
            break;

        case Formattable::kString:
            aString = va_arg(ap, UChar*);
            if(aString){
                args[i].getString(temp);
                len = temp.length();
                temp.extract(0,len,aString);
                aString[len]=0;
            }else{
                *status= U_ILLEGAL_ARGUMENT_ERROR;
            }
            break;

        case Formattable::kObject:
            // This will never happen because MessageFormat doesn't
            // support kObject.  When MessageFormat is changed to
            // understand MeasureFormats, modify this code to do the
            // right thing. [alan]
            U_ASSERT(FALSE);
            break;

        // better not happen!
        case Formattable::kArray:
            U_ASSERT(FALSE);
            break;
        }
    }

    // clean up
    delete [] args;
}

#define SINGLE_QUOTE      ((UChar)0x0027)
#define CURLY_BRACE_LEFT  ((UChar)0x007B)
#define CURLY_BRACE_RIGHT ((UChar)0x007D)

#define STATE_INITIAL 0
#define STATE_SINGLE_QUOTE 1
#define STATE_IN_QUOTE 2
#define STATE_MSG_ELEMENT 3

#define MAppend(c) if (len < destCapacity) dest[len++] = c; else len++

int32_t umsg_autoQuoteApostrophe(const UChar* pattern, 
                 int32_t patternLength,
                 UChar* dest,
                 int32_t destCapacity,
                 UErrorCode* ec)
{
    int32_t state = STATE_INITIAL;
    int32_t braceCount = 0;
    int32_t len = 0;

    if (ec == NULL || U_FAILURE(*ec)) {
        return -1;
    }

    if (pattern == NULL || patternLength < -1 || (dest == NULL && destCapacity > 0)) {
        *ec = U_ILLEGAL_ARGUMENT_ERROR;
        return -1;
    }

    if (patternLength == -1) {
        patternLength = u_strlen(pattern);
    }

    for (int i = 0; i < patternLength; ++i) {
        UChar c = pattern[i];
        switch (state) {
        case STATE_INITIAL:
            switch (c) {
            case SINGLE_QUOTE:
                state = STATE_SINGLE_QUOTE;
                break;
            case CURLY_BRACE_LEFT:
                state = STATE_MSG_ELEMENT;
                ++braceCount;
                break;
            }
            break;

        case STATE_SINGLE_QUOTE:
            switch (c) {
            case SINGLE_QUOTE:
                state = STATE_INITIAL;
                break;
            case CURLY_BRACE_LEFT:
            case CURLY_BRACE_RIGHT:
                state = STATE_IN_QUOTE;
                break;
            default:
                MAppend(SINGLE_QUOTE);
                state = STATE_INITIAL;
                break;
            }
        break;

        case STATE_IN_QUOTE:
            switch (c) {
            case SINGLE_QUOTE:
                state = STATE_INITIAL;
                break;
            }
            break;

        case STATE_MSG_ELEMENT:
            switch (c) {
            case CURLY_BRACE_LEFT:
                ++braceCount;
                break;
            case CURLY_BRACE_RIGHT:
                if (--braceCount == 0) {
                    state = STATE_INITIAL;
                }
                break;
            }
            break;

        default: // Never happens.
            break;
        }

        MAppend(c);
    }

    // End of scan
    if (state == STATE_SINGLE_QUOTE || state == STATE_IN_QUOTE) {
        MAppend(SINGLE_QUOTE);
    }

    return u_terminateUChars(dest, destCapacity, len, ec);
}

#endif /* #if !UCONFIG_NO_FORMATTING */
