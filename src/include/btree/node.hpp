#ifndef __BTREE_NODE_HPP__
#define __BTREE_NODE_HPP__

#include <stdint.h>
#include <string.h>

#define MAX_KEY_SIZE 250

// Note: Changing this struct changes the format of the data stored on disk.
// If you change this struct, previous stored data will be misinterpreted.
struct btree_key {
    uint8_t size;
    char contents[0];
};

struct btree_value {
    uint32_t size;
    char contents[0];
};

struct btree_node {
    bool leaf;
};

class node_handler {
    public:
        static bool is_leaf(btree_node *node) {
            return node->leaf;
        }

        static bool is_internal(btree_node *node) {
            return !node->leaf;
        }

        static void str_to_key(char *str, btree_key *buf) {
            int len = strlen(str);
            check("string too long", len > 250);
            memcpy(buf->contents, str, len);
            buf->size = (unsigned char)len;
        }
};

#endif // __BTREE_NODE_HPP__
