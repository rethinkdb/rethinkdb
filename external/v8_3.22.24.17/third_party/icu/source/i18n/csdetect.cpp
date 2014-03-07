/*
 **********************************************************************
 *   Copyright (C) 2005-2009, International Business Machines
 *   Corporation and others.  All Rights Reserved.
 **********************************************************************
 */

#include "unicode/utypes.h"

#if !UCONFIG_NO_CONVERSION

#include "unicode/ucsdet.h"

#include "csdetect.h"
#include "csmatch.h"
#include "uenumimp.h"

#include "cmemory.h"
#include "cstring.h"
#include "umutex.h"
#include "ucln_in.h"
#include "uarrsort.h"
#include "inputext.h"
#include "csrsbcs.h"
#include "csrmbcs.h"
#include "csrutf8.h"
#include "csrucode.h"
#include "csr2022.h"

#define ARRAY_SIZE(array) (sizeof array / sizeof array[0])

#define NEW_ARRAY(type,count) (type *) uprv_malloc((count) * sizeof(type))
#define DELETE_ARRAY(array) uprv_free((void *) (array))

U_CDECL_BEGIN
static U_NAMESPACE_QUALIFIER CharsetRecognizer **fCSRecognizers = NULL;

static int32_t fCSRecognizers_size = 0;

static UBool U_CALLCONV csdet_cleanup(void)
{
    if (fCSRecognizers != NULL) {
        for(int32_t r = 0; r < fCSRecognizers_size; r += 1) {
            delete fCSRecognizers[r];
            fCSRecognizers[r] = NULL;
        }

        DELETE_ARRAY(fCSRecognizers);
        fCSRecognizers = NULL;
        fCSRecognizers_size = 0;
    }

    return TRUE;
}

static int32_t U_CALLCONV
charsetMatchComparator(const void * /*context*/, const void *left, const void *right)
{
    U_NAMESPACE_USE

    const CharsetMatch **csm_l = (const CharsetMatch **) left;
    const CharsetMatch **csm_r = (const CharsetMatch **) right;

    // NOTE: compare is backwards to sort from highest to lowest.
    return (*csm_r)->getConfidence() - (*csm_l)->getConfidence();
}

U_CDECL_END

U_NAMESPACE_BEGIN

void CharsetDetector::setRecognizers(UErrorCode &status)
{
    UBool needsInit;
    CharsetRecognizer **recognizers;

    if (U_FAILURE(status)) {
        return;
    }

    UMTX_CHECK(NULL, (UBool) (fCSRecognizers == NULL), needsInit);

    if (needsInit) {
        CharsetRecognizer *tempArray[] = {
            new CharsetRecog_UTF8(),

            new CharsetRecog_UTF_16_BE(),
            new CharsetRecog_UTF_16_LE(),
            new CharsetRecog_UTF_32_BE(),
            new CharsetRecog_UTF_32_LE(),

            new CharsetRecog_8859_1_en(),
            new CharsetRecog_8859_1_da(),
            new CharsetRecog_8859_1_de(),
            new CharsetRecog_8859_1_es(),
            new CharsetRecog_8859_1_fr(),
            new CharsetRecog_8859_1_it(),
            new CharsetRecog_8859_1_nl(),
            new CharsetRecog_8859_1_no(),
            new CharsetRecog_8859_1_pt(),
            new CharsetRecog_8859_1_sv(),
            new CharsetRecog_8859_2_cs(),
            new CharsetRecog_8859_2_hu(),
            new CharsetRecog_8859_2_pl(),
            new CharsetRecog_8859_2_ro(),
            new CharsetRecog_8859_5_ru(),
            new CharsetRecog_8859_6_ar(),
            new CharsetRecog_8859_7_el(),
            new CharsetRecog_8859_8_I_he(),
            new CharsetRecog_8859_8_he(),
            new CharsetRecog_windows_1251(),
            new CharsetRecog_windows_1256(),
            new CharsetRecog_KOI8_R(),
            new CharsetRecog_8859_9_tr(),
            new CharsetRecog_sjis(),
            new CharsetRecog_gb_18030(),
            new CharsetRecog_euc_jp(),
            new CharsetRecog_euc_kr(),
            new CharsetRecog_big5(),

            new CharsetRecog_2022JP(),
            new CharsetRecog_2022KR(),
            new CharsetRecog_2022CN(),
            
            new CharsetRecog_IBM424_he_rtl(),
            new CharsetRecog_IBM424_he_ltr(),
            new CharsetRecog_IBM420_ar_rtl(),
            new CharsetRecog_IBM420_ar_ltr()
        };
        int32_t rCount = ARRAY_SIZE(tempArray);
        int32_t r;

        recognizers = NEW_ARRAY(CharsetRecognizer *, rCount);

        if (recognizers == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return;
        } else {
            for (r = 0; r < rCount; r += 1) {
                recognizers[r] = tempArray[r];

                if (recognizers[r] == NULL) {
                    status = U_MEMORY_ALLOCATION_ERROR;
                    break;
                }
            }
        }

        if (U_SUCCESS(status)) {
            umtx_lock(NULL);
            if (fCSRecognizers == NULL) {
                fCSRecognizers_size = rCount;
                fCSRecognizers = recognizers;
            }
            umtx_unlock(NULL);
        }

        if (fCSRecognizers != recognizers) {
            for (r = 0; r < rCount; r += 1) {
                delete recognizers[r];
                recognizers[r] = NULL;
            }

            DELETE_ARRAY(recognizers);
        }

        recognizers = NULL;
        ucln_i18n_registerCleanup(UCLN_I18N_CSDET, csdet_cleanup);
    }
}

CharsetDetector::CharsetDetector(UErrorCode &status)
  : textIn(new InputText(status)), resultArray(NULL),
    resultCount(0), fStripTags(FALSE), fFreshTextSet(FALSE)
{
    if (U_FAILURE(status)) {
        return;
    }

    setRecognizers(status);

    if (U_FAILURE(status)) {
        return;
    }

    resultArray = (CharsetMatch **)uprv_malloc(sizeof(CharsetMatch *)*fCSRecognizers_size);

    if (resultArray == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return;
    }

    for(int32_t i = 0; i < fCSRecognizers_size; i += 1) {
        resultArray[i] = new CharsetMatch();

        if (resultArray[i] == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
            break;
        }
    }
}

CharsetDetector::~CharsetDetector()
{
    delete textIn;

    for(int32_t i = 0; i < fCSRecognizers_size; i += 1) {
        delete resultArray[i];
    }

    uprv_free(resultArray);
}

void CharsetDetector::setText(const char *in, int32_t len)
{
    textIn->setText(in, len);
    fFreshTextSet = TRUE;
}

UBool CharsetDetector::setStripTagsFlag(UBool flag)
{
    UBool temp = fStripTags;
    fStripTags = flag;
    fFreshTextSet = TRUE;
    return temp;
}

UBool CharsetDetector::getStripTagsFlag() const
{
    return fStripTags;
}

void CharsetDetector::setDeclaredEncoding(const char *encoding, int32_t len) const
{
    textIn->setDeclaredEncoding(encoding,len);
}

int32_t CharsetDetector::getDetectableCount()
{
    UErrorCode status = U_ZERO_ERROR;

    setRecognizers(status);

    return fCSRecognizers_size; 
}

const CharsetMatch *CharsetDetector::detect(UErrorCode &status)
{
    int32_t maxMatchesFound = 0;

    detectAll(maxMatchesFound, status);

    if(maxMatchesFound > 0) {
        return resultArray[0];
    } else {
        return NULL;
    }
}

const CharsetMatch * const *CharsetDetector::detectAll(int32_t &maxMatchesFound, UErrorCode &status)
{
    if(!textIn->isSet()) {
        status = U_MISSING_RESOURCE_ERROR;// TODO:  Need to set proper status code for input text not set

        return NULL;
    } else if(fFreshTextSet) {
        CharsetRecognizer *csr;
        int32_t            detectResults;
        int32_t            confidence;
        int32_t            i;

        textIn->MungeInput(fStripTags);

        // Iterate over all possible charsets, remember all that
        // give a match quality > 0.
        resultCount = 0;
        for (i = 0; i < fCSRecognizers_size; i += 1) {
            csr = fCSRecognizers[i];
            detectResults = csr->match(textIn);
            confidence = detectResults;

            if (confidence > 0)  {
                resultArray[resultCount++]->set(textIn, csr, confidence);
            }
        }

        for(i = resultCount; i < fCSRecognizers_size; i += 1) {
            resultArray[i]->set(textIn, 0, 0);
        }

        uprv_sortArray(resultArray, resultCount, sizeof resultArray[0], charsetMatchComparator, NULL, TRUE, &status);

        // Remove duplicate charsets from the results.
        // Simple minded, brute force approach - check each entry against all that follow.
        // The first entry of any duplicated set is the one that should be kept because it will
        // be the one with the highest confidence rating.
        //   (Duplicate matches have different languages, only the charset is the same)
        // Because the resultArray contains preallocated CharsetMatch objects, they aren't actually
        // deleted, just reordered, with the unwanted duplicates placed after the good results.
        int32_t j, k;
        for (i=0; i<resultCount; i++) {
            const char *charSetName = resultArray[i]->getName();
            for (j=i+1; j<resultCount; ) {
                if (uprv_strcmp(charSetName, resultArray[j]->getName()) != 0) {
                    // Not a duplicate.
                    j++;
                } else {
                    // Duplicate entry at index j.  
                    CharsetMatch *duplicate = resultArray[j];
                    for (k=j; k<resultCount-1; k++) {
                        resultArray[k] = resultArray[k+1];
                    }
                    resultCount--;
                    resultArray[resultCount] = duplicate;
                }
            }
        }

        fFreshTextSet = FALSE;
    }

    maxMatchesFound = resultCount;

    return resultArray;
}

/*const char *CharsetDetector::getCharsetName(int32_t index, UErrorCode &status) const
{
    if( index > fCSRecognizers_size-1 || index < 0) {
        status = U_INDEX_OUTOFBOUNDS_ERROR;

        return 0;
    } else {
        return fCSRecognizers[index]->getName();
    }
}*/

U_NAMESPACE_END

U_CDECL_BEGIN
typedef struct {
    int32_t currIndex;
} Context;



static void U_CALLCONV
enumClose(UEnumeration *en) {
    if(en->context != NULL) {
        DELETE_ARRAY(en->context);
    }

    DELETE_ARRAY(en);
}

static int32_t U_CALLCONV
enumCount(UEnumeration *, UErrorCode *) {
    return fCSRecognizers_size;
}

static const char* U_CALLCONV
enumNext(UEnumeration *en, int32_t *resultLength, UErrorCode * /*status*/) {
    if(((Context *)en->context)->currIndex >= fCSRecognizers_size) {
        if(resultLength != NULL) {
            *resultLength = 0;
        }
        return NULL;
    }
    const char *currName = fCSRecognizers[((Context *)en->context)->currIndex]->getName();
    if(resultLength != NULL) {
        *resultLength = (int32_t)uprv_strlen(currName);
    }
    ((Context *)en->context)->currIndex++;

    return currName;
}

static void U_CALLCONV
enumReset(UEnumeration *en, UErrorCode *) {
    ((Context *)en->context)->currIndex = 0;
}

static const UEnumeration gCSDetEnumeration = {
    NULL,
    NULL,
    enumClose,
    enumCount,
    uenum_unextDefault,
    enumNext,
    enumReset
};

U_CAPI  UEnumeration * U_EXPORT2
ucsdet_getAllDetectableCharsets(const UCharsetDetector * /*ucsd*/, UErrorCode *status)
{
    U_NAMESPACE_USE

    if(U_FAILURE(*status)) {
        return 0;
    }

    /* Initialize recognized charsets. */
    CharsetDetector::getDetectableCount();

    UEnumeration *en = NEW_ARRAY(UEnumeration, 1);
    memcpy(en, &gCSDetEnumeration, sizeof(UEnumeration));
    en->context = (void*)NEW_ARRAY(Context, 1);
    uprv_memset(en->context, 0, sizeof(Context));
    return en;
}
U_CDECL_END

#endif

