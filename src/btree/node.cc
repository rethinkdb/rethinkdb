#include "btree/node.hpp"
#include "btree/leaf_node.hpp"
#include "btree/internal_node.hpp"

const block_magic_t btree_superblock_t::expected_magic = { {'s','u','p','e'} };
const block_magic_t internal_node_t::expected_magic = { {'i','n','t','e'} };
const block_magic_t leaf_node_t::expected_magic = { {'l','e','a','f'} };




bool node_handler::is_underfull(block_size_t block_size, const node_t *node) {
    return (node_handler::is_leaf(node) && leaf_node_handler::is_underfull(block_size, ptr_cast<leaf_node_t>(node)))
        || (node_handler::is_internal(node) && internal_node_handler::is_underfull(block_size, ptr_cast<internal_node_t>(node)));
}


bool node_handler::is_mergable(block_size_t block_size, const node_t *node, const node_t *sibling, const internal_node_t *parent) {
    return (node_handler::is_leaf(node)     &&     leaf_node_handler::is_mergable(block_size, ptr_cast<leaf_node_t>(node), ptr_cast<leaf_node_t>(sibling)))
        || (node_handler::is_internal(node) && internal_node_handler::is_mergable(block_size, ptr_cast<internal_node_t>(node), ptr_cast<internal_node_t>(sibling), parent));
}


int node_handler::nodecmp(const node_t *node1, const node_t *node2) {
    assert(node_handler::is_leaf(node1) == node_handler::is_leaf(node2));
    if (node_handler::is_leaf(node1)) {
        return leaf_node_handler::nodecmp(ptr_cast<leaf_node_t>(node1), ptr_cast<leaf_node_t>(node2));
    } else {
        return internal_node_handler::nodecmp(ptr_cast<internal_node_t>(node1), ptr_cast<internal_node_t>(node2));
    }
}

void node_handler::split(block_size_t block_size, node_t *node, node_t *rnode, btree_key *median) {
    if (node_handler::is_leaf(node)) {
        leaf_node_handler::split(block_size, ptr_cast<leaf_node_t>(node), ptr_cast<leaf_node_t>(rnode), median);
    } else {
        internal_node_handler::split(block_size, ptr_cast<internal_node_t>(node), ptr_cast<internal_node_t>(rnode), median);
    }
}

void node_handler::merge(block_size_t block_size, const node_t *node, node_t *rnode, btree_key *key_to_remove, internal_node_t *parent) {
    if (node_handler::is_leaf(node)) {
        leaf_node_handler::merge(block_size, ptr_cast<leaf_node_t>(node), ptr_cast<leaf_node_t>(rnode), key_to_remove);
    } else {
        internal_node_handler::merge(block_size, ptr_cast<internal_node_t>(node), ptr_cast<internal_node_t>(rnode), key_to_remove, parent);
    }
}

bool node_handler::level(block_size_t block_size, node_t *node, node_t *rnode, btree_key *key_to_replace, btree_key *replacement_key, internal_node_t *parent) {
    if (node_handler::is_leaf(node)) {
        return leaf_node_handler::level(block_size, ptr_cast<leaf_node_t>(node), ptr_cast<leaf_node_t>(rnode), key_to_replace, replacement_key);
    } else {
        return internal_node_handler::level(block_size, ptr_cast<internal_node_t>(node), ptr_cast<internal_node_t>(rnode), key_to_replace, replacement_key, parent);
    }
}

void node_handler::print(const node_t *node) {
    if (node_handler::is_leaf(node)) {
        leaf_node_handler::print(ptr_cast<leaf_node_t>(node));
    } else {
        internal_node_handler::print(ptr_cast<internal_node_t>(node));
    }
}

void node_handler::validate(block_size_t block_size, const node_t *node) {
#ifndef NDEBUG
    if (check_magic<leaf_node_t>(node->magic)) {
        leaf_node_handler::validate(block_size, (leaf_node_t *)node);
    } else if (check_magic<internal_node_t>(node->magic)) {
        internal_node_handler::validate(block_size, (internal_node_t *)node);
    } else {
        unreachable("Invalid leaf node type.");
    }
#endif
}

template <>
bool check_magic<node_t>(block_magic_t magic) {
    return check_magic<leaf_node_t>(magic) || check_magic<internal_node_t>(magic);
}

