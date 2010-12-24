#ifndef __BTREE_SLICE_HPP__
#define __BTREE_SLICE_HPP__

#include "store.hpp"
#include "buffer_cache/buffer_cache.hpp"
#include "serializer/translator.hpp"

class initialize_superblock_fsm_t;
struct btree_replicant_t;

/* btree_slice_t is a thin wrapper around cache_t that handles initializing the buffer
cache for the purpose of storing a btree. There are many btree_slice_ts per
btree_key_value_store_t. */

struct btree_key_value_store_t;

class btree_slice_t :
    private cache_t::ready_callback_t,
    private cache_t::shutdown_callback_t,
    public home_thread_mixin_t
{
    friend class initialize_superblock_fsm_t;
    
public:
    btree_slice_t(
        translator_serializer_t *serializer,
        mirrored_cache_config_t *config);
    ~btree_slice_t();
    
public:
    typedef btree_key_value_store_t ready_callback_t;
    bool start_new(ready_callback_t *cb);
    bool start_existing(ready_callback_t *cb);
    
private:
    bool start(ready_callback_t *cb);
    bool next_starting_up_step();
    void on_cache_ready();
    void on_initialize_superblock();
    bool is_start_existing;
    ready_callback_t *ready_callback;

public:
    cas_t gen_cas();
private:
    uint32_t cas_counter;

public:
    std::vector<btree_replicant_t *> replicants;

public:
    typedef btree_key_value_store_t shutdown_callback_t;
    bool shutdown(shutdown_callback_t *cb);
private:
    bool next_shutting_down_step();
    void on_cache_shutdown();
    shutdown_callback_t *shutdown_callback;

private:
    enum state_t {
        state_unstarted,
        
        state_starting_up_start_serializer,
        state_starting_up_waiting_for_serializer,
        state_starting_up_start_cache,
        state_starting_up_waiting_for_cache,
        state_starting_up_initialize_superblock,
        state_starting_up_waiting_for_superblock,
        state_starting_up_finish,
        
        state_ready,
        
        state_shutting_down_shutdown_serializer,
        state_shutting_down_waiting_for_serializer,
        state_shutting_down_shutdown_cache,
        state_shutting_down_waiting_for_cache,
        state_shutting_down_finish,
        
        state_shut_down
    } state;
    
    initialize_superblock_fsm_t *sb_fsm;
    
public:
    cache_t cache;

private:
    DISABLE_COPYING(btree_slice_t);
};

#endif /* __BTREE_SLICE_HPP__ */
