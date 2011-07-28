#include "btree/node.hpp"
#include "btree/leaf_node.hpp"
#include "btree/internal_node.hpp"

const block_magic_t btree_superblock_t::expected_magic = { {'s','u','p','e'} };
const block_magic_t internal_node_t::expected_magic = { {'i','n','t','e'} };
const block_magic_t leaf_node_t::expected_magic = { {'l','e','a','f'} };

namespace node {


bool has_sensible_offsets(block_size_t block_size, const node_t *node) {
    return (is_leaf(node) && leaf::has_sensible_offsets(block_size, reinterpret_cast<const leaf_node_t *>(node)))
        || (is_internal(node) && internal_node::has_sensible_offsets(block_size, reinterpret_cast<const internal_node_t *>(node)));
}


bool is_underfull(block_size_t block_size, const node_t *node) {
    return (is_leaf(node) && leaf::is_underfull(block_size, reinterpret_cast<const leaf_node_t *>(node)))
        || (is_internal(node) && internal_node::is_underfull(block_size, reinterpret_cast<const internal_node_t *>(node)));
}


bool is_mergable(block_size_t block_size, const node_t *node, const node_t *sibling, const internal_node_t *parent) {
    return (is_leaf(node)     &&     leaf::is_mergable(block_size, reinterpret_cast<const leaf_node_t *>(node), reinterpret_cast<const leaf_node_t *>(sibling)))
        || (is_internal(node) && internal_node::is_mergable(block_size, reinterpret_cast<const internal_node_t *>(node), reinterpret_cast<const internal_node_t *>(sibling), parent));
}


int nodecmp(const node_t *node1, const node_t *node2) {
    rassert(is_leaf(node1) == is_leaf(node2));
    if (is_leaf(node1)) {
        return leaf::nodecmp(reinterpret_cast<const leaf_node_t *>(node1), reinterpret_cast<const leaf_node_t *>(node2));
    } else {
        return internal_node::nodecmp(reinterpret_cast<const internal_node_t *>(node1), reinterpret_cast<const internal_node_t *>(node2));
    }
}


void print(const node_t *node) {
    if (is_leaf(node)) {
        // No such function exists.
        //        leaf::print(reinterpret_cast<const leaf_node_t *>(node));
    } else {
        internal_node::print(reinterpret_cast<const internal_node_t *>(node));
    }
}

}  // namespace node
