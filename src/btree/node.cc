#include "btree/node.hpp"
#include "btree/leaf_node.hpp"
#include "btree/internal_node.hpp"

const block_magic_t btree_superblock_t::expected_magic = { {'s','u','p','e'} };
const block_magic_t internal_node_t::expected_magic = { {'i','n','t','e'} };

namespace node {

void print(const node_t *node) {
    if (!is_internal(node)) {
        // We need a value sizer to call the leaf::print function.
        // TODO: Take a sizer.
    } else {
        internal_node::print(reinterpret_cast<const internal_node_t *>(node));
    }
}

}  // namespace node
