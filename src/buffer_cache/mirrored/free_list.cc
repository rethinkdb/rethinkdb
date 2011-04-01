#include "free_list.hpp"
#include "buffer_cache/stats.hpp"

array_free_list_t::array_free_list_t(translator_serializer_t *serializer)
    : serializer(serializer)
{
    on_thread_t switcher(serializer->home_thread);
    num_blocks_in_use = 0;
    next_new_block_id = serializer->max_block_id();
    for (block_id_t i = 0; i < next_new_block_id; i++) {
        if (!serializer->block_in_use(i)) {
            free_ids.push_back(i);
        } else {
            pm_n_blocks_total++;
            num_blocks_in_use++;
        }
    }
}

array_free_list_t::~array_free_list_t() {
}

void array_free_list_t::reserve_block_id(block_id_t id) {
    if (id >= next_new_block_id) {
        for (block_id_t i = next_new_block_id; i < id; i++) {
            free_ids.push_back(i);
        }
        next_new_block_id = id + 1;
        num_blocks_in_use++;
    } else {
        for (std::deque<block_id_t>::iterator it = free_ids.begin(); it != free_ids.end(); it++) {
            if (*it == id) {
                free_ids.erase(it);
                num_blocks_in_use++;
                return;
            }
        }
    }
}

block_id_t array_free_list_t::gen_block_id() {
    block_id_t id;
    if (free_ids.empty()) {
        id = next_new_block_id++;
    } else {
        id = free_ids.back();
        free_ids.pop_back();
    }
    
    pm_n_blocks_total++;
    num_blocks_in_use++;
    
    return id;
}

void array_free_list_t::release_block_id(block_id_t id) {
    free_ids.push_back(id);
    
    pm_n_blocks_total--;
    num_blocks_in_use--;
}
