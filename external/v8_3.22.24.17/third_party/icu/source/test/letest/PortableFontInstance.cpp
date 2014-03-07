/*
 *******************************************************************************
 *
 *   Copyright (C) 1999-2008, International Business Machines
 *   Corporation and others.  All Rights Reserved.
 *
 *******************************************************************************
 *   file name:  PortableFontInstance.cpp
 *
 *   created on: 11/22/1999
 *   created by: Eric R. Mader
 */

#include <stdio.h>

#include "layout/LETypes.h"
#include "layout/LEFontInstance.h"
#include "layout/LESwaps.h"

#include "PortableFontInstance.h"

#include "letest.h"
#include "sfnt.h"

#include <string.h>

//
// Finds the high bit by binary searching
// through the bits in n.
//
le_int8 PortableFontInstance::highBit(le_int32 value)
{
    if (value <= 0) {
        return -32;
    }

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

PortableFontInstance::PortableFontInstance(const char *fileName, float pointSize, LEErrorCode &status)
    : fFile(NULL), fPointSize(pointSize), fUnitsPerEM(0), fFontChecksum(0), fAscent(0), fDescent(0), fLeading(0),
      fDirectory(NULL), fNAMETable(NULL), fNameCount(0), fNameStringOffset(0), fCMAPMapper(NULL), fHMTXTable(NULL), fNumGlyphs(0), fNumLongHorMetrics(0)
{
    if (LE_FAILURE(status)) {
        return;
    }

    // open the font file
    fFile = fopen(fileName, "rb");

    if (fFile == NULL) {
        status = LE_FONT_FILE_NOT_FOUND_ERROR;
        return;
    }

    // read in the directory
    SFNTDirectory tempDir;

    fread(&tempDir, sizeof tempDir, 1, fFile);

    le_int32 dirSize = sizeof tempDir + ((SWAPW(tempDir.numTables) - ANY_NUMBER) * sizeof(DirectoryEntry));
    const LETag headTag = LE_HEAD_TABLE_TAG;
    const LETag hheaTag = LE_HHEA_TABLE_TAG;
    const HEADTable *headTable = NULL;
    const HHEATable *hheaTable = NULL;
//  const NAMETable *nameTable = NULL;
    le_uint16 numTables = 0;

    fDirectory = (const SFNTDirectory *) NEW_ARRAY(char, dirSize);

    if (fDirectory == NULL) {
        status = LE_MEMORY_ALLOCATION_ERROR;
        goto error_exit;
    }

    fseek(fFile, 0L, SEEK_SET);
    fread((void *) fDirectory, sizeof(char), dirSize, fFile);

    //
    // We calculate these numbers 'cause some fonts
    // have bogus values for them in the directory header.
    //
    numTables = SWAPW(fDirectory->numTables);
    fDirPower = 1 << highBit(numTables);
    fDirExtra = numTables - fDirPower;

    // read unitsPerEm from 'head' table
    headTable = (const HEADTable *) readFontTable(headTag);

    if (headTable == NULL) {
        status = LE_MISSING_FONT_TABLE_ERROR;
        goto error_exit;
    }

    fUnitsPerEM   = SWAPW(headTable->unitsPerEm);
    fFontChecksum = SWAPL(headTable->checksumAdjustment);
    freeFontTable(headTable);

    //nameTable = (NAMETable *) readFontTable(nameTag);

    //if (nameTable == NULL) {
    //    status = LE_MISSING_FONT_TABLE_ERROR;
    //    goto error_exit;
    //}

    //fFontVersionString = findName(nameTable, NAME_VERSION_STRING, PLATFORM_MACINTOSH, MACINTOSH_ROMAN, MACINTOSH_ENGLISH);

    //if (fFontVersionString == NULL) {
    //    status = LE_MISSING_FONT_TABLE_ERROR;
    //    goto error_exit;
    //}

    //freeFontTable(nameTable);

    hheaTable = (HHEATable *) readFontTable(hheaTag);

    if (hheaTable == NULL) {
        status = LE_MISSING_FONT_TABLE_ERROR;
        goto error_exit;
    }

    fAscent  = (le_int32) yUnitsToPoints((float) SWAPW(hheaTable->ascent));
    fDescent = (le_int32) yUnitsToPoints((float) SWAPW(hheaTable->descent));
    fLeading = (le_int32) yUnitsToPoints((float) SWAPW(hheaTable->lineGap));

    fNumLongHorMetrics = SWAPW(hheaTable->numOfLongHorMetrics);

    freeFontTable((void *) hheaTable);

    fCMAPMapper = findUnicodeMapper();

    if (fCMAPMapper == NULL) {
        status = LE_MISSING_FONT_TABLE_ERROR;
        goto error_exit;
    }

    return;

error_exit:
    fclose(fFile);
    fFile = NULL;
    return;
}

PortableFontInstance::~PortableFontInstance()
{
    if (fFile != NULL) {
        fclose(fFile);

        freeFontTable(fHMTXTable);
        freeFontTable(fNAMETable);

        delete fCMAPMapper;

        DELETE_ARRAY(fDirectory);
    }
}

const DirectoryEntry *PortableFontInstance::findTable(LETag tag) const
{
    if (fDirectory != NULL) {
        le_uint16 table = 0;
        le_uint16 probe = fDirPower;

        if (SWAPL(fDirectory->tableDirectory[fDirExtra].tag) <= tag) {
            table = fDirExtra;
        }

        while (probe > (1 << 0)) {
            probe >>= 1;

            if (SWAPL(fDirectory->tableDirectory[table + probe].tag) <= tag) {
                table += probe;
            }
        }

        if (SWAPL(fDirectory->tableDirectory[table].tag) == tag) {
            return &fDirectory->tableDirectory[table];
        }
    }

    return NULL;
}

const void *PortableFontInstance::readTable(LETag tag, le_uint32 *length) const
{
    const DirectoryEntry *entry = findTable(tag);

    if (entry == NULL) {
        *length = 0;
        return NULL;
    }

    *length = SWAPL(entry->length);

    void *table = NEW_ARRAY(char, *length);

    if (table != NULL) {
        fseek(fFile, SWAPL(entry->offset), SEEK_SET);
        fread(table, sizeof(char), *length, fFile);
    }

    return table;
}

const void *PortableFontInstance::getFontTable(LETag tableTag) const
{
    return FontTableCache::find(tableTag);
}

const void *PortableFontInstance::readFontTable(LETag tableTag) const
{
    le_uint32 len;

    return readTable(tableTag, &len);
}

CMAPMapper *PortableFontInstance::findUnicodeMapper()
{
    LETag cmapTag = LE_CMAP_TABLE_TAG;
    const CMAPTable *cmap = (CMAPTable *) readFontTable(cmapTag);

    if (cmap == NULL) {
        return NULL;
    }

    return CMAPMapper::createUnicodeMapper(cmap);
}

const char *PortableFontInstance::getNameString(le_uint16 nameID, le_uint16 platformID, le_uint16 encodingID, le_uint16 languageID) const
{
    if (fNAMETable == NULL) {
        LETag nameTag = LE_NAME_TABLE_TAG;
        PortableFontInstance *realThis = (PortableFontInstance *) this;

        realThis->fNAMETable = (const NAMETable *) readFontTable(nameTag);

        if (realThis->fNAMETable != NULL) {
            realThis->fNameCount        = SWAPW(realThis->fNAMETable->count);
            realThis->fNameStringOffset = SWAPW(realThis->fNAMETable->stringOffset);
        }
    }

    for(le_int32 i = 0; i < fNameCount; i += 1) {
        const NameRecord *nameRecord = &fNAMETable->nameRecords[i];
        
        if (SWAPW(nameRecord->platformID) == platformID && SWAPW(nameRecord->encodingID) == encodingID &&
            SWAPW(nameRecord->languageID) == languageID && SWAPW(nameRecord->nameID) == nameID) {
            char *name = ((char *) fNAMETable) + fNameStringOffset + SWAPW(nameRecord->offset);
            le_uint16 length = SWAPW(nameRecord->length);
            char *result = NEW_ARRAY(char, length + 2);

            ARRAY_COPY(result, name, length);
            result[length] = result[length + 1] = 0;

            return result;
        }
    }

    return NULL;
}

const LEUnicode16 *PortableFontInstance::getUnicodeNameString(le_uint16 nameID, le_uint16 platformID, le_uint16 encodingID, le_uint16 languageID) const
{
    if (fNAMETable == NULL) {
        LETag nameTag = LE_NAME_TABLE_TAG;
        PortableFontInstance *realThis = (PortableFontInstance *) this;

        realThis->fNAMETable = (const NAMETable *) readFontTable(nameTag);

        if (realThis->fNAMETable != NULL) {
            realThis->fNameCount        = SWAPW(realThis->fNAMETable->count);
            realThis->fNameStringOffset = SWAPW(realThis->fNAMETable->stringOffset);
        }
    }

    for(le_int32 i = 0; i < fNameCount; i += 1) {
        const NameRecord *nameRecord = &fNAMETable->nameRecords[i];
        
        if (SWAPW(nameRecord->platformID) == platformID && SWAPW(nameRecord->encodingID) == encodingID &&
            SWAPW(nameRecord->languageID) == languageID && SWAPW(nameRecord->nameID) == nameID) {
            LEUnicode16 *name = (LEUnicode16 *) (((char *) fNAMETable) + fNameStringOffset + SWAPW(nameRecord->offset));
            le_uint16 length = SWAPW(nameRecord->length) / 2;
            LEUnicode16 *result = NEW_ARRAY(LEUnicode16, length + 2);

            for (le_int32 c = 0; c < length; c += 1) {
                result[c] = SWAPW(name[c]);
            }

            result[length] = 0;

            return result;
        }
    }

    return NULL;
}

void PortableFontInstance::deleteNameString(const char *name) const
{
    DELETE_ARRAY(name);
}

void PortableFontInstance::deleteNameString(const LEUnicode16 *name) const
{
    DELETE_ARRAY(name);
}

void PortableFontInstance::getGlyphAdvance(LEGlyphID glyph, LEPoint &advance) const
{
    TTGlyphID ttGlyph = (TTGlyphID) LE_GET_GLYPH(glyph);

    if (fHMTXTable == NULL) {
        LETag maxpTag = LE_MAXP_TABLE_TAG;
        LETag hmtxTag = LE_HMTX_TABLE_TAG;
        const MAXPTable *maxpTable = (MAXPTable *) readFontTable(maxpTag);
        PortableFontInstance *realThis = (PortableFontInstance *) this;

        if (maxpTable != NULL) {
            realThis->fNumGlyphs = SWAPW(maxpTable->numGlyphs);
            freeFontTable(maxpTable);
        }

        realThis->fHMTXTable = (const HMTXTable *) readFontTable(hmtxTag);
    }

    le_uint16 index = ttGlyph;

    if (ttGlyph >= fNumGlyphs || fHMTXTable == NULL) {
        advance.fX = advance.fY = 0;
        return;
    }

    if (ttGlyph >= fNumLongHorMetrics) {
        index = fNumLongHorMetrics - 1;
    }

    advance.fX = xUnitsToPoints(SWAPW(fHMTXTable->hMetrics[index].advanceWidth));
    advance.fY = 0;
}

le_bool PortableFontInstance::getGlyphPoint(LEGlyphID /*glyph*/, le_int32 /*pointNumber*/, LEPoint &/*point*/) const
{
    return FALSE;
}

le_int32 PortableFontInstance::getUnitsPerEM() const
{
    return fUnitsPerEM;
}

le_uint32 PortableFontInstance::getFontChecksum() const
{
    return fFontChecksum;
}

le_int32 PortableFontInstance::getAscent() const
{
    return fAscent;
}

le_int32 PortableFontInstance::getDescent() const
{
    return fDescent;
}

le_int32 PortableFontInstance::getLeading() const
{
    return fLeading;
}

// We really want to inherit this method from the superclass, but some compilers
// issue a warning if we don't implement it...
LEGlyphID PortableFontInstance::mapCharToGlyph(LEUnicode32 ch, const LECharMapper *mapper, le_bool filterZeroWidth) const
{
    return LEFontInstance::mapCharToGlyph(ch, mapper, filterZeroWidth);
}

// We really want to inherit this method from the superclass, but some compilers
// issue a warning if we don't implement it...
LEGlyphID PortableFontInstance::mapCharToGlyph(LEUnicode32 ch, const LECharMapper *mapper) const
{
    return LEFontInstance::mapCharToGlyph(ch, mapper);
}

LEGlyphID PortableFontInstance::mapCharToGlyph(LEUnicode32 ch) const
{
    return fCMAPMapper->unicodeToGlyph(ch);
}

float PortableFontInstance::getXPixelsPerEm() const
{
    return fPointSize;
}

float PortableFontInstance::getYPixelsPerEm() const
{
    return fPointSize;
}

float PortableFontInstance::getScaleFactorX() const
{
    return 1.0;
}

float PortableFontInstance::getScaleFactorY() const
{
    return 1.0;
}
