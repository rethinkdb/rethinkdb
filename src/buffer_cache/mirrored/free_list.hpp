#ifndef BUFFER_CACHE_MIRRORED_FREE_LIST_HPP_
#define BUFFER_CACHE_MIRRORED_FREE_LIST_HPP_

#include <deque>

#include "buffer_cache/types.hpp"
#include "utils.hpp"

class serializer_t;
/* TODO We could implement a free list in the unused cells of the page map, saving a little bit
of memory. */

struct mc_cache_stats_t;

class array_free_list_t : public home_thread_mixin_debug_only_t {
public:
    array_free_list_t(serializer_t *, mc_cache_stats_t *);
    ~array_free_list_t();

    int num_blocks_in_use;
    mc_cache_stats_t *stats;
    void reserve_block_id(block_id_t id);
    block_id_t gen_block_id();
    void release_block_id(block_id_t);

private:
    serializer_t *serializer;

    /* A block ID is free if it is >= next_new_block_id or if it is in free_ids. All the IDs in
    free_ids are less than next_new_block_id. */
    block_id_t next_new_block_id;
    std::deque<block_id_t> free_ids;
};

#endif /* BUFFER_CACHE_MIRRORED_FREE_LIST_HPP_ */
