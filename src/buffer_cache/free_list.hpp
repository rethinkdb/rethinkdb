#ifndef BUFFER_CACHE_FREE_LIST_HPP_
#define BUFFER_CACHE_FREE_LIST_HPP_

#include <forward_list>
#include <boost/pool/pool_alloc.hpp>

#include "serializer/types.hpp"

namespace alt {

class free_list_t {
public:
    explicit free_list_t(serializer_t *serializer);
    ~free_list_t();

    // Returns a block id.  The current_page_t for this block id, if present, has no
    // acquirers.
    MUST_USE block_id_t acquire_block_id();
    MUST_USE block_id_t acquire_aux_block_id();
    void release_block_id(block_id_t block_id);

    // Like acquire_block_id, only instead of being told what block id you acquired,
    // you tell it which block id you're acquiring.  It must be available!
    void acquire_chosen_block_id(block_id_t block_id);

private:
    typedef std::forward_list<block_id_t, boost::pool_allocator<block_id_t>> block_ids_container;

    block_id_t next_new_block_id_;
    block_ids_container free_ids_;
    block_id_t next_new_aux_block_id_;
    block_ids_container free_aux_ids_;
    DISABLE_COPYING(free_list_t);
};


}  // namespace alt

#endif  // BUFFER_CACHE_FREE_LIST_HPP_
