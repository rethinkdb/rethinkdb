
#ifndef __BTREE_HPP__
#define __BTREE_HPP__

#include <assert.h>

// TODO: mapping only int->int, allow arbitrary key and value types
// TODO: ignoring duplicates, allow duplicate elements
// TODO: stays only in memory, add disk support
// TODO: not thread safe, implement concurrency control methods
// TODO: develop a memory allocator
// TODO: develop a page cache
// TODO: perhaps allow memory reclamation due to oversplitting?
// TODO: multiple values require cursor/iterator mechanism
// TODO: because of AIO we need a state machine
// TODO: consider redoing with a visitor pattern to avoid ugly casts
// TODO: consider B#/B* trees to improve space efficiency

template <class node_t>
class btree {
public:
    btree() {
        // TODO: We need a good allocator here
        root = new leaf_node_t();
    }

    int lookup(int key, int *value) {
        node_t *node = root;
        while(node->is_internal()) {
            node = ((internal_node_t*)node)->lookup(key);
            assert(node);
        }
        return ((leaf_node_t*)node)->lookup(key, value);
    }
    
    void insert(int key, int value) {
        node_t *node = root;
        internal_node_t *last_node = NULL;
        do {
            if(node->is_full()) {
                int median;
                node_t *lnode, *rnode;
                split_node(node, &lnode, &rnode, &median);
                if(last_node == NULL) {
                    // TODO: We need a good allocator here
                    root = new internal_node_t();
                    ((internal_node_t*)root)->insert(median, lnode, rnode);
                } else {
                    last_node->insert(median, lnode, rnode);
                }
                if(key < median) {
                    node = lnode;
                } else if(key > median) {
                    node = rnode;
                } else {
                    // TODO: what to do here?
                }
            }
            if(node->is_leaf()) {
                ((leaf_node_t*)node)->insert(key, value);
                break;
            } else {
                last_node = (internal_node_t*)node;
                node = ((internal_node_t*)node)->lookup(key);
            }
        } while(1);
    }

    typedef typename node_t::leaf_node_t leaf_node_t;
    typedef typename node_t::internal_node_t internal_node_t;

private:
    void split_node(node_t *node, node_t **lnode, node_t **rnode, int *median) {
        if(node->is_leaf()) {
            ((leaf_node_t*)node)->split((leaf_node_t**)lnode,
                                        (leaf_node_t**)rnode,
                                        median);
        } else {
            ((internal_node_t*)node)->split((internal_node_t**)lnode,
                                            (internal_node_t**)rnode,
                                            median);
        }
    }
    
private:
    node_t *root;
};

#endif // __BTREE_HPP__

