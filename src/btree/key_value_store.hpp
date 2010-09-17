
#ifndef __BTREE_KEY_VALUE_STORE_HPP__
#define __BTREE_KEY_VALUE_STORE_HPP__

#include "config/code.hpp"
#include "buffer_cache/callbacks.hpp"
#include "btree/node.hpp"   // For btree_superblock_t
#include "btree/fsm.hpp"
#include "utils.hpp"
#include "containers/intrusive_list.hpp"
#include "concurrency/access.hpp"

/* When the serializer starts up, it will create an initial superblock and initialize it to zero.
This isn't quite the behavior we want. The job of initialize_superblock_fsm is to initialize the
superblock to contain NULL_BLOCK_ID rather than zero as the root node. */

template <class config_t>
class initialize_superblock_fsm :
    private block_available_callback<config_t>,
    private transaction_commit_callback<config_t>,
    public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, initialize_superblock_fsm<config_t> >{
public:
    typedef typename config_t::cache_t cache_t;
    typedef typename config_t::transaction_t transaction_t;
    typedef typename config_t::buf_t buf_t;

public:
    initialize_superblock_fsm(cache_t *cache)
        : state(state_unstarted), cache(cache), sb_buf(NULL), txn(NULL)
        {}
    ~initialize_superblock_fsm() {
        assert(state == state_unstarted || state == state_done);
    }
    
    struct callback_t {
        virtual void on_initialize_superblock() = 0;
    };
    
    bool initialize_superblock_if_necessary(callback_t *cb) {
        assert(state == state_unstarted);
        state = state_acquire_superblock;
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
    callback_t *callback;
    
    bool next_initialize_superblock_step() {
        
        if (state == state_acquire_superblock) {
            txn = cache->begin_transaction(rwi_write, NULL);
            assert(txn);   // We should be the only cache user
            buf_t *sb_buf = txn->acquire(SUPERBLOCK_ID, rwi_write, this);
            if (sb_buf) {
                state = state_make_change;
            } else {
                state = state_acquiring_superblock;
                return false;
            }
        }
        
        if (state == state_make_change) {
            btree_superblock_t *sb = (btree_superblock_t*)(sb_buf->get_data());
            // The serializer will init the superblock to zeroes if the database is newly created.
            if (!sb->database_exists) {
                sb->database_exists = 1;
                sb->root_block = NULL_BLOCK_ID;
                sb_buf->set_dirty();
            }
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

/* btree_key_value_store is a thin wrapper around buffer_cache_t that handles initializing the buffer
cache for the purpose of storing a btree. */

template<class config_t>
class btree_key_value_store :
    private config_t::cache_t::ready_callback_t,
    private initialize_superblock_fsm<config_t>::callback_t,
    private config_t::cache_t::shutdown_callback_t
{
    typedef typename config_t::btree_fsm_t btree_fsm_t;
    
public:
    btree_key_value_store(
        char *filename,
        size_t block_size,
        size_t max_size,
        bool wait_for_flush,
        unsigned int flush_timer_ms,
        unsigned int flush_threshold_percent)
        : state(state_unstarted),
          cache(filename, block_size, max_size, wait_for_flush, flush_timer_ms, flush_threshold_percent)
        { }

public:
    struct ready_callback_t {
        virtual void on_store_ready() = 0;
    };
    bool start(ready_callback_t *cb) {
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
private:
    bool next_starting_up_step() {
        
        if (state == state_starting_up_start_cache) {
            if (cache.start(this)) {
                state = state_starting_up_initialize_superblock;
            } else {
                state = state_starting_up_waiting_for_cache;
                return false;
            }
        }
        
        if (state == state_starting_up_initialize_superblock) {
            sb_fsm = new initialize_superblock_fsm<config_t>(&cache);
            if (sb_fsm->initialize_superblock_if_necessary(this)) {
                state = state_starting_up_finish;
            } else {
                state = state_starting_up_waiting_for_superblock;
                return false;
            }
        }
        
        if (state == state_starting_up_finish) {
            
            delete sb_fsm;
            
            state = state_ready;
            if (ready_callback) ready_callback->on_store_ready();
            
            typename std::vector<btree_fsm_t*, gnew_alloc<btree_fsm_t *> >::iterator it;
            for (it = fsms_waiting_for_ready.begin(); it != fsms_waiting_for_ready.end(); it ++) {
                btree_fsm_t *fsm = *it;
                bool done = (fsm->do_transition(NULL) == btree_fsm_t::transition_complete);
                if (done && fsm->on_complete) {
                    fsm->on_complete(fsm);
                }
            }
            fsms_waiting_for_ready.clear();
            
            return true;
        }
        
        fail("Unexpected state");
    }
    void on_cache_ready() {
        assert(state == state_starting_up_waiting_for_cache);
        state = state_starting_up_initialize_superblock;
        next_starting_up_step();
    }
    void on_initialize_superblock() {
        assert(state == state_starting_up_waiting_for_superblock);
        state = state_starting_up_finish;
        next_starting_up_step();
    }
    ready_callback_t *ready_callback;
    
    // TODO It's kind of hacky to queue requests here. Should we queue them on the worker instead or
    // perhaps refuse to accept connections until all the key-value stores are ready?
    std::vector<btree_fsm_t*, gnew_alloc<btree_fsm_t *> > fsms_waiting_for_ready;

public:
    bool run_fsm(btree_fsm_t *fsm, typename btree_fsm_t::on_complete_t cb) {
        
        // The btree is constructed with no cache; here we must assign it its proper cache
        assert(!fsm->cache);
        fsm->cache = &cache;
        
        fsm->on_complete = cb;
        if (state == state_ready) {
            return (fsm->do_transition(NULL) == btree_fsm_t::transition_complete);
        } else {
            fsms_waiting_for_ready.push_back(fsm);
            return false;
        }
    }

public:
    struct shutdown_callback_t {
        virtual void on_store_shutdown() = 0;
    };
    bool shutdown(shutdown_callback_t *cb) {
        assert(state == state_ready);
        if (cache.shutdown(this)) {
            state = state_shut_down;
            return true;
        } else {
            state = state_shutting_down;
            shutdown_callback = cb;
            return false;
        }
    }
private:
    void on_cache_shutdown() {
        state = state_shut_down;
        if (shutdown_callback) shutdown_callback->on_store_shutdown();
    }
    shutdown_callback_t *shutdown_callback;

private:
    enum state_t {
        state_unstarted,
        
        state_starting_up_start_cache,
        state_starting_up_waiting_for_cache,
        state_starting_up_initialize_superblock,
        state_starting_up_waiting_for_superblock,
        state_starting_up_finish,
        
        state_ready,
        
        state_shutting_down,
        
        state_shut_down
    } state;
    
    initialize_superblock_fsm<config_t> *sb_fsm;
    
    typename config_t::cache_t cache;
    
};

#endif /* __BTREE_KEY_VALUE_STORE_HPP__ */
