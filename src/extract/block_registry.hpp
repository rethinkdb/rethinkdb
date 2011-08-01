#ifndef __EXTRACT_LEAF_REGISTRY_HPP__
#define __EXTRACT_LEAF_REGISTRY_HPP__

#include <map>
#include "serializer/log/data_block_manager.hpp"
#include "serializer/log/log_serializer.hpp"
struct btree_leaf_node;

class block_registry {
public:
    block_registry();

    // Tells the block_registry about some new block that has been read.
    void tell_block(off64_t offset, const ls_buf_data_t& buf_data);

    bool has_block(block_id_t block_id, off64_t offset);
  
    // Destroys block sequence_ids, we don't need them any more and we'd like to
    // free up memory.
    const std::map<size_t, off64_t>& destroy_block_sequence_ids();

    enum { null = -1 };

private:

    // We use maps to be more safe about corrupted block and block sequence ids. (otherwise those
    // could easily lead to excessive memory consumption)
    std::map<size_t, block_sequence_id_t> block_sequence_ids;
    std::map<size_t, off64_t> offsets;

    DISABLE_COPYING(block_registry);
};



#endif  // __EXTRACT_LEAF_REGISTRY_HPP__
