
#include <vector>
#include <algorithm>
#include <retest.hpp>
#include "config/args.hpp"
#include "alloc/malloc.hpp"
#include "alloc/object_static.hpp"
#include "btree/array_node.hpp"
#include "btree/get_fsm.hpp"
#include "btree/set_fsm.hpp"
#include "buffer_cache/volatile.hpp"
#include "buffer_cache/stats.hpp"
#include "event.hpp"

using namespace std;

// Forward declarations
struct mock_config_t;

// Mock definitions
template <class config_t>
struct mock_fsm_t {};

// TODO: we're only testing the case where cache reads return
// immediately. We should also test the case where the reads come back
// via AIO. (Note, since we're doing it this way, cache_stats is
// unlikely to fail, though it might fail with AIO)

template <class config_t>
struct recording_cache_t : public volatile_cache_t<config_t> {
public:
    typedef volatile_cache_t<config_t> vcache_t;
    typedef typename vcache_t::block_id_t block_id_t;
    typedef typename vcache_t::fsm_t fsm_t;

public:
    recording_cache_t(size_t _block_size, size_t _max_size)
        : vcache_t(_block_size, _max_size) {}

    block_id_t release(block_id_t block_id, void *block, bool dirty, fsm_t *state) {
        if(dirty) {
            // Record the write
            event_t e;
            e.event_type = et_disk;
            e.result = BTREE_BLOCK_SIZE;
            e.op = eo_write;
            e.offset = (off64_t)block_id;
            e.buf = block;
            record.push_back(e);
        }
        return vcache_t::release(block_id, block, dirty, state);
    }

public:
    vector<event_t> record;
};

// Mock config
struct mock_config_t {
    typedef buffer_t<IO_BUFFER_SIZE> iobuf_t;
    typedef object_static_alloc_t<malloc_alloc_t, iobuf_t> alloc_t;

    // Connection fsm
    typedef mock_fsm_t<mock_config_t> fsm_t;

    // Caching
    typedef cache_stats_t<recording_cache_t<mock_config_t> > cache_t;

    // BTree
    typedef array_node_t<cache_t::block_id_t> node_t;
    typedef btree_fsm<mock_config_t> btree_fsm_t;
    typedef btree_get_fsm<mock_config_t> btree_get_fsm_t;
    typedef btree_set_fsm<mock_config_t> btree_set_fsm_t;
};

typedef mock_config_t::cache_t cache_t;
typedef mock_config_t::btree_fsm_t btree_fsm_t;
typedef mock_config_t::btree_get_fsm_t get_fsm_t;
typedef mock_config_t::btree_set_fsm_t set_fsm_t;

// Helpers
get_fsm_t::op_result_t lookup(cache_t *cache, int k, int expected = -1) {
    // Initialize get operation state machine
    get_fsm_t tree(cache, NULL);

    // Perform lookup
    tree.init_lookup(k);
    btree_fsm_t::transition_result_t res = tree.do_transition(NULL);

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
    set_fsm_t tree(cache, NULL);

    // Perform update
    tree.init_update(k, v);
    btree_fsm_t::transition_result_t res = tree.do_transition(NULL);

    // Ensure we inserted the item successfully
    assert_eq(res, btree_fsm_t::transition_ok);

    // Notify the btree fsm of writes
    res = writes_notify(&tree, cache);

    return res == btree_fsm_t::transition_complete;
}

// Tests

void test_lookup_api() {
    // Initialize underlying cache
    cache_t cache(BTREE_BLOCK_SIZE, 0);

    get_fsm_t::op_result_t res = lookup(&cache, 1);
    assert_eq(res, get_fsm_t::btree_not_found);
}

void test_insert_api() {
    // Initialize underlying cache
    cache_t cache(BTREE_BLOCK_SIZE, 0);
    
    bool complete = insert(&cache, 1, 1);
    assert(complete);
}

void test_small_insert() {
    // Initialize underlying cache
    cache_t cache(BTREE_BLOCK_SIZE, 0);

    // Insert a node full of items
    for(int i = 0; i < NODE_ORDER; i++) {
        bool complete = insert(&cache, i, i);
        assert(complete);
    }

    // Make sure the items are there
    int value;
    for(int i = 0; i < NODE_ORDER; i++) {
        get_fsm_t::op_result_t res = lookup(&cache, i, i);
        assert_eq(res, get_fsm_t::btree_found);
    }

    // Make sure missing items aren't there
    get_fsm_t::op_result_t res = lookup(&cache, NODE_ORDER);
    assert_eq(res, get_fsm_t::btree_not_found);
}

void test_multinode_insert() {
    // Initialize underlying cache
    cache_t cache(BTREE_BLOCK_SIZE, 0);

    // Insert a node full of items
    for(int i = 0; i < NODE_ORDER + 1; i++) {
        bool complete = insert(&cache, i, i);
        assert(complete);
    }

    // Make sure the items are there
    int value;
    for(int i = 0; i < NODE_ORDER + 1; i++) {
        get_fsm_t::op_result_t res = lookup(&cache, i, i);
        assert_eq(res, get_fsm_t::btree_found);
    }

    // Make sure missing items aren't there
    get_fsm_t::op_result_t res = lookup(&cache, NODE_ORDER + 1);
    assert_eq(res, get_fsm_t::btree_not_found);
}

void test_large_insert() {
    const int lots_of_items = 1000000;
    
    // Initialize underlying cache
    cache_t cache(BTREE_BLOCK_SIZE, 0);

    // Insert a node full of items
    for(int i = 0; i < lots_of_items; i++) {
        bool complete = insert(&cache, i, i);
        assert(complete);
    }

    // Make sure the items are there
    int value;
    for(int i = 0; i < lots_of_items; i++) {
        get_fsm_t::op_result_t res = lookup(&cache, i, i);
        assert_eq(res, get_fsm_t::btree_found);
    }

    // Make sure missing items aren't there
    get_fsm_t::op_result_t res = lookup(&cache, lots_of_items + 1);
    assert_eq(res, get_fsm_t::btree_not_found);
}

void test_large_insert_permuted() {
    const int lots_of_items = 1000000;

    // Permute an array of numbers
    std::vector<int> numbers(lots_of_items);
    for(int i = 0; i < lots_of_items; i++) {
        numbers[i] = i;
    }
    std::random_shuffle(numbers.begin(), numbers.end());

    // Initialize underlying cache
    cache_t cache(BTREE_BLOCK_SIZE, 0);

    // Insert a node full of items
    for(int i = 0; i < lots_of_items; i++) {
        bool complete = insert(&cache, numbers[i], numbers[i]);
        assert(complete);
    }

    // Make sure the items are there
    int value;
    for(int i = 0; i < lots_of_items; i++) {
        get_fsm_t::op_result_t res = lookup(&cache, numbers[i], numbers[i]);
        assert_eq(res, get_fsm_t::btree_found);
    }

    // Make sure missing items aren't there
    get_fsm_t::op_result_t res = lookup(&cache, lots_of_items + 1);
    assert_eq(res, get_fsm_t::btree_not_found);
}

