/*
*******************************************************************************
*
*   Copyright (C) 2000-2009, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*
* File reslist.h
*
* Modification History:
*
*   Date        Name        Description
*   02/21/00    weiv        Creation.
*******************************************************************************
*/

#ifndef RESLIST_H
#define RESLIST_H

#define KEY_SPACE_SIZE 65536
#define RESLIST_MAX_INT_VECTOR 2048

#include "unicode/utypes.h"
#include "unicode/ures.h"
#include "unicode/ustring.h"
#include "uresdata.h"
#include "cmemory.h"
#include "cstring.h"
#include "unewdata.h"
#include "ustr.h"
#include "uhash.h"

U_CDECL_BEGIN

typedef struct KeyMapEntry {
    int32_t oldpos, newpos;
} KeyMapEntry;

/* Resource bundle root table */
struct SRBRoot {
  struct SResource *fRoot;
  char *fLocale;
  int32_t fIndexLength;
  int32_t fMaxTableLength;
  UBool noFallback; /* see URES_ATT_NO_FALLBACK */
  int8_t fStringsForm; /* default STRINGS_UTF16_V1 */
  UBool fIsPoolBundle;

  char *fKeys;
  KeyMapEntry *fKeyMap;
  int32_t fKeysBottom, fKeysTop;
  int32_t fKeysCapacity;
  int32_t fKeysCount;
  int32_t fLocalKeyLimit; /* key offset < limit fits into URES_TABLE */

  UHashtable *fStringSet;
  uint16_t *f16BitUnits;
  int32_t f16BitUnitsCapacity;
  int32_t f16BitUnitsLength;

  const char *fPoolBundleKeys;
  int32_t fPoolBundleKeysLength;
  int32_t fPoolBundleKeysCount;
  int32_t fPoolChecksum;
};

struct SRBRoot *bundle_open(const struct UString* comment, UBool isPoolBundle, UErrorCode *status);
void bundle_write(struct SRBRoot *bundle, const char *outputDir, const char *outputPkg, char *writtenFilename, int writtenFilenameLen, UErrorCode *status);

/* write a java resource file */
void bundle_write_java(struct SRBRoot *bundle, const char *outputDir, const char* outputEnc, char *writtenFilename, 
                       int writtenFilenameLen, const char* packageName, const char* bundleName, UErrorCode *status);

/* write a xml resource file */
/* commented by Jing*/
/* void bundle_write_xml(struct SRBRoot *bundle, const char *outputDir,const char* outputEnc, 
                  char *writtenFilename, int writtenFilenameLen,UErrorCode *status); */

/* added by Jing*/
void bundle_write_xml(struct SRBRoot *bundle, const char *outputDir,const char* outputEnc, const char* rbname,
                  char *writtenFilename, int writtenFilenameLen, const char* language, const char* package, UErrorCode *status);

void bundle_close(struct SRBRoot *bundle, UErrorCode *status);
void bundle_setlocale(struct SRBRoot *bundle, UChar *locale, UErrorCode *status);
int32_t bundle_addtag(struct SRBRoot *bundle, const char *tag, UErrorCode *status);

const char *
bundle_getKeyBytes(struct SRBRoot *bundle, int32_t *pLength);

int32_t
bundle_addKeyBytes(struct SRBRoot *bundle, const char *keyBytes, int32_t length, UErrorCode *status);

void
bundle_compactKeys(struct SRBRoot *bundle, UErrorCode *status);

/* Various resource types */

/*
 * Return a unique pointer to a dummy object,
 * for use in non-error cases when no resource is to be added to the bundle.
 * (NULL is used in error cases.)
 */
struct SResource* res_none(void);

struct SResTable {
    uint32_t fCount;
    int8_t fType;  /* determined by table_write16() for table_preWrite() & table_write() */
    struct SResource *fFirst;
    struct SRBRoot *fRoot;
};

struct SResource* table_open(struct SRBRoot *bundle, const char *tag, const struct UString* comment, UErrorCode *status);
void table_add(struct SResource *table, struct SResource *res, int linenumber, UErrorCode *status);

struct SResArray {
    uint32_t fCount;
    struct SResource *fFirst;
    struct SResource *fLast;
};

struct SResource* array_open(struct SRBRoot *bundle, const char *tag, const struct UString* comment, UErrorCode *status);
void array_add(struct SResource *array, struct SResource *res, UErrorCode *status);

struct SResString {
    struct SResource *fSame;  /* used for duplicates */
    UChar *fChars;
    int32_t fLength;
    int32_t fSuffixOffset;  /* this string is a suffix of fSame at this offset */
    int8_t fNumCharsForLength;
};

struct SResource *string_open(struct SRBRoot *bundle, char *tag, const UChar *value, int32_t len, const struct UString* comment, UErrorCode *status);

/**
 * Remove a string from a bundle and close (delete) it.
 * The string must not have been added to a table or array yet.
 * This function only undoes what string_open() did.
 */
void bundle_closeString(struct SRBRoot *bundle, struct SResource *string);

struct SResource *alias_open(struct SRBRoot *bundle, char *tag, UChar *value, int32_t len, const struct UString* comment, UErrorCode *status);

struct SResIntVector {
    uint32_t fCount;
    uint32_t *fArray;
};

struct SResource* intvector_open(struct SRBRoot *bundle, char *tag,  const struct UString* comment, UErrorCode *status);
void intvector_add(struct SResource *intvector, int32_t value, UErrorCode *status);

struct SResInt {
    uint32_t fValue;
};

struct SResource *int_open(struct SRBRoot *bundle, char *tag, int32_t value, const struct UString* comment, UErrorCode *status);

struct SResBinary {
    uint32_t fLength;
    uint8_t *fData;
    char* fFileName; /* file name for binary or import binary tags if any */
};

struct SResource *bin_open(struct SRBRoot *bundle, const char *tag, uint32_t length, uint8_t *data, const char* fileName, const struct UString* comment, UErrorCode *status);

/* Resource place holder */

struct SResource {
    int8_t   fType;     /* nominal type: fRes (when != 0xffffffff) may use subtype */
    UBool    fWritten;  /* res_write() can exit early */
    uint32_t fRes;      /* resource item word; 0xffffffff if not known yet */
    int32_t  fKey;      /* Index into bundle->fKeys; -1 if no key. */
    int      line;      /* used internally to report duplicate keys in tables */
    struct SResource *fNext; /*This is for internal chaining while building*/
    struct UString fComment;
    union {
        struct SResTable fTable;
        struct SResArray fArray;
        struct SResString fString;
        struct SResIntVector fIntVector;
        struct SResInt fIntValue;
        struct SResBinary fBinaryValue;
    } u;
};

const char *
res_getKeyString(const struct SRBRoot *bundle, const struct SResource *res, char temp[8]);

void res_close(struct SResource *res);

void setIncludeCopyright(UBool val);
UBool getIncludeCopyright(void);

void setFormatVersion(int32_t formatVersion);

void setUsePoolBundle(UBool use);

/* in wrtxml.cpp */
uint32_t computeCRC(char *ptr, uint32_t len, uint32_t lastcrc);

U_CDECL_END
#endif /* #ifndef RESLIST_H */
