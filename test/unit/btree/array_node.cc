
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

void test_leaf_remove() {
    int value;
    test_leaf_node_t node;
    for(int i = 0; i < NODE_ORDER; i++) {
        assert_cond(node.insert(i,i));
    }
    for(int i = 0; i < NODE_ORDER; i++) {
        assert_cond(node.remove(i));
    }
    assert_cond(node.is_empty());
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

void test_leaf_level() {
    block_id_t meptr = (block_id_t) 0, siblingptr = (block_id_t) 1, otherptr = (block_id_t) 2;
    {
        test_internal_node_t parent;
        test_leaf_node_t me, sibling;

        for (int i = 0; i < (NODE_ORDER / 2) - 1; i++) {
            assert_cond(me.insert(i, i));
        }
        for (int i = 0; i < NODE_ORDER; i++) {
            assert_cond(sibling.insert(i + NODE_ORDER, i + NODE_ORDER));
        }

        assert_cond(parent.insert(NODE_ORDER - 1, meptr, siblingptr));
        assert_cond(parent.insert(NODE_ORDER * 2, siblingptr, otherptr));

        assert_cond(me.is_underfull());
        assert_cond(!sibling.is_underfull());

        me.level(&parent, &sibling);
        assert_cond(!me.is_underfull());
        assert_cond(!sibling.is_underfull());
        
        //test to make sure the values are in the correct nodes
        for(int i = 0; i < (NODE_ORDER / 2) - 1; i++) {
            assert_eq(me.lookup_key(i), i);
        }
        for(int i = NODE_ORDER; i < NODE_ORDER + (NODE_ORDER / 4); i++) {
            assert_eq(me.lookup_key(i), i);
        }

        for(int i = NODE_ORDER + (NODE_ORDER / 4); i < NODE_ORDER + (3 * (NODE_ORDER / 4)); i++) {
            assert_eq(sibling.lookup_key(i), i);
        }

        for(int i = 0; i < 3 * NODE_ORDER; i++) {
            if (i < (5 * (NODE_ORDER / 4)))
                assert_eq((void *)parent.lookup(i), meptr);
            else if (i < (2 * NODE_ORDER))
                assert_eq((void *)parent.lookup(i), siblingptr);
            else
                assert_eq((void *)parent.lookup(i), otherptr);
        }
    }
    //now try with a sibling that's smaller than us
    {
        test_internal_node_t parent;
        test_leaf_node_t me, sibling;

        for (int i = 0; i < (NODE_ORDER / 2) - 1; i++) {
            assert_cond(me.insert(i + NODE_ORDER, i + NODE_ORDER));
        }
        for (int i = 0; i < NODE_ORDER; i++) {
            assert_cond(sibling.insert(i, i));
        }

        assert_cond(parent.insert(NODE_ORDER, siblingptr, meptr));
        assert_cond(parent.insert(NODE_ORDER * 2, meptr, otherptr));

        assert_cond(me.is_underfull());
        assert_cond(!sibling.is_underfull());

        me.level(&parent, &sibling);
        assert_cond(!me.is_underfull());
        assert_cond(!sibling.is_underfull());

        //test to make sure the values are in the correct nodes
        //XXX these tests were written with NODE_ORDER = 16, they may break with other node orders
        for(int i = (3 * NODE_ORDER) / 4; i < NODE_ORDER + (NODE_ORDER / 2) - 1; i++) {
            assert_eq(me.lookup_key(i), i);
        }
        for(int i = 0; i < (3 * NODE_ORDER) / 4; i++) {
            assert_eq(sibling.lookup_key(i), i);
        }

        for(int i = 0; i < 3 * NODE_ORDER; i++) {
            if (i < (3 * (NODE_ORDER / 4)))
                assert_eq((void *)parent.lookup(i), siblingptr);
            else if (i < (2 * NODE_ORDER))
                assert_eq((void *)parent.lookup(i), meptr);
            else
                assert_eq((void *)parent.lookup(i), otherptr);
        }

    }
}

/**
 * Testing internal node
 */
void test_internal_basic() {
    test_internal_node_t node;

    for(int i = 0; i < NODE_ORDER; i++) {
        assert_cond(node.insert(key(i * 2), (block_id_t)(i * 2), (block_id_t)(i * 2 + 1)));
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

void test_internal_remove() {
    test_internal_node_t node;
    for(int i = 0; i < NODE_ORDER; i++) {
        assert_cond(node.insert(i * 2, (block_id_t)(i * 2), (block_id_t)(i * 2 + 1)));
    }

    for(int i = 0; i < NODE_ORDER; i++) {
        assert_cond(node.remove(i * 2 - 1, i * 2 + 1));
    }

    assert_cond(node.is_empty());

    node.insert(10, (void *) 0, (void *) 1);
    node.insert(15, (void *) 1, (void *) 2);

    for (int i = 0; i < 20; i++) {
        if (i < 10)
            assert_eq(node.lookup(i), (void *) 0);
        else if (i < 15)
            assert_eq(node.lookup(i), (void *) 1);
        else
            assert_eq(node.lookup(i), (void *) 2);
    }

    assert_cond(node.remove(9, 11));

    for (int i = 0; i < 20; i++) {
        if (i < 15)
            assert_eq(node.lookup(i), (void *) 0);
        else
            assert_eq(node.lookup(i), (void *) 2);
    }
}

void test_internal_level() {
    {
        test_internal_node_t parent, me, sibling;
        block_id_t meptr = (block_id_t) 0, siblingptr = (block_id_t) 1, otherptr = (block_id_t) 2;

        for(int i = 0; i < (NODE_ORDER / 2) - 1; i++)
            assert_cond(me.insert(i, (block_id_t) i, (block_id_t) (i + 1)));

        for (int i = 0; i < NODE_ORDER; i++)
            assert_cond(sibling.insert(i + NODE_ORDER, (block_id_t) (i + NODE_ORDER), (block_id_t) (i + NODE_ORDER + 1)));

        assert_cond(parent.insert((NODE_ORDER / 2), meptr, siblingptr));
        assert_cond(parent.insert((2 * NODE_ORDER), siblingptr, otherptr));    

        me.level(&parent, &sibling);

        assert_cond(!me.is_underfull());
        assert_cond(!sibling.is_underfull());
        
        for(int i = -1; i < (NODE_ORDER / 2) - 1; i++)
            assert_eq((block_id_t) me.lookup(i), (block_id_t) (i + 1));

        assert_eq((block_id_t) me.lookup_key(NODE_ORDER / 2), (block_id_t) (NODE_ORDER / 2));

        for(int i = NODE_ORDER; i < NODE_ORDER + (((NODE_ORDER / 2) + 1) / 2) - 1; i++)
            assert_eq((block_id_t) me.lookup(i), (block_id_t) (i + 1));
        for(int i = ((5 * NODE_ORDER) / 4) - 1; i < 2 * NODE_ORDER; i++)
            assert_eq((block_id_t) sibling.lookup(i), (block_id_t) (i + 1));
        for(int i = 0; i < 3 * NODE_ORDER; i++) {
            if (i < ((5 * NODE_ORDER) / 4) - 1)
                assert_eq(parent.lookup(i), meptr);
            else if (i < (2 * NODE_ORDER))
                assert_eq(parent.lookup(i), siblingptr);
            else
                assert_eq(parent.lookup(i), otherptr);
        }
    }
    //now test when the sibling is smaller than us
    {
        test_internal_node_t parent, me, sibling;
        block_id_t meptr = (block_id_t) 0, siblingptr = (block_id_t) 1, otherptr = (block_id_t) 2;

        for(int i = 0; i < (NODE_ORDER / 2) - 2; i++)
            assert_cond(me.insert(i + NODE_ORDER + 2, (block_id_t) (i + NODE_ORDER + 2), (block_id_t) (i + NODE_ORDER + 3)));

        for (int i = 0; i < NODE_ORDER; i++)
            assert_cond(sibling.insert(i, (block_id_t) i, (block_id_t) (i + 1)));

        assert_cond(parent.insert(NODE_ORDER + 1, siblingptr, meptr));
        assert_cond(parent.insert((2 * NODE_ORDER), meptr, otherptr));    

        me.level(&parent, &sibling);

        assert_cond(!me.is_underfull());
        assert_cond(!sibling.is_underfull());
    }
}

void test_leaf_merge() {
    block_id_t meptr = (block_id_t) 0, siblingptr = (block_id_t) 1, otherptr = (block_id_t) 2;
    {
        test_internal_node_t parent;
        test_leaf_node_t me, sibling;

        for (int i = 0; i < NODE_ORDER - 2; i++) {
            if (i < (NODE_ORDER - 2) / 2)
                me.insert(i, i);
            else
                sibling.insert(i, i);
        }
        parent.insert(((NODE_ORDER - 2) / 2), meptr, siblingptr);
        parent.insert((NODE_ORDER), siblingptr, otherptr);        

        me.merge(&parent, &sibling);

        int val;
        for (int i = 0; i < NODE_ORDER - 2; i++) {
            assert_cond(me.lookup(i, &val));
            assert_eq(val, i);
        }

        for (int i = 0; i < 2 * NODE_ORDER; i++) {
            if (i < NODE_ORDER)
                assert_eq(parent.lookup(i), meptr);
            else
                assert_eq(parent.lookup(i), otherptr);
        }
    }
    {
        test_internal_node_t parent;
        test_leaf_node_t me, sibling;

        for (int i = 0; i < NODE_ORDER - 3; i++) {
            if (i < (NODE_ORDER - 2) / 2)
                sibling.insert(i, i);
            else
                me.insert(i, i);
        }
        parent.insert(((NODE_ORDER - 2) / 2), siblingptr, meptr);
        parent.insert((NODE_ORDER), meptr, otherptr);        

        me.merge(&parent, &sibling);

        int val;
        for (int i = 0; i < NODE_ORDER - 3; i++) {
            assert_cond(me.lookup(i, &val));
            assert_eq(val, i);
        }

        for (int i = 0; i < 2 * NODE_ORDER; i++) {
            if (i < NODE_ORDER)
                assert_eq(parent.lookup(i), meptr);
            else
                assert_eq(parent.lookup(i), otherptr);
        }
    }
}

void test_internal_merge() {
    block_id_t meptr = (block_id_t) 0, siblingptr = (block_id_t) 1, otherptr = (block_id_t) 2;
    {
        test_internal_node_t parent, me, sibling;

        for (int i = 0; i < (NODE_ORDER - 2); i++) {
            if (i < (NODE_ORDER - 2) / 2)
                me.insert(i, (block_id_t) (i - 1), (block_id_t) i);
            else
                sibling.insert(i + 1, (block_id_t) (i), (block_id_t) (i + 1));
        }

        parent.insert((NODE_ORDER - 2) / 2, meptr, siblingptr);
        parent.insert(NODE_ORDER, siblingptr, otherptr);

        me.merge(&parent, &sibling);
        
        for (int i = 0; i < NODE_ORDER - 2; i++) {
            assert_eq(me.lookup(i), (block_id_t) i);
        }

        for (int i = 0; i < 2 * NODE_ORDER; i++) {
            if (i < NODE_ORDER)
                assert_eq(parent.lookup(i), meptr);
            else
                assert_eq(parent.lookup(i), otherptr);
        }
    }
    {
        test_internal_node_t parent, me, sibling;

        for (int i = 0; i < (NODE_ORDER - 2); i++) {
            if (i < (NODE_ORDER - 2) / 2)
                sibling.insert(i, (block_id_t) (i - 1), (block_id_t) i);
            else
                me.insert(i + 1, (block_id_t) (i), (block_id_t) (i + 1));
        }

        parent.insert((NODE_ORDER - 2) / 2, siblingptr, meptr);
        parent.insert(NODE_ORDER, meptr, otherptr);

        me.merge(&parent, &sibling);
        
        for (int i = 0; i < NODE_ORDER - 2; i++) {
            assert_eq(me.lookup(i), (block_id_t) i);
        }

        for (int i = 0; i < 2 * NODE_ORDER; i++) {
            if (i < NODE_ORDER)
                assert_eq(parent.lookup(i), meptr);
            else
                assert_eq(parent.lookup(i), otherptr);
        }
    }
}

















