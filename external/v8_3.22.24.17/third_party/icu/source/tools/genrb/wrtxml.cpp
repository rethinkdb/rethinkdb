/*
*******************************************************************************
*
*   Copyright (C) 2002-2009, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*
* File wrtxml.cpp
*
* Modification History:
*
*   Date        Name        Description
*   10/01/02    Ram         Creation.
*   02/07/08    Spieth      Correct XLIFF generation on EBCDIC platform
*
*******************************************************************************
*/
#include "reslist.h"
#include "unewdata.h"
#include "unicode/ures.h"
#include "errmsg.h"
#include "filestrm.h"
#include "cstring.h"
#include "unicode/ucnv.h"
#include "genrb.h"
#include "rle.h"
#include "ucol_tok.h"
#include "uhash.h"
#include "uresimp.h"
#include "unicode/ustring.h"
#include "unicode/uchar.h"
#include "ustr.h"
#include "prscmnts.h"
#include "unicode/unistr.h"
#include <time.h>

U_NAMESPACE_USE

static int tabCount = 0;

static FileStream* out=NULL;
static struct SRBRoot* srBundle ;
static const char* outDir = NULL;
static const char* enc ="";
static UConverter* conv = NULL;

const char* const* ISOLanguages;
const char* const* ISOCountries;
const char* textExt = ".txt";
const char* xliffExt = ".xlf";

static int32_t write_utf8_file(FileStream* fileStream, UnicodeString outString)
{
    UErrorCode status = U_ZERO_ERROR;
    int32_t len = 0;

    // preflight to get the destination buffer size
    u_strToUTF8(NULL,
                0,
                &len,
                outString.getBuffer(),
                outString.length(),
                &status);

    // allocate the buffer
    char* dest = (char*)uprv_malloc(len);
    status = U_ZERO_ERROR;

    // convert the data
    u_strToUTF8(dest,
                len,
                &len,
                outString.getBuffer(),
                outString.length(),
                &status);

    // write data to out file
    int32_t ret = T_FileStream_write(fileStream, dest, len);
    uprv_free(dest);
    return (ret);
}

/*write indentation for formatting*/
static void write_tabs(FileStream* os){
    int i=0;
    for(;i<=tabCount;i++){
        write_utf8_file(os,UnicodeString("    "));
    }
}

/*get ID for each element. ID is globally unique.*/
static char* getID(const char* id, const char* curKey, char* result) {
    if(curKey == NULL) {
        result = (char *)uprv_malloc(sizeof(char)*uprv_strlen(id) + 1);
        uprv_memset(result, 0, sizeof(char)*uprv_strlen(id) + 1);
        uprv_strcpy(result, id);
    } else {
        result = (char *)uprv_malloc(sizeof(char)*(uprv_strlen(id) + 1 + uprv_strlen(curKey)) + 1);
        uprv_memset(result, 0, sizeof(char)*(uprv_strlen(id) + 1 + uprv_strlen(curKey)) + 1);
        if(id[0]!='\0'){
            uprv_strcpy(result, id);
            uprv_strcat(result, "_");
        }
        uprv_strcat(result, curKey);
    }
    return result;
}

/*compute CRC for binary code*/
/* The code is from  http://www.theorem.com/java/CRC32.java
 * Calculates the CRC32 - 32 bit Cyclical Redundancy Check
 * <P> This check is used in numerous systems to verify the integrity
 * of information.  It's also used as a hashing function.  Unlike a regular
 * checksum, it's sensitive to the order of the characters.
 * It produces a 32 bit
 *
 * @author Michael Lecuyer (mjl@theorem.com)
 * @version 1.1 August 11, 1998
 */

/* ICU is not endian portable, because ICU data generated on big endian machines can be
 * ported to big endian machines but not to little endian machines and vice versa. The
 * conversion is not portable across platforms with different endianess.
 */

uint32_t computeCRC(char *ptr, uint32_t len, uint32_t lastcrc){
    int32_t crc;
    uint32_t temp1;
    uint32_t temp2;

    int32_t crc_ta[256];
    int i = 0;
    int j = 0;
    uint32_t crc2 = 0;

#define CRC32_POLYNOMIAL 0xEDB88320

    /*build crc table*/
    for (i = 0; i <= 255; i++) {
        crc2 = i;
        for (j = 8; j > 0; j--) {
            if ((crc2 & 1) == 1) {
                crc2 = (crc2 >> 1) ^ CRC32_POLYNOMIAL;
            } else {
                crc2 >>= 1;
            }
        }
        crc_ta[i] = crc2;
    }

    crc = lastcrc;
    while(len--!=0) {
        temp1 = (uint32_t)crc>>8;
        temp2 = crc_ta[(crc^*ptr) & 0xFF];
        crc = temp1^temp2;
        ptr++;
    }
    return(crc);
}

static void strnrepchr(char* src, int32_t srcLen, char s, char r){
    int32_t i = 0;
    for(i=0;i<srcLen;i++){
        if(src[i]==s){
            src[i]=r;
        }
    }
}
/* Parse the filename, and get its language information.
 * If it fails to get the language information from the filename,
 * use "en" as the default value for language
 */
static char* parseFilename(const char* id, char* /*lang*/) {
    int idLen = (int) uprv_strlen(id);
    char* localeID = (char*) uprv_malloc(idLen);
    int pos = 0;
    int canonCapacity = 0;
    char* canon = NULL;
    int canonLen = 0;
    /*int i;*/
    UErrorCode status = U_ZERO_ERROR;
    const char *ext = uprv_strchr(id, '.');

    if(ext != NULL){
        pos = (int) (ext - id);
    } else {
        pos = idLen;
    }
    uprv_memcpy(localeID, id, pos);
    localeID[pos]=0; /* NUL terminate the string */

    canonCapacity =pos*3;
    canon = (char*) uprv_malloc(canonCapacity);
    canonLen = uloc_canonicalize(localeID, canon, canonCapacity, &status);

    if(U_FAILURE(status)){
        fprintf(stderr, "Could not canonicalize the locale ID: %s. Error: %s\n", localeID, u_errorName(status));
        exit(status);
    }
    strnrepchr(canon, canonLen, '_', '-');
    return canon;
}

static const char* xmlHeader = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
#if 0
static const char* bundleStart = "<xliff version = \"1.2\" "
                                        "xmlns='urn:oasis:names:tc:xliff:document:1.2' "
                                        "xmlns:xsi='http://www.w3.org/2001/XMLSchema-instance' "
                                        "xsi:schemaLocation='urn:oasis:names:tc:xliff:document:1.2 xliff-core-1.2-transitional.xsd'>\n";
#else
static const char* bundleStart = "<xliff version = \"1.1\" "
                                        "xmlns='urn:oasis:names:tc:xliff:document:1.1' "
                                        "xmlns:xsi='http://www.w3.org/2001/XMLSchema-instance' "
                                        "xsi:schemaLocation='urn:oasis:names:tc:xliff:document:1.1 http://www.oasis-open.org/committees/xliff/documents/xliff-core-1.1.xsd'>\n";
#endif
static const char* bundleEnd   = "</xliff>\n";

void res_write_xml(struct SResource *res, const char* id, const char* language, UBool isTopLevel, UErrorCode *status);

static char* convertAndEscape(char** pDest, int32_t destCap, int32_t* destLength,
                              const UChar* src, int32_t srcLen, UErrorCode* status){
    int32_t srcIndex=0;
    char* dest=NULL;
    char* temp=NULL;
    int32_t destLen=0;
    UChar32 c = 0;

    if(status==NULL || U_FAILURE(*status) || pDest==NULL  || srcLen==0 || src == NULL){
        return NULL;
    }
    dest =*pDest;
    if(dest==NULL || destCap <=0){
        destCap = srcLen * 8;
        dest = (char*) uprv_malloc(sizeof(char) * destCap);
        if(dest==NULL){
            *status=U_MEMORY_ALLOCATION_ERROR;
            return NULL;
        }
    }

    dest[0]=0;

    while(srcIndex<srcLen){
        U16_NEXT(src, srcIndex, srcLen, c);

        if (U16_IS_LEAD(c) || U16_IS_TRAIL(c)) {
            *status = U_ILLEGAL_CHAR_FOUND;
            fprintf(stderr, "Illegal Surrogate! \n");
            uprv_free(dest);
            return NULL;
        }

        if((destLen+UTF8_CHAR_LENGTH(c)) < destCap){

            /* ASCII Range */
            if(c <=0x007F){
                switch(c) {
                case '\x26':
                    uprv_strcpy(dest+( destLen),"\x26\x61\x6d\x70\x3b"); /* &amp;*/
                    destLen+=(int32_t)uprv_strlen("\x26\x61\x6d\x70\x3b");
                    break;
                case '\x3c':
                    uprv_strcpy(dest+(destLen),"\x26\x6c\x74\x3b"); /* &lt;*/
                    destLen+=(int32_t)uprv_strlen("\x26\x6c\x74\x3b");
                    break;
                case '\x3e':
                    uprv_strcpy(dest+(destLen),"\x26\x67\x74\x3b"); /* &gt;*/
                    destLen+=(int32_t)uprv_strlen("\x26\x67\x74\x3b");
                    break;
                case '\x22':
                    uprv_strcpy(dest+(destLen),"\x26\x71\x75\x6f\x74\x3b"); /* &quot;*/
                    destLen+=(int32_t)uprv_strlen("\x26\x71\x75\x6f\x74\x3b");
                    break;
                case '\x27':
                    uprv_strcpy(dest+(destLen),"\x26\x61\x70\x6f\x73\x3b"); /* &apos; */
                    destLen+=(int32_t)uprv_strlen("\x26\x61\x70\x6f\x73\x3b");
                    break;

                 /* Disallow C0 controls except TAB, CR, LF*/
                case 0x00:
                case 0x01:
                case 0x02:
                case 0x03:
                case 0x04:
                case 0x05:
                case 0x06:
                case 0x07:
                case 0x08:
                /*case 0x09:*/
                /*case 0x0A: */
                case 0x0B:
                case 0x0C:
                /*case 0x0D:*/
                case 0x0E:
                case 0x0F:
                case 0x10:
                case 0x11:
                case 0x12:
                case 0x13:
                case 0x14:
                case 0x15:
                case 0x16:
                case 0x17:
                case 0x18:
                case 0x19:
                case 0x1A:
                case 0x1B:
                case 0x1C:
                case 0x1D:
                case 0x1E:
                case 0x1F:
                    *status = U_ILLEGAL_CHAR_FOUND;
                    fprintf(stderr, "Illegal Character \\u%04X!\n",(int)c);
                    uprv_free(dest);
                    return NULL;
                default:
                    dest[destLen++]=(char)c;
                }
            }else{
                UBool isError = FALSE;
                U8_APPEND((unsigned char*)dest,destLen,destCap,c,isError);
                if(isError){
                    *status = U_ILLEGAL_CHAR_FOUND;
                    fprintf(stderr, "Illegal Character \\U%08X!\n",(int)c);
                    uprv_free(dest);
                    return NULL;
                }
            }
        }else{
            destCap += destLen;

            temp = (char*) uprv_malloc(sizeof(char)*destCap);
            if(temp==NULL){
                *status=U_MEMORY_ALLOCATION_ERROR;
                uprv_free(dest);
                return NULL;
            }
            uprv_memmove(temp,dest,destLen);
            destLen=0;
            uprv_free(dest);
            dest=temp;
            temp=NULL;
        }

    }
    *destLength = destLen;
    return dest;
}

#define ASTERISK 0x002A
#define SPACE    0x0020
#define CR       0x000A
#define LF       0x000D
#define AT_SIGN  0x0040

static void
trim(char **src, int32_t *len){

    char *s = NULL;
    int32_t i = 0;
    if(src == NULL || *src == NULL){
        return;
    }
    s = *src;
    /* trim from the end */
    for( i=(*len-1); i>= 0; i--){
        switch(s[i]){
        case ASTERISK:
        case SPACE:
        case CR:
        case LF:
            s[i] = 0;
            continue;
        default:
            break;
        }
        break;

    }
    *len = i+1;
}

static void
print(UChar* src, int32_t srcLen,const char *tagStart,const char *tagEnd,  UErrorCode *status){
    int32_t bufCapacity   = srcLen*4;
    char *buf       = NULL;
    int32_t bufLen = 0;

    if(U_FAILURE(*status)){
        return;
    }

    buf = (char*) (uprv_malloc(bufCapacity));
    if(buf==0){
        fprintf(stderr, "Could not allocate memory!!");
        exit(U_MEMORY_ALLOCATION_ERROR);
    }
    buf = convertAndEscape(&buf, bufCapacity, &bufLen, src, srcLen,status);
    if(U_SUCCESS(*status)){
        trim(&buf,&bufLen);
        write_utf8_file(out,UnicodeString(tagStart));
        write_utf8_file(out,UnicodeString(buf, bufLen, "UTF-8"));
        write_utf8_file(out,UnicodeString(tagEnd));
        write_utf8_file(out,UnicodeString("\n"));

    }
}
static void
printNoteElements(struct UString *src, UErrorCode *status){

#if UCONFIG_NO_REGULAR_EXPRESSIONS==0 /* donot compile when no RegularExpressions are available */

    int32_t capacity = 0;
    UChar* note = NULL;
    int32_t noteLen = 0;
    int32_t count = 0,i;

    if(src == NULL){
        return;
    }

    capacity = src->fLength;
    note  = (UChar*) uprv_malloc(U_SIZEOF_UCHAR * capacity);

    count = getCount(src->fChars,src->fLength, UPC_NOTE, status);
    if(U_FAILURE(*status)){
        uprv_free(note);
        return;
    }
    for(i=0; i < count; i++){
        noteLen =  getAt(src->fChars,src->fLength, &note, capacity, i, UPC_NOTE, status);
        if(U_FAILURE(*status)){
            uprv_free(note);
            return;
        }
        if(noteLen > 0){
            write_tabs(out);
            print(note, noteLen,"<note>", "</note>", status);
        }
    }
    uprv_free(note);
#else

    fprintf(stderr, "Warning: Could not output comments to XLIFF file. ICU has been built without RegularExpression support.\n");

#endif /* UCONFIG_NO_REGULAR_EXPRESSIONS */

}

static void printAttribute(const char *name, const char *value, int32_t /*len*/)
{
    write_utf8_file(out, UnicodeString(" "));
    write_utf8_file(out, UnicodeString(name));
    write_utf8_file(out, UnicodeString(" = \""));
    write_utf8_file(out, UnicodeString(value));
    write_utf8_file(out, UnicodeString("\""));
}

static void printAttribute(const char *name, const UnicodeString value, int32_t /*len*/)
{
    write_utf8_file(out, UnicodeString(" "));
    write_utf8_file(out, UnicodeString(name));
    write_utf8_file(out, UnicodeString(" = \""));
    write_utf8_file(out, value);
    write_utf8_file(out, UnicodeString("\""));
}

static void
printComments(struct UString *src, const char *resName, UBool printTranslate, UErrorCode *status){

#if UCONFIG_NO_REGULAR_EXPRESSIONS==0 /* donot compile when no RegularExpressions are available */

    if(status==NULL || U_FAILURE(*status)){
        return;
    }

    int32_t capacity = src->fLength + 1;
    char* buf = NULL;
    int32_t bufLen = 0;
    UChar* desc  = (UChar*) uprv_malloc(U_SIZEOF_UCHAR * capacity);
    UChar* trans = (UChar*) uprv_malloc(U_SIZEOF_UCHAR * capacity);

    int32_t descLen = 0, transLen=0;
    if(desc==NULL || trans==NULL){
        *status = U_MEMORY_ALLOCATION_ERROR;
        uprv_free(desc);
        uprv_free(trans);
        return;
    }
    src->fLength = removeCmtText(src->fChars, src->fLength, status);
    descLen  = getDescription(src->fChars,src->fLength, &desc, capacity, status);
    transLen = getTranslate(src->fChars,src->fLength, &trans, capacity, status);

    /* first print translate attribute */
    if(transLen > 0){
        if(printTranslate){
            /* print translate attribute */
            buf = convertAndEscape(&buf, 0, &bufLen, trans, transLen, status);
            if(U_SUCCESS(*status)){
                printAttribute("translate", UnicodeString(buf, bufLen, "UTF-8"), bufLen);
                write_utf8_file(out,UnicodeString(">\n"));
            }
        }else if(getShowWarning()){
            fprintf(stderr, "Warning: Tranlate attribute for resource %s cannot be set. XLIFF prohibits it.\n", resName);
            /* no translate attribute .. just close the tag */
            write_utf8_file(out,UnicodeString(">\n"));
        }
    }else{
        /* no translate attribute .. just close the tag */
        write_utf8_file(out,UnicodeString(">\n"));
    }

    if(descLen > 0){
        write_tabs(out);
        print(desc, descLen, "<!--", "-->", status);
    }

    uprv_free(desc);
    uprv_free(trans);
#else

    fprintf(stderr, "Warning: Could not output comments to XLIFF file. ICU has been built without RegularExpression support.\n");

#endif /* UCONFIG_NO_REGULAR_EXPRESSIONS */

}

/*
 * Print out a containing element, like:
 * <trans-unit id = "blah" resname = "blah" restype = "x-id-alias" translate = "no">
 * <group id "calendar_gregorian" resname = "gregorian" restype = "x-icu-array">
 */
static char *printContainer(struct SResource *res, const char *container, const char *restype, const char *mimetype, const char *id, UErrorCode *status)
{
    char resKeyBuffer[8];
    const char *resname = NULL;
    char *sid = NULL;

    write_tabs(out);

    resname = res_getKeyString(srBundle, res, resKeyBuffer);
    if (resname != NULL && *resname != 0) {
        sid = getID(id, resname, sid);
    } else {
        sid = getID(id, NULL, sid);
    }

    write_utf8_file(out, UnicodeString("<"));
    write_utf8_file(out, UnicodeString(container));
    printAttribute("id", sid, (int32_t) uprv_strlen(sid));

    if (resname != NULL) {
        printAttribute("resname", resname, (int32_t) uprv_strlen(resname));
    }

    if (mimetype != NULL) {
        printAttribute("mime-type", mimetype, (int32_t) uprv_strlen(mimetype));
    }

    if (restype != NULL) {
        printAttribute("restype", restype, (int32_t) uprv_strlen(restype));
    }

    tabCount += 1;
    if (res->fComment.fLength > 0) {
        /* printComments will print the closing ">\n" */
        printComments(&res->fComment, resname, TRUE, status);
    } else {
        write_utf8_file(out, UnicodeString(">\n"));
    }

    return sid;
}

/* Writing Functions */

static const char *trans_unit = "trans-unit";
static const char *close_trans_unit = "</trans-unit>\n";
static const char *source = "<source>";
static const char *close_source = "</source>\n";
static const char *group = "group";
static const char *close_group = "</group>\n";

static const char *bin_unit = "bin-unit";
static const char *close_bin_unit = "</bin-unit>\n";
static const char *bin_source = "<bin-source>\n";
static const char *close_bin_source = "</bin-source>\n";
static const char *external_file = "<external-file";
/*static const char *close_external_file = "</external-file>\n";*/
static const char *internal_file = "<internal-file";
static const char *close_internal_file = "</internal-file>\n";

static const char *application_mimetype = "application"; /* add "/octet-stream"? */

static const char *alias_restype     = "x-icu-alias";
static const char *array_restype     = "x-icu-array";
static const char *binary_restype    = "x-icu-binary";
static const char *integer_restype   = "x-icu-integer";
static const char *intvector_restype = "x-icu-intvector";
static const char *table_restype     = "x-icu-table";

static void
string_write_xml(struct SResource *res, const char* id, const char* /*language*/, UErrorCode *status) {

    char *sid = NULL;
    char* buf = NULL;
    int32_t bufLen = 0;

    if(status==NULL || U_FAILURE(*status)){
        return;
    }

    sid = printContainer(res, trans_unit, NULL, NULL, id, status);

    write_tabs(out);

    write_utf8_file(out, UnicodeString(source));

    buf = convertAndEscape(&buf, 0, &bufLen, res->u.fString.fChars, res->u.fString.fLength, status);

    if (U_FAILURE(*status)) {
        return;
    }

    write_utf8_file(out, UnicodeString(buf, bufLen, "UTF-8"));
    write_utf8_file(out, UnicodeString(close_source));

    printNoteElements(&res->fComment, status);

    tabCount -= 1;
    write_tabs(out);

    write_utf8_file(out, UnicodeString(close_trans_unit));

    uprv_free(buf);
    uprv_free(sid);
}

static void
alias_write_xml(struct SResource *res, const char* id, const char* /*language*/, UErrorCode *status) {
    char *sid = NULL;
    char* buf = NULL;
    int32_t bufLen=0;

    sid = printContainer(res, trans_unit, alias_restype, NULL, id, status);

    write_tabs(out);

    write_utf8_file(out, UnicodeString(source));

    buf = convertAndEscape(&buf, 0, &bufLen, res->u.fString.fChars, res->u.fString.fLength, status);

    if(U_FAILURE(*status)){
        return;
    }
    write_utf8_file(out, UnicodeString(buf, bufLen, "UTF-8"));
    write_utf8_file(out, UnicodeString(close_source));

    printNoteElements(&res->fComment, status);

    tabCount -= 1;
    write_tabs(out);

    write_utf8_file(out, UnicodeString(close_trans_unit));

    uprv_free(buf);
    uprv_free(sid);
}

static void
array_write_xml(struct SResource *res, const char* id, const char* language, UErrorCode *status) {
    char* sid = NULL;
    int index = 0;

    struct SResource *current = NULL;
    struct SResource *first =NULL;

    sid = printContainer(res, group, array_restype, NULL, id, status);

    current = res->u.fArray.fFirst;
    first=current;

    while (current != NULL) {
        char c[256] = {0};
        char* subId = NULL;

        itostr(c, index, 10, 0);
        index += 1;
        subId = getID(sid, c, subId);

        res_write_xml(current, subId, language, FALSE, status);
        uprv_free(subId);
        subId = NULL;

        if(U_FAILURE(*status)){
            return;
        }

        current = current->fNext;
    }

    tabCount -= 1;
    write_tabs(out);
    write_utf8_file(out, UnicodeString(close_group));

    uprv_free(sid);
}

static void
intvector_write_xml(struct SResource *res, const char* id, const char* /*language*/, UErrorCode *status) {
    char* sid = NULL;
    char* ivd = NULL;
    uint32_t i=0;
    uint32_t len=0;
    char buf[256] = {'0'};

    sid = printContainer(res, group, intvector_restype, NULL, id, status);

    for(i = 0; i < res->u.fIntVector.fCount; i += 1) {
        char c[256] = {0};

        itostr(c, i, 10, 0);
        ivd = getID(sid, c, ivd);
        len = itostr(buf, res->u.fIntVector.fArray[i], 10, 0);

        write_tabs(out);
        write_utf8_file(out, UnicodeString("<"));
        write_utf8_file(out, UnicodeString(trans_unit));

        printAttribute("id", ivd, (int32_t)uprv_strlen(ivd));
        printAttribute("restype", integer_restype, (int32_t) strlen(integer_restype));

        write_utf8_file(out, UnicodeString(">\n"));

        tabCount += 1;
        write_tabs(out);
        write_utf8_file(out, UnicodeString(source));

        write_utf8_file(out, UnicodeString(buf, len));

        write_utf8_file(out, UnicodeString(close_source));
        tabCount -= 1;
        write_tabs(out);
        write_utf8_file(out, UnicodeString(close_trans_unit));

        uprv_free(ivd);
        ivd = NULL;
    }

    tabCount -= 1;
    write_tabs(out);

    write_utf8_file(out, UnicodeString(close_group));
    uprv_free(sid);
    sid = NULL;
}

static void
int_write_xml(struct SResource *res, const char* id, const char* /*language*/, UErrorCode *status) {
    char* sid = NULL;
    char buf[256] = {0};
    uint32_t len = 0;

    sid = printContainer(res, trans_unit, integer_restype, NULL, id, status);

    write_tabs(out);

    write_utf8_file(out, UnicodeString(source));

    len = itostr(buf, res->u.fIntValue.fValue, 10, 0);
    write_utf8_file(out, UnicodeString(buf, len));

    write_utf8_file(out, UnicodeString(close_source));

    printNoteElements(&res->fComment, status);

    tabCount -= 1;
    write_tabs(out);

    write_utf8_file(out, UnicodeString(close_trans_unit));

    uprv_free(sid);
    sid = NULL;
}

static void
bin_write_xml(struct SResource *res, const char* id, const char* /*language*/, UErrorCode *status) {
    const char* m_type = application_mimetype;
    char* sid = NULL;
    uint32_t crc = 0xFFFFFFFF;

    char fileName[1024] ={0};
    int32_t tLen = ( outDir == NULL) ? 0 :(int32_t)uprv_strlen(outDir);
    char* fn =  (char*) uprv_malloc(sizeof(char) * (tLen+1024 +
                                                    (res->u.fBinaryValue.fFileName !=NULL ?
                                                    uprv_strlen(res->u.fBinaryValue.fFileName) :0)));
    const char* ext = NULL;

    char* f = NULL;

    fn[0]=0;

    if(res->u.fBinaryValue.fFileName != NULL){
        uprv_strcpy(fileName, res->u.fBinaryValue.fFileName);
        f = uprv_strrchr(fileName, '\\');

        if (f != NULL) {
            f++;
        } else {
            f = fileName;
        }

        ext = uprv_strrchr(fileName, '.');

        if (ext == NULL) {
            fprintf(stderr, "Error: %s is an unknown binary filename type.\n", fileName);
            exit(U_ILLEGAL_ARGUMENT_ERROR);
        }

        if(uprv_strcmp(ext, ".jpg")==0 || uprv_strcmp(ext, ".jpeg")==0 || uprv_strcmp(ext, ".gif")==0 ){
            m_type = "image";
        } else if(uprv_strcmp(ext, ".wav")==0 || uprv_strcmp(ext, ".au")==0 ){
            m_type = "audio";
        } else if(uprv_strcmp(ext, ".avi")==0 || uprv_strcmp(ext, ".mpg")==0 || uprv_strcmp(ext, ".mpeg")==0){
            m_type = "video";
        } else if(uprv_strcmp(ext, ".txt")==0 || uprv_strcmp(ext, ".text")==0){
            m_type = "text";
        }

        sid = printContainer(res, bin_unit, binary_restype, m_type, id, status);

        write_tabs(out);

        write_utf8_file(out, UnicodeString(bin_source));

        tabCount+= 1;
        write_tabs(out);

        write_utf8_file(out, UnicodeString(external_file));
        printAttribute("href", f, (int32_t)uprv_strlen(f));
        write_utf8_file(out, UnicodeString("/>\n"));
        tabCount -= 1;
        write_tabs(out);

        write_utf8_file(out, UnicodeString(close_bin_source));

        printNoteElements(&res->fComment, status);
        tabCount -= 1;
        write_tabs(out);
        write_utf8_file(out, UnicodeString(close_bin_unit));
    } else {
        char temp[256] = {0};
        uint32_t i = 0;
        int32_t len=0;

        sid = printContainer(res, bin_unit, binary_restype, m_type, id, status);

        write_tabs(out);
        write_utf8_file(out, UnicodeString(bin_source));

        tabCount += 1;
        write_tabs(out);

        write_utf8_file(out, UnicodeString(internal_file));
        printAttribute("form", application_mimetype, (int32_t) uprv_strlen(application_mimetype));

        while(i <res->u.fBinaryValue.fLength){
            len = itostr(temp, res->u.fBinaryValue.fData[i], 16, 2);
            crc = computeCRC(temp, len, crc);
            i++;
        }

        len = itostr(temp, crc, 10, 0);
        printAttribute("crc", temp, len);

        write_utf8_file(out, UnicodeString(">"));

        i = 0;
        while(i <res->u.fBinaryValue.fLength){
            len = itostr(temp, res->u.fBinaryValue.fData[i], 16, 2);
            write_utf8_file(out, UnicodeString(temp));
            i += 1;
        }

        write_utf8_file(out, UnicodeString(close_internal_file));

        tabCount -= 2;
        write_tabs(out);

        write_utf8_file(out, UnicodeString(close_bin_source));
        printNoteElements(&res->fComment, status);

        tabCount -= 1;
        write_tabs(out);
        write_utf8_file(out, UnicodeString(close_bin_unit));

        uprv_free(sid);
        sid = NULL;
    }

    uprv_free(fn);
}



static void
table_write_xml(struct SResource *res, const char* id, const char* language, UBool isTopLevel, UErrorCode *status) {

    uint32_t  i         = 0;

    struct SResource *current = NULL;
    struct SResource *save = NULL;
    char* sid = NULL;

    if (U_FAILURE(*status)) {
        return ;
    }

    sid = printContainer(res, group, table_restype, NULL, id, status);

    if(isTopLevel) {
        sid[0] = '\0';
    }

    save = current = res->u.fTable.fFirst;
    i = 0;

    while (current != NULL) {
        res_write_xml(current, sid, language, FALSE, status);

        if(U_FAILURE(*status)){
            return;
        }

        i += 1;
        current = current->fNext;
    }

    tabCount -= 1;
    write_tabs(out);

    write_utf8_file(out, UnicodeString(close_group));

    uprv_free(sid);
    sid = NULL;
}

void
res_write_xml(struct SResource *res, const char* id,  const char* language, UBool isTopLevel, UErrorCode *status) {

    if (U_FAILURE(*status)) {
        return ;
    }

    if (res != NULL) {
        switch (res->fType) {
        case URES_STRING:
             string_write_xml    (res, id, language, status);
             return;

        case URES_ALIAS:
             alias_write_xml     (res, id, language, status);
             return;

        case URES_INT_VECTOR:
             intvector_write_xml (res, id, language, status);
             return;

        case URES_BINARY:
             bin_write_xml       (res, id, language, status);
             return;

        case URES_INT:
             int_write_xml       (res, id, language, status);
             return;

        case URES_ARRAY:
             array_write_xml     (res, id, language, status);
             return;

        case URES_TABLE:
             table_write_xml     (res, id, language, isTopLevel, status);
             return;

        default:
            break;
        }
    }

    *status = U_INTERNAL_PROGRAM_ERROR;
}

void
bundle_write_xml(struct SRBRoot *bundle, const char *outputDir,const char* outputEnc, const char* filename,
                  char *writtenFilename, int writtenFilenameLen,
                  const char* language, const char* outFileName, UErrorCode *status) {

    char* xmlfileName = NULL;
    char* outputFileName = NULL;
    char* originalFileName = NULL;
    const char* fileStart = "<file xml:space = \"preserve\" source-language = \"";
    const char* file1 = "\" datatype = \"x-icu-resource-bundle\" ";
    const char* file2 = "original = \"";
    const char* file4 = "\" date = \"";
    const char* fileEnd = "</file>\n";
    const char* headerStart = "<header>\n";
    const char* headerEnd = "</header>\n";
    const char* bodyStart = "<body>\n";
    const char* bodyEnd = "</body>\n";

    const char *tool_start = "<tool";
    const char *tool_id = "genrb-" GENRB_VERSION "-icu-" U_ICU_VERSION;
    const char *tool_name = "genrb";

    char* temp = NULL;
    char* lang = NULL;
    const char* pos = NULL;
    int32_t first, index;
    time_t currTime;
    char timeBuf[128];

    outDir = outputDir;

    srBundle = bundle;

    pos = uprv_strrchr(filename, '\\');
    if(pos != NULL) {
        first = (int32_t)(pos - filename + 1);
    } else {
        first = 0;
    }
    index = (int32_t)(uprv_strlen(filename) - uprv_strlen(textExt) - first);
    originalFileName = (char *)uprv_malloc(sizeof(char)*index+1);
    uprv_memset(originalFileName, 0, sizeof(char)*index+1);
    uprv_strncpy(originalFileName, filename + first, index);

    if(uprv_strcmp(originalFileName, srBundle->fLocale) != 0) {
        fprintf(stdout, "Warning: The file name is not same as the resource name!\n");
    }

    temp = originalFileName;
    originalFileName = (char *)uprv_malloc(sizeof(char)* (uprv_strlen(temp)+uprv_strlen(textExt)) + 1);
    uprv_memset(originalFileName, 0, sizeof(char)* (uprv_strlen(temp)+uprv_strlen(textExt)) + 1);
    uprv_strcat(originalFileName, temp);
    uprv_strcat(originalFileName, textExt);
    uprv_free(temp);
    temp = NULL;


    if (language == NULL) {
/*        lang = parseFilename(filename, lang);
        if (lang == NULL) {*/
            /* now check if locale name is valid or not
             * this is to cater for situation where
             * pegasusServer.txt contains
             *
             * en{
             *      ..
             * }
             */
             lang = parseFilename(srBundle->fLocale, lang);
             /*
              * Neither  the file name nor the table name inside the
              * txt file contain a valid country and language codes
              * throw an error.
              * pegasusServer.txt contains
              *
              * testelements{
              *     ....
              * }
              */
             if(lang==NULL){
                 fprintf(stderr, "Error: The file name and table name do not contain a valid language code. Please use -l option to specify it.\n");
                 exit(U_ILLEGAL_ARGUMENT_ERROR);
             }
       /* }*/
    } else {
        lang = (char *)uprv_malloc(sizeof(char)*uprv_strlen(language) +1);
        uprv_memset(lang, 0, sizeof(char)*uprv_strlen(language) +1);
        uprv_strcpy(lang, language);
    }

    if(outFileName) {
        outputFileName = (char *)uprv_malloc(sizeof(char)*uprv_strlen(outFileName) + 1);
        uprv_memset(outputFileName, 0, sizeof(char)*uprv_strlen(outFileName) + 1);
        uprv_strcpy(outputFileName,outFileName);
    } else {
        outputFileName = (char *)uprv_malloc(sizeof(char)*uprv_strlen(srBundle->fLocale) + 1);
        uprv_memset(outputFileName, 0, sizeof(char)*uprv_strlen(srBundle->fLocale) + 1);
        uprv_strcpy(outputFileName,srBundle->fLocale);
    }

    if(outputDir) {
        xmlfileName = (char *)uprv_malloc(sizeof(char)*(uprv_strlen(outputDir) + uprv_strlen(outputFileName) + uprv_strlen(xliffExt) + 1) +1);
        uprv_memset(xmlfileName, 0, sizeof(char)*(uprv_strlen(outputDir)+ uprv_strlen(outputFileName) + uprv_strlen(xliffExt) + 1) +1);
    } else {
        xmlfileName = (char *)uprv_malloc(sizeof(char)*(uprv_strlen(outputFileName) + uprv_strlen(xliffExt)) +1);
        uprv_memset(xmlfileName, 0, sizeof(char)*(uprv_strlen(outputFileName) + uprv_strlen(xliffExt)) +1);
    }

    if(outputDir){
        uprv_strcpy(xmlfileName, outputDir);
        if(outputDir[uprv_strlen(outputDir)-1] !=U_FILE_SEP_CHAR){
            uprv_strcat(xmlfileName,U_FILE_SEP_STRING);
        }
    }
    uprv_strcat(xmlfileName,outputFileName);
    uprv_strcat(xmlfileName,xliffExt);

    if (writtenFilename) {
        uprv_strncpy(writtenFilename, xmlfileName, writtenFilenameLen);
    }

    if (U_FAILURE(*status)) {
        goto cleanup_bundle_write_xml;
    }

    out= T_FileStream_open(xmlfileName,"w");

    if(out==NULL){
        *status = U_FILE_ACCESS_ERROR;
        goto cleanup_bundle_write_xml;
    }
    write_utf8_file(out, xmlHeader);

    if(outputEnc && *outputEnc!='\0'){
        /* store the output encoding */
        enc = outputEnc;
        conv=ucnv_open(enc,status);
        if(U_FAILURE(*status)){
            goto cleanup_bundle_write_xml;
        }
    }
    write_utf8_file(out, bundleStart);
    write_tabs(out);
    write_utf8_file(out, fileStart);
    /* check if lang and language are the same */
    if(language != NULL && uprv_strcmp(lang, srBundle->fLocale)!=0){
        fprintf(stderr,"Warning: The top level tag in the resource and language specified are not the same. Please check the input.\n");
    }
    write_utf8_file(out, UnicodeString(lang));
    write_utf8_file(out, UnicodeString(file1));
    write_utf8_file(out, UnicodeString(file2));
    write_utf8_file(out, UnicodeString(originalFileName));
    write_utf8_file(out, UnicodeString(file4));

    time(&currTime);
    strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%dT%H:%M:%SZ", gmtime(&currTime));
    write_utf8_file(out, UnicodeString(timeBuf));
    write_utf8_file(out, UnicodeString("\">\n"));

    tabCount += 1;
    write_tabs(out);
    write_utf8_file(out, headerStart);

    tabCount += 1;
    write_tabs(out);

    write_utf8_file(out, tool_start);
    printAttribute("tool-id", tool_id, (int32_t) uprv_strlen(tool_id));
    printAttribute("tool-name", tool_name, (int32_t) uprv_strlen(tool_name));
    write_utf8_file(out, UnicodeString("/>\n"));

    tabCount -= 1;
    write_tabs(out);

    write_utf8_file(out, UnicodeString(headerEnd));

    write_tabs(out);
    tabCount += 1;

    write_utf8_file(out, UnicodeString(bodyStart));


    res_write_xml(bundle->fRoot, bundle->fLocale, lang, TRUE, status);

    tabCount -= 1;
    write_tabs(out);

    write_utf8_file(out, UnicodeString(bodyEnd));
    tabCount--;
    write_tabs(out);
    write_utf8_file(out, UnicodeString(fileEnd));
    tabCount--;
    write_tabs(out);
    write_utf8_file(out, UnicodeString(bundleEnd));
    T_FileStream_close(out);

    ucnv_close(conv);

cleanup_bundle_write_xml:
    uprv_free(originalFileName);
    uprv_free(lang);
    if(xmlfileName != NULL) {
        uprv_free(xmlfileName);
    }
    if(outputFileName != NULL){
        uprv_free(outputFileName);
    }
}
