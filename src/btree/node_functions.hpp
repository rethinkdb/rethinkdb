#ifndef __BTREE_NODE_FUNCTIONS_HPP__
#define __BTREE_NODE_FUNCTIONS_HPP__

// We have to have this file because putting these definitions in node.hpp would require mutually recursive header files.

#include "btree/node.hpp"
#include "btree/leaf_node.hpp"

namespace node {

template <class Value>
void split(value_sizer_t<Value> *sizer, buf_t *node_buf, buf_t *rnode_buf, btree_key_t *median) {
    if (is_leaf(reinterpret_cast<const node_t *>(node_buf->get_data_read()))) {
        leaf::split(sizer, node_buf, rnode_buf, median);
    } else {
        internal_node::split(sizer->block_size(), node_buf, rnode_buf, median);
    }
}

}  // namespace node


#endif  // __BTREE_NODE_FUNCTIONS_HPP__
