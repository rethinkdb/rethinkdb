#ifndef BTREE_DETEMPLATIZER_HPP_
#define BTREE_DETEMPLATIZER_HPP_

// This is disgusting.

// TODO: Uhm, refactor the sizer definitions to a central place, so we don't have to include
// files from non-btree directories here
//#include "server/nested_demo/redis_utils.hpp"
//#include "server/nested_demo/redis_sortedset_values.hpp"
#include "redis/redis_types.hpp"
#include "memcached/btree/value.hpp"

/*
 op_name is the name of a template function to call, arguments are the function
 arguments, and leaf_node is a pointer to a leaf_node_t.
 DETEMPLATIZE_LEAF_NODE_OP(op_name, leaf_node, sizer_argument, ...)
 selects a typename T based on the leaf_node's magic and runs the following:
 value_sizer_t<T>(sizer_argument) sizer;
 op_name<T>(&sizer, ...);
 */
#define DETEMPLATIZE_LEAF_NODE_OP(op_name, leaf_node, sizer_argument, ...) \
    do {                                                                \
        if (leaf_node->magic == value_sizer_t<memcached_value_t>::leaf_magic()) { \
            value_sizer_t<memcached_value_t> sizer(sizer_argument);     \
            op_name<memcached_value_t>(&sizer, __VA_ARGS__);            \
        } else if (leaf_node->magic == value_sizer_t<riak_value_t>::leaf_magic()) { \
            value_sizer_t<riak_value_t> sizer(sizer_argument);          \
            op_name<riak_value_t>(&sizer, __VA_ARGS__);                 \
        } else if (leaf_node->magic == value_sizer_t<redis_nested_string_value_t>::leaf_magic()) { \
            value_sizer_t<redis_nested_string_value_t> sizer(sizer_argument); \
            op_name<redis_nested_string_value_t>(&sizer, __VA_ARGS__);  \
        } else if (leaf_node->magic == value_sizer_t<redis_value_t>::leaf_magic()) { \
            value_sizer_t<redis_value_t> sizer(sizer_argument);         \
            op_name<redis_value_t>(&sizer, __VA_ARGS__);                \
        } else if (leaf_node->magic == value_sizer_t<redis_nested_set_value_t>::leaf_magic()) { \
            value_sizer_t<redis_nested_set_value_t> sizer(sizer_argument); \
            op_name<redis_nested_set_value_t>(&sizer, __VA_ARGS__);     \
        } else if (leaf_node->magic == value_sizer_t<redis_nested_sorted_set_value_t>::leaf_magic()) { \
            value_sizer_t<redis_nested_sorted_set_value_t> sizer(sizer_argument); \
            op_name<redis_nested_sorted_set_value_t>(&sizer, __VA_ARGS__); \
        } else {                                                        \
            crash("Unmatched leaf node magic: %.*s", (int)sizeof(block_magic_t), leaf_node->magic.bytes); \
        }                                                               \
    } while(0)

#endif /* BTREE_DETEMPLATIZER_HPP_ */

