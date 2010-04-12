
#ifndef __VOLATILE_CACHE_HPP__
#define __VOLATILE_CACHE_HPP__

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
template <class config_t>
struct volatile_cache_t {
public:
    typedef typename config_t::fsm_t fsm_t;
    typedef void* block_id_t;

public:
    static const block_id_t null_block_id;

    /* Returns true iff block_id is NULL. */
    static bool is_block_id_null(block_id_t block_id) {
        return block_id == NULL;
    }

    block_id_t get_superblock_id() {
        return superblock_id;
    }

public:
    volatile_cache_t(size_t _block_size) : block_size(_block_size) {
        superblock_id = malloc_aligned(block_size, block_size);
    }

    virtual ~volatile_cache_t() {
        free(superblock_id);
    }
    
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
    void* acquire(block_id_t block_id, fsm_t *state) {
        return block_id;
    }

    /* Releases a block at the given memory address. If dirty is true,
     * schedules the block to be flushed according to the flushing
     * policy. After release is called, the block may be evicted at
     * any time. Returns the new id of the block - it may or may not
     * be the same as the old block id, depending on the behavior of
     * the underlying serializer. */
    block_id_t release(block_id_t block_id, void *block, bool dirty, fsm_t *state) {
        // Since this is a volatile cache, we have nothing to do here.
        return block_id;
    }

    /* Normally handles an AIO completion event, but there's nothing
     * to do for the vilatile cache. */
    void aio_complete(block_id_t block_id, void *block, bool written) {
    }

protected:
    size_t block_size;
    block_id_t superblock_id;
};

template <class config_t>
const typename volatile_cache_t<config_t>::block_id_t volatile_cache_t<config_t>::null_block_id = NULL;

#endif // __VOLATILE_CACHE_HPP__

