
#ifndef __BUFFER_CACHE_PAGE_MAP_ARRAY_HPP__
#define __BUFFER_CACHE_PAGE_MAP_ARRAY_HPP__

#include "containers/two_level_array.hpp"

#define BUFFERS_PER_CHUNK (1 << 16)

// Nobody will have more than 500 GB of memory. (Famous last words...)
#define MAX_SANE_MEMORY_SIZE (500L * (1 << 30))

template <class config_t>
struct array_map_t {

public:
    typedef typename config_t::serializer_t serializer_t;
    typedef typename config_t::buf_t buf_t;
    
public:
    array_map_t() { }
    ~array_map_t() {
        assert(array.size() == 0);
    }

    buf_t* find(block_id_t block_id) {
        return array.get(block_id);
    }
    
    void insert(buf_t *block) {
        assert(!array.get(block->get_block_id()));
        array.set(block->get_block_id(), block);
    }
    
    void erase(buf_t *block) {
        assert(array.get(block->get_block_id()));
        array.set(block->get_block_id(), NULL);
    }

private:
    two_level_array_t<buf_t, MAX_SANE_MEMORY_SIZE / BTREE_BLOCK_SIZE, BUFFERS_PER_CHUNK > array;
};

#endif // __BUFFER_CACHE_PAGE_MAP_ARRAY_HPP__

