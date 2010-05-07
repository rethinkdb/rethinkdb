
#ifndef __BUFFER_CACHE_IMMEDIATE_WRITEBACK_HPP__
#define __BUFFER_CACHE_IMMEDIATE_WRITEBACK_HPP__

template <class config_t>
struct immediate_writeback_t {
public:
    typedef typename config_t::serializer_t serializer_t;
    typedef typename config_t::concurrency_t concurrency_t;
    typedef typename serializer_t::block_id_t block_id_t;
    typedef typename config_t::cache_t cache_t;
    typedef typename config_t::conn_fsm_t conn_fsm_t;
    
public:
    immediate_writeback_t(serializer_t *_serializer, concurrency_t *_concurrency)
        : serializer(_serializer),
          concurrency(_concurrency)
        {}
    
    block_id_t mark_dirty(block_id_t block_id, void *block, conn_fsm_t *state) {
        concurrency->begin_mark_dirty(block_id, block, state);
        block_id_t new_block_id = serializer->do_write(block_id, block, state);
        concurrency->end_mark_dirty();
        
        return new_block_id;
    }

    bool is_dirty(block_id_t block_id) {
        check("TODO: implement immediate_writeback_t::is_dirty", 1);
    }

private:
    serializer_t *serializer;
    concurrency_t *concurrency;
};

#endif // __BUFFER_CACHE_IMMEDIATE_WRITEBACK_HPP__

