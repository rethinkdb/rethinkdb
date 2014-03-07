/*
*******************************************************************************
*
*   Copyright (C) 1998-2010, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*
* File parse.c
*
* Modification History:
*
*   Date          Name          Description
*   05/26/99     stephen       Creation.
*   02/25/00     weiv          Overhaul to write udata
*   5/10/01      Ram           removed ustdio dependency
*   06/10/2001  Dominic Ludlam <dom@recoil.org> Rewritten
*******************************************************************************
*/

#include "ucol_imp.h"
#include "parse.h"
#include "errmsg.h"
#include "uhash.h"
#include "cmemory.h"
#include "cstring.h"
#include "uinvchar.h"
#include "read.h"
#include "ustr.h"
#include "reslist.h"
#include "rbt_pars.h"
#include "genrb.h"
#include "unicode/ustring.h"
#include "unicode/uscript.h"
#include "unicode/putil.h"
#include <stdio.h>

extern UBool gIncludeUnihanColl;

/* Number of tokens to read ahead of the current stream position */
#define MAX_LOOKAHEAD   3

#define CR               0x000D
#define LF               0x000A
#define SPACE            0x0020
#define TAB              0x0009
#define ESCAPE           0x005C
#define HASH             0x0023
#define QUOTE            0x0027
#define ZERO             0x0030
#define STARTCOMMAND     0x005B
#define ENDCOMMAND       0x005D
#define OPENSQBRACKET    0x005B
#define CLOSESQBRACKET   0x005D

struct Lookahead
{
     enum   ETokenType type;
     struct UString    value;
     struct UString    comment;
     uint32_t          line;
};

/* keep in sync with token defines in read.h */
const char *tokenNames[TOK_TOKEN_COUNT] =
{
     "string",             /* A string token, such as "MonthNames" */
     "'{'",                 /* An opening brace character */
     "'}'",                 /* A closing brace character */
     "','",                 /* A comma */
     "':'",                 /* A colon */

     "<end of file>",     /* End of the file has been reached successfully */
     "<end of line>"
};

/* Just to store "TRUE" */
static const UChar trueValue[] = {0x0054, 0x0052, 0x0055, 0x0045, 0x0000};

typedef struct {
    struct Lookahead  lookahead[MAX_LOOKAHEAD + 1];
    uint32_t          lookaheadPosition;
    UCHARBUF         *buffer;
    struct SRBRoot *bundle;
    const char     *inputdir;
    uint32_t        inputdirLength;
    const char     *outputdir;
    uint32_t        outputdirLength;
} ParseState;

static UBool gMakeBinaryCollation = TRUE;
static UBool gOmitCollationRules  = FALSE;

typedef struct SResource *
ParseResourceFunction(ParseState* state, char *tag, uint32_t startline, const struct UString* comment, UErrorCode *status);

static struct SResource *parseResource(ParseState* state, char *tag, const struct UString *comment, UErrorCode *status);

/* The nature of the lookahead buffer:
   There are MAX_LOOKAHEAD + 1 slots, used as a circular buffer.  This provides
   MAX_LOOKAHEAD lookahead tokens and a slot for the current token and value.
   When getToken is called, the current pointer is moved to the next slot and the
   old slot is filled with the next token from the reader by calling getNextToken.
   The token values are stored in the slot, which means that token values don't
   survive a call to getToken, ie.

   UString *value;

   getToken(&value, NULL, status);
   getToken(NULL,   NULL, status);       bad - value is now a different string
*/
static void
initLookahead(ParseState* state, UCHARBUF *buf, UErrorCode *status)
{
    static uint32_t initTypeStrings = 0;
    uint32_t i;

    if (!initTypeStrings)
    {
        initTypeStrings = 1;
    }

    state->lookaheadPosition   = 0;
    state->buffer              = buf;

    resetLineNumber();

    for (i = 0; i < MAX_LOOKAHEAD; i++)
    {
        state->lookahead[i].type = getNextToken(state->buffer, &state->lookahead[i].value, &state->lookahead[i].line, &state->lookahead[i].comment, status);
        if (U_FAILURE(*status))
        {
            return;
        }
    }

    *status = U_ZERO_ERROR;
}

static void
cleanupLookahead(ParseState* state)
{
    uint32_t i;
    for (i = 0; i < MAX_LOOKAHEAD; i++)
    {
        ustr_deinit(&state->lookahead[i].value);
        ustr_deinit(&state->lookahead[i].comment);
    }

}

static enum ETokenType
getToken(ParseState* state, struct UString **tokenValue, struct UString* comment, uint32_t *linenumber, UErrorCode *status)
{
    enum ETokenType result;
    uint32_t          i;

    result = state->lookahead[state->lookaheadPosition].type;

    if (tokenValue != NULL)
    {
        *tokenValue = &state->lookahead[state->lookaheadPosition].value;
    }

    if (linenumber != NULL)
    {
        *linenumber = state->lookahead[state->lookaheadPosition].line;
    }

    if (comment != NULL)
    {
        ustr_cpy(comment, &(state->lookahead[state->lookaheadPosition].comment), status);
    }

    i = (state->lookaheadPosition + MAX_LOOKAHEAD) % (MAX_LOOKAHEAD + 1);
    state->lookaheadPosition = (state->lookaheadPosition + 1) % (MAX_LOOKAHEAD + 1);
    ustr_setlen(&state->lookahead[i].comment, 0, status);
    ustr_setlen(&state->lookahead[i].value, 0, status);
    state->lookahead[i].type = getNextToken(state->buffer, &state->lookahead[i].value, &state->lookahead[i].line, &state->lookahead[i].comment, status);

    /* printf("getToken, returning %s\n", tokenNames[result]); */

    return result;
}

static enum ETokenType
peekToken(ParseState* state, uint32_t lookaheadCount, struct UString **tokenValue, uint32_t *linenumber, struct UString *comment, UErrorCode *status)
{
    uint32_t i = (state->lookaheadPosition + lookaheadCount) % (MAX_LOOKAHEAD + 1);

    if (U_FAILURE(*status))
    {
        return TOK_ERROR;
    }

    if (lookaheadCount >= MAX_LOOKAHEAD)
    {
        *status = U_INTERNAL_PROGRAM_ERROR;
        return TOK_ERROR;
    }

    if (tokenValue != NULL)
    {
        *tokenValue = &state->lookahead[i].value;
    }

    if (linenumber != NULL)
    {
        *linenumber = state->lookahead[i].line;
    }

    if(comment != NULL){
        ustr_cpy(comment, &(state->lookahead[state->lookaheadPosition].comment), status);
    }

    return state->lookahead[i].type;
}

static void
expect(ParseState* state, enum ETokenType expectedToken, struct UString **tokenValue, struct UString *comment, uint32_t *linenumber, UErrorCode *status)
{
    uint32_t        line;

    enum ETokenType token = getToken(state, tokenValue, comment, &line, status);

    if (linenumber != NULL)
    {
        *linenumber = line;
    }

    if (U_FAILURE(*status))
    {
        return;
    }

    if (token != expectedToken)
    {
        *status = U_INVALID_FORMAT_ERROR;
        error(line, "expecting %s, got %s", tokenNames[expectedToken], tokenNames[token]);
    }
    else
    {
        *status = U_ZERO_ERROR;
    }
}

static char *getInvariantString(ParseState* state, uint32_t *line, struct UString *comment, UErrorCode *status)
{
    struct UString *tokenValue;
    char           *result;
    uint32_t        count;

    expect(state, TOK_STRING, &tokenValue, comment, line, status);

    if (U_FAILURE(*status))
    {
        return NULL;
    }

    count = u_strlen(tokenValue->fChars);
    if(!uprv_isInvariantUString(tokenValue->fChars, count)) {
        *status = U_INVALID_FORMAT_ERROR;
        error(*line, "invariant characters required for table keys, binary data, etc.");
        return NULL;
    }

    result = uprv_malloc(count+1);

    if (result == NULL)
    {
        *status = U_MEMORY_ALLOCATION_ERROR;
        return NULL;
    }

    u_UCharsToChars(tokenValue->fChars, result, count+1);
    return result;
}

static struct SResource *
parseUCARules(ParseState* state, char *tag, uint32_t startline, const struct UString* comment, UErrorCode *status)
{
    struct SResource *result = NULL;
    struct UString   *tokenValue;
    FileStream       *file          = NULL;
    char              filename[256] = { '\0' };
    char              cs[128]       = { '\0' };
    uint32_t          line;
    int               len=0;
    UBool quoted = FALSE;
    UCHARBUF *ucbuf=NULL;
    UChar32   c     = 0;
    const char* cp  = NULL;
    UChar *pTarget     = NULL;
    UChar *target      = NULL;
    UChar *targetLimit = NULL;
    int32_t size = 0;

    expect(state, TOK_STRING, &tokenValue, NULL, &line, status);

    if(isVerbose()){
        printf(" %s at line %i \n",  (tag == NULL) ? "(null)" : tag, (int)startline);
    }

    if (U_FAILURE(*status))
    {
        return NULL;
    }
    /* make the filename including the directory */
    if (state->inputdir != NULL)
    {
        uprv_strcat(filename, state->inputdir);

        if (state->inputdir[state->inputdirLength - 1] != U_FILE_SEP_CHAR)
        {
            uprv_strcat(filename, U_FILE_SEP_STRING);
        }
    }

    u_UCharsToChars(tokenValue->fChars, cs, tokenValue->fLength);

    expect(state, TOK_CLOSE_BRACE, NULL, NULL, NULL, status);

    if (U_FAILURE(*status))
    {
        return NULL;
    }
    uprv_strcat(filename, cs);

    if(gOmitCollationRules) {
        return res_none();
    }

    ucbuf = ucbuf_open(filename, &cp, getShowWarning(),FALSE, status);

    if (U_FAILURE(*status)) {
        error(line, "An error occured while opening the input file %s\n", filename);
        return NULL;
    }

    /* We allocate more space than actually required
    * since the actual size needed for storing UChars
    * is not known in UTF-8 byte stream
    */
    size        = ucbuf_size(ucbuf) + 1;
    pTarget     = (UChar*) uprv_malloc(U_SIZEOF_UCHAR * size);
    uprv_memset(pTarget, 0, size*U_SIZEOF_UCHAR);
    target      = pTarget;
    targetLimit = pTarget+size;

    /* read the rules into the buffer */
    while (target < targetLimit)
    {
        c = ucbuf_getc(ucbuf, status);
        if(c == QUOTE) {
            quoted = (UBool)!quoted;
        }
        /* weiv (06/26/2002): adding the following:
         * - preserving spaces in commands [...]
         * - # comments until the end of line
         */
        if (c == STARTCOMMAND && !quoted)
        {
            /* preserve commands
             * closing bracket will be handled by the
             * append at the end of the loop
             */
            while(c != ENDCOMMAND) {
                U_APPEND_CHAR32(c, target,len);
                c = ucbuf_getc(ucbuf, status);
            }
        }
        else if (c == HASH && !quoted) {
            /* skip comments */
            while(c != CR && c != LF) {
                c = ucbuf_getc(ucbuf, status);
            }
            continue;
        }
        else if (c == ESCAPE)
        {
            c = unescape(ucbuf, status);

            if (c == U_ERR)
            {
                uprv_free(pTarget);
                T_FileStream_close(file);
                return NULL;
            }
        }
        else if (!quoted && (c == SPACE || c == TAB || c == CR || c == LF))
        {
            /* ignore spaces carriage returns
            * and line feed unless in the form \uXXXX
            */
            continue;
        }

        /* Append UChar * after dissembling if c > 0xffff*/
        if (c != U_EOF)
        {
            U_APPEND_CHAR32(c, target,len);
        }
        else
        {
            break;
        }
    }

    /* terminate the string */
    if(target < targetLimit){
        *target = 0x0000;
    }

    result = string_open(state->bundle, tag, pTarget, (int32_t)(target - pTarget), NULL, status);


    ucbuf_close(ucbuf);
    uprv_free(pTarget);
    T_FileStream_close(file);

    return result;
}

static struct SResource *
parseTransliterator(ParseState* state, char *tag, uint32_t startline, const struct UString* comment, UErrorCode *status)
{
    struct SResource *result = NULL;
    struct UString   *tokenValue;
    FileStream       *file          = NULL;
    char              filename[256] = { '\0' };
    char              cs[128]       = { '\0' };
    uint32_t          line;
    UCHARBUF *ucbuf=NULL;
    const char* cp  = NULL;
    UChar *pTarget     = NULL;
    const UChar *pSource     = NULL;
    int32_t size = 0;

    expect(state, TOK_STRING, &tokenValue, NULL, &line, status);

    if(isVerbose()){
        printf(" %s at line %i \n",  (tag == NULL) ? "(null)" : tag, (int)startline);
    }

    if (U_FAILURE(*status))
    {
        return NULL;
    }
    /* make the filename including the directory */
    if (state->inputdir != NULL)
    {
        uprv_strcat(filename, state->inputdir);

        if (state->inputdir[state->inputdirLength - 1] != U_FILE_SEP_CHAR)
        {
            uprv_strcat(filename, U_FILE_SEP_STRING);
        }
    }

    u_UCharsToChars(tokenValue->fChars, cs, tokenValue->fLength);

    expect(state, TOK_CLOSE_BRACE, NULL, NULL, NULL, status);

    if (U_FAILURE(*status))
    {
        return NULL;
    }
    uprv_strcat(filename, cs);


    ucbuf = ucbuf_open(filename, &cp, getShowWarning(),FALSE, status);

    if (U_FAILURE(*status)) {
        error(line, "An error occured while opening the input file %s\n", filename);
        return NULL;
    }

    /* We allocate more space than actually required
    * since the actual size needed for storing UChars
    * is not known in UTF-8 byte stream
    */
    pSource = ucbuf_getBuffer(ucbuf, &size, status);
    pTarget     = (UChar*) uprv_malloc(U_SIZEOF_UCHAR * (size + 1));
    uprv_memset(pTarget, 0, size*U_SIZEOF_UCHAR);

#if !UCONFIG_NO_TRANSLITERATION
    size = utrans_stripRules(pSource, size, pTarget, status);
#else
    size = 0;
    fprintf(stderr, " Warning: writing empty transliteration data ( UCONFIG_NO_TRANSLITERATION ) \n");
#endif
    result = string_open(state->bundle, tag, pTarget, size, NULL, status);

    ucbuf_close(ucbuf);
    uprv_free(pTarget);
    T_FileStream_close(file);

    return result;
}
static struct SResource* dependencyArray = NULL;

static struct SResource *
parseDependency(ParseState* state, char *tag, uint32_t startline, const struct UString* comment, UErrorCode *status)
{
    struct SResource *result = NULL;
    struct SResource *elem = NULL;
    struct UString   *tokenValue;
    uint32_t          line;
    char              filename[256] = { '\0' };
    char              cs[128]       = { '\0' };
    
    expect(state, TOK_STRING, &tokenValue, NULL, &line, status);

    if(isVerbose()){
        printf(" %s at line %i \n",  (tag == NULL) ? "(null)" : tag, (int)startline);
    }

    if (U_FAILURE(*status))
    {
        return NULL;
    }
    /* make the filename including the directory */
    if (state->outputdir != NULL)
    {
        uprv_strcat(filename, state->outputdir);

        if (state->outputdir[state->outputdirLength - 1] != U_FILE_SEP_CHAR)
        {
            uprv_strcat(filename, U_FILE_SEP_STRING);
        }
    }
    
    u_UCharsToChars(tokenValue->fChars, cs, tokenValue->fLength);

    if (U_FAILURE(*status))
    {
        return NULL;
    }
    uprv_strcat(filename, cs);
    if(!T_FileStream_file_exists(filename)){
        if(isStrict()){
            error(line, "The dependency file %s does not exist. Please make sure it exists.\n",filename);
        }else{
            warning(line, "The dependency file %s does not exist. Please make sure it exists.\n",filename);       
        }
    }
    if(dependencyArray==NULL){
        dependencyArray = array_open(state->bundle, "%%DEPENDENCY", NULL, status);
    }
    if(tag!=NULL){
        result = string_open(state->bundle, tag, tokenValue->fChars, tokenValue->fLength, comment, status);
    }
    elem = string_open(state->bundle, NULL, tokenValue->fChars, tokenValue->fLength, comment, status);

    array_add(dependencyArray, elem, status);

    if (U_FAILURE(*status))
    {
        return NULL;
    }
    expect(state, TOK_CLOSE_BRACE, NULL, NULL, NULL, status);
    return result;
}
static struct SResource *
parseString(ParseState* state, char *tag, uint32_t startline, const struct UString* comment, UErrorCode *status)
{
    struct UString   *tokenValue;
    struct SResource *result = NULL;

/*    if (tag != NULL && uprv_strcmp(tag, "%%UCARULES") == 0)
    {
        return parseUCARules(tag, startline, status);
    }*/
    if(isVerbose()){
        printf(" string %s at line %i \n",  (tag == NULL) ? "(null)" : tag, (int)startline);
    }
    expect(state, TOK_STRING, &tokenValue, NULL, NULL, status);

    if (U_SUCCESS(*status))
    {
        /* create the string now - tokenValue doesn't survive a call to getToken (and therefore
        doesn't survive expect either) */

        result = string_open(state->bundle, tag, tokenValue->fChars, tokenValue->fLength, comment, status);
        if(U_SUCCESS(*status) && result) {
            expect(state, TOK_CLOSE_BRACE, NULL, NULL, NULL, status);

            if (U_FAILURE(*status))
            {
                res_close(result);
                return NULL;
            }
        }
    }

    return result;
}

static struct SResource *
parseAlias(ParseState* state, char *tag, uint32_t startline, const struct UString *comment, UErrorCode *status)
{
    struct UString   *tokenValue;
    struct SResource *result  = NULL;

    expect(state, TOK_STRING, &tokenValue, NULL, NULL, status);

    if(isVerbose()){
        printf(" alias %s at line %i \n",  (tag == NULL) ? "(null)" : tag, (int)startline);
    }

    if (U_SUCCESS(*status))
    {
        /* create the string now - tokenValue doesn't survive a call to getToken (and therefore
        doesn't survive expect either) */

        result = alias_open(state->bundle, tag, tokenValue->fChars, tokenValue->fLength, comment, status);

        expect(state, TOK_CLOSE_BRACE, NULL, NULL, NULL, status);

        if (U_FAILURE(*status))
        {
            res_close(result);
            return NULL;
        }
    }

    return result;
}

typedef struct{
    const char* inputDir;
    const char* outputDir;
} GenrbData;

static struct SResource* resLookup(struct SResource* res, const char* key){
    struct SResource *current = NULL;
    struct SResTable *list;
    if (res == res_none()) {
        return NULL;
    }

    list = &(res->u.fTable);

    current = list->fFirst;
    while (current != NULL) {
        if (uprv_strcmp(((list->fRoot->fKeys) + (current->fKey)), key) == 0) {
            return current;
        }
        current = current->fNext;
    }
    return NULL;
}

static const UChar* importFromDataFile(void* context, const char* locale, const char* type, int32_t* pLength, UErrorCode* status){
    struct SRBRoot *data         = NULL;
    UCHARBUF       *ucbuf        = NULL;
    GenrbData* genrbdata = (GenrbData*) context;
    int localeLength = strlen(locale);
    char* filename = (char*)uprv_malloc(localeLength+5);
    char           *inputDirBuf  = NULL;
    char           *openFileName = NULL;
    const char* cp = "";
    UChar* urules = NULL;
    int32_t urulesLength = 0;
    int32_t i = 0;
    int32_t dirlen  = 0;
    int32_t filelen = 0;
    struct SResource* root;
    struct SResource* collations;
    struct SResource* collation;
    struct SResource* sequence;

    memcpy(filename, locale, localeLength);
    for(i = 0; i < localeLength; i++){
        if(filename[i] == '-'){
            filename[i] = '_';
        }
    }
    filename[localeLength]   = '.';
    filename[localeLength+1] = 't';
    filename[localeLength+2] = 'x';
    filename[localeLength+3] = 't';
    filename[localeLength+4] = 0;


    if (status==NULL || U_FAILURE(*status)) {
        return NULL;
    }
    if(filename==NULL){
        *status=U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    }else{
        filelen = (int32_t)uprv_strlen(filename);
    }
    if(genrbdata->inputDir == NULL) {
        const char *filenameBegin = uprv_strrchr(filename, U_FILE_SEP_CHAR);
        openFileName = (char *) uprv_malloc(dirlen + filelen + 2);
        openFileName[0] = '\0';
        if (filenameBegin != NULL) {
            /*
             * When a filename ../../../data/root.txt is specified,
             * we presume that the input directory is ../../../data
             * This is very important when the resource file includes
             * another file, like UCARules.txt or thaidict.brk.
             */
            int32_t filenameSize = (int32_t)(filenameBegin - filename + 1);
            inputDirBuf = uprv_strncpy((char *)uprv_malloc(filenameSize), filename, filenameSize);

            /* test for NULL */
            if(inputDirBuf == NULL) {
                *status = U_MEMORY_ALLOCATION_ERROR;
                goto finish;
            }

            inputDirBuf[filenameSize - 1] = 0;
            genrbdata->inputDir = inputDirBuf;
            dirlen  = (int32_t)uprv_strlen(genrbdata->inputDir);
        }
    }else{
        dirlen  = (int32_t)uprv_strlen(genrbdata->inputDir);

        if(genrbdata->inputDir[dirlen-1] != U_FILE_SEP_CHAR) {
            openFileName = (char *) uprv_malloc(dirlen + filelen + 2);

            /* test for NULL */
            if(openFileName == NULL) {
                *status = U_MEMORY_ALLOCATION_ERROR;
                goto finish;
            }

            openFileName[0] = '\0';
            /*
             * append the input dir to openFileName if the first char in
             * filename is not file seperation char and the last char input directory is  not '.'.
             * This is to support :
             * genrb -s. /home/icu/data
             * genrb -s. icu/data
             * The user cannot mix notations like
             * genrb -s. /icu/data --- the absolute path specified. -s redundant
             * user should use
             * genrb -s. icu/data  --- start from CWD and look in icu/data dir
             */
            if( (filename[0] != U_FILE_SEP_CHAR) && (genrbdata->inputDir[dirlen-1] !='.')){
                uprv_strcpy(openFileName, genrbdata->inputDir);
                openFileName[dirlen]     = U_FILE_SEP_CHAR;
            }
            openFileName[dirlen + 1] = '\0';
        } else {
            openFileName = (char *) uprv_malloc(dirlen + filelen + 1);

            /* test for NULL */
            if(openFileName == NULL) {
                *status = U_MEMORY_ALLOCATION_ERROR;
                goto finish;
            }

            uprv_strcpy(openFileName, genrbdata->inputDir);

        }
    }
    uprv_strcat(openFileName, filename);
    /* printf("%s\n", openFileName);  */
    *status = U_ZERO_ERROR;
    ucbuf = ucbuf_open(openFileName, &cp,getShowWarning(),TRUE, status);

    if(*status == U_FILE_ACCESS_ERROR) {

        fprintf(stderr, "couldn't open file %s\n", openFileName == NULL ? filename : openFileName);
        goto finish;
    }
    if (ucbuf == NULL || U_FAILURE(*status)) {
        fprintf(stderr, "An error occured processing file %s. Error: %s\n", openFileName == NULL ? filename : openFileName,u_errorName(*status));
        goto finish;
    }

    /* Parse the data into an SRBRoot */
    data = parse(ucbuf, genrbdata->inputDir, genrbdata->outputDir, status);

    root = data->fRoot;
    collations = resLookup(root, "collations");
    collation = resLookup(collations, type);
    sequence = resLookup(collation, "Sequence");
    urules = sequence->u.fString.fChars;
    urulesLength = sequence->u.fString.fLength;
    *pLength = urulesLength;

finish:

    if (inputDirBuf != NULL) {
        uprv_free(inputDirBuf);
    }

    if (openFileName != NULL) {
        uprv_free(openFileName);
    }

    if(ucbuf) {
        ucbuf_close(ucbuf);
    }

    return urules;
}

static struct SResource *
addCollation(ParseState* state, struct SResource  *result, uint32_t startline, UErrorCode *status)
{
    struct SResource  *member = NULL;
    struct UString    *tokenValue;
    struct UString     comment;
    enum   ETokenType  token;
    char               subtag[1024];
    UVersionInfo       version;
    UBool              override = FALSE;
    uint32_t           line;
    GenrbData genrbdata;
    /* '{' . (name resource)* '}' */
    version[0]=0; version[1]=0; version[2]=0; version[3]=0;

    for (;;)
    {
        ustr_init(&comment);
        token = getToken(state, &tokenValue, &comment, &line, status);

        if (token == TOK_CLOSE_BRACE)
        {
            return result;
        }

        if (token != TOK_STRING)
        {
            res_close(result);
            *status = U_INVALID_FORMAT_ERROR;

            if (token == TOK_EOF)
            {
                error(startline, "unterminated table");
            }
            else
            {
                error(line, "Unexpected token %s", tokenNames[token]);
            }

            return NULL;
        }

        u_UCharsToChars(tokenValue->fChars, subtag, u_strlen(tokenValue->fChars) + 1);

        if (U_FAILURE(*status))
        {
            res_close(result);
            return NULL;
        }

        member = parseResource(state, subtag, NULL, status);

        if (U_FAILURE(*status))
        {
            res_close(result);
            return NULL;
        }

        if (uprv_strcmp(subtag, "Version") == 0)
        {
            char     ver[40];
            int32_t length = member->u.fString.fLength;

            if (length >= (int32_t) sizeof(ver))
            {
                length = (int32_t) sizeof(ver) - 1;
            }

            u_UCharsToChars(member->u.fString.fChars, ver, length + 1); /* +1 for copying NULL */
            u_versionFromString(version, ver);

            table_add(result, member, line, status);

        }
        else if (uprv_strcmp(subtag, "Override") == 0)
        {
            override = FALSE;

            if (u_strncmp(member->u.fString.fChars, trueValue, u_strlen(trueValue)) == 0)
            {
                override = TRUE;
            }
            table_add(result, member, line, status);

        }
        else if(uprv_strcmp(subtag, "%%CollationBin")==0)
        {
            /* discard duplicate %%CollationBin if any*/
        }
        else if (uprv_strcmp(subtag, "Sequence") == 0)
        {
#if UCONFIG_NO_COLLATION || UCONFIG_NO_FILE_IO
            warning(line, "Not building collation elements because of UCONFIG_NO_COLLATION and/or UCONFIG_NO_FILE_IO, see uconfig.h");
#else
            if(gMakeBinaryCollation) {
                UErrorCode intStatus = U_ZERO_ERROR;

                /* do the collation elements */
                int32_t     len   = 0;
                uint8_t   *data  = NULL;
                UCollator *coll  = NULL;
                int32_t reorderCodes[USCRIPT_CODE_LIMIT + (UCOL_REORDER_CODE_LIMIT - UCOL_REORDER_CODE_FIRST)];
                uint32_t reorderCodeCount;
                int32_t reorderCodeIndex;
                UParseError parseError;

                genrbdata.inputDir = state->inputdir;
                genrbdata.outputDir = state->outputdir;

                coll = ucol_openRulesForImport(member->u.fString.fChars, member->u.fString.fLength,
                                               UCOL_OFF, UCOL_DEFAULT_STRENGTH,&parseError, importFromDataFile, &genrbdata, &intStatus);

                if (U_SUCCESS(intStatus) && coll != NULL)
                {
                    len = ucol_cloneBinary(coll, NULL, 0, &intStatus);
                    data = (uint8_t *)uprv_malloc(len);
                    intStatus = U_ZERO_ERROR;
                    len = ucol_cloneBinary(coll, data, len, &intStatus);
                    /*data = ucol_cloneRuleData(coll, &len, &intStatus);*/

                    /* tailoring rules version */
                    /* This is wrong! */
                    /*coll->dataInfo.dataVersion[1] = version[0];*/
                    /* Copy tailoring version. Builder version already */
                    /* set in ucol_openRules */
                    ((UCATableHeader *)data)->version[1] = version[0];
                    ((UCATableHeader *)data)->version[2] = version[1];
                    ((UCATableHeader *)data)->version[3] = version[2];

                    if (U_SUCCESS(intStatus) && data != NULL)
                    {
                        struct SResource *collationBin = bin_open(state->bundle, "%%CollationBin", len, data, NULL, NULL, status);
                        table_add(result, collationBin, line, status);
                        uprv_free(data);
                        
                        reorderCodeCount = ucol_getReorderCodes(
                            coll, reorderCodes, USCRIPT_CODE_LIMIT + (UCOL_REORDER_CODE_LIMIT - UCOL_REORDER_CODE_FIRST), &intStatus);
                        if (U_SUCCESS(intStatus) && reorderCodeCount > 0) {
                            struct SResource *reorderCodeRes = intvector_open(state->bundle, "%%ReorderCodes", NULL, status);
                            for (reorderCodeIndex = 0; reorderCodeIndex < reorderCodeCount; reorderCodeIndex++) {
                                intvector_add(reorderCodeRes, reorderCodes[reorderCodeIndex], status);
                            }
                            table_add(result, reorderCodeRes, line, status);
                        }
                    }
                    else
                    {
                        warning(line, "could not obtain rules from collator");
                        if(isStrict()){
                            *status = U_INVALID_FORMAT_ERROR;
                            return NULL;
                        }
                    }

                    ucol_close(coll);
                }
                else
                {
                    if(intStatus == U_FILE_ACCESS_ERROR) {
                      error(startline, "Collation could not be built- U_FILE_ACCESS_ERROR. Make sure ICU's data has been built and is loading properly.");
                      *status = intStatus;
                      return NULL;
                    }
                    warning(line, "%%Collation could not be constructed from CollationElements - check context!");
                    if(isStrict()){
                        *status = intStatus;
                        return NULL;
                    }
                }
            } else {
                if(isVerbose()) {
                    printf("Not building Collation binary\n");
                }
            }
#endif
            /* in order to achieve smaller data files, we can direct genrb */
            /* to omit collation rules */
            if(gOmitCollationRules) {
                bundle_closeString(state->bundle, member);
            } else {
                table_add(result, member, line, status);
            }
        }

        /*member = string_open(bundle, subtag, tokenValue->fChars, tokenValue->fLength, status);*/

        /*expect(TOK_CLOSE_BRACE, NULL, NULL, status);*/

        if (U_FAILURE(*status))
        {
            res_close(result);
            return NULL;
        }
    }

    /* not reached */
    /* A compiler warning will appear if all paths don't contain a return statement. */
/*    *status = U_INTERNAL_PROGRAM_ERROR;
    return NULL;*/
}

static struct SResource *
parseCollationElements(ParseState* state, char *tag, uint32_t startline, UBool newCollation, UErrorCode *status)
{
    struct SResource  *result = NULL;
    struct SResource  *member = NULL;
    struct SResource  *collationRes = NULL;
    struct UString    *tokenValue;
    struct UString     comment;
    enum   ETokenType  token;
    char               subtag[1024], typeKeyword[1024];
    uint32_t           line;

    result = table_open(state->bundle, tag, NULL, status);

    if (result == NULL || U_FAILURE(*status))
    {
        return NULL;
    }
    if(isVerbose()){
        printf(" collation elements %s at line %i \n",  (tag == NULL) ? "(null)" : tag, (int)startline);
    }
    if(!newCollation) {
        return addCollation(state, result, startline, status);
    }
    else {
        for(;;) {
            ustr_init(&comment);
            token = getToken(state, &tokenValue, &comment, &line, status);

            if (token == TOK_CLOSE_BRACE)
            {
                return result;
            }

            if (token != TOK_STRING)
            {
                res_close(result);
                *status = U_INVALID_FORMAT_ERROR;

                if (token == TOK_EOF)
                {
                    error(startline, "unterminated table");
                }
                else
                {
                    error(line, "Unexpected token %s", tokenNames[token]);
                }

                return NULL;
            }

            u_UCharsToChars(tokenValue->fChars, subtag, u_strlen(tokenValue->fChars) + 1);

            if (U_FAILURE(*status))
            {
                res_close(result);
                return NULL;
            }

            if (uprv_strcmp(subtag, "default") == 0)
            {
                member = parseResource(state, subtag, NULL, status);

                if (U_FAILURE(*status))
                {
                    res_close(result);
                    return NULL;
                }

                table_add(result, member, line, status);
            }
            else
            {
                token = peekToken(state, 0, &tokenValue, &line, &comment, status);
                /* this probably needs to be refactored or recursively use the parser */
                /* first we assume that our collation table won't have the explicit type */
                /* then, we cannot handle aliases */
                if(token == TOK_OPEN_BRACE) {
                    token = getToken(state, &tokenValue, &comment, &line, status);
                    collationRes = table_open(state->bundle, subtag, NULL, status);
                    collationRes = addCollation(state, collationRes, startline, status); /* need to parse the collation data regardless */
                    if (gIncludeUnihanColl || uprv_strcmp(subtag, "unihan") != 0) {
                        table_add(result, collationRes, startline, status);
                    }
                } else if(token == TOK_COLON) { /* right now, we'll just try to see if we have aliases */
                    /* we could have a table too */
                    token = peekToken(state, 1, &tokenValue, &line, &comment, status);
                    u_UCharsToChars(tokenValue->fChars, typeKeyword, u_strlen(tokenValue->fChars) + 1);
                    if(uprv_strcmp(typeKeyword, "alias") == 0) {
                        member = parseResource(state, subtag, NULL, status);

                        if (U_FAILURE(*status))
                        {
                            res_close(result);
                            return NULL;
                        }

                        table_add(result, member, line, status);
                    } else {
                        res_close(result);
                        *status = U_INVALID_FORMAT_ERROR;
                        return NULL;
                    }
                } else {
                    res_close(result);
                    *status = U_INVALID_FORMAT_ERROR;
                    return NULL;
                }
            }

            /*member = string_open(bundle, subtag, tokenValue->fChars, tokenValue->fLength, status);*/

            /*expect(TOK_CLOSE_BRACE, NULL, NULL, status);*/

            if (U_FAILURE(*status))
            {
                res_close(result);
                return NULL;
            }
        }
    }
}

/* Necessary, because CollationElements requires the bundle->fRoot member to be present which,
   if this weren't special-cased, wouldn't be set until the entire file had been processed. */
static struct SResource *
realParseTable(ParseState* state, struct SResource *table, char *tag, uint32_t startline, UErrorCode *status)
{
    struct SResource  *member = NULL;
    struct UString    *tokenValue=NULL;
    struct UString    comment;
    enum   ETokenType token;
    char              subtag[1024];
    uint32_t          line;
    UBool             readToken = FALSE;

    /* '{' . (name resource)* '}' */
    if(isVerbose()){
        printf(" parsing table %s at line %i \n", (tag == NULL) ? "(null)" : tag, (int)startline);
    }
    for (;;)
    {
        ustr_init(&comment);
        token = getToken(state, &tokenValue, &comment, &line, status);

        if (token == TOK_CLOSE_BRACE)
        {
            if (!readToken) {
                warning(startline, "Encountered empty table");
            }
            return table;
        }

        if (token != TOK_STRING)
        {
            *status = U_INVALID_FORMAT_ERROR;

            if (token == TOK_EOF)
            {
                error(startline, "unterminated table");
            }
            else
            {
                error(line, "unexpected token %s", tokenNames[token]);
            }

            return NULL;
        }

        if(uprv_isInvariantUString(tokenValue->fChars, -1)) {
            u_UCharsToChars(tokenValue->fChars, subtag, u_strlen(tokenValue->fChars) + 1);
        } else {
            *status = U_INVALID_FORMAT_ERROR;
            error(line, "invariant characters required for table keys");
            return NULL;
        }

        if (U_FAILURE(*status))
        {
            error(line, "parse error. Stopped parsing tokens with %s", u_errorName(*status));
            return NULL;
        }

        member = parseResource(state, subtag, &comment, status);

        if (member == NULL || U_FAILURE(*status))
        {
            error(line, "parse error. Stopped parsing resource with %s", u_errorName(*status));
            return NULL;
        }

        table_add(table, member, line, status);

        if (U_FAILURE(*status))
        {
            error(line, "parse error. Stopped parsing table with %s", u_errorName(*status));
            return NULL;
        }
        readToken = TRUE;
        ustr_deinit(&comment);
    }

    /* not reached */
    /* A compiler warning will appear if all paths don't contain a return statement. */
/*     *status = U_INTERNAL_PROGRAM_ERROR;
     return NULL;*/
}

static struct SResource *
parseTable(ParseState* state, char *tag, uint32_t startline, const struct UString *comment, UErrorCode *status)
{
    struct SResource *result;

    if (tag != NULL && uprv_strcmp(tag, "CollationElements") == 0)
    {
        return parseCollationElements(state, tag, startline, FALSE, status);
    }
    if (tag != NULL && uprv_strcmp(tag, "collations") == 0)
    {
        return parseCollationElements(state, tag, startline, TRUE, status);
    }
    if(isVerbose()){
        printf(" table %s at line %i \n",  (tag == NULL) ? "(null)" : tag, (int)startline);
    }

    result = table_open(state->bundle, tag, comment, status);

    if (result == NULL || U_FAILURE(*status))
    {
        return NULL;
    }

    return realParseTable(state, result, tag, startline,  status);
}

static struct SResource *
parseArray(ParseState* state, char *tag, uint32_t startline, const struct UString *comment, UErrorCode *status)
{
    struct SResource  *result = NULL;
    struct SResource  *member = NULL;
    struct UString    *tokenValue;
    struct UString    memberComments;
    enum   ETokenType token;
    UBool             readToken = FALSE;

    result = array_open(state->bundle, tag, comment, status);

    if (result == NULL || U_FAILURE(*status))
    {
        return NULL;
    }
    if(isVerbose()){
        printf(" array %s at line %i \n",  (tag == NULL) ? "(null)" : tag, (int)startline);
    }

    ustr_init(&memberComments);

    /* '{' . resource [','] '}' */
    for (;;)
    {
        /* reset length */
        ustr_setlen(&memberComments, 0, status);

        /* check for end of array, but don't consume next token unless it really is the end */
        token = peekToken(state, 0, &tokenValue, NULL, &memberComments, status);


        if (token == TOK_CLOSE_BRACE)
        {
            getToken(state, NULL, NULL, NULL, status);
            if (!readToken) {
                warning(startline, "Encountered empty array");
            }
            break;
        }

        if (token == TOK_EOF)
        {
            res_close(result);
            *status = U_INVALID_FORMAT_ERROR;
            error(startline, "unterminated array");
            return NULL;
        }

        /* string arrays are a special case */
        if (token == TOK_STRING)
        {
            getToken(state, &tokenValue, &memberComments, NULL, status);
            member = string_open(state->bundle, NULL, tokenValue->fChars, tokenValue->fLength, &memberComments, status);
        }
        else
        {
            member = parseResource(state, NULL, &memberComments, status);
        }

        if (member == NULL || U_FAILURE(*status))
        {
            res_close(result);
            return NULL;
        }

        array_add(result, member, status);

        if (U_FAILURE(*status))
        {
            res_close(result);
            return NULL;
        }

        /* eat optional comma if present */
        token = peekToken(state, 0, NULL, NULL, NULL, status);

        if (token == TOK_COMMA)
        {
            getToken(state, NULL, NULL, NULL, status);
        }

        if (U_FAILURE(*status))
        {
            res_close(result);
            return NULL;
        }
        readToken = TRUE;
    }

    ustr_deinit(&memberComments);
    return result;
}

static struct SResource *
parseIntVector(ParseState* state, char *tag, uint32_t startline, const struct UString *comment, UErrorCode *status)
{
    struct SResource  *result = NULL;
    enum   ETokenType  token;
    char              *string;
    int32_t            value;
    UBool              readToken = FALSE;
    char              *stopstring;
    uint32_t           len;
    struct UString     memberComments;

    result = intvector_open(state->bundle, tag, comment, status);

    if (result == NULL || U_FAILURE(*status))
    {
        return NULL;
    }

    if(isVerbose()){
        printf(" vector %s at line %i \n",  (tag == NULL) ? "(null)" : tag, (int)startline);
    }
    ustr_init(&memberComments);
    /* '{' . string [','] '}' */
    for (;;)
    {
        ustr_setlen(&memberComments, 0, status);

        /* check for end of array, but don't consume next token unless it really is the end */
        token = peekToken(state, 0, NULL, NULL,&memberComments, status);

        if (token == TOK_CLOSE_BRACE)
        {
            /* it's the end, consume the close brace */
            getToken(state, NULL, NULL, NULL, status);
            if (!readToken) {
                warning(startline, "Encountered empty int vector");
            }
            ustr_deinit(&memberComments);
            return result;
        }

        string = getInvariantString(state, NULL, NULL, status);

        if (U_FAILURE(*status))
        {
            res_close(result);
            return NULL;
        }

        /* For handling illegal char in the Intvector */
        value = uprv_strtoul(string, &stopstring, 0);/* make intvector support decimal,hexdigit,octal digit ranging from -2^31-2^32-1*/
        len=(uint32_t)(stopstring-string);

        if(len==uprv_strlen(string))
        {
            intvector_add(result, value, status);
            uprv_free(string);
            token = peekToken(state, 0, NULL, NULL, NULL, status);
        }
        else
        {
            uprv_free(string);
            *status=U_INVALID_CHAR_FOUND;
        }

        if (U_FAILURE(*status))
        {
            res_close(result);
            return NULL;
        }

        /* the comma is optional (even though it is required to prevent the reader from concatenating
        consecutive entries) so that a missing comma on the last entry isn't an error */
        if (token == TOK_COMMA)
        {
            getToken(state, NULL, NULL, NULL, status);
        }
        readToken = TRUE;
    }

    /* not reached */
    /* A compiler warning will appear if all paths don't contain a return statement. */
/*    intvector_close(result, status);
    *status = U_INTERNAL_PROGRAM_ERROR;
    return NULL;*/
}

static struct SResource *
parseBinary(ParseState* state, char *tag, uint32_t startline, const struct UString *comment, UErrorCode *status)
{
    struct SResource *result = NULL;
    uint8_t          *value;
    char             *string;
    char              toConv[3] = {'\0', '\0', '\0'};
    uint32_t          count;
    uint32_t          i;
    uint32_t          line;
    char             *stopstring;
    uint32_t          len;

    string = getInvariantString(state, &line, NULL, status);

    if (string == NULL || U_FAILURE(*status))
    {
        return NULL;
    }

    expect(state, TOK_CLOSE_BRACE, NULL, NULL, NULL, status);

    if (U_FAILURE(*status))
    {
        uprv_free(string);
        return NULL;
    }

    if(isVerbose()){
        printf(" binary %s at line %i \n",  (tag == NULL) ? "(null)" : tag, (int)startline);
    }

    count = (uint32_t)uprv_strlen(string);
    if (count > 0){
        if((count % 2)==0){
            value = uprv_malloc(sizeof(uint8_t) * count);

            if (value == NULL)
            {
                uprv_free(string);
                *status = U_MEMORY_ALLOCATION_ERROR;
                return NULL;
            }

            for (i = 0; i < count; i += 2)
            {
                toConv[0] = string[i];
                toConv[1] = string[i + 1];

                value[i >> 1] = (uint8_t) uprv_strtoul(toConv, &stopstring, 16);
                len=(uint32_t)(stopstring-toConv);

                if(len!=uprv_strlen(toConv))
                {
                    uprv_free(string);
                    *status=U_INVALID_CHAR_FOUND;
                    return NULL;
                }
            }

            result = bin_open(state->bundle, tag, (i >> 1), value,NULL, comment, status);

            uprv_free(value);
        }
        else
        {
            *status = U_INVALID_CHAR_FOUND;
            uprv_free(string);
            error(line, "Encountered invalid binary string");
            return NULL;
        }
    }
    else
    {
        result = bin_open(state->bundle, tag, 0, NULL, "",comment,status);
        warning(startline, "Encountered empty binary tag");
    }
    uprv_free(string);

    return result;
}

static struct SResource *
parseInteger(ParseState* state, char *tag, uint32_t startline, const struct UString *comment, UErrorCode *status)
{
    struct SResource *result = NULL;
    int32_t           value;
    char             *string;
    char             *stopstring;
    uint32_t          len;

    string = getInvariantString(state, NULL, NULL, status);

    if (string == NULL || U_FAILURE(*status))
    {
        return NULL;
    }

    expect(state, TOK_CLOSE_BRACE, NULL, NULL, NULL, status);

    if (U_FAILURE(*status))
    {
        uprv_free(string);
        return NULL;
    }

    if(isVerbose()){
        printf(" integer %s at line %i \n",  (tag == NULL) ? "(null)" : tag, (int)startline);
    }

    if (uprv_strlen(string) <= 0)
    {
        warning(startline, "Encountered empty integer. Default value is 0.");
    }

    /* Allow integer support for hexdecimal, octal digit and decimal*/
    /* and handle illegal char in the integer*/
    value = uprv_strtoul(string, &stopstring, 0);
    len=(uint32_t)(stopstring-string);
    if(len==uprv_strlen(string))
    {
        result = int_open(state->bundle, tag, value, comment, status);
    }
    else
    {
        *status=U_INVALID_CHAR_FOUND;
    }
    uprv_free(string);

    return result;
}

static struct SResource *
parseImport(ParseState* state, char *tag, uint32_t startline, const struct UString* comment, UErrorCode *status)
{
    struct SResource *result;
    FileStream       *file;
    int32_t           len;
    uint8_t          *data;
    char             *filename;
    uint32_t          line;
    char     *fullname = NULL;
    int32_t numRead = 0;
    filename = getInvariantString(state, &line, NULL, status);

    if (U_FAILURE(*status))
    {
        return NULL;
    }

    expect(state, TOK_CLOSE_BRACE, NULL, NULL, NULL, status);

    if (U_FAILURE(*status))
    {
        uprv_free(filename);
        return NULL;
    }

    if(isVerbose()){
        printf(" import %s at line %i \n",  (tag == NULL) ? "(null)" : tag, (int)startline);
    }

    /* Open the input file for reading */
    if (state->inputdir == NULL)
    {
#if 1
        /* 
         * Always save file file name, even if there's
         * no input directory specified. MIGHT BREAK SOMETHING
         */
        int32_t filenameLength = uprv_strlen(filename);

        fullname = (char *) uprv_malloc(filenameLength + 1);
        uprv_strcpy(fullname, filename);
#endif

        file = T_FileStream_open(filename, "rb");
    }
    else
    {

        int32_t  count     = (int32_t)uprv_strlen(filename);

        if (state->inputdir[state->inputdirLength - 1] != U_FILE_SEP_CHAR)
        {
            fullname = (char *) uprv_malloc(state->inputdirLength + count + 2);

            /* test for NULL */
            if(fullname == NULL)
            {
                *status = U_MEMORY_ALLOCATION_ERROR;
                return NULL;
            }

            uprv_strcpy(fullname, state->inputdir);

            fullname[state->inputdirLength]      = U_FILE_SEP_CHAR;
            fullname[state->inputdirLength + 1] = '\0';

            uprv_strcat(fullname, filename);
        }
        else
        {
            fullname = (char *) uprv_malloc(state->inputdirLength + count + 1);

            /* test for NULL */
            if(fullname == NULL)
            {
                *status = U_MEMORY_ALLOCATION_ERROR;
                return NULL;
            }

            uprv_strcpy(fullname, state->inputdir);
            uprv_strcat(fullname, filename);
        }

        file = T_FileStream_open(fullname, "rb");

    }

    if (file == NULL)
    {
        error(line, "couldn't open input file %s", filename);
        *status = U_FILE_ACCESS_ERROR;
        return NULL;
    }

    len  = T_FileStream_size(file);
    data = (uint8_t*)uprv_malloc(len * sizeof(uint8_t));
    /* test for NULL */
    if(data == NULL)
    {
        *status = U_MEMORY_ALLOCATION_ERROR;
        T_FileStream_close (file);
        return NULL;
    }

    numRead = T_FileStream_read  (file, data, len);
    T_FileStream_close (file);

    result = bin_open(state->bundle, tag, len, data, fullname, comment, status);

    uprv_free(data);
    uprv_free(filename);
    uprv_free(fullname);

    return result;
}

static struct SResource *
parseInclude(ParseState* state, char *tag, uint32_t startline, const struct UString* comment, UErrorCode *status)
{
    struct SResource *result;
    int32_t           len=0;
    char             *filename;
    uint32_t          line;
    UChar *pTarget     = NULL;

    UCHARBUF *ucbuf;
    char     *fullname = NULL;
    int32_t  count     = 0;
    const char* cp = NULL;
    const UChar* uBuffer = NULL;

    filename = getInvariantString(state, &line, NULL, status);
    count     = (int32_t)uprv_strlen(filename);

    if (U_FAILURE(*status))
    {
        return NULL;
    }

    expect(state, TOK_CLOSE_BRACE, NULL, NULL, NULL, status);

    if (U_FAILURE(*status))
    {
        uprv_free(filename);
        return NULL;
    }

    if(isVerbose()){
        printf(" include %s at line %i \n",  (tag == NULL) ? "(null)" : tag, (int)startline);
    }

    fullname = (char *) uprv_malloc(state->inputdirLength + count + 2);
    /* test for NULL */
    if(fullname == NULL)
    {
        *status = U_MEMORY_ALLOCATION_ERROR;
        uprv_free(filename);
        return NULL;
    }

    if(state->inputdir!=NULL){
        if (state->inputdir[state->inputdirLength - 1] != U_FILE_SEP_CHAR)
        {

            uprv_strcpy(fullname, state->inputdir);

            fullname[state->inputdirLength]      = U_FILE_SEP_CHAR;
            fullname[state->inputdirLength + 1] = '\0';

            uprv_strcat(fullname, filename);
        }
        else
        {
            uprv_strcpy(fullname, state->inputdir);
            uprv_strcat(fullname, filename);
        }
    }else{
        uprv_strcpy(fullname,filename);
    }

    ucbuf = ucbuf_open(fullname, &cp,getShowWarning(),FALSE,status);

    if (U_FAILURE(*status)) {
        error(line, "couldn't open input file %s\n", filename);
        return NULL;
    }

    uBuffer = ucbuf_getBuffer(ucbuf,&len,status);
    result = string_open(state->bundle, tag, uBuffer, len, comment, status);

    uprv_free(pTarget);

    uprv_free(filename);
    uprv_free(fullname);

    return result;
}





U_STRING_DECL(k_type_string,    "string",    6);
U_STRING_DECL(k_type_binary,    "binary",    6);
U_STRING_DECL(k_type_bin,       "bin",       3);
U_STRING_DECL(k_type_table,     "table",     5);
U_STRING_DECL(k_type_table_no_fallback,     "table(nofallback)",         17);
U_STRING_DECL(k_type_int,       "int",       3);
U_STRING_DECL(k_type_integer,   "integer",   7);
U_STRING_DECL(k_type_array,     "array",     5);
U_STRING_DECL(k_type_alias,     "alias",     5);
U_STRING_DECL(k_type_intvector, "intvector", 9);
U_STRING_DECL(k_type_import,    "import",    6);
U_STRING_DECL(k_type_include,   "include",   7);
U_STRING_DECL(k_type_reserved,  "reserved",  8);

/* Various non-standard processing plugins that create one or more special resources. */
U_STRING_DECL(k_type_plugin_uca_rules,      "process(uca_rules)",        18);
U_STRING_DECL(k_type_plugin_collation,      "process(collation)",        18);
U_STRING_DECL(k_type_plugin_transliterator, "process(transliterator)",   23);
U_STRING_DECL(k_type_plugin_dependency,     "process(dependency)",       19);

typedef enum EResourceType
{
    RT_UNKNOWN,
    RT_STRING,
    RT_BINARY,
    RT_TABLE,
    RT_TABLE_NO_FALLBACK,
    RT_INTEGER,
    RT_ARRAY,
    RT_ALIAS,
    RT_INTVECTOR,
    RT_IMPORT,
    RT_INCLUDE,
    RT_PROCESS_UCA_RULES,
    RT_PROCESS_COLLATION,
    RT_PROCESS_TRANSLITERATOR,
    RT_PROCESS_DEPENDENCY,
    RT_RESERVED
} EResourceType;

static struct {
    const char *nameChars;   /* only used for debugging */
    const UChar *nameUChars;
    ParseResourceFunction *parseFunction;
} gResourceTypes[] = {
    {"Unknown", NULL, NULL},
    {"string", k_type_string, parseString},
    {"binary", k_type_binary, parseBinary},
    {"table", k_type_table, parseTable},
    {"table(nofallback)", k_type_table_no_fallback, NULL}, /* parseFunction will never be called */
    {"integer", k_type_integer, parseInteger},
    {"array", k_type_array, parseArray},
    {"alias", k_type_alias, parseAlias},
    {"intvector", k_type_intvector, parseIntVector},
    {"import", k_type_import, parseImport},
    {"include", k_type_include, parseInclude},
    {"process(uca_rules)", k_type_plugin_uca_rules, parseUCARules},
    {"process(collation)", k_type_plugin_collation, NULL /* not implemented yet */},
    {"process(transliterator)", k_type_plugin_transliterator, parseTransliterator},
    {"process(dependency)", k_type_plugin_dependency, parseDependency},
    {"reserved", NULL, NULL}
};

void initParser(UBool omitBinaryCollation, UBool omitCollationRules)
{
    U_STRING_INIT(k_type_string,    "string",    6);
    U_STRING_INIT(k_type_binary,    "binary",    6);
    U_STRING_INIT(k_type_bin,       "bin",       3);
    U_STRING_INIT(k_type_table,     "table",     5);
    U_STRING_INIT(k_type_table_no_fallback,     "table(nofallback)",         17);
    U_STRING_INIT(k_type_int,       "int",       3);
    U_STRING_INIT(k_type_integer,   "integer",   7);
    U_STRING_INIT(k_type_array,     "array",     5);
    U_STRING_INIT(k_type_alias,     "alias",     5);
    U_STRING_INIT(k_type_intvector, "intvector", 9);
    U_STRING_INIT(k_type_import,    "import",    6);
    U_STRING_INIT(k_type_reserved,  "reserved",  8);
    U_STRING_INIT(k_type_include,   "include",   7);

    U_STRING_INIT(k_type_plugin_uca_rules,      "process(uca_rules)",        18);
    U_STRING_INIT(k_type_plugin_collation,      "process(collation)",        18);
    U_STRING_INIT(k_type_plugin_transliterator, "process(transliterator)",   23);
    U_STRING_INIT(k_type_plugin_dependency,     "process(dependency)",       19);

    gMakeBinaryCollation = !omitBinaryCollation;
    gOmitCollationRules = omitCollationRules;
}

static U_INLINE UBool isTable(enum EResourceType type) {
    return (UBool)(type==RT_TABLE || type==RT_TABLE_NO_FALLBACK);
}

static enum EResourceType
parseResourceType(ParseState* state, UErrorCode *status)
{
    struct UString        *tokenValue;
    struct UString        comment;
    enum   EResourceType  result = RT_UNKNOWN;
    uint32_t              line=0;
    ustr_init(&comment);
    expect(state, TOK_STRING, &tokenValue, &comment, &line, status);

    if (U_FAILURE(*status))
    {
        return RT_UNKNOWN;
    }

    *status = U_ZERO_ERROR;

    /* Search for normal types */
    result=RT_UNKNOWN;
    while (++result < RT_RESERVED) {
        if (u_strcmp(tokenValue->fChars, gResourceTypes[result].nameUChars) == 0) {
            break;
        }
    }
    /* Now search for the aliases */
    if (u_strcmp(tokenValue->fChars, k_type_int) == 0) {
        result = RT_INTEGER;
    }
    else if (u_strcmp(tokenValue->fChars, k_type_bin) == 0) {
        result = RT_BINARY;
    }
    else if (result == RT_RESERVED) {
        char tokenBuffer[1024];
        u_austrncpy(tokenBuffer, tokenValue->fChars, sizeof(tokenBuffer));
        tokenBuffer[sizeof(tokenBuffer) - 1] = 0;
        *status = U_INVALID_FORMAT_ERROR;
        error(line, "unknown resource type '%s'", tokenBuffer);
    }

    return result;
}

/* parse a non-top-level resource */
static struct SResource *
parseResource(ParseState* state, char *tag, const struct UString *comment, UErrorCode *status)
{
    enum   ETokenType      token;
    enum   EResourceType  resType = RT_UNKNOWN;
    ParseResourceFunction *parseFunction = NULL;
    struct UString        *tokenValue;
    uint32_t                 startline;
    uint32_t                 line;

    token = getToken(state, &tokenValue, NULL, &startline, status);

    if(isVerbose()){
        printf(" resource %s at line %i \n",  (tag == NULL) ? "(null)" : tag, (int)startline);
    }

    /* name . [ ':' type ] '{' resource '}' */
    /* This function parses from the colon onwards.  If the colon is present, parse the
    type then try to parse a resource of that type.  If there is no explicit type,
    work it out using the lookahead tokens. */
    switch (token)
    {
    case TOK_EOF:
        *status = U_INVALID_FORMAT_ERROR;
        error(startline, "Unexpected EOF encountered");
        return NULL;

    case TOK_ERROR:
        *status = U_INVALID_FORMAT_ERROR;
        return NULL;

    case TOK_COLON:
        resType = parseResourceType(state, status);
        expect(state, TOK_OPEN_BRACE, &tokenValue, NULL, &startline, status);

        if (U_FAILURE(*status))
        {
            return NULL;
        }

        break;

    case TOK_OPEN_BRACE:
        break;

    default:
        *status = U_INVALID_FORMAT_ERROR;
        error(startline, "syntax error while reading a resource, expected '{' or ':'");
        return NULL;
    }

    if (resType == RT_UNKNOWN)
    {
        /* No explicit type, so try to work it out.  At this point, we've read the first '{'.
        We could have any of the following:
        { {         => array (nested)
        { :/}       => array
        { string ,  => string array

        { string {  => table

        { string :/{    => table
        { string }      => string
        */

        token = peekToken(state, 0, NULL, &line, NULL,status);

        if (U_FAILURE(*status))
        {
            return NULL;
        }

        if (token == TOK_OPEN_BRACE || token == TOK_COLON ||token ==TOK_CLOSE_BRACE )
        {
            resType = RT_ARRAY;
        }
        else if (token == TOK_STRING)
        {
            token = peekToken(state, 1, NULL, &line, NULL, status);

            if (U_FAILURE(*status))
            {
                return NULL;
            }

            switch (token)
            {
            case TOK_COMMA:         resType = RT_ARRAY;  break;
            case TOK_OPEN_BRACE:    resType = RT_TABLE;  break;
            case TOK_CLOSE_BRACE:   resType = RT_STRING; break;
            case TOK_COLON:         resType = RT_TABLE;  break;
            default:
                *status = U_INVALID_FORMAT_ERROR;
                error(line, "Unexpected token after string, expected ',', '{' or '}'");
                return NULL;
            }
        }
        else
        {
            *status = U_INVALID_FORMAT_ERROR;
            error(line, "Unexpected token after '{'");
            return NULL;
        }

        /* printf("Type guessed as %s\n", resourceNames[resType]); */
    } else if(resType == RT_TABLE_NO_FALLBACK) {
        *status = U_INVALID_FORMAT_ERROR;
        error(startline, "error: %s resource type not valid except on top bundle level", gResourceTypes[resType].nameChars);
        return NULL;
    }

    /* We should now know what we need to parse next, so call the appropriate parser
    function and return. */
    parseFunction = gResourceTypes[resType].parseFunction;
    if (parseFunction != NULL) {
        return parseFunction(state, tag, startline, comment, status);
    }
    else {
        *status = U_INTERNAL_PROGRAM_ERROR;
        error(startline, "internal error: %s resource type found and not handled", gResourceTypes[resType].nameChars);
    }

    return NULL;
}

/* parse the top-level resource */
struct SRBRoot *
parse(UCHARBUF *buf, const char *inputDir, const char *outputDir, UErrorCode *status)
{
    struct UString    *tokenValue;
    struct UString    comment;
    uint32_t           line;
    enum EResourceType bundleType;
    enum ETokenType    token;
    ParseState state;
    uint32_t i;
    int encLength;
    char* enc;
    for (i = 0; i < MAX_LOOKAHEAD + 1; i++)
    {
        ustr_init(&state.lookahead[i].value);
        ustr_init(&state.lookahead[i].comment);
    }

    initLookahead(&state, buf, status);

    state.inputdir       = inputDir;
    state.inputdirLength = (state.inputdir != NULL) ? (uint32_t)uprv_strlen(state.inputdir) : 0;
    state.outputdir       = outputDir;
    state.outputdirLength = (state.outputdir != NULL) ? (uint32_t)uprv_strlen(state.outputdir) : 0;

    ustr_init(&comment);
    expect(&state, TOK_STRING, &tokenValue, &comment, NULL, status);

    state.bundle = bundle_open(&comment, FALSE, status);

    if (state.bundle == NULL || U_FAILURE(*status))
    {
        return NULL;
    }


    bundle_setlocale(state.bundle, tokenValue->fChars, status);

    /* The following code is to make Empty bundle work no matter with :table specifer or not */
    token = getToken(&state, NULL, NULL, &line, status);
    if(token==TOK_COLON) {
        *status=U_ZERO_ERROR;
        bundleType=parseResourceType(&state, status);

        if(isTable(bundleType))
        {
            expect(&state, TOK_OPEN_BRACE, NULL, NULL, &line, status);
        }
        else
        {
            *status=U_PARSE_ERROR;
            /* printf("asdsdweqdasdad\n"); */

            error(line, "parse error. Stopped parsing with %s", u_errorName(*status));
        }
    }
    else
    {
        /* not a colon */
        if(token==TOK_OPEN_BRACE)
        {
            *status=U_ZERO_ERROR;
            bundleType=RT_TABLE;
        }
        else
        {
            /* neither colon nor open brace */
            *status=U_PARSE_ERROR;
            bundleType=RT_UNKNOWN;
            error(line, "parse error, did not find open-brace '{' or colon ':', stopped with %s", u_errorName(*status));
        }
    }

    if (U_FAILURE(*status))
    {
        bundle_close(state.bundle, status);
        return NULL;
    }

    if(bundleType==RT_TABLE_NO_FALLBACK) {
        /*
         * Parse a top-level table with the table(nofallback) declaration.
         * This is the same as a regular table, but also sets the
         * URES_ATT_NO_FALLBACK flag in indexes[URES_INDEX_ATTRIBUTES] .
         */
        state.bundle->noFallback=TRUE;
    }
    /* top-level tables need not handle special table names like "collations" */
    realParseTable(&state, state.bundle->fRoot, NULL, line, status);

    if(dependencyArray!=NULL){
        table_add(state.bundle->fRoot, dependencyArray, 0, status);
        dependencyArray = NULL;
    }
    if (U_FAILURE(*status))
    {
        bundle_close(state.bundle, status);
        res_close(dependencyArray);
        return NULL;
    }

    if (getToken(&state, NULL, NULL, &line, status) != TOK_EOF)
    {
        warning(line, "extraneous text after resource bundle (perhaps unmatched braces)");
        if(isStrict()){
            *status = U_INVALID_FORMAT_ERROR;
            return NULL;
        }
    }

    cleanupLookahead(&state);
    ustr_deinit(&comment);
    return state.bundle;
}
