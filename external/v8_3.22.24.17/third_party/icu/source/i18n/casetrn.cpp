/*
*******************************************************************************
*
*   Copyright (C) 2001-2010, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  casetrn.cpp
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2004sep03
*   created by: Markus W. Scherer
*
*   Implementation class for lower-/upper-/title-casing transliterators.
*/

#include "unicode/utypes.h"

#if !UCONFIG_NO_TRANSLITERATION

#include "unicode/uchar.h"
#include "unicode/ustring.h"
#include "tolowtrn.h"
#include "ucase.h"
#include "cpputils.h"

/* case context iterator using a Replaceable */
U_CFUNC UChar32 U_CALLCONV
utrans_rep_caseContextIterator(void *context, int8_t dir)
{
    U_NAMESPACE_USE

    UCaseContext *csc=(UCaseContext *)context;
    Replaceable *rep=(Replaceable *)csc->p;
    UChar32 c;

    if(dir<0) {
        /* reset for backward iteration */
        csc->index=csc->cpStart;
        csc->dir=dir;
    } else if(dir>0) {
        /* reset for forward iteration */
        csc->index=csc->cpLimit;
        csc->dir=dir;
    } else {
        /* continue current iteration direction */
        dir=csc->dir;
    }

    // automatically adjust start and limit if the Replaceable disagrees
    // with the original values
    if(dir<0) {
        if(csc->start<csc->index) {
            c=rep->char32At(csc->index-1);
            if(c<0) {
                csc->start=csc->index;
            } else {
                csc->index-=U16_LENGTH(c);
                return c;
            }
        }
    } else {
        // detect, and store in csc->b1, if we hit the limit
        if(csc->index<csc->limit) {
            c=rep->char32At(csc->index);
            if(c<0) {
                csc->limit=csc->index;
                csc->b1=TRUE;
            } else {
                csc->index+=U16_LENGTH(c);
                return c;
            }
        } else {
            csc->b1=TRUE;
        }
    }
    return U_SENTINEL;
}

U_NAMESPACE_BEGIN

UOBJECT_DEFINE_ABSTRACT_RTTI_IMPLEMENTATION(CaseMapTransliterator)

/**
 * Constructs a transliterator.
 */
CaseMapTransliterator::CaseMapTransliterator(const UnicodeString &id, UCaseMapFull *map) : 
    Transliterator(id, 0),
    fCsp(ucase_getSingleton()),
    fMap(map)
{
    // TODO test incremental mode with context-sensitive text (e.g. greek sigma)
    // TODO need to call setMaximumContextLength()?!
}

/**
 * Destructor.
 */
CaseMapTransliterator::~CaseMapTransliterator() {
}

/**
 * Copy constructor.
 */
CaseMapTransliterator::CaseMapTransliterator(const CaseMapTransliterator& o) :
    Transliterator(o),
    fCsp(o.fCsp), fMap(o.fMap)
{
}

/**
 * Assignment operator.
 */
/*CaseMapTransliterator& CaseMapTransliterator::operator=(const CaseMapTransliterator& o) {
    Transliterator::operator=(o);
    fCsp = o.fCsp;
    fMap = o.fMap;
    return *this;
}*/

/**
 * Transliterator API.
 */
/*Transliterator* CaseMapTransliterator::clone(void) const {
    return new CaseMapTransliterator(*this);
}*/

/**
 * Implements {@link Transliterator#handleTransliterate}.
 */
void CaseMapTransliterator::handleTransliterate(Replaceable& text,
                                 UTransPosition& offsets, 
                                 UBool isIncremental) const
{
    if (offsets.start >= offsets.limit) {
        return;
    }

    UCaseContext csc;
    uprv_memset(&csc, 0, sizeof(csc));
    csc.p = &text;
    csc.start = offsets.contextStart;
    csc.limit = offsets.contextLimit;

    UnicodeString tmp;
    const UChar *s;
    UChar32 c;
    int32_t textPos, delta, result, locCache=0;

    for(textPos=offsets.start; textPos<offsets.limit;) {
        csc.cpStart=textPos;
        c=text.char32At(textPos);
        csc.cpLimit=textPos+=U16_LENGTH(c);

        result=fMap(fCsp, c, utrans_rep_caseContextIterator, &csc, &s, "", &locCache);

        if(csc.b1 && isIncremental) {
            // fMap() tried to look beyond the context limit
            // wait for more input
            offsets.start=csc.cpStart;
            return;
        }

        if(result>=0) {
            // replace the current code point with its full case mapping result
            // see UCASE_MAX_STRING_LENGTH
            if(result<=UCASE_MAX_STRING_LENGTH) {
                // string s[result]
                tmp.setTo(FALSE, s, result);
                delta=result-U16_LENGTH(c);
            } else {
                // single code point
                tmp.setTo(result);
                delta=tmp.length()-U16_LENGTH(c);
            }
            text.handleReplaceBetween(csc.cpStart, textPos, tmp);
            if(delta!=0) {
                textPos+=delta;
                csc.limit=offsets.contextLimit+=delta;
                offsets.limit+=delta;
            }
        }
    }
    offsets.start=textPos;
}

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_TRANSLITERATION */
