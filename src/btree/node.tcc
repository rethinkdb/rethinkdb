
#ifndef __BTREE_NODE_TCC__
#define __BTREE_NODE_TCC__

#include "btree/leaf_node.hpp"
#include "btree/internal_node.hpp"

bool node_handler::is_underfull(btree_node *node) {
    return (node_handler::is_leaf(node)     &&     leaf_node_handler::is_underfull(    (leaf_node_t *) node))
        || (node_handler::is_internal(node) && internal_node_handler::is_underfull((internal_node_t *) node));
}

void node_handler::validate(btree_node *node) {
    switch(node->type) {
        case btree_node_type_leaf:
            leaf_node_handler::validate((btree_leaf_node *)node);
            break;
        case btree_node_type_internal:
            internal_node_handler::validate((btree_internal_node *)node);
            break;
        default:
            assert(0);
    }
}
#endif
