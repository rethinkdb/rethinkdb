#include "buffer_cache/mock.hpp"
#include "arch/arch.hpp"

/* Internal buf object */

struct internal_buf_t
{
    mock_cache_t *cache;
    block_id_t block_id;
    void *data;
    bool writing;
    int n_reading;
    std::vector<mock_block_available_callback_t*> waiting_readers, waiting_writers;
    
    internal_buf_t(mock_cache_t *cache, block_id_t block_id)
        : cache(cache), block_id(block_id), data(malloc(cache->block_size)), writing(false), n_reading(0) {
        assert(data);
    }
    
    ~internal_buf_t() {
        free(data);
    }
    
    bool can_acquire(access_t access) {
    
        switch(access) {
            case rwi_read: return !writing;
            case rwi_write: return !writing && n_reading == 0;
            default: fail("Bad access.");
        }
    }
    
    void acquire(access_t access) {
    
        assert(can_acquire(access));
        switch(access) {    
            case rwi_read:
                n_reading++;
                break;
            case rwi_write:
                writing = true;
                break;
            default: fail("Bad access.");
        }
    }
    
    void release(access_t access) {
    
        switch(access) {
            case rwi_read:
                assert(n_reading > 0);
                n_reading--;
                break;
            case rwi_write:
                assert(writing);
                writing = false;
                break;
            default: fail("Bad access.");
        }
        
        if (!writing && n_reading == 0) {
        
            /* If both writers and readers are waiting, then randomly either let all the
            readers go or one of the writers go */
            
            if (waiting_writers.size() > 0 && (waiting_readers.size() == 0 || (random() & 1) == 0)) {
            
                mock_buf_t *buf = new mock_buf_t(this, rwi_write);
                random_delay(waiting_writers[0], &mock_block_available_callback_t::on_block_available, buf);
                waiting_writers.erase(waiting_writers.begin());
            
            } else if (waiting_readers.size() > 0) {
            
                for (unsigned i = 0; i < waiting_readers.size(); i++) {
                    mock_buf_t *buf = new mock_buf_t(this, rwi_read);
                    random_delay(waiting_readers[i], &mock_block_available_callback_t::on_block_available, buf);
                }
                waiting_readers.clear();
            }
        }
    }
    
    void add_waiter(mock_block_available_callback_t *waiter, access_t access) {
        assert(!can_acquire(access));
        switch(access) {
            case rwi_read:
                waiting_readers.push_back(waiter);
                break;
            case rwi_write:
                waiting_writers.push_back(waiter);
                break;
            default: fail("Bad access.");
        }
    }
    
    void destroy() {
        assert(writing);
        assert(n_reading == 0);
        assert(waiting_readers.size() == 0);
        assert(waiting_writers.size() == 0);
        
        assert(cache->bufs[block_id] == this);
        cache->bufs[block_id] = NULL;
        
        delete this;
    }
};

/* Buf */

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
    if (deleted) {
        internal_buf->destroy();
    } else {
        internal_buf->release(access);
    }
    delete this;
}

mock_buf_t::mock_buf_t(internal_buf_t *internal_buf, access_t access)
    : internal_buf(internal_buf), access(access), dirty(false), deleted(false) {
    internal_buf->acquire(access);
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
    
    if (internal_buf->can_acquire(mode)) {
        mock_buf_t *buf = new mock_buf_t(internal_buf, mode);
        if (maybe_random_delay(callback, &mock_block_available_callback_t::on_block_available, buf)) {
            return buf;
        } else {
            return NULL;
        }
    } else {
        internal_buf->add_waiter(callback, mode);
        return NULL;
    }
}

mock_buf_t *mock_transaction_t::allocate(block_id_t *new_block_id) {
    
    assert(this->access == rwi_write);
    
    block_id_t block_id = *new_block_id = cache->bufs.get_size();
    cache->bufs.set_size(block_id + 1);
    internal_buf_t *internal_buf = cache->bufs[block_id] = new internal_buf_t(cache, block_id);
    
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
        cache->shutdown_destroy_bufs();
    }
}

/* Cache */

mock_cache_t::mock_cache_t(
    serializer_t *serializer,
    mirrored_cache_config_t *config)
    : serializer(serializer), running(false), n_transactions(0), block_size(serializer->get_block_size()) { }

mock_cache_t::~mock_cache_t() {
    assert(!running);
    assert(n_transactions == 0);
}

bool mock_cache_t::start(ready_callback_t *cb) {
    
    running = true;
    
    // Create the superblock
    bufs.set_size(1);
    assert(SUPERBLOCK_ID == 0);
    internal_buf_t *internal_buf = bufs[SUPERBLOCK_ID] = new internal_buf_t(this, SUPERBLOCK_ID);
    bzero(internal_buf->data, block_size);
    
    return maybe_random_delay(cb, &ready_callback_t::on_cache_ready);
}

size_t mock_cache_t::get_block_size() {
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
        if (shutdown_destroy_bufs()) {
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
