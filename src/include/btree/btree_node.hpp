#ifndef __BTREE_NODE_HPP__
#define __BTREE_NODE_HPP__

#include <stdint.h>
#include <string.h>

struct btree_key {
    uint8_t size;
    char contents[0];
};

struct btree_node {
    bool leaf;
};

class btree {
    public:
        static bool is_leaf(btree_node *node) {
            return node->leaf;
        }

        static bool is_internal(btree_node *node) {
            return !node->leaf;
        }

        static btree_key *str_to_key(char *str) {
            //TODO: Use a different allocator
            int len = strlen(str);
            btree_key *key = (btree_key *)malloc(sizeof(btree_key) + len);
            memcpy(key->contents, str, len);
            key->size = (unsigned char)len;
            return key;
        }
};

#endif // __BTREE_NODE_HPP__
