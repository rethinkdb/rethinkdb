
#ifndef __BTREE_NODE_IMPL_HPP__
#define __BTREE_NODE_IMPL_HPP__

#include "btree/leaf_node.hpp"
#include "btree/internal_node.hpp"

void node_handler::validate(btree_node *node) {
    switch(node->kind) {
        case btree_node_kind_leaf:
            leaf_node_handler::validate((btree_leaf_node *)node);
            break;
        case btree_node_kind_internal:
            internal_node_handler::validate((btree_internal_node *)node);
            break;
        default:
            assert(0);
    }
}

#endif