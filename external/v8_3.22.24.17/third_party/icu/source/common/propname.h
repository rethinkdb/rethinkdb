/*
**********************************************************************
* Copyright (c) 2002-2004, International Business Machines
* Corporation and others.  All Rights Reserved.
**********************************************************************
* Author: Alan Liu
* Created: October 30 2002
* Since: ICU 2.4
**********************************************************************
*/
#ifndef PROPNAME_H
#define PROPNAME_H

#include "unicode/utypes.h"
#include "unicode/uchar.h"
#include "udataswp.h"
#include "uprops.h"

/*
 * This header defines the in-memory layout of the property names data
 * structure representing the UCD data files PropertyAliases.txt and
 * PropertyValueAliases.txt.  It is used by:
 *   propname.cpp - reads data
 *   genpname     - creates data
 */

/* low-level char * property name comparison -------------------------------- */

U_CDECL_BEGIN

/**
 * \var uprv_comparePropertyNames
 * Unicode property names and property value names are compared "loosely".
 *
 * UCD.html 4.0.1 says:
 *   For all property names, property value names, and for property values for
 *   Enumerated, Binary, or Catalog properties, use the following
 *   loose matching rule:
 *
 *   LM3. Ignore case, whitespace, underscore ('_'), and hyphens.
 *
 * This function does just that, for (char *) name strings.
 * It is almost identical to ucnv_compareNames() but also ignores
 * C0 White_Space characters (U+0009..U+000d, and U+0085 on EBCDIC).
 *
 * @internal
 */

U_CAPI int32_t U_EXPORT2
uprv_compareASCIIPropertyNames(const char *name1, const char *name2);

U_CAPI int32_t U_EXPORT2
uprv_compareEBCDICPropertyNames(const char *name1, const char *name2);

#if U_CHARSET_FAMILY==U_ASCII_FAMILY
#   define uprv_comparePropertyNames uprv_compareASCIIPropertyNames
#elif U_CHARSET_FAMILY==U_EBCDIC_FAMILY
#   define uprv_comparePropertyNames uprv_compareEBCDICPropertyNames
#else
#   error U_CHARSET_FAMILY is not valid
#endif

U_CDECL_END

/* UDataMemory structure and signatures ------------------------------------- */

#define PNAME_DATA_NAME "pnames"
#define PNAME_DATA_TYPE "icu"

/* Fields in UDataInfo: */

/* PNAME_SIG[] is encoded as numeric literals for compatibility with the HP compiler */
#define PNAME_SIG_0 ((uint8_t)0x70) /* p */
#define PNAME_SIG_1 ((uint8_t)0x6E) /* n */
#define PNAME_SIG_2 ((uint8_t)0x61) /* a */
#define PNAME_SIG_3 ((uint8_t)0x6D) /* m */

#define PNAME_FORMAT_VERSION ((int8_t)1) /* formatVersion[0] */

/**
 * Swap pnames.icu. See udataswp.h.
 * @internal
 */
U_CAPI int32_t U_EXPORT2
upname_swap(const UDataSwapper *ds,
            const void *inData, int32_t length, void *outData,
            UErrorCode *pErrorCode);


#ifdef XP_CPLUSPLUS

class Builder;

U_NAMESPACE_BEGIN

/**
 * An offset from the start of the pnames data to a contained entity.
 * This must be a signed value, since negative offsets are used as an
 * end-of-list marker.  Offsets to actual objects are non-zero.  A
 * zero offset indicates an absent entry; this corresponds to aliases
 * marked "n/a" in the original Unicode data files.
 */
typedef int16_t Offset; /*  must be signed */

#define MAX_OFFSET 0x7FFF

/**
 * A generic value for a property or property value.  Typically an
 * enum from uchar.h, but sometimes a non-enum value.  It must be
 * large enough to accomodate the largest enum value, which as of this
 * writing is the largest general category mask.  Need not be signed
 * but may be.  Typically it doesn't matter, since the caller will
 * cast it to the proper type before use.  Takes the special value
 * UCHAR_INVALID_CODE for invalid input.
 */
typedef int32_t EnumValue;

/* ---------------------------------------------------------------------- */
/*  ValueMap */

/**
 * For any top-level property that has named values (binary and
 * enumerated properties), there is a ValueMap object.  This object
 * maps from enum values to two other maps.  One goes from value enums
 * to value names.  The other goes from value names to value enums.
 * 
 * The value enum values may be contiguous or disjoint.  If they are
 * contiguous then the enumToName_offset is nonzero, and the
 * ncEnumToName_offset is zero.  Vice versa if the value enums are
 * disjoint.
 *
 * There are n of these objects, where n is the number of binary
 * properties + the number of enumerated properties.
 */
struct ValueMap {

    /*  -- begin pnames data -- */
    /*  Enum=>name EnumToOffset / NonContiguousEnumToOffset objects. */
    /*  Exactly one of these will be nonzero. */
    Offset enumToName_offset;
    Offset ncEnumToName_offset;

    Offset nameToEnum_offset; /*  Name=>enum data */
    /*  -- end pnames data -- */
};

/* ---------------------------------------------------------------------- */
/*  PropertyAliases class */

/**
 * A class encapsulating access to the memory-mapped data representing
 * property aliases and property value aliases (pnames).  The class
 * MUST have no v-table and declares certain methods inline -- small
 * methods and methods that are called from only one point.
 *
 * The data members in this class correspond to the in-memory layout
 * of the header of the pnames data.
 */
class PropertyAliases {

    /*  -- begin pnames data -- */
    /*  Enum=>name EnumToOffset object for binary and enumerated */
    /*  properties */
    Offset enumToName_offset;

    /*  Name=>enum data for binary & enumerated properties */
    Offset nameToEnum_offset;

    /*  Enum=>offset EnumToOffset object mapping enumerated properties */
    /*  to ValueMap objects */
    Offset enumToValue_offset;

    /*  The following are needed by external readers of this data. */
    /*  We don't use them ourselves. */
    int16_t total_size; /*  size in bytes excluding the udata header */
    Offset valueMap_offset; /*  offset to start of array */
    int16_t valueMap_count; /*  number of entries */
    Offset nameGroupPool_offset; /*  offset to start of array */
    int16_t nameGroupPool_count; /*  number of entries (not groups) */
    Offset stringPool_offset; /*  offset to start of pool */
    int16_t stringPool_count; /*  number of strings (not size in bytes) */

    /*  -- end pnames data -- */

    friend class ::Builder;

    const ValueMap* getValueMap(EnumValue prop) const;

    const char* chooseNameInGroup(Offset offset,
                                  UPropertyNameChoice choice) const;

 public:

    inline const int8_t* getPointer(Offset o) const {
        return ((const int8_t*) this) + o;
    }

    inline const int8_t* getPointerNull(Offset o) const {
        return o ? getPointer(o) : NULL;
    }

    inline const char* getPropertyName(EnumValue prop,
                                       UPropertyNameChoice choice) const;
    
    inline EnumValue getPropertyEnum(const char* alias) const;

    inline const char* getPropertyValueName(EnumValue prop, EnumValue value,
                                            UPropertyNameChoice choice) const;
    
    inline EnumValue getPropertyValueEnum(EnumValue prop,
                                          const char* alias) const;

    static int32_t
    swap(const UDataSwapper *ds,
         const uint8_t *inBytes, int32_t length, uint8_t *outBytes,
         UErrorCode *pErrorCode);
};

/* ---------------------------------------------------------------------- */
/*  EnumToOffset */

/**
 * A generic map from enum values to Offsets.  The enum values must be
 * contiguous, from enumStart to enumLimit.  The Offset values may
 * point to anything.
 */
class EnumToOffset {

    /*  -- begin pnames data -- */
    EnumValue enumStart;
    EnumValue enumLimit;
    Offset _offsetArray; /*  [array of enumLimit-enumStart] */
    /*  -- end pnames data -- */

    friend class ::Builder;

    Offset* getOffsetArray() {
        return &_offsetArray;
    }

    const Offset* getOffsetArray() const {
        return &_offsetArray;
    }

    static int32_t getSize(int32_t n) {
        return sizeof(EnumToOffset) + sizeof(Offset) * (n - 1);
    }

    int32_t getSize() {
        return getSize(enumLimit - enumStart);
    }

 public:

    Offset getOffset(EnumValue enumProbe) const {
        if (enumProbe < enumStart ||
            enumProbe >= enumLimit) {
            return 0; /*  not found */
        }
        const Offset* p = getOffsetArray();
        return p[enumProbe - enumStart];
    }

    static int32_t
    swap(const UDataSwapper *ds,
         const uint8_t *inBytes, int32_t length, uint8_t *outBytes,
         uint8_t *temp, int32_t pos,
         UErrorCode *pErrorCode);
};

/* ---------------------------------------------------------------------- */
/*  NonContiguousEnumToOffset */

/**
 * A generic map from enum values to Offsets.  The enum values may be
 * disjoint.  If they are contiguous, an EnumToOffset should be used
 * instead.  The Offset values may point to anything.
 */
class NonContiguousEnumToOffset {

    /*  -- begin pnames data -- */
    int32_t count;
    EnumValue _enumArray; /*  [array of count] */
    /*  Offset _offsetArray; // [array of count] after enumValue[count-1] */
    /*  -- end pnames data -- */

    friend class ::Builder;

    EnumValue* getEnumArray() {
        return &_enumArray;
    }

    const EnumValue* getEnumArray() const {
        return &_enumArray;
    }
    
    Offset* getOffsetArray() {
        return (Offset*) (getEnumArray() + count);
    }

    const Offset* getOffsetArray() const {
        return (Offset*) (getEnumArray() + count);
    }

    static int32_t getSize(int32_t n) {
        return sizeof(int32_t) + (sizeof(EnumValue) + sizeof(Offset)) * n;
    }

    int32_t getSize() {
        return getSize(count);
    }

 public:

    Offset getOffset(EnumValue enumProbe) const {
        const EnumValue* e = getEnumArray();
        const Offset* p = getOffsetArray();
        /*  linear search; binary later if warranted */
        /*  (binary is not faster for short lists) */
        for (int32_t i=0; i<count; ++i) {
            if (e[i] < enumProbe) continue;
            if (e[i] > enumProbe) break;
            return p[i];
        }
        return 0; /*  not found */
    }

    static int32_t
    swap(const UDataSwapper *ds,
         const uint8_t *inBytes, int32_t length, uint8_t *outBytes,
         uint8_t *temp, int32_t pos,
         UErrorCode *pErrorCode);
};

/* ---------------------------------------------------------------------- */
/*  NameToEnum */

/**
 * A map from names to enum values.
 */
class NameToEnum {

    /*  -- begin pnames data -- */
    int32_t count;       /*  number of entries */
    EnumValue _enumArray; /*  [array of count] EnumValues */
    /*  Offset _nameArray; // [array of count] offsets to names */
    /*  -- end pnames data -- */

    friend class ::Builder;

    EnumValue* getEnumArray() {
        return &_enumArray;
    }

    const EnumValue* getEnumArray() const {
        return &_enumArray;
    }

    Offset* getNameArray() {
        return (Offset*) (getEnumArray() + count);
    }

    const Offset* getNameArray() const {
        return (Offset*) (getEnumArray() + count);
    }

    static int32_t getSize(int32_t n) {
        return sizeof(int32_t) + (sizeof(Offset) + sizeof(EnumValue)) * n;
    }

    int32_t getSize() {
        return getSize(count);
    }

 public:
  
    EnumValue getEnum(const char* alias, const PropertyAliases& data) const {

        const Offset* n = getNameArray();
        const EnumValue* e = getEnumArray();

        /*  linear search; binary later if warranted */
        /*  (binary is not faster for short lists) */
        for (int32_t i=0; i<count; ++i) {
            const char* name = (const char*) data.getPointer(n[i]);
            int32_t c = uprv_comparePropertyNames(alias, name);
            if (c > 0) continue;
            if (c < 0) break;
            return e[i];
        }
        
        return UCHAR_INVALID_CODE;
    }

    static int32_t
    swap(const UDataSwapper *ds,
         const uint8_t *inBytes, int32_t length, uint8_t *outBytes,
         uint8_t *temp, int32_t pos,
         UErrorCode *pErrorCode);
};

/*----------------------------------------------------------------------
 * 
 * In-memory layout.  THIS IS NOT A STANDALONE DOCUMENT.  It goes
 * together with above C++ declarations and gives an overview.
 *
 * See above for definitions of Offset and EnumValue.  Also, refer to
 * above class declarations for the "bottom line" on data layout.
 *
 * Sizes:
 * '*_offset' is an Offset (see above)
 * 'count' members are typically int32_t (see above declarations)
 * 'enumArray' is an array of EnumValue (see above)
 * 'offsetArray' is an array of Offset (see above)
 * 'nameArray' is an array of Offset (see above)
 * 'enum*' is an EnumValue (see above)
 * '*Array [x n]' means that *Array has n elements
 *
 * References:
 * Instead of pointers, this flat data structure contains offsets.
 * All offsets are relative to the start of 'header'.  A notation
 * is used to indicate what structure each offset points to:
 * 'foo (>x)' the offset(s) in foo point to structure x
 * 
 * Structures:
 * Each structure is assigned a number, except for the header,
 * which is called 'header'.  The numbers are not contiguous
 * for historical reasons.  Some structures have sub-parts
 * that are denoted with a letter, e.g., "5a".
 * 
 * BEGIN LAYOUT
 * ============
 * header:
 *  enumToName_offset (>0)
 *  nameToEnum_offset (>2)
 *  enumToValue_offset (>3)
 *  (alignment padding build in to header)
 *
 * The header also contains the following, used by "external readers"
 * like ICU4J and icuswap.
 *
 *  // The following are needed by external readers of this data.
 *  // We don't use them ourselves.
 *  int16_t total_size; // size in bytes excluding the udata header
 *  Offset valueMap_offset; // offset to start of array
 *  int16_t valueMap_count; // number of entries
 *  Offset nameGroupPool_offset; // offset to start of array
 *  int16_t nameGroupPool_count; // number of entries (not groups)
 *  Offset stringPool_offset; // offset to start of pool
 *  int16_t stringPool_count; // number of strings (not size in bytes)
 *
 * 0: # NonContiguousEnumToOffset obj for props => name groups
 *  count
 *  enumArray [x count]
 *  offsetArray [x count] (>98)
 * 
 * => pad to next 4-byte boundary
 * 
 * (1: omitted -- no longer used)
 * 
 * 2: # NameToEnum obj for binary & enumerated props
 *  count
 *  enumArray [x count]
 *  nameArray [x count] (>99)
 * 
 * => pad to next 4-byte boundary
 * 
 * 3: # NonContiguousEnumToOffset obj for enumerated props => ValueMaps
 *  count
 *  enumArray [x count]
 *  offsetArray [x count] (>4)
 * 
 * => pad to next 4-byte boundary
 * 
 * 4: # ValueMap array [x one for each enumerated prop i]
 *  enumToName_offset (>5a +2*i)   one of these two is NULL, one is not
 *  ncEnumToName_offset (>5b +2*i)
 *  nameToEnums_offset (>6 +2*i)
 * 
 * => pad to next 4-byte boundary
 * 
 * for each enumerated prop (either 5a or 5b):
 * 
 *   5a: # EnumToOffset for enumerated prop's values => name groups
 *    enumStart
 *    enumLimit
 *    offsetArray [x enumLimit - enumStart] (>98) 
 * 
 *   => pad to next 4-byte boundary
 * 
 *   5b: # NonContiguousEnumToOffset for enumerated prop's values => name groups
 *    count
 *    enumArray [x count]
 *    offsetArray [x count] (>98)
 * 
 *   => pad to next 4-byte boundary
 * 
 *   6: # NameToEnum for enumerated prop's values
 *    count
 *    enumArray [x count]
 *    nameArray [x count] (>99)
 * 
 *   => pad to next 4-byte boundary
 * 
 * 98: # name group pool {NGP}
 *  [array of Offset values] (>99)
 * 
 * 99: # string pool {SP}
 *  [pool of nul-terminated char* strings]
 */
U_NAMESPACE_END

#endif /* C++ */

#endif
