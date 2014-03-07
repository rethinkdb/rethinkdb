/********************************************************************
 * COPYRIGHT:
 * Copyright (C) 2001-2005 IBM, Inc.   All Rights Reserved.
 *
 ********************************************************************/
/********************************************************************************
*
* File ubrkperf.cpp
*
* Modification History:
*        Name                     Description
*     Vladimir Weinstein          First Version, based on collperf
*
*********************************************************************************
*/

//
//  This program tests break iterator performance
//      Currently we test only ICU APIs with the future possibility of testing *nix & win32 APIs
//      (if any)
//      A text file is required as input.  It must be in utf-8 or utf-16 format,
//      and include a byte order mark.  Either LE or BE format is OK.
//

const char gUsageString[] =
 "usage:  ubrkperf options...\n"
    "-help                      Display this message.\n"
    "-file file_name            utf-16/utf-8 format file.\n"
    "-locale name               ICU locale to use.  Default is en_US\n"
    "-langid 0x1234             Windows Language ID number.  Default to value for -locale option\n"
    "                              see http://msdn.microsoft.com/library/psdk/winbase/nls_8xo3.htm\n"
    "-win                       Run test using Windows native services. (currently not working) (ICU is default)\n"
    "-unix                      Run test using Unix word breaking services. (currently not working) \n"
    "-mac                       Run test using MacOSX word breaking services.\n"
    "-uselen                    Use API with string lengths.  Default is null-terminated strings\n"
    "-char                      Use character break iterator\n"
    "-word                      Use word break iterator\n"
    "-line                      Use line break iterator\n"
    "-sentence                  Use sentence break iterator\n"
    "-loop nnnn                 Loopcount for test.  Adjust for reasonable total running time.\n"
    "-iloop n                   Inner Loop Count.  Default = 1.  Number of calls to function\n"
    "                               under test at each call point.  For measuring test overhead.\n"
    "-terse                     Terse numbers-only output.  Intended for use by scripts.\n"
    "-dump                      Display stuff.\n"
    "-capi                      Use C APIs instead of C++ APIs (currently not working)\n"
    "-next                      Do the next test\n"
    "-isBound                   Do the isBound test\n"
    ;


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <locale.h>
#include <errno.h>
#include <sys/stat.h>

#include <unicode/utypes.h>
#include <unicode/ucol.h>
#include <unicode/ucoleitr.h>
#include <unicode/uloc.h>
#include <unicode/ustring.h>
#include <unicode/ures.h>
#include <unicode/uchar.h>
#include <unicode/ucnv.h>
#include <unicode/utf8.h>

#include <unicode/brkiter.h>


#ifdef U_WINDOWS
#include <windows.h>
#else
//
//  Stubs for Windows API functions when building on UNIXes.
//
#include <sys/time.h>
unsigned long timeGetTime() {
    struct timeval t;
    gettimeofday(&t, 0);
    unsigned long val = t.tv_sec * 1000;  // Let it overflow.  Who cares.
    val += t.tv_usec / 1000;
    return val;
};
#define MAKELCID(a,b) 0
#endif


//
//  Command line option variables
//     These global variables are set according to the options specified
//     on the command line by the user.
char * opt_fName      = 0;
char * opt_locale     = "en_US";
int    opt_langid     = 0;         // Defaults to value corresponding to opt_locale.
char * opt_rules      = 0;
UBool  opt_help       = FALSE;
int    opt_time       = 0;
int    opt_loopCount  = 0;
int    opt_passesCount= 1;
UBool  opt_terse      = FALSE;
UBool  opt_icu        = TRUE;
UBool  opt_win        = FALSE;      // Run with Windows native functions.
UBool  opt_unix       = FALSE;      // Run with UNIX strcoll, strxfrm functions.
UBool  opt_mac        = FALSE;      // Run with MacOSX word break services.
UBool  opt_uselen     = FALSE;
UBool  opt_dump       = FALSE;
UBool  opt_char       = FALSE;
UBool  opt_word       = FALSE;
UBool  opt_line       = FALSE;
UBool  opt_sentence   = FALSE;
UBool  opt_capi       = FALSE;

UBool  opt_next       = FALSE;
UBool  opt_isBound    = FALSE;



//
//   Definitions for the command line options
//
struct OptSpec {
    const char *name;
    enum {FLAG, NUM, STRING} type;
    void *pVar;
};

OptSpec opts[] = {
    {"-file",        OptSpec::STRING, &opt_fName},
    {"-locale",      OptSpec::STRING, &opt_locale},
    {"-langid",      OptSpec::NUM,    &opt_langid},
    {"-win",         OptSpec::FLAG,   &opt_win},
    {"-unix",        OptSpec::FLAG,   &opt_unix},
    {"-mac",         OptSpec::FLAG,   &opt_mac},
    {"-uselen",      OptSpec::FLAG,   &opt_uselen},
    {"-loop",        OptSpec::NUM,    &opt_loopCount},
    {"-time",        OptSpec::NUM,    &opt_time},
    {"-passes",      OptSpec::NUM,    &opt_passesCount},
    {"-char",        OptSpec::FLAG,   &opt_char},
    {"-word",        OptSpec::FLAG,   &opt_word},
    {"-line",        OptSpec::FLAG,   &opt_line},
    {"-sentence",    OptSpec::FLAG,   &opt_sentence},
    {"-terse",       OptSpec::FLAG,   &opt_terse},
    {"-dump",        OptSpec::FLAG,   &opt_dump},
    {"-capi",        OptSpec::FLAG,   &opt_capi},
    {"-next",        OptSpec::FLAG,   &opt_next},
    {"-isBound",     OptSpec::FLAG,   &opt_isBound},
    {"-help",        OptSpec::FLAG,   &opt_help},
    {"-?",           OptSpec::FLAG,   &opt_help},
    {0, OptSpec::FLAG, 0}
};


//---------------------------------------------------------------------------
//
//  Global variables pointing to and describing the test file
//
//---------------------------------------------------------------------------

//DWORD          gWinLCID;
BreakIterator *brkit = NULL;
UChar *text = NULL;
int32_t textSize = 0;



#ifdef U_DARWIN
#include <ApplicationServices/ApplicationServices.h>
enum{
  kUCTextBreakAllMask = (kUCTextBreakClusterMask | kUCTextBreakWordMask | kUCTextBreakLineMask)
    };
UCTextBreakType breakTypes[4] = {kUCTextBreakCharMask, kUCTextBreakClusterMask, kUCTextBreakWordMask, kUCTextBreakLineMask};
TextBreakLocatorRef breakRef;
UCTextBreakType macBreakType;

void createMACBrkIt() {
  OSStatus status = noErr;
  LocaleRef lref;
  status = LocaleRefFromLocaleString(opt_locale, &lref);
  status = UCCreateTextBreakLocator(lref, 0, kUCTextBreakAllMask, (TextBreakLocatorRef*)&breakRef);
  if(opt_char == TRUE) {
    macBreakType = kUCTextBreakClusterMask;
  } else if(opt_word == TRUE) {
    macBreakType = kUCTextBreakWordMask;
  } else if(opt_line == TRUE) {
    macBreakType = kUCTextBreakLineMask;
  } else if(opt_sentence == TRUE) {
    // error
    // brkit = BreakIterator::createSentenceInstance(opt_locale, status);
  } else {
    // default is character iterator
    macBreakType = kUCTextBreakClusterMask;
      }
}
#endif

void createICUBrkIt() {
  //
  //  Set up an ICU break iterator
  //
  UErrorCode          status = U_ZERO_ERROR;
  if(opt_char == TRUE) {
    brkit = BreakIterator::createCharacterInstance(opt_locale, status);
  } else if(opt_word == TRUE) {
    brkit = BreakIterator::createWordInstance(opt_locale, status);
  } else if(opt_line == TRUE) {
    brkit = BreakIterator::createLineInstance(opt_locale, status);
  } else if(opt_sentence == TRUE) {
    brkit = BreakIterator::createSentenceInstance(opt_locale, status);
  } else {
    // default is character iterator
    brkit = BreakIterator::createCharacterInstance(opt_locale, status);
  }
  if (status==U_USING_DEFAULT_WARNING && opt_terse==FALSE) {
    fprintf(stderr, "Warning, U_USING_DEFAULT_WARNING for %s\n", opt_locale);
  }
  if (status==U_USING_FALLBACK_WARNING && opt_terse==FALSE) {
    fprintf(stderr, "Warning, U_USING_FALLBACK_ERROR for %s\n", opt_locale);
  }

}

//---------------------------------------------------------------------------
//
//  ProcessOptions()    Function to read the command line options.
//
//---------------------------------------------------------------------------
UBool ProcessOptions(int argc, const char **argv, OptSpec opts[])
{
    int         i;
    int         argNum;
    const char  *pArgName;
    OptSpec    *pOpt;

    for (argNum=1; argNum<argc; argNum++) {
        pArgName = argv[argNum];
        for (pOpt = opts;  pOpt->name != 0; pOpt++) {
            if (strcmp(pOpt->name, pArgName) == 0) {
                switch (pOpt->type) {
                case OptSpec::FLAG:
                    *(UBool *)(pOpt->pVar) = TRUE;
                    break;
                case OptSpec::STRING:
                    argNum ++;
                    if (argNum >= argc) {
                        fprintf(stderr, "value expected for \"%s\" option.\n", pOpt->name);
                        return FALSE;
                    }
                    *(const char **)(pOpt->pVar)  = argv[argNum];
                    break;
                case OptSpec::NUM:
                    argNum ++;
                    if (argNum >= argc) {
                        fprintf(stderr, "value expected for \"%s\" option.\n", pOpt->name);
                        return FALSE;
                    }
                    char *endp;
                    i = strtol(argv[argNum], &endp, 0);
                    if (endp == argv[argNum]) {
                        fprintf(stderr, "integer value expected for \"%s\" option.\n", pOpt->name);
                        return FALSE;
                    }
                    *(int *)(pOpt->pVar) = i;
                }
                break;
            }
        }
        if (pOpt->name == 0)
        {
            fprintf(stderr, "Unrecognized option \"%s\"\n", pArgName);
            return FALSE;
        }
    }
return TRUE;
}


void doForwardTest() {
  if (opt_terse == FALSE) {
    printf("Doing the forward test\n");
  }
  int32_t noBreaks = 0;
  int32_t i = 0;
  unsigned long startTime = timeGetTime();
  unsigned long elapsedTime = 0;
  if(opt_icu) {
    createICUBrkIt();
    brkit->setText(UnicodeString(text, textSize));
    brkit->first();
    if (opt_terse == FALSE) {
      printf("Warmup\n");
    }
    int j;
    while((j = brkit->next()) != BreakIterator::DONE) {
      noBreaks++;
      //fprintf(stderr, "%d ", j);
    }
  
    if (opt_terse == FALSE) {
      printf("Measure\n");
    } 
    startTime = timeGetTime();
    for(i = 0; i < opt_loopCount; i++) {
      brkit->first();  
      while(brkit->next() != BreakIterator::DONE) {
      }
    }

    elapsedTime = timeGetTime()-startTime;
  } else if(opt_mac) {
#ifdef U_DARWIN
    createMACBrkIt();
    UniChar* filePtr = text;
    OSStatus status = noErr;
    UniCharCount startOffset = 0, breakOffset = 0, numUniChars = textSize;
    startOffset = 0;
    //printf("\t---Search forward--\n");
			
    while (startOffset < numUniChars)
    {
	status = UCFindTextBreak(breakRef, macBreakType, kUCTextBreakLeadingEdgeMask, filePtr, numUniChars,
                               startOffset, &breakOffset);
      //require_action(status == noErr, EXIT, printf( "**UCFindTextBreak failed: startOffset %d, status %d\n", (int)startOffset, (int)status));
      //require_action((breakOffset <= numUniChars),EXIT, printf("**UCFindTextBreak breakOffset too big: startOffset %d, breakOffset %d\n", (int)startOffset, (int)breakOffset));
				
      // Output break
      //printf("\t%d\n", (int)breakOffset);
				
      // Increment counters
	noBreaks++;
      startOffset = breakOffset;
    }
    startTime = timeGetTime();
    for(i = 0; i < opt_loopCount; i++) {
      startOffset = 0;
			
      while (startOffset < numUniChars)
	{
	  status = UCFindTextBreak(breakRef, macBreakType, kUCTextBreakLeadingEdgeMask, filePtr, numUniChars,
				   startOffset, &breakOffset);
	  // Increment counters
	  startOffset = breakOffset;
	}
    }
    elapsedTime = timeGetTime()-startTime;
    UCDisposeTextBreakLocator(&breakRef);
#endif


  }


  if (opt_terse == FALSE) {
  int32_t loopTime = (int)(float(1000) * ((float)elapsedTime/(float)opt_loopCount));
      int32_t timePerCU = (int)(float(1000) * ((float)loopTime/(float)textSize));
      int32_t timePerBreak = (int)(float(1000) * ((float)loopTime/(float)noBreaks));
      printf("forward break iteration average loop time %d\n", loopTime);
      printf("number of code units %d average time per code unit %d\n", textSize, timePerCU);
      printf("number of breaks %d average time per break %d\n", noBreaks, timePerBreak);
  } else {
      printf("time=%d\nevents=%d\nsize=%d\n", elapsedTime, noBreaks, textSize);
  }


}

void doIsBoundTest() {
  int32_t noBreaks = 0, hit = 0;
  int32_t i = 0, j = 0;
  unsigned long startTime = timeGetTime();
  unsigned long elapsedTime = 0;
  createICUBrkIt();
  brkit->setText(UnicodeString(text, textSize));
  brkit->first();
  for(j = 0; j < textSize; j++) {
    if(brkit->isBoundary(j)) {
      noBreaks++;
      //fprintf(stderr, "%d ", j);
    }
  }
  /*
  while(brkit->next() != BreakIterator::DONE) {
    noBreaks++;
  }
  */
  
  startTime = timeGetTime();
  for(i = 0; i < opt_loopCount; i++) {
    for(j = 0; j < textSize; j++) {
      if(brkit->isBoundary(j)) {
        hit++;
      }
    }
  }

  elapsedTime = timeGetTime()-startTime;
  int32_t loopTime = (int)(float(1000) * ((float)elapsedTime/(float)opt_loopCount));
  if (opt_terse == FALSE) {
      int32_t timePerCU = (int)(float(1000) * ((float)loopTime/(float)textSize));
      int32_t timePerBreak = (int)(float(1000) * ((float)loopTime/(float)noBreaks));
      printf("forward break iteration average loop time %d\n", loopTime);
      printf("number of code units %d average time per code unit %d\n", textSize, timePerCU);
      printf("number of breaks %d average time per break %d\n", noBreaks, timePerBreak);
  } else {
      printf("time=%d\nevents=%d\nsize=%d\n", elapsedTime, noBreaks, textSize);
  }
}

//----------------------------------------------------------------------------------------
//
//   UnixConvert   -- Convert the lines of the file to the encoding for UNIX
//                    Since it appears that Unicode support is going in the general
//                    direction of the use of UTF-8 locales, that is the approach
//                    that is used here.
//
//----------------------------------------------------------------------------------------
void  UnixConvert() {
#if 0
    int    line;

    UConverter   *cvrtr;    // An ICU code page converter.
    UErrorCode    status = U_ZERO_ERROR;


    cvrtr = ucnv_open("utf-8", &status);    // we are just doing UTF-8 locales for now.
    if (U_FAILURE(status)) {
        fprintf(stderr, "ICU Converter open failed.: %d\n", &status);
        exit(-1);
    }
    // redo for unix
    for (line=0; line < gNumFileLines; line++) {
        int sizeNeeded = ucnv_fromUChars(cvrtr,
                                         0,            // ptr to target buffer.
                                         0,            // length of target buffer.
                                         gFileLines[line].name,
                                         -1,           //  source is null terminated
                                         &status);
        if (status != U_BUFFER_OVERFLOW_ERROR && status != U_ZERO_ERROR) {
            fprintf(stderr, "Conversion from Unicode, something is wrong.\n");
            exit(-1);
        }
        status = U_ZERO_ERROR;
        gFileLines[line].unixName = new char[sizeNeeded+1];
        sizeNeeded = ucnv_fromUChars(cvrtr,
                                         gFileLines[line].unixName, // ptr to target buffer.
                                         sizeNeeded+1, // length of target buffer.
                                         gFileLines[line].name,
                                         -1,           //  source is null terminated
                                         &status);
        if (U_FAILURE(status)) {
            fprintf(stderr, "ICU Conversion Failed.: %d\n", status);
            exit(-1);
        }
        gFileLines[line].unixName[sizeNeeded] = 0;
    };
    ucnv_close(cvrtr);
#endif
}


//----------------------------------------------------------------------------------------
//
//  class UCharFile   Class to hide all the gorp to read a file in
//                    and produce a stream of UChars.
//
//----------------------------------------------------------------------------------------
class UCharFile {
public:
    UCharFile(const char *fileName);
    ~UCharFile();
    UChar   get();
    UBool   eof() {return fEof;};
    UBool   error() {return fError;};
    int32_t size() { return fFileSize; };
    
private:
    UCharFile (const UCharFile &other) {};                         // No copy constructor.
    UCharFile & operator = (const UCharFile &other) {return *this;};   // No assignment op

    FILE         *fFile;
    const char   *fName;
    UBool        fEof;
    UBool        fError;
    UChar        fPending2ndSurrogate;
    int32_t      fFileSize;
    
    enum {UTF16LE, UTF16BE, UTF8} fEncoding;
};

UCharFile::UCharFile(const char * fileName) {
    fEof                 = FALSE;
    fError               = FALSE;
    fName                = fileName;
    struct stat buf;
    int32_t result = stat(fileName, &buf);
    if(result != 0) {
      fprintf(stderr, "Error getting info\n");
      fFileSize = -1;
    } else {
      fFileSize = buf.st_size;
    }
    fFile                = fopen(fName, "rb");
    fPending2ndSurrogate = 0;
    if (fFile == NULL) {
        fprintf(stderr, "Can not open file \"%s\"\n", opt_fName);
        fError = TRUE;
        return;
    }
    //
    //  Look for the byte order mark at the start of the file.
    //
    int BOMC1, BOMC2, BOMC3;
    BOMC1 = fgetc(fFile);
    BOMC2 = fgetc(fFile);

    if (BOMC1 == 0xff && BOMC2 == 0xfe) {
        fEncoding = UTF16LE; }
    else if (BOMC1 == 0xfe && BOMC2 == 0xff) {
        fEncoding = UTF16BE; }
    else if (BOMC1 == 0xEF && BOMC2 == 0xBB && (BOMC3 = fgetc(fFile)) == 0xBF ) {
        fEncoding = UTF8; }
    else
    {
        fprintf(stderr, "collperf:  file \"%s\" encoding must be UTF-8 or UTF-16, and "
            "must include a BOM.\n", fileName);
        fError = true;
        return;
    }
}


UCharFile::~UCharFile() {
    fclose(fFile);
}



UChar UCharFile::get() {
    UChar   c;
    switch (fEncoding) {
    case UTF16LE:
        {
            int  cL, cH;
            cL = fgetc(fFile);
            cH = fgetc(fFile);
            c  = cL  | (cH << 8);
            if (cH == EOF) {
                c   = 0;
                fEof = TRUE;
            }
            break;
        }
    case UTF16BE:
        {
            int  cL, cH;
            cH = fgetc(fFile);
            cL = fgetc(fFile);
            c  = cL  | (cH << 8);
            if (cL == EOF) {
                c   = 0;
                fEof = TRUE;
            }
            break;
        }
    case UTF8:
        {
            if (fPending2ndSurrogate != 0) {
                c = fPending2ndSurrogate;
                fPending2ndSurrogate = 0;
                break;
            }
            
            int ch = fgetc(fFile);   // Note:  c and ch are separate cause eof test doesn't work on UChar type.
            if (ch == EOF) {
                c = 0;
                fEof = TRUE;
                break;
            }
            
            if (ch <= 0x7f) {
                // It's ascii.  No further utf-8 conversion.
                c = ch;
                break;
            }
            
            // Figure out the lenght of the char and read the rest of the bytes
            //   into a temp array.
            int nBytes;
            if (ch >= 0xF0) {nBytes=4;}
            else if (ch >= 0xE0) {nBytes=3;}
            else if (ch >= 0xC0) {nBytes=2;}
            else {
                fprintf(stderr, "not likely utf-8 encoded file %s contains corrupt data at offset %d.\n", fName, ftell(fFile));
                fError = TRUE;
                return 0;
            }
            
            unsigned char  bytes[10];
            bytes[0] = (unsigned char)ch;
            int i;
            for (i=1; i<nBytes; i++) {
                bytes[i] = fgetc(fFile);
                if (bytes[i] < 0x80 || bytes[i] >= 0xc0) {
                    fprintf(stderr, "utf-8 encoded file %s contains corrupt data at offset %d. Expected %d bytes, byte %d is invalid. First byte is %02X\n", fName, ftell(fFile), nBytes, i, ch);
                    fError = TRUE;
                    return 0;
                }
            }
            
            // Convert the bytes from the temp array to a Unicode char.
            i = 0;
            uint32_t  cp;
            UTF8_NEXT_CHAR_UNSAFE(bytes, i, cp);
            c = (UChar)cp;
            
            if (cp >= 0x10000) {
                // The code point needs to be broken up into a utf-16 surrogate pair.
                //  Process first half this time through the main loop, and
                //   remember the other half for the next time through.
                UChar utf16Buf[3];
                i = 0;
                UTF16_APPEND_CHAR_UNSAFE(utf16Buf, i, cp);
                fPending2ndSurrogate = utf16Buf[1];
                c = utf16Buf[0];
            }
            break;
        };
    }
    return c;
}


//----------------------------------------------------------------------------------------
//
//    Main   --  process command line, read in and pre-process the test file,
//                 call other functions to do the actual tests.
//
//----------------------------------------------------------------------------------------
int main(int argc, const char** argv) {
    if (ProcessOptions(argc, argv, opts) != TRUE || opt_help || opt_fName == 0) {
        printf(gUsageString);
        exit (1);
    }
    // Make sure that we've only got one API selected.
    if (opt_mac || opt_unix || opt_win) opt_icu = FALSE;
    if (opt_mac || opt_unix) opt_win = FALSE;
    if (opt_mac) opt_unix = FALSE;

    UErrorCode          status = U_ZERO_ERROR;



    //
    //  Set up a Windows LCID
    //
  /*
    if (opt_langid != 0) {
        gWinLCID = MAKELCID(opt_langid, SORT_DEFAULT);
    }
    else {
        gWinLCID = uloc_getLCID(opt_locale);
    }
  */

    //
    //  Set the UNIX locale
    //
    if (opt_unix) {
        if (setlocale(LC_ALL, opt_locale) == 0) {
            fprintf(stderr, "setlocale(LC_ALL, %s) failed.\n", opt_locale);
            exit(-1);
        }
    }

    // Read in  the input file.
    //   File assumed to be utf-16.
    //   Lines go onto heap buffers.  Global index array to line starts is created.
    //   Lines themselves are null terminated.
    //

    UCharFile f(opt_fName);
    if (f.error()) {
        exit(-1);
    }
    int32_t fileSize = f.size();
    const int STARTSIZE = 70000;
    int32_t bufSize = 0;
    int32_t charCount = 0;
    if(fileSize != -1) {
      text = (UChar *)malloc(fileSize*sizeof(UChar));
      bufSize = fileSize;
    } else {
      text = (UChar *)malloc(STARTSIZE*sizeof(UChar));
      bufSize = STARTSIZE;
    }
    if(text == NULL) {
      fprintf(stderr, "Allocating buffer failed\n");
      exit(-1);
    }
    

    //  Read the file, split into lines, and save in memory.
    //  Loop runs once per utf-16 value from the input file,
    //    (The number of bytes read from file per loop iteration depends on external encoding.)
    for (;;) {

        UChar c = f.get();
        if(f.eof()) {
          break;
        }
        if (f.error()){
          exit(-1);
        }
        // We now have a good UTF-16 value in c.
        text[charCount++] = c;
        if(charCount == bufSize) {
          text = (UChar *)realloc(text, 2*bufSize*sizeof(UChar));
          if(text == NULL) {
            fprintf(stderr, "Reallocating buffer failed\n");
            exit(-1);
          }
          bufSize *= 2;
        }
    }


    if (opt_terse == FALSE) {
        printf("file \"%s\", %d charCount code units.\n", opt_fName, charCount);
    }

    textSize = charCount;




    //
    //  Dump file contents if requested.
    //
    if (opt_dump) {
      // dump file, etc... possibly
    }


    //
    //  We've got the file read into memory.  Go do something with it.
    //
    int32_t i = 0;
    for(i = 0; i < opt_passesCount; i++) {
      if(opt_loopCount != 0) {
        if(opt_next) {
          doForwardTest();
        } else if(opt_isBound) {
          doIsBoundTest();
        } else {
          doForwardTest();
        }
      } else if(opt_time != 0) {

      }
    }

  if(text != NULL) {
    free(text);
  }
    if(brkit != NULL) {
      delete brkit;
    }

    return 0;
}
