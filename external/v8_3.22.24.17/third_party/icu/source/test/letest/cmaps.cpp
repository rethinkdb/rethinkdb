/***************************************************************************
*
*   Copyright (C) 1998-2003, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
************************************************************************/

#include "LETypes.h"
#include "LESwaps.h"

#include "sfnt.h"
#include "cmaps.h"

#define SWAPU16(code) ((LEUnicode16) SWAPW(code))
#define SWAPU32(code) ((LEUnicode32) SWAPL(code))

//
// Finds the high bit by binary searching
// through the bits in value.
//
le_int8 highBit(le_uint32 value)
{
    le_uint8 bit = 0;

    if (value >= 1 << 16) {
        value >>= 16;
        bit += 16;
    }

    if (value >= 1 << 8) {
        value >>= 8;
        bit += 8;
    }

    if (value >= 1 << 4) {
        value >>= 4;
        bit += 4;
    }

    if (value >= 1 << 2) {
        value >>= 2;
        bit += 2;
    }

    if (value >= 1 << 1) {
        value >>= 1;
        bit += 1;
    }

    return bit;
}

CMAPMapper *CMAPMapper::createUnicodeMapper(const CMAPTable *cmap)
{
    le_uint16 i;
    le_uint16 nSubtables = SWAPW(cmap->numberSubtables);
    const CMAPEncodingSubtable *subtable = NULL;
    le_uint32 offset1 = 0, offset10 = 0;

    for (i = 0; i < nSubtables; i += 1) {
        const CMAPEncodingSubtableHeader *esh = &cmap->encodingSubtableHeaders[i];

        if (SWAPW(esh->platformID) == 3) {
            switch (SWAPW(esh->platformSpecificID)) {
            case 1:
                offset1 = SWAPL(esh->encodingOffset);
                break;

            case 10:
                offset10 = SWAPL(esh->encodingOffset);
                break;
            }
        }
    }


    if (offset10 != 0)
    {
        subtable = (const CMAPEncodingSubtable *) ((const char *) cmap + offset10);
    } else if (offset1 != 0) {
        subtable = (const CMAPEncodingSubtable *) ((const char *) cmap + offset1);
    } else {
        return NULL;
    }

    switch (SWAPW(subtable->format)) {
    case 4:
        return new CMAPFormat4Mapper(cmap, (const CMAPFormat4Encoding *) subtable);

    case 12:
    {
        const CMAPFormat12Encoding *encoding = (const CMAPFormat12Encoding *) subtable;

        return new CMAPGroupMapper(cmap, encoding->groups, SWAPL(encoding->nGroups));
    }

    default:
        break;
    }

    return NULL;
}

CMAPFormat4Mapper::CMAPFormat4Mapper(const CMAPTable *cmap, const CMAPFormat4Encoding *header)
    : CMAPMapper(cmap)
{
    le_uint16 segCount = SWAPW(header->segCountX2) / 2;

    fEntrySelector = SWAPW(header->entrySelector);
    fRangeShift = SWAPW(header->rangeShift) / 2;
    fEndCodes = &header->endCodes[0];
    fStartCodes = &header->endCodes[segCount + 1]; // + 1 for reservedPad...
    fIdDelta = &fStartCodes[segCount];
    fIdRangeOffset = &fIdDelta[segCount];
}

LEGlyphID CMAPFormat4Mapper::unicodeToGlyph(LEUnicode32 unicode32) const
{
    if (unicode32 >= 0x10000) {
        return 0;
    }

    LEUnicode16 unicode = (LEUnicode16) unicode32;
    le_uint16 index = 0;
    le_uint16 probe = 1 << fEntrySelector;
    TTGlyphID result = 0;

    if (SWAPU16(fStartCodes[fRangeShift]) <= unicode) {
        index = fRangeShift;
    }

    while (probe > (1 << 0)) {
        probe >>= 1;

        if (SWAPU16(fStartCodes[index + probe]) <= unicode) {
            index += probe;
        }
    }

    if (unicode >= SWAPU16(fStartCodes[index]) && unicode <= SWAPU16(fEndCodes[index])) {
        if (fIdRangeOffset[index] == 0) {
            result = (TTGlyphID) unicode;
        } else {
            le_uint16 offset = unicode - SWAPU16(fStartCodes[index]);
            le_uint16 rangeOffset = SWAPW(fIdRangeOffset[index]);
            le_uint16 *glyphIndexTable = (le_uint16 *) ((char *) &fIdRangeOffset[index] + rangeOffset);

            result = SWAPW(glyphIndexTable[offset]);
        }

        result += SWAPW(fIdDelta[index]);
    } else {
        result = 0;
    }

    return LE_SET_GLYPH(0, result);
}

CMAPFormat4Mapper::~CMAPFormat4Mapper()
{
    // parent destructor does it all
}

CMAPGroupMapper::CMAPGroupMapper(const CMAPTable *cmap, const CMAPGroup *groups, le_uint32 nGroups)
    : CMAPMapper(cmap), fGroups(groups)
{
    le_uint8 bit = highBit(nGroups);
    fPower = 1 << bit;
    fRangeOffset = nGroups - fPower;
}

LEGlyphID CMAPGroupMapper::unicodeToGlyph(LEUnicode32 unicode32) const
{
    le_int32 probe = fPower;
    le_int32 range = 0;

    if (SWAPU32(fGroups[fRangeOffset].startCharCode) <= unicode32) {
        range = fRangeOffset;
    }

    while (probe > (1 << 0)) {
        probe >>= 1;

        if (SWAPU32(fGroups[range + probe].startCharCode) <= unicode32) {
            range += probe;
        }
    }

    if (SWAPU32(fGroups[range].startCharCode) <= unicode32 && SWAPU32(fGroups[range].endCharCode) >= unicode32) {
        return (LEGlyphID) (SWAPU32(fGroups[range].startGlyphCode) + unicode32 - SWAPU32(fGroups[range].startCharCode));
    }

    return 0;
}

CMAPGroupMapper::~CMAPGroupMapper()
{
    // parent destructor does it all
}

