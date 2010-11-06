#include "free_list.hpp"
#include "buffer_cache/stats.hpp"

#define BLOCK_IN_USE (block_id_t(-2))

array_free_list_t::array_free_list_t(serializer_t *serializer)
    : serializer(serializer) { }

bool array_free_list_t::start(ready_callback_t *cb) {
    
    ready_callback = NULL;
    if (do_on_cpu(serializer->home_cpu, this, &array_free_list_t::do_get_size)) {
        return true;
    } else {
        ready_callback = cb;
        return false;
    }
}

/* We have to build the free list in several phases. First, we go to the serializer
core and get the max block ID. Then we return to our core and resize the free list array.
Then we go to the serializer core and build the free list. Then we return to our core
and call the callback. The reason for this is that the free list array must be resized
on our core because it may allocate memory in the process of resizing. */

bool array_free_list_t::do_get_size() {
    
    serializer->assert_cpu();
    
    ser_block_id_t max_block_id = serializer->max_block_id();
    
    return do_on_cpu(home_cpu, this, &array_free_list_t::have_gotten_size, max_block_id);
}

bool array_free_list_t::have_gotten_size(ser_block_id_t m) {
    
    free_list.set_size(m);
    
    return do_on_cpu(serializer->home_cpu, this, &array_free_list_t::do_make_list);
}

bool array_free_list_t::do_make_list() {
    
    first_block = NULL_BLOCK_ID;
    num_blocks_in_use = 0;
    
    for (block_id_t i = 0; i < free_list.get_size(); i++) {
        if (serializer->block_in_use(i)) {
            free_list[i] = BLOCK_IN_USE;
            pm_n_blocks_total++;
            num_blocks_in_use++;
        } else {
            free_list[i] = first_block;
            first_block = i;
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
    
    if (first_block == NULL_BLOCK_ID) {
        id = free_list.get_size();
        free_list.set_size(free_list.get_size() + 1);
    } else {
        id = first_block;
        first_block = free_list[first_block];
    }
    
    pm_n_blocks_total++;
    num_blocks_in_use++;
    
    free_list[id] = BLOCK_IN_USE;
    return id;
}

void array_free_list_t::release_block_id(block_id_t id) {
    
    assert(free_list[id] == BLOCK_IN_USE);
    
    free_list[id] = first_block;
    first_block = id;
    
    pm_n_blocks_total--;
    num_blocks_in_use--;
}
