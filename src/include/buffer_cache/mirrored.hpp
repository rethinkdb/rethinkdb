
#ifndef __MIRRORED_CACHE_HPP__
#define __MIRRORED_CACHE_HPP__

#include "event_queue.hpp"
#include "cpu_context.hpp"
#include "concurrency/access.hpp"

// This cache doesn't actually do any operations itself. Instead, it
// provides a framework that collects all components of the cache
// (memory allocation, page lookup, page replacement, writeback, etc.)
// into a coherent whole. This allows easily experimenting with
// various components of the cache to improve performance.

template <class config_t>
struct aio_context : public alloc_mixin_t<tls_small_obj_alloc_accessor<typename config_t::alloc_t>, aio_context<config_t> > {
    typedef typename config_t::serializer_t serializer_t;
    typedef typename serializer_t::block_id_t block_id_t;
    typedef typename config_t::cache_t::buf_t buf_t;

    buf_t *buf;
    void *user_state;
    block_id_t block_id;
#ifndef NDEBUG
    // We use this member in debug mode to ensure all operations
    // associated with the context occur on the same event queue.
    event_queue_t *event_queue;
#endif
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
    typedef typename config_t::node_t node_t;
    typedef aio_context<config_t> aio_context_t;
    class transaction_t;

    /* Buffer class. */
    class buf_t : public writeback_t::buf_t,
                  public page_repl_t::buf_t,
                  public concurrency_t::buf_t
    {
    public:
        buf_t(transaction_t *transaction, block_id_t block_id, void *data);

        void set_cached(bool _cached) { cached = _cached; }
        bool is_cached() const { return cached; }
        void *ptr() { return data; } /* XXX Should we have const version? */
        void release(void *state); /* XXX: This callback needs to be removed. */

        node_t *node(); /* XXX Return data as the correct node_t type. */

    private:
        transaction_t *transaction;
        block_id_t block_id;
        bool cached; /* Is data valid, or are we waiting for a read? */
        void *data;
    };

    /* Transaction class. */
    class transaction_t {
    public:
        explicit transaction_t(mirrored_cache_t *cache);
        ~transaction_t();

        mirrored_cache_t *get_cache() const { return cache; }

        void commit(/*void *state*/); /* XXX This will require a callback. */
        //void abort(void *state); // TODO: We need this someday, but not yet.

        buf_t *acquire(block_id_t, void *state, access_t mode);
        buf_t *allocate(block_id_t *new_block_id);

    private:
        mirrored_cache_t *cache;
        bool open;
#ifndef NDEBUG
        event_queue_t *event_queue; // For asserts that we haven't changed CPU.
#endif
    };

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
        writeback_t(this) {}

    void start() {
        writeback_t::start();
    }

    // Transaction API
    transaction_t *begin_transaction();

    void aio_complete(aio_context_t *ctx, void *block, bool written);
};

#include "buffer_cache/mirrored_impl.hpp"

#endif // __MIRRORED_CACHE_HPP__

