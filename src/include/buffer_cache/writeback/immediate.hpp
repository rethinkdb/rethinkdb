
#ifndef __BUFFER_CACHE_IMMEDIATE_WRITEBACK_HPP__
#define __BUFFER_CACHE_IMMEDIATE_WRITEBACK_HPP__

// TODO: we should change writeback API to operate within a
// transaction. This way we can hold off flushing until a transaction
// completes. This will also let us use scatter/gather IO even in the
// case of immediate writeback.

template <class config_t>
struct immediate_writeback_t {
public:
    typedef typename config_t::serializer_t serializer_t;
    typedef typename serializer_t::block_id_t block_id_t;
    typedef typename config_t::cache_t cache_t;
    typedef typename config_t::btree_fsm_t btree_fsm_t;
    typedef aio_context<config_t> aio_context_t;
    
public:
    immediate_writeback_t(serializer_t *_serializer)
        : serializer(_serializer)
        {}
    
    block_id_t mark_dirty(event_queue_t *event_queue, block_id_t block_id, void *block, void *state) {
        aio_context_t *ctx = new (&event_queue->alloc) aio_context_t();
        ctx->user_state = state;
        ctx->block_id = block_id;

        block_id_t new_block_id;
        new_block_id = serializer->do_write(event_queue, block_id, block, ctx);
        
        return new_block_id;
    }

    bool is_dirty(block_id_t block_id) {
        check("TODO: implement immediate_writeback_t::is_dirty", 1);
    }

private:
    serializer_t *serializer;
};

#endif // __BUFFER_CACHE_IMMEDIATE_WRITEBACK_HPP__

