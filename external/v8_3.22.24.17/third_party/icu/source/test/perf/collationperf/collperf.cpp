/********************************************************************
 * COPYRIGHT:
 * Copyright (C) 2001-2010 IBM, Inc.   All Rights Reserved.
 *
 ********************************************************************/
/********************************************************************************
*
* File CALLCOLL.C
*
* Modification History:
*        Name                     Description
*     Andy Heninger             First Version
*
*********************************************************************************
*/

//
//  This program tests string collation and sort key generation performance.
//      Three APIs can be teste: ICU C , Unix strcoll, strxfrm and Windows LCMapString
//      A file of names is required as input, one per line.  It must be in utf-8 or utf-16 format,
//      and include a byte order mark.  Either LE or BE format is OK.
//

const char gUsageString[] =
 "usage:  collperf options...\n"
    "-help                      Display this message.\n"
    "-file file_name            utf-16 format file of names.\n"
    "-locale name               ICU locale to use.  Default is en_US\n"
    "-rules file_name           Collation rules file (overrides locale)\n"
    "-langid 0x1234             Windows Language ID number.  Default to value for -locale option\n"
    "                              see http://msdn.microsoft.com/library/psdk/winbase/nls_8xo3.htm\n"
    "-win                       Run test using Windows native services.  (ICU is default)\n"
    "-unix                      Run test using Unix strxfrm, strcoll services.\n"
    "-uselen                    Use API with string lengths.  Default is null-terminated strings\n"
    "-usekeys                   Run tests using sortkeys rather than strcoll\n"
    "-strcmp                    Run tests using u_strcmp rather than strcoll\n"
    "-strcmpCPO                 Run tests using u_strcmpCodePointOrder rather than strcoll\n"
    "-loop nnnn                 Loopcount for test.  Adjust for reasonable total running time.\n"
    "-iloop n                   Inner Loop Count.  Default = 1.  Number of calls to function\n"
    "                               under test at each call point.  For measuring test overhead.\n"
    "-terse                     Terse numbers-only output.  Intended for use by scripts.\n"
    "-french                    French accent ordering\n"
    "-frenchoff                 No French accent ordering (for use with French locales.)\n"
    "-norm                      Normalizing mode on\n"
    "-shifted                   Shifted mode\n"
    "-lower                     Lower case first\n"
    "-upper                     Upper case first\n"
    "-case                      Enable separate case level\n"
    "-level n                   Sort level, 1 to 5, for Primary, Secndary, Tertiary, Quaternary, Identical\n"
    "-keyhist                   Produce a table sort key size vs. string length\n"
    "-binsearch                 Binary Search timing test\n"
    "-keygen                    Sort Key Generation timing test\n"
    "-qsort                     Quicksort timing test\n"
    "-iter                      Iteration Performance Test\n"
    "-dump                      Display strings, sort keys and CEs.\n"
    ;



#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <locale.h>
#include <errno.h>

#include <unicode/utypes.h>
#include <unicode/ucol.h>
#include <unicode/ucoleitr.h>
#include <unicode/uloc.h>
#include <unicode/ustring.h>
#include <unicode/ures.h>
#include <unicode/uchar.h>
#include <unicode/ucnv.h>
#include <unicode/utf8.h>

#ifdef WIN32
#include <windows.h>
#else
//
//  Stubs for Windows API functions when building on UNIXes.
//
typedef int DWORD;
inline int CompareStringW(DWORD, DWORD, UChar *, int, UChar *, int) {return 0;}
#include <sys/time.h>
unsigned long timeGetTime() {
    struct timeval t;
    gettimeofday(&t, 0);
    unsigned long val = t.tv_sec * 1000;  // Let it overflow.  Who cares.
    val += t.tv_usec / 1000;
    return val;
}
inline int LCMapStringW(DWORD, DWORD, UChar *, int, UChar *, int) {return 0;}
const int LCMAP_SORTKEY = 0;
#define MAKELCID(a,b) 0
const int SORT_DEFAULT = 0;
#endif



//
//  Command line option variables
//     These global variables are set according to the options specified
//     on the command line by the user.
char * opt_fName      = 0;
const char * opt_locale     = "en_US";
int    opt_langid     = 0;         // Defaults to value corresponding to opt_locale.
char * opt_rules      = 0;
UBool  opt_help       = FALSE;
int    opt_loopCount  = 1;
int    opt_iLoopCount = 1;
UBool  opt_terse      = FALSE;
UBool  opt_qsort      = FALSE;
UBool  opt_binsearch  = FALSE;
UBool  opt_icu        = TRUE;
UBool  opt_win        = FALSE;      // Run with Windows native functions.
UBool  opt_unix       = FALSE;      // Run with UNIX strcoll, strxfrm functions.
UBool  opt_uselen     = FALSE;
UBool  opt_usekeys    = FALSE;
UBool  opt_strcmp     = FALSE;
UBool  opt_strcmpCPO  = FALSE;
UBool  opt_norm       = FALSE;
UBool  opt_keygen     = FALSE;
UBool  opt_french     = FALSE;
UBool  opt_frenchoff  = FALSE;
UBool  opt_shifted    = FALSE;
UBool  opt_lower      = FALSE;
UBool  opt_upper      = FALSE;
UBool  opt_case       = FALSE;
int    opt_level      = 0;
UBool  opt_keyhist    = FALSE;
UBool  opt_itertest   = FALSE;
UBool  opt_dump       = FALSE;



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
    {"-rules",       OptSpec::STRING, &opt_rules},
    {"-qsort",       OptSpec::FLAG,   &opt_qsort},
    {"-binsearch",   OptSpec::FLAG,   &opt_binsearch},
    {"-iter",        OptSpec::FLAG,   &opt_itertest},
    {"-win",         OptSpec::FLAG,   &opt_win},
    {"-unix",        OptSpec::FLAG,   &opt_unix},
    {"-uselen",      OptSpec::FLAG,   &opt_uselen},
    {"-usekeys",     OptSpec::FLAG,   &opt_usekeys},
    {"-strcmp",      OptSpec::FLAG,   &opt_strcmp},
    {"-strcmpCPO",   OptSpec::FLAG,   &opt_strcmpCPO},
    {"-norm",        OptSpec::FLAG,   &opt_norm},
    {"-french",      OptSpec::FLAG,   &opt_french},
    {"-frenchoff",   OptSpec::FLAG,   &opt_frenchoff},
    {"-shifted",     OptSpec::FLAG,   &opt_shifted},
    {"-lower",       OptSpec::FLAG,   &opt_lower},
    {"-upper",       OptSpec::FLAG,   &opt_upper},
    {"-case",        OptSpec::FLAG,   &opt_case},
    {"-level",       OptSpec::NUM,    &opt_level},
    {"-keyhist",     OptSpec::FLAG,   &opt_keyhist},
    {"-keygen",      OptSpec::FLAG,   &opt_keygen},
    {"-loop",        OptSpec::NUM,    &opt_loopCount},
    {"-iloop",       OptSpec::NUM,    &opt_iLoopCount},
    {"-terse",       OptSpec::FLAG,   &opt_terse},
    {"-dump",        OptSpec::FLAG,   &opt_dump},
    {"-help",        OptSpec::FLAG,   &opt_help},
    {"-?",           OptSpec::FLAG,   &opt_help},
    {0, OptSpec::FLAG, 0}
};


//---------------------------------------------------------------------------
//
//  Global variables pointing to and describing the test file
//
//---------------------------------------------------------------------------

//
//   struct Line
//
//      Each line from the source file (containing a name, presumably) gets
//      one of these structs.
//
struct  Line {
    UChar     *name;
    int        len;
    char      *winSortKey;
    char      *icuSortKey;
    char      *unixSortKey;
    char      *unixName;
};



Line          *gFileLines;           // Ptr to array of Line structs, one per line in the file.
int            gNumFileLines;
UCollator     *gCol;
DWORD          gWinLCID;

Line          **gSortedLines;
Line          **gRandomLines;
int            gCount;



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

//---------------------------------------------------------------------------------------
//
//   Comparison functions for use by qsort.
//
//       Six flavors, ICU or Windows, SortKey or String Compare, Strings with length
//           or null terminated.
//
//---------------------------------------------------------------------------------------
int ICUstrcmpK(const void *a, const void *b) {
    gCount++;
    int t = strcmp((*(Line **)a)->icuSortKey, (*(Line **)b)->icuSortKey);
    return t;
}


int ICUstrcmpL(const void *a, const void *b) {
    gCount++;
    UCollationResult t;
    t = ucol_strcoll(gCol, (*(Line **)a)->name, (*(Line **)a)->len, (*(Line **)b)->name, (*(Line **)b)->len);
    if (t == UCOL_LESS) return -1;
    if (t == UCOL_GREATER) return +1;
    return 0;
}


int ICUstrcmp(const void *a, const void *b) {
    gCount++;
    UCollationResult t;
    t = ucol_strcoll(gCol, (*(Line **)a)->name, -1, (*(Line **)b)->name, -1);
    if (t == UCOL_LESS) return -1;
    if (t == UCOL_GREATER) return +1;
    return 0;
}


int Winstrcmp(const void *a, const void *b) {
    gCount++;
    int t;
    t = CompareStringW(gWinLCID, 0, (*(Line **)a)->name, -1, (*(Line **)b)->name, -1);
    return t-2;
}


int UNIXstrcmp(const void *a, const void *b) {
    gCount++;
    int t;
    t = strcoll((*(Line **)a)->unixName, (*(Line **)b)->unixName);
    return t;
}


int WinstrcmpL(const void *a, const void *b) {
    gCount++;
    int t;
    t = CompareStringW(gWinLCID, 0, (*(Line **)a)->name, (*(Line **)a)->len, (*(Line **)b)->name, (*(Line **)b)->len);
    return t-2;
}


int WinstrcmpK(const void *a, const void *b) {
    gCount++;
    int t = strcmp((*(Line **)a)->winSortKey, (*(Line **)b)->winSortKey);
    return t;
}


//---------------------------------------------------------------------------------------
//
//   Function for sorting the names (lines) into a random order.
//      Order is based on a hash of the  ICU Sort key for the lines
//      The randomized order is used as input for the sorting timing tests.
//
//---------------------------------------------------------------------------------------
int ICURandomCmp(const void *a, const void *b) {
    char  *ask = (*(Line **)a)->icuSortKey;
    char  *bsk = (*(Line **)b)->icuSortKey;
    int   aVal = 0;
    int   bVal = 0;
    int   retVal;
    while (*ask != 0) {
        aVal += aVal*37 + *ask++;
    }
    while (*bsk != 0) {
        bVal += bVal*37 + *bsk++;
    }
    retVal = -1;
    if (aVal == bVal) {
        retVal = 0;
    }
    else if (aVal > bVal) {
        retVal = 1;
    }
    return retVal;
}

//---------------------------------------------------------------------------------------
//
//   doKeyGen()     Key Generation Timing Test
//
//---------------------------------------------------------------------------------------
void doKeyGen()
{
    int  line;
    int  loops = 0;
    int  iLoop;
    int  t;
    int  len=-1;

    // Adjust loop count to compensate for file size.   Should be order n
    double dLoopCount = double(opt_loopCount) * (1000. /  double(gNumFileLines));
    int adj_loopCount = int(dLoopCount);
    if (adj_loopCount < 1) adj_loopCount = 1;


    unsigned long startTime = timeGetTime();

    if (opt_win) {
        for (loops=0; loops<adj_loopCount; loops++) {
            for (line=0; line < gNumFileLines; line++) {
                if (opt_uselen) {
                    len = gFileLines[line].len;
                }
                for (iLoop=0; iLoop < opt_iLoopCount; iLoop++) {
                    t=LCMapStringW(gWinLCID, LCMAP_SORTKEY,
                        gFileLines[line].name, len,
                        (unsigned short *)gFileLines[line].winSortKey, 5000);    // TODO  something with length.
                }
            }
        }
    }
    else if (opt_icu)
    {
        for (loops=0; loops<adj_loopCount; loops++) {
            for (line=0; line < gNumFileLines; line++) {
                if (opt_uselen) {
                    len = gFileLines[line].len;
                }
                for (iLoop=0; iLoop < opt_iLoopCount; iLoop++) {
                    t = ucol_getSortKey(gCol, gFileLines[line].name, len, (unsigned char *)gFileLines[line].icuSortKey, 5000);
                }
            }
        }
    }
    else if (opt_unix)
    {
        for (loops=0; loops<adj_loopCount; loops++) {
            for (line=0; line < gNumFileLines; line++) {
                for (iLoop=0; iLoop < opt_iLoopCount; iLoop++) {
                t = strxfrm(gFileLines[line].unixSortKey, gFileLines[line].unixName, 5000);
                }
            }
        }
    }

    unsigned long elapsedTime = timeGetTime() - startTime;
    int ns = (int)(float(1000000) * (float)elapsedTime / (float)(adj_loopCount*gNumFileLines));

    if (opt_terse == FALSE) {
        printf("Sort Key Generation:  total # of keys = %d\n", loops*gNumFileLines);
        printf("Sort Key Generation:  time per key = %d ns\n", ns);
    }
    else {
        printf("%d,  ", ns);
    }

    int   totalKeyLen = 0;
    int   totalChars  = 0;
    for (line=0; line<gNumFileLines; line++) {
        totalChars += u_strlen(gFileLines[line].name);
        if (opt_win) {
            totalKeyLen += strlen(gFileLines[line].winSortKey);
        }
        else if (opt_icu) {
            totalKeyLen += strlen(gFileLines[line].icuSortKey);
        }
        else if (opt_unix) {
            totalKeyLen += strlen(gFileLines[line].unixSortKey);
        }

    }
    if (opt_terse == FALSE) {
        printf("Key Length / character = %f\n", (float)totalKeyLen / (float)totalChars);
    } else {
        printf("%f, ", (float)totalKeyLen / (float)totalChars);
    }
}



//---------------------------------------------------------------------------------------
//
//    doBinarySearch()    Binary Search timing test.  Each name from the list
//                        is looked up in the full sorted list of names.
//
//---------------------------------------------------------------------------------------
void doBinarySearch()
{

    gCount = 0;
    int  line;
    int  loops = 0;
    int  iLoop = 0;
    unsigned long elapsedTime = 0;

    // Adjust loop count to compensate for file size.   Should be order n (lookups) * log n  (compares/lookup)
    // Accurate timings do not depend on this being perfect.  The correction is just to try to
    //   get total running times of about the right order, so the that user doesn't need to
    //   manually adjust the loop count for every different file size.
    double dLoopCount = double(opt_loopCount) * 3000. / (log10(gNumFileLines) * double(gNumFileLines));
    if (opt_usekeys) dLoopCount *= 5;
    int adj_loopCount = int(dLoopCount);
    if (adj_loopCount < 1) adj_loopCount = 1;


    for (;;) {  // not really a loop, just allows "break" to work, to simplify
                //   inadvertantly running more than one test through here.
        if (opt_strcmp || opt_strcmpCPO) 
        {
            unsigned long startTime = timeGetTime();
            typedef int32_t (U_EXPORT2 *PF)(const UChar *, const UChar *);
            PF pf = u_strcmp;
            if (opt_strcmpCPO) {pf = u_strcmpCodePointOrder;}
            //if (opt_strcmp && opt_win) {pf = (PF)wcscmp;}   // Damn the difference between int32_t and int
                                                            //   which forces the use of a cast here.
            
            int r = 0;
            for (loops=0; loops<adj_loopCount; loops++) {
                
                for (line=0; line < gNumFileLines; line++) {
                    int hi      = gNumFileLines-1;
                    int lo      = 0;
                    int  guess = -1;
                    for (;;) {
                        int newGuess = (hi + lo) / 2;
                        if (newGuess == guess)
                            break;
                        guess = newGuess;
                        for (iLoop=0; iLoop < opt_iLoopCount; iLoop++) {
                            r = (*pf)((gSortedLines[line])->name, (gSortedLines[guess])->name);
                        }
                        gCount++;
                        if (r== 0)
                            break;
                        if (r < 0)
                            hi = guess;
                        else
                            lo   = guess;
                    }
                }
            }
            elapsedTime = timeGetTime() - startTime;
            break;
        }
        
        
        if (opt_icu)
        {
            unsigned long startTime = timeGetTime();
            UCollationResult  r = UCOL_EQUAL;
            for (loops=0; loops<adj_loopCount; loops++) {
                
                for (line=0; line < gNumFileLines; line++) {
                    int lineLen  = -1;
                    int guessLen = -1;
                    if (opt_uselen) {
                        lineLen = (gSortedLines[line])->len;
                    }
                    int hi      = gNumFileLines-1;
                    int lo      = 0;
                    int  guess = -1;
                    for (;;) {
                        int newGuess = (hi + lo) / 2;
                        if (newGuess == guess)
                            break;
                        guess = newGuess;
                        int ri = 0;
                        if (opt_usekeys) {
                            for (iLoop=0; iLoop < opt_iLoopCount; iLoop++) {
                                ri = strcmp((gSortedLines[line])->icuSortKey, (gSortedLines[guess])->icuSortKey);
                            }
                            gCount++;
                            r=UCOL_GREATER; if(ri<0) {r=UCOL_LESS;} else if (ri==0) {r=UCOL_EQUAL;}
                        }
                        else
                        {
                            if (opt_uselen) {
                                guessLen = (gSortedLines[guess])->len;
                            }
                            for (iLoop=0; iLoop < opt_iLoopCount; iLoop++) {
                                r = ucol_strcoll(gCol, (gSortedLines[line])->name, lineLen, (gSortedLines[guess])->name, guessLen);
                            }
                            gCount++;
                        }
                        if (r== UCOL_EQUAL)
                            break;
                        if (r == UCOL_LESS)
                            hi = guess;
                        else
                            lo   = guess;
                    }
                }
            }
            elapsedTime = timeGetTime() - startTime;
            break;
        }
        
        if (opt_win)
        {
            unsigned long startTime = timeGetTime();
            int r = 0;
            for (loops=0; loops<adj_loopCount; loops++) {
                
                for (line=0; line < gNumFileLines; line++) {
                    int lineLen  = -1;
                    int guessLen = -1;
                    if (opt_uselen) {
                        lineLen = (gSortedLines[line])->len;
                    }
                    int hi   = gNumFileLines-1;
                    int lo   = 0;
                    int  guess = -1;
                    for (;;) {
                        int newGuess = (hi + lo) / 2;
                        if (newGuess == guess)
                            break;
                        guess = newGuess;
                        if (opt_usekeys) {
                            for (iLoop=0; iLoop < opt_iLoopCount; iLoop++) {
                                r = strcmp((gSortedLines[line])->winSortKey, (gSortedLines[guess])->winSortKey);
                            }
                            gCount++;
                            r+=2;
                        }
                        else
                        {
                            if (opt_uselen) {
                                guessLen = (gSortedLines[guess])->len;
                            }
                            for (iLoop=0; iLoop < opt_iLoopCount; iLoop++) {
                                r = CompareStringW(gWinLCID, 0, (gSortedLines[line])->name, lineLen, (gSortedLines[guess])->name, guessLen);
                            }
                            if (r == 0) {
                                if (opt_terse == FALSE) {
                                    fprintf(stderr, "Error returned from Windows CompareStringW.\n");
                                }
                                exit(-1);
                            }
                            gCount++;
                        }
                        if (r== 2)   //  strings ==
                            break;
                        if (r == 1)  //  line < guess
                            hi = guess;
                        else         //  line > guess
                            lo   = guess;
                    }
                }
            }
            elapsedTime = timeGetTime() - startTime;
            break;
        }
        
        if (opt_unix)
        {
            unsigned long startTime = timeGetTime();
            int r = 0;
            for (loops=0; loops<adj_loopCount; loops++) {
                
                for (line=0; line < gNumFileLines; line++) {
                    int hi   = gNumFileLines-1;
                    int lo   = 0;
                    int  guess = -1;
                    for (;;) {
                        int newGuess = (hi + lo) / 2;
                        if (newGuess == guess)
                            break;
                        guess = newGuess;
                        if (opt_usekeys) {
                            for (iLoop=0; iLoop < opt_iLoopCount; iLoop++) {
                                 r = strcmp((gSortedLines[line])->unixSortKey, (gSortedLines[guess])->unixSortKey);
                            }
                            gCount++;
                        }
                        else
                        {
                            for (iLoop=0; iLoop < opt_iLoopCount; iLoop++) {
                                r = strcoll((gSortedLines[line])->unixName, (gSortedLines[guess])->unixName);
                            }
                            errno = 0;
                            if (errno != 0) {
                                fprintf(stderr, "Error %d returned from strcoll.\n", errno);
                                exit(-1);
                            }
                            gCount++;
                        }
                        if (r == 0)   //  strings ==
                            break;
                        if (r < 0)  //  line < guess
                            hi = guess;
                        else         //  line > guess
                            lo   = guess;
                    }
                }
            }
            elapsedTime = timeGetTime() - startTime;
            break;
        }
        break;
    }

    int ns = (int)(float(1000000) * (float)elapsedTime / (float)gCount);
    if (opt_terse == FALSE) {
        printf("binary search:  total # of string compares = %d\n", gCount);
        printf("binary search:  compares per loop = %d\n", gCount / loops);
        printf("binary search:  time per compare = %d ns\n", ns);
    } else {
        printf("%d, ", ns);
    }

}




//---------------------------------------------------------------------------------------
//
//   doQSort()    The quick sort timing test.  Uses the C library qsort function.
//
//---------------------------------------------------------------------------------------
void doQSort() {
    int i;
    Line **sortBuf = new Line *[gNumFileLines];

    // Adjust loop count to compensate for file size.   QSort should be n log(n)
    double dLoopCount = double(opt_loopCount) * 3000. / (log10(gNumFileLines) * double(gNumFileLines));
    if (opt_usekeys) dLoopCount *= 5;
    int adj_loopCount = int(dLoopCount);
    if (adj_loopCount < 1) adj_loopCount = 1;


    gCount = 0;
    unsigned long startTime = timeGetTime();
    if (opt_win && opt_usekeys) {
        for (i=0; i<opt_loopCount; i++) {
            memcpy(sortBuf, gRandomLines, gNumFileLines * sizeof(Line *));
            qsort(sortBuf, gNumFileLines, sizeof(Line *), WinstrcmpK);
        }
    }

    else if (opt_win && opt_uselen) {
        for (i=0; i<adj_loopCount; i++) {
            memcpy(sortBuf, gRandomLines, gNumFileLines * sizeof(Line *));
            qsort(sortBuf, gNumFileLines, sizeof(Line *), WinstrcmpL);
        }
    }


    else if (opt_win && !opt_uselen) {
        for (i=0; i<adj_loopCount; i++) {
            memcpy(sortBuf, gRandomLines, gNumFileLines * sizeof(Line *));
            qsort(sortBuf, gNumFileLines, sizeof(Line *), Winstrcmp);
        }
    }

    else if (opt_icu && opt_usekeys) {
        for (i=0; i<adj_loopCount; i++) {
            memcpy(sortBuf, gRandomLines, gNumFileLines * sizeof(Line *));
            qsort(sortBuf, gNumFileLines, sizeof(Line *), ICUstrcmpK);
        }
    }

    else if (opt_icu && opt_uselen) {
        for (i=0; i<adj_loopCount; i++) {
            memcpy(sortBuf, gRandomLines, gNumFileLines * sizeof(Line *));
            qsort(sortBuf, gNumFileLines, sizeof(Line *), ICUstrcmpL);
        }
    }


    else if (opt_icu && !opt_uselen) {
        for (i=0; i<adj_loopCount; i++) {
            memcpy(sortBuf, gRandomLines, gNumFileLines * sizeof(Line *));
            qsort(sortBuf, gNumFileLines, sizeof(Line *), ICUstrcmp);
        }
    }

    else if (opt_unix && !opt_usekeys) {
        for (i=0; i<adj_loopCount; i++) {
            memcpy(sortBuf, gRandomLines, gNumFileLines * sizeof(Line *));
            qsort(sortBuf, gNumFileLines, sizeof(Line *), UNIXstrcmp);
        }
    }

    unsigned long elapsedTime = timeGetTime() - startTime;
    int ns = (int)(float(1000000) * (float)elapsedTime / (float)gCount);
    if (opt_terse == FALSE) {
        printf("qsort:  total # of string compares = %d\n", gCount);
        printf("qsort:  time per compare = %d ns\n", ns);
    } else {
        printf("%d, ", ns);
    }
}



//---------------------------------------------------------------------------------------
//
//    doKeyHist()       Output a table of data for
//                        average sort key size vs. string length.
//
//---------------------------------------------------------------------------------------
void doKeyHist() {
    int     i;
    int     maxLen = 0;

    // Find the maximum string length
    for (i=0; i<gNumFileLines; i++) {
        if (gFileLines[i].len > maxLen) maxLen = gFileLines[i].len;
    }

    // Allocate arrays to hold the histogram data
    int *accumulatedLen  = new int[maxLen+1];
    int *numKeysOfSize   = new int[maxLen+1];
    for (i=0; i<=maxLen; i++) {
        accumulatedLen[i] = 0;
        numKeysOfSize[i] = 0;
    }

    // Fill the arrays...
    for (i=0; i<gNumFileLines; i++) {
        int len = gFileLines[i].len;
        accumulatedLen[len] += strlen(gFileLines[i].icuSortKey);
        numKeysOfSize[len] += 1;
    }

    // And write out averages
    printf("String Length,  Avg Key Length,  Avg Key Len per char\n");
    for (i=1; i<=maxLen; i++) {
        if (numKeysOfSize[i] > 0) {
            printf("%d, %f, %f\n", i, (float)accumulatedLen[i] / (float)numKeysOfSize[i],
                (float)accumulatedLen[i] / (float)(numKeysOfSize[i] * i));
        }
    }
    delete []accumulatedLen;
    delete []numKeysOfSize ;
}

//---------------------------------------------------------------------------------------
//
//    doForwardIterTest(UBool)       Forward iteration test
//                                   argument null-terminated string used
//
//---------------------------------------------------------------------------------------
void doForwardIterTest(UBool haslen) {
    int count = 0;
    
    UErrorCode error = U_ZERO_ERROR;
    printf("\n\nPerforming forward iteration performance test with ");

    if (haslen) {
        printf("non-null terminated data -----------\n");
    }
    else {
        printf("null terminated data -----------\n");
    }
    printf("performance test on strings from file -----------\n");

    UChar dummytext[] = {0, 0};
    UCollationElements *iter = ucol_openElements(gCol, NULL, 0, &error);
    ucol_setText(iter, dummytext, 1, &error);
    
    gCount = 0;
    unsigned long startTime = timeGetTime();
    while (count < opt_loopCount) {
        int linecount = 0;
        while (linecount < gNumFileLines) {
            UChar *str = gFileLines[linecount].name;
            int strlen = haslen?gFileLines[linecount].len:-1;
            ucol_setText(iter, str, strlen, &error);
            while (ucol_next(iter, &error) != UCOL_NULLORDER) {
                gCount++;
            }

            linecount ++;
        }
        count ++;
    }
    unsigned long elapsedTime = timeGetTime() - startTime;
    printf("elapsedTime %ld\n", elapsedTime);

    // empty loop recalculation
    count = 0;
    startTime = timeGetTime();
    while (count < opt_loopCount) {
        int linecount = 0;
        while (linecount < gNumFileLines) {
            UChar *str = gFileLines[linecount].name;
            int strlen = haslen?gFileLines[linecount].len:-1;
            ucol_setText(iter, str, strlen, &error);
            linecount ++;
        }
        count ++;
    }
    elapsedTime -= (timeGetTime() - startTime);
    printf("elapsedTime %ld\n", elapsedTime);

    ucol_closeElements(iter);

    int ns = (int)(float(1000000) * (float)elapsedTime / (float)gCount);
    printf("Total number of strings compared %d in %d loops\n", gNumFileLines,
                                                                opt_loopCount);
    printf("Average time per ucol_next() nano seconds %d\n", ns);

    printf("performance test on skipped-5 concatenated strings from file -----------\n");

    UChar *str;
    int    strlen = 0;
    // appending all the strings
    int linecount = 0;
    while (linecount < gNumFileLines) {
        strlen += haslen?gFileLines[linecount].len:
                                      u_strlen(gFileLines[linecount].name);
        linecount ++;
    }
    str = (UChar *)malloc(sizeof(UChar) * strlen);
    int strindex = 0;
    linecount = 0;
    while (strindex < strlen) {
        int len = 0;
        len += haslen?gFileLines[linecount].len:
                                      u_strlen(gFileLines[linecount].name);
        memcpy(str + strindex, gFileLines[linecount].name, 
               sizeof(UChar) * len);
        strindex += len;
        linecount ++;
    }

    printf("Total size of strings %d\n", strlen);

    gCount = 0;
    count  = 0;

    if (!haslen) {
        strlen = -1;
    }
    iter = ucol_openElements(gCol, str, strlen, &error);
    if (!haslen) {
        strlen = u_strlen(str);
    }
    strlen -= 5; // any left over characters are not iterated,
                 // this is to ensure the backwards and forwards iterators
                 // gets the same position
    startTime = timeGetTime();
    while (count < opt_loopCount) {
        int count5 = 5;
        strindex = 0;
        ucol_setOffset(iter, strindex, &error);
        while (TRUE) {
            if (ucol_next(iter, &error) == UCOL_NULLORDER) {
                break;
            }
            gCount++;
            count5 --;
            if (count5 == 0) {
                strindex += 10;
                if (strindex > strlen) {
                    break;
                }
                ucol_setOffset(iter, strindex, &error);
                count5 = 5;
            }
        }
        count ++;
    }

    elapsedTime = timeGetTime() - startTime;
    printf("elapsedTime %ld\n", elapsedTime);
    
    // empty loop recalculation
    int tempgCount = 0;
    count = 0;
    startTime = timeGetTime();
    while (count < opt_loopCount) {
        int count5 = 5;
        strindex = 0;
        ucol_setOffset(iter, strindex, &error);
        while (TRUE) {
            tempgCount ++;
            count5 --;
            if (count5 == 0) {
                strindex += 10;
                if (strindex > strlen) {
                    break;
                }
                ucol_setOffset(iter, strindex, &error);
                count5 = 5;
            }
        }
        count ++;
    }
    elapsedTime -= (timeGetTime() - startTime);
    printf("elapsedTime %ld\n", elapsedTime);

    ucol_closeElements(iter);

    printf("gCount %d\n", gCount);
    ns = (int)(float(1000000) * (float)elapsedTime / (float)gCount);
    printf("Average time per ucol_next() nano seconds %d\n", ns);
}

//---------------------------------------------------------------------------------------
//
//    doBackwardIterTest(UBool)      Backwards iteration test
//                                   argument null-terminated string used
//
//---------------------------------------------------------------------------------------
void doBackwardIterTest(UBool haslen) {
    int count = 0;
    UErrorCode error = U_ZERO_ERROR;
    printf("\n\nPerforming backward iteration performance test with ");

    if (haslen) {
        printf("non-null terminated data -----------\n");
    }
    else {
        printf("null terminated data -----------\n");
    }
    
    printf("performance test on strings from file -----------\n");

    UCollationElements *iter = ucol_openElements(gCol, NULL, 0, &error);
    UChar dummytext[] = {0, 0};
    ucol_setText(iter, dummytext, 1, &error);

    gCount = 0;
    unsigned long startTime = timeGetTime();
    while (count < opt_loopCount) {
        int linecount = 0;
        while (linecount < gNumFileLines) {
            UChar *str = gFileLines[linecount].name;
            int strlen = haslen?gFileLines[linecount].len:-1;
            ucol_setText(iter, str, strlen, &error);
            while (ucol_previous(iter, &error) != UCOL_NULLORDER) {
                gCount ++;
            }

            linecount ++;
        }
        count ++;
    }
    unsigned long elapsedTime = timeGetTime() - startTime;

    printf("elapsedTime %ld\n", elapsedTime);

    // empty loop recalculation
    count = 0;
    startTime = timeGetTime();
    while (count < opt_loopCount) {
        int linecount = 0;
        while (linecount < gNumFileLines) {
            UChar *str = gFileLines[linecount].name;
            int strlen = haslen?gFileLines[linecount].len:-1;
            ucol_setText(iter, str, strlen, &error);
            linecount ++;
        }
        count ++;
    }
    elapsedTime -= (timeGetTime() - startTime);

    printf("elapsedTime %ld\n", elapsedTime);
    ucol_closeElements(iter);

    int ns = (int)(float(1000000) * (float)elapsedTime / (float)gCount);
    printf("Total number of strings compared %d in %d loops\n", gNumFileLines,
                                                                opt_loopCount);
    printf("Average time per ucol_previous() nano seconds %d\n", ns);

    printf("performance test on skipped-5 concatenated strings from file -----------\n");

    UChar *str;
    int    strlen = 0;
    // appending all the strings
    int linecount = 0;
    while (linecount < gNumFileLines) {
        strlen += haslen?gFileLines[linecount].len:
                                      u_strlen(gFileLines[linecount].name);
        linecount ++;
    }
    str = (UChar *)malloc(sizeof(UChar) * strlen);
    int strindex = 0;
    linecount = 0;
    while (strindex < strlen) {
        int len = 0;
        len += haslen?gFileLines[linecount].len:
                                      u_strlen(gFileLines[linecount].name);
        memcpy(str + strindex, gFileLines[linecount].name, 
               sizeof(UChar) * len);
        strindex += len;
        linecount ++;
    }

    printf("Total size of strings %d\n", strlen);

    gCount = 0;
    count  = 0;

    if (!haslen) {
        strlen = -1;
    }

    iter = ucol_openElements(gCol, str, strlen, &error);
    if (!haslen) {
        strlen = u_strlen(str);
    }

    startTime = timeGetTime();
    while (count < opt_loopCount) {
        int count5 = 5;
        strindex = 5;
        ucol_setOffset(iter, strindex, &error);
        while (TRUE) {
            if (ucol_previous(iter, &error) == UCOL_NULLORDER) {
                break;
            }
             gCount ++;
             count5 --;
             if (count5 == 0) {
                 strindex += 10;
                 if (strindex > strlen) {
                    break;
                 }
                 ucol_setOffset(iter, strindex, &error);
                 count5 = 5;
             }
        }
        count ++;
    }

    elapsedTime = timeGetTime() - startTime;
    printf("elapsedTime %ld\n", elapsedTime);
    
    // empty loop recalculation
    count = 0;
    int tempgCount = 0;
    startTime = timeGetTime();
    while (count < opt_loopCount) {
        int count5 = 5;
        strindex = 5;
        ucol_setOffset(iter, strindex, &error);
        while (TRUE) {
             tempgCount ++;
             count5 --;
             if (count5 == 0) {
                 strindex += 10;
                 if (strindex > strlen) {
                    break;
                 }
                 ucol_setOffset(iter, strindex, &error);
                 count5 = 5;
             }
        }
        count ++;
    }
    elapsedTime -= (timeGetTime() - startTime);
    printf("elapsedTime %ld\n", elapsedTime);
    ucol_closeElements(iter);

    printf("gCount %d\n", gCount);
    ns = (int)(float(1000000) * (float)elapsedTime / (float)gCount);
    printf("Average time per ucol_previous() nano seconds %d\n", ns);
}

//---------------------------------------------------------------------------------------
//
//    doIterTest()       Iteration test
//
//---------------------------------------------------------------------------------------
void doIterTest() {
    doForwardIterTest(opt_uselen);
    doBackwardIterTest(opt_uselen);
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
    int    line;

    UConverter   *cvrtr;    // An ICU code page converter.
    UErrorCode    status = U_ZERO_ERROR;


    cvrtr = ucnv_open("utf-8", &status);    // we are just doing UTF-8 locales for now.
    if (U_FAILURE(status)) {
        fprintf(stderr, "ICU Converter open failed.: %s\n", u_errorName(status));
        exit(-1);
    }

    for (line=0; line < gNumFileLines; line++) {
        int sizeNeeded = ucnv_fromUChars(cvrtr,
                                         0,            // ptr to target buffer.
                                         0,            // length of target buffer.
                                         gFileLines[line].name,
                                         -1,           //  source is null terminated
                                         &status);
        if (status != U_BUFFER_OVERFLOW_ERROR && status != U_ZERO_ERROR) {
            //fprintf(stderr, "Conversion from Unicode, something is wrong.\n");
            //exit(-1);
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
    
private:
    UCharFile (const UCharFile & /*other*/) {};                         // No copy constructor.
    UCharFile & operator = (const UCharFile &/*other*/) {return *this;};   // No assignment op

    FILE         *fFile;
    const char   *fName;
    UBool        fEof;
    UBool        fError;
    UChar        fPending2ndSurrogate;
    
    enum {UTF16LE, UTF16BE, UTF8} fEncoding;
};

UCharFile::UCharFile(const char * fileName) {
    fEof                 = FALSE;
    fError               = FALSE;
    fName                = fileName;
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
                fprintf(stderr, "utf-8 encoded file contains corrupt data.\n");
                fError = TRUE;
                return 0;
            }
            
            unsigned char  bytes[10];
            bytes[0] = (unsigned char)ch;
            int i;
            for (i=1; i<nBytes; i++) {
                bytes[i] = fgetc(fFile);
                if (bytes[i] < 0x80 || bytes[i] >= 0xc0) {
                    fprintf(stderr, "utf-8 encoded file contains corrupt data.\n");
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
    default:
        c = 0xFFFD; /* Error, unspecified codepage*/
        fprintf(stderr, "UCharFile: Error: unknown fEncoding\n");
        exit(1);
    }
    return c;
}

//----------------------------------------------------------------------------------------
//
//   openRulesCollator  - Command line specified a rules file.  Read it in
//                        and open a collator with it.
//
//----------------------------------------------------------------------------------------
UCollator *openRulesCollator() {
    UCharFile f(opt_rules);
    if (f.error()) {
        return 0;
    }

    int  bufLen = 10000;
    UChar *buf = (UChar *)malloc(bufLen * sizeof(UChar));
    int i = 0;

    for(;;) {
        buf[i] = f.get();
        if (f.eof()) {
            break;
        }
        if (f.error()) {
            return 0;
        }
        i++;
        if (i >= bufLen) {
            bufLen += 10000;
            buf = (UChar *)realloc(buf, bufLen);
        }
    }
    buf[i] = 0;

    UErrorCode    status = U_ZERO_ERROR;
    UCollator *coll = ucol_openRules(buf, u_strlen(buf), UCOL_OFF,
                                         UCOL_DEFAULT_STRENGTH, NULL, &status);
    if (U_FAILURE(status)) {
        fprintf(stderr, "ICU ucol_openRules() open failed.: %d\n", status);
        return 0;
    }
    free(buf);
    return coll;
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
    if (opt_unix || opt_win) opt_icu = FALSE;
    if (opt_unix) opt_win = FALSE;

    //
    //  Set up an ICU collator
    //
    UErrorCode          status = U_ZERO_ERROR;

    if (opt_rules != 0) {
        gCol = openRulesCollator();
        if (gCol == 0) {return -1;}
    }
    else {
        gCol = ucol_open(opt_locale, &status);
        if (U_FAILURE(status)) {
            fprintf(stderr, "Collator creation failed.: %d\n", status);
            return -1;
        }
    }
    if (status==U_USING_DEFAULT_WARNING && opt_terse==FALSE) {
        fprintf(stderr, "Warning, U_USING_DEFAULT_WARNING for %s\n", opt_locale);
    }
    if (status==U_USING_FALLBACK_WARNING && opt_terse==FALSE) {
        fprintf(stderr, "Warning, U_USING_FALLBACK_ERROR for %s\n", opt_locale);
    }

    if (opt_norm) {
        ucol_setAttribute(gCol, UCOL_NORMALIZATION_MODE, UCOL_ON, &status);
    }
    if (opt_french && opt_frenchoff) {
        fprintf(stderr, "collperf:  Error, specified both -french and -frenchoff options.");
        exit(-1);
    }
    if (opt_french) {
        ucol_setAttribute(gCol, UCOL_FRENCH_COLLATION, UCOL_ON, &status);
    }
    if (opt_frenchoff) {
        ucol_setAttribute(gCol, UCOL_FRENCH_COLLATION, UCOL_OFF, &status);
    }
    if (opt_lower) {
        ucol_setAttribute(gCol, UCOL_CASE_FIRST, UCOL_LOWER_FIRST, &status);
    }
    if (opt_upper) {
        ucol_setAttribute(gCol, UCOL_CASE_FIRST, UCOL_UPPER_FIRST, &status);
    }
    if (opt_case) {
        ucol_setAttribute(gCol, UCOL_CASE_LEVEL, UCOL_ON, &status);
    }
    if (opt_shifted) {
        ucol_setAttribute(gCol, UCOL_ALTERNATE_HANDLING, UCOL_SHIFTED, &status);
    }
    if (opt_level != 0) {
        switch (opt_level) {
        case 1:
            ucol_setAttribute(gCol, UCOL_STRENGTH, UCOL_PRIMARY, &status);
            break;
        case 2:
            ucol_setAttribute(gCol, UCOL_STRENGTH, UCOL_SECONDARY, &status);
            break;
        case 3:
            ucol_setAttribute(gCol, UCOL_STRENGTH, UCOL_TERTIARY, &status);
            break;
        case 4:
            ucol_setAttribute(gCol, UCOL_STRENGTH, UCOL_QUATERNARY, &status);
            break;
        case 5:
            ucol_setAttribute(gCol, UCOL_STRENGTH, UCOL_IDENTICAL, &status);
            break;
        default:
            fprintf(stderr, "-level param must be between 1 and 5\n");
            exit(-1);
        }
    }

    if (U_FAILURE(status)) {
        fprintf(stderr, "Collator attribute setting failed.: %d\n", status);
        return -1;
    }


    //
    //  Set up a Windows LCID
    //
    if (opt_langid != 0) {
        gWinLCID = MAKELCID(opt_langid, SORT_DEFAULT);
    }
    else {
        gWinLCID = uloc_getLCID(opt_locale);
    }


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

    const int MAXLINES = 100000;
    gFileLines = new Line[MAXLINES];
    UChar buf[1024];
    int   column = 0;

    //  Read the file, split into lines, and save in memory.
    //  Loop runs once per utf-16 value from the input file,
    //    (The number of bytes read from file per loop iteration depends on external encoding.)
    for (;;) {

        UChar c = f.get();
        if (f.error()){
            exit(-1);
        }


        // We now have a good UTF-16 value in c.

        // Watch for CR, LF, EOF; these finish off a line.
        if (c == 0xd) {
            continue;
        }

        if (f.eof() || c == 0x0a || c==0x2028) {  // Unipad inserts 2028 line separators!
            buf[column++] = 0;
            if (column > 1) {
                gFileLines[gNumFileLines].name  = new UChar[column];
                gFileLines[gNumFileLines].len   = column-1;
                memcpy(gFileLines[gNumFileLines].name, buf, column * sizeof(UChar));
                gNumFileLines++;
                column = 0;
                if (gNumFileLines >= MAXLINES) {
                    fprintf(stderr, "File too big.  Max number of lines is %d\n", MAXLINES);
                    exit(-1);
                }

            }
            if (c == 0xa || c == 0x2028)
                continue;
            else
                break;  // EOF
        }
        buf[column++] = c;
        if (column >= 1023)
        {
            static UBool warnFlag = TRUE;
            if (warnFlag) {
                fprintf(stderr, "Warning - file line longer than 1023 chars truncated.\n");
                warnFlag = FALSE;
            }
            column--;
        }
    }

    if (opt_terse == FALSE) {
        printf("file \"%s\", %d lines.\n", opt_fName, gNumFileLines);
    }


    // Convert the lines to the UNIX encoding.
    if (opt_unix) {
        UnixConvert();
    }

    //
    //  Pre-compute ICU sort keys for the lines of the file.
    //
    int line;
    int32_t t;

    for (line=0; line<gNumFileLines; line++) {
         t = ucol_getSortKey(gCol, gFileLines[line].name, -1, (unsigned char *)buf, sizeof(buf));
         gFileLines[line].icuSortKey  = new char[t];

         if (t > (int32_t)sizeof(buf)) {
             t = ucol_getSortKey(gCol, gFileLines[line].name, -1, (unsigned char *)gFileLines[line].icuSortKey , t);
         }
         else
         {
             memcpy(gFileLines[line].icuSortKey, buf, t);
         }
    }



    //
    //  Pre-compute Windows sort keys for the lines of the file.
    //
    for (line=0; line<gNumFileLines; line++) {
         t=LCMapStringW(gWinLCID, LCMAP_SORTKEY, gFileLines[line].name, -1, buf, sizeof(buf));
         gFileLines[line].winSortKey  = new char[t];
         if (t > (int32_t)sizeof(buf)) {
             t = LCMapStringW(gWinLCID, LCMAP_SORTKEY, gFileLines[line].name, -1, (unsigned short *)(gFileLines[line].winSortKey), t);
         }
         else
         {
             memcpy(gFileLines[line].winSortKey, buf, t);
         }
    }

    //
    //  Pre-compute UNIX sort keys for the lines of the file.
    //
    if (opt_unix) {
        for (line=0; line<gNumFileLines; line++) {
            t=strxfrm((char *)buf,  gFileLines[line].unixName,  sizeof(buf));
            gFileLines[line].unixSortKey  = new char[t];
            if (t > (int32_t)sizeof(buf)) {
                t = strxfrm(gFileLines[line].unixSortKey,  gFileLines[line].unixName,  sizeof(buf));
            }
            else
            {
                memcpy(gFileLines[line].unixSortKey, buf, t);
            }
        }
    }


    //
    //  Dump file lines, CEs, Sort Keys if requested.
    //
    if (opt_dump) {
        int  i;
        for (line=0; line<gNumFileLines; line++) {
            for (i=0;;i++) {
                UChar  c = gFileLines[line].name[i];
                if (c == 0)
                    break;
                if (c < 0x20 || c > 0x7e) {
                    printf("\\u%.4x", c);
                }
                else {
                    printf("%c", c);
                }
            }
            printf("\n");

            printf("   CEs: ");
            UCollationElements *CEiter = ucol_openElements(gCol, gFileLines[line].name, -1, &status);
            int32_t ce;
            i = 0;
            for (;;) {
                ce = ucol_next(CEiter, &status);
                if (ce == UCOL_NULLORDER) {
                    break;
                }
                printf(" %.8x", ce);
                if (++i > 8) {
                    printf("\n        ");
                    i = 0;
                }
            }
            printf("\n");
            ucol_closeElements(CEiter);


            printf("   ICU Sort Key: ");
            for (i=0; ; i++) {
                unsigned char c = gFileLines[line].icuSortKey[i];
                printf("%02x ", c);
                if (c == 0) {
                    break;
                }
                if (i > 0 && i % 20 == 0) {
                    printf("\n                 ");
                }
           }
            printf("\n");
        }
    }


    //
    //  Pre-sort the lines.
    //
    int i;
    gSortedLines = new Line *[gNumFileLines];
    for (i=0; i<gNumFileLines; i++) {
        gSortedLines[i] = &gFileLines[i];
    }

    if (opt_win) {
        qsort(gSortedLines, gNumFileLines, sizeof(Line *), Winstrcmp);
    }
    else if (opt_unix) {
        qsort(gSortedLines, gNumFileLines, sizeof(Line *), UNIXstrcmp);
    }
    else   /* ICU */
    {
        qsort(gSortedLines, gNumFileLines, sizeof(Line *), ICUstrcmp);
    }


    //
    //  Make up a randomized order, will be used for sorting tests.
    //
    gRandomLines = new Line *[gNumFileLines];
    for (i=0; i<gNumFileLines; i++) {
        gRandomLines[i] = &gFileLines[i];
    }
    qsort(gRandomLines, gNumFileLines, sizeof(Line *), ICURandomCmp);




    //
    //  We've got the file read into memory.  Go do something with it.
    //

    if (opt_qsort)     doQSort();
    if (opt_binsearch) doBinarySearch();
    if (opt_keygen)    doKeyGen();
    if (opt_keyhist)   doKeyHist();
    if (opt_itertest)  doIterTest();

    return 0;

}
