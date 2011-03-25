#ifndef __EXTRACT_LEAF_REGISTRY_HPP__
#define __EXTRACT_LEAF_REGISTRY_HPP__

#include "serializer/log/data_block_manager.hpp"
#include "serializer/log/log_serializer.hpp"
struct btree_leaf_node;

class block_registry {
public:
    block_registry();

    // Tells the block_registry about some new block that has been read.
    void tell_block(off64_t offset, const ls_buf_data_t& buf_data);

    bool has_block(ser_block_id_t block_id, off64_t offset) const;

    // Checks that we have seen a block with every single block id.
    bool check_block_id_contiguity() const;
  
    // Destroys transaction_ids, we don't need them any more and we'd
    // like to free up memory.
    const segmented_vector_t<off64_t, MAX_BLOCK_ID>& destroy_block_sequence_ids();

    enum { null = -1 };

private:

    segmented_vector_t<ser_block_sequence_id_t, MAX_BLOCK_ID> block_sequence_ids;
    segmented_vector_t<off64_t, MAX_BLOCK_ID> offsets;

    DISABLE_COPYING(block_registry);
};



#endif  // __EXTRACT_LEAF_REGISTRY_HPP__
