
#ifndef __CACHE_STATS_HPP__
#define __CACHE_STATS_HPP__

#include <assert.h>
#include <stdio.h>

// TODO: Measure all kinds of interesting things (cache faults, etc.)

/* This class wraps around another cache and collects some basic
 * statistics. It is useful to ensure that every acquired block is
 * eventually released. */
template <class parent_cache_t>
struct cache_stats_t : public parent_cache_t {
public:
    typedef typename parent_cache_t::transaction_t transaction_t;

public:
    cache_stats_t(size_t _block_size, size_t _max_size)
        : parent_cache_t(_block_size, _max_size), nacquired(0), nreleased(0) {}

    ~cache_stats_t() {
        if(nacquired != nreleased) {
            printf("na: %d, nr: %d\n", nacquired, nreleased);
        }
        assert(nacquired == nreleased);
    }
    
    void* allocate(transaction_t *tm, block_id_t *block_id) {
        nacquired++;
        return parent_cache_t::allocate(tm, block_id);
    }
    
    void* acquire(transaction_t *tm, block_id_t block_id, void *state) {
        nacquired++;
        return parent_cache_t::acquire(tm, block_id, state);
    }

    block_id_t release(block_id_t block_id) {
        nreleased++;
        return parent_cache_t::release(block_id);
    }

protected:
    int nacquired, nreleased;
};

#endif // __CACHE_STATS_HPP__

