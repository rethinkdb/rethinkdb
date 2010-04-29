
#ifndef __BUFFER_CACHE_IMMEDIATE_WRITEBACK_HPP__
#define __BUFFER_CACHE_IMMEDIATE_WRITEBACK_HPP__

template <class config_t>
struct immediate_writeback_t {
public:
    typedef typename config_t::serializer_t serializer_t;
    typedef typename serializer_t::block_id_t block_id_t;
    
public:
    block_id_t mark_dirty(block_id_t block_id) {
        // Set the dirty bit
        // Keep a list of dirty blocks
        return block_id;
    }

    bool is_dirty(block_id_t block_id) {
    }
};

#endif // __BUFFER_CACHE_IMMEDIATE_WRITEBACK_HPP__

