#include "btree/slice.hpp"

/* When the serializer starts up, it will create an initial superblock and initialize it to zero.
This isn't quite the behavior we want. The job of initialize_superblock_fsm is to initialize the
superblock to contain NULL_BLOCK_ID rather than zero as the root node. */

class initialize_superblock_fsm_t :
    private block_available_callback_t,
    private transaction_begin_callback_t,
    private transaction_commit_callback_t
{

public:
    initialize_superblock_fsm_t(cache_t *cache)
        : state(state_unstarted), cache(cache), sb_buf(NULL), txn(NULL)
        {}
    ~initialize_superblock_fsm_t() {
        assert(state == state_unstarted || state == state_done);
    }
    
    bool initialize_superblock_if_necessary(btree_slice_t *cb) {
        assert(state == state_unstarted);
        state = state_begin_transaction;
        callback = NULL;
        if (next_initialize_superblock_step()) {
            return true;
        } else {
            callback = cb;
            return false;
        }
    }

private:
    enum state_t {
        state_unstarted,
        state_begin_transaction,
        state_beginning_transaction,
        state_acquire_superblock,
        state_acquiring_superblock,
        state_make_change,
        state_commit_transaction,
        state_committing_transaction,
        state_finish,
        state_done
    } state;
    
    cache_t *cache;
    buf_t *sb_buf;
    transaction_t *txn;
    btree_slice_t *callback;
    
    bool next_initialize_superblock_step() {
        
        if (state == state_begin_transaction) {
            txn = cache->begin_transaction(rwi_write, this);
            if (txn) {
                state = state_acquire_superblock;
            } else {
                state = state_beginning_transaction;
                return false;
            }
        }
        
        if (state == state_acquire_superblock) {
            sb_buf = txn->acquire(SUPERBLOCK_ID, rwi_write, this);
            if (sb_buf) {
                state = state_make_change;
            } else {
                state = state_acquiring_superblock;
                return false;
            }
        }
        
        if (state == state_make_change) {
            btree_superblock_t *sb = (btree_superblock_t*)(sb_buf->get_data_write());
            sb->magic = btree_superblock_t::expected_magic;
            sb->root_block = NULL_BLOCK_ID;
            sb_buf->release();
            state = state_commit_transaction;
        }
        
        if (state == state_commit_transaction) {
            if (txn->commit(this)) {
                state = state_finish;
            } else {
                state = state_committing_transaction;
                return false;
            }
        }
        
        if (state == state_finish) {
            state = state_done;
            if (callback) callback->on_initialize_superblock();
            return true;
        }
        
        fail("Unexpected state");
    }
    
    void on_txn_begin(transaction_t *t) {
        assert(state == state_beginning_transaction);
        txn = t;
        state = state_acquire_superblock;
        next_initialize_superblock_step();
    }
    
    void on_txn_commit(transaction_t *t) {
        assert(state == state_committing_transaction);
        state = state_finish;
        next_initialize_superblock_step();
    }
    
    void on_block_available(buf_t *buf) {
        assert(state == state_acquiring_superblock);
        sb_buf = buf;
        state = state_make_change;
        next_initialize_superblock_step();
    }
};

btree_slice_t::btree_slice_t(
    serializer_t *serializer,
    mirrored_cache_config_t *config)
    : cas_counter(0),
      state(state_unstarted),
      cache(serializer, config)
    { }

btree_slice_t::~btree_slice_t() {
    assert(state == state_unstarted || state == state_shut_down);
}

bool btree_slice_t::start_new(ready_callback_t *cb) {
    is_start_existing = false;
    return start(cb);
}

bool btree_slice_t::start_existing(ready_callback_t *cb) {
    is_start_existing = true;
    return start(cb);
}

bool btree_slice_t::start(ready_callback_t *cb) {
    assert(state == state_unstarted);
    state = state_starting_up_start_cache;
    ready_callback = NULL;
    if (next_starting_up_step()) {
        return true;
    } else {
        ready_callback = cb;
        return false;
    }
}

bool btree_slice_t::next_starting_up_step() {
    
    if (state == state_starting_up_start_cache) {
        
        /* For now, the cache's startup process is the same whether it is starting a new
        database or an existing one. This could change in the future, though. */
        
        if (cache.start(this)) {
            state = state_starting_up_initialize_superblock;
        } else {
            state = state_starting_up_waiting_for_cache;
            return false;
        }
    }
    
    if (state == state_starting_up_initialize_superblock) {
    
        if (is_start_existing) {
            state = state_starting_up_finish;
    
        } else {
            sb_fsm = new initialize_superblock_fsm_t(&cache);
            if (sb_fsm->initialize_superblock_if_necessary(this)) {
                state = state_starting_up_finish;
            } else {
                state = state_starting_up_waiting_for_superblock;
                return false;
            }
        }
    }
    
    if (state == state_starting_up_finish) {
        
        if (!is_start_existing) delete sb_fsm;
        
        state = state_ready;
        if (ready_callback) ready_callback->on_slice_ready();
        
        return true;
    }
    
    fail("Unexpected state");
}

void btree_slice_t::on_cache_ready() {
    assert(state == state_starting_up_waiting_for_cache);
    state = state_starting_up_initialize_superblock;
    next_starting_up_step();
}

void btree_slice_t::on_initialize_superblock() {
    assert(state == state_starting_up_waiting_for_superblock);
    state = state_starting_up_finish;
    next_starting_up_step();
}

btree_value::cas_t btree_slice_t::gen_cas() {
    // A CAS value is made up of both a timestamp and a per-slice counter,
    // which should be enough to guarantee that it'll be unique.
    return (time(NULL) << 32) | (++cas_counter);
}

bool btree_slice_t::shutdown(shutdown_callback_t *cb) {
    assert(state == state_ready);
    state = state_shutting_down_shutdown_cache;
    shutdown_callback = NULL;
    if (next_shutting_down_step()) {
        return true;
    } else {
        shutdown_callback = cb;
        return false;
    }
}

bool btree_slice_t::next_shutting_down_step() {
    
    if (state == state_shutting_down_shutdown_cache) {
        if (cache.shutdown(this)) {
            state = state_shutting_down_finish;
        } else {
            state = state_shutting_down_waiting_for_cache;
            return false;
        }
    }
    
    if (state == state_shutting_down_finish) {
        state = state_shut_down;
        if (shutdown_callback) shutdown_callback->on_slice_shutdown(this);
        return true;
    }
    
    fail("Invalid state.");
}

void btree_slice_t::on_cache_shutdown() {
    assert(state == state_shutting_down_waiting_for_cache);
    state = state_shutting_down_finish;
    next_shutting_down_step();
}
