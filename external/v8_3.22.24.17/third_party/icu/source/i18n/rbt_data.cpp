/*
**********************************************************************
*   Copyright (C) 1999-2008, International Business Machines
*   Corporation and others.  All Rights Reserved.
**********************************************************************
*   Date        Name        Description
*   11/17/99    aliu        Creation.
**********************************************************************
*/

#include "unicode/utypes.h"
#include "umutex.h"

#if !UCONFIG_NO_TRANSLITERATION

#include "unicode/unistr.h"
#include "unicode/uniset.h"
#include "rbt_data.h"
#include "hash.h"
#include "cmemory.h"

U_NAMESPACE_BEGIN

TransliterationRuleData::TransliterationRuleData(UErrorCode& status)
 : UMemory(), ruleSet(status), variableNames(status),
    variables(0), variablesAreOwned(TRUE)
{
    if (U_FAILURE(status)) {
        return;
    }
    variableNames.setValueDeleter(uhash_deleteUnicodeString);
    variables = 0;
    variablesLength = 0;
}

TransliterationRuleData::TransliterationRuleData(const TransliterationRuleData& other) :
    UMemory(other), ruleSet(other.ruleSet),
    variablesAreOwned(TRUE),
    variablesBase(other.variablesBase),
    variablesLength(other.variablesLength)
{
    UErrorCode status = U_ZERO_ERROR;
    int32_t i = 0;
    variableNames.setValueDeleter(uhash_deleteUnicodeString);
    int32_t pos = -1;
    const UHashElement *e;
    while ((e = other.variableNames.nextElement(pos)) != 0) {
        UnicodeString* value =
            new UnicodeString(*(const UnicodeString*)e->value.pointer);
        // Exit out if value could not be created.
        if (value == NULL) {
        	return;
        }
        variableNames.put(*(UnicodeString*)e->key.pointer, value, status);
    }

    variables = 0;
    if (other.variables != 0) {
        variables = (UnicodeFunctor **)uprv_malloc(variablesLength * sizeof(UnicodeFunctor *));
        /* test for NULL */
        if (variables == 0) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return;
        }
        for (i=0; i<variablesLength; ++i) {
            variables[i] = other.variables[i]->clone();
            if (variables[i] == NULL) {
                status = U_MEMORY_ALLOCATION_ERROR;
                break;
            }
        }
    }
    // Remove the array and exit if memory allocation error occured.
    if (U_FAILURE(status)) {
        for (int32_t n = i-1; n >= 0; n--) {
            delete variables[n];
        }
        uprv_free(variables);
        variables = NULL;
        return;
    }

    // Do this last, _after_ setting up variables[].
    ruleSet.setData(this); // ruleSet must already be frozen
}

TransliterationRuleData::~TransliterationRuleData() {
    if (variablesAreOwned && variables != 0) {
        for (int32_t i=0; i<variablesLength; ++i) {
            delete variables[i];
        }
    }
    uprv_free(variables);
}

UnicodeFunctor*
TransliterationRuleData::lookup(UChar32 standIn) const {
    int32_t i = standIn - variablesBase;
    return (i >= 0 && i < variablesLength) ? variables[i] : 0;
}

UnicodeMatcher*
TransliterationRuleData::lookupMatcher(UChar32 standIn) const {
    UnicodeFunctor *f = lookup(standIn);
    return (f != 0) ? f->toMatcher() : 0;
}

UnicodeReplacer*
TransliterationRuleData::lookupReplacer(UChar32 standIn) const {
    UnicodeFunctor *f = lookup(standIn);
    return (f != 0) ? f->toReplacer() : 0;
}


U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_TRANSLITERATION */
