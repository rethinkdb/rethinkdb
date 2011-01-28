#ifndef __BUFFER_CACHE_MIRRORED_FREE_LIST_HPP__
#define __BUFFER_CACHE_MIRRORED_FREE_LIST_HPP__

#include "serializer/serializer.hpp"
#include "serializer/translator.hpp"
#include "buffer_cache/types.hpp"
#include "utils.hpp"

/* TODO We could implement a free list in the unused cells of the page map, saving a little bit
of memory. */

class array_free_list_t : public home_thread_mixin_t {
public:
    explicit array_free_list_t(translator_serializer_t *);

    struct ready_callback_t {
        virtual void on_free_list_ready() = 0;
        virtual ~ready_callback_t() {}
    };
    bool start(ready_callback_t *cb);   // Must be called after serializer has started up
    
    ~array_free_list_t();
    
    int num_blocks_in_use;
    block_id_t gen_block_id();
    void release_block_id(block_id_t);

private:
    ready_callback_t *ready_callback;
    bool do_make_list();   // Called on serializer thread
    bool have_made_list();   // Called on cache thread
    
    translator_serializer_t *serializer;
    
    /* A block ID is free if it is >= next_new_block_id or if it is in free_ids. All the IDs in
    free_ids are less than next_new_block_id. */
    block_id_t next_new_block_id;
    std::deque<block_id_t> free_ids;
};

#endif /* __BUFFER_CACHE_MIRRORED_FREE_LIST_HPP__ */
