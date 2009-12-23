
#include <vector>
#include <algorithm>
#include <retest.hpp>
#include "btree/btree.hpp"
#include "btree/array_node.hpp"
#include "buffer_cache/volatile.hpp"
#include "buffer_cache/cache_stats.hpp"

#define NODE_SIZE 1024

typedef cache_stats_t<volatile_cache_t> test_cache_t;

void test_lookup_api() {
    btree<array_node_t, test_cache_t> tree(NODE_SIZE);
    int value;
    assert_eq(tree.lookup(1, &value), 0);
}

void test_insert_api() {
    btree<array_node_t, test_cache_t> tree(NODE_SIZE);
    tree.insert(1, 1);
}

void test_small_insert() {
    btree<array_node_t, test_cache_t> tree(NODE_SIZE);

    // Insert a node full of items
    for(int i = 0; i < NODE_ORDER; i++) {
        tree.insert(i, i);
    }

    // Make sure the items are there
    int value;
    for(int i = 0; i < NODE_ORDER; i++) {
        assert_cond(tree.lookup(i, &value));
        assert_eq(value, i);
    }
}

void test_multinode_insert() {
    btree<array_node_t, test_cache_t> tree(NODE_SIZE);

    // Insert a node full of items, plus one more
    for(int i = 0; i < NODE_ORDER + 1; i++) {
        tree.insert(i, i);
    }

    // Make sure the items are there
    int value;
    for(int i = 0; i < NODE_ORDER + 1; i++) {
        assert_cond(tree.lookup(i, &value));
        assert_eq(value, i);
    }
}

void test_large_insert() {
    const int lots_of_items = 1000000;
    
    btree<array_node_t, test_cache_t> tree(NODE_SIZE);

    for(int i = 0; i < lots_of_items; i++) {
        tree.insert(i, i);
    }
    for(int i = 0; i < lots_of_items; i++) {
        int value;
        assert_cond(tree.lookup(i, &value));
        assert_eq(i, value);
    }
}

void test_large_insert_permuted() {
    const int lots_of_items = 1000000;

    // Permute an array of numbers
    std::vector<int> numbers(lots_of_items);
    for(int i = 0; i < lots_of_items; i++) {
        numbers[i] = i;
    }
    std::random_shuffle(numbers.begin(), numbers.end());

    // Insert the permuted array into a btree
    btree<array_node_t, test_cache_t> tree(NODE_SIZE);
    for(int i = 0; i < lots_of_items; i++) {
        tree.insert(i, numbers[i]);
    }

    // Look the numbers up
    int value;
    for(int i = 0; i < lots_of_items; i++) {
        assert_cond(tree.lookup(i, &value));
        assert_eq(numbers[i], value);
    }

    // Perform a negative test
    assert_cond(!tree.lookup(lots_of_items, &value));
}

