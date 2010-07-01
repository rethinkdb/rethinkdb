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
    // Choose 1 and 2 instead of 0 and 1 to make it less likely that garbage will be interpreted as
    // a valid node
    btree_node_kind_leaf = 1,
    btree_node_kind_internal = 2
};

struct btree_node {
    uint8_t kind;
};

class node_handler {
    public:
        static bool is_leaf(btree_node *node) {
            validate(node);
            return node->kind == btree_node_kind_leaf;
        }

        static bool is_internal(btree_node *node) {
            validate(node);
            return node->kind == btree_node_kind_internal;
        }

        static void str_to_key(char *str, btree_key *buf) {
            int len = strlen(str);
            check("string too long", len > MAX_KEY_SIZE);
            memcpy(buf->contents, str, len);
            buf->size = (unsigned char)len;
        }
        
        static void validate(btree_node *node);
};

#endif // __BTREE_NODE_HPP__
