// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "buffer_cache/mock.hpp"

#include "arch/arch.hpp"
#include "arch/random_delay.hpp"
#include "serializer/serializer.hpp"

/* Internal buf object */

class internal_buf_t {
public:
    mock_cache_t *cache;
    block_id_t block_id;
    repli_timestamp_t subtree_recency;
    void *data;
    rwi_lock_t lock;

    internal_buf_t(mock_cache_t *_cache, block_id_t _block_id, repli_timestamp_t _subtree_recency)
        : cache(_cache), block_id(_block_id), subtree_recency(_subtree_recency),
          data(cache->serializer->malloc()) {
        rassert(data);
        bzero(data, cache->block_size.value());
    }

    ~internal_buf_t() {
        cache->serializer->free(data);
    }

    void destroy() {
        rassert(!lock.locked());

        rassert(cache->bufs->get(block_id) == this);
        cache->bufs->get(block_id) = NULL;

        delete this;
    }
};

/* Buf */

block_id_t mock_buf_lock_t::get_block_id() const {
    return internal_buf->block_id;
}

const void *mock_buf_lock_t::get_data_read() const {
    rassert(acquired);
    rassert(internal_buf->data != NULL);
    return internal_buf->data;
}

void *mock_buf_lock_t::get_data_major_write() {
    rassert(access == rwi_write);
    dirty = true;
    return internal_buf->data;
}

void mock_buf_lock_t::apply_patch(buf_patch_t *patch) {
    rassert(access == rwi_write);

    patch->apply_to_buf(reinterpret_cast<char *>(internal_buf->data), internal_buf->cache->block_size);
    dirty = true;

    delete patch;
}

patch_counter_t mock_buf_lock_t::get_next_patch_counter() {
    return 0;
}

void mock_buf_lock_t::set_data(void *dest, const void *src, const size_t n) {
    size_t offset = reinterpret_cast<const char *>(dest) - reinterpret_cast<const char *>(internal_buf->data);
    apply_patch(new memcpy_patch_t(internal_buf->block_id, get_next_patch_counter(), offset, reinterpret_cast<const char *>(src), n));
}

void mock_buf_lock_t::move_data(void *dest, const void *src, const size_t n) {
    size_t dest_offset = reinterpret_cast<const char *>(dest) - reinterpret_cast<const char *>(internal_buf->data);
    size_t src_offset = reinterpret_cast<const char *>(src) - reinterpret_cast<const char *>(internal_buf->data);
    apply_patch(new memmove_patch_t(internal_buf->block_id, get_next_patch_counter(), dest_offset, src_offset, n));
}

void mock_buf_lock_t::mark_deleted() {
    rassert(access == rwi_write);
    deleted = true;
}

void mock_buf_lock_t::touch_recency(repli_timestamp_t timestamp) {
    rassert(access == rwi_write);
    internal_buf->subtree_recency = timestamp;
}

void mock_buf_lock_t::release() {
    internal_buf->lock.unlock();
    if (deleted) internal_buf->destroy();
    acquired = false;
}

void mock_buf_lock_t::release_if_acquired() {
    if (is_acquired()) {
        release();
    }
}

// TODO: Add notiont of recency_dirty
bool mock_buf_lock_t::is_deleted() const {
    return deleted;
}

/* Transaction */
mock_buf_lock_t::mock_buf_lock_t() :
    internal_buf(NULL),
    access(rwi_read),
    dirty(false),
    deleted(false),
    acquired(false)
{ }

mock_buf_lock_t::mock_buf_lock_t(mock_transaction_t *txn, block_id_t block_id, access_t mode, UNUSED buffer_cache_order_mode_t order_mode, lock_in_line_callback_t *call_when_in_line) :
    internal_buf(txn->cache->bufs->get(block_id)),
    access(mode),
    dirty(false),
    deleted(false),
    acquired(true)
{
    assert_thread();
    rassert(mode != rwi_write || txn->access == rwi_write);
    rassert(block_id < txn->cache->bufs->get_size());
    rassert(internal_buf);

    internal_buf->lock.co_lock(mode == rwi_read_outdated_ok ? rwi_read : mode, call_when_in_line);

    if (!(mode == rwi_read || mode == rwi_read_outdated_ok || mode == rwi_read_sync)) {
        internal_buf->subtree_recency = txn->recency_timestamp;
    }

    nap(5);   // TODO: We should nap for a random time like `maybe_random_delay()` does
}

mock_buf_lock_t::~mock_buf_lock_t() {
    release_if_acquired();
}

mock_buf_lock_t::mock_buf_lock_t(mock_transaction_t *txn) :
    internal_buf(NULL),
    access(txn->access),
    dirty(false),
    deleted(false),
    acquired(true)
{
    rassert(access == rwi_write);

    block_id_t block_id = txn->cache->bufs->get_size();
    txn->cache->bufs->set_size(block_id + 1);
    internal_buf = new internal_buf_t(txn->cache, block_id, txn->recency_timestamp);
    txn->cache->bufs->get(block_id) = internal_buf;
    DEBUG_VAR bool locked = internal_buf->lock.lock(rwi_write, NULL);
    rassert(locked);
}

void mock_transaction_t::get_subtree_recencies(block_id_t *block_ids, size_t num_block_ids, repli_timestamp_t *recencies_out, get_subtree_recencies_callback_t *cb) {
    for (size_t i = 0; i < num_block_ids; ++i) {
        rassert(block_ids[i] < cache->bufs->get_size());
        internal_buf_t *internal_buf = cache->bufs->get(block_ids[i]);
        rassert(internal_buf);
        recencies_out[i] = internal_buf->subtree_recency;
    }
    cb->got_subtree_recencies();
}

mock_transaction_t::mock_transaction_t(mock_cache_t *_cache, access_t _access, UNUSED int expected_change_count, repli_timestamp_t _recency_timestamp, order_token_t _order_token)
    : cache(_cache), order_token(_order_token), access(_access), recency_timestamp(_recency_timestamp),
      keepalive(_cache->transaction_counter.get()) {
    coro_fifo_acq_t write_throttle_acq;
    if (is_write_mode(access)) {
        write_throttle_acq.enter(&cache->transaction_constructor_coro_fifo_);
        nap(5);   // TODO: Nap for a random amount of time.
    }
}

mock_transaction_t::~mock_transaction_t() {
    assert_thread();
    if (access == rwi_write) nap(5);   // TODO: Nap for a random amount of time.
}

/* Cache */

// TODO: Why do we take a static_config if we don't use it?
// (I.i.r.c. we have a similar situation in the mirrored cache.)

void mock_cache_t::create(serializer_t *serializer, UNUSED mirrored_cache_static_config_t *static_config) {
    on_thread_t switcher(serializer->home_thread());

    void *superblock = serializer->malloc();
    bzero(superblock, serializer->get_block_size().value());

    serializer_transaction_t ser_txn;

    index_write_op_t op(SUPERBLOCK_ID);
    op.token = serializer->block_write(&ser_txn, superblock, SUPERBLOCK_ID, DEFAULT_DISK_ACCOUNT);
    op.recency = repli_timestamp_t::invalid;
    serializer_index_write(serializer, &ser_txn, op, DEFAULT_DISK_ACCOUNT);

    serializer->free(superblock);
}

// dynamic_config is unused because this is a mock cache and the
// configuration parameters don't apply.
mock_cache_t::mock_cache_t(serializer_t *_serializer, UNUSED mirrored_cache_config_t *dynamic_config, UNUSED perfmon_collection_t *parent)
    : serializer(_serializer), transaction_counter(new auto_drainer_t),
      block_size(_serializer->get_block_size()),
      bufs(new segmented_vector_t<internal_buf_t *, MAX_BLOCK_ID>) {

    on_thread_t switcher(serializer->home_thread());

    struct read_callback_t : public iocallback_t, public cond_t {
        read_callback_t() : count(0) { }
        void on_io_complete() {
            count--;
            if (count == 0) pulse();
        }
        int count;
    } read_cb;

    block_id_t end_block_id = serializer->max_block_id();
    bufs->set_size(end_block_id, NULL);
    for (block_id_t i = 0; i < end_block_id; i++) {
        if (!serializer->get_delete_bit(i)) {
            internal_buf_t *internal_buf = bufs->get(i) = new internal_buf_t(this, i, serializer->get_recency(i));
            read_cb.count++;
            serializer->block_read(serializer->index_read(i), internal_buf->data, DEFAULT_DISK_ACCOUNT, &read_cb);
        }
    }

    /* Block until all readers are done */
    read_cb.wait();
}

struct mock_cb_t : public iocallback_t, public cond_t {
    void on_io_complete() { pulse(); }
};

mock_cache_t::~mock_cache_t() {
    /* Wait for all transactions to complete */
    transaction_counter.reset();

    {
        on_thread_t thread_switcher(serializer->home_thread());
        std::vector<serializer_write_t> writes;
        for (block_id_t i = 0; i < bufs->get_size(); i++)
            writes.push_back(
                             bufs->get(i)
                             ? serializer_write_t::make_update(i, bufs->get(i)->subtree_recency, bufs->get(i)->data)
                             : serializer_write_t::make_delete(i));
        do_writes(serializer, writes, DEFAULT_DISK_ACCOUNT);
    }

    for (block_id_t i = 0; i < bufs->get_size(); i++) {
        delete bufs->get(i);
    }
}

block_size_t mock_cache_t::get_block_size() {
    return block_size;
}

bool mock_cache_t::offer_read_ahead_buf(UNUSED block_id_t block_id, UNUSED void *buf, UNUSED const intrusive_ptr_t<standard_block_token_t>& token, UNUSED repli_timestamp_t recency_timestamp) {
    // We never use read-ahead.
    return false;
}

bool mock_cache_t::contains_block(UNUSED block_id_t id) {
    return true;    // TODO (maybe) write a more sensible implementation
}

void mock_buf_lock_t::swap(mock_buf_lock_t &swapee) {
    assert_thread();
    swapee.assert_thread();
    std::swap(internal_buf, swapee.internal_buf);
    std::swap(access, swapee.access);
    std::swap(dirty, swapee.dirty);
    std::swap(deleted, swapee.deleted);
    std::swap(acquired, swapee.acquired);
#ifndef NDEBUG
    std::swap(real_home_thread, swapee.real_home_thread);
#endif
}

bool mock_buf_lock_t::is_acquired() const {
    return acquired;
}

repli_timestamp_t mock_buf_lock_t::get_recency() const {
    return internal_buf->subtree_recency;
}

