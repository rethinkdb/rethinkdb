// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef BUFFER_CACHE_MIRRORED_PAGE_REPL_RANDOM_HPP_
#define BUFFER_CACHE_MIRRORED_PAGE_REPL_RANDOM_HPP_

#include "buffer_cache/types.hpp"
#include "config/args.hpp"
#include "containers/two_level_array.hpp"

// TODO: We should use mlock (or mlockall or related) to make sure the
// OS doesn't swap out our pages, since we're doing swapping
// ourselves.

/*
The random page replacement algorithm needs to be able to quickly choose a random buf among all the
bufs in memory. This is accomplished using a dense array of buf_lock_t* in a completely arbitrary order.
Because the array is dense, choosing a random buf is as simple as choosing a random number less
than the number of bufs in memory. When a buf is removed from memory, the last buf in the array is
moved to the slot it last occupied, keeping the array dense. Each buf carries an index which is its
position in the dense random array; this allows all insertion, deletion, and random selection to be
done in constant time.
*/

class mc_cache_t;

class evictable_t {
public:
    explicit evictable_t(mc_cache_t *cache, bool loaded = true);
    virtual ~evictable_t();    // removes us from the page repl if necessary; does not call unload()

    // Returns true if this object can be unloaded from the cache.
    virtual bool safe_to_unload() = 0;
    // Called when the page_repl_random_t decides to evict this object. Must relinquish the buf
    // associated with this object.
    virtual void unload() = 0;

    bool in_page_repl();
    void insert_into_page_repl();
    void remove_from_page_repl(); // does *not* call unload()

    /* The eviction priority represents how bad of a choice a buf is for
     * eviction the buffer cache will (probabalistically) evict blocks of
     * lower priority first. */
    eviction_priority_t eviction_priority;

protected:
    mc_cache_t *cache;
private:
    unsigned int page_repl_index;
};

class page_repl_random_t {
    typedef mc_cache_t cache_t;
    friend class evictable_t;

public:

    page_repl_random_t(unsigned int _unload_threshold, cache_t *_cache);

    // If is_full(space_needed), the next call to make_space(space_needed) probably has to evict something
    bool is_full(unsigned int space_needed);

    // make_space tries to make sure that the number of blocks currently in memory is at least
    // 'space_needed' less than the user-specified memory limit.
    void make_space(unsigned int space_needed = 0);

    /* The page replacement component actually serves two roles. In addition to its primary role as
    a mechanism for kicking out buffers when memory runs low, it also has the job of keeping track
    of all of the buffers in memory in such a way that the cache can quickly request a pointer to
    the next buffer in memory. This is used during the cache's destructor. The rationale is that any
    reasonable implementation of a page replacement system will need to keep track of all of the
    buffers in memory anyway, so the cache can depend on the page replacement system's buffer list
    rather than keeping a buffer list of its own. */
    evictable_t *get_first_buf();

private:
    unsigned int unload_threshold;
    cache_t *cache;
    two_level_array_t<evictable_t*, MAX_BLOCKS_IN_MEMORY, (1 << 12)> array;
};

#endif // BUFFER_CACHE_MIRRORED_PAGE_REPL_RANDOM_HPP_
