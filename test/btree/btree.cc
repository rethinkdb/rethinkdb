
#include <retest.hpp>
#include "btree/get_fsm.hpp"
#include "btree/set_fsm.hpp"
#include "btree_helper.hpp"

void do_test_lookup_api(cache_t &cache) {
    get_fsm_t::op_result_t res = lookup(&cache, 1);
    assert_eq(res, get_fsm_t::btree_not_found);
}

void do_test_insert_api(cache_t &cache) {
    bool complete = insert(&cache, 1, 1);
    assert(complete);
}

void do_test_small_insert(cache_t &cache) {
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

void do_test_multinode_insert(cache_t &cache) {
    // Insert a node full of items, plus one extra
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

void do_test_large_insert(cache_t &cache) {
    const int lots_of_items = 1000000;
    
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

void do_test_large_insert_permuted(cache_t &cache) {
    const int lots_of_items = 1000000;

    // Permute an array of numbers
    std::vector<int> numbers(lots_of_items);
    for(int i = 0; i < lots_of_items; i++) {
        numbers[i] = i;
    }
    std::random_shuffle(numbers.begin(), numbers.end());

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

// Actual tests
void test_lookup_api_immediate() {
    cache_t cache(BTREE_BLOCK_SIZE, rc_immediate);
    do_test_lookup_api(cache);
}

void test_lookup_api_delayed() {
    cache_t cache(BTREE_BLOCK_SIZE, rc_delayed);
    do_test_lookup_api(cache);
}

void test_insert_api_immediate() {
    cache_t cache(BTREE_BLOCK_SIZE, rc_immediate);
    do_test_insert_api(cache);
}

void test_insert_api_delayed() {
    cache_t cache(BTREE_BLOCK_SIZE, rc_delayed);
    do_test_insert_api(cache);
}

void test_small_insert_immediate() {
    cache_t cache(BTREE_BLOCK_SIZE, rc_immediate);
    do_test_small_insert(cache);
}

void test_small_insert_delayed() {
    cache_t cache(BTREE_BLOCK_SIZE, rc_delayed);
    do_test_small_insert(cache);
}

void test_multinode_insert_immediate() {
    cache_t cache(BTREE_BLOCK_SIZE, rc_immediate);
    do_test_multinode_insert(cache);
}

void test_multinode_insert_delayed() {
    cache_t cache(BTREE_BLOCK_SIZE, rc_delayed);
    do_test_multinode_insert(cache);
}

void test_large_insert_immediate() {
    cache_t cache(BTREE_BLOCK_SIZE, rc_immediate);
    do_test_large_insert(cache);
}
    
void test_large_insert_delayed() {
    cache_t cache(BTREE_BLOCK_SIZE, rc_delayed);
    do_test_large_insert(cache);
}

void test_large_insert_permuted_immediate() {
    cache_t cache(BTREE_BLOCK_SIZE, rc_immediate);
    do_test_large_insert_permuted(cache);
}

void test_large_insert_permuted_delayed() {
    cache_t cache(BTREE_BLOCK_SIZE, rc_delayed);
    do_test_large_insert_permuted(cache);
}

void test_large_insert_permuted_mixed() {
    cache_t cache(BTREE_BLOCK_SIZE, rc_mixed);
    do_test_large_insert_permuted(cache);
}

