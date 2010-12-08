#include "btree/node.hpp"
#include "btree/leaf_node.hpp"
#include "btree/internal_node.hpp"

const block_magic_t btree_superblock_t::expected_magic = { {'s','u','p','e'} };
const block_magic_t internal_node_t::expected_magic = { {'i','n','t','e'} };
const block_magic_t leaf_node_t::expected_magic = { {'l','e','a','f'} };




bool node_handler::is_underfull(block_size_t block_size, const btree_node *node) {
    return (node_handler::is_leaf(node)     &&     leaf_node_handler::is_underfull(block_size,     leaf_node_handler::leaf_node(node)))
        || (node_handler::is_internal(node) && internal_node_handler::is_underfull(block_size, internal_node_handler::internal_node(node)));
}


bool node_handler::is_mergable(block_size_t block_size, const btree_node *node, const btree_node *sibling, const btree_node *parent) {
    return (node_handler::is_leaf(node)     &&     leaf_node_handler::is_mergable(block_size, leaf_node_handler::leaf_node(node), leaf_node_handler::leaf_node(sibling)))
        || (node_handler::is_internal(node) && internal_node_handler::is_mergable(block_size, internal_node_handler::internal_node(node), internal_node_handler::internal_node(sibling), internal_node_handler::internal_node(parent)));
}


int node_handler::nodecmp(const btree_node *node1, const btree_node *node2) {
    assert(node_handler::is_leaf(node1) == node_handler::is_leaf(node2));
    if (node_handler::is_leaf(node1)) {
        return leaf_node_handler::nodecmp(leaf_node_handler::leaf_node(node1), leaf_node_handler::leaf_node(node2));
    } else {
        return internal_node_handler::nodecmp(internal_node_handler::internal_node(node1), internal_node_handler::internal_node(node2));
    }
}


void node_handler::merge(block_size_t block_size, btree_node *node, btree_node *rnode, btree_key *key_to_remove, btree_node *parent) {
    if (node_handler::is_leaf(node)) {
        leaf_node_handler::merge(block_size, leaf_node_handler::leaf_node(node), leaf_node_handler::leaf_node(rnode), key_to_remove);
    } else {
        internal_node_handler::merge(block_size, internal_node_handler::internal_node(node), internal_node_handler::internal_node(rnode), key_to_remove, internal_node_handler::internal_node(parent));
    }
}

bool node_handler::level(block_size_t block_size, btree_node *node, btree_node *rnode, btree_key *key_to_replace, btree_key *replacement_key, btree_node *parent) {
    if (node_handler::is_leaf(node)) {
        return leaf_node_handler::level(block_size, leaf_node_handler::leaf_node(node), leaf_node_handler::leaf_node(rnode), key_to_replace, replacement_key);
    } else {
        return internal_node_handler::level(block_size, internal_node_handler::internal_node(node), internal_node_handler::internal_node(rnode), key_to_replace, replacement_key, internal_node_handler::internal_node(parent));
    }
}

void node_handler::print(const btree_node *node) {
    if (node_handler::is_leaf(node)) {
        leaf_node_handler::print(leaf_node_handler::leaf_node(node));
    } else {
        internal_node_handler::print(internal_node_handler::internal_node(node));
    }
}

void node_handler::validate(block_size_t block_size, const btree_node *node) {
#ifndef NDEBUG
    if (check_magic<leaf_node_t>(node->magic)) {
        leaf_node_handler::validate(block_size, (leaf_node_t *)node);
    } else if (check_magic<internal_node_t>(node->magic)) {
        internal_node_handler::validate(block_size, (internal_node_t *)node);
    } else {
        fail("Invalid leaf node type.");
    }
#endif
}

template <>
bool check_magic<btree_node>(block_magic_t magic) {
    return check_magic<leaf_node_t>(magic) || check_magic<internal_node_t>(magic);
}

