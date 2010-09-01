
#include "btree/leaf_node.hpp"
#include "btree/internal_node.hpp"
#include "btree/leaf_node.tcc"
#include "btree/internal_node.tcc"
#include "config/args.hpp"
#include <stdlib.h>
#include <time.h>

#define iterations 2000

/* \brief make sure splits don't result in 2 mergeable nodes
 */
void test_leaf_split_merge() {
    srand(time(NULL));
    char nm1[BTREE_BLOCK_SIZE], nm2[BTREE_BLOCK_SIZE];
    btree_leaf_node *ln1 = (btree_leaf_node *) nm1,  *ln2 = (btree_leaf_node *) nm2;

    char max_key[MAX_KEY_SIZE + sizeof(btree_key)];
    btree_key *key = (btree_key *) max_key;

    char max_value[MAX_TOTAL_NODE_CONTENTS_SIZE];
    btree_value *value = (btree_value *) max_value;

    for (int i = 0; i < iterations; i++) {
        /* initialize */
        leaf_node_handler::init(ln1);
        leaf_node_handler::init(ln2);

        /* fill the node up */
        while (!leaf_node_handler::is_full(ln1, (btree_key *) key, (btree_value *) value)) {
            key->size = rand() % MAX_KEY_SIZE;
            value->size = rand() % MAX_IN_NODE_VALUE_SIZE;
            leaf_node_handler::insert(ln1, (btree_key *) key, (btree_value *) value);
        }

        leaf_node_handler::split(ln1, ln2, key);

        assert(!leaf_node_handler::is_mergable(ln1, ln2));
        printf("Success\n");
    }
}

void test_internal_split_merge() {
    srand(time(NULL));
    /* test internal nodes */
    char nm1[BTREE_BLOCK_SIZE], nm2[BTREE_BLOCK_SIZE], pnm3[BTREE_BLOCK_SIZE];
    btree_internal_node *in1 = (btree_internal_node *) nm1,  *in2 = (btree_internal_node *) nm2, *pn = (btree_internal_node *) pnm3;;

    char max_key[MAX_KEY_SIZE + sizeof(btree_key)];
    btree_key *key = (btree_key *) max_key;

    block_id_t inode, rnode;

    key->size = 1;
    internal_node_handler::init(pn);
    internal_node_handler::insert(pn, (btree_key *) key, inode, rnode);

    for (int i = 0; i < iterations; i++) {
        /* initialize */
        internal_node_handler::init(in1);
        internal_node_handler::init(in2);

        /* fill the node up */
        while (!internal_node_handler::is_full(in1)) {
            key->size = rand() % MAX_KEY_SIZE;
            if (key->size < 5)
                key->size = 5;

            for (int j = 0; j < key->size; j++)
                key->contents[j] = (char) rand();

            internal_node_handler::insert(in1, (btree_key *) key, inode, rnode);
        }

        internal_node_handler::split(in1, in2, key);

        assert(!internal_node_handler::is_mergable(in1, in2, pn));
        printf("Success\n");
    }
}

/* \brief make sure merges don't result in a splittable node
 */
void test_merge_split() {
}
