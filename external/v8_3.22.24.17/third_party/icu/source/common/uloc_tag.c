/*
**********************************************************************
*   Copyright (C) 2009-2010, International Business Machines
*   Corporation and others.  All Rights Reserved.
**********************************************************************
*/

#include "unicode/utypes.h"
#include "unicode/ures.h"
#include "unicode/putil.h"
#include "unicode/uloc.h"
#include "ustr_imp.h"
#include "cmemory.h"
#include "cstring.h"
#include "putilimp.h"
#include "uinvchar.h"

/* struct holding a single variant */
typedef struct VariantListEntry {
    const char              *variant;
    struct VariantListEntry *next;
} VariantListEntry;

/* struct holding a single extension */
typedef struct ExtensionListEntry {
    const char                  *key;
    const char                  *value;
    struct ExtensionListEntry   *next;
} ExtensionListEntry;

#define MAXEXTLANG 3
typedef struct ULanguageTag {
    char                *buf;   /* holding parsed subtags */
    const char          *language;
    const char          *extlang[MAXEXTLANG];
    const char          *script;
    const char          *region;
    VariantListEntry    *variants;
    ExtensionListEntry  *extensions;
    const char          *privateuse;
    const char          *grandfathered;
} ULanguageTag;

#define MINLEN 2
#define SEP '-'
#define PRIVATEUSE 'x'
#define LDMLEXT 'u'

#define LOCALE_SEP '_'
#define LOCALE_EXT_SEP '@'
#define LOCALE_KEYWORD_SEP ';'
#define LOCALE_KEY_TYPE_SEP '='

#define ISALPHA(c) (((c)>='A' && (c)<='Z') || ((c)>='a' && (c)<='z'))
#define ISNUMERIC(c) ((c)>='0' && (c)<='9')

static const char* EMPTY = "";
static const char* LANG_UND = "und";
static const char* PRIVATEUSE_KEY = "x";
static const char* _POSIX = "_POSIX";
static const char* POSIX_KEY = "va";
static const char* POSIX_VALUE = "posix";

#define LANG_UND_LEN 3

static const char* GRANDFATHERED[] = {
/*  grandfathered   preferred */
    "art-lojban",   "jbo",
    "cel-gaulish",  "",
    "en-GB-oed",    "",
    "i-ami",        "ami",
    "i-bnn",        "bnn",
    "i-default",    "",
    "i-enochian",   "",
    "i-hak",        "hak",
    "i-klingon",    "tlh",
    "i-lux",        "lb",
    "i-mingo",      "",
    "i-navajo",     "nv",
    "i-pwn",        "pwn",
    "i-tao",        "tao",
    "i-tay",        "tay",
    "i-tsu",        "tsu",
    "no-bok",       "nb",
    "no-nyn",       "nn",
    "sgn-be-fr",    "sfb",
    "sgn-be-nl",    "vgt",
    "sgn-ch-de",    "sgg",
    "zh-guoyu",     "cmn",
    "zh-hakka",     "hak",
    "zh-min",       "",
    "zh-min-nan",   "nan",
    "zh-xiang",     "hsn",
    NULL,           NULL
};

static const char* DEPRECATEDLANGS[] = {
/*  deprecated  new */
    "iw",       "he",
    "ji",       "yi",
    "in",       "id",
    NULL,       NULL
};

/*
* -------------------------------------------------
*
* These ultag_ functions may be exposed as APIs later
*
* -------------------------------------------------
*/

static ULanguageTag*
ultag_parse(const char* tag, int32_t tagLen, int32_t* parsedLen, UErrorCode* status);

static void
ultag_close(ULanguageTag* langtag);

static const char*
ultag_getLanguage(const ULanguageTag* langtag);

#if 0
static const char*
ultag_getJDKLanguage(const ULanguageTag* langtag);
#endif

static const char*
ultag_getExtlang(const ULanguageTag* langtag, int32_t idx);

static int32_t
ultag_getExtlangSize(const ULanguageTag* langtag);

static const char*
ultag_getScript(const ULanguageTag* langtag);

static const char*
ultag_getRegion(const ULanguageTag* langtag);

static const char*
ultag_getVariant(const ULanguageTag* langtag, int32_t idx);

static int32_t
ultag_getVariantsSize(const ULanguageTag* langtag);

static const char*
ultag_getExtensionKey(const ULanguageTag* langtag, int32_t idx);

static const char*
ultag_getExtensionValue(const ULanguageTag* langtag, int32_t idx);

static int32_t
ultag_getExtensionsSize(const ULanguageTag* langtag);

static const char*
ultag_getPrivateUse(const ULanguageTag* langtag);

#if 0
static const char*
ultag_getGrandfathered(const ULanguageTag* langtag);
#endif

/*
* -------------------------------------------------
*
* Language subtag syntax validation functions
*
* -------------------------------------------------
*/

static UBool
_isAlphaString(const char* s, int32_t len) {
    int32_t i;
    for (i = 0; i < len; i++) {
        if (!ISALPHA(*(s + i))) {
            return FALSE;
        }
    }
    return TRUE;
}

static UBool
_isNumericString(const char* s, int32_t len) {
    int32_t i;
    for (i = 0; i < len; i++) {
        if (!ISNUMERIC(*(s + i))) {
            return FALSE;
        }
    }
    return TRUE;
}

static UBool
_isAlphaNumericString(const char* s, int32_t len) {
    int32_t i;
    for (i = 0; i < len; i++) {
        if (!ISALPHA(*(s + i)) && !ISNUMERIC(*(s + i))) {
            return FALSE;
        }
    }
    return TRUE;
}

static UBool
_isLanguageSubtag(const char* s, int32_t len) {
    /*
     * language      = 2*3ALPHA            ; shortest ISO 639 code
     *                 ["-" extlang]       ; sometimes followed by
     *                                     ;   extended language subtags
     *               / 4ALPHA              ; or reserved for future use
     *               / 5*8ALPHA            ; or registered language subtag
     */
    if (len < 0) {
        len = (int32_t)uprv_strlen(s);
    }
    if (len >= 2 && len <= 8 && _isAlphaString(s, len)) {
        return TRUE;
    }
    return FALSE;
}

static UBool
_isExtlangSubtag(const char* s, int32_t len) {
    /*
     * extlang       = 3ALPHA              ; selected ISO 639 codes
     *                 *2("-" 3ALPHA)      ; permanently reserved
     */
    if (len < 0) {
        len = (int32_t)uprv_strlen(s);
    }
    if (len == 3 && _isAlphaString(s, len)) {
        return TRUE;
    }
    return FALSE;
}

static UBool
_isScriptSubtag(const char* s, int32_t len) {
    /*
     * script        = 4ALPHA              ; ISO 15924 code
     */
    if (len < 0) {
        len = (int32_t)uprv_strlen(s);
    }
    if (len == 4 && _isAlphaString(s, len)) {
        return TRUE;
    }
    return FALSE;
}

static UBool
_isRegionSubtag(const char* s, int32_t len) {
    /*
     * region        = 2ALPHA              ; ISO 3166-1 code
     *               / 3DIGIT              ; UN M.49 code
     */
    if (len < 0) {
        len = (int32_t)uprv_strlen(s);
    }
    if (len == 2 && _isAlphaString(s, len)) {
        return TRUE;
    }
    if (len == 3 && _isNumericString(s, len)) {
        return TRUE;
    }
    return FALSE;
}

static UBool
_isVariantSubtag(const char* s, int32_t len) {
    /*
     * variant       = 5*8alphanum         ; registered variants
     *               / (DIGIT 3alphanum)
     */
    if (len < 0) {
        len = (int32_t)uprv_strlen(s);
    }
    if (len >= 5 && len <= 8 && _isAlphaString(s, len)) {
        return TRUE;
    }
    if (len == 4 && ISNUMERIC(*s) && _isAlphaNumericString(s + 1, 3)) {
        return TRUE;
    }
    return FALSE;
}

static UBool
_isExtensionSingleton(const char* s, int32_t len) {
    /*
     * extension     = singleton 1*("-" (2*8alphanum))
     */
    if (len < 0) {
        len = (int32_t)uprv_strlen(s);
    }
    if (len == 1 && ISALPHA(*s) && (uprv_tolower(*s) != PRIVATEUSE)) {
        return TRUE;
    }
    return FALSE;
}

static UBool
_isExtensionSubtag(const char* s, int32_t len) {
    /*
     * extension     = singleton 1*("-" (2*8alphanum))
     */
    if (len < 0) {
        len = (int32_t)uprv_strlen(s);
    }
    if (len >= 2 && len <= 8 && _isAlphaNumericString(s, len)) {
        return TRUE;
    }
    return FALSE;
}

static UBool
_isExtensionSubtags(const char* s, int32_t len) {
    const char *p = s;
    const char *pSubtag = NULL;

    if (len < 0) {
        len = (int32_t)uprv_strlen(s);
    }

    while ((p - s) < len) {
        if (*p == SEP) {
            if (pSubtag == NULL) {
                return FALSE;
            }
            if (!_isExtensionSubtag(pSubtag, (int32_t)(p - pSubtag))) {
                return FALSE;
            }
            pSubtag = NULL;
        } else if (pSubtag == NULL) {
            pSubtag = p;
        }
        p++;
    }
    if (pSubtag == NULL) {
        return FALSE;
    }
    return _isExtensionSubtag(pSubtag, (int32_t)(p - pSubtag));
}

static UBool
_isPrivateuseValueSubtag(const char* s, int32_t len) {
    /*
     * privateuse    = "x" 1*("-" (1*8alphanum))
     */
    if (len < 0) {
        len = (int32_t)uprv_strlen(s);
    }
    if (len >= 1 && len <= 8 && _isAlphaNumericString(s, len)) {
        return TRUE;
    }
    return FALSE;
}

static UBool
_isPrivateuseValueSubtags(const char* s, int32_t len) {
    const char *p = s;
    const char *pSubtag = NULL;

    if (len < 0) {
        len = (int32_t)uprv_strlen(s);
    }

    while ((p - s) < len) {
        if (*p == SEP) {
            if (pSubtag == NULL) {
                return FALSE;
            }
            if (!_isPrivateuseValueSubtag(pSubtag, (int32_t)(p - pSubtag))) {
                return FALSE;
            }
            pSubtag = NULL;
        } else if (pSubtag == NULL) {
            pSubtag = p;
        }
        p++;
    }
    if (pSubtag == NULL) {
        return FALSE;
    }
    return _isPrivateuseValueSubtag(pSubtag, (int32_t)(p - pSubtag));
}

static UBool
_isLDMLKey(const char* s, int32_t len) {
    if (len < 0) {
        len = (int32_t)uprv_strlen(s);
    }
    if (len == 2 && _isAlphaNumericString(s, len)) {
        return TRUE;
    }
    return FALSE;
}

static UBool
_isLDMLType(const char* s, int32_t len) {
    if (len < 0) {
        len = (int32_t)uprv_strlen(s);
    }
    if (len >= 3 && len <= 8 && _isAlphaNumericString(s, len)) {
        return TRUE;
    }
    return FALSE;
}

/*
* -------------------------------------------------
*
* Helper functions
*
* -------------------------------------------------
*/

static UBool
_addVariantToList(VariantListEntry **first, VariantListEntry *var) {
    UBool bAdded = TRUE;

    if (*first == NULL) {
        var->next = NULL;
        *first = var;
    } else {
        VariantListEntry *prev, *cur;
        int32_t cmp;

        /* reorder variants in alphabetical order */
        prev = NULL;
        cur = *first;
        while (TRUE) {
            if (cur == NULL) {
                prev->next = var;
                var->next = NULL;
                break;
            }
            cmp = uprv_compareInvCharsAsAscii(var->variant, cur->variant);
            if (cmp < 0) {
                if (prev == NULL) {
                    *first = var;
                } else {
                    prev->next = var;
                }
                var->next = cur;
                break;
            }
            if (cmp == 0) {
                /* duplicated variant */
                bAdded = FALSE;
                break;
            }
            prev = cur;
            cur = cur->next;
        }
    }

    return bAdded;
}


static UBool
_addExtensionToList(ExtensionListEntry **first, ExtensionListEntry *ext, UBool localeToBCP) {
    UBool bAdded = TRUE;

    if (*first == NULL) {
        ext->next = NULL;
        *first = ext;
    } else {
        ExtensionListEntry *prev, *cur;
        int32_t cmp;

        /* reorder variants in alphabetical order */
        prev = NULL;
        cur = *first;
        while (TRUE) {
            if (cur == NULL) {
                prev->next = ext;
                ext->next = NULL;
                break;
            }
            if (localeToBCP) {
                /* special handling for locale to bcp conversion */
                int32_t len, curlen;

                len = (int32_t)uprv_strlen(ext->key);
                curlen = (int32_t)uprv_strlen(cur->key);

                if (len == 1 && curlen == 1) {
                    if (*(ext->key) == *(cur->key)) {
                        cmp = 0;
                    } else if (*(ext->key) == PRIVATEUSE) {
                        cmp = 1;
                    } else if (*(cur->key) == PRIVATEUSE) {
                        cmp = -1;
                    } else {
                        cmp = *(ext->key) - *(cur->key);
                    }
                } else if (len == 1) {
                    cmp = *(ext->key) - LDMLEXT; 
                } else if (curlen == 1) {
                    cmp = LDMLEXT - *(cur->key);
                } else {
                    cmp = uprv_compareInvCharsAsAscii(ext->key, cur->key);
                }
            } else {
                cmp = uprv_compareInvCharsAsAscii(ext->key, cur->key);
            }
            if (cmp < 0) {
                if (prev == NULL) {
                    *first = ext;
                } else {
                    prev->next = ext;
                }
                ext->next = cur;
                break;
            }
            if (cmp == 0) {
                /* duplicated extension key */
                bAdded = FALSE;
                break;
            }
            prev = cur;
            cur = cur->next;
        }
    }

    return bAdded;
}

static void
_initializeULanguageTag(ULanguageTag* langtag) {
    int32_t i;

    langtag->buf = NULL;

    langtag->language = EMPTY;
    for (i = 0; i < MAXEXTLANG; i++) {
        langtag->extlang[i] = NULL;
    }

    langtag->script = EMPTY;
    langtag->region = EMPTY;

    langtag->variants = NULL;
    langtag->extensions = NULL;

    langtag->grandfathered = EMPTY;
    langtag->privateuse = EMPTY;
}

#define KEYTYPEDATA     "keyTypeData"
#define KEYMAP          "keyMap"
#define TYPEMAP         "typeMap"
#define TYPEALIAS       "typeAlias"
#define MAX_BCP47_SUBTAG_LEN    9   /* including null terminator */
#define MAX_LDML_KEY_LEN        22
#define MAX_LDML_TYPE_LEN       32

static int32_t
_ldmlKeyToBCP47(const char* key, int32_t keyLen,
                char* bcpKey, int32_t bcpKeyCapacity,
                UErrorCode *status) {
    UResourceBundle *rb;
    char keyBuf[MAX_LDML_KEY_LEN];
    char bcpKeyBuf[MAX_BCP47_SUBTAG_LEN];
    int32_t resultLen = 0;
    int32_t i;
    UErrorCode tmpStatus = U_ZERO_ERROR;
    const UChar *uBcpKey;
    int32_t bcpKeyLen;

    if (keyLen < 0) {
        keyLen = (int32_t)uprv_strlen(key);
    }

    if (keyLen >= sizeof(keyBuf)) {
        /* no known valid LDML key exceeding 21 */
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    uprv_memcpy(keyBuf, key, keyLen);
    keyBuf[keyLen] = 0;

    /* to lower case */
    for (i = 0; i < keyLen; i++) {
        keyBuf[i] = uprv_tolower(keyBuf[i]);
    }

    rb = ures_openDirect(NULL, KEYTYPEDATA, status);
    ures_getByKey(rb, KEYMAP, rb, status);

    if (U_FAILURE(*status)) {
        ures_close(rb);
        return 0;
    }

    uBcpKey = ures_getStringByKey(rb, keyBuf, &bcpKeyLen, &tmpStatus);
    if (U_SUCCESS(tmpStatus)) {
        u_UCharsToChars(uBcpKey, bcpKeyBuf, bcpKeyLen);
        bcpKeyBuf[bcpKeyLen] = 0;
        resultLen = bcpKeyLen;
    } else {
        if (_isLDMLKey(key, keyLen)) {
            uprv_memcpy(bcpKeyBuf, key, keyLen);
            bcpKeyBuf[keyLen] = 0;
            resultLen = keyLen;
        } else {
            /* mapping not availabe */
            *status = U_ILLEGAL_ARGUMENT_ERROR;
        }
    }
    ures_close(rb);

    if (U_FAILURE(*status)) {
        return 0;
    }

    uprv_memcpy(bcpKey, bcpKeyBuf, uprv_min(resultLen, bcpKeyCapacity));
    return u_terminateChars(bcpKey, bcpKeyCapacity, resultLen, status);
}

static int32_t
_bcp47ToLDMLKey(const char* bcpKey, int32_t bcpKeyLen,
                char* key, int32_t keyCapacity,
                UErrorCode *status) {
    UResourceBundle *rb;
    char bcpKeyBuf[MAX_BCP47_SUBTAG_LEN];
    int32_t resultLen = 0;
    int32_t i;
    const char *resKey = NULL;
    UResourceBundle *mapData;

    if (bcpKeyLen < 0) {
        bcpKeyLen = (int32_t)uprv_strlen(bcpKey);
    }

    if (bcpKeyLen >= sizeof(bcpKeyBuf)) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    uprv_memcpy(bcpKeyBuf, bcpKey, bcpKeyLen);
    bcpKeyBuf[bcpKeyLen] = 0;

    /* to lower case */
    for (i = 0; i < bcpKeyLen; i++) {
        bcpKeyBuf[i] = uprv_tolower(bcpKeyBuf[i]);
    }

    rb = ures_openDirect(NULL, KEYTYPEDATA, status);
    ures_getByKey(rb, KEYMAP, rb, status);
    if (U_FAILURE(*status)) {
        ures_close(rb);
        return 0;
    }

    mapData = ures_getNextResource(rb, NULL, status);
    while (U_SUCCESS(*status)) {
        const UChar *uBcpKey;
        char tmpBcpKeyBuf[MAX_BCP47_SUBTAG_LEN];
        int32_t tmpBcpKeyLen;

        uBcpKey = ures_getString(mapData, &tmpBcpKeyLen, status);
        if (U_FAILURE(*status)) {
            break;
        }
        u_UCharsToChars(uBcpKey, tmpBcpKeyBuf, tmpBcpKeyLen);
        tmpBcpKeyBuf[tmpBcpKeyLen] = 0;
        if (uprv_compareInvCharsAsAscii(bcpKeyBuf, tmpBcpKeyBuf) == 0) {
            /* found a matching BCP47 key */
            resKey = ures_getKey(mapData);
            resultLen = (int32_t)uprv_strlen(resKey);
            break;
        }
        if (!ures_hasNext(rb)) {
            break;
        }
        ures_getNextResource(rb, mapData, status);
    }
    ures_close(mapData);
    ures_close(rb);

    if (U_FAILURE(*status)) {
        return 0;
    }

    if (resKey == NULL) {
        resKey = bcpKeyBuf;
        resultLen = bcpKeyLen;
    }

    uprv_memcpy(key, resKey, uprv_min(resultLen, keyCapacity));
    return u_terminateChars(key, keyCapacity, resultLen, status);
}

static int32_t
_ldmlTypeToBCP47(const char* key, int32_t keyLen,
                 const char* type, int32_t typeLen,
                 char* bcpType, int32_t bcpTypeCapacity,
                 UErrorCode *status) {
    UResourceBundle *rb, *keyTypeData, *typeMapForKey;
    char keyBuf[MAX_LDML_KEY_LEN];
    char typeBuf[MAX_LDML_TYPE_LEN];
    char bcpTypeBuf[MAX_BCP47_SUBTAG_LEN];
    int32_t resultLen = 0;
    int32_t i;
    UErrorCode tmpStatus = U_ZERO_ERROR;
    const UChar *uBcpType, *uCanonicalType;
    int32_t bcpTypeLen, canonicalTypeLen;
    UBool isTimezone = FALSE;

    if (keyLen < 0) {
        keyLen = (int32_t)uprv_strlen(key);
    }
    if (keyLen >= sizeof(keyBuf)) {
        /* no known valid LDML key exceeding 21 */
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }
    uprv_memcpy(keyBuf, key, keyLen);
    keyBuf[keyLen] = 0;

    /* to lower case */
    for (i = 0; i < keyLen; i++) {
        keyBuf[i] = uprv_tolower(keyBuf[i]);
    }
    if (uprv_compareInvCharsAsAscii(keyBuf, "timezone") == 0) {
        isTimezone = TRUE;
    }

    if (typeLen < 0) {
        typeLen = (int32_t)uprv_strlen(type);
    }
    if (typeLen >= sizeof(typeBuf)) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    if (isTimezone) {
        /* replace '/' with ':' */
        for (i = 0; i < typeLen; i++) {
            if (*(type + i) == '/') {
                typeBuf[i] = ':';
            } else {
                typeBuf[i] = *(type + i);
            }
        }
        typeBuf[typeLen] = 0;
        type = &typeBuf[0];
    }

    keyTypeData = ures_openDirect(NULL, KEYTYPEDATA, status);
    rb = ures_getByKey(keyTypeData, TYPEMAP, NULL, status);
    if (U_FAILURE(*status)) {
        ures_close(rb);
        ures_close(keyTypeData);
        return 0;
    }

    typeMapForKey = ures_getByKey(rb, keyBuf, NULL, &tmpStatus);
    uBcpType = ures_getStringByKey(typeMapForKey, type, &bcpTypeLen, &tmpStatus);
    if (U_SUCCESS(tmpStatus)) {
        u_UCharsToChars(uBcpType, bcpTypeBuf, bcpTypeLen);
        resultLen = bcpTypeLen;
    } else if (tmpStatus == U_MISSING_RESOURCE_ERROR) {
        /* is this type alias? */
        tmpStatus = U_ZERO_ERROR;
        ures_getByKey(keyTypeData, TYPEALIAS, rb, &tmpStatus);
        ures_getByKey(rb, keyBuf, rb, &tmpStatus);
        uCanonicalType = ures_getStringByKey(rb, type, &canonicalTypeLen, &tmpStatus);
        if (U_SUCCESS(tmpStatus)) {
            u_UCharsToChars(uCanonicalType, typeBuf, canonicalTypeLen);
            if (isTimezone) {
                /* replace '/' with ':' */
                for (i = 0; i < canonicalTypeLen; i++) {
                    if (typeBuf[i] == '/') {
                        typeBuf[i] = ':';
                    }
                }
            }
            typeBuf[canonicalTypeLen] = 0;

            /* look up the canonical type */
            uBcpType = ures_getStringByKey(typeMapForKey, typeBuf, &bcpTypeLen, &tmpStatus);
            if (U_SUCCESS(tmpStatus)) {
                u_UCharsToChars(uBcpType, bcpTypeBuf, bcpTypeLen);
                resultLen = bcpTypeLen;
            }
        }
        if (tmpStatus == U_MISSING_RESOURCE_ERROR) {
            if (_isLDMLType(type, typeLen)) {
                uprv_memcpy(bcpTypeBuf, type, typeLen);
                resultLen = typeLen;
            } else {
                /* mapping not availabe */
                *status = U_ILLEGAL_ARGUMENT_ERROR;
            }
        }
    } else {
        *status = tmpStatus;
    }
    ures_close(rb);
    ures_close(typeMapForKey);
    ures_close(keyTypeData);

    if (U_FAILURE(*status)) {
        return 0;
    }

    uprv_memcpy(bcpType, bcpTypeBuf, uprv_min(resultLen, bcpTypeCapacity));
    return u_terminateChars(bcpType, bcpTypeCapacity, resultLen, status);
}

static int32_t
_bcp47ToLDMLType(const char* key, int32_t keyLen,
                 const char* bcpType, int32_t bcpTypeLen,
                 char* type, int32_t typeCapacity,
                 UErrorCode *status) {
    UResourceBundle *rb;
    char keyBuf[MAX_LDML_KEY_LEN];
    char bcpTypeBuf[MAX_BCP47_SUBTAG_LEN];
    int32_t resultLen = 0;
    int32_t i;
    const char *resType = NULL;
    UResourceBundle *mapData;
    UErrorCode tmpStatus = U_ZERO_ERROR;
    int32_t copyLen;

    if (keyLen < 0) {
        keyLen = (int32_t)uprv_strlen(key);
    }

    if (keyLen >= sizeof(keyBuf)) {
        /* no known valid LDML key exceeding 21 */
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }
    uprv_memcpy(keyBuf, key, keyLen);
    keyBuf[keyLen] = 0;

    /* to lower case */
    for (i = 0; i < keyLen; i++) {
        keyBuf[i] = uprv_tolower(keyBuf[i]);
    }


    if (bcpTypeLen < 0) {
        bcpTypeLen = (int32_t)uprv_strlen(bcpType);
    }

    if (bcpTypeLen >= sizeof(bcpTypeBuf)) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    uprv_memcpy(bcpTypeBuf, bcpType, bcpTypeLen);
    bcpTypeBuf[bcpTypeLen] = 0;

    /* to lower case */
    for (i = 0; i < bcpTypeLen; i++) {
        bcpTypeBuf[i] = uprv_tolower(bcpTypeBuf[i]);
    }

    rb = ures_openDirect(NULL, KEYTYPEDATA, status);
    ures_getByKey(rb, TYPEMAP, rb, status);
    if (U_FAILURE(*status)) {
        ures_close(rb);
        return 0;
    }

    ures_getByKey(rb, keyBuf, rb, &tmpStatus);
    mapData = ures_getNextResource(rb, NULL, &tmpStatus);
    while (U_SUCCESS(tmpStatus)) {
        const UChar *uBcpType;
        char tmpBcpTypeBuf[MAX_BCP47_SUBTAG_LEN];
        int32_t tmpBcpTypeLen;

        uBcpType = ures_getString(mapData, &tmpBcpTypeLen, &tmpStatus);
        if (U_FAILURE(tmpStatus)) {
            break;
        }
        u_UCharsToChars(uBcpType, tmpBcpTypeBuf, tmpBcpTypeLen);
        tmpBcpTypeBuf[tmpBcpTypeLen] = 0;
        if (uprv_compareInvCharsAsAscii(bcpTypeBuf, tmpBcpTypeBuf) == 0) {
            /* found a matching BCP47 type */
            resType = ures_getKey(mapData);
            resultLen = (int32_t)uprv_strlen(resType);
            break;
        }
        if (!ures_hasNext(rb)) {
            break;
        }
        ures_getNextResource(rb, mapData, &tmpStatus);
    }
    ures_close(mapData);
    ures_close(rb);

    if (U_FAILURE(tmpStatus) && tmpStatus != U_MISSING_RESOURCE_ERROR) {
        *status = tmpStatus;
        return 0;
    }

    if (resType == NULL) {
        resType = bcpTypeBuf;
        resultLen = bcpTypeLen;
    }

    copyLen = uprv_min(resultLen, typeCapacity);
    uprv_memcpy(type, resType, copyLen);

    if (uprv_compareInvCharsAsAscii(keyBuf, "timezone") == 0) {
        for (i = 0; i < copyLen; i++) {
            if (*(type + i) == ':') {
                *(type + i) = '/';
            }
        }
    }

    return u_terminateChars(type, typeCapacity, resultLen, status);
}

static int32_t
_appendLanguageToLanguageTag(const char* localeID, char* appendAt, int32_t capacity, UBool strict, UErrorCode* status) {
    char buf[ULOC_LANG_CAPACITY];
    UErrorCode tmpStatus = U_ZERO_ERROR;
    int32_t len, i;
    int32_t reslen = 0;

    if (U_FAILURE(*status)) {
        return 0;
    }

    len = uloc_getLanguage(localeID, buf, sizeof(buf), &tmpStatus);
    if (U_FAILURE(tmpStatus) || tmpStatus == U_STRING_NOT_TERMINATED_WARNING) {
        if (strict) {
            *status = U_ILLEGAL_ARGUMENT_ERROR;
            return 0;
        }
        len = 0;
    }

    /* Note: returned language code is in lower case letters */

    if (len == 0) {
        if (reslen < capacity) {
            uprv_memcpy(appendAt + reslen, LANG_UND, uprv_min(LANG_UND_LEN, capacity - reslen));
        }
        reslen += LANG_UND_LEN;
    } else if (!_isLanguageSubtag(buf, len)) {
            /* invalid language code */
        if (strict) {
            *status = U_ILLEGAL_ARGUMENT_ERROR;
            return 0;
        }
        if (reslen < capacity) {
            uprv_memcpy(appendAt + reslen, LANG_UND, uprv_min(LANG_UND_LEN, capacity - reslen));
        }
        reslen += LANG_UND_LEN;
    } else {
        /* resolve deprecated */
        for (i = 0; DEPRECATEDLANGS[i] != NULL; i += 2) {
            if (uprv_compareInvCharsAsAscii(buf, DEPRECATEDLANGS[i]) == 0) {
                uprv_strcpy(buf, DEPRECATEDLANGS[i + 1]);
                len = (int32_t)uprv_strlen(buf);
                break;
            }
        }
        if (reslen < capacity) {
            uprv_memcpy(appendAt + reslen, buf, uprv_min(len, capacity - reslen));
        }
        reslen += len;
    }
    u_terminateChars(appendAt, capacity, reslen, status);
    return reslen;
}

static int32_t
_appendScriptToLanguageTag(const char* localeID, char* appendAt, int32_t capacity, UBool strict, UErrorCode* status) {
    char buf[ULOC_SCRIPT_CAPACITY];
    UErrorCode tmpStatus = U_ZERO_ERROR;
    int32_t len;
    int32_t reslen = 0;

    if (U_FAILURE(*status)) {
        return 0;
    }

    len = uloc_getScript(localeID, buf, sizeof(buf), &tmpStatus);
    if (U_FAILURE(tmpStatus) || tmpStatus == U_STRING_NOT_TERMINATED_WARNING) {
        if (strict) {
            *status = U_ILLEGAL_ARGUMENT_ERROR;
        }
        return 0;
    }

    if (len > 0) {
        if (!_isScriptSubtag(buf, len)) {
            /* invalid script code */
            if (strict) {
                *status = U_ILLEGAL_ARGUMENT_ERROR;
            }
            return 0;
        } else {
            if (reslen < capacity) {
                *(appendAt + reslen) = SEP;
            }
            reslen++;

            if (reslen < capacity) {
                uprv_memcpy(appendAt + reslen, buf, uprv_min(len, capacity - reslen));
            }
            reslen += len;
        }
    }
    u_terminateChars(appendAt, capacity, reslen, status);
    return reslen;
}

static int32_t
_appendRegionToLanguageTag(const char* localeID, char* appendAt, int32_t capacity, UBool strict, UErrorCode* status) {
    char buf[ULOC_COUNTRY_CAPACITY];
    UErrorCode tmpStatus = U_ZERO_ERROR;
    int32_t len;
    int32_t reslen = 0;

    if (U_FAILURE(*status)) {
        return 0;
    }

    len = uloc_getCountry(localeID, buf, sizeof(buf), &tmpStatus);
    if (U_FAILURE(tmpStatus) || tmpStatus == U_STRING_NOT_TERMINATED_WARNING) {
        if (strict) {
            *status = U_ILLEGAL_ARGUMENT_ERROR;
        }
        return 0;
    }

    if (len > 0) {
        if (!_isRegionSubtag(buf, len)) {
            /* invalid region code */
            if (strict) {
                *status = U_ILLEGAL_ARGUMENT_ERROR;
            }
            return 0;
        } else {
            if (reslen < capacity) {
                *(appendAt + reslen) = SEP;
            }
            reslen++;

            if (reslen < capacity) {
                uprv_memcpy(appendAt + reslen, buf, uprv_min(len, capacity - reslen));
            }
            reslen += len;
        }
    }
    u_terminateChars(appendAt, capacity, reslen, status);
    return reslen;
}

static int32_t
_appendVariantsToLanguageTag(const char* localeID, char* appendAt, int32_t capacity, UBool strict, UBool *hadPosix, UErrorCode* status) {
    char buf[ULOC_FULLNAME_CAPACITY];
    UErrorCode tmpStatus = U_ZERO_ERROR;
    int32_t len, i;
    int32_t reslen = 0;

    if (U_FAILURE(*status)) {
        return 0;
    }

    len = uloc_getVariant(localeID, buf, sizeof(buf), &tmpStatus);
    if (U_FAILURE(tmpStatus) || tmpStatus == U_STRING_NOT_TERMINATED_WARNING) {
        if (strict) {
            *status = U_ILLEGAL_ARGUMENT_ERROR;
        }
        return 0;
    }

    if (len > 0) {
        char *p, *pVar;
        UBool bNext = TRUE;
        VariantListEntry *var;
        VariantListEntry *varFirst = NULL;

        pVar = NULL;
        p = buf;
        while (bNext) {
            if (*p == SEP || *p == LOCALE_SEP || *p == 0) {
                if (*p == 0) {
                    bNext = FALSE;
                } else {
                    *p = 0; /* terminate */
                }
                if (pVar == NULL) {
                    if (strict) {
                        *status = U_ILLEGAL_ARGUMENT_ERROR;
                        break;
                    }
                    /* ignore empty variant */
                } else {
                    /* ICU uses upper case letters for variants, but
                       the canonical format is lowercase in BCP47 */
                    for (i = 0; *(pVar + i) != 0; i++) {
                        *(pVar + i) = uprv_tolower(*(pVar + i));
                    }

                    /* validate */
                    if (_isVariantSubtag(pVar, -1)) {
                        if (uprv_strcmp(pVar,POSIX_VALUE)) {
                            /* emit the variant to the list */
                            var = uprv_malloc(sizeof(VariantListEntry));
                            if (var == NULL) {
                                *status = U_MEMORY_ALLOCATION_ERROR;
                                break;
                            }
                            var->variant = pVar;
                            if (!_addVariantToList(&varFirst, var)) {
                                /* duplicated variant */
                                uprv_free(var);
                                if (strict) {
                                    *status = U_ILLEGAL_ARGUMENT_ERROR;
                                    break;
                                }
                            }
                        } else {
                            /* Special handling for POSIX variant, need to remember that we had it and then */
                            /* treat it like an extension later. */
                            *hadPosix = TRUE;
                        }
                    } else if (strict) {
                        *status = U_ILLEGAL_ARGUMENT_ERROR;
                        break;
                    }
                }
                /* reset variant starting position */
                pVar = NULL;
            } else if (pVar == NULL) {
                pVar = p;
            }
            p++;
        }

        if (U_SUCCESS(*status)) {
            if (varFirst != NULL) {
                int32_t varLen;

                /* write out sorted/validated/normalized variants to the target */
                var = varFirst;
                while (var != NULL) {
                    if (reslen < capacity) {
                        *(appendAt + reslen) = SEP;
                    }
                    reslen++;
                    varLen = (int32_t)uprv_strlen(var->variant);
                    if (reslen < capacity) {
                        uprv_memcpy(appendAt + reslen, var->variant, uprv_min(varLen, capacity - reslen));
                    }
                    reslen += varLen;
                    var = var->next;
                }
            }
        }

        /* clean up */
        var = varFirst;
        while (var != NULL) {
            VariantListEntry *tmpVar = var->next;
            uprv_free(var);
            var = tmpVar;
        }

        if (U_FAILURE(*status)) {
            return 0;
        }
    }

    u_terminateChars(appendAt, capacity, reslen, status);
    return reslen;
}

static int32_t
_appendKeywordsToLanguageTag(const char* localeID, char* appendAt, int32_t capacity, UBool strict, UBool hadPosix, UErrorCode* status) {
    char buf[ULOC_KEYWORD_AND_VALUES_CAPACITY];
    UEnumeration *keywordEnum = NULL;
    int32_t reslen = 0;

    keywordEnum = uloc_openKeywords(localeID, status);
    if (U_FAILURE(*status) && !hadPosix) {
        uenum_close(keywordEnum);
        return 0;
    }
    if (keywordEnum != NULL || hadPosix) {
        /* reorder extensions */
        int32_t len;
        const char *key;
        ExtensionListEntry *firstExt = NULL;
        ExtensionListEntry *ext;
        char extBuf[ULOC_KEYWORD_AND_VALUES_CAPACITY];
        char *pExtBuf = extBuf;
        int32_t extBufCapacity = sizeof(extBuf);
        const char *bcpKey, *bcpValue;
        UErrorCode tmpStatus = U_ZERO_ERROR;
        int32_t keylen;
        UBool isLDMLKeyword;

        while (TRUE) {
            key = uenum_next(keywordEnum, NULL, status);
            if (key == NULL) {
                break;
            }
            len = uloc_getKeywordValue(localeID, key, buf, sizeof(buf), &tmpStatus);
            if (U_FAILURE(tmpStatus)) {
                if (strict) {
                    *status = U_ILLEGAL_ARGUMENT_ERROR;
                    break;
                }
                /* ignore this keyword */
                tmpStatus = U_ZERO_ERROR;
                continue;
            }

            keylen = (int32_t)uprv_strlen(key);
            isLDMLKeyword = (keylen > 1);

            if (isLDMLKeyword) {
                int32_t modKeyLen;

                /* transform key and value to bcp47 style */
                modKeyLen = _ldmlKeyToBCP47(key, keylen, pExtBuf, extBufCapacity, &tmpStatus);
                if (U_FAILURE(tmpStatus) || tmpStatus == U_STRING_NOT_TERMINATED_WARNING) {
                    if (strict) {
                        *status = U_ILLEGAL_ARGUMENT_ERROR;
                        break;
                    }
                    tmpStatus = U_ZERO_ERROR;
                    continue;
                }

                bcpKey = pExtBuf;
                pExtBuf += (modKeyLen + 1);
                extBufCapacity -= (modKeyLen + 1);

                len = _ldmlTypeToBCP47(key, keylen, buf, len, pExtBuf, extBufCapacity, &tmpStatus);
                if (U_FAILURE(tmpStatus) || tmpStatus == U_STRING_NOT_TERMINATED_WARNING) {
                    if (strict) {
                        *status = U_ILLEGAL_ARGUMENT_ERROR;
                        break;
                    }
                    tmpStatus = U_ZERO_ERROR;
                    continue;
                }
                bcpValue = pExtBuf;
                pExtBuf += (len + 1);
                extBufCapacity -= (len + 1);
            } else {
                if (*key == PRIVATEUSE) {
                    if (!_isPrivateuseValueSubtags(buf, len)) {
                        if (strict) {
                            *status = U_ILLEGAL_ARGUMENT_ERROR;
                            break;
                        }
                        continue;
                    }
                } else {
                    if (!_isExtensionSingleton(key, keylen) || !_isExtensionSubtags(buf, len)) {
                        if (strict) {
                            *status = U_ILLEGAL_ARGUMENT_ERROR;
                            break;
                        }
                        continue;
                    }
                }
                bcpKey = key;
                if ((len + 1) < extBufCapacity) {
                    uprv_memcpy(pExtBuf, buf, len);
                    bcpValue = pExtBuf;

                    pExtBuf += len;

                    *pExtBuf = 0;
                    pExtBuf++;

                    extBufCapacity -= (len + 1);
                } else {
                    *status = U_ILLEGAL_ARGUMENT_ERROR;
                    break;
                }
            }

            /* create ExtensionListEntry */
            ext = uprv_malloc(sizeof(ExtensionListEntry));
            if (ext == NULL) {
                *status = U_MEMORY_ALLOCATION_ERROR;
                break;
            }
            ext->key = bcpKey;
            ext->value = bcpValue;

            if (!_addExtensionToList(&firstExt, ext, TRUE)) {
                uprv_free(ext);
                if (strict) {
                    *status = U_ILLEGAL_ARGUMENT_ERROR;
                    break;
                }
            }
        }

        /* Special handling for POSIX variant - add the keywords for POSIX */
        if (hadPosix) {
            /* create ExtensionListEntry for POSIX */
            ext = uprv_malloc(sizeof(ExtensionListEntry));
            if (ext == NULL) {
                *status = U_MEMORY_ALLOCATION_ERROR;
            }
            ext->key = POSIX_KEY;
            ext->value = POSIX_VALUE;

            if (!_addExtensionToList(&firstExt, ext, TRUE)) {
                uprv_free(ext);
            }
        }

        if (U_SUCCESS(*status) && (firstExt != NULL)) {
            UBool startLDMLExtension = FALSE;

            /* write out the sorted BCP47 extensions and private use */
            ext = firstExt;
            while (ext != NULL) {
                if ((int32_t)uprv_strlen(ext->key) > 1 && !startLDMLExtension) {
                    /* write LDML singleton extension */
                    if (reslen < capacity) {
                        *(appendAt + reslen) = SEP;
                    }
                    reslen++;
                    if (reslen < capacity) {
                        *(appendAt + reslen) = LDMLEXT;
                    }
                    reslen++;
                    startLDMLExtension = TRUE;
                }

                if (reslen < capacity) {
                    *(appendAt + reslen) = SEP;
                }
                reslen++;
                len = (int32_t)uprv_strlen(ext->key);
                if (reslen < capacity) {
                    uprv_memcpy(appendAt + reslen, ext->key, uprv_min(len, capacity - reslen));
                }
                reslen += len;
                if (reslen < capacity) {
                    *(appendAt + reslen) = SEP;
                }
                reslen++;
                len = (int32_t)uprv_strlen(ext->value);
                if (reslen < capacity) {
                    uprv_memcpy(appendAt + reslen, ext->value, uprv_min(len, capacity - reslen));
                }
                reslen += len;

                ext = ext->next;
            }
        }
        /* clean up */
        ext = firstExt;
        while (ext != NULL) {
            ExtensionListEntry *tmpExt = ext->next;
            uprv_free(ext);
            ext = tmpExt;
        }

        uenum_close(keywordEnum);

        if (U_FAILURE(*status)) {
            return 0;
        }
    }

    return u_terminateChars(appendAt, capacity, reslen, status);
}

/**
 * Append keywords parsed from LDML extension value
 * e.g. "u-ca-gregory-co-trad" -> {calendar = gregorian} {collation = traditional}
 * Note: char* buf is used for storing keywords
 */
static void
_appendLDMLExtensionAsKeywords(const char* ldmlext, ExtensionListEntry** appendTo, char* buf, int32_t bufSize, UBool *posixVariant, UErrorCode *status) {
    const char *p, *pNext, *pSep;
    const char *pBcpKey, *pBcpType;
    const char *pKey, *pType;
    int32_t bcpKeyLen = 0, bcpTypeLen;
    ExtensionListEntry *kwd, *nextKwd;
    ExtensionListEntry *kwdFirst = NULL;
    int32_t bufIdx = 0;
    int32_t  len;

    pNext = ldmlext;
    pBcpKey = pBcpType = NULL;
    while (pNext) {
        p = pSep = pNext;

        /* locate next separator char */
        while (*pSep) {
            if (*pSep == SEP) {
                break;
            }
            pSep++;
        }
        if (*pSep == 0) {
            /* last subtag */
            pNext = NULL;
        } else {
            pNext = pSep + 1;
        }

        if (pBcpKey == NULL) {
            pBcpKey = p;
            bcpKeyLen = (int32_t)(pSep - p);
        } else {
            pBcpType = p;
            bcpTypeLen = (int32_t)(pSep - p);

            /* BCP key to locale key */
            len = _bcp47ToLDMLKey(pBcpKey, bcpKeyLen, buf + bufIdx, bufSize - bufIdx - 1, status);
            if (U_FAILURE(*status)) {
                goto cleanup;
            }
            pKey = buf + bufIdx;
            bufIdx += len;
            *(buf + bufIdx) = 0;
            bufIdx++;

            /* BCP type to locale type */
            len = _bcp47ToLDMLType(pKey, -1, pBcpType, bcpTypeLen, buf + bufIdx, bufSize - bufIdx - 1, status);
            if (U_FAILURE(*status)) {
                goto cleanup;
            }
            pType = buf + bufIdx;
            bufIdx += len;
            *(buf + bufIdx) = 0;
            bufIdx++;

            /* Special handling for u-va-posix, since we want to treat this as a variant, not */
            /* as a keyword.                                                                  */

            if ( !uprv_strcmp(pKey,POSIX_KEY) && !uprv_strcmp(pType,POSIX_VALUE) ) {
                *posixVariant = TRUE;
            } else {
                /* create an ExtensionListEntry for this keyword */
                kwd = uprv_malloc(sizeof(ExtensionListEntry));
                if (kwd == NULL) {
                    *status = U_MEMORY_ALLOCATION_ERROR;
                    goto cleanup;
                }

                kwd->key = pKey;
                kwd->value = pType;

                if (!_addExtensionToList(&kwdFirst, kwd, FALSE)) {
                    *status = U_ILLEGAL_ARGUMENT_ERROR;
                    uprv_free(kwd);
                    goto cleanup;
                }
            }

            /* for next pair */
            pBcpKey = NULL;
            pBcpType = NULL;
        }
    }

    if (pBcpKey != NULL) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        goto cleanup;
    }

    kwd = kwdFirst;
    while (kwd != NULL) {
        nextKwd = kwd->next;
        _addExtensionToList(appendTo, kwd, FALSE);
        kwd = nextKwd;
    }

    return;

cleanup:
    kwd = kwdFirst;
    while (kwd != NULL) {
        nextKwd = kwd->next;
        uprv_free(kwd);
        kwd = nextKwd;
    }
}


static int32_t
_appendKeywords(ULanguageTag* langtag, char* appendAt, int32_t capacity, UErrorCode* status) {
    int32_t reslen = 0;
    int32_t i, n;
    int32_t len;
    ExtensionListEntry *kwdFirst = NULL;
    ExtensionListEntry *kwd;
    const char *key, *type;
    char kwdBuf[ULOC_KEYWORDS_CAPACITY];
    UBool posixVariant = FALSE;

    if (U_FAILURE(*status)) {
        return 0;
    }

    n = ultag_getExtensionsSize(langtag);

    /* resolve locale keywords and reordering keys */
    for (i = 0; i < n; i++) {
        key = ultag_getExtensionKey(langtag, i);
        type = ultag_getExtensionValue(langtag, i);
        if (*key == LDMLEXT) {
            _appendLDMLExtensionAsKeywords(type, &kwdFirst, kwdBuf, sizeof(kwdBuf), &posixVariant, status);
            if (U_FAILURE(*status)) {
                break;
            }
        } else {
            kwd = uprv_malloc(sizeof(ExtensionListEntry));
            if (kwd == NULL) {
                *status = U_MEMORY_ALLOCATION_ERROR;
                break;
            }
            kwd->key = key;
            kwd->value = type;
            if (!_addExtensionToList(&kwdFirst, kwd, FALSE)) {
                uprv_free(kwd);
                *status = U_ILLEGAL_ARGUMENT_ERROR;
                break;
            }
        }
    }

    if (U_SUCCESS(*status)) {
        type = ultag_getPrivateUse(langtag);
        if ((int32_t)uprv_strlen(type) > 0) {
            /* add private use as a keyword */
            kwd = uprv_malloc(sizeof(ExtensionListEntry));
            if (kwd == NULL) {
                *status = U_MEMORY_ALLOCATION_ERROR;
            } else {
                kwd->key = PRIVATEUSE_KEY;
                kwd->value = type;
                if (!_addExtensionToList(&kwdFirst, kwd, FALSE)) {
                    uprv_free(kwd);
                    *status = U_ILLEGAL_ARGUMENT_ERROR;
                }
            }
        }
    }

    /* If a POSIX variant was in the extensions, write it out before writing the keywords. */

    if (U_SUCCESS(*status) && posixVariant) {
        len = (int32_t) uprv_strlen(_POSIX);
        if (reslen < capacity) {
            uprv_memcpy(appendAt + reslen, _POSIX, uprv_min(len, capacity - reslen));
        }
        reslen += len;
    }

    if (U_SUCCESS(*status) && kwdFirst != NULL) {
        /* write out the sorted keywords */
        kwd = kwdFirst;
        while (kwd != NULL) {
            if (reslen < capacity) {
                if (kwd == kwdFirst) {
                    /* '@' */
                    *(appendAt + reslen) = LOCALE_EXT_SEP;
                } else {
                    /* ';' */
                    *(appendAt + reslen) = LOCALE_KEYWORD_SEP;
                }
            }
            reslen++;

            /* key */
            len = (int32_t)uprv_strlen(kwd->key);
            if (reslen < capacity) {
                uprv_memcpy(appendAt + reslen, kwd->key, uprv_min(len, capacity - reslen));
            }
            reslen += len;

            /* '=' */
            if (reslen < capacity) {
                *(appendAt + reslen) = LOCALE_KEY_TYPE_SEP;
            }
            reslen++;

            /* type */
            len = (int32_t)uprv_strlen(kwd->value);
            if (reslen < capacity) {
                uprv_memcpy(appendAt + reslen, kwd->value, uprv_min(len, capacity - reslen));
            }
            reslen += len;

            kwd = kwd->next;
        }
    }

    /* clean up */
    kwd = kwdFirst;
    while (kwd != NULL) {
        ExtensionListEntry *tmpKwd = kwd->next;
        uprv_free(kwd);
        kwd = tmpKwd;
    }

    if (U_FAILURE(*status)) {
        return 0;
    }

    return u_terminateChars(appendAt, capacity, reslen, status);
}

/*
* -------------------------------------------------
*
* ultag_ functions
*
* -------------------------------------------------
*/

/* Bit flags used by the parser */
#define LANG 0x0001
#define EXTL 0x0002
#define SCRT 0x0004
#define REGN 0x0008
#define VART 0x0010
#define EXTS 0x0020
#define EXTV 0x0040
#define PRIV 0x0080

static ULanguageTag*
ultag_parse(const char* tag, int32_t tagLen, int32_t* parsedLen, UErrorCode* status) {
    ULanguageTag *t;
    char *tagBuf;
    int16_t next;
    char *pSubtag, *pNext, *pLastGoodPosition;
    int32_t subtagLen;
    int32_t extlangIdx;
    ExtensionListEntry *pExtension;
    char *pExtValueSubtag, *pExtValueSubtagEnd;
    int32_t i;
    UBool isLDMLExtension, reqLDMLType;

    if (parsedLen != NULL) {
        *parsedLen = 0;
    }

    if (U_FAILURE(*status)) {
        return NULL;
    }

    if (tagLen < 0) {
        tagLen = (int32_t)uprv_strlen(tag);
    }

    /* copy the entire string */
    tagBuf = (char*)uprv_malloc(tagLen + 1);
    if (tagBuf == NULL) {
        *status = U_MEMORY_ALLOCATION_ERROR;
        return NULL;
    }
    uprv_memcpy(tagBuf, tag, tagLen);
    *(tagBuf + tagLen) = 0;

    /* create a ULanguageTag */
    t = (ULanguageTag*)uprv_malloc(sizeof(ULanguageTag));
    _initializeULanguageTag(t);
    t->buf = tagBuf;
    if (t == NULL) {
        uprv_free(tagBuf);
        *status = U_MEMORY_ALLOCATION_ERROR;
        return NULL;
    }

    if (tagLen < MINLEN) {
        /* the input tag is too short - return empty ULanguageTag */
        return t;
    }

    /* check if the tag is grandfathered */
    for (i = 0; GRANDFATHERED[i] != NULL; i += 2) {
        if (T_CString_stricmp(GRANDFATHERED[i], tagBuf) == 0) {
            /* a grandfathered tag is always longer than its preferred mapping */
            uprv_strcpy(t->buf, GRANDFATHERED[i + 1]);
            t->language = t->buf;
            if (parsedLen != NULL) {
                *parsedLen = tagLen;
            }
            return t;
        }
    }

    /*
     * langtag      =   language
     *                  ["-" script]
     *                  ["-" region]
     *                  *("-" variant)
     *                  *("-" extension)
     *                  ["-" privateuse]
     */

    next = LANG | PRIV;
    pNext = pLastGoodPosition = tagBuf;
    extlangIdx = 0;
    pExtension = NULL;
    pExtValueSubtag = NULL;
    pExtValueSubtagEnd = NULL;
    isLDMLExtension = FALSE;
    reqLDMLType = FALSE;

    while (pNext) {
        char *pSep;

        pSubtag = pNext;

        /* locate next separator char */
        pSep = pSubtag;
        while (*pSep) {
            if (*pSep == SEP) {
                break;
            }
            pSep++;
        }
        if (*pSep == 0) {
            /* last subtag */
            pNext = NULL;
        } else {
            pNext = pSep + 1;
        }
        subtagLen = (int32_t)(pSep - pSubtag);

        if (next & LANG) {
            if (_isLanguageSubtag(pSubtag, subtagLen)) {
                *pSep = 0;  /* terminate */
                t->language = T_CString_toLowerCase(pSubtag);

                pLastGoodPosition = pSep;
                next = EXTL | SCRT | REGN | VART | EXTS | PRIV;
                continue;
            }
        }
        if (next & EXTL) {
            if (_isExtlangSubtag(pSubtag, subtagLen)) {
                *pSep = 0;
                t->extlang[extlangIdx++] = T_CString_toLowerCase(pSubtag);

                pLastGoodPosition = pSep;
                if (extlangIdx < 3) {
                    next = EXTL | SCRT | REGN | VART | EXTS | PRIV;
                } else {
                    next = SCRT | REGN | VART | EXTS | PRIV;
                }
                continue;
            }
        }
        if (next & SCRT) {
            if (_isScriptSubtag(pSubtag, subtagLen)) {
                char *p = pSubtag;

                *pSep = 0;

                /* to title case */
                *p = uprv_toupper(*p);
                p++;
                for (; *p; p++) {
                    *p = uprv_tolower(*p);
                }

                t->script = pSubtag;

                pLastGoodPosition = pSep;
                next = REGN | VART | EXTS | PRIV;
                continue;
            }
        }
        if (next & REGN) {
            if (_isRegionSubtag(pSubtag, subtagLen)) {
                *pSep = 0;
                t->region = T_CString_toUpperCase(pSubtag);

                pLastGoodPosition = pSep;
                next = VART | EXTS | PRIV;
                continue;
            }
        }
        if (next & VART) {
            if (_isVariantSubtag(pSubtag, subtagLen)) {
                VariantListEntry *var;
                UBool isAdded;

                var = (VariantListEntry*)uprv_malloc(sizeof(VariantListEntry));
                if (var == NULL) {
                    *status = U_MEMORY_ALLOCATION_ERROR;
                    goto error;
                }
                *pSep = 0;
                var->variant = T_CString_toUpperCase(pSubtag);
                isAdded = _addVariantToList(&(t->variants), var);
                if (!isAdded) {
                    /* duplicated variant entry */
                    uprv_free(var);
                    break;
                }
                pLastGoodPosition = pSep;
                next = VART | EXTS | PRIV;
                continue;
            }
        }
        if (next & EXTS) {
            if (_isExtensionSingleton(pSubtag, subtagLen)) {
                if (pExtension != NULL) {
                    if (pExtValueSubtag == NULL || pExtValueSubtagEnd == NULL) {
                        /* the previous extension is incomplete */
                        uprv_free(pExtension);
                        pExtension = NULL;
                        break;
                    }

                    /* terminate the previous extension value */
                    *pExtValueSubtagEnd = 0;
                    pExtension->value = T_CString_toLowerCase(pExtValueSubtag);

                    /* insert the extension to the list */
                    if (_addExtensionToList(&(t->extensions), pExtension, FALSE)) {
                        pLastGoodPosition = pExtValueSubtagEnd;
                    } else {
                        /* stop parsing here */
                        uprv_free(pExtension);
                        pExtension = NULL;
                        break;
                    }

                    if (isLDMLExtension && reqLDMLType) {
                        /* incomplete LDML extension key and type pair */
                        pExtension = NULL;
                        break;
                    }
                }

                isLDMLExtension = (uprv_tolower(*pSubtag) == LDMLEXT);

                /* create a new extension */
                pExtension = uprv_malloc(sizeof(ExtensionListEntry));
                if (pExtension == NULL) {
                    *status = U_MEMORY_ALLOCATION_ERROR;
                    goto error;
                }
                *pSep = 0;
                pExtension->key = T_CString_toLowerCase(pSubtag);
                pExtension->value = NULL;   /* will be set later */

                /*
                 * reset the start and the end location of extension value
                 * subtags for this extension
                 */
                pExtValueSubtag = NULL;
                pExtValueSubtagEnd = NULL;

                next = EXTV;
                continue;
            }
        }
        if (next & EXTV) {
            if (_isExtensionSubtag(pSubtag, subtagLen)) {
                if (isLDMLExtension) {
                    if (reqLDMLType) {
                        /* already saw an LDML key */
                        if (!_isLDMLType(pSubtag, subtagLen)) {
                            /* stop parsing here and let the valid LDML extension key/type
                               pairs processed by the code out of this while loop */
                            break;
                        }
                        pExtValueSubtagEnd = pSep;
                        reqLDMLType = FALSE;
                        next = EXTS | EXTV | PRIV;
                    } else {
                        /* LDML key */
                        if (!_isLDMLKey(pSubtag, subtagLen)) {
                            /* stop parsing here and let the valid LDML extension key/type
                               pairs processed by the code out of this while loop */
                            break;
                        }
                        reqLDMLType = TRUE;
                        next = EXTV;
                    }
                } else {
                    /* Mark the end of this subtag */
                    pExtValueSubtagEnd = pSep;
                    next = EXTS | EXTV | PRIV;
                }

                if (pExtValueSubtag == NULL) {
                    /* if the start postion of this extension's value is not yet,
                       this one is the first value subtag */
                    pExtValueSubtag = pSubtag;
                }
                continue;
            }
        }
        if (next & PRIV) {
            if (uprv_tolower(*pSubtag) == PRIVATEUSE) {
                char *pPrivuseVal;

                if (pExtension != NULL) {
                    /* Process the last extension */
                    if (pExtValueSubtag == NULL || pExtValueSubtagEnd == NULL) {
                        /* the previous extension is incomplete */
                        uprv_free(pExtension);
                        pExtension = NULL;
                        break;
                    } else {
                        /* terminate the previous extension value */
                        *pExtValueSubtagEnd = 0;
                        pExtension->value = T_CString_toLowerCase(pExtValueSubtag);

                        /* insert the extension to the list */
                        if (_addExtensionToList(&(t->extensions), pExtension, FALSE)) {
                            pLastGoodPosition = pExtValueSubtagEnd;
                            pExtension = NULL;
                        } else {
                        /* stop parsing here */
                            uprv_free(pExtension);
                            pExtension = NULL;
                            break;
                        }
                    }
                }

                /* The rest of part will be private use value subtags */
                if (pNext == NULL) {
                    /* empty private use subtag */
                    break;
                }
                /* back up the private use value start position */
                pPrivuseVal = pNext;

                /* validate private use value subtags */
                while (pNext) {
                    pSubtag = pNext;
                    pSep = pSubtag;
                    while (*pSep) {
                        if (*pSep == SEP) {
                            break;
                        }
                        pSep++;
                    }
                    if (*pSep == 0) {
                        /* last subtag */
                        pNext = NULL;
                    } else {
                        pNext = pSep + 1;
                    }
                    subtagLen = (int32_t)(pSep - pSubtag);

                    if (_isPrivateuseValueSubtag(pSubtag, subtagLen)) {
                        pLastGoodPosition = pSep;
                    } else {
                        break;
                    }
                }
                if (pLastGoodPosition - pPrivuseVal > 0) {
                    *pLastGoodPosition = 0;
                    t->privateuse = T_CString_toLowerCase(pPrivuseVal);
                }
                /* No more subtags, exiting the parse loop */
                break;
            }
            break;
        }
        /* If we fell through here, it means this subtag is illegal - quit parsing */
        break;
    }

    if (pExtension != NULL) {
        /* Process the last extension */
        if (pExtValueSubtag == NULL || pExtValueSubtagEnd == NULL) {
            /* the previous extension is incomplete */
            uprv_free(pExtension);
        } else {
            /* terminate the previous extension value */
            *pExtValueSubtagEnd = 0;
            pExtension->value = T_CString_toLowerCase(pExtValueSubtag);
            /* insert the extension to the list */
            if (_addExtensionToList(&(t->extensions), pExtension, FALSE)) {
                pLastGoodPosition = pExtValueSubtagEnd;
            } else {
                uprv_free(pExtension);
            }
        }
    }

    if (parsedLen != NULL) {
        *parsedLen = (int32_t)(pLastGoodPosition - t->buf);
    }

    return t;

error:
    uprv_free(t);
    return NULL;
}

static void
ultag_close(ULanguageTag* langtag) {

    if (langtag == NULL) {
        return;
    }

    uprv_free(langtag->buf);

    if (langtag->variants) {
        VariantListEntry *curVar = langtag->variants;
        while (curVar) {
            VariantListEntry *nextVar = curVar->next;
            uprv_free(curVar);
            curVar = nextVar;
        }
    }

    if (langtag->extensions) {
        ExtensionListEntry *curExt = langtag->extensions;
        while (curExt) {
            ExtensionListEntry *nextExt = curExt->next;
            uprv_free(curExt);
            curExt = nextExt;
        }
    }

    uprv_free(langtag);
}

static const char*
ultag_getLanguage(const ULanguageTag* langtag) {
    return langtag->language;
}

#if 0
static const char*
ultag_getJDKLanguage(const ULanguageTag* langtag) {
    int32_t i;
    for (i = 0; DEPRECATEDLANGS[i] != NULL; i += 2) {
        if (uprv_compareInvCharsAsAscii(DEPRECATEDLANGS[i], langtag->language) == 0) {
            return DEPRECATEDLANGS[i + 1];
        }
    }
    return langtag->language;
}
#endif

static const char*
ultag_getExtlang(const ULanguageTag* langtag, int32_t idx) {
    if (idx >= 0 && idx < MAXEXTLANG) {
        return langtag->extlang[idx];
    }
    return NULL;
}

static int32_t
ultag_getExtlangSize(const ULanguageTag* langtag) {
    int32_t size = 0;
    int32_t i;
    for (i = 0; i < MAXEXTLANG; i++) {
        if (langtag->extlang[i]) {
            size++;
        }
    }
    return size;
}

static const char*
ultag_getScript(const ULanguageTag* langtag) {
    return langtag->script;
}

static const char*
ultag_getRegion(const ULanguageTag* langtag) {
    return langtag->region;
}

static const char*
ultag_getVariant(const ULanguageTag* langtag, int32_t idx) {
    const char *var = NULL;
    VariantListEntry *cur = langtag->variants;
    int32_t i = 0;
    while (cur) {
        if (i == idx) {
            var = cur->variant;
            break;
        }
        cur = cur->next;
        i++;
    }
    return var;
}

static int32_t
ultag_getVariantsSize(const ULanguageTag* langtag) {
    int32_t size = 0;
    VariantListEntry *cur = langtag->variants;
    while (TRUE) {
        if (cur == NULL) {
            break;
        }
        size++;
        cur = cur->next;
    }
    return size;
}

static const char*
ultag_getExtensionKey(const ULanguageTag* langtag, int32_t idx) {
    const char *key = NULL;
    ExtensionListEntry *cur = langtag->extensions;
    int32_t i = 0;
    while (cur) {
        if (i == idx) {
            key = cur->key;
            break;
        }
        cur = cur->next;
        i++;
    }
    return key;
}

static const char*
ultag_getExtensionValue(const ULanguageTag* langtag, int32_t idx) {
    const char *val = NULL;
    ExtensionListEntry *cur = langtag->extensions;
    int32_t i = 0;
    while (cur) {
        if (i == idx) {
            val = cur->value;
            break;
        }
        cur = cur->next;
        i++;
    }
    return val;
}

static int32_t
ultag_getExtensionsSize(const ULanguageTag* langtag) {
    int32_t size = 0;
    ExtensionListEntry *cur = langtag->extensions;
    while (TRUE) {
        if (cur == NULL) {
            break;
        }
        size++;
        cur = cur->next;
    }
    return size;
}

static const char*
ultag_getPrivateUse(const ULanguageTag* langtag) {
    return langtag->privateuse;
}

#if 0
static const char*
ultag_getGrandfathered(const ULanguageTag* langtag) {
    return langtag->grandfathered;
}
#endif


/*
* -------------------------------------------------
*
* Locale/BCP47 conversion APIs, exposed as uloc_*
*
* -------------------------------------------------
*/
U_DRAFT int32_t U_EXPORT2
uloc_toLanguageTag(const char* localeID,
                   char* langtag,
                   int32_t langtagCapacity,
                   UBool strict,
                   UErrorCode* status) {
    /* char canonical[ULOC_FULLNAME_CAPACITY]; */ /* See #6822 */
    char canonical[256];
    int32_t reslen = 0;
    UErrorCode tmpStatus = U_ZERO_ERROR;
    UBool hadPosix = FALSE;

    /* Note: uloc_canonicalize returns "en_US_POSIX" for input locale ID "".  See #6835 */
    canonical[0] = 0;
    if (uprv_strlen(localeID) > 0) {
        uloc_canonicalize(localeID, canonical, sizeof(canonical), &tmpStatus);
        if (tmpStatus != U_ZERO_ERROR) {
            *status = U_ILLEGAL_ARGUMENT_ERROR;
            return 0;
        }
    }

    reslen += _appendLanguageToLanguageTag(canonical, langtag, langtagCapacity, strict, status);
    reslen += _appendScriptToLanguageTag(canonical, langtag + reslen, langtagCapacity - reslen, strict, status);
    reslen += _appendRegionToLanguageTag(canonical, langtag + reslen, langtagCapacity - reslen, strict, status);
    reslen += _appendVariantsToLanguageTag(canonical, langtag + reslen, langtagCapacity - reslen, strict, &hadPosix, status);
    reslen += _appendKeywordsToLanguageTag(canonical, langtag + reslen, langtagCapacity - reslen, strict, hadPosix, status);

    return reslen;
}


U_DRAFT int32_t U_EXPORT2
uloc_forLanguageTag(const char* langtag,
                    char* localeID,
                    int32_t localeIDCapacity,
                    int32_t* parsedLength,
                    UErrorCode* status) {
    ULanguageTag *lt;
    int32_t reslen = 0;
    const char *subtag, *p;
    int32_t len;
    int32_t i, n;
    UBool noRegion = TRUE;

    lt = ultag_parse(langtag, -1, parsedLength, status);
    if (U_FAILURE(*status)) {
        return 0;
    }

    /* language */
    subtag = ultag_getExtlangSize(lt) > 0 ? ultag_getExtlang(lt, 0) : ultag_getLanguage(lt);
    if (uprv_compareInvCharsAsAscii(subtag, LANG_UND) != 0) {
        len = (int32_t)uprv_strlen(subtag);
        if (len > 0) {
            if (reslen < localeIDCapacity) {
                uprv_memcpy(localeID, subtag, uprv_min(len, localeIDCapacity - reslen));
            }
            reslen += len;
        }
    }

    /* script */
    subtag = ultag_getScript(lt);
    len = (int32_t)uprv_strlen(subtag);
    if (len > 0) {
        if (reslen < localeIDCapacity) {
            *(localeID + reslen) = LOCALE_SEP;
        }
        reslen++;

        /* write out the script in title case */
        p = subtag;
        while (*p) {
            if (reslen < localeIDCapacity) {
                if (p == subtag) {
                    *(localeID + reslen) = uprv_toupper(*p);
                } else {
                    *(localeID + reslen) = *p;
                }
            }
            reslen++;
            p++;
        }
    }

    /* region */
    subtag = ultag_getRegion(lt);
    len = (int32_t)uprv_strlen(subtag);
    if (len > 0) {
        if (reslen < localeIDCapacity) {
            *(localeID + reslen) = LOCALE_SEP;
        }
        reslen++;
        /* write out the retion in upper case */
        p = subtag;
        while (*p) {
            if (reslen < localeIDCapacity) {
                *(localeID + reslen) = uprv_toupper(*p);
            }
            reslen++;
            p++;
        }
        noRegion = FALSE;
    }

    /* variants */
    n = ultag_getVariantsSize(lt);
    if (n > 0) {
        if (noRegion) {
            if (reslen < localeIDCapacity) {
                *(localeID + reslen) = LOCALE_SEP;
            }
            reslen++;
        }

        for (i = 0; i < n; i++) {
            subtag = ultag_getVariant(lt, i);
            if (reslen < localeIDCapacity) {
                *(localeID + reslen) = LOCALE_SEP;
            }
            reslen++;
            /* write out the variant in upper case */
            p = subtag;
            while (*p) {
                if (reslen < localeIDCapacity) {
                    *(localeID + reslen) = uprv_toupper(*p);
                }
                reslen++;
                p++;
            }
        }
    }

    /* keywords */
    n = ultag_getExtensionsSize(lt);
    subtag = ultag_getPrivateUse(lt);
    if (n > 0 || uprv_strlen(subtag) > 0) {
        if (reslen == 0) {
            /* need a language */
            if (reslen < localeIDCapacity) {
                uprv_memcpy(localeID + reslen, LANG_UND, uprv_min(LANG_UND_LEN, localeIDCapacity - reslen));
            }
            reslen += LANG_UND_LEN;
        }
        len = _appendKeywords(lt, localeID + reslen, localeIDCapacity - reslen, status);
        reslen += len;
    }

    ultag_close(lt);
    return u_terminateChars(localeID, localeIDCapacity, reslen, status);
}


