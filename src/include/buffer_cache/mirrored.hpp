
#ifndef __MIRRORED_CACHE_HPP__
#define __MIRRORED_CACHE_HPP__

// This cache doesn't actually do any operations itself. Instead, it
// provides a framework that collects all components of the cache
// (memory allocation, page lookup, page replacement, writeback,
// concurrency control, etc.) into a coherent whole. This allows
// easily experimenting with various components of the cache to
// improve performance.

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
    typedef typename config_t::fsm_t fsm_t;

public:
    mirrored_cache_t(size_t _block_size) : serializer_t(_block_size), page_repl_t(_block_size, _block_size * 100) {}

    void* allocate(block_id_t *block_id) {
        concurrency_t::begin_allocate(block_id);
        
        *block_id = serializer_t::gen_block_id();
        void *block = buffer_alloc_t::malloc(serializer_t::block_size);
        page_map_t::set(*block_id, block);
        page_repl_t::pin(*block_id);
        
        concurrency_t::end_allocate();
        return block;
    }
    
    void* acquire(block_id_t block_id, fsm_t *state) {
        concurrency_t::begin_acquire(block_id, state);

        void *block = page_map_t::find(block_id);
        if(!block) {
            void *buf = buffer_alloc_t::malloc(serializer_t::block_size);
            do_read(block_id, buf, state);
        } else {
            page_repl_t::pin(block_id);
        }

        concurrency_t::end_acquire();
        return block;
    }

    block_id_t release(block_id_t block_id, void *block, bool dirty, fsm_t *state) {
        concurrency_t::begin_release(block_id, block, dirty, state);

        block_id_t new_block_id = block_id;
        if(dirty) {
            new_block_id = writeback_t::mark_dirty(block_id);
            page_repl_t::unpin(block_id);
        } else {
            page_repl_t::unpin(block_id);
        }

        concurrency_t::end_release();
        return new_block_id;
    }

    void aio_complete(block_id_t block_id, void *block, bool written) {
        concurrency_t::begin_aio_complete(block_id, block, written);
        
        if(written) {
            page_repl_t::unpin(block_id);
        } else {
            page_map_t::set(block_id, block);
            page_repl_t::pin(block_id);
        }

        concurrency_t::end_aio_complete();
    }
};

#endif // __MIRRORED_CACHE_HPP__

