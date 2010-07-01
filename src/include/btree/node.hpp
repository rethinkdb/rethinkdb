#ifndef __BTREE_NODE_HPP__
#define __BTREE_NODE_HPP__

#include <stdint.h>
#include <string.h>
#include <assert.h>

#define MAX_KEY_SIZE 250

// Note: Changing this struct changes the format of the data stored on disk.
// If you change this struct, previous stored data will be misinterpreted.
struct btree_key {
    uint8_t size;
    char contents[0];
};

enum btree_node_kind {
	btree_node_kind_leaf,
	btree_node_kind_internal
};

struct btree_node {
    uint8_t kind;
};

class node_handler {
    public:
        static bool is_leaf(btree_node *node) {
        
        	// Check to make sure the data is not corrupted
        	assert(node->kind == btree_node_kind_leaf || node->kind == btree_node_kind_internal);
        	
            return node->kind == btree_node_kind_leaf;
        }

        static bool is_internal(btree_node *node) {
            return !is_leaf(node);
        }

        static void str_to_key(char *str, btree_key *buf) {
            int len = strlen(str);
            check("string too long", len > 250);
            memcpy(buf->contents, str, len);
            buf->size = (unsigned char)len;
        }
};

#endif // __BTREE_NODE_HPP__
