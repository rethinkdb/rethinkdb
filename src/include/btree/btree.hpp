
#ifndef __BTREE_HPP__
#define __BTREE_HPP__

#include <assert.h>

// TODO: support AIO via a state machine
// TODO: mapping only int->int, allow arbitrary key and value types
// TODO: ignoring duplicates, allow duplicate elements
// TODO: not thread safe, implement concurrency control methods
// TODO: perhaps allow memory reclamation due to oversplitting?
// TODO: multiple values require cursor/iterator mechanism
// TODO: consider redoing with a visitor pattern to avoid ugly casts
// TODO: consider B#/B* trees to improve space efficiency

template <class node_t, class cache_t>
class btree : public cache_t {
public:
    btree(size_t _block_size) : cache_t(_block_size), root_id(NULL) {}

    int lookup(int key, int *value) {
        if(root_id == NULL)
            return 0;
        // TODO: handle async IO/state
        block_id_t node_id = root_id;
        node_t *node = (node_t*)acquire(node_id, NULL);
        while(node->is_internal()) {
            block_id_t next_node_id = ((internal_node_t*)node)->lookup(key);
            release(node_id, (void*)node, false, NULL);
            node_id = next_node_id;
            node = (node_t*)acquire(node_id, NULL);
        }
        int result = ((leaf_node_t*)node)->lookup(key, value);
        release(node_id, (void*)node, false, NULL);
        return result;
    }
    
    void insert(int key, int value) {
        // TODO: handle async IO/state
        node_t *node;
        block_id_t node_id = root_id;
        bool node_dirty = false;

        internal_node_t *last_node = NULL;
        block_id_t last_node_id = NULL;
        bool last_node_dirty = false;

        // Grab the root
        if(node_id == NULL) {
            void *ptr = allocate(&root_id);
            node = new (ptr) leaf_node_t();
            node_dirty = true;
            node_id = root_id;
        } else {
            node = (node_t*)acquire(node_id, NULL);
        }
        
        do {
            // Proactively split the node
            if(node->is_full()) {
                int median;
                node_t *rnode;
                block_id_t rnode_id;
                split_node(node, &rnode, &rnode_id, &median);
                if(last_node == NULL) {
                    void *ptr = allocate(&root_id);
                    last_node = new (ptr) internal_node_t();
                    last_node_id = root_id;
                }
                last_node->insert(median, node_id, rnode_id);
                last_node_dirty = true;
                
                // Figure out where the key goes
                if(key < median) {
                    // Left node and node are the same thing
                    release(rnode_id, (void*)rnode, true, NULL);
                } else if(key >= median) {
                    release(node_id, (void*)node, true, NULL);
                    node = rnode;
                    node_id = rnode_id;
                }
            }

            // Insert the value, or move up the tree
            if(node->is_leaf()) {
                ((leaf_node_t*)node)->insert(key, value);
                release(node_id, (void*)node, true, NULL);
                break;
            } else {
                // Release and update the last node
                if(last_node_id) {
                    release(last_node_id, (void*)last_node, last_node_dirty, NULL);
                }
                last_node = (internal_node_t*)node;
                last_node_id = node_id;
                last_node_dirty = false;
                
                // Look up the next node; release and update the node
                node_id = ((internal_node_t*)node)->lookup(key);
                node = (node_t*)acquire(node_id, NULL);
                node_dirty = false;
            }
        } while(1);

        // Release the final last node
        if(last_node_id) {
            release(last_node_id, (void*)last_node, last_node_dirty, NULL);
        }
    }

    typedef typename node_t::leaf_node_t leaf_node_t;
    typedef typename node_t::internal_node_t internal_node_t;
    typedef typename cache_t::block_id_t block_id_t;

private:
    void split_node(node_t *node, node_t **rnode, block_id_t *rnode_id, int *median) {
        void *ptr = allocate(rnode_id);
        if(node->is_leaf()) {
            *rnode = new (ptr) leaf_node_t();
            ((leaf_node_t*)node)->split((leaf_node_t*)*rnode,
                                        median);
        } else {
            *rnode = new (ptr) internal_node_t();
            ((internal_node_t*)node)->split((internal_node_t*)*rnode,
                                            median);
        }
    }
    
private:
    typename cache_t::block_id_t root_id;
};

#endif // __BTREE_HPP__

