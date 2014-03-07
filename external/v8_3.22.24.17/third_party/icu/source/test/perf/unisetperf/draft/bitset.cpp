/*  
**********************************************************************
*   Copyright (C) 2007, International Business Machines
*   Corporation and others.  All Rights Reserved.
**********************************************************************
*   file name:  bitset.cpp
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2007jan15
*   created by: Markus Scherer
*
*   Idea for a "compiled", fast, read-only (immutable) version of a UnicodeSet
*   using a folded bit set consisting of a 1k-entry index table and a
*   compacted array of 64-bit words.
*   Uses a simple hash table for compaction.
*   Uses the original set for supplementary code points.
*/

#include "unicode/utypes.h"
#include "unicont.h"

/*
 * Hash table for up to 1k 64-bit words, for 1 bit per BMP code point.
 * Hashes 64-bit words and maps them to 16-bit integers which are
 * assigned in order of new incoming words for subsequent storage
 * in a contiguous array.
 */
struct BMPBitHash : public UObject {
    int64_t keys[0x800];  // 2k
    uint16_t values[0x800];
    uint16_t reverse[0x400];
    uint16_t count;
    const int32_t prime=1301;  // Less than 2k.

    BMPBitHash() : count(0) {
        // Fill values[] with 0xffff.
        uprv_memset(values, 0xff, sizeof(values));
    }

    /*
     * Map a key to an integer count.
     * Map at most 1k=0x400 different keys with this data structure.
     */
    uint16_t map(int64_t key) {
        int32_t hash=(int32_t)(key>>55)&0x1ff;
        hash^=(int32_t)(key>>44)&0x7ff;
        hash^=(int32_t)(key>>33)&0x7ff;
        hash^=(int32_t)(key>>22)&0x7ff;
        hash^=(int32_t)(key>>11)&0x7ff;
        hash^=(int32_t)key&0x7ff;
        for(;;) {
            if(values[hash]==0xffff) {
                // Unused slot.
                keys[hash]=key;
                reverse[count]=hash;
                return values[hash]=count++;
            } else if(keys[hash]==key) {
                // Found a slot with this key.
                return values[hash];
            } else {
                // Used slot with a different key, move to another slot.
                hash=(hash+prime)&0x7ff;
            }
        }
    }

    uint16_t countKeys() const { return count; }

    /*
     * Invert the hash map: Fill an array of length countKeys() with the keys
     * indexed by their mapped values.
     */
    void invert(int64_t *k) const {
        uint16_t i;

        for(i=0; i<count; ++i) {
            k[i]=keys[reverse[i]];
        }
    }
};

class BitSet : public UObject, public UnicodeContainable {
public:
    BitSet(const UnicodeSet &set, UErrorCode &errorCode) : bits(shortBits), restSet(set.clone()) {
        if(U_FAILURE(errorCode)) {
            return;
        }
        BMPBitHash *bitHash=new BMPBitHash;
        if(bitHash==NULL || restSet==NULL) {
            errorCode=U_MEMORY_ALLOCATION_ERROR;
            return;
        }

        UnicodeSetIterator iter(set);
        int64_t b;
        UChar32 start, end;
        int32_t prevIndex, i, j;

        b=0;  // Not necessary but makes compilers happy.
        prevIndex=-1;
        for(;;) {
            if(iter.nextRange() && !iter.isString()) {
                start=iter.getCodepoint();
                end=iter.getCodepointEnd();
            } else {
                start=0x10000;
            }
            i=start>>6;
            if(prevIndex!=i) {
                // Finish the end of the previous range.
                if(prevIndex<0) {
                    prevIndex=0;
                } else {
                    index[prevIndex++]=bitHash->map(b);
                }
                // Fill all-zero entries between ranges.
                if(prevIndex<i) {
                    uint16_t zero=bitHash->map(0);
                    do {
                        index[prevIndex++]=zero;
                    } while(prevIndex<i);
                }
                b=0;
            }
            if(start>0xffff) {
                break;
            }
            b|=~((INT64_C(1)<<(start&0x3f))-1);
            j=end>>6;
            if(i<j) {
                // Set bits for the start of the range.
                index[i++]=bitHash->map(b);
                // Fill all-one entries inside the range.
                if(i<j) {
                    uint16_t all=bitHash->map(INT64_C(0xffffffffffffffff));
                    do {
                        index[i++]=all;
                    } while(i<j);
                }
                b=INT64_C(0xffffffffffffffff);
            }
            /* i==j */
            b&=(INT64_C(1)<<(end&0x3f))-1;
            prevIndex=j;
        }

        if(bitHash->countKeys()>LENGTHOF(shortBits)) {
            bits=(int64_t *)uprv_malloc(bitHash->countKeys()*8);
        }
        if(bits!=NULL) {
            bitHash->invert(bits);
        } else {
            bits=shortBits;
            errorCode=U_MEMORY_ALLOCATION_ERROR;
            return;
        }

        latin1Set[0]=(uint32_t)bits[0];
        latin1Set[1]=(uint32_t)(bits[0]>>32);
        latin1Set[2]=(uint32_t)bits[1];
        latin1Set[3]=(uint32_t)(bits[1]>>32);
        latin1Set[4]=(uint32_t)bits[2];
        latin1Set[5]=(uint32_t)(bits[2]>>32);
        latin1Set[6]=(uint32_t)bits[3];
        latin1Set[7]=(uint32_t)(bits[3]>>32);

        restSet.remove(0, 0xffff);
    }

    ~BitSet() {
        if(bits!=shortBits) {
            uprv_free(bits);
        }
        delete restSet;
    }

    UBool contains(UChar32 c) const {
        if((uint32_t)c<=0xff) {
            return (UBool)((latin1Set[c>>5]&((uint32_t)1<<(c&0x1f)))!=0);
        } else if((uint32_t)c<0xffff) {
            return (UBool)((bits[c>>6]&(INT64_C(1)<<(c&0x3f)))!=0);
        } else {
            return restSet->contains(c);
        }
    }

private:
    uint16_t index[0x400];
    int64_t shortBits[32];
    int64_t *bits;

    uint32_t latin1Bits[8];

    UnicodeSet *restSet;
};
