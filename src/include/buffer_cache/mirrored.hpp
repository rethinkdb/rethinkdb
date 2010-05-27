
#ifndef __MIRRORED_CACHE_HPP__
#define __MIRRORED_CACHE_HPP__

// This cache doesn't actually do any operations itself. Instead, it
// provides a framework that collects all components of the cache
// (memory allocation, page lookup, page replacement, writeback,
// concurrency control, etc.) into a coherent whole. This allows
// easily experimenting with various components of the cache to
// improve performance.

template <class config_t>
struct aio_context {
    typedef typename config_t::alloc_t alloc_t;
    typedef typename config_t::serializer_t serializer_t;
    typedef typename serializer_t::block_id_t block_id_t;

    alloc_t *alloc;
    void *user_state;
    block_id_t block_id;
};

template <class config_t>
struct mirrored_cache_t : public config_t::serializer_t,
                          public config_t::buffer_alloc_t,
                          public config_t::page_map_t,
                          public config_t::page_repl_t,
                          public config_t::writeback_t,
                          public config_t::concurrency_t
{
public:
    typedef typename config_t::serializer_t serializer_t;
    typedef typename serializer_t::block_id_t block_id_t;
    typedef typename config_t::page_repl_t page_repl_t;
    typedef typename config_t::writeback_t writeback_t;
    typedef typename config_t::concurrency_t concurrency_t;
    typedef typename config_t::buffer_alloc_t buffer_alloc_t;
    typedef typename config_t::page_map_t page_map_t;
    typedef typename config_t::btree_fsm_t btree_fsm_t;
    typedef typename config_t::conn_fsm_t conn_fsm_t;
    typedef typename concurrency_t::transaction_t transaction_t;
    typedef aio_context<config_t> aio_context_t;

public:
    // TODO: how do we design communication between cache policies?
    // Should they all have access to the cache, or should they only
    // be given access to each other as necessary? The first is more
    // flexible as anyone can access anyone else, but encourages too
    // many dependencies. The second is more strict, but might not be
    // extensible when some policy implementation requires access to
    // components it wasn't originally given.
    mirrored_cache_t(size_t _block_size, size_t _max_size) : 
        serializer_t(_block_size),
        page_repl_t(_block_size, _max_size, this, this),
        writeback_t(this)
        {
            // TODO: can we move the printing out of here?
            printf("Total RAM: %ldMB\n", get_total_ram() / 1024 / 1024);
            printf("Free RAM: %ldMB (%.2f%%)\n",
                   get_available_ram() / 1024 / 1024,
                   (double)get_available_ram() / (double)get_total_ram() * 100.0f);
            printf("Max cache size: %ldMB\n",
                   _max_size / 1024 / 1024);
        }

    // Transaction API
    transaction_t* begin_transaction(conn_fsm_t *state) {
        return concurrency_t::begin_transaction(state);
    }
    void end_transaction(transaction_t* transaction) {
        concurrency_t::end_transaction(transaction);
    }

    // TODO: right now we don't honor the fact that concurrency class
    // might return a -1 on acquire. We need to implement this
    // functionality.

    // TODO: Most of the operations below depend on the concurrency
    // being implemented as the BKL. Fix this.

    // TODO: each operation can only be performed within a
    // transaction. Much the API nicer (from the OOP/C++ point of
    // view), and move the following methods into a separate
    // transaction class.
    void* allocate(transaction_t* tm, block_id_t *block_id) {
        *block_id = serializer_t::gen_block_id();
        void *block = buffer_alloc_t::malloc(serializer_t::block_size);
        page_map_t::set(*block_id, block);
        page_repl_t::pin(*block_id);
        
        concurrency_t::acquire(tm, *block_id);
        return block;
    }
    
    void* acquire(transaction_t* tm, block_id_t block_id, btree_fsm_t *state) {
        concurrency_t::acquire(tm, block_id);

        void *block = page_map_t::find(block_id);
        if(!block) {
            void *buf = buffer_alloc_t::malloc(serializer_t::block_size);
            aio_context_t *ctx = state->netfsm->event_queue->
                alloc.template malloc<aio_context_t>();
            ctx->alloc = &state->netfsm->event_queue->alloc;
            ctx->user_state = state;
            ctx->block_id = block_id;

            do_read(state->netfsm->event_queue, block_id, buf, ctx);
        } else {
            page_repl_t::pin(block_id);
        }

        return block;
    }

    block_id_t release(transaction_t* tm, block_id_t block_id, void *block, bool dirty, btree_fsm_t *state) {

        block_id_t new_block_id = block_id;
        if(dirty) {
            new_block_id = writeback_t::mark_dirty(block_id, block, state);
            // Already pinned by 'acquire'. Will unpin in aio_complete
            // when the block is written
        } else {
            page_repl_t::unpin(block_id);
        }

        concurrency_t::release(tm, block_id, block, dirty);
        return new_block_id;
    }

    void aio_complete(aio_context_t *ctx, void *block, bool written) {
        block_id_t block_id = ctx->block_id;

        ctx->alloc->free(ctx);
        if(written) {
            page_repl_t::unpin(block_id);
        } else {
            page_map_t::set(block_id, block);
            page_repl_t::pin(block_id);
        }
    }
};

#endif // __MIRRORED_CACHE_HPP__

