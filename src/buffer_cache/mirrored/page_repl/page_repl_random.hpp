
#ifndef __PAGE_REPL_RANDOM_HPP__
#define __PAGE_REPL_RANDOM_HPP__

#include "config/args.hpp"
#include "logger.hpp"
#include "containers/two_level_array.hpp"

// TODO: We should use mlock (or mlockall or related) to make sure the
// OS doesn't swap out our pages, since we're doing swapping
// ourselves.

/*
The random page replacement algorithm needs to be able to quickly choose a random buf among all the
bufs in memory. This is accomplished using a dense array of buf_t* in a completely arbitrary order.
Because the array is dense, choosing a random buf is as simple as choosing a random number less
than the number of bufs in memory. When a buf is removed from memory, the last buf in the array is
moved to the slot it last occupied, keeping the array dense. Each buf carries an index which is its
position in the dense random array; this allows all insertion, deletion, and random selection to be
done in constant time.
*/

class mc_cache_t;

struct evictable_t {
    /* When bufs are created or destroyed, this constructor and destructor are called; this is how
     * the page replacement system keeps track of the buffers in memory. */
    explicit evictable_t(mc_cache_t *cache);
    virtual ~evictable_t();
    virtual bool safe_to_unload() = 0; // Must be implemented by subclass.
    //evictable_t *get_next_buf();     // commented out because not actually used.
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
    void make_space(unsigned int space_needed);
    
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
    two_level_array_t<evictable_t*, MAX_BLOCKS_IN_MEMORY> array;
};

#endif // __PAGE_REPL_RANDOM_HPP__
