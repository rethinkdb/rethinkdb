#include "buffer_cache/mock.hpp"
#include "arch/arch.hpp"

/* Internal buf object */

struct internal_buf_t
{
    mock_cache_t *cache;
    block_id_t block_id;
    repli_timestamp subtree_recency;
    void *data;
    rwi_lock_t lock;
    
    internal_buf_t(mock_cache_t *_cache, block_id_t _block_id, repli_timestamp _subtree_recency)
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
        
        rassert(cache->bufs[block_id] == this);
        cache->bufs[block_id] = NULL;
        
        delete this;
    }
};

/* Buf */

void mock_buf_t::on_lock_available() {
    random_delay(cb, &mock_block_available_callback_t::on_block_available, this);
}

block_id_t mock_buf_t::get_block_id() {
    return internal_buf->block_id;
}

const void *mock_buf_t::get_data_read() {
    return internal_buf->data;
}

void *mock_buf_t::get_data_major_write() {
    rassert(access == rwi_write);
    dirty = true;
    return internal_buf->data;
}

void mock_buf_t::apply_patch(buf_patch_t *patch) {
    rassert(access == rwi_write);

    patch->apply_to_buf((char*)internal_buf->data);
    dirty = true;

    delete patch;
}

patch_counter_t mock_buf_t::get_next_patch_counter() {
    return 0;
}

void mock_buf_t::set_data(void *dest, const void *src, const size_t n) {
    size_t offset = reinterpret_cast<const char *>(dest) - reinterpret_cast<const char *>(internal_buf->data);
    apply_patch(new memcpy_patch_t(internal_buf->block_id, get_next_patch_counter(), offset, reinterpret_cast<const char *>(src), n));
}

void mock_buf_t::move_data(void *dest, const void *src, const size_t n) {
    size_t dest_offset = reinterpret_cast<const char *>(dest) - reinterpret_cast<const char *>(internal_buf->data);
    size_t src_offset = reinterpret_cast<const char *>(src) - reinterpret_cast<const char *>(internal_buf->data);
    apply_patch(new memmove_patch_t(internal_buf->block_id, get_next_patch_counter(), dest_offset, src_offset, n));
}

void mock_buf_t::mark_deleted(UNUSED bool write_null) {
    // write_null is ignored for the mock cache.
    rassert(access == rwi_write);
    deleted = true;
}

void mock_buf_t::touch_recency(repli_timestamp timestamp) {
    rassert(access == rwi_write);
    internal_buf->subtree_recency = timestamp;
}

void mock_buf_t::release() {
    internal_buf->lock.unlock();
    if (deleted) internal_buf->destroy();
    delete this;
}

// TODO: Add notiont of recency_dirty
bool mock_buf_t::is_dirty() {
    return dirty;
}

mock_buf_t::mock_buf_t(internal_buf_t *internal_buf, access_t access)
    : internal_buf(internal_buf), access(access), dirty(false), deleted(false) {
}

/* Transaction */

bool mock_transaction_t::commit(mock_transaction_commit_callback_t *callback) {
    switch (access) {
        case rwi_read:
            delete this;
            return true;
        case rwi_write:
            if (maybe_random_delay(this, &mock_transaction_t::finish_committing, callback)) {
                finish_committing(NULL);
                return true;
            } else {
                return false;
            }
        case rwi_read_outdated_ok:
        case rwi_intent:
        case rwi_upgrade:
        default:
            unreachable("Bad access");
    }
}

void mock_transaction_t::finish_committing(mock_transaction_commit_callback_t *cb) {
    if (cb) cb->on_txn_commit(this);
    delete this;
}

mock_buf_t *mock_transaction_t::acquire(block_id_t block_id, access_t mode, mock_block_available_callback_t *callback, UNUSED bool should_load) {
    // should_load is ignored for the mock cache.
    if (mode == rwi_write) rassert(this->access == rwi_write);
    
    rassert(block_id < cache->bufs.get_size());
    internal_buf_t *internal_buf = cache->bufs[block_id];
    rassert(internal_buf);

    if (!(mode == rwi_read || mode == rwi_read_outdated_ok)) {
        internal_buf->subtree_recency = recency_timestamp;
    }

    mock_buf_t *buf = new mock_buf_t(internal_buf, mode);
    if (internal_buf->lock.lock(mode, buf)) {
        if (maybe_random_delay(callback, &mock_block_available_callback_t::on_block_available, buf)) {
            return buf;
        } else {
            return NULL;
        }
    } else {
        buf->cb = callback;
        return NULL;
    }
}

mock_buf_t *mock_transaction_t::allocate() {
    rassert(this->access == rwi_write);
    
    block_id_t block_id = cache->bufs.get_size();
    cache->bufs.set_size(block_id + 1);
    internal_buf_t *internal_buf = new internal_buf_t(cache, block_id, recency_timestamp);
    cache->bufs[block_id] = internal_buf;
    bool locked __attribute__((unused)) = internal_buf->lock.lock(rwi_write, NULL);
    rassert(locked);
    
    mock_buf_t *buf = new mock_buf_t(internal_buf, rwi_write);
    return buf;
}

void mock_transaction_t::get_subtree_recencies(block_id_t *block_ids, size_t num_block_ids, repli_timestamp *recencies_out) {
    for (size_t i = 0; i < num_block_ids; ++i) {
        rassert(block_ids[i] < cache->bufs.get_size());
        internal_buf_t *internal_buf = cache->bufs[block_ids[i]];
        rassert(internal_buf);
        recencies_out[i] = internal_buf->subtree_recency;
    }
}

mock_transaction_t::mock_transaction_t(mock_cache_t *_cache, access_t _access, repli_timestamp _recency_timestamp)
    : cache(_cache), access(_access), recency_timestamp(_recency_timestamp) {
    cache->transaction_counter.acquire();
}

mock_transaction_t::~mock_transaction_t() {
    cache->transaction_counter.release();
}

/* Cache */

// TODO: Why do we take a static_config if we don't use it?
// (I.i.r.c. we have a similar situation in the mirrored cache.)

void mock_cache_t::create(
    translator_serializer_t *serializer,
    UNUSED mirrored_cache_static_config_t *static_config)
{
    on_thread_t switcher(serializer->home_thread);

    void *superblock = serializer->malloc();
    bzero(superblock, serializer->get_block_size().value());
    translator_serializer_t::write_t write = translator_serializer_t::write_t::make(
        SUPERBLOCK_ID, repli_timestamp::invalid, superblock, false, NULL);

    struct : public serializer_t::write_txn_callback_t, public cond_t {
        void on_serializer_write_txn() { pulse(); }
    } cb;
    if (!serializer->do_write(&write, 1, &cb)) cb.wait();

    serializer->free(superblock);
}

// dynamic_config is unused because this is a mock cache and the
// configuration parameters don't apply.
mock_cache_t::mock_cache_t(
    translator_serializer_t *serializer,
    UNUSED mirrored_cache_config_t *dynamic_config)
    : serializer(serializer), block_size(serializer->get_block_size())
{
    on_thread_t switcher(serializer->home_thread);

    struct : public serializer_t::read_callback_t, public drain_semaphore_t {
        void on_serializer_read() { release(); }
    } read_cb;

    block_id_t end_block_id = serializer->max_block_id();
    bufs.set_size(end_block_id, NULL);
    for (block_id_t i = 0; i < end_block_id; i++) {
        if (serializer->block_in_use(i)) {
            internal_buf_t *internal_buf = new internal_buf_t(this, i, serializer->get_recency(i));
            bufs[i] = internal_buf;
            if (!serializer->do_read(i, internal_buf->data, &read_cb)) read_cb.acquire();
        }
    }

    /* Block until all readers are done */
    read_cb.drain();
}

mock_cache_t::~mock_cache_t() {

    /* Wait for all transactions to complete */
    transaction_counter.drain();

    {
        on_thread_t thread_switcher(serializer->home_thread);

        std::vector<translator_serializer_t::write_t> writes;
        for (block_id_t i = 0; i < bufs.get_size(); i++) {
            writes.push_back(translator_serializer_t::write_t::make(
                i, bufs[i] ? bufs[i]->subtree_recency : repli_timestamp::invalid,
                bufs[i] ? bufs[i]->data : NULL,
                true, NULL));
        }

        struct : public serializer_t::write_txn_callback_t, public cond_t {
            void on_serializer_write_txn() { pulse(); }
        } cb;
        if (!serializer->do_write(writes.data(), writes.size(), &cb)) cb.wait();
    }

    for (block_id_t i = 0; i < bufs.get_size(); i++) {
        if (bufs[i]) delete bufs[i];
    }
}

block_size_t mock_cache_t::get_block_size() {
    return block_size;
}

mock_transaction_t *mock_cache_t::begin_transaction(access_t access, UNUSED int expected_change_count, repli_timestamp recency_timestamp, mock_transaction_begin_callback_t *callback) {
    
    mock_transaction_t *txn = new mock_transaction_t(this, access, recency_timestamp);
    
    switch (access) {
        case rwi_read:
            return txn;
        case rwi_write:
            if (maybe_random_delay(callback, &mock_transaction_begin_callback_t::on_txn_begin, txn)) {
                return txn;
            } else {
                return NULL;
            }
        case rwi_read_outdated_ok:
        case rwi_intent:
        case rwi_upgrade:
        default:
            unreachable("Bad access.");
    }
}
