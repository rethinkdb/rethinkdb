
#ifndef __PAGE_REPL_RANDOM_HPP__
#define __PAGE_REPL_RANDOM_HPP__

#include "config/args.hpp"

// TODO: We should use mlock (or mlockall or related) to make sure the
// OS doesn't swap out our pages, since we're doing swapping
// ourselves.

// TODO: we might want to decouple selection of pages to free from the
// actual cleaning process (e.g. we might have random vs. lru
// selection strategies, and immediate vs. proactive cleaing
// strategy).

// These two parameters may or may not be the same as the ones in page_map/array.hpp.
#define PAGE_REPL_BUFFERS_PER_CHUNK (1 << 16)
#define PAGE_REPL_MAX_SANE_MEMORY_SIZE (500L * (1 << 30))

template <class config_t>
struct page_repl_random_t {

public:
	typedef typename config_t::cache_t cache_t;
    typedef typename config_t::buf_t buf_t;
    
public:
    page_repl_random_t(unsigned int _unload_threshold,
                       cache_t *_cache)
        : unload_threshold(_unload_threshold),
          cache(_cache)
        {}
    
    class local_buf_t {
        friend class page_repl_random_t;
        
    public:
        /* When bufs are created or destroyed, this constructor and destructor are called; this is
        how the page replacement system keeps track of the buffers in memory. */
        local_buf_t(buf_t *gbuf) : gbuf(gbuf) {
            index = gbuf->cache->page_repl_array.size();
            gbuf->cache->page_repl_array.set(index, gbuf);
        }
        ~local_buf_t() {
            unsigned int last_index = gbuf->cache->page_repl_array.size() - 1;
            if (index == last_index) {
                gbuf->cache->page_repl_array.set(index, NULL);
            } else {
                buf_t *replacement = gbuf->cache->page_repl_array.get(last_index);
                replacement->page_repl_buf.index = index;
                gbuf->cache->page_repl_array.set(index, replacement);
                gbuf->cache->page_repl_array.set(last_index, NULL);
                index = -1;
            }
        }
    
    private:
        buf_t *gbuf;
    
    protected:
        unsigned int index;
    };
    friend class local_buf_t;
    
    // make_space tries to make sure that the number of blocks currently in memory is at least
    // 'space_needed' less than the user-specified memory limit.
    void make_space(unsigned int space_needed) {
        
        unsigned int target;
        if (space_needed > unload_threshold) target = unload_threshold;
        else target = unload_threshold - space_needed;
                
        while (page_repl_array.size() > target) {
                    
            // Try to find a block we can unload. Blocks are ineligible to be unloaded if they are
            // dirty or in use.
            buf_t *block_to_unload = NULL;
            for (int tries = PAGE_REPL_NUM_TRIES; tries > 0; tries --) {
                
                /* Choose a block in memory at random. */
                unsigned int n = random() % page_repl_array.size();
                buf_t *block = page_repl_array.get(n);
                
                if (block->safe_to_unload()) {
                    block_to_unload = block;
                    break;
                }
            }
            
            if (block_to_unload) {
                cache->do_unload_buf(block_to_unload);
            } else {
#ifndef NDEBUG
                printf("thread %d exceeding memory target. %d blocks in memory, %d dirty, target is %d.\n",
                    get_cpu_context()->event_queue->queue_id, page_repl_array.size(), cache->num_dirty_blocks(), target);
#endif
                break;
            }
        }
    }
    
    /* The page replacement component actually serves two roles. In addition to its primary role as
    a mechanism for kicking out buffers when memory runs low, it also has the job of keeping track
    of all of the buffers in memory in such a way that the cache can quickly request a pointer to
    the next buffer in memory. This is used during the cache's destructor. The rationale is that any
    reasonable implementation of a page replacement system will need to keep track of all of the
    buffers in memory anyway, so the cache can depend on the page replacement system's buffer list
    rather than keeping a buffer list of its own. */
    
    buf_t *get_first_buf() {
        if (page_repl_array.size() == 0) return NULL;
        return page_repl_array.get(0);
    }
    
    buf_t *get_next_buf(buf_t *buf) {
        if (buf->page_repl_buf.index == page_repl_array.size() - 1) return NULL;
        else return page_repl_array.get(buf->page_repl_buf.index + 1);
    }
    
private:
    unsigned int unload_threshold;
    cache_t *cache;

protected:
    two_level_array_t<buf_t, PAGE_REPL_MAX_SANE_MEMORY_SIZE / BTREE_BLOCK_SIZE, PAGE_REPL_BUFFERS_PER_CHUNK> page_repl_array;
};

#endif // __PAGE_REPL_RANDOM_HPP__
