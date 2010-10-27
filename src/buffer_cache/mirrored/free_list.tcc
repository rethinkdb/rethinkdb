#define BLOCK_IN_USE (block_id_t(-2))

template<class mc_config_t>
array_free_list_t<mc_config_t>::array_free_list_t(mc_cache_t<mc_config_t> *cache)
    : cache(cache), pm_num_blocks_in_use("blocks_total", &num_blocks_in_use, perfmon_combiner_sum) { }

template<class mc_config_t>
bool array_free_list_t<mc_config_t>::start(ready_callback_t *cb) {
    
    ready_callback = NULL;
    if (do_on_cpu(cache->serializer->home_cpu, this, &array_free_list_t::do_get_size)) {
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

template<class mc_config_t>
bool array_free_list_t<mc_config_t>::do_get_size() {
    
    cache->serializer->assert_cpu();
    
    ser_block_id_t max_block_id = cache->serializer->max_block_id();
    
    return do_on_cpu(cache->home_cpu, this, &array_free_list_t::have_gotten_size, max_block_id);
}

template<class mc_config_t>
bool array_free_list_t<mc_config_t>::have_gotten_size(ser_block_id_t m) {
    
    free_list.set_size(m);
    
    return do_on_cpu(cache->serializer->home_cpu, this, &array_free_list_t::do_make_list);
}

template<class mc_config_t>
bool array_free_list_t<mc_config_t>::do_make_list() {
    
    first_block = NULL_BLOCK_ID;
    num_blocks_in_use = 0;
    
    for (block_id_t i = 0; i < free_list.get_size(); i++) {
        if (cache->serializer->block_in_use(i)) {
            free_list[i] = BLOCK_IN_USE;
            num_blocks_in_use++;
        } else {
            free_list[i] = first_block;
            first_block = i;
        }
    }
    
    return do_on_cpu(cache->home_cpu, this, &array_free_list_t::have_made_list);
}

template<class mc_config_t>
bool array_free_list_t<mc_config_t>::have_made_list() {
    
    cache->assert_cpu();
    
    if (ready_callback) ready_callback->on_free_list_ready();
    
    return true;
}

template<class mc_config_t>
array_free_list_t<mc_config_t>::~array_free_list_t() {
}

template<class mc_config_t>
block_id_t array_free_list_t<mc_config_t>::gen_block_id() {
    
    block_id_t id;
    
    if (first_block == NULL_BLOCK_ID) {
        id = free_list.get_size();
        free_list.set_size(free_list.get_size() + 1);
    } else {
        id = first_block;
        first_block = free_list[first_block];
    }
    
    num_blocks_in_use++;
    
    free_list[id] = BLOCK_IN_USE;
    return id;
}

template<class mc_config_t>
void array_free_list_t<mc_config_t>::release_block_id(block_id_t id) {
    
    assert(free_list[id] == BLOCK_IN_USE);
    
    free_list[id] = first_block;
    first_block = id;
    
    num_blocks_in_use--;
}
