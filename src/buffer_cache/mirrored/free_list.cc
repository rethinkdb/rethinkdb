#include "free_list.hpp"
#include "buffer_cache/stats.hpp"

#define BLOCK_IN_USE (block_id_t(-2))

array_free_list_t::array_free_list_t(serializer_t *serializer)
    : serializer(serializer) { }

bool array_free_list_t::start(ready_callback_t *cb) {
    
    ready_callback = NULL;
    if (do_on_cpu(serializer->home_cpu, this, &array_free_list_t::do_make_list)) {
        return true;
    } else {
        ready_callback = cb;
        return false;
    }
}


bool array_free_list_t::do_make_list() {
    
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
    
    return do_on_cpu(home_cpu, this, &array_free_list_t::have_made_list);
}

bool array_free_list_t::have_made_list() {
    
    assert_cpu();
    
    if (ready_callback) ready_callback->on_free_list_ready();
    
    return true;
}

array_free_list_t::~array_free_list_t() {
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
