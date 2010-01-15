
#ifndef __CACHE_STATS_HPP__
#define __CACHE_STATS_HPP__

#include <assert.h>
#include <stdio.h>

/* This class wraps around another cache and collects some basic
 * statistics. It is useful to ensure that every acquired block is
 * eventually released. */
template <class parent_cache_t>
struct cache_stats_t : public parent_cache_t {
public:
    typedef typename parent_cache_t::block_id_t block_id_t;

public:
    cache_stats_t(size_t _block_size)
        : parent_cache_t(_block_size), nacquired(0), nreleased(0) {}

    ~cache_stats_t() {
        if(nacquired != nreleased) {
            printf("na: %d, nr: %d\n", nacquired, nreleased);
        }
        assert(nacquired == nreleased);
    }
    
    void* allocate(block_id_t *block_id) {
        nacquired++;
        return parent_cache_t::allocate(block_id);
    }
    
    void* acquire(block_id_t block_id, void *state) {
        nacquired++;
        return parent_cache_t::acquire(block_id, state);
    }

    block_id_t release(block_id_t block_id, void *block, bool dirty, void *state) {
        nreleased++;
        return parent_cache_t::release(block_id, block, dirty, state);
    }

private:
    int nacquired, nreleased;
};

#endif // __CACHE_STATS_HPP__

