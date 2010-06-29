
#ifndef __MIRRORED_CACHE_HPP__
#define __MIRRORED_CACHE_HPP__

#include <queue>
#include "event_queue.hpp"
#include "cpu_context.hpp"
#include "concurrency/access.hpp"
#include "buffer_cache/callbacks.hpp"

// This cache doesn't actually do any operations itself. Instead, it
// provides a framework that collects all components of the cache
// (memory allocation, page lookup, page replacement, writeback, etc.)
// into a coherent whole. This allows easily experimenting with
// various components of the cache to improve performance.

/* Buffer class. */
// TODO: make sure we use small object allocator for buf_t
template <class config_t>
class buf : public config_t::writeback_t::buf_t,
            public config_t::page_repl_t::buf_t,
            public config_t::concurrency_t::buf_t {
public:
    typedef typename config_t::serializer_t serializer_t;
    typedef typename serializer_t::block_id_t block_id_t;
    typedef typename config_t::transaction_t transaction_t;
    typedef typename config_t::concurrency_t concurrency_t;
    typedef typename config_t::cache_t cache_t;
    typedef typename config_t::node_t node_t;
    typedef block_available_callback<config_t> block_available_callback_t;

    buf(transaction_t *transaction, block_id_t block_id);
    ~buf();

    void release(void *state); /* TODO(NNW): Callback should be removed. */

    void *ptr() { return data; } /* TODO(NNW) We may want const version. */
    node_t *node();

    void set_cached(bool _cached) { cached = _cached; }
    bool is_cached() const { return cached; }

    void set_dirty() { config_t::writeback_t::buf_t::set_dirty(this); }

    transaction_t *get_transaction() { return transaction; }

    // Callback API
    void add_lock_callback(block_available_callback_t *callback);
    void add_load_callback(block_available_callback_t *callback);
    void notify_on_lock();
    void notify_on_load();

private:
    transaction_t *transaction; // TODO(NNW): We need to invalidate this.
    cache_t *cache;
    block_id_t block_id;
    bool cached; /* Is data valid, or are we waiting for a read? */
    void *data;
    std::queue<block_available_callback_t*> lock_callbacks;
    std::queue<block_available_callback_t*> load_callbacks;
};

/* Transaction class. */
template <class config_t>
class transaction {
public:
    typedef typename config_t::serializer_t serializer_t;
    typedef typename config_t::concurrency_t concurrency_t;
    typedef typename serializer_t::block_id_t block_id_t;
    typedef typename config_t::cache_t cache_t;
    typedef typename config_t::buf_t buf_t;
    typedef block_available_callback<config_t> block_available_callback_t;

    explicit transaction(cache_t *cache);
    ~transaction();

    cache_t *get_cache() const { return cache; }

    bool commit(void *state);
    //void abort(void *state); // TODO: We need this someday, but not yet.

    buf_t *acquire(block_id_t block_id, access_t mode,
                   block_available_callback_t *callback);
    buf_t *allocate(block_id_t *new_block_id);

private:
    cache_t *cache;
    bool open;

public:
#ifndef NDEBUG
    event_queue_t *event_queue; // For asserts that we haven't changed CPU.
#endif
};

template <class config_t>
struct mirrored_cache_t : public config_t::serializer_t,
                          public config_t::buffer_alloc_t,
                          public config_t::page_map_t,
                          public config_t::page_repl_t,
                          public config_t::writeback_t
{
public:
    typedef typename config_t::serializer_t serializer_t;
    typedef typename serializer_t::block_id_t block_id_t;
    typedef typename config_t::page_repl_t page_repl_t;
    typedef typename config_t::writeback_t writeback_t;
    typedef typename config_t::transaction_t transaction_t;
    typedef typename config_t::buffer_alloc_t buffer_alloc_t;
    typedef typename config_t::concurrency_t concurrency_t;
    typedef typename config_t::page_map_t page_map_t;
    typedef typename config_t::buf_t buf_t;

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
    ~mirrored_cache_t();

    void start() {
        writeback_t::start();
    }

    // Transaction API
    transaction_t *begin_transaction();

    void aio_complete(buf_t *buf, void *block, bool written);

private:
    using page_map_t::ft_map;
};

#include "buffer_cache/mirrored_impl.hpp"

#endif // __MIRRORED_CACHE_HPP__

