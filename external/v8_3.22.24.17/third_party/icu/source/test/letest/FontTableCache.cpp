/*
 **********************************************************************
 *   Copyright (C) 2003-2008, International Business Machines
 *   Corporation and others.  All Rights Reserved.
 **********************************************************************
 */

#include "layout/LETypes.h"

#include "letest.h"
#include "FontTableCache.h"

#define TABLE_CACHE_INIT 5
#define TABLE_CACHE_GROW 5

struct FontTableCacheEntry
{
    LETag tag;
    const void *table;
};

FontTableCache::FontTableCache()
    : fTableCacheCurr(0), fTableCacheSize(TABLE_CACHE_INIT)
{
    fTableCache = NEW_ARRAY(FontTableCacheEntry, fTableCacheSize);

    if (fTableCache == NULL) {
        fTableCacheSize = 0;
        return;
    }

    for (int i = 0; i < fTableCacheSize; i += 1) {
        fTableCache[i].tag   = 0;
        fTableCache[i].table = NULL;
    }
}

FontTableCache::~FontTableCache()
{
    for (int i = fTableCacheCurr - 1; i >= 0; i -= 1) {
        DELETE_ARRAY(fTableCache[i].table);

        fTableCache[i].tag   = 0;
        fTableCache[i].table = NULL;
    }

    fTableCacheCurr = 0;

    DELETE_ARRAY(fTableCache);
}

void FontTableCache::freeFontTable(const void *table) const
{
    DELETE_ARRAY(table);
}

const void *FontTableCache::find(LETag tableTag) const
{
    for (int i = 0; i < fTableCacheCurr; i += 1) {
        if (fTableCache[i].tag == tableTag) {
            return fTableCache[i].table;
        }
    }

    const void *table = readFontTable(tableTag);

    ((FontTableCache *) this)->add(tableTag, table);

    return table;
}

void FontTableCache::add(LETag tableTag, const void *table)
{
    if (fTableCacheCurr >= fTableCacheSize) {
        le_int32 newSize = fTableCacheSize + TABLE_CACHE_GROW;

        fTableCache = (FontTableCacheEntry *) GROW_ARRAY(fTableCache, newSize);

        for (le_int32 i = fTableCacheSize; i < newSize; i += 1) {
            fTableCache[i].tag   = 0;
            fTableCache[i].table = NULL;
        }

        fTableCacheSize = newSize;
    }

    fTableCache[fTableCacheCurr].tag   = tableTag;
    fTableCache[fTableCacheCurr].table = table;

    fTableCacheCurr += 1;
}
