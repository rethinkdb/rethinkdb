
#ifndef __VOLATILE_HPP__
#define __VOLATILE_HPP__

#include <stdlib.h>
#include "utils.hpp"

// TODO: what about block eviction priority? (do we need this info
// here, or will dumb global LRU suffice?)
// TODO: what about multiple modifications to the tree that need to be
// performed atomically?
// TODO: what about different sized blocks?

/* This is a non-persistent cache that maintains all blocks in
 * memory. It can be used for testing, for non-volatile needs, and in
 * combination with disk caches to allow for persistence. */
struct volatile_cache_t {
public:
    typedef void* block_id_t;

public:
    volatile_cache_t(size_t _block_size) : block_size(_block_size) {}
    
    /* Allocates a new block and records its id in *block_id. The
     * block is automatically acquired and must be released later. */
    void* allocate(block_id_t *block_id) {
        // TODO: use a good allocator
        *block_id = malloc_aligned(block_size, block_size);
        return *block_id;
    }
    
    /* Acquire a block with a given id, and return a pointer to the
     * associated memory. If the block cannot be acquired immediately
     * and must be acquired asynchronously, returns NULL and attaches
     * state to the asynchronous request. The acquired block is
     * guranteed not to be evicted from the cache until release is
     * called on it. */
    void* acquire(block_id_t block_id, void *state) {
        return block_id;
    }

    /* Releases a block at the given memory address. If dirty is true,
     * schedules the block to be flushed according to the flushing
     * policy. After release is called, the block may be evicted at
     * any time. Returns the new id of the block - it may or may not
     * be the same as the old block id, depending on the behavior of
     * the underlying serializer. */
    block_id_t release(block_id_t block_id, void *block, bool dirty, void *state) {
        // Since this is a volatile cache, we have nothing to do here.
        return block_id;
    }

protected:
    size_t block_size;
};

#endif // __VOLATILE_HPP__

