#include "btree/node.hpp"

#include "btree/leaf_node.hpp"
#include "btree/internal_node.hpp"
#include "buffer_cache/buffer_cache.hpp"

const block_magic_t btree_superblock_t::expected_magic = { { 's', 'u', 'p', 'e' } };
const block_magic_t internal_node_t::expected_magic = { { 'i', 'n', 't', 'e' } };

namespace node {

void print(const node_t *node) {
    if (!is_internal(node)) {
        // We need a value sizer to call the leaf::print function.
        // TODO: Take a sizer.
    } else {
        internal_node::print(reinterpret_cast<const internal_node_t *>(node));
    }
}

bool is_underfull(value_sizer_t<void> *sizer, const node_t *node) {
    if (node->magic == sizer->btree_leaf_magic()) {
        return leaf::is_underfull(sizer, reinterpret_cast<const leaf_node_t *>(node));
    } else {
        rassert(is_internal(node));
        return internal_node::is_underfull(sizer->block_size(), reinterpret_cast<const internal_node_t *>(node));
    }
}

bool is_mergable(value_sizer_t<void> *sizer, const node_t *node, const node_t *sibling, const internal_node_t *parent) {
    if (sizer->btree_leaf_magic() == node->magic) {
        return leaf::is_mergable(sizer, reinterpret_cast<const leaf_node_t *>(node), reinterpret_cast<const leaf_node_t *>(sibling));
    } else {
        rassert(is_internal(node));
        return internal_node::is_mergable(sizer->block_size(), reinterpret_cast<const internal_node_t *>(node), reinterpret_cast<const internal_node_t *>(sibling), parent);
    }
}


void split(value_sizer_t<void> *sizer, buf_lock_t *node_buf, node_t *rnode, btree_key_t *median) {
    if (is_leaf(reinterpret_cast<const node_t *>(node_buf->get_data_read()))) {
        leaf_node_t *node = reinterpret_cast<leaf_node_t *>(node_buf->get_data_major_write());
        leaf::split(sizer, node, reinterpret_cast<leaf_node_t *>(rnode), median);
    } else {
        internal_node::split(sizer->block_size(), node_buf, reinterpret_cast<internal_node_t *>(rnode), median);
    }
}

void merge(value_sizer_t<void> *sizer, node_t *node, buf_lock_t *rnode_buf, const internal_node_t *parent) {
    if (is_leaf(node)) {
        leaf::merge(sizer, reinterpret_cast<leaf_node_t *>(node), reinterpret_cast<leaf_node_t *>(rnode_buf->get_data_major_write()));
    } else {
        // TODO: internal_node::merge should not take a buf_lock_t, just
        // have it take an internal_node_t *rnode.
        internal_node::merge(sizer->block_size(), reinterpret_cast<const internal_node_t *>(node), rnode_buf, parent);
    }
}

bool level(value_sizer_t<void> *sizer, int nodecmp_node_with_sib, buf_lock_t *node_buf, buf_lock_t *rnode_buf, btree_key_t *replacement_key, const internal_node_t *parent) {
    if (is_leaf(reinterpret_cast<const node_t *>(node_buf->get_data_read()))) {
        return leaf::level(sizer, nodecmp_node_with_sib, reinterpret_cast<leaf_node_t *>(node_buf->get_data_major_write()), reinterpret_cast<leaf_node_t *>(rnode_buf->get_data_major_write()), replacement_key);
    } else {
        return internal_node::level(sizer->block_size(), node_buf, rnode_buf, replacement_key, parent);
    }
}

void validate(DEBUG_VAR value_sizer_t<void> *sizer, DEBUG_VAR const node_t *node) {
#ifndef NDEBUG
    if (node->magic == sizer->btree_leaf_magic()) {
        leaf::validate(sizer, reinterpret_cast<const leaf_node_t *>(node));
    } else if (node->magic == internal_node_t::expected_magic) {
        internal_node::validate(sizer->block_size(), reinterpret_cast<const internal_node_t *>(node));
    } else {
        unreachable("Invalid leaf node type.");
    }
#endif
}

}  // namespace node
