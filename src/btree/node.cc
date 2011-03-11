#include "btree/node.hpp"
#include "btree/leaf_node.hpp"
#include "btree/internal_node.hpp"

const block_magic_t btree_superblock_t::expected_magic = { {'s','u','p','e'} };
const block_magic_t internal_node_t::expected_magic = { {'i','n','t','e'} };
const block_magic_t leaf_node_t::expected_magic = { {'l','e','a','f'} };

namespace node {


bool has_sensible_offsets(block_size_t block_size, const node_t *node) {
    return (is_leaf(node) && leaf::has_sensible_offsets(block_size, ptr_cast<leaf_node_t>(node)))
        || (is_internal(node) && internal_node::has_sensible_offsets(block_size, ptr_cast<internal_node_t>(node)));
}


bool is_underfull(block_size_t block_size, const node_t *node) {
    return (is_leaf(node) && leaf::is_underfull(block_size, ptr_cast<leaf_node_t>(node)))
        || (is_internal(node) && internal_node::is_underfull(block_size, ptr_cast<internal_node_t>(node)));
}


bool is_mergable(block_size_t block_size, const node_t *node, const node_t *sibling, const internal_node_t *parent) {
    return (is_leaf(node)     &&     leaf::is_mergable(block_size, ptr_cast<leaf_node_t>(node), ptr_cast<leaf_node_t>(sibling)))
        || (is_internal(node) && internal_node::is_mergable(block_size, ptr_cast<internal_node_t>(node), ptr_cast<internal_node_t>(sibling), parent));
}


int nodecmp(const node_t *node1, const node_t *node2) {
    rassert(is_leaf(node1) == is_leaf(node2));
    if (is_leaf(node1)) {
        return leaf::nodecmp(ptr_cast<leaf_node_t>(node1), ptr_cast<leaf_node_t>(node2));
    } else {
        return internal_node::nodecmp(ptr_cast<internal_node_t>(node1), ptr_cast<internal_node_t>(node2));
    }
}

void split(block_size_t block_size, buf_t &node_buf, buf_t &rnode_buf, btree_key_t *median) {
    if (is_leaf(ptr_cast<node_t>(node_buf.get_data_read()))) {
        leaf::split(block_size, node_buf, rnode_buf, median);
    } else {
        internal_node::split(block_size, node_buf, rnode_buf, median);
    }
}

void merge(block_size_t block_size, const node_t *node, buf_t &rnode_buf, btree_key_t *key_to_remove, const internal_node_t *parent) {
    if (is_leaf(node)) {
        leaf::merge(block_size, ptr_cast<leaf_node_t>(node), rnode_buf, key_to_remove);
    } else {
        internal_node::merge(block_size, ptr_cast<internal_node_t>(node), rnode_buf, key_to_remove, parent);
    }
}

bool level(block_size_t block_size, buf_t &node_buf, buf_t &rnode_buf, btree_key_t *key_to_replace, btree_key_t *replacement_key, const internal_node_t *parent) {
    if (is_leaf(ptr_cast<node_t>(node_buf.get_data_read()))) {
        return leaf::level(block_size, node_buf, rnode_buf, key_to_replace, replacement_key);
    } else {
        return internal_node::level(block_size, node_buf, rnode_buf, key_to_replace, replacement_key, parent);
    }
}

void print(const node_t *node) {
    if (is_leaf(node)) {
        leaf::print(ptr_cast<leaf_node_t>(node));
    } else {
        internal_node::print(ptr_cast<internal_node_t>(node));
    }
}

void validate(block_size_t block_size, const node_t *node) {
#ifndef NDEBUG
    if (check_magic<leaf_node_t>(node->magic)) {
        leaf::validate(block_size, (leaf_node_t *)node);
    } else if (check_magic<internal_node_t>(node->magic)) {
        internal_node::validate(block_size, (internal_node_t *)node);
    } else {
        unreachable("Invalid leaf node type.");
    }
#endif
}

}  // namespace node

template <>
bool check_magic<node_t>(block_magic_t magic) {
    return check_magic<leaf_node_t>(magic) || check_magic<internal_node_t>(magic);
}

