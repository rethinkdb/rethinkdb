/***************************************************************************
*
*   Copyright (C) 1998-2002, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
************************************************************************/

#include <stdio.h>

#include "LETypes.h"
#include "FontObject.h"
#include "LESwaps.h"

FontObject::FontObject(char *fileName)
  : directory(NULL), numTables(0), searchRange(0),entrySelector(0),
    cmapTable(NULL), cmSegCount(0), cmSearchRange(0), cmEntrySelector(0),
    cmEndCodes(NULL), cmStartCodes(NULL), cmIdDelta(0), cmIdRangeOffset(0),
    headTable(NULL), hmtxTable(NULL), numGlyphs(0), numOfLongHorMetrics(0), file(NULL)
{
    file = fopen(fileName, "rb");

    if (file == NULL) {
        printf("?? Couldn't open %s", fileName);
        return;
    }

    SFNTDirectory tempDir;

    fread(&tempDir, sizeof tempDir, 1, file);

    numTables       = SWAPW(tempDir.numTables);
    searchRange     = SWAPW(tempDir.searchRange) >> 4;
    entrySelector   = SWAPW(tempDir.entrySelector);
    rangeShift      = SWAPW(tempDir.rangeShift) >> 4;

    int dirSize = sizeof tempDir + ((numTables - ANY_NUMBER) * sizeof(DirectoryEntry));

    directory = (SFNTDirectory *) new char[dirSize];

    fseek(file, 0L, SEEK_SET);
    fread(directory, sizeof(char), dirSize, file);

    initUnicodeCMAP();
}

FontObject::~FontObject()
{
    fclose(file);
    delete[] directory;
    delete[] cmapTable;
    delete[] headTable;
    delete[] hmtxTable;
}

void FontObject::deleteTable(void *table)
{
    delete[] (char *) table;
}

DirectoryEntry *FontObject::findTable(LETag tag)
{
    le_uint16 table = 0;
    le_uint16 probe = 1 << entrySelector;

    if (SWAPL(directory->tableDirectory[rangeShift].tag) <= tag) {
        table = rangeShift;
    }

    while (probe > (1 << 0)) {
        probe >>= 1;

        if (SWAPL(directory->tableDirectory[table + probe].tag) <= tag) {
            table += probe;
        }
    }

    if (SWAPL(directory->tableDirectory[table].tag) == tag) {
        return &directory->tableDirectory[table];
    }

    return NULL;
}

void *FontObject::readTable(LETag tag, le_uint32 *length)
{
    DirectoryEntry *entry = findTable(tag);

    if (entry == NULL) {
        *length = 0;
        return NULL;
    }

    *length = SWAPL(entry->length);

    void *table = new char[*length];

    fseek(file, SWAPL(entry->offset), SEEK_SET);
    fread(table, sizeof(char), *length, file);

    return table;
}

CMAPEncodingSubtable *FontObject::findCMAP(le_uint16 platformID, le_uint16 platformSpecificID)
{
    LETag cmapTag = 0x636D6170; // 'cmap'

    if (cmapTable == NULL) {
        le_uint32 length;

        cmapTable = (CMAPTable *) readTable(cmapTag, &length);
    }

    if (cmapTable != NULL) {
        le_uint16 i;
        le_uint16 nSubtables = SWAPW(cmapTable->numberSubtables);


        for (i = 0; i < nSubtables; i += 1) {
            CMAPEncodingSubtableHeader *esh = &cmapTable->encodingSubtableHeaders[i];

            if (SWAPW(esh->platformID) == platformID &&
                SWAPW(esh->platformSpecificID) == platformSpecificID) {
                return (CMAPEncodingSubtable *) ((char *) cmapTable + SWAPL(esh->encodingOffset));
            }
        }
    }

    return NULL;
}

void FontObject::initUnicodeCMAP()
{
    CMAPEncodingSubtable *encodingSubtable = findCMAP(3, 1);

    if (encodingSubtable == 0 ||
        SWAPW(encodingSubtable->format) != 4) {
        printf("Can't find unicode 'cmap'");
        return;
    }

    CMAPFormat4Encoding *header = (CMAPFormat4Encoding *) encodingSubtable;

    cmSegCount = SWAPW(header->segCountX2) / 2;
    cmSearchRange = SWAPW(header->searchRange);
    cmEntrySelector = SWAPW(header->entrySelector);
    cmRangeShift = SWAPW(header->rangeShift) / 2;
    cmEndCodes = &header->endCodes[0];
    cmStartCodes = &header->endCodes[cmSegCount + 1]; // + 1 for reservedPad...
    cmIdDelta = &cmStartCodes[cmSegCount];
    cmIdRangeOffset = &cmIdDelta[cmSegCount];
}

LEGlyphID FontObject::unicodeToGlyph(LEUnicode32 unicode32)
{
    if (unicode32 >= 0x10000) {
        return 0;
    }

    LEUnicode16 unicode = (LEUnicode16) unicode32;
    le_uint16 index = 0;
    le_uint16 probe = 1 << cmEntrySelector;
    LEGlyphID result = 0;

    if (SWAPW(cmStartCodes[cmRangeShift]) <= unicode) {
        index = cmRangeShift;
    }

    while (probe > (1 << 0)) {
        probe >>= 1;

        if (SWAPW(cmStartCodes[index + probe]) <= unicode) {
            index += probe;
        }
    }

    if (unicode >= SWAPW(cmStartCodes[index]) && unicode <= SWAPW(cmEndCodes[index])) {
        if (cmIdRangeOffset[index] == 0) {
            result = (LEGlyphID) unicode;
        } else {
            le_uint16 offset = unicode - SWAPW(cmStartCodes[index]);
            le_uint16 rangeOffset = SWAPW(cmIdRangeOffset[index]);
            le_uint16 *glyphIndexTable = (le_uint16 *) ((char *) &cmIdRangeOffset[index] + rangeOffset);

            result = SWAPW(glyphIndexTable[offset]);
        }

        result += SWAPW(cmIdDelta[index]);
    } else {
        result = 0;
    }

    return result;
}

le_uint16 FontObject::getUnitsPerEM()
{
    if (headTable == NULL) {
        LETag headTag = 0x68656164; // 'head'
        le_uint32 length;

        headTable = (HEADTable *) readTable(headTag, &length);
    }

    return SWAPW(headTable->unitsPerEm);
}

le_uint16 FontObject::getGlyphAdvance(LEGlyphID glyph)
{
    if (hmtxTable == NULL) {
        LETag maxpTag = 0x6D617870; // 'maxp'
        LETag hheaTag = 0x68686561; // 'hhea'
        LETag hmtxTag = 0x686D7478; // 'hmtx'
        le_uint32 length;
        HHEATable *hheaTable;
        MAXPTable *maxpTable = (MAXPTable *) readTable(maxpTag, &length);

        numGlyphs = SWAPW(maxpTable->numGlyphs);
        deleteTable(maxpTable);

        hheaTable = (HHEATable *) readTable(hheaTag, &length);
        numOfLongHorMetrics = SWAPW(hheaTable->numOfLongHorMetrics);
        deleteTable(hheaTable);

        hmtxTable = (HMTXTable *) readTable(hmtxTag, &length);
    }

    le_uint16 index = glyph;

    if (glyph >= numGlyphs) {
        return 0;
    }

    if (glyph >= numOfLongHorMetrics) {
        index = numOfLongHorMetrics - 1;
    }

    return SWAPW(hmtxTable->hMetrics[index].advanceWidth);
}


