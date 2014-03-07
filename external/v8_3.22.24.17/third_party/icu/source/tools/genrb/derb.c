/*
*******************************************************************************
*
*   Copyright (C) 1999-2010, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  derb.c
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2000sep6
*   created by: Vladimir Weinstein as an ICU workshop example
*   maintained by: Yves Arrouye <yves@realnames.com>
*/

#include "unicode/ucnv.h"
#include "unicode/ustring.h"
#include "unicode/putil.h"

#include "uresimp.h"
#include "cmemory.h"
#include "cstring.h"
#include "uoptions.h"
#include "toolutil.h"
#include "ustrfmt.h"

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#if defined(U_WINDOWS) || defined(U_CYGWIN)
#include <io.h>
#include <fcntl.h>
#define USE_FILENO_BINARY_MODE 1
/* Windows likes to rename Unix-like functions */
#ifndef fileno
#define fileno _fileno
#endif
#ifndef setmode
#define setmode _setmode
#endif
#ifndef O_BINARY
#define O_BINARY _O_BINARY
#endif
#endif

#define DERB_VERSION "1.0"

#define DERB_DEFAULT_TRUNC 80

static UConverter *defaultConverter = 0;

static const int32_t indentsize = 4;
static int32_t truncsize = DERB_DEFAULT_TRUNC;
static UBool trunc = FALSE;

static const char *getEncodingName(const char *encoding);
static void reportError(const char *pname, UErrorCode *status, const char *when);
static UChar *quotedString(const UChar *string);
static void printOutBundle(FILE *out, UConverter *converter, UResourceBundle *resource, int32_t indent, const char *pname, UErrorCode *status);
static void printString(FILE *out, UConverter *converter, const UChar *str, int32_t len);
static void printCString(FILE *out, UConverter *converter, const char *str, int32_t len);
static void printIndent(FILE *out, UConverter *converter, int32_t indent);
static void printHex(FILE *out, UConverter *converter, uint8_t what);

static UOption options[]={
    UOPTION_HELP_H,
    UOPTION_HELP_QUESTION_MARK,
/* 2 */    UOPTION_ENCODING,
/* 3 */    { "to-stdout", NULL, NULL, NULL, 'c', UOPT_NO_ARG, 0 } ,
/* 4 */    { "truncate", NULL, NULL, NULL, 't', UOPT_OPTIONAL_ARG, 0 },
/* 5 */    UOPTION_VERBOSE,
/* 6 */    UOPTION_DESTDIR,
/* 7 */    UOPTION_SOURCEDIR,
/* 8 */    { "bom", NULL, NULL, NULL, 0, UOPT_NO_ARG, 0 },
/* 9 */    UOPTION_ICUDATADIR,
/* 10 */   UOPTION_VERSION,
/* 11 */   { "suppressAliases", NULL, NULL, NULL, 'A', UOPT_NO_ARG, 0 }
};

static UBool verbose = FALSE;
static UBool suppressAliases = FALSE;

extern int
main(int argc, char* argv[]) {
    const char *encoding = NULL;
    const char *outputDir = NULL; /* NULL = no output directory, use current */
    const char *inputDir  = ".";
    int tostdout = 0;
    int prbom = 0;

    const char *pname;

    UResourceBundle *bundle = NULL;
    UErrorCode status = U_ZERO_ERROR;
    int32_t i = 0;

    UConverter *converter;

    const char* arg;

    /* Get the name of tool. */
    pname = uprv_strrchr(*argv, U_FILE_SEP_CHAR);
#if U_FILE_SEP_CHAR != U_FILE_ALT_SEP_CHAR
    if (!pname) {
        pname = uprv_strrchr(*argv, U_FILE_ALT_SEP_CHAR);
    }
#endif
    if (!pname) {
        pname = *argv;
    } else {
        ++pname;
    }

    /* error handling, printing usage message */
    argc=u_parseArgs(argc, argv, sizeof(options)/sizeof(options[0]), options);

    /* error handling, printing usage message */
    if(argc<0) {
        fprintf(stderr,
            "%s: error in command line argument \"%s\"\n", pname,
            argv[-argc]);
    }
    if(argc<0 || options[0].doesOccur || options[1].doesOccur) {
        fprintf(argc < 0 ? stderr : stdout,
            "%csage: %s [ -h, -?, --help ] [ -V, --version ]\n"
            " [ -v, --verbose ] [ -e, --encoding encoding ] [ --bom ]\n"
            " [ -t, --truncate [ size ] ]\n"
            " [ -s, --sourcedir source ] [ -d, --destdir destination ]\n"
            " [ -i, --icudatadir directory ] [ -c, --to-stdout ]\n"
            " [ -A, --suppressAliases]\n"
            " bundle ...\n", argc < 0 ? 'u' : 'U',
            pname);
        return argc<0 ? U_ILLEGAL_ARGUMENT_ERROR : U_ZERO_ERROR;
    }

    if(options[10].doesOccur) {
        fprintf(stderr,
                "%s version %s (ICU version %s).\n"
                "%s\n",
                pname, DERB_VERSION, U_ICU_VERSION, U_COPYRIGHT_STRING);
        return U_ZERO_ERROR;
    }
    if(options[2].doesOccur) {
        encoding = options[2].value;
    }

    if (options[3].doesOccur) {
        tostdout = 1;
    }

    if(options[4].doesOccur) {
        trunc = TRUE;
        if(options[4].value != NULL) {
            truncsize = atoi(options[4].value); /* user defined printable size */
        } else {
            truncsize = DERB_DEFAULT_TRUNC; /* we'll use default omitting size */
        }
    } else {
        trunc = FALSE;
    }

    if(options[5].doesOccur) {
        verbose = TRUE;
    }

    if (options[6].doesOccur) {
        outputDir = options[6].value;
    }

    if(options[7].doesOccur) {
        inputDir = options[7].value; /* we'll use users resources */
    }

    if (options[8].doesOccur) {
        prbom = 1;
    }

    if (options[9].doesOccur) {
        u_setDataDirectory(options[9].value);
    }

    if (options[11].doesOccur) {
      suppressAliases = TRUE;
    }

    converter = ucnv_open(encoding, &status);
    if (U_FAILURE(status)) {
        fprintf(stderr, "%s: couldn't create %s converter for encoding\n", pname, encoding ? encoding : ucnv_getDefaultName());
        return 2;
    }
    ucnv_setFromUCallBack(converter, UCNV_FROM_U_CALLBACK_ESCAPE, UCNV_ESCAPE_C, 0, 0, &status);
    if (U_FAILURE(status)) {
        fprintf(stderr, "%s: couldn't configure converter for encoding\n", pname);
        return 3;
    }

    defaultConverter = ucnv_open(0, &status);
    if (U_FAILURE(status)) {
        fprintf(stderr, "%s: couldn't create %s converter for encoding\n", ucnv_getDefaultName(), pname);
        return 2;
    }

    for (i = 1; i < argc; ++i) {
        static const UChar sp[] = { 0x0020 }; /* " " */
        char infile[4096]; /* XXX Sloppy. */
        char locale[64];
        const char *thename = 0, *p, *q;
        UBool fromICUData = FALSE;

        arg = getLongPathname(argv[i]);

        if (verbose) {
            printf("processing bundle \"%s\"\n", argv[i]);
        }

        p = uprv_strrchr(arg, U_FILE_SEP_CHAR);
#if U_FILE_SEP_CHAR != U_FILE_ALT_SEP_CHAR
        if (p == NULL) {
            p = uprv_strrchr(arg, U_FILE_ALT_SEP_CHAR);
        }
#endif
        if (!p) {
            p = arg;
        } else {
            p++;
        }
        q = uprv_strrchr(p, '.');
        if (!q) {
            for (q = p; *q; ++q)
                ;
        }
        uprv_strncpy(locale, p, q - p);
        locale[q - p] = 0;

        if (!(fromICUData = !uprv_strcmp(inputDir, "-"))) {
            UBool absfilename = *arg == U_FILE_SEP_CHAR;
#ifdef U_WINDOWS
            if (!absfilename) {
                absfilename = (uprv_strlen(arg) > 2 && isalpha(arg[0])
                    && arg[1] == ':' && arg[2] == U_FILE_SEP_CHAR);
            }
#endif
            if (absfilename) {
                thename = arg;
            } else {
                q = uprv_strrchr(arg, U_FILE_SEP_CHAR);
#if U_FILE_SEP_CHAR != U_FILE_ALT_SEP_CHAR
                if (q == NULL) {
                    q = uprv_strrchr(arg, U_FILE_ALT_SEP_CHAR);
                }
#endif
                uprv_strcpy(infile, inputDir);
                if(q != NULL) {
                    uprv_strcat(infile, U_FILE_SEP_STRING);
                    strncat(infile, arg, q-arg);
                }
                thename = infile;
            }
        }
        status = U_ZERO_ERROR;
        if (thename) {
            bundle = ures_openDirect(thename, locale, &status);
        } else {
            bundle = ures_open(fromICUData ? 0 : inputDir, locale, &status);
        }
        if (status == U_ZERO_ERROR) {
            FILE *out;

            const char *filename = 0;
            const char *ext = 0;

            if (!locale[0] || !tostdout) {
                filename = uprv_strrchr(arg, U_FILE_SEP_CHAR);

#if U_FILE_SEP_CHAR != U_FILE_ALT_SEP_CHAR
                if (!filename) {
                    filename = uprv_strrchr(arg, U_FILE_ALT_SEP_CHAR);
                }
#endif
                if (!filename) {
                    filename = arg;
                } else {
                    ++filename;
                }
                ext = uprv_strrchr(arg, '.');
                if (!ext) {
                    ext = filename + uprv_strlen(filename);
                }
            }

            if (tostdout) {
                out = stdout;
#if defined(U_WINDOWS) || defined(U_CYGWIN)
                if (setmode(fileno(out), O_BINARY) == -1) {
                    fprintf(stderr, "%s: couldn't set standard output to binary mode\n", pname);
                    return 4;
                }
#endif
            } else {
                char thefile[4096], *tp;
                int32_t len;

                if (outputDir) {
                    uprv_strcpy(thefile, outputDir);
                    uprv_strcat(thefile, U_FILE_SEP_STRING);
                } else {
                    *thefile = 0;
                }
                uprv_strcat(thefile, filename);
                tp = thefile + uprv_strlen(thefile);
                len = (int32_t)uprv_strlen(ext);
                if (len) {
                    tp -= len - 1;
                } else {
                    *tp++ = '.';
                }
                uprv_strcpy(tp, "txt");

                out = fopen(thefile, "w");
                if (!out) {
                    fprintf(stderr, "%s: couldn't create %s\n", pname, thefile);
                    return 4;
                }
            }

            if (prbom) { /* XXX: Should be done only for UTFs */
                static const UChar bom[] = { 0xFEFF };
                printString(out, converter, bom, (int32_t)(sizeof(bom)/sizeof(*bom)));
            }

            printCString(out, converter, "// -*- Coding: ", -1);
            printCString(out, converter, encoding ? encoding : getEncodingName(ucnv_getDefaultName()), -1);
            printCString(out, converter, "; -*-\n//\n", -1);
            printCString(out, converter, "// This file was dumped by derb(8) from ", -1);
            if (thename) {
                printCString(out, converter, thename, -1);
            } else if (fromICUData) {
                printCString(out, converter, "the ICU internal ", -1);
                printCString(out, converter, locale, -1);
                printCString(out, converter, " locale", -1);
            }

            printCString(out, converter, "\n// derb(8) by Vladimir Weinstein and Yves Arrouye\n\n", -1);

            if (locale[0]) {
                printCString(out, converter, locale, -1);
            } else {
                printCString(out, converter, filename, (int32_t)(ext - filename));
                printString(out, converter, sp, (int32_t)(sizeof(sp)/sizeof(*sp)));
            }
            printOutBundle(out, converter, bundle, 0, pname, &status);

            if (out != stdout) {
                fclose(out);
            }
        }
        else {
            reportError(pname, &status, "opening resource file");
        }

        ures_close(bundle);
    }

    ucnv_close(defaultConverter);
    ucnv_close(converter);

    return 0;
}

static UChar *quotedString(const UChar *string) {
    int len = u_strlen(string);
    int alen = len;
    const UChar *sp;
    UChar *newstr, *np;

    for (sp = string; *sp; ++sp) {
        switch (*sp) {
            case '\n':
            case 0x0022:
                ++alen;
                break;
        }
    }

    newstr = (UChar *) uprv_malloc((1 + alen) * sizeof(*newstr));
    for (sp = string, np = newstr; *sp; ++sp) {
        switch (*sp) {
            case '\n':
                *np++ = 0x005C;
                *np++ = 0x006E;
                break;

            case 0x0022:
                *np++ = 0x005C;

            default:
                *np++ = *sp;
                break;
        }
    }
    *np = 0;

    return newstr;
}


static void printString(FILE *out, UConverter *converter, const UChar *str, int32_t len) {
    char buf[256];
    const UChar *strEnd;

    if (len < 0) {
        len = u_strlen(str);
    }
    strEnd = str + len;

    do {
        UErrorCode err = U_ZERO_ERROR;
        char *bufp = buf, *bufend = buf + sizeof(buf) - 1 ;

        ucnv_fromUnicode(converter, &bufp, bufend, &str, strEnd, 0, 0, &err);
        *bufp = 0;

        fprintf(out, "%s", buf);
    } while (str < strEnd);
}

static void printCString(FILE *out, UConverter *converter, const char *str, int32_t len) {
    UChar buf[256];
    const char *strEnd;

    if (len < 0) {
        len = (int32_t)uprv_strlen(str);
    }
    strEnd = str + len;

    do {
        UErrorCode err = U_ZERO_ERROR;
        UChar *bufp = buf, *bufend = buf + (sizeof(buf)/sizeof(buf[0])) - 1 ;

        ucnv_toUnicode(defaultConverter, &bufp, bufend, &str, strEnd, 0, 0, &err);
        *bufp = 0;

        printString(out, converter, buf, (int32_t)(bufp - buf));
    } while (str < strEnd);
}

static void printIndent(FILE *out, UConverter *converter, int32_t indent) {
    UChar inchar[256];
    int32_t i = 0;
    for(i = 0; i<indent; i++) {
        inchar[i] = 0x0020;
    }
    inchar[indent] = 0;

    printString(out, converter, inchar, indent);
}

static void printHex(FILE *out, UConverter *converter, uint8_t what) {
    static const char map[] = "0123456789ABCDEF";
    UChar hex[2];

    hex[0] = map[what >> 4];
    hex[1] = map[what & 0xf];

    printString(out, converter, hex, (int32_t)(sizeof(hex)/sizeof(*hex)));
}

static void printOutAlias(FILE *out,  UConverter *converter, UResourceBundle *parent, Resource r, const char *key, int32_t indent, const char *pname, UErrorCode *status) {
    static const UChar cr[] = { '\n' };
    int32_t len = 0;
    const UChar* thestr = res_getAlias(&(parent->fResData), r, &len);
    UChar *string = quotedString(thestr);
    if(trunc && len > truncsize) {
        char msg[128];
        printIndent(out, converter, indent);
        sprintf(msg, "// WARNING: this resource, size %li is truncated to %li\n",
            (long)len, (long)truncsize/2);
        printCString(out, converter, msg, -1);
        len = truncsize;
    }
    if(U_SUCCESS(*status)) {
        static const UChar openStr[] = { 0x003A, 0x0061, 0x006C, 0x0069, 0x0061, 0x0073, 0x0020, 0x007B, 0x0020, 0x0022 }; /* ":alias { \"" */
        static const UChar closeStr[] = { 0x0022, 0x0020, 0x007D, 0x0020 }; /* "\" } " */
        printIndent(out, converter, indent);
        if(key != NULL) {
            printCString(out, converter, key, -1);
        }
        printString(out, converter, openStr, (int32_t)(sizeof(openStr) / sizeof(*openStr)));
        printString(out, converter, string, len);
        printString(out, converter, closeStr, (int32_t)(sizeof(closeStr) / sizeof(*closeStr)));
        if(verbose) {
            printCString(out, converter, " // ALIAS", -1);
        }
        printString(out, converter, cr, (int32_t)(sizeof(cr) / sizeof(*cr)));
    } else {
        reportError(pname, status, "getting binary value");
    }
    uprv_free(string);
}

static void printOutBundle(FILE *out, UConverter *converter, UResourceBundle *resource, int32_t indent, const char *pname, UErrorCode *status)
{
    static const UChar cr[] = { '\n' };

/*    int32_t noOfElements = ures_getSize(resource);*/
    int32_t i = 0;
    const char *key = ures_getKey(resource);

    switch(ures_getType(resource)) {
    case URES_STRING :
        {
            int32_t len=0;
            const UChar* thestr = ures_getString(resource, &len, status);
            UChar *string = quotedString(thestr);

            /* TODO: String truncation */
            if(trunc && len > truncsize) {
                char msg[128];
                printIndent(out, converter, indent);
                sprintf(msg, "// WARNING: this resource, size %li is truncated to %li\n",
                        (long)len, (long)(truncsize/2));
                printCString(out, converter, msg, -1);
                len = truncsize/2;
            }
            printIndent(out, converter, indent);
            if(key != NULL) {
                static const UChar openStr[] = { 0x0020, 0x007B, 0x0020, 0x0022 }; /* " { \"" */
                static const UChar closeStr[] = { 0x0022, 0x0020, 0x007D }; /* "\" }" */
                printCString(out, converter, key, (int32_t)uprv_strlen(key));
                printString(out, converter, openStr, (int32_t)(sizeof(openStr)/sizeof(*openStr)));
                printString(out, converter, string, len);
                printString(out, converter, closeStr, (int32_t)(sizeof(closeStr) / sizeof(*closeStr)));
            } else {
                static const UChar openStr[] = { 0x0022 }; /* "\"" */
                static const UChar closeStr[] = { 0x0022, 0x002C }; /* "\"," */

                printString(out, converter, openStr, (int32_t)(sizeof(openStr) / sizeof(*openStr)));
                printString(out, converter, string, (int32_t)(u_strlen(string)));
                printString(out, converter, closeStr, (int32_t)(sizeof(closeStr) / sizeof(*closeStr)));
            }

            if(verbose) {
                printCString(out, converter, "// STRING", -1);
            }
            printString(out, converter, cr, (int32_t)(sizeof(cr) / sizeof(*cr)));

            uprv_free(string);
        }
        break;

    case URES_INT :
        {
            static const UChar openStr[] = { 0x003A, 0x0069, 0x006E, 0x0074, 0x0020, 0x007B, 0x0020 }; /* ":int { " */
            static const UChar closeStr[] = { 0x0020, 0x007D }; /* " }" */
            UChar num[20];

            printIndent(out, converter, indent);
            if(key != NULL) {
                printCString(out, converter, key, -1);
            }
            printString(out, converter, openStr, (int32_t)(sizeof(openStr) / sizeof(*openStr)));
            uprv_itou(num, 20, ures_getInt(resource, status), 10, 0);
            printString(out, converter, num, u_strlen(num));
            printString(out, converter, closeStr, (int32_t)(sizeof(closeStr) / sizeof(*closeStr)));

            if(verbose) {
                printCString(out, converter, "// INT", -1);
            }
            printString(out, converter, cr, (int32_t)(sizeof(cr) / sizeof(*cr)));
            break;
        }
    case URES_BINARY :
        {
            int32_t len = 0;
            const int8_t *data = (const int8_t *)ures_getBinary(resource, &len, status);
            if(trunc && len > truncsize) {
                char msg[128];
                printIndent(out, converter, indent);
                sprintf(msg, "// WARNING: this resource, size %li is truncated to %li\n",
                        (long)len, (long)(truncsize/2));
                printCString(out, converter, msg, -1);
                len = truncsize;
            }
            if(U_SUCCESS(*status)) {
                static const UChar openStr[] = { 0x003A, 0x0062, 0x0069, 0x006E, 0x0061, 0x0072, 0x0079, 0x0020, 0x007B, 0x0020 }; /* ":binary { " */
                static const UChar closeStr[] = { 0x0020, 0x007D, 0x0020 }; /* " } " */
                printIndent(out, converter, indent);
                if(key != NULL) {
                    printCString(out, converter, key, -1);
                }
                printString(out, converter, openStr, (int32_t)(sizeof(openStr) / sizeof(*openStr)));
                for(i = 0; i<len; i++) {
                    printHex(out, converter, *data++);
                }
                printString(out, converter, closeStr, (int32_t)(sizeof(closeStr) / sizeof(*closeStr)));
                if(verbose) {
                    printCString(out, converter, " // BINARY", -1);
                }
                printString(out, converter, cr, (int32_t)(sizeof(cr) / sizeof(*cr)));
            } else {
                reportError(pname, status, "getting binary value");
            }
        }
        break;
    case URES_INT_VECTOR :
        {
            int32_t len = 0;
            const int32_t *data = ures_getIntVector(resource, &len, status);
            if(U_SUCCESS(*status)) {
                static const UChar openStr[] = { 0x003A, 0x0069, 0x006E, 0x0074, 0x0076, 0x0065, 0x0063, 0x0074, 0x006F, 0x0072, 0x0020, 0x007B, 0x0020 }; /* ":intvector { " */
                static const UChar closeStr[] = { 0x0020, 0x007D, 0x0020 }; /* " } " */
                UChar num[20];

                printIndent(out, converter, indent);
                if(key != NULL) {
                    printCString(out, converter, key, -1);
                }
                printString(out, converter, openStr, (int32_t)(sizeof(openStr) / sizeof(*openStr)));
                for(i = 0; i < len - 1; i++) {
                    int32_t numLen =  uprv_itou(num, 20, data[i], 10, 0);
                    num[numLen++] = 0x002C; /* ',' */
                    num[numLen++] = 0x0020; /* ' ' */
                    num[numLen] = 0;
                    printString(out, converter, num, u_strlen(num));
                }
                if(len > 0) {
                    uprv_itou(num, 20, data[len - 1], 10, 0);
                    printString(out, converter, num, u_strlen(num));
                }
                printString(out, converter, closeStr, (int32_t)(sizeof(closeStr) / sizeof(*closeStr)));
                if(verbose) {
                    printCString(out, converter, "// INTVECTOR", -1);
                }
                printString(out, converter, cr, (int32_t)(sizeof(cr) / sizeof(*cr)));
            } else {
                reportError(pname, status, "getting int vector");
            }
      }
      break;
    case URES_TABLE :
    case URES_ARRAY :
        {
            static const UChar openStr[] = { 0x007B }; /* "{" */
            static const UChar closeStr[] = { 0x007D, '\n' }; /* "}\n" */

            UResourceBundle *t = NULL;
            ures_resetIterator(resource);
            printIndent(out, converter, indent);
            if(key != NULL) {
                printCString(out, converter, key, -1);
            }
            printString(out, converter, openStr, (int32_t)(sizeof(openStr) / sizeof(*openStr)));
            if(verbose) {
                if(ures_getType(resource) == URES_TABLE) {
                    printCString(out, converter, "// TABLE", -1);
                } else {
                    printCString(out, converter, "// ARRAY", -1);
                }
            }
            printString(out, converter, cr, (int32_t)(sizeof(cr) / sizeof(*cr)));

            if(suppressAliases == FALSE) {
              while(U_SUCCESS(*status) && ures_hasNext(resource)) {
                  t = ures_getNextResource(resource, t, status);
                  if(U_SUCCESS(*status)) {
                    printOutBundle(out, converter, t, indent+indentsize, pname, status);
                  } else {
                    reportError(pname, status, "While processing table");
                    *status = U_ZERO_ERROR;
                  }
              }
            } else { /* we have to use low level access to do this */
              Resource r;
              int32_t resSize = ures_getSize(resource);
              UBool isTable = (UBool)(ures_getType(resource) == URES_TABLE);
              for(i = 0; i < resSize; i++) {
                /* need to know if it's an alias */
                if(isTable) {
                  r = res_getTableItemByIndex(&resource->fResData, resource->fRes, i, &key);
                } else {
                  r = res_getArrayItem(&resource->fResData, resource->fRes, i);
                }
                if(U_SUCCESS(*status)) {
                  if(res_getPublicType(r) == URES_ALIAS) {
                    printOutAlias(out, converter, resource, r, key, indent+indentsize, pname, status);
                  } else {
                    t = ures_getByIndex(resource, i, t, status);
                    printOutBundle(out, converter, t, indent+indentsize, pname, status);
                  }
                } else {
                  reportError(pname, status, "While processing table");
                  *status = U_ZERO_ERROR;
                }
              }
            }

            printIndent(out, converter, indent);
            printString(out, converter, closeStr, (int32_t)(sizeof(closeStr) / sizeof(*closeStr)));
            ures_close(t);
        }
        break;
    default:
        break;
    }

}

static const char *getEncodingName(const char *encoding) {
    UErrorCode err;
    const char *enc;

    err = U_ZERO_ERROR;
    if (!(enc = ucnv_getStandardName(encoding, "MIME", &err))) {
        err = U_ZERO_ERROR;
        if (!(enc = ucnv_getStandardName(encoding, "IANA", &err))) {
            ;
        }
    }

    return enc;
}

static void reportError(const char *pname, UErrorCode *status, const char *when) {
    fprintf(stderr, "%s: error %d while %s: %s\n", pname, *status, when, u_errorName(*status));
}

/*
 * Local Variables:
 * indent-tabs-mode: nil
 * End:
 */


