/*
**********************************************************************
*   Copyright (C) 2002-2010, International Business Machines
*   Corporation and others.  All Rights Reserved.
**********************************************************************
*
* File genctd.c
*/

//--------------------------------------------------------------------
//
//   Tool for generating CompactTrieDictionary data files (.ctd files).
//
//   Usage:  genctd [options] -o output-file.ctd input-file
//
//       options:   -v         verbose
//                  -? or -h   help
//
//   The input  file is a plain text file containing words, one per line.
//    Words end at the first whitespace; lines beginning with whitespace
//    are ignored.
//    The file can be encoded as utf-8, or utf-16 (either endian), or
//    in the default code page (platform dependent.).  utf encoded
//    files must include a BOM.
//
//--------------------------------------------------------------------

#include "unicode/utypes.h"
#include "unicode/uchar.h"
#include "unicode/ucnv.h"
#include "unicode/uniset.h"
#include "unicode/unistr.h"
#include "unicode/uclean.h"
#include "unicode/udata.h"
#include "unicode/putil.h"

//#include "unicode/ustdio.h"

#include "uoptions.h"
#include "unewdata.h"
#include "ucmndata.h"
#include "rbbidata.h"
#include "triedict.h"
#include "cmemory.h"
#include "uassert.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

U_NAMESPACE_USE

static char *progName;
static UOption options[]={
    UOPTION_HELP_H,             /* 0 */
    UOPTION_HELP_QUESTION_MARK, /* 1 */
    UOPTION_VERBOSE,            /* 2 */
    { "out",   NULL, NULL, NULL, 'o', UOPT_REQUIRES_ARG, 0 },   /* 3 */
    UOPTION_ICUDATADIR,         /* 4 */
    UOPTION_DESTDIR,            /* 5 */
    UOPTION_COPYRIGHT,          /* 6 */
};

void usageAndDie(int retCode) {
        printf("Usage: %s [-v] [-options] -o output-file dictionary-file\n", progName);
        printf("\tRead in word list and write out compact trie dictionary\n"
            "options:\n"
            "\t-h or -? or --help  this usage text\n"
            "\t-V or --version     show a version message\n"
            "\t-c or --copyright   include a copyright notice\n"
            "\t-v or --verbose     turn on verbose output\n"
            "\t-i or --icudatadir  directory for locating any needed intermediate data files,\n"
            "\t                    followed by path, defaults to %s\n"
            "\t-d or --destdir     destination directory, followed by the path\n",
            u_getDataDirectory());
        exit (retCode);
}


#if UCONFIG_NO_BREAK_ITERATION || UCONFIG_NO_FILE_IO

/* dummy UDataInfo cf. udata.h */
static UDataInfo dummyDataInfo = {
    sizeof(UDataInfo),
    0,

    U_IS_BIG_ENDIAN,
    U_CHARSET_FAMILY,
    U_SIZEOF_UCHAR,
    0,

    { 0, 0, 0, 0 },                 /* dummy dataFormat */
    { 0, 0, 0, 0 },                 /* dummy formatVersion */
    { 0, 0, 0, 0 }                  /* dummy dataVersion */
};

#else

//
//  Set up the ICU data header, defined in ucmndata.h
//
DataHeader dh ={
    {sizeof(DataHeader),           // Struct MappedData
        0xda,
        0x27},

    {                               // struct UDataInfo
        sizeof(UDataInfo),          //     size
        0,                          //     reserved
        U_IS_BIG_ENDIAN,
        U_CHARSET_FAMILY,
        U_SIZEOF_UCHAR,
        0,                          //     reserved

    { 0x54, 0x72, 0x44, 0x63 },     // "TrDc" Trie Dictionary
    { 1, 0, 0, 0 },                 // 1.0.0.0
    { 0, 0, 0, 0 },                 // Irrelevant for this data type
    }};

#endif

//----------------------------------------------------------------------------
//
//  main      for genctd
//
//----------------------------------------------------------------------------
int  main(int argc, char **argv) {
    UErrorCode  status = U_ZERO_ERROR;
    const char *wordFileName;
    const char *outFileName;
    const char *outDir = NULL;
    const char *copyright = NULL;

    //
    // Pick up and check the command line arguments,
    //    using the standard ICU tool utils option handling.
    //
    U_MAIN_INIT_ARGS(argc, argv);
    progName = argv[0];
    argc=u_parseArgs(argc, argv, sizeof(options)/sizeof(options[0]), options);
    if(argc<0) {
        // Unrecognized option
        fprintf(stderr, "error in command line argument \"%s\"\n", argv[-argc]);
        usageAndDie(U_ILLEGAL_ARGUMENT_ERROR);
    }

    if(options[0].doesOccur || options[1].doesOccur) {
        //  -? or -h for help.
        usageAndDie(0);
    }

    if (!options[3].doesOccur || argc < 2) {
        fprintf(stderr, "input and output file must both be specified.\n");
        usageAndDie(U_ILLEGAL_ARGUMENT_ERROR);
    }
    outFileName  = options[3].value;
    wordFileName = argv[1];

    if (options[4].doesOccur) {
        u_setDataDirectory(options[4].value);
    }

    status = U_ZERO_ERROR;

    /* Combine the directory with the file name */
    if(options[5].doesOccur) {
        outDir = options[5].value;
    }
    if (options[6].doesOccur) {
        copyright = U_COPYRIGHT_STRING;
    }

#if UCONFIG_NO_BREAK_ITERATION || UCONFIG_NO_FILE_IO

    UNewDataMemory *pData;
    char msg[1024];

    /* write message with just the name */
    sprintf(msg, "genctd writes dummy %s because of UCONFIG_NO_BREAK_ITERATION and/or UCONFIG_NO_FILE_IO, see uconfig.h", outFileName);
    fprintf(stderr, "%s\n", msg);

    /* write the dummy data file */
    pData = udata_create(outDir, NULL, outFileName, &dummyDataInfo, NULL, &status);
    udata_writeBlock(pData, msg, strlen(msg));
    udata_finish(pData, &status);
    return (int)status;

#else
    /* Initialize ICU */
    u_init(&status);
    if (U_FAILURE(status)) {
        fprintf(stderr, "%s: can not initialize ICU.  status = %s\n",
            argv[0], u_errorName(status));
        exit(1);
    }
    status = U_ZERO_ERROR;

    //
    //  Read in the dictionary source file
    //
    long        result;
    long        wordFileSize;
    FILE        *file;
    char        *wordBufferC;
    MutableTrieDictionary *mtd = NULL;
    
    file = fopen(wordFileName, "rb");
    if( file == 0 ) { //cannot find file
        //create 1-line dummy file: ie 1 char, 1 value
        UNewDataMemory *pData;
        char msg[1024];

        /* write message with just the name */
        sprintf(msg, "%s not found, genctd writes dummy %s", wordFileName, outFileName);
        fprintf(stderr, "%s\n", msg);

        UChar c = 0x0020;
        mtd = new MutableTrieDictionary(c, status, TRUE);
        mtd->addWord(&c, 1, status, 1);

    } else { //read words in from input file
        fseek(file, 0, SEEK_END);
        wordFileSize = ftell(file);
        fseek(file, 0, SEEK_SET);
        wordBufferC = new char[wordFileSize+10];
    
        result = (long)fread(wordBufferC, 1, wordFileSize, file);
        if (result != wordFileSize)  {
            fprintf(stderr, "Error reading file \"%s\"\n", wordFileName);
            exit (-1);
        }
        wordBufferC[wordFileSize]=0;
        fclose(file);
    
        //
        // Look for a Unicode Signature (BOM) on the word file
        //
        int32_t        signatureLength;
        const char *   wordSourceC = wordBufferC;
        const char*    encoding = ucnv_detectUnicodeSignature(
                               wordSourceC, wordFileSize, &signatureLength, &status);
        if (U_FAILURE(status)) {
            exit(status);
        }
        if(encoding!=NULL ){
            wordSourceC  += signatureLength;
            wordFileSize -= signatureLength;
        }
    
        //
        // Open a converter to take the rule file to UTF-16
        //
        UConverter* conv;
        conv = ucnv_open(encoding, &status);
        if (U_FAILURE(status)) {
            fprintf(stderr, "ucnv_open: ICU Error \"%s\"\n", u_errorName(status));
            exit(status);
        }
    
        //
        // Convert the words to UChar.
        //  Preflight first to determine required buffer size.
        //
        uint32_t destCap = ucnv_toUChars(conv,
                           NULL,           //  dest,
                           0,              //  destCapacity,
                           wordSourceC,
                           wordFileSize,
                           &status);
        if (status != U_BUFFER_OVERFLOW_ERROR) {
            fprintf(stderr, "ucnv_toUChars: ICU Error \"%s\"\n", u_errorName(status));
            exit(status);
        };
    
        status = U_ZERO_ERROR;
        UChar *wordSourceU = new UChar[destCap+1];
        ucnv_toUChars(conv,
                      wordSourceU,     //  dest,
                      destCap+1,
                      wordSourceC,
                      wordFileSize,
                      &status);
        if (U_FAILURE(status)) {
            fprintf(stderr, "ucnv_toUChars: ICU Error \"%s\"\n", u_errorName(status));
            exit(status);
        };
        ucnv_close(conv);
    
        // Get rid of the original file buffer
        delete[] wordBufferC;
    
        // Create a MutableTrieDictionary, and loop through all the lines, inserting
        // words.
    
        // First, pick a median character.
        UChar *current = wordSourceU + (destCap/2);
        UChar uc = *current++;
        UnicodeSet breaks;
        breaks.add(0x000A);     // Line Feed
        breaks.add(0x000D);     // Carriage Return
        breaks.add(0x2028);     // Line Separator
        breaks.add(0x2029);     // Paragraph Separator
    
        do { 
            // Look for line break
            while (uc && !breaks.contains(uc)) {
                uc = *current++;
            }
            // Now skip to first non-line-break
            while (uc && breaks.contains(uc)) {
                uc = *current++;
            }
        }
        while (uc && (breaks.contains(uc) || u_isspace(uc)));
    
        mtd = new MutableTrieDictionary(uc, status);
        
        if (U_FAILURE(status)) {
            fprintf(stderr, "new MutableTrieDictionary: ICU Error \"%s\"\n", u_errorName(status));
            exit(status);
        }
        
        // Now add the words. Words are non-space characters at the beginning of
        // lines, and must be at least one UChar. If a word has an associated value,
        // the value should follow the word on the same line after a tab character.
        current = wordSourceU;
        UChar *candidate = current;
        uc = *current++;
        int32_t length = 0;
        int count = 0;
                
        while (uc) {
            while (uc && !u_isspace(uc)) {
                ++length;
                uc = *current++;
            }
            
            UnicodeString valueString;
            UChar candidateValue;
            if(uc == 0x0009){ //separator is a tab char, read in number after space
            	while (uc && u_isspace(uc)) {
            		uc = *current++;
            	}
                while (uc && !u_isspace(uc)) {
                    valueString.append(uc);
                    uc = *current++;
                }
            }
            
            if (length > 0) {
                count++;
                if(valueString.length() > 0){
                    mtd->setValued(TRUE);
    
                    uint32_t value = 0;
                    char* s = new char[valueString.length()];
                    valueString.extract(0,valueString.length(), s, valueString.length());
                    int n = sscanf(s, "%ud", &value);
                    U_ASSERT(n == 1);
                    U_ASSERT(value >= 0); 
                    mtd->addWord(candidate, length, status, (uint16_t)value);
                    delete[] s;
                } else {
                    mtd->addWord(candidate, length, status);
                }
    
                if (U_FAILURE(status)) {
                    fprintf(stderr, "MutableTrieDictionary::addWord: ICU Error \"%s\" at line %d in input file\n",
                            u_errorName(status), count);
                    exit(status);
                }
            }
    
            // Find beginning of next line
            while (uc && !breaks.contains(uc)) {
                uc = *current++;
            }
            // Find next non-line-breaking character
            while (uc && breaks.contains(uc)) {
                uc = *current++;
            }
            candidate = current-1;
            length = 0;
        }
    
        // Get rid of the Unicode text buffer
        delete[] wordSourceU;
    }

    // Now, create a CompactTrieDictionary from the mutable dictionary
    CompactTrieDictionary *ctd = new CompactTrieDictionary(*mtd, status);
    if (U_FAILURE(status)) {
        fprintf(stderr, "new CompactTrieDictionary: ICU Error \"%s\"\n", u_errorName(status));
        exit(status);
    }
    
    // Get rid of the MutableTrieDictionary
    delete mtd;

    //
    //  Get the binary data from the dictionary.
    //
    uint32_t        outDataSize = ctd->dataSize();
    const uint8_t  *outData = (const uint8_t *)ctd->data();

    //
    //  Create the output file
    //
    size_t bytesWritten;
    UNewDataMemory *pData;
    pData = udata_create(outDir, NULL, outFileName, &(dh.info), copyright, &status);
    if(U_FAILURE(status)) {
        fprintf(stderr, "genctd: Could not open output file \"%s\", \"%s\"\n", 
                         outFileName, u_errorName(status));
        exit(status);
    }


    //  Write the data itself.
    udata_writeBlock(pData, outData, outDataSize);
    // finish up 
    bytesWritten = udata_finish(pData, &status);
    if(U_FAILURE(status)) {
        fprintf(stderr, "genctd: error \"%s\" writing the output file\n", u_errorName(status));
        exit(status);
    }
    
    if (bytesWritten != outDataSize) {
        fprintf(stderr, "Error writing to output file \"%s\"\n", outFileName);
        exit(-1);
    }
    
    // Get rid of the CompactTrieDictionary
    delete ctd;

    u_cleanup();

    printf("genctd: tool completed successfully.\n");
    return 0;

#endif /* #if !UCONFIG_NO_BREAK_ITERATION */
}
