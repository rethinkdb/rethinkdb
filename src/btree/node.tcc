
#ifndef __BTREE_NODE_TCC__
#define __BTREE_NODE_TCC__

#include "btree/leaf_node.hpp"
#include "btree/internal_node.hpp"

bool node_handler::is_underfull(btree_node *node) {
    return (node_handler::is_leaf(node)     &&     leaf_node_handler::is_underfull(    (leaf_node_t *) node))
        || (node_handler::is_internal(node) && internal_node_handler::is_underfull((internal_node_t *) node));
}


bool node_handler::is_mergable(btree_node *node, btree_node *sibling, btree_node *parent) {
    return (node_handler::is_leaf(node)     &&     leaf_node_handler::is_mergable((leaf_node_t *) node, (leaf_node_t *) sibling))
        || (node_handler::is_internal(node) && internal_node_handler::is_mergable((internal_node_t *) node, (internal_node_t *) sibling, (internal_node_t *) parent));
}


int node_handler::nodecmp(btree_node *node1, btree_node *node2) {
    assert(node_handler::is_leaf(node1) == node_handler::is_leaf(node2));
    if (node_handler::is_leaf(node1)) {
        return leaf_node_handler::nodecmp((leaf_node_t *) node1, (leaf_node_t *) node2);
    } else {
        return internal_node_handler::nodecmp((internal_node_t *) node1, (internal_node_t *) node2);
    }
}


void node_handler::merge(btree_node *node, btree_node *rnode, btree_key *key_to_remove, btree_node *parent) {
    if (node_handler::is_leaf(node)) {
        leaf_node_handler::merge((leaf_node_t *) node, (leaf_node_t *) rnode, key_to_remove);
    } else {
        internal_node_handler::merge((internal_node_t *) node, (internal_node_t *) rnode, key_to_remove, (internal_node_t *) parent);
    }
}

void node_handler::remove(btree_node *node, btree_key *key) {
    if (node_handler::is_leaf(node)) {
        leaf_node_handler::remove((leaf_node_t *) node, key);
    } else {
        internal_node_handler::remove((internal_node_t *) node, key);    
    }
}

bool node_handler::level(btree_node *node, btree_node *rnode, btree_key *key_to_replace, btree_key *replacement_key, btree_node *parent) {
    if (node_handler::is_leaf(node)) {
        return leaf_node_handler::level((leaf_node_t *) node, (leaf_node_t *) rnode, key_to_replace, replacement_key);
    } else {
        return internal_node_handler::level((internal_node_t *) node, (internal_node_t *) rnode, key_to_replace, replacement_key, (internal_node_t *) parent);
    }
}

void node_handler::print(btree_node *node) {
    if (node_handler::is_leaf(node)) {
        leaf_node_handler::print((leaf_node_t *) node);
    } else {
        internal_node_handler::print((internal_node_t *) node);
    }
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
            fail("Invalid leaf node type.");
    }
}
#endif
