
#ifndef __BTREE_HELPER_HPP__
#define __BTREE_HELPER_HPP__

#include <set>
#include <vector>
#include <algorithm>
#include "config/args.hpp"
#include "alloc/malloc.hpp"
#include "btree/array_node.hpp"
#include "buffer_cache/volatile.hpp"
#include "buffer_cache/stats.hpp"
#include "event.hpp"

using namespace std;

// Forward declarations
struct mock_config_t;

// The recording cache emulates a real cache with various IO modes
// (returns blocks immediately as if they were always in ram, always
// delays as if blocks were never in ram until requested, and mixed
// mode which chooses to return a block or delay randomly).
enum aio_mode_t {
    rc_immediate,
    rc_delayed,
    rc_mixed
};

template <class config_t>
struct recording_cache_t : public cache_stats_t<volatile_cache_t<config_t> > {
public:
    typedef cache_stats_t<volatile_cache_t<config_t> > vcache_t;
    typedef typename vcache_t::block_id_t block_id_t;
    typedef typename vcache_t::transaction_t transaction_t;

public:
    recording_cache_t(size_t _block_size, aio_mode_t _aio_mode)
        : vcache_t(_block_size, 0), aio_mode(_aio_mode), read_event_exists(false) {}

    void* acquire(transaction_t *tm, block_id_t block_id, void *state) {
        aio_mode_t mode = aio_mode;
        if(aio_mode == rc_mixed)
            mode = rand() % 2 ? rc_immediate : rc_delayed;

        if(mode == rc_immediate) {
            return vcache_t::acquire(tm, block_id, state);
        } else if(mode == rc_delayed) {
            // Store the read event
            read_event_exists = true;
            read_event.event_type = et_cache;
            read_event.result = BTREE_BLOCK_SIZE;
            read_event.op = eo_read;
            read_event.offset = (off64_t)block_id;
            read_event.buf = vcache_t::acquire(tm, block_id, state);
                
            return NULL;
        }
        
        assert(0);
    }
    
    block_id_t release(transaction_t *tm, block_id_t block_id, void *block, bool dirty, void *state) {
        if(dirty) {
            // Record the write
            event_t e;
            e.event_type = et_cache;
            e.result = BTREE_BLOCK_SIZE;
            e.op = eo_write;
            e.offset = (off64_t)block_id;
            e.buf = block;
            record.push_back(e);
        }
        return vcache_t::release(tm, block_id, block, dirty, state);
    }

    bool get_read_event(event_t *_read_event) {
        if(read_event_exists) {
            read_event_exists = false;
            *_read_event = read_event;
            return true;
        } else
            return false;
    }

public:
    bool read_event_exists;
    event_t read_event;
    vector<event_t> record;
    aio_mode_t aio_mode;
};

// Mock config
struct mock_config_t {
    typedef buffer_t<IO_BUFFER_SIZE> iobuf_t;
    typedef malloc_alloc_t alloc_t;

    // BTree
    typedef btree_fsm<mock_config_t> btree_fsm_t;

    // Caching
    typedef recording_cache_t<mock_config_t> cache_t;

    // BTree
    typedef array_node_t<cache_t::block_id_t> node_t;
    typedef btree_get_fsm<mock_config_t> btree_get_fsm_t;
    typedef btree_set_fsm<mock_config_t> btree_set_fsm_t;

    // Request
    typedef request<mock_config_t> request_t;
};

typedef mock_config_t::cache_t cache_t;
typedef mock_config_t::btree_fsm_t btree_fsm_t;
typedef mock_config_t::btree_get_fsm_t get_fsm_t;
typedef mock_config_t::btree_set_fsm_t set_fsm_t;

// Helpers
get_fsm_t::op_result_t lookup(cache_t *cache, int k, int expected = -1) {
    // Initialize get operation state machine
    get_fsm_t tree(cache);

    // Perform lookup
    tree.init_lookup(k);
    btree_fsm_t::transition_result_t res;
    
    if(cache->aio_mode == rc_immediate) {
        res = tree.do_transition(NULL);
    } else {
        res = tree.do_transition(NULL);
        event_t read_event;
        while(cache->get_read_event(&read_event)) {
            assert_ne(res, btree_fsm_t::transition_complete);
            res = tree.do_transition(&read_event);
        }
    }
    
    // Ensure transaction is complete
    assert_eq(res, btree_fsm_t::transition_complete);
    if(tree.op_result == get_fsm_t::btree_found)
        assert_eq(tree.value, expected);

    return tree.op_result;
}

btree_fsm_t::transition_result_t writes_notify(btree_fsm_t *btree, cache_t *cache) {
    btree_fsm_t::transition_result_t res;
    for(vector<event_t>::iterator i = cache->record.begin(); i != cache->record.end(); i++) {
        res = btree->do_transition(&(*i));
    }
    cache->record.clear();
    return res;
}

bool insert(cache_t *cache, int k, int v) {
    // Initialize set operation state machine
    set_fsm_t tree(cache);

    // Perform update
    tree.init_update(k, v);
    btree_fsm_t::transition_result_t res;
    
    if(cache->aio_mode == rc_immediate) {
        res = tree.do_transition(NULL);
    } else {
        res = tree.do_transition(NULL);
        event_t read_event;
        while(cache->get_read_event(&read_event)) {
            assert_ne(res, btree_fsm_t::transition_complete);
            res = tree.do_transition(&cache->read_event);
        }
    }

    // Notify the btree fsm of writes
    res = writes_notify(&tree, cache);

    return res == btree_fsm_t::transition_complete;
}

#endif // __BTREE_HELPER_HPP__

