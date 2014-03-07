/**
 ************************************************************************************
 * Copyright (C) 2006-2009, International Business Machines Corporation and others. *
 * All Rights Reserved.                                                             *
 ************************************************************************************
 */

#include "unicode/utypes.h"

#if !UCONFIG_NO_BREAK_ITERATION

#include "brkeng.h"
#include "dictbe.h"
#include "triedict.h"
#include "unicode/uchar.h"
#include "unicode/uniset.h"
#include "unicode/chariter.h"
#include "unicode/ures.h"
#include "unicode/udata.h"
#include "unicode/putil.h"
#include "unicode/ustring.h"
#include "unicode/uscript.h"
#include "uvector.h"
#include "umutex.h"
#include "uresimp.h"
#include "ubrkimpl.h"

U_NAMESPACE_BEGIN

/*
 ******************************************************************
 */

LanguageBreakEngine::LanguageBreakEngine() {
}

LanguageBreakEngine::~LanguageBreakEngine() {
}

/*
 ******************************************************************
 */

LanguageBreakFactory::LanguageBreakFactory() {
}

LanguageBreakFactory::~LanguageBreakFactory() {
}

/*
 ******************************************************************
 */

UnhandledEngine::UnhandledEngine(UErrorCode &/*status*/) {
    for (int32_t i = 0; i < (int32_t)(sizeof(fHandled)/sizeof(fHandled[0])); ++i) {
        fHandled[i] = 0;
    }
}

UnhandledEngine::~UnhandledEngine() {
    for (int32_t i = 0; i < (int32_t)(sizeof(fHandled)/sizeof(fHandled[0])); ++i) {
        if (fHandled[i] != 0) {
            delete fHandled[i];
        }
    }
}

UBool
UnhandledEngine::handles(UChar32 c, int32_t breakType) const {
    return (breakType >= 0 && breakType < (int32_t)(sizeof(fHandled)/sizeof(fHandled[0]))
        && fHandled[breakType] != 0 && fHandled[breakType]->contains(c));
}

int32_t
UnhandledEngine::findBreaks( UText *text,
                                 int32_t startPos,
                                 int32_t endPos,
                                 UBool reverse,
                                 int32_t breakType,
                                 UStack &/*foundBreaks*/ ) const {
    if (breakType >= 0 && breakType < (int32_t)(sizeof(fHandled)/sizeof(fHandled[0]))) {
        UChar32 c = utext_current32(text); 
        if (reverse) {
            while((int32_t)utext_getNativeIndex(text) > startPos && fHandled[breakType]->contains(c)) {
                c = utext_previous32(text);
            }
        }
        else {
            while((int32_t)utext_getNativeIndex(text) < endPos && fHandled[breakType]->contains(c)) {
                utext_next32(text);            // TODO:  recast loop to work with post-increment operations.
                c = utext_current32(text);
            }
        }
    }
    return 0;
}

void
UnhandledEngine::handleCharacter(UChar32 c, int32_t breakType) {
    if (breakType >= 0 && breakType < (int32_t)(sizeof(fHandled)/sizeof(fHandled[0]))) {
        if (fHandled[breakType] == 0) {
            fHandled[breakType] = new UnicodeSet();
            if (fHandled[breakType] == 0) {
                return;
            }
        }
        if (!fHandled[breakType]->contains(c)) {
            UErrorCode status = U_ZERO_ERROR;
            // Apply the entire script of the character.
            int32_t script = u_getIntPropertyValue(c, UCHAR_SCRIPT);
            fHandled[breakType]->applyIntPropertyValue(UCHAR_SCRIPT, script, status);
        }
    }
}

/*
 ******************************************************************
 */

ICULanguageBreakFactory::ICULanguageBreakFactory(UErrorCode &/*status*/) {
    fEngines = 0;
}

ICULanguageBreakFactory::~ICULanguageBreakFactory() {
    if (fEngines != 0) {
        delete fEngines;
    }
}

U_NAMESPACE_END
U_CDECL_BEGIN
static void U_CALLCONV _deleteEngine(void *obj) {
    delete (const U_NAMESPACE_QUALIFIER LanguageBreakEngine *) obj;
}
U_CDECL_END
U_NAMESPACE_BEGIN

const LanguageBreakEngine *
ICULanguageBreakFactory::getEngineFor(UChar32 c, int32_t breakType) {
    UBool       needsInit;
    int32_t     i;
    const LanguageBreakEngine *lbe = NULL;
    UErrorCode  status = U_ZERO_ERROR;

    // TODO: The global mutex should not be used.
    // The global mutex should only be used for short periods.
    // A ICULanguageBreakFactory specific mutex should be used.
    umtx_lock(NULL);
    needsInit = (UBool)(fEngines == NULL);
    if (!needsInit) {
        i = fEngines->size();
        while (--i >= 0) {
            lbe = (const LanguageBreakEngine *)(fEngines->elementAt(i));
            if (lbe != NULL && lbe->handles(c, breakType)) {
                break;
            }
            lbe = NULL;
        }
    }
    umtx_unlock(NULL);
    
    if (lbe != NULL) {
        return lbe;
    }
    
    if (needsInit) {
        UStack  *engines = new UStack(_deleteEngine, NULL, status);
        if (U_SUCCESS(status) && engines == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
        }
        else if (U_FAILURE(status)) {
            delete engines;
            engines = NULL;
        }
        else {
            umtx_lock(NULL);
            if (fEngines == NULL) {
                fEngines = engines;
                engines = NULL;
            }
            umtx_unlock(NULL);
            delete engines;
        }
    }
    
    if (fEngines == NULL) {
        return NULL;
    }

    // We didn't find an engine the first time through, or there was no
    // stack. Create an engine.
    const LanguageBreakEngine *newlbe = loadEngineFor(c, breakType);
    
    // Now get the lock, and see if someone else has created it in the
    // meantime
    umtx_lock(NULL);
    i = fEngines->size();
    while (--i >= 0) {
        lbe = (const LanguageBreakEngine *)(fEngines->elementAt(i));
        if (lbe != NULL && lbe->handles(c, breakType)) {
            break;
        }
        lbe = NULL;
    }
    if (lbe == NULL && newlbe != NULL) {
        fEngines->push((void *)newlbe, status);
        lbe = newlbe;
        newlbe = NULL;
    }
    umtx_unlock(NULL);
    
    delete newlbe;

    return lbe;
}

const LanguageBreakEngine *
ICULanguageBreakFactory::loadEngineFor(UChar32 c, int32_t breakType) {
    UErrorCode status = U_ZERO_ERROR;
    UScriptCode code = uscript_getScript(c, &status);
    if (U_SUCCESS(status)) {
        const CompactTrieDictionary *dict = loadDictionaryFor(code, breakType);
        if (dict != NULL) {
            const LanguageBreakEngine *engine = NULL;
            switch(code) {
            case USCRIPT_THAI:
                engine = new ThaiBreakEngine(dict, status);
                break;
                
            case USCRIPT_HANGUL:
                engine = new CjkBreakEngine(dict, kKorean, status);
                break;

            // use same BreakEngine and dictionary for both Chinese and Japanese
            case USCRIPT_HIRAGANA:
            case USCRIPT_KATAKANA:
            case USCRIPT_HAN:
                engine = new CjkBreakEngine(dict, kChineseJapanese, status);
                break;
#if 0
            // TODO: Have to get some characters with script=common handled
            // by CjkBreakEngine (e.g. U+309B). Simply subjecting
            // them to CjkBreakEngine does not work. The engine has to
            // special-case them.
            case USCRIPT_COMMON:
            {
                UBlockCode block = ublock_getCode(code);
                if (block == UBLOCK_HIRAGANA || block == UBLOCK_KATAKANA)
                   engine = new CjkBreakEngine(dict, kChineseJapanese, status);
                break;
            }
#endif
            default:
                break;
            }
            if (engine == NULL) {
                delete dict;
            }
            else if (U_FAILURE(status)) {
                delete engine;
                engine = NULL;
            }
            return engine;
        }
    }
    return NULL;
}

const CompactTrieDictionary *
ICULanguageBreakFactory::loadDictionaryFor(UScriptCode script, int32_t /*breakType*/) {
    UErrorCode status = U_ZERO_ERROR;
    // Open root from brkitr tree.
    char dictnbuff[256];
    char ext[4]={'\0'};

    UResourceBundle *b = ures_open(U_ICUDATA_BRKITR, "", &status);
    b = ures_getByKeyWithFallback(b, "dictionaries", b, &status);
    b = ures_getByKeyWithFallback(b, uscript_getShortName(script), b, &status);
    int32_t dictnlength = 0;
    const UChar *dictfname = ures_getString(b, &dictnlength, &status);
    if (U_SUCCESS(status) && (size_t)dictnlength >= sizeof(dictnbuff)) {
        dictnlength = 0;
        status = U_BUFFER_OVERFLOW_ERROR;
    }
    if (U_SUCCESS(status) && dictfname) {
        UChar* extStart=u_strchr(dictfname, 0x002e);
        int len = 0;
        if(extStart!=NULL){
            len = (int)(extStart-dictfname);
            u_UCharsToChars(extStart+1, ext, sizeof(ext)); // nul terminates the buff
            u_UCharsToChars(dictfname, dictnbuff, len);
        }
        dictnbuff[len]=0; // nul terminate
    }
    ures_close(b);
    UDataMemory *file = udata_open(U_ICUDATA_BRKITR, ext, dictnbuff, &status);
    if (U_SUCCESS(status)) {
        const CompactTrieDictionary *dict = new CompactTrieDictionary(
            file, status);
        if (U_SUCCESS(status) && dict == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
        }
        if (U_FAILURE(status)) {
            delete dict;
            dict = NULL;
        }
        return dict;
    } else if (dictfname != NULL){
        //create dummy dict if dictionary filename not valid
        UChar c = 0x0020;
        status = U_ZERO_ERROR;
        MutableTrieDictionary *mtd = new MutableTrieDictionary(c, status, TRUE);
        mtd->addWord(&c, 1, status, 1);
        return new CompactTrieDictionary(*mtd, status);  
    }
    return NULL;
}

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_BREAK_ITERATION */
