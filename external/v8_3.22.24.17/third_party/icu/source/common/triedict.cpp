/**
 *******************************************************************************
 * Copyright (C) 2006-2008, International Business Machines Corporation        *
 * and others. All Rights Reserved.                                            *
 *******************************************************************************
 */

#include "unicode/utypes.h"

#if !UCONFIG_NO_BREAK_ITERATION

#include "triedict.h"
#include "unicode/chariter.h"
#include "unicode/uchriter.h"
#include "unicode/strenum.h"
#include "unicode/uenum.h"
#include "unicode/udata.h"
#include "cmemory.h"
#include "udataswp.h"
#include "uvector.h"
#include "uvectr32.h"
#include "uarrsort.h"
#include "hash.h"

//#define DEBUG_TRIE_DICT 1

#ifdef DEBUG_TRIE_DICT
#include <sys/times.h>
#include <limits.h>
#include <stdio.h>
#include <time.h>
#ifndef CLK_TCK
#define CLK_TCK      CLOCKS_PER_SEC
#endif

#endif

U_NAMESPACE_BEGIN

/*******************************************************************
 * TrieWordDictionary
 */

TrieWordDictionary::TrieWordDictionary() {
}

TrieWordDictionary::~TrieWordDictionary() {
}

/*******************************************************************
 * MutableTrieDictionary
 */

//#define MAX_VALUE 65535

// forward declaration
inline uint16_t scaleLogProbabilities(double logprob);

// Node structure for the ternary, uncompressed trie
struct TernaryNode : public UMemory {
    UChar       ch;         // UTF-16 code unit
    uint16_t    flags;      // Flag word
    TernaryNode *low;       // Less-than link
    TernaryNode *equal;     // Equal link
    TernaryNode *high;      // Greater-than link

    TernaryNode(UChar uc);
    ~TernaryNode();
};

enum MutableTrieNodeFlags {
    kEndsWord = 0x0001      // This node marks the end of a valid word
};

inline
TernaryNode::TernaryNode(UChar uc) {
    ch = uc;
    flags = 0;
    low = NULL;
    equal = NULL;
    high = NULL;
}

// Not inline since it's recursive
TernaryNode::~TernaryNode() {
    delete low;
    delete equal;
    delete high;
}

MutableTrieDictionary::MutableTrieDictionary( UChar median, UErrorCode &status,
                                              UBool containsValue /* = FALSE */  ) {
    // Start the trie off with something. Having the root node already present
    // cuts a special case out of the search/insertion functions.
    // Making it a median character cuts the worse case for searches from
    // 4x a balanced trie to 2x a balanced trie. It's best to choose something
    // that starts a word that is midway in the list.
    fTrie = new TernaryNode(median);
    if (fTrie == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
    }
    fIter = utext_openUChars(NULL, NULL, 0, &status);
    if (U_SUCCESS(status) && fIter == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
    }

    fValued = containsValue;
}

MutableTrieDictionary::MutableTrieDictionary( UErrorCode &status, 
                                              UBool containsValue /* = false */ ) {
    fTrie = NULL;
    fIter = utext_openUChars(NULL, NULL, 0, &status);
    if (U_SUCCESS(status) && fIter == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
    }

    fValued = containsValue;
}

MutableTrieDictionary::~MutableTrieDictionary() {
    delete fTrie;
    utext_close(fIter);
}

int32_t
MutableTrieDictionary::search( UText *text,
                               int32_t maxLength,
                               int32_t *lengths,
                               int &count,
                               int limit,
                               TernaryNode *&parent,
                               UBool &pMatched,
                               uint16_t *values /*=NULL*/) const {
    // TODO: current implementation works in UTF-16 space
    const TernaryNode *up = NULL;
    const TernaryNode *p = fTrie;
    int mycount = 0;
    pMatched = TRUE;
    int i;

    if (!fValued) {
        values = NULL;
    }

    UChar uc = utext_current32(text);
    for (i = 0; i < maxLength && p != NULL; ++i) {
        while (p != NULL) {
            if (uc < p->ch) {
                up = p;
                p = p->low;
            }
            else if (uc == p->ch) {
                break;
            }
            else {
                up = p;
                p = p->high;
            }
        }
        if (p == NULL) {
            pMatched = FALSE;
            break;
        }
        // Must be equal to get here
        if (limit > 0 && (p->flags > 0)) {
            //is there a more efficient way to add values? ie. remove if stmt
            if(values != NULL) {
                values[mycount] = p->flags;
            }
            lengths[mycount++] = i+1;
            --limit;
        }
        up = p;
        p = p->equal;
        uc = utext_next32(text);
        uc = utext_current32(text);
    }
    
    // Note that there is no way to reach here with up == 0 unless
    // maxLength is 0 coming in.
    parent = (TernaryNode *)up;
    count = mycount;
    return i;
}

void
MutableTrieDictionary::addWord( const UChar *word,
                                int32_t length,
                                UErrorCode &status,
                                uint16_t value /* = 0 */ ) {
    // dictionary cannot store zero values, would interfere with flags
    if (length <= 0 || (!fValued && value > 0) || (fValued && value == 0)) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }

    TernaryNode *parent;
    UBool pMatched;
    int count;
    fIter = utext_openUChars(fIter, word, length, &status);
    
    int matched;
    matched = search(fIter, length, NULL, count, 0, parent, pMatched);
    
    while (matched++ < length) {
        UChar32 uc = utext_next32(fIter);  // TODO:  supplementary support?
        U_ASSERT(uc != U_SENTINEL);
        TernaryNode *newNode = new TernaryNode(uc);
        if (newNode == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return;
        }
        if (pMatched) {
            parent->equal = newNode;
        }
        else {
            pMatched = TRUE;
            if (uc < parent->ch) {
                parent->low = newNode;
            }
            else {
                parent->high = newNode;
            }
        }
        parent = newNode;
    }

    if(fValued && value > 0){
        parent->flags = value;
    } else {
        parent->flags |= kEndsWord;
    }
}

int32_t
MutableTrieDictionary::matches( UText *text,
                                int32_t maxLength,
                                int32_t *lengths,
                                int &count,
                                int limit,
                                uint16_t *values /*=NULL*/) const {
    TernaryNode *parent;
    UBool pMatched;
    return search(text, maxLength, lengths, count, limit, parent, pMatched, values);
}

// Implementation of iteration for MutableTrieDictionary
class MutableTrieEnumeration : public StringEnumeration {
private:
    UStack      fNodeStack;     // Stack of nodes to process
    UVector32   fBranchStack;   // Stack of which branch we are working on
    TernaryNode *fRoot;         // Root node
    enum StackBranch {
        kLessThan,
        kEqual,
        kGreaterThan,
        kDone
    };

public:
    static UClassID U_EXPORT2 getStaticClassID(void);
    virtual UClassID getDynamicClassID(void) const;
public:
    MutableTrieEnumeration(TernaryNode *root, UErrorCode &status) 
        : fNodeStack(status), fBranchStack(status) {
        fRoot = root;
        fNodeStack.push(root, status);
        fBranchStack.push(kLessThan, status);
        unistr.remove();
    }
    
    virtual ~MutableTrieEnumeration() {
    }
    
    virtual StringEnumeration *clone() const {
        UErrorCode status = U_ZERO_ERROR;
        return new MutableTrieEnumeration(fRoot, status);
    }
    
    virtual const UnicodeString *snext(UErrorCode &status) {
        if (fNodeStack.empty() || U_FAILURE(status)) {
            return NULL;
        }
        TernaryNode *node = (TernaryNode *) fNodeStack.peek();
        StackBranch where = (StackBranch) fBranchStack.peeki();
        while (!fNodeStack.empty() && U_SUCCESS(status)) {
            UBool emit;
            UBool equal;

            switch (where) {
            case kLessThan:
                if (node->low != NULL) {
                    fBranchStack.setElementAt(kEqual, fBranchStack.size()-1);
                    node = (TernaryNode *) fNodeStack.push(node->low, status);
                    where = (StackBranch) fBranchStack.push(kLessThan, status);
                    break;
                }
            case kEqual:
                emit = node->flags > 0;
                equal = (node->equal != NULL);
                // If this node should be part of the next emitted string, append
                // the UChar to the string, and make sure we pop it when we come
                // back to this node. The character should only be in the string
                // for as long as we're traversing the equal subtree of this node
                if (equal || emit) {
                    unistr.append(node->ch);
                    fBranchStack.setElementAt(kGreaterThan, fBranchStack.size()-1);
                }
                if (equal) {
                    node = (TernaryNode *) fNodeStack.push(node->equal, status);
                    where = (StackBranch) fBranchStack.push(kLessThan, status);
                }
                if (emit) {
                    return &unistr;
                }
                if (equal) {
                    break;
                }
            case kGreaterThan:
                // If this node's character is in the string, remove it.
                if (node->equal != NULL || node->flags > 0) {
                    unistr.truncate(unistr.length()-1);
                }
                if (node->high != NULL) {
                    fBranchStack.setElementAt(kDone, fBranchStack.size()-1);
                    node = (TernaryNode *) fNodeStack.push(node->high, status);
                    where = (StackBranch) fBranchStack.push(kLessThan, status);
                    break;
                }
            case kDone:
                fNodeStack.pop();
                fBranchStack.popi();
                node = (TernaryNode *) fNodeStack.peek();
                where = (StackBranch) fBranchStack.peeki();
                break;
            default:
                return NULL;
            }
        }
        return NULL;
    }
    
    // Very expensive, but this should never be used.
    virtual int32_t count(UErrorCode &status) const {
        MutableTrieEnumeration counter(fRoot, status);
        int32_t result = 0;
        while (counter.snext(status) != NULL && U_SUCCESS(status)) {
            ++result;
        }
        return result;
    }
    
    virtual void reset(UErrorCode &status) {
        fNodeStack.removeAllElements();
        fBranchStack.removeAllElements();
        fNodeStack.push(fRoot, status);
        fBranchStack.push(kLessThan, status);
        unistr.remove();
    }
};

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(MutableTrieEnumeration)

StringEnumeration *
MutableTrieDictionary::openWords( UErrorCode &status ) const {
    if (U_FAILURE(status)) {
        return NULL;
    }
    return new MutableTrieEnumeration(fTrie, status);
}

/*******************************************************************
 * CompactTrieDictionary
 */

//TODO further optimization:
// minimise size of trie with logprobs by storing values
// for terminal nodes directly in offsets[]
// --> calculating from next offset *might* be simpler, but would have to add
// one last offset for logprob of last node
// --> if calculate from current offset, need to factor in possible overflow
// as well.
// idea: store in offset, set first bit to indicate logprob storage-->won't
// have to access additional node

// {'Dic', 1}, version 1: uses old header, no values
#define COMPACT_TRIE_MAGIC_1 0x44696301
// version 2: uses new header (more than 2^16 nodes), no values
#define COMPACT_TRIE_MAGIC_2 0x44696302
// version 3: uses new header, includes values
#define COMPACT_TRIE_MAGIC_3 0x44696303

struct CompactTrieHeader {
    uint32_t        size;           // Size of the data in bytes
    uint32_t        magic;          // Magic number (including version)
    uint32_t        nodeCount;      // Number of entries in offsets[]
    uint32_t        root;           // Node number of the root node
    uint32_t        offsets[1];     // Offsets to nodes from start of data
};

// old version of CompactTrieHeader kept for backwards compatibility
struct CompactTrieHeaderV1 {
    uint32_t        size;           // Size of the data in bytes
    uint32_t        magic;          // Magic number (including version)
    uint16_t        nodeCount;      // Number of entries in offsets[]
    uint16_t        root;           // Node number of the root node
    uint32_t        offsets[1];     // Offsets to nodes from start of data
};

// Helper class for managing CompactTrieHeader and CompactTrieHeaderV1
struct CompactTrieInfo {
    uint32_t        size;           // Size of the data in bytes
    uint32_t        magic;          // Magic number (including version)
    uint32_t        nodeCount;      // Number of entries in offsets[]
    uint32_t        root;           // Node number of the root node
    uint32_t        *offsets;       // Offsets to nodes from start of data
    uint8_t         *address;       // pointer to header bytes in memory

    CompactTrieInfo(const void *data, UErrorCode &status){
        CompactTrieHeader *header = (CompactTrieHeader *) data;
        if (header->magic != COMPACT_TRIE_MAGIC_1 && 
                header->magic != COMPACT_TRIE_MAGIC_2 &&
                header->magic != COMPACT_TRIE_MAGIC_3) {
            status = U_ILLEGAL_ARGUMENT_ERROR;
        } else {
            size = header->size;
            magic = header->magic;

            if (header->magic == COMPACT_TRIE_MAGIC_1) {
                CompactTrieHeaderV1 *headerV1 = (CompactTrieHeaderV1 *) header;
                nodeCount = headerV1->nodeCount;
                root = headerV1->root;
                offsets = &(headerV1->offsets[0]);
                address = (uint8_t *)headerV1;
            } else {
                nodeCount = header->nodeCount;
                root = header->root;
                offsets = &(header->offsets[0]);
                address = (uint8_t *)header;
            }
        }
    }

    ~CompactTrieInfo(){}
};

// Note that to avoid platform-specific alignment issues, all members of the node
// structures should be the same size, or should contain explicit padding to
// natural alignment boundaries.

// We can't use a bitfield for the flags+count field, because the layout of those
// is not portable. 12 bits of count allows for up to 4096 entries in a node.
struct CompactTrieNode {
    uint16_t        flagscount;     // Count of sub-entries, plus flags
};

enum CompactTrieNodeFlags {
    kVerticalNode   = 0x1000,       // This is a vertical node
    kParentEndsWord = 0x2000,       // The node whose equal link points to this ends a word
    kExceedsCount   = 0x4000,       // new MSB for count >= 4096, originally kReservedFlag1
    kEqualOverflows = 0x8000,       // Links to nodeIDs > 2^16, orig. kReservedFlag2
    kCountMask      = 0x0FFF,       // The count portion of flagscount
    kFlagMask       = 0xF000,       // The flags portion of flagscount
    kRootCountMask  = 0x7FFF        // The count portion of flagscount in the root node

    //offset flags:
    //kOffsetContainsValue = 0x80000000       // Offset contains value for parent node
};

// The two node types are distinguished by the kVerticalNode flag.

struct CompactTrieHorizontalEntry {
    uint16_t        ch;             // UChar
    uint16_t        equal;          // Equal link node index
};

// We don't use inheritance here because C++ does not guarantee that the
// base class comes first in memory!!

struct CompactTrieHorizontalNode {
    uint16_t        flagscount;     // Count of sub-entries, plus flags
    CompactTrieHorizontalEntry      entries[1];
};

struct CompactTrieVerticalNode {
    uint16_t        flagscount;     // Count of sub-entries, plus flags
    uint16_t        equal;          // Equal link node index
    uint16_t        chars[1];       // Code units
};

CompactTrieDictionary::CompactTrieDictionary(UDataMemory *dataObj,
                                                UErrorCode &status )
: fUData(dataObj)
{
    fInfo = (CompactTrieInfo *)uprv_malloc(sizeof(CompactTrieInfo));
    *fInfo = CompactTrieInfo(udata_getMemory(dataObj), status);
    fOwnData = FALSE;
}

CompactTrieDictionary::CompactTrieDictionary( const void *data,
                                                UErrorCode &status )
: fUData(NULL)
{
    fInfo = (CompactTrieInfo *)uprv_malloc(sizeof(CompactTrieInfo));
    *fInfo = CompactTrieInfo(data, status);
    fOwnData = FALSE;
}

CompactTrieDictionary::CompactTrieDictionary( const MutableTrieDictionary &dict,
                                                UErrorCode &status )
: fUData(NULL)
{
    const CompactTrieHeader* header = compactMutableTrieDictionary(dict, status);
    if (U_SUCCESS(status)) {
        fInfo = (CompactTrieInfo *)uprv_malloc(sizeof(CompactTrieInfo));
        *fInfo = CompactTrieInfo(header, status);
    }

    fOwnData = !U_FAILURE(status);
}

CompactTrieDictionary::~CompactTrieDictionary() {
    if (fOwnData) {
        uprv_free((void *)(fInfo->address));
    }
    uprv_free((void *)fInfo);

    if (fUData) {
        udata_close(fUData);
    }
}

UBool CompactTrieDictionary::getValued() const{
    return fInfo->magic == COMPACT_TRIE_MAGIC_3;
}

uint32_t
CompactTrieDictionary::dataSize() const {
    return fInfo->size;
}

const void *
CompactTrieDictionary::data() const {
    return fInfo->address;
}

//This function finds the address of a node for us, given its node ID
static inline const CompactTrieNode *
getCompactNode(const CompactTrieInfo *info, uint32_t node) {
    if(node < info->root-1) {
        return (const CompactTrieNode *)(&info->offsets[node]);
    } else {
        return (const CompactTrieNode *)(info->address + info->offsets[node]);
    }
}

//this version of getCompactNode is currently only used in compactMutableTrieDictionary()
static inline const CompactTrieNode *
getCompactNode(const CompactTrieHeader *header, uint32_t node) {
    if(node < header->root-1) {
        return (const CompactTrieNode *)(&header->offsets[node]);
    } else {
        return (const CompactTrieNode *)((const uint8_t *)header + header->offsets[node]);
    }
}


/**
 * Calculates the number of links in a node
 * @node The specified node
 */
static inline const uint16_t
getCount(const CompactTrieNode *node){
    return (node->flagscount & kCountMask);
    //use the code below if number of links ever exceed 4096
    //return (node->flagscount & kCountMask) + ((node->flagscount & kExceedsCount) >> 2);
}

/**
 * calculates an equal link node ID of a horizontal node
 * @hnode The horizontal node containing the equal link
 * @param index The index into hnode->entries[]
 * @param nodeCount The length of hnode->entries[]
 */
static inline uint32_t calcEqualLink(const CompactTrieVerticalNode *vnode){
    if(vnode->flagscount & kEqualOverflows){
        // treat overflow bits as an extension of chars[]
        uint16_t *overflow = (uint16_t *) &vnode->chars[getCount((CompactTrieNode*)vnode)];
        return vnode->equal + (((uint32_t)*overflow) << 16);
    }else{
        return vnode->equal;
    }
}

/**
 * calculates an equal link node ID of a horizontal node
 * @hnode The horizontal node containing the equal link
 * @param index The index into hnode->entries[]
 * @param nodeCount The length of hnode->entries[]
 */
static inline uint32_t calcEqualLink(const CompactTrieHorizontalNode *hnode, uint16_t index, uint16_t nodeCount){
    if(hnode->flagscount & kEqualOverflows){
        //set overflow to point to the uint16_t containing the overflow bits 
        uint16_t *overflow = (uint16_t *) &hnode->entries[nodeCount];
        overflow += index/4;
        uint16_t extraBits = (*overflow >> (3 - (index % 4)) * 4) % 0x10;
        return hnode->entries[index].equal + (((uint32_t)extraBits) << 16);
    } else {
        return hnode->entries[index].equal;
    }
}

/**
 * Returns the value stored in the specified node which is associated with its
 * parent node.
 * TODO: how to tell that value is stored in node or in offset? check whether
 * node ID < fInfo->root!
 */
static inline uint16_t getValue(const CompactTrieHorizontalNode *hnode){
    uint16_t count = getCount((CompactTrieNode *)hnode);
    uint16_t overflowSize = 0; //size of node ID overflow storage in bytes

    if(hnode->flagscount & kEqualOverflows)
        overflowSize = (count + 3) / 4 * sizeof(uint16_t);
    return *((uint16_t *)((uint8_t *)&hnode->entries[count] + overflowSize)); 
}

static inline uint16_t getValue(const CompactTrieVerticalNode *vnode){
    // calculate size of total node ID overflow storage in bytes
    uint16_t overflowSize = (vnode->flagscount & kEqualOverflows)? sizeof(uint16_t) : 0;
    return *((uint16_t *)((uint8_t *)&vnode->chars[getCount((CompactTrieNode *)vnode)] + overflowSize)); 
}

static inline uint16_t getValue(const CompactTrieNode *node){
    if(node->flagscount & kVerticalNode)
        return getValue((const CompactTrieVerticalNode *)node);
    else
        return getValue((const CompactTrieHorizontalNode *)node);
}

//returns index of match in CompactTrieHorizontalNode.entries[] using binary search
inline int16_t 
searchHorizontalEntries(const CompactTrieHorizontalEntry *entries, 
        UChar uc, uint16_t nodeCount){
    int low = 0;
    int high = nodeCount-1;
    int middle;
    while (high >= low) {
        middle = (high+low)/2;
        if (uc == entries[middle].ch) {
            return middle;
        }
        else if (uc < entries[middle].ch) {
            high = middle-1;
        }
        else {
            low = middle+1;
        }
    }

    return -1;
}

int32_t
CompactTrieDictionary::matches( UText *text,
                                int32_t maxLength,
                                int32_t *lengths,
                                int &count,
                                int limit,
                                uint16_t *values /*= NULL*/) const {
    if (fInfo->magic == COMPACT_TRIE_MAGIC_2)
        values = NULL;

    // TODO: current implementation works in UTF-16 space
    const CompactTrieNode *node = getCompactNode(fInfo, fInfo->root);
    int mycount = 0;

    UChar uc = utext_current32(text);
    int i = 0;

    // handle root node with only kEqualOverflows flag: assume horizontal node without parent
    if(node != NULL){
        const CompactTrieHorizontalNode *root = (const CompactTrieHorizontalNode *) node;
        int index = searchHorizontalEntries(root->entries, uc, root->flagscount & kRootCountMask);
        if(index > -1){
            node = getCompactNode(fInfo, calcEqualLink(root, index, root->flagscount & kRootCountMask));
            utext_next32(text);
            uc = utext_current32(text);
            ++i;
        }else{
            node = NULL;
        }
    }

    while (node != NULL) {
        // Check if the node we just exited ends a word
        if (limit > 0 && (node->flagscount & kParentEndsWord)) {
            if(values != NULL){
                values[mycount] = getValue(node);
            }
            lengths[mycount++] = i;
            --limit;
        }
        // Check that we haven't exceeded the maximum number of input characters.
        // We have to do that here rather than in the while condition so that
        // we can check for ending a word, above.
        if (i >= maxLength) {
            break;
        }

        int nodeCount = getCount(node);
        if (nodeCount == 0) {
            // Special terminal node; return now
            break;
        }
        if (node->flagscount & kVerticalNode) {
            // Vertical node; check all the characters in it
            const CompactTrieVerticalNode *vnode = (const CompactTrieVerticalNode *)node;
            for (int j = 0; j < nodeCount && i < maxLength; ++j) {
                if (uc != vnode->chars[j]) {
                    // We hit a non-equal character; return
                    goto exit;
                }
                utext_next32(text);
                uc = utext_current32(text);
                ++i;
            }
            // To get here we must have come through the whole list successfully;
            // go on to the next node. Note that a word cannot end in the middle
            // of a vertical node.
            node = getCompactNode(fInfo, calcEqualLink(vnode));
        }
        else {
            // Horizontal node; do binary search
            const CompactTrieHorizontalNode *hnode = (const CompactTrieHorizontalNode *)node;
            const CompactTrieHorizontalEntry *entries;
            entries = hnode->entries;

            int index = searchHorizontalEntries(entries, uc, nodeCount);
            if(index > -1){  //
                // We hit a match; get the next node and next character
                node = getCompactNode(fInfo, calcEqualLink(hnode, index, nodeCount));
                utext_next32(text);
                uc = utext_current32(text);
                ++i;
            }else{
                node = NULL;    // If we don't find a match, we'll fall out of the loop              
            }
        }
    }
    exit:
    count = mycount;
    return i;
}

// Implementation of iteration for CompactTrieDictionary
class CompactTrieEnumeration : public StringEnumeration {
private:
    UVector32               fNodeStack;     // Stack of nodes to process
    UVector32               fIndexStack;    // Stack of where in node we are
    const CompactTrieInfo   *fInfo;         // Trie data

public:
    static UClassID U_EXPORT2 getStaticClassID(void);
    virtual UClassID getDynamicClassID(void) const;
public:
    CompactTrieEnumeration(const CompactTrieInfo *info, UErrorCode &status) 
        : fNodeStack(status), fIndexStack(status) {
        fInfo = info;
        fNodeStack.push(info->root, status);
        fIndexStack.push(0, status);
        unistr.remove();
    }
    
    virtual ~CompactTrieEnumeration() {
    }
    
    virtual StringEnumeration *clone() const {
        UErrorCode status = U_ZERO_ERROR;
        return new CompactTrieEnumeration(fInfo, status);
    }
    
    virtual const UnicodeString * snext(UErrorCode &status);

    // Very expensive, but this should never be used.
    virtual int32_t count(UErrorCode &status) const {
        CompactTrieEnumeration counter(fInfo, status);
        int32_t result = 0;
        while (counter.snext(status) != NULL && U_SUCCESS(status)) {
            ++result;
        }
        return result;
    }
    
    virtual void reset(UErrorCode &status) {
        fNodeStack.removeAllElements();
        fIndexStack.removeAllElements();
        fNodeStack.push(fInfo->root, status);
        fIndexStack.push(0, status);
        unistr.remove();
    }
};

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(CompactTrieEnumeration)

const UnicodeString *
CompactTrieEnumeration::snext(UErrorCode &status) {
    if (fNodeStack.empty() || U_FAILURE(status)) {
        return NULL;
    }
    const CompactTrieNode *node = getCompactNode(fInfo, fNodeStack.peeki());
    int where = fIndexStack.peeki();
    while (!fNodeStack.empty() && U_SUCCESS(status)) {
        int nodeCount;

        bool isRoot = fNodeStack.peeki() == static_cast<int32_t>(fInfo->root);
        if(isRoot){
            nodeCount = node->flagscount & kRootCountMask;
        } else {
            nodeCount = getCount(node);
        }

        UBool goingDown = FALSE;
        if (nodeCount == 0) {
            // Terminal node; go up immediately
            fNodeStack.popi();
            fIndexStack.popi();
            node = getCompactNode(fInfo, fNodeStack.peeki());
            where = fIndexStack.peeki();
        }
        else if ((node->flagscount & kVerticalNode) && !isRoot) {
            // Vertical node
            const CompactTrieVerticalNode *vnode = (const CompactTrieVerticalNode *)node;
            if (where == 0) {
                // Going down
                unistr.append((const UChar *)vnode->chars, nodeCount);
                fIndexStack.setElementAt(1, fIndexStack.size()-1);
                node = getCompactNode(fInfo, fNodeStack.push(calcEqualLink(vnode), status));
                where = fIndexStack.push(0, status);
                goingDown = TRUE;
            }
            else {
                // Going up
                unistr.truncate(unistr.length()-nodeCount);
                fNodeStack.popi();
                fIndexStack.popi();
                node = getCompactNode(fInfo, fNodeStack.peeki());
                where = fIndexStack.peeki();
            }
        }
        else {
            // Horizontal node
            const CompactTrieHorizontalNode *hnode = (const CompactTrieHorizontalNode *)node;
            if (where > 0) {
                // Pop previous char
                unistr.truncate(unistr.length()-1);
            }
            if (where < nodeCount) {
                // Push on next node
                unistr.append((UChar)hnode->entries[where].ch);
                fIndexStack.setElementAt(where+1, fIndexStack.size()-1);
                node = getCompactNode(fInfo, fNodeStack.push(calcEqualLink(hnode, where, nodeCount), status));
                where = fIndexStack.push(0, status);
                goingDown = TRUE;
            }
            else {
                // Going up
                fNodeStack.popi();
                fIndexStack.popi();
                node = getCompactNode(fInfo, fNodeStack.peeki());
                where = fIndexStack.peeki();
            }
        }

        // Check if the parent of the node we've just gone down to ends a
        // word. If so, return it.
        // The root node should never end up here.
        if (goingDown && (node->flagscount & kParentEndsWord)) {
            return &unistr;
        }
    }
    return NULL;
}

StringEnumeration *
CompactTrieDictionary::openWords( UErrorCode &status ) const {
    if (U_FAILURE(status)) {
        return NULL;
    }
    return new CompactTrieEnumeration(fInfo, status);
}

//
// Below here is all code related to converting a ternary trie to a compact trie
// and back again
//

enum CompactTrieNodeType {
    kHorizontalType = 0,
    kVerticalType = 1,
    kValueType = 2
};

/**
 * The following classes (i.e. BuildCompactTrie*Node) are helper classes to 
 * construct the compact trie by storing information for each node and later 
 * writing the node to memory in a sequential format.
 */
class BuildCompactTrieNode: public UMemory {
public:
    UBool           fParentEndsWord;
    CompactTrieNodeType fNodeType;
    UBool           fHasDuplicate;
    UBool           fEqualOverflows;
    int32_t         fNodeID;
    UnicodeString   fChars;
    uint16_t        fValue;

public:
    BuildCompactTrieNode(UBool parentEndsWord, CompactTrieNodeType nodeType, 
            UStack &nodes, UErrorCode &status, uint16_t value = 0) {
        fParentEndsWord = parentEndsWord;
        fHasDuplicate = FALSE;
        fNodeType = nodeType;
        fEqualOverflows = FALSE;
        fNodeID = nodes.size();
        fValue = parentEndsWord? value : 0;
        nodes.push(this, status);
    }
    
    virtual ~BuildCompactTrieNode() {
    }
    
    virtual uint32_t size() {
        if(fValue > 0)
            return sizeof(uint16_t) * 2;
        else
            return sizeof(uint16_t);
    }
    
    virtual void write(uint8_t *bytes, uint32_t &offset, const UVector32 &/*translate*/) {
        // Write flag/count

        // if this ever fails, a flag bit (i.e. kExceedsCount) will need to be
        // used as a 5th MSB.
        U_ASSERT(fChars.length() < 4096 || fNodeID == 2);

        *((uint16_t *)(bytes+offset)) = (fEqualOverflows? kEqualOverflows : 0) | 
        ((fNodeID == 2)? (fChars.length() & kRootCountMask): 
            (
                    (fChars.length() & kCountMask) | 
                    //((fChars.length() << 2) & kExceedsCount) |
                    (fNodeType == kVerticalType ? kVerticalNode : 0) | 
                    (fParentEndsWord ? kParentEndsWord : 0 )
            )
        );
        offset += sizeof(uint16_t);
    }

    virtual void writeValue(uint8_t *bytes, uint32_t &offset) {
        if(fValue > 0){
            *((uint16_t *)(bytes+offset)) = fValue;
            offset += sizeof(uint16_t);
        }
    }

};

/**
 * Stores value of parent terminating nodes that have no more subtries. 
 */
class BuildCompactTrieValueNode: public BuildCompactTrieNode {
public:
    BuildCompactTrieValueNode(UStack &nodes, UErrorCode &status, uint16_t value)
        : BuildCompactTrieNode(TRUE, kValueType, nodes, status, value){
    }

    virtual ~BuildCompactTrieValueNode(){
    }

    virtual uint32_t size() {
        return sizeof(uint16_t) * 2;
    }

    virtual void write(uint8_t *bytes, uint32_t &offset, const UVector32 &translate) {
        // don't write value directly to memory but store it in offset to be written later
        //offset = fValue & kOffsetContainsValue;
        BuildCompactTrieNode::write(bytes, offset, translate);
        BuildCompactTrieNode::writeValue(bytes, offset);
    }
};

class BuildCompactTrieHorizontalNode: public BuildCompactTrieNode {
 public:
    UStack          fLinks;
    UBool           fMayOverflow; //intermediate value for fEqualOverflows

 public:
    BuildCompactTrieHorizontalNode(UBool parentEndsWord, UStack &nodes, UErrorCode &status, uint16_t value = 0)
    : BuildCompactTrieNode(parentEndsWord, kHorizontalType, nodes, status, value), fLinks(status) {
        fMayOverflow = FALSE;
    }
    
    virtual ~BuildCompactTrieHorizontalNode() {
    }
    
    // It is impossible to know beforehand exactly how much space the node will
    // need in memory before being written, because the node IDs in the equal
    // links may or may not overflow after node coalescing. Therefore, this method 
    // returns the maximum size possible for the node.
    virtual uint32_t size() {
        uint32_t estimatedSize = offsetof(CompactTrieHorizontalNode,entries) +
        (fChars.length()*sizeof(CompactTrieHorizontalEntry));

        if(fValue > 0)
            estimatedSize += sizeof(uint16_t);

        //estimate extra space needed to store overflow for node ID links
        //may be more than what is actually needed
        for(int i=0; i < fChars.length(); i++){
            if(((BuildCompactTrieNode *)fLinks[i])->fNodeID > 0xFFFF){
                fMayOverflow = TRUE;
                break;
            }          
        }
        if(fMayOverflow) // added space for overflow should be same as ceil(fChars.length()/4) * sizeof(uint16_t)
            estimatedSize += (sizeof(uint16_t) * fChars.length() + 2)/4;

        return estimatedSize;
    }
    
    virtual void write(uint8_t *bytes, uint32_t &offset, const UVector32 &translate) {
        int32_t count = fChars.length();

        //if largest nodeID > 2^16, set flag
        //large node IDs are more likely to be at the back of the array
        for (int32_t i = count-1; i >= 0; --i) {
            if(translate.elementAti(((BuildCompactTrieNode *)fLinks[i])->fNodeID) > 0xFFFF){
                fEqualOverflows = TRUE;
                break;
            }
        }

        BuildCompactTrieNode::write(bytes, offset, translate);

        // write entries[] to memory
        for (int32_t i = 0; i < count; ++i) {
            CompactTrieHorizontalEntry *entry = (CompactTrieHorizontalEntry *)(bytes+offset);
            entry->ch = fChars[i];
            entry->equal = translate.elementAti(((BuildCompactTrieNode *)fLinks[i])->fNodeID);
#ifdef DEBUG_TRIE_DICT

            if ((entry->equal == 0) && !fEqualOverflows) {
                fprintf(stderr, "ERROR: horizontal link %d, logical node %d maps to physical node zero\n",
                        i, ((BuildCompactTrieNode *)fLinks[i])->fNodeID);
            }
#endif
            offset += sizeof(CompactTrieHorizontalEntry);
        }

        // append extra bits of equal nodes to end if fEqualOverflows
        if (fEqualOverflows) {
            uint16_t leftmostBits = 0;
            for (int16_t i = 0; i < count; i++) {
                leftmostBits = (leftmostBits << 4) | getLeftmostBits(translate, i);

                // write filled uint16_t to memory
                if(i % 4 == 3){
                    *((uint16_t *)(bytes+offset)) = leftmostBits;
                    leftmostBits = 0;
                    offset += sizeof(uint16_t);
                }
            }

            // pad last uint16_t with zeroes if necessary
            int remainder = count % 4;
            if (remainder > 0) {
                *((uint16_t *)(bytes+offset)) = (leftmostBits << (16 - 4 * remainder));
                offset += sizeof(uint16_t);
            }
        }

        BuildCompactTrieNode::writeValue(bytes, offset);
    }

    // returns leftmost bits of physical node link
    uint16_t getLeftmostBits(const UVector32 &translate, uint32_t i){
        uint16_t leftmostBits = (uint16_t) (translate.elementAti(((BuildCompactTrieNode *)fLinks[i])->fNodeID) >> 16);
#ifdef DEBUG_TRIE_DICT
        if (leftmostBits > 0xF) {
            fprintf(stderr, "ERROR: horizontal link %d, logical node %d exceeds maximum possible node ID value\n",
                    i, ((BuildCompactTrieNode *)fLinks[i])->fNodeID);
        }
#endif
        return leftmostBits;
    }
    
    void addNode(UChar ch, BuildCompactTrieNode *link, UErrorCode &status) {
        fChars.append(ch);
        fLinks.push(link, status);
    }

};

class BuildCompactTrieVerticalNode: public BuildCompactTrieNode {
public:
    BuildCompactTrieNode    *fEqual;

public:
    BuildCompactTrieVerticalNode(UBool parentEndsWord, UStack &nodes, UErrorCode &status, uint16_t value = 0)
    : BuildCompactTrieNode(parentEndsWord, kVerticalType, nodes, status, value) {
        fEqual = NULL;
    }
    
    virtual ~BuildCompactTrieVerticalNode() {
    }
    
    // Returns the maximum possible size of this node. See comment in 
    // BuildCompactTrieHorizontal node for more information.
    virtual uint32_t size() {
        uint32_t estimatedSize = offsetof(CompactTrieVerticalNode,chars) + (fChars.length()*sizeof(uint16_t));
        if(fValue > 0){
            estimatedSize += sizeof(uint16_t);
        }

        if(fEqual->fNodeID > 0xFFFF){
            estimatedSize += sizeof(uint16_t);
        }
        return estimatedSize;
    }
    
    virtual void write(uint8_t *bytes, uint32_t &offset, const UVector32 &translate) {
        CompactTrieVerticalNode *node = (CompactTrieVerticalNode *)(bytes+offset);
        fEqualOverflows = (translate.elementAti(fEqual->fNodeID) > 0xFFFF);
        BuildCompactTrieNode::write(bytes, offset, translate);
        node->equal = translate.elementAti(fEqual->fNodeID);
        offset += sizeof(node->equal);
#ifdef DEBUG_TRIE_DICT
        if ((node->equal == 0) && !fEqualOverflows) {
            fprintf(stderr, "ERROR: vertical link, logical node %d maps to physical node zero\n",
                    fEqual->fNodeID);
        }
#endif
        fChars.extract(0, fChars.length(), (UChar *)node->chars);
        offset += sizeof(UChar)*fChars.length();

        // append 16 bits of to end for equal node if fEqualOverflows
        if (fEqualOverflows) {
            *((uint16_t *)(bytes+offset)) = (translate.elementAti(fEqual->fNodeID) >> 16);
            offset += sizeof(uint16_t);
        }

        BuildCompactTrieNode::writeValue(bytes, offset);
    }
    
    void addChar(UChar ch) {
        fChars.append(ch);
    }
    
    void setLink(BuildCompactTrieNode *node) {
        fEqual = node;
    }
    
};

// Forward declaration
static void walkHorizontal(const TernaryNode *node,
                            BuildCompactTrieHorizontalNode *building,
                            UStack &nodes,
                            UErrorCode &status,
                            Hashtable *values);

// Convert one TernaryNode into a BuildCompactTrieNode. Uses recursion.

static BuildCompactTrieNode *
compactOneNode(const TernaryNode *node, UBool parentEndsWord, UStack &nodes, 
        UErrorCode &status, Hashtable *values = NULL, uint16_t parentValue = 0) {
    if (U_FAILURE(status)) {
        return NULL;
    }
    BuildCompactTrieNode *result = NULL;
    UBool horizontal = (node->low != NULL || node->high != NULL);
    if (horizontal) {
        BuildCompactTrieHorizontalNode *hResult;
        if(values != NULL){
            hResult = new BuildCompactTrieHorizontalNode(parentEndsWord, nodes, status, parentValue);
        } else {
            hResult = new BuildCompactTrieHorizontalNode(parentEndsWord, nodes, status);
        }

        if (hResult == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return NULL;
        }
        if (U_SUCCESS(status)) {
            walkHorizontal(node, hResult, nodes, status, values);
            result = hResult;
        }
    }
    else {
        BuildCompactTrieVerticalNode *vResult;
        if(values != NULL){
            vResult = new BuildCompactTrieVerticalNode(parentEndsWord, nodes, status, parentValue);
        } else { 
            vResult = new BuildCompactTrieVerticalNode(parentEndsWord, nodes, status);
        }

        if (vResult == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return NULL;
        }
        else if (U_SUCCESS(status)) {
            uint16_t   value = 0;
            UBool endsWord = FALSE;
            // Take up nodes until we end a word, or hit a node with < or > links
            do {
                vResult->addChar(node->ch);
                value = node->flags;
                endsWord = value > 0;
                node = node->equal;
            }
            while(node != NULL && !endsWord && node->low == NULL && node->high == NULL);

            if (node == NULL) {
                if (!endsWord) {
                    status = U_ILLEGAL_ARGUMENT_ERROR;  // Corrupt input trie
                }
                else if(values != NULL){
                    UnicodeString key(value); //store value as a single-char UnicodeString
                    BuildCompactTrieValueNode *link = (BuildCompactTrieValueNode *) values->get(key);
                    if(link == NULL){
                        link = new BuildCompactTrieValueNode(nodes, status, value); //take out nodes?
                        values->put(key, link, status);
                    }
                    vResult->setLink(link);
                } else {
                    vResult->setLink((BuildCompactTrieNode *)nodes[1]);
                }
            }
            else {
                vResult->setLink(compactOneNode(node, endsWord, nodes, status, values, value));
            }
            result = vResult;
        }
    }
    return result;
}

// Walk the set of peers at the same level, to build a horizontal node.
// Uses recursion.

static void walkHorizontal(const TernaryNode *node,
                           BuildCompactTrieHorizontalNode *building,
                           UStack &nodes,
                           UErrorCode &status, Hashtable *values = NULL) {
    while (U_SUCCESS(status) && node != NULL) {
        if (node->low != NULL) {
            walkHorizontal(node->low, building, nodes, status, values);
        }
        BuildCompactTrieNode *link = NULL;
        if (node->equal != NULL) {
            link = compactOneNode(node->equal, node->flags > 0, nodes, status, values, node->flags);
        }
        else if (node->flags > 0) {
            if(values != NULL) {
                UnicodeString key(node->flags); //store value as a single-char UnicodeString
                link = (BuildCompactTrieValueNode *) values->get(key);
                if(link == NULL) {
                    link = new BuildCompactTrieValueNode(nodes, status, node->flags); //take out nodes?
                    values->put(key, link, status);
                }
            } else {
                link = (BuildCompactTrieNode *)nodes[1];
            }
        }
        if (U_SUCCESS(status) && link != NULL) {
            building->addNode(node->ch, link, status);
        }
        // Tail recurse manually instead of leaving it to the compiler.
        //if (node->high != NULL) {
        //    walkHorizontal(node->high, building, nodes, status);
        //}
        node = node->high;
    }
}

U_NAMESPACE_END
U_NAMESPACE_USE
U_CDECL_BEGIN
static int32_t U_CALLCONV
_sortBuildNodes(const void * /*context*/, const void *voidl, const void *voidr) {
    BuildCompactTrieNode *left = *(BuildCompactTrieNode **)voidl;
    BuildCompactTrieNode *right = *(BuildCompactTrieNode **)voidr;

    // Check for comparing a node to itself, to avoid spurious duplicates
    if (left == right) {
        return 0;
    }

    // Most significant is type of node. Can never coalesce.
    if (left->fNodeType != right->fNodeType) {
        return left->fNodeType - right->fNodeType;
    }
    // Next, the "parent ends word" flag. If that differs, we cannot coalesce.
    if (left->fParentEndsWord != right->fParentEndsWord) {
        return left->fParentEndsWord - right->fParentEndsWord;
    }
    // Next, the string. If that differs, we can never coalesce.
    int32_t result = left->fChars.compare(right->fChars);
    if (result != 0) {
        return result;
    }

    // If the node value differs, we should not coalesce.
    // If values aren't stored, all fValues should be 0.
    if (left->fValue != right->fValue) {
        return left->fValue - right->fValue;
    }

    // We know they're both the same node type, so branch for the two cases.
    if (left->fNodeType == kVerticalType) {
        result = ((BuildCompactTrieVerticalNode *)left)->fEqual->fNodeID
        - ((BuildCompactTrieVerticalNode *)right)->fEqual->fNodeID;
    }
    else if(left->fChars.length() > 0 && right->fChars.length() > 0){
        // We need to compare the links vectors. They should be the
        // same size because the strings were equal.
        // We compare the node IDs instead of the pointers, to handle
        // coalesced nodes.
        BuildCompactTrieHorizontalNode *hleft, *hright;
        hleft = (BuildCompactTrieHorizontalNode *)left;
        hright = (BuildCompactTrieHorizontalNode *)right;
        int32_t count = hleft->fLinks.size();
        for (int32_t i = 0; i < count && result == 0; ++i) {
            result = ((BuildCompactTrieNode *)(hleft->fLinks[i]))->fNodeID -
            ((BuildCompactTrieNode *)(hright->fLinks[i]))->fNodeID;
        }
    }

    // If they are equal to each other, mark them (speeds coalescing)
    if (result == 0) {
        left->fHasDuplicate = TRUE;
        right->fHasDuplicate = TRUE;
    }
    return result;
}
U_CDECL_END
U_NAMESPACE_BEGIN

static void coalesceDuplicates(UStack &nodes, UErrorCode &status) {
    // We sort the array of nodes to place duplicates next to each other
    if (U_FAILURE(status)) {
        return;
    }
    int32_t size = nodes.size();
    void **array = (void **)uprv_malloc(sizeof(void *)*size);
    if (array == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return;
    }
    (void) nodes.toArray(array);
    
    // Now repeatedly identify duplicates until there are no more
    int32_t dupes = 0;
    long    passCount = 0;
#ifdef DEBUG_TRIE_DICT
    long    totalDupes = 0;
#endif
    do {
        BuildCompactTrieNode *node;
        BuildCompactTrieNode *first = NULL;
        BuildCompactTrieNode **p;
        BuildCompactTrieNode **pFirst = NULL;
        int32_t counter = size - 2;
        // Sort the array, skipping nodes 0 and 1. Use quicksort for the first
        // pass for speed. For the second and subsequent passes, we use stable
        // (insertion) sort for two reasons:
        // 1. The array is already mostly ordered, so we get better performance.
        // 2. The way we find one and only one instance of a set of duplicates is to
        //    check that the node ID equals the array index. If we used an unstable
        //    sort for the second or later passes, it's possible that none of the
        //    duplicates would wind up with a node ID equal to its array index.
        //    The sort stability guarantees that, because as we coalesce more and
        //    more groups, the first element of the resultant group will be one of
        //    the first elements of the groups being coalesced.
        // To use quicksort for the second and subsequent passes, we would have to
        // find the minimum of the node numbers in a group, and set all the nodes
        // in the group to that node number.
        uprv_sortArray(array+2, counter, sizeof(void *), _sortBuildNodes, NULL, (passCount > 0), &status);
        dupes = 0;
        for (p = (BuildCompactTrieNode **)array + 2; counter > 0; --counter, ++p) {
            node = *p;
            if (node->fHasDuplicate) {
                if (first == NULL) {
                    first = node;
                    pFirst = p;
                }
                else if (_sortBuildNodes(NULL, pFirst, p) != 0) {
                    // Starting a new run of dupes
                    first = node;
                    pFirst = p;
                }
                else if (node->fNodeID != first->fNodeID) {
                    // Slave one to the other, note duplicate
                    node->fNodeID = first->fNodeID;
                    dupes += 1;
                }
            }
            else {
                // This node has no dupes
                first = NULL;
                pFirst = NULL;
            }
        }
        passCount += 1;
#ifdef DEBUG_TRIE_DICT
        totalDupes += dupes;
        fprintf(stderr, "Trie node dupe removal, pass %d: %d nodes tagged\n", passCount, dupes);
#endif
    }
    while (dupes > 0);
#ifdef DEBUG_TRIE_DICT
    fprintf(stderr, "Trie node dupe removal complete: %d tagged in %d passes\n", totalDupes, passCount);
#endif

    // We no longer need the temporary array, as the nodes have all been marked appropriately.
    uprv_free(array);
}

U_NAMESPACE_END
U_CDECL_BEGIN
static void U_CALLCONV _deleteBuildNode(void *obj) {
    delete (BuildCompactTrieNode *) obj;
}
U_CDECL_END
U_NAMESPACE_BEGIN

CompactTrieHeader *
CompactTrieDictionary::compactMutableTrieDictionary( const MutableTrieDictionary &dict,
                                UErrorCode &status ) {
    if (U_FAILURE(status)) {
        return NULL;
    }
#ifdef DEBUG_TRIE_DICT
    struct tms timing;
    struct tms previous;
    (void) ::times(&previous);
#endif
    UStack nodes(_deleteBuildNode, NULL, status);      // Index of nodes

    // Add node 0, used as the NULL pointer/sentinel.
    nodes.addElement((int32_t)0, status);

    Hashtable *values = NULL;                           // Index of (unique) values
    if (dict.fValued) {
        values = new Hashtable(status);
    }

    // Start by creating the special empty node we use to indicate that the parent
    // terminates a word. This must be node 1, because the builder assumes
    // that. This node will never be used for tries storing numerical values.
    if (U_FAILURE(status)) {
        return NULL;
    }
    BuildCompactTrieNode *terminal = new BuildCompactTrieNode(TRUE, kHorizontalType, nodes, status);
    if (terminal == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
    }

    // This call does all the work of building the new trie structure. The root
    // will have node ID 2 before writing to memory.
    BuildCompactTrieNode *root = compactOneNode(dict.fTrie, FALSE, nodes, status, values);
#ifdef DEBUG_TRIE_DICT
    (void) ::times(&timing);
    fprintf(stderr, "Compact trie built, %d nodes, time user %f system %f\n",
        nodes.size(), (double)(timing.tms_utime-previous.tms_utime)/CLK_TCK,
        (double)(timing.tms_stime-previous.tms_stime)/CLK_TCK);
    previous = timing;
#endif

    // Now coalesce all duplicate nodes.
    coalesceDuplicates(nodes, status);
#ifdef DEBUG_TRIE_DICT
    (void) ::times(&timing);
    fprintf(stderr, "Duplicates coalesced, time user %f system %f\n",
        (double)(timing.tms_utime-previous.tms_utime)/CLK_TCK,
        (double)(timing.tms_stime-previous.tms_stime)/CLK_TCK);
    previous = timing;
#endif

    // Next, build the output trie.
    // First we compute all the sizes and build the node ID translation table.
    uint32_t totalSize = offsetof(CompactTrieHeader,offsets);
    int32_t count = nodes.size();
    int32_t nodeCount = 1;              // The sentinel node we already have
    BuildCompactTrieNode *node;
    int32_t i;
    UVector32 translate(count, status); // Should be no growth needed after this
    translate.push(0, status);          // The sentinel node
    
    if (U_FAILURE(status)) {
        return NULL;
    }

    //map terminal value nodes
    int valueCount = 0;
    UVector valueNodes(status);
    if(values != NULL) {
        valueCount = values->count(); //number of unique terminal value nodes
    }
     
    // map non-terminal nodes
    int valuePos = 1;//, nodePos = valueCount + valuePos;
    nodeCount = valueCount + valuePos;
    for (i = 1; i < count; ++i) {
        node = (BuildCompactTrieNode *)nodes[i];
        if (node->fNodeID == i) {
            // Only one node out of each duplicate set is used
            if (node->fNodeID >= translate.size()) {
                // Logically extend the mapping table
                translate.setSize(i + 1);
            }
            //translate.setElementAt(object, index)!
            if(node->fNodeType == kValueType) {
                valueNodes.addElement(node, status);
               translate.setElementAt(valuePos++, i);
             } else {
                translate.setElementAt(nodeCount++, i);
            }
            totalSize += node->size();
        }
    }

    // Check for overflowing 20 bits worth of nodes.
    if (nodeCount > 0x100000) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    }
    
    // Add enough room for the offsets.
    totalSize += nodeCount*sizeof(uint32_t);
#ifdef DEBUG_TRIE_DICT
    (void) ::times(&timing);
    fprintf(stderr, "Sizes/mapping done, time user %f system %f\n",
        (double)(timing.tms_utime-previous.tms_utime)/CLK_TCK,
        (double)(timing.tms_stime-previous.tms_stime)/CLK_TCK);
    previous = timing;
    fprintf(stderr, "%d nodes, %d unique, %d bytes\n", nodes.size(), nodeCount, totalSize);
#endif
    uint8_t *bytes = (uint8_t *)uprv_malloc(totalSize);
    if (bytes == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return NULL;
    }
    
    CompactTrieHeader *header = (CompactTrieHeader *)bytes;
    //header->size = totalSize;
    if(dict.fValued){
        header->magic = COMPACT_TRIE_MAGIC_3;
    } else {
        header->magic = COMPACT_TRIE_MAGIC_2;
    }
    header->nodeCount = nodeCount;
    header->offsets[0] = 0;                     // Sentinel
    header->root = translate.elementAti(root->fNodeID);
#ifdef DEBUG_TRIE_DICT
    if (header->root == 0) {
        fprintf(stderr, "ERROR: root node %d translate to physical zero\n", root->fNodeID);
    }
#endif
    uint32_t offset = offsetof(CompactTrieHeader,offsets)+(nodeCount*sizeof(uint32_t));
    nodeCount = valueCount + 1;

    // Write terminal value nodes to memory
    for (i=0; i < valueNodes.size(); i++) {
        //header->offsets[i + 1] = offset;
        uint32_t tmpOffset = 0;
        node = (BuildCompactTrieNode *) valueNodes.elementAt(i);
        //header->offsets[i + 1] = (uint32_t)node->fValue;
        node->write((uint8_t *)&header->offsets[i+1], tmpOffset, translate);
    }

    // Now write the data
    for (i = 1; i < count; ++i) {
        node = (BuildCompactTrieNode *)nodes[i];
        if (node->fNodeID == i && node->fNodeType != kValueType) {
            header->offsets[nodeCount++] = offset;
            node->write(bytes, offset, translate);
        }
    }

    //free all extra space
    uprv_realloc(bytes, offset);
    header->size = offset;

#ifdef DEBUG_TRIE_DICT
    fprintf(stdout, "Space freed: %d\n", totalSize-offset);

    (void) ::times(&timing);
    fprintf(stderr, "Trie built, time user %f system %f\n",
        (double)(timing.tms_utime-previous.tms_utime)/CLK_TCK,
        (double)(timing.tms_stime-previous.tms_stime)/CLK_TCK);
    previous = timing;
    fprintf(stderr, "Final offset is %d\n", offset);

    // Collect statistics on node types and sizes
    int hCount = 0;
    int vCount = 0;
    size_t hSize = 0;
    size_t vSize = 0;
    size_t hItemCount = 0;
    size_t vItemCount = 0;
    uint32_t previousOff = offset;
    uint32_t numOverflow = 0;
    uint32_t valueSpace = 0;
    for (uint32_t nodeIdx = nodeCount-1; nodeIdx >= 2; --nodeIdx) {
        const CompactTrieNode *node = getCompactNode(header, nodeIdx);
        int itemCount;
        if(nodeIdx == header->root)
            itemCount = node->flagscount & kRootCountMask;
        else
            itemCount = getCount(node);
        if(node->flagscount & kEqualOverflows){
            numOverflow++;
        }
        if (node->flagscount & kVerticalNode && nodeIdx != header->root) {
            vCount += 1;
            vItemCount += itemCount;
            vSize += previousOff-header->offsets[nodeIdx];
        }
        else {
            hCount += 1;
            hItemCount += itemCount;
            if(nodeIdx >= header->root) {
                hSize += previousOff-header->offsets[nodeIdx];
            }
        }
        
        if(header->magic == COMPACT_TRIE_MAGIC_3 && node->flagscount & kParentEndsWord)
            valueSpace += sizeof(uint16_t);
        previousOff = header->offsets[nodeIdx];
    }
    fprintf(stderr, "Horizontal nodes: %d total, average %f bytes with %f items\n", hCount,
                (double)hSize/hCount, (double)hItemCount/hCount);
    fprintf(stderr, "Vertical nodes: %d total, average %f bytes with %f items\n", vCount,
                (double)vSize/vCount, (double)vItemCount/vCount);
    fprintf(stderr, "Number of nodes with overflowing nodeIDs: %d \n", numOverflow);
    fprintf(stderr, "Space taken up by values: %d \n", valueSpace);
#endif

    if (U_FAILURE(status)) {
        uprv_free(bytes);
        header = NULL;
    }
    return header;
}

// Forward declaration
static TernaryNode *
unpackOneNode( const CompactTrieInfo *info, const CompactTrieNode *node, UErrorCode &status );

// Convert a horizontal node (or subarray thereof) into a ternary subtrie
static TernaryNode *
unpackHorizontalArray( const CompactTrieInfo *info, const CompactTrieHorizontalNode *hnode,
        int low, int high, int nodeCount, UErrorCode &status) {
    if (U_FAILURE(status) || low > high) {
        return NULL;
    }
    int middle = (low+high)/2;
    TernaryNode *result = new TernaryNode(hnode->entries[middle].ch);
    if (result == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return NULL;
    }
    const CompactTrieNode *equal = getCompactNode(info, calcEqualLink(hnode, middle, nodeCount));
    if (equal->flagscount & kParentEndsWord) {
        if(info->magic == COMPACT_TRIE_MAGIC_3){
            result->flags = getValue(equal);
        }else{
            result->flags |= kEndsWord;
        }
    }
    result->low = unpackHorizontalArray(info, hnode, low, middle-1, nodeCount, status);
    result->high = unpackHorizontalArray(info, hnode, middle+1, high, nodeCount, status);
    result->equal = unpackOneNode(info, equal, status);
    return result;
}                            

// Convert one compact trie node into a ternary subtrie
static TernaryNode *
unpackOneNode( const CompactTrieInfo *info, const CompactTrieNode *node, UErrorCode &status ) {
    int nodeCount = getCount(node);
    if (nodeCount == 0 || U_FAILURE(status)) {
        // Failure, or terminal node
        return NULL;
    }
    if (node->flagscount & kVerticalNode) {
        const CompactTrieVerticalNode *vnode = (const CompactTrieVerticalNode *)node;
        TernaryNode *head = NULL;
        TernaryNode *previous = NULL;
        TernaryNode *latest = NULL;
        for (int i = 0; i < nodeCount; ++i) {
            latest = new TernaryNode(vnode->chars[i]);
            if (latest == NULL) {
                status = U_MEMORY_ALLOCATION_ERROR;
                break;
            }
            if (head == NULL) {
                head = latest;
            }
            if (previous != NULL) {
                previous->equal = latest;
            }
            previous = latest;
        }
        if (latest != NULL) {
            const CompactTrieNode *equal = getCompactNode(info, calcEqualLink(vnode));
            if (equal->flagscount & kParentEndsWord) {
                if(info->magic == COMPACT_TRIE_MAGIC_3){
                    latest->flags = getValue(equal);
                } else {
                    latest->flags |= kEndsWord;
                }
            }
            latest->equal = unpackOneNode(info, equal, status);
        }
        return head;
    }
    else {
        // Horizontal node
        const CompactTrieHorizontalNode *hnode = (const CompactTrieHorizontalNode *)node;
        return unpackHorizontalArray(info, hnode, 0, nodeCount-1, nodeCount, status);
    }
}

// returns a MutableTrieDictionary generated from the CompactTrieDictionary
MutableTrieDictionary *
CompactTrieDictionary::cloneMutable( UErrorCode &status ) const {
    MutableTrieDictionary *result = new MutableTrieDictionary( status, fInfo->magic == COMPACT_TRIE_MAGIC_3 );
    if (result == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return NULL;
    }
    // treat root node as special case: don't call unpackOneNode() or unpackHorizontalArray() directly
    // because only kEqualOverflows flag should be checked in root's flagscount
    const CompactTrieHorizontalNode *hnode = (const CompactTrieHorizontalNode *) 
    getCompactNode(fInfo, fInfo->root);
    uint16_t nodeCount = hnode->flagscount & kRootCountMask;
    TernaryNode *root = unpackHorizontalArray(fInfo, hnode, 0, nodeCount-1, 
            nodeCount, status);

    if (U_FAILURE(status)) {
        delete root;    // Clean up
        delete result;
        return NULL;
    }
    result->fTrie = root;
    return result;
}

U_NAMESPACE_END

U_CAPI int32_t U_EXPORT2
triedict_swap(const UDataSwapper *ds, const void *inData, int32_t length, void *outData,
        UErrorCode *status) {
    
    if (status == NULL || U_FAILURE(*status)) {
        return 0;
    }
    if(ds==NULL || inData==NULL || length<-1 || (length>0 && outData==NULL)) {
        *status=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    //
    //  Check that the data header is for for dictionary data.
    //    (Header contents are defined in genxxx.cpp)
    //
    const UDataInfo *pInfo = (const UDataInfo *)((const uint8_t *)inData+4);
    if(!(  pInfo->dataFormat[0]==0x54 &&   /* dataFormat="TrDc" */
            pInfo->dataFormat[1]==0x72 &&
            pInfo->dataFormat[2]==0x44 &&
            pInfo->dataFormat[3]==0x63 &&
            pInfo->formatVersion[0]==1  )) {
        udata_printError(ds, "triedict_swap(): data format %02x.%02x.%02x.%02x (format version %02x) is not recognized\n",
                pInfo->dataFormat[0], pInfo->dataFormat[1],
                pInfo->dataFormat[2], pInfo->dataFormat[3],
                pInfo->formatVersion[0]);
        *status=U_UNSUPPORTED_ERROR;
        return 0;
    }

    //
    // Swap the data header.  (This is the generic ICU Data Header, not the 
    //                         CompactTrieHeader).  This swap also conveniently gets us
    //                         the size of the ICU d.h., which lets us locate the start
    //                         of the RBBI specific data.
    //
    int32_t headerSize=udata_swapDataHeader(ds, inData, length, outData, status);

    //
    // Get the CompactTrieHeader, and check that it appears to be OK.
    //
    const uint8_t  *inBytes =(const uint8_t *)inData+headerSize;
    const CompactTrieHeader *header = (const CompactTrieHeader *)inBytes;
    uint32_t magic = ds->readUInt32(header->magic);
    if (magic != COMPACT_TRIE_MAGIC_1 && magic != COMPACT_TRIE_MAGIC_2 && magic != COMPACT_TRIE_MAGIC_3
            || magic == COMPACT_TRIE_MAGIC_1 && ds->readUInt32(header->size) < sizeof(CompactTrieHeaderV1)
            || magic != COMPACT_TRIE_MAGIC_1 && ds->readUInt32(header->size) < sizeof(CompactTrieHeader))
    {
        udata_printError(ds, "triedict_swap(): CompactTrieHeader is invalid.\n");
        *status=U_UNSUPPORTED_ERROR;
        return 0;
    }

    //
    // Prefight operation?  Just return the size
    //
    uint32_t totalSize = ds->readUInt32(header->size);
    int32_t sizeWithUData = (int32_t)totalSize + headerSize;
    if (length < 0) {
        return sizeWithUData;
    }

    //
    // Check that length passed in is consistent with length from RBBI data header.
    //
    if (length < sizeWithUData) {
        udata_printError(ds, "triedict_swap(): too few bytes (%d after ICU Data header) for trie data.\n",
                totalSize);
        *status=U_INDEX_OUTOFBOUNDS_ERROR;
        return 0;
    }

    //
    // Swap the Data.  Do the data itself first, then the CompactTrieHeader, because
    //                 we need to reference the header to locate the data, and an
    //                 inplace swap of the header leaves it unusable.
    //
    uint8_t             *outBytes = (uint8_t *)outData + headerSize;
    CompactTrieHeader   *outputHeader = (CompactTrieHeader *)outBytes;

#if 0
    //
    // If not swapping in place, zero out the output buffer before starting.
    //
    if (inBytes != outBytes) {
        uprv_memset(outBytes, 0, totalSize);
    }

    // We need to loop through all the nodes in the offset table, and swap each one.
    uint32_t nodeCount, rootId;
    if(header->magic == COMPACT_TRIE_MAGIC_1) {
        nodeCount = ds->readUInt16(((CompactTrieHeaderV1 *)header)->nodeCount);
        rootId = ds->readUInt16(((CompactTrieHeaderV1 *)header)->root);
    } else {
        nodeCount = ds->readUInt32(header->nodeCount);
        rootId = ds->readUInt32(header->root);
    }

    // Skip node 0, which should always be 0.
    for (uint32_t i = 1; i < nodeCount; ++i) {
        uint32_t nodeOff = ds->readUInt32(header->offsets[i]);
        const CompactTrieNode *inNode = (const CompactTrieNode *)(inBytes + nodeOff);
        CompactTrieNode *outNode = (CompactTrieNode *)(outBytes + nodeOff);
        uint16_t flagscount = ds->readUInt16(inNode->flagscount);
        uint16_t itemCount = getCount(inNode);
        //uint16_t itemCount = flagscount & kCountMask;
        ds->writeUInt16(&outNode->flagscount, flagscount);
        if (itemCount > 0) {
            uint16_t overflow = 0; //number of extra uint16_ts needed to be swapped
            if (flagscount & kVerticalNode && i != rootId) {
                if(flagscount & kEqualOverflows){
                    // include overflow bits
                    overflow += 1;
                }
                if (header->magic == COMPACT_TRIE_MAGIC_3 && flagscount & kEndsParentWord) {
                    //include values
                    overflow += 1;
                }
                ds->swapArray16(ds, inBytes+nodeOff+offsetof(CompactTrieVerticalNode,chars),
                        (itemCount + overflow)*sizeof(uint16_t),
                        outBytes+nodeOff+offsetof(CompactTrieVerticalNode,chars), status);
                uint16_t equal = ds->readUInt16(inBytes+nodeOff+offsetof(CompactTrieVerticalNode,equal);
                ds->writeUInt16(outBytes+nodeOff+offsetof(CompactTrieVerticalNode,equal));
            }
            else {
                const CompactTrieHorizontalNode *inHNode = (const CompactTrieHorizontalNode *)inNode;
                CompactTrieHorizontalNode *outHNode = (CompactTrieHorizontalNode *)outNode;
                for (int j = 0; j < itemCount; ++j) {
                    uint16_t word = ds->readUInt16(inHNode->entries[j].ch);
                    ds->writeUInt16(&outHNode->entries[j].ch, word);
                    word = ds->readUInt16(inHNode->entries[j].equal);
                    ds->writeUInt16(&outHNode->entries[j].equal, word);
                }

                // swap overflow/value information
                if(flagscount & kEqualOverflows){
                    overflow += (itemCount + 3) / 4;
                }

                if (header->magic == COMPACT_TRIE_MAGIC_3 && i != rootId && flagscount & kEndsParentWord) {
                    //include values
                    overflow += 1;
                }

                uint16_t *inOverflow = (uint16_t *) &inHNode->entries[itemCount];
                uint16_t *outOverflow = (uint16_t *) &outHNode->entries[itemCount];
                for(int j = 0; j<overflow; j++){
                    uint16_t extraInfo = ds->readUInt16(*inOverflow);
                    ds->writeUInt16(outOverflow, extraInfo);

                    inOverflow++;
                    outOverflow++;
                }
            }
        }
    }
#endif

    // Swap the header
    ds->writeUInt32(&outputHeader->size, totalSize);
    ds->writeUInt32(&outputHeader->magic, magic);

    uint32_t nodeCount;
    uint32_t offsetPos;
    if (header->magic == COMPACT_TRIE_MAGIC_1) {
        CompactTrieHeaderV1 *headerV1 = (CompactTrieHeaderV1 *)header;
        CompactTrieHeaderV1 *outputHeaderV1 = (CompactTrieHeaderV1 *)outputHeader;

        nodeCount = ds->readUInt16(headerV1->nodeCount);
        ds->writeUInt16(&outputHeaderV1->nodeCount, nodeCount);
        uint16_t root = ds->readUInt16(headerV1->root);
        ds->writeUInt16(&outputHeaderV1->root, root);
        offsetPos = offsetof(CompactTrieHeaderV1,offsets);
    } else {
        nodeCount = ds->readUInt32(header->nodeCount);
        ds->writeUInt32(&outputHeader->nodeCount, nodeCount);
        uint32_t root = ds->readUInt32(header->root);
        ds->writeUInt32(&outputHeader->root, root);
        offsetPos = offsetof(CompactTrieHeader,offsets);
    }

    // All the data in all the nodes consist of 16 bit items. Swap them all at once.
    uint32_t nodesOff = offsetPos+((uint32_t)nodeCount*sizeof(uint32_t));
    ds->swapArray16(ds, inBytes+nodesOff, totalSize-nodesOff, outBytes+nodesOff, status);

    //swap offsets
    ds->swapArray32(ds, inBytes+offsetPos,
            sizeof(uint32_t)*(uint32_t)nodeCount,
            outBytes+offsetPos, status);

    return sizeWithUData;
}

#endif /* #if !UCONFIG_NO_BREAK_ITERATION */
