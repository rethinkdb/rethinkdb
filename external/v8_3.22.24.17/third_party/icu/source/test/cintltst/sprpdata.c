/*
 *******************************************************************************
 *
 *   Copyright (C) 2003-2008, International Business Machines
 *   Corporation and others.  All Rights Reserved.
 *
 *******************************************************************************
 *   file name:  spreptst.c
 *   encoding:   US-ASCII
 *   tab size:   8 (not used)
 *   indentation:4
 *
 *   created on: 2003jul11
 *   created by: Ram Viswanadha
 */
#define USPREP_TYPE_NAMES_ARRAY

#include "unicode/utypes.h"

#if !UCONFIG_NO_IDNA

#include "unicode/ustring.h"
#include "unicode/putil.h"
#include "cintltst.h"
#include "unicode/usprep.h"
#include "sprpimpl.h"
#include "uparse.h"
#include "cmemory.h"
#include "ustr_imp.h"
#include "cstring.h"

static void
parseMappings(const char *filename, UStringPrepProfile* data, UBool reportError, UErrorCode *pErrorCode);

static void
compareMapping(UStringPrepProfile* data, uint32_t codepoint, uint32_t* mapping, int32_t mapLength,
               UStringPrepType option);

static void
compareFlagsForRange(UStringPrepProfile* data, uint32_t start, uint32_t end,UStringPrepType option);

void
doStringPrepTest(const char* binFileName, const char* txtFileName, int32_t options, UErrorCode* errorCode);

static void U_CALLCONV
strprepProfileLineFn(void *context,
              char *fields[][2], int32_t fieldCount,
              UErrorCode *pErrorCode) {
    uint32_t mapping[40];
    char *end, *map;
    uint32_t code;
    int32_t length;
    UStringPrepProfile* data = (UStringPrepProfile*) context;
    const char* typeName;
    uint32_t rangeStart=0,rangeEnd =0;

    typeName = fields[2][0];
    map = fields[1][0];

    if(strstr(typeName, usprepTypeNames[USPREP_UNASSIGNED])!=NULL){

        u_parseCodePointRange(fields[0][0], &rangeStart,&rangeEnd, pErrorCode);

        /* store the range */
        compareFlagsForRange(data, rangeStart,rangeEnd,USPREP_UNASSIGNED);

    }else if(strstr(typeName, usprepTypeNames[USPREP_PROHIBITED])!=NULL){

        u_parseCodePointRange(fields[0][0], &rangeStart,&rangeEnd, pErrorCode);

        /* store the range */
        compareFlagsForRange(data, rangeStart,rangeEnd,USPREP_PROHIBITED);

    }else if(strstr(typeName, usprepTypeNames[USPREP_MAP])!=NULL){
        /* get the character code, field 0 */
        code=(uint32_t)uprv_strtoul(fields[0][0], &end, 16);

        /* parse the mapping string */
        length=u_parseCodePoints(map, mapping, sizeof(mapping)/4, pErrorCode);

        /* compare the mapping */
        compareMapping(data, code,mapping, length,USPREP_MAP);
    }else{
        *pErrorCode = U_INVALID_FORMAT_ERROR;
    }

}



static void
parseMappings(const char *filename, UStringPrepProfile* data, UBool reportError, UErrorCode *pErrorCode) {
    char *fields[3][2];

    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return;
    }

    u_parseDelimitedFile(filename, ';', fields, 3, strprepProfileLineFn, (void*)data, pErrorCode);

    /*fprintf(stdout,"Number of code points that have mappings with length >1 : %i\n",len);*/

    if(U_FAILURE(*pErrorCode) && (reportError || *pErrorCode!=U_FILE_ACCESS_ERROR)) {
        log_err( "testidn error: u_parseDelimitedFile(\"%s\") failed - %s\n", filename, u_errorName(*pErrorCode));
    }
}


static UStringPrepType
getValues(uint32_t result, int32_t* value, UBool* isIndex){

    UStringPrepType type;
    if(result == 0){
        /*
         * Initial value stored in the mapping table
         * just return USPREP_TYPE_LIMIT .. so that
         * the source codepoint is copied to the destination
         */
        type = USPREP_TYPE_LIMIT;
    }else if(result >= _SPREP_TYPE_THRESHOLD){
        type = (UStringPrepType) (result - _SPREP_TYPE_THRESHOLD);
    }else{
        /* get the type */
        type = USPREP_MAP;
        /* ascertain if the value is index or delta */
        if(result & 0x02){
            *isIndex = TRUE;
            *value = result  >> 2;

        }else{
            *isIndex = FALSE;
            *value = (int16_t)result;
            *value =  (*value >> 2);

        }
        if((result>>2) == _SPREP_MAX_INDEX_VALUE){
            type = USPREP_DELETE;
            isIndex =FALSE;
            value = 0;
        }
    }
    return type;
}

static void
compareMapping(UStringPrepProfile* data, uint32_t codepoint, uint32_t* mapping,int32_t mapLength,
               UStringPrepType type){
    uint32_t result = 0;
    int32_t length=0;
    UBool isIndex = FALSE;
    UStringPrepType retType;
    int32_t value=0, index=0, delta=0;
    int32_t* indexes = data->indexes;
    UTrie trie = data->sprepTrie;
    const uint16_t* mappingData = data->mappingData;
    int32_t realLength =0;
    int32_t j=0;
    int8_t i=0;

    UTRIE_GET16(&trie, codepoint, result);
    retType = getValues(result,&value,&isIndex);


    if(type != retType && retType != USPREP_DELETE){

        log_err( "Did not get the assigned type for codepoint 0x%08X. Expected: %i Got: %i\n",codepoint, USPREP_MAP, type);

    }

    if(isIndex){
        index = value;
        if(index >= indexes[_SPREP_ONE_UCHAR_MAPPING_INDEX_START] &&
                 index < indexes[_SPREP_TWO_UCHARS_MAPPING_INDEX_START]){
            length = 1;
        }else if(index >= indexes[_SPREP_TWO_UCHARS_MAPPING_INDEX_START] &&
                 index < indexes[_SPREP_THREE_UCHARS_MAPPING_INDEX_START]){
            length = 2;
        }else if(index >= indexes[_SPREP_THREE_UCHARS_MAPPING_INDEX_START] &&
                 index < indexes[_SPREP_FOUR_UCHARS_MAPPING_INDEX_START]){
            length = 3;
        }else{
            length = mappingData[index++];
        }
    }else{
        delta = value;
        length = (retType == USPREP_DELETE)? 0 :  1;
    }

    /* figure out the real length */
    for(j=0; j<mapLength; j++){
        if(mapping[j] > 0xFFFF){
            realLength +=2;
        }else{
            realLength++;
        }
    }

    if(realLength != length){
        log_err( "Did not get the expected length. Expected: %i Got: %i\n", mapLength, length);
    }

    if(isIndex){
        for(i =0; i< mapLength; i++){
            if(mapping[i] <= 0xFFFF){
                if(mappingData[index+i] != (uint16_t)mapping[i]){
                    log_err("Did not get the expected result. Expected: 0x%04X Got: 0x%04X \n", mapping[i], mappingData[index+i]);
                }
            }else{
                UChar lead  = UTF16_LEAD(mapping[i]);
                UChar trail = UTF16_TRAIL(mapping[i]);
                if(mappingData[index+i] != lead ||
                    mappingData[index+i+1] != trail){
                    log_err( "Did not get the expected result. Expected: 0x%04X 0x%04X  Got: 0x%04X 0x%04X\n", lead, trail, mappingData[index+i], mappingData[index+i+1]);
                }
            }
        }
    }else{
        if(retType!=USPREP_DELETE && (codepoint-delta) != (uint16_t)mapping[0]){
           log_err("Did not get the expected result. Expected: 0x%04X Got: 0x%04X \n", mapping[0],(codepoint-delta));
        }
    }

}

static void
compareFlagsForRange(UStringPrepProfile* data,
                     uint32_t start, uint32_t end,
                     UStringPrepType type){

    uint32_t result =0 ;
    UStringPrepType retType;
    UBool isIndex=FALSE;
    int32_t value=0;
    UTrie trie = data->sprepTrie;
/*
    // supplementary code point
    UChar __lead16=UTF16_LEAD(0x2323E);
    int32_t __offset;

    // get data for lead surrogate
    (result)=_UTRIE_GET_RAW((&idnTrie), index, 0, (__lead16));
    __offset=(&idnTrie)->getFoldingOffset(result);

    // get the real data from the folded lead/trail units
    if(__offset>0) {
        (result)=_UTRIE_GET_RAW((&idnTrie), index, __offset, (0x2323E)&0x3ff);
    } else {
        (result)=(uint32_t)((&idnTrie)->initialValue);
    }

    UTRIE_GET16(&idnTrie,0x2323E, result);
*/
    while(start < end+1){
        UTRIE_GET16(&trie,start, result);
        retType = getValues(result, &value, &isIndex);
        if(result > _SPREP_TYPE_THRESHOLD){
            if(retType != type){
                log_err( "FAIL: Did not get the expected type for 0x%06X. Expected: %s Got: %s\n",start,usprepTypeNames[type], usprepTypeNames[retType]);
            }
        }else{
            if(type == USPREP_PROHIBITED && ((result & 0x01) != 0x01)){
                log_err( "FAIL: Did not get the expected type for 0x%06X. Expected: %s Got: %s\n",start,usprepTypeNames[type], usprepTypeNames[retType]);
            }
        }

        start++;
    }

}

void
doStringPrepTest(const char* binFileName, const char* txtFileName, int32_t options, UErrorCode* errorCode){

    const char *testdatapath = loadTestData(errorCode);
    const char *srcdatapath = NULL;
    const char *relativepath = NULL;
    char *filename = NULL;
    UStringPrepProfile* profile = NULL;

#ifdef U_TOPSRCDIR
    srcdatapath = U_TOPSRCDIR;
    relativepath = U_FILE_SEP_STRING"test"U_FILE_SEP_STRING"testdata"U_FILE_SEP_STRING;
#else
    srcdatapath = ctest_dataOutDir();
    relativepath = ".."U_FILE_SEP_STRING".."U_FILE_SEP_STRING"test"U_FILE_SEP_STRING"testdata"U_FILE_SEP_STRING;
#endif

    filename = (char*) malloc(strlen(srcdatapath)+strlen(relativepath)+strlen(txtFileName)+10 );
    profile = usprep_open(testdatapath, binFileName, errorCode);

    if(*errorCode == U_FILE_ACCESS_ERROR) {
        log_data_err("Failed to load %s data file. Error: %s \n", binFileName, u_errorName(*errorCode));
        return;
    } else if(U_FAILURE(*errorCode)){
        log_err("Failed to load %s data file. Error: %s \n", binFileName, u_errorName(*errorCode));
        return;
    }
    /* open and load the txt file */
    strcpy(filename,srcdatapath);
    strcat(filename,relativepath);
    strcat(filename,txtFileName);

    parseMappings(filename,profile, TRUE,errorCode);

    free(filename);
}
#endif
/*
 * Hey, Emacs, please set the following:
 *
 * Local Variables:
 * indent-tabs-mode: nil
 * End:
 *
 */
