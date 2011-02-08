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
    
    internal_buf_t(mock_cache_t *cache, block_id_t block_id)
        : cache(cache), block_id(block_id), subtree_recency(
                                                            // cache->serializer->get_recency(block_id)
                                                            // TODO: We can't call cache->serializer->get_recency because it's on a different core.  Rearchitect this when we start actually using timestamps.
                                                            repli_timestamp::invalid
), data(cache->serializer->malloc()) {
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

void mock_buf_t::apply_patch(buf_patch_t& patch) {
    rassert(access == rwi_write);

    patch.apply_to_buf((char*)internal_buf->data);
    dirty = true;

    delete &patch;
}

patch_counter_t mock_buf_t::get_next_patch_counter() {
    return 0;
}

void mock_buf_t::set_data(const void* dest, const void* src, const size_t n) {
    size_t offset = (const char*)dest - (const char*)internal_buf->data;
    apply_patch(*(new memcpy_patch_t(internal_buf->block_id, get_next_patch_counter(), offset, (const char*)src, n)));
}

void mock_buf_t::move_data(const void* dest, const void* src, const size_t n) {
    size_t dest_offset = (const char*)dest - (const char*)internal_buf->data;
    size_t src_offset = (const char*)src - (const char*)internal_buf->data;
    apply_patch(*(new memmove_patch_t(internal_buf->block_id, get_next_patch_counter(), dest_offset, src_offset, n)));
}

void mock_buf_t::mark_deleted() {
    rassert(access == rwi_write);
    deleted = true;
}

void mock_buf_t::release() {
    internal_buf->lock.unlock();
    if (deleted) internal_buf->destroy();
    delete this;
}

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

mock_buf_t *mock_transaction_t::acquire(block_id_t block_id, access_t mode, mock_block_available_callback_t *callback) {
    if (mode == rwi_write) rassert(this->access == rwi_write);
    
    rassert(block_id < cache->bufs.get_size());
    internal_buf_t *internal_buf = cache->bufs[block_id];
    rassert(internal_buf);
    
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
    internal_buf_t *internal_buf = cache->bufs[block_id] = new internal_buf_t(cache, block_id);
    bool locked __attribute__((unused)) = internal_buf->lock.lock(rwi_write, NULL);
    rassert(locked);
    
    mock_buf_t *buf = new mock_buf_t(internal_buf, rwi_write);
    return buf;
}

repli_timestamp mock_transaction_t::get_subtree_recency(block_id_t block_id) {
    rassert(block_id < cache->bufs.get_size());
    internal_buf_t *internal_buf = cache->bufs[block_id];
    rassert(internal_buf);

    return internal_buf->subtree_recency;
}

mock_transaction_t::mock_transaction_t(mock_cache_t *cache, access_t access)
    : cache(cache), access(access) {
    
    rassert(cache->running);
    cache->n_transactions++;
}

mock_transaction_t::~mock_transaction_t() {
    cache->n_transactions--;
    if (cache->n_transactions == 0 && !cache->running) {
        cache->shutdown_write_bufs();
    }
}

/* Cache */

mock_cache_t::mock_cache_t(
    translator_serializer_t *serializer,
    mirrored_cache_config_t *dynamic_config,
    mirrored_cache_static_config_t *static_config)
    : serializer(serializer), running(false), n_transactions(0), block_size(serializer->get_block_size()) { }

mock_cache_t::~mock_cache_t() {
    rassert(!running);
    rassert(n_transactions == 0);
}

bool mock_cache_t::start(ready_callback_t *cb) {
    ready_callback = NULL;
    if (home_thread == serializer->home_thread) {
        return load_blocks_from_serializer(cb);
    } else {
        do_on_thread(serializer->home_thread, boost::bind(&mock_cache_t::do_load_blocks_from_serializer, this, cb));
        return false;
    }
}

void mock_cache_t::do_load_blocks_from_serializer(ready_callback_t *cb) {
    load_blocks_from_serializer(cb);
}

bool mock_cache_t::load_blocks_from_serializer(ready_callback_t *cb) {
    rassert(get_thread_id() == serializer->home_thread);

    block_id_t end_block_id = serializer->max_block_id();
    if (end_block_id == 0) {
        // Create the superblock
        bufs.set_size(1);
        rassert(SUPERBLOCK_ID == 0);
        bufs[SUPERBLOCK_ID] = new internal_buf_t(this, SUPERBLOCK_ID);

        if (home_thread == serializer->home_thread) {
            have_loaded_blocks();
            return true;
        } else {
            do_on_thread(home_thread, boost::bind(&mock_cache_t::have_loaded_blocks, this));
            ready_callback = cb;
            return false;
        }
    } else {
        // Load the blocks from the serializer
        bufs.set_size(end_block_id, NULL);
        blocks_to_load = 0;
        for (block_id_t i = 0; i < end_block_id; i++) {
            if (serializer->block_in_use(i)) {
                internal_buf_t *internal_buf = bufs[i] = new internal_buf_t(this, i);
                serializer->do_read(i, internal_buf->data, this);
                blocks_to_load++;
            }
        }
        if (blocks_to_load == 0) {
            if (home_thread == serializer->home_thread) {
                have_loaded_blocks();
                return true;
            } else {
                do_on_thread(home_thread, boost::bind(&mock_cache_t::have_loaded_blocks, this));
                ready_callback = cb;
                return false;
            }
        } else {
            ready_callback = cb;
            return false;
        }
    }
}

void mock_cache_t::on_serializer_read() {
    blocks_to_load--;
    if (blocks_to_load == 0) {
        do_on_thread(home_thread, boost::bind(&mock_cache_t::have_loaded_blocks, this));
    }
}

void mock_cache_t::have_loaded_blocks() {
    running = true;
    if (ready_callback) {
        ready_callback->on_cache_ready();
    }
}

block_size_t mock_cache_t::get_block_size() {
    return block_size;
}

mock_transaction_t *mock_cache_t::begin_transaction(access_t access, mock_transaction_begin_callback_t *callback) {
    rassert(running);
    
    mock_transaction_t *txn = new mock_transaction_t(this, access);
    
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

bool mock_cache_t::shutdown(shutdown_callback_t *cb) {
    running = false;
    
    shutdown_callback = NULL;
    if (n_transactions == 0) {
        if (shutdown_write_bufs()) {
            return true;
        } else {
            shutdown_callback = cb;
            return false;
        }
    } else {
        shutdown_callback = cb;
        return false;
    }
}

bool mock_cache_t::shutdown_write_bufs() {
    if (home_thread == serializer->home_thread) {
        return shutdown_do_send_bufs_to_serializer();
    } else {
        do_on_thread(serializer->home_thread, boost::bind(&mock_cache_t::do_shutdown_do_send_bufs_to_serializer, this));
        return false;
    }
}

void mock_cache_t::do_shutdown_do_send_bufs_to_serializer() {
    shutdown_do_send_bufs_to_serializer();
}

bool mock_cache_t::shutdown_do_send_bufs_to_serializer() {
    std::vector<translator_serializer_t::write_t> writes;
    for (block_id_t i = 0; i < bufs.get_size(); i++) {
        writes.push_back(translator_serializer_t::write_t::make(i, bufs[i] ? bufs[i]->subtree_recency : repli_timestamp::invalid,
                                                                bufs[i] ? bufs[i]->data : NULL, NULL));
    }

    if (serializer->do_write(writes.data(), writes.size(), this)) {
        if (home_thread == serializer->home_thread) {
            return shutdown_destroy_bufs();
        } else {
            do_on_thread(home_thread, boost::bind(&mock_cache_t::do_shutdown_destroy_bufs, this));
            return false;
        }
    } else {
        return false;
    }
}

void mock_cache_t::on_serializer_write_txn() {
    do_on_thread(home_thread, boost::bind(&mock_cache_t::do_shutdown_destroy_bufs, this));
}

void mock_cache_t::do_shutdown_destroy_bufs() {
    shutdown_destroy_bufs();
}

bool mock_cache_t::shutdown_destroy_bufs() {
    for (block_id_t i = 0; i < bufs.get_size(); i++) {
        if (bufs[i]) delete bufs[i];
    }
    
    if (maybe_random_delay(this, &mock_cache_t::shutdown_finish)) {
        shutdown_finish();
        return true;
    } else {
        return false;
    }
}

void mock_cache_t::shutdown_finish() {
    if (shutdown_callback) shutdown_callback->on_cache_shutdown();
}
