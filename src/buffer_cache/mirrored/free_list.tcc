#define BLOCK_IN_USE (block_id_t(-2))

template<class mc_config_t>
array_free_list_t<mc_config_t>::array_free_list_t(mc_cache_t<mc_config_t> *cache)
    : cache(cache) { }

template<class mc_config_t>
bool array_free_list_t<mc_config_t>::start(ready_callback_t *cb) {
    
    first_block = NULL_BLOCK_ID;
    free_list.set_size(cache->serializer->max_block_id());
    for (block_id_t i = 0; i < cache->serializer->max_block_id(); i++) {
        if (cache->serializer->block_in_use(i)) {
            free_list[i] = BLOCK_IN_USE;
        } else {
            free_list[i] = first_block;
            first_block = i;
        }
    }
    
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
    
    free_list[id] = BLOCK_IN_USE;
    return id;
}

template<class mc_config_t>
void array_free_list_t<mc_config_t>::release_block_id(block_id_t id) {
    
    assert(free_list[id] == BLOCK_IN_USE);
    
    free_list[id] = first_block;
    first_block = id;
}
