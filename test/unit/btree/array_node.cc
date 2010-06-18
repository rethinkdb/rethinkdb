
#include <retest.hpp>
#include "btree/array_node.hpp"

typedef void* block_id_t;
typedef array_leaf_node_t<block_id_t> test_leaf_node_t;
typedef array_internal_node_t<block_id_t> test_internal_node_t;

/**
 * Testing leaf node
 */
void test_leaf_basic() {
    int value;
    test_leaf_node_t node;
    for(int i = 0; i < NODE_ORDER; i++) {
        assert_cond(node.insert(i, i));
    }
    for(int i = 0; i < NODE_ORDER; i++) {
        assert_cond(node.lookup(i, &value));
        assert_eq(i, value);
    }
    assert_cond(!node.insert(NODE_ORDER, NODE_ORDER));
    assert_cond(!node.lookup(NODE_ORDER, &value));
}

void test_leaf_middle() {
    int value;
    test_leaf_node_t node;
    // Insert items except one in the middle
    for(int i = 0; i < NODE_ORDER; i++) {
        if(i != NODE_ORDER / 2) {
            assert_cond(node.insert(i, i));
        }
    }
    // Look them up
    for(int i = 0; i < NODE_ORDER; i++) {
        if(i != NODE_ORDER / 2) {
            assert_cond(node.lookup(i, &value));
            assert_eq(i, value);
        } else {
            assert_cond(!node.lookup(i, &value));
        }
    }
    // Insert the middle item
    assert_cond(node.insert(NODE_ORDER / 2, NODE_ORDER / 2));
    
    // Look everything up again
    for(int i = 0; i < NODE_ORDER; i++) {
        assert_cond(node.lookup(i, &value));
        assert_eq(i, value);
    }
}

void test_leaf_basic_reverse() {
    int value;
    test_leaf_node_t node;
    for(int i = 0; i < NODE_ORDER; i++) {
        int j = NODE_ORDER - i;
        assert_cond(node.insert(j, j));
    }
    for(int i = 0; i < NODE_ORDER; i++) {
        int j = NODE_ORDER - i;
        assert_cond(node.lookup(j, &value));
        assert_eq(j, value);
    }
    assert_cond(!node.insert(0, 0));
    assert_cond(!node.lookup(0, &value));
}

void test_leaf_negative() {
    int value;
    test_leaf_node_t node;
    for(int i = 0; i < NODE_ORDER; i++) {
        assert_cond(node.insert(i, i));
    }
    assert_cond(!node.lookup(NODE_ORDER, &value));
}

void test_leaf_split() {
    // Fill up the node
    test_leaf_node_t node;
    for(int i = 0; i < NODE_ORDER; i++) {
        assert_cond(node.insert(i, i));
    }
    int median, value;
    test_leaf_node_t
        *lnode = &node,
        *rnode = new test_leaf_node_t();

    // Split it
    node.split(rnode, &median);

    // Make sure median is right
    assert_eq(median, NODE_ORDER / 2);

    // Check the left node
    for(int i = 0; i < median; i++) {
        assert_cond(((test_leaf_node_t*)lnode)->lookup(i, &value));
        assert_eq(i, value);
    }
    assert_cond(!((test_leaf_node_t*)lnode)->lookup(median, &value));
    
    // Check the right node
    for(int i = median; i < NODE_ORDER; i++) {
        assert_cond(((test_leaf_node_t*)rnode)->lookup(i, &value));
        assert_eq(i, value);
    }
    assert_cond(!((test_leaf_node_t*)lnode)->lookup(NODE_ORDER, &value));
}

/**
 * Testing internal node
 */
void test_internal_basic() {
    test_internal_node_t node;

    for(int i = 0; i < NODE_ORDER; i++) {
        assert_cond(node.insert(i * 2, (block_id_t)(i * 2), (block_id_t)(i * 2 + 1)));
    }
    assert_cond(!node.insert(0, 0, 0));
    for(int i = -1; i < NODE_ORDER * 2; i++) {
        int val = (long)node.lookup(i);
        if(i < 0) {
            assert_eq(val, 0);
        } else if(i >= (NODE_ORDER - 1) * 2) {
            assert_eq(val, NODE_ORDER * 2 - 1);
        } else if(i % 2 == 0) {
            assert_eq(val, i + 2);
        } else {
            assert_eq(val, i + 1);
        }
    }
}

void test_internal_reverse() {
    test_internal_node_t node;

    for(int i = 0; i < NODE_ORDER; i++) {
        int j = NODE_ORDER - i - 1;
        assert_cond(node.insert(j * 2, (block_id_t)(j * 2), (block_id_t)(j * 2 + 1)));
    }
    assert_cond(!node.insert(0, 0, 0));
    for(int i = -1; i < NODE_ORDER * 2; i++) {
        int val = (long)node.lookup(i);
        if(i < 0) {
            assert_eq(val, 0);
        } else if(i >= (NODE_ORDER - 1) * 2) {
            assert_eq(val, NODE_ORDER * 2 - 1);
        } else if(i % 2 == 0) {
            assert_eq(val, i + 1);
        } else {
            assert_eq(val, i);
        }
    }
}

void test_internal_middle() {
    test_internal_node_t node;

    // Insert all but one
    for(int i = 0; i < NODE_ORDER; i++) {
        if(i != NODE_ORDER / 2) {
            assert_cond(node.insert(i * 2, (block_id_t)(i * 2), (block_id_t)(i * 2 + 1)));
        }
    }

    // Add the element in the middle
    int i = NODE_ORDER / 2;
    assert_cond(node.insert(i * 2, (block_id_t)(i * 2), (block_id_t)(i * 2 + 1)));

    // Check elements
    for(int i = -1; i < NODE_ORDER * 2; i++) {
        if(i < NODE_ORDER && i >= NODE_ORDER + 2) {
            int val = (long)node.lookup(i);
            if(i < 0) {
                assert_eq(val, 0);
            } else if(i >= (NODE_ORDER - 1) * 2) {
                assert_eq(val, NODE_ORDER * 2 - 1);
            } else if(i % 2 == 0) {
                assert_eq(val, i + 2);
            } else {
                assert_eq(val, i + 1);
            }
        }
    }
    
    // Test special cases
    int val = (long)node.lookup(NODE_ORDER);
    assert_eq(val, NODE_ORDER + 1);
    val = (long)node.lookup(NODE_ORDER + 1);
    assert_eq(val, NODE_ORDER + 1);
}

void test_internal_split() {
    test_internal_node_t node;
    // Insert elements
    for(int i = 0; i < NODE_ORDER; i++) {
        assert_cond(node.insert(i * 2, (block_id_t)(i * 2), (block_id_t)(i * 2 + 1)));
    }
    
    // Split the node
    int median;
    test_internal_node_t
        *lnode = &node,
        *rnode = new test_internal_node_t();
    node.split(rnode, &median);

    // Check the left node
    for(int i = -1; i < NODE_ORDER; i++) {
        int val = (long)lnode->lookup(i);
        if(i < 0) {
            assert_eq(val, 0);
        } else if(i % 2 == 0) {
            assert_eq(val, i + 2);
        } else {
            assert_eq(val, i + 1);
        }
    }
    assert_eq((long)(lnode->lookup(NODE_ORDER)), NODE_ORDER);

    // Check the right node
    for(int i = NODE_ORDER; i < NODE_ORDER * 2; i++) {
        int val = (long)rnode->lookup(i);
        if(i < NODE_ORDER + 2) {
            assert_eq(val, NODE_ORDER + 2);
        } else if(i >= NODE_ORDER * 2 - 2) {
            assert_eq(val, NODE_ORDER * 2 - 1);
        } else if(i % 2 == 0) {
            assert_eq(val, i + 2);
        } else {
            assert_eq(val, i + 1);
        }
    }
}

