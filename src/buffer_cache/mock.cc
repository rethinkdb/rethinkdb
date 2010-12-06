#include "buffer_cache/mock.hpp"
#include "arch/arch.hpp"

/* Internal buf object */

struct internal_buf_t
{
    mock_cache_t *cache;
    block_id_t block_id;
    void *data;
    rwi_lock_t lock;
    
    internal_buf_t(mock_cache_t *cache, block_id_t block_id)
        : cache(cache), block_id(block_id), data(cache->serializer->malloc()) {
        assert(data);
        bzero(data, cache->block_size.value());
    }
    
    ~internal_buf_t() {
        cache->serializer->free(data);
    }
    
    void destroy() {
        
        assert(!lock.locked());
        
        assert(cache->bufs[block_id] == this);
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

void *mock_buf_t::get_data_write() {
    assert(access == rwi_write);
    dirty = true;
    return internal_buf->data;
}

void mock_buf_t::mark_deleted() {
    assert(access == rwi_write);
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
    switch(access) {
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
        default: fail("Bad access");
    }
}

void mock_transaction_t::finish_committing(mock_transaction_commit_callback_t *cb) {
    
    if (cb) cb->on_txn_commit(this);
    delete this;
}

mock_buf_t *mock_transaction_t::acquire(block_id_t block_id, access_t mode, mock_block_available_callback_t *callback) {
    
    if (mode == rwi_write) assert(this->access == rwi_write);
    
    assert(block_id < cache->bufs.get_size());
    internal_buf_t *internal_buf = cache->bufs[block_id];
    assert(internal_buf);
    
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

mock_buf_t *mock_transaction_t::allocate(block_id_t *new_block_id) {
    
    assert(this->access == rwi_write);
    
    block_id_t block_id = *new_block_id = cache->bufs.get_size();
    cache->bufs.set_size(block_id + 1);
    internal_buf_t *internal_buf = cache->bufs[block_id] = new internal_buf_t(cache, block_id);
    bool locked __attribute__((unused)) = internal_buf->lock.lock(rwi_write, NULL);
    assert(locked);
    
    mock_buf_t *buf = new mock_buf_t(internal_buf, rwi_write);
    return buf;
}

mock_transaction_t::mock_transaction_t(mock_cache_t *cache, access_t access)
    : cache(cache), access(access) {
    
    assert(cache->running);
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
    mirrored_cache_config_t *config)
    : serializer(serializer), running(false), n_transactions(0), block_size(serializer->get_block_size()) { }

mock_cache_t::~mock_cache_t() {
    assert(!running);
    assert(n_transactions == 0);
}

bool mock_cache_t::start(ready_callback_t *cb) {
    
    ready_callback = NULL;
    if (do_on_cpu(serializer->home_cpu, this, &mock_cache_t::load_blocks_from_serializer)) {
        return true;
    } else {
        ready_callback = cb;
        return false;
    }
}

bool mock_cache_t::load_blocks_from_serializer() {
    
    if (serializer->max_block_id() == 0) {
        
        // Create the superblock
        bufs.set_size(1);
        assert(SUPERBLOCK_ID == 0);
        bufs[SUPERBLOCK_ID] = new internal_buf_t(this, SUPERBLOCK_ID);
        
        return do_on_cpu(home_cpu, this, &mock_cache_t::have_loaded_blocks);
        
    } else {
        
        // Load the blocks from the serializer
        bufs.set_size(serializer->max_block_id(), NULL);
        blocks_to_load = 0;
        for (block_id_t i = 0; i < serializer->max_block_id(); i++) {
            if (serializer->block_in_use(i)) {
                internal_buf_t *internal_buf = bufs[i] = new internal_buf_t(this, i);
                serializer->do_read(i, internal_buf->data, this);
                blocks_to_load++;
            }
        }
        if (blocks_to_load == 0) {
            return do_on_cpu(home_cpu, this, &mock_cache_t::have_loaded_blocks);
        } else {
            return false;
        }
    }
}

void mock_cache_t::on_serializer_read() {
    blocks_to_load--;
    if (blocks_to_load == 0) {
        do_on_cpu(home_cpu, this, &mock_cache_t::have_loaded_blocks);
    }
}

bool mock_cache_t::have_loaded_blocks() {
    
    running = true;
    if (ready_callback) ready_callback->on_cache_ready();
    return true;
}

block_size_t mock_cache_t::get_block_size() {
    return block_size;
}

mock_transaction_t *mock_cache_t::begin_transaction(access_t access, mock_transaction_begin_callback_t *callback) {
    
    assert(running);
    
    mock_transaction_t *txn = new mock_transaction_t(this, access);
    
    switch(access) {
        case rwi_read:
            return txn;
        case rwi_write:
            if (maybe_random_delay(callback, &mock_transaction_begin_callback_t::on_txn_begin, txn)) {
                return txn;
            } else {
                return NULL;
            }
        default: fail("Bad access.");
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
    
    return do_on_cpu(serializer->home_cpu, this, &mock_cache_t::shutdown_do_send_bufs_to_serializer);
}

bool mock_cache_t::shutdown_do_send_bufs_to_serializer() {
    
    translator_serializer_t::write_t writes[bufs.get_size()];
    for (block_id_t i = 0; i < bufs.get_size(); i++) {
        writes[i].block_id = i;
        writes[i].buf = bufs[i] ? bufs[i]->data : NULL;
        writes[i].callback = NULL;
    }
    
    if (serializer->do_write(writes, bufs.get_size(), this)) {
        return do_on_cpu(home_cpu, this, &mock_cache_t::shutdown_destroy_bufs);
    } else {
        return false;
    }
}

void mock_cache_t::on_serializer_write_txn() {
    
    do_on_cpu(home_cpu, this, &mock_cache_t::shutdown_destroy_bufs);
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
