
#ifndef __BTREE_KEY_VALUE_STORE_HPP__
#define __BTREE_KEY_VALUE_STORE_HPP__

#include "config/alloc.hpp"
#include "buffer_cache/buffer_cache.hpp"
#include "btree/node.hpp"   // For btree_superblock_t
#include "utils.hpp"
#include "containers/intrusive_list.hpp"
#include "concurrency/access.hpp"
#include "config/cmd_args.hpp"
#include "arch/arch.hpp"

class btree_slice_t;
class initialize_superblock_fsm_t;

/* btree_key_value_store_t represents a collection of slices, possibly distributed
across several cores, each of which holds a btree. Together with the btree_fsms, it
provides the abstraction of a key-value store. */

class btree_key_value_store_t :
    public home_cpu_mixin_t
{

public:
    btree_key_value_store_t(cmd_config_t *cmd_config);
    ~btree_key_value_store_t();
    
    struct ready_callback_t {
        virtual void on_store_ready() = 0;
    };
    bool start(ready_callback_t *cb);
    
    btree_slice_t *slice_for_key(btree_key *key);
    
    struct shutdown_callback_t {
        virtual void on_store_shutdown() = 0;
    };
    bool shutdown(shutdown_callback_t *cb);

public:
    cmd_config_t *cmd_config;
    btree_slice_t *slices[MAX_SLICES];
    enum state_t {
        state_off,
        state_starting_up,
        state_ready,
        state_shutting_down
    } state;
    int messages_out;
    ready_callback_t *ready_callback;
    shutdown_callback_t *shutdown_callback;
};

/* btree_slice_t is a thin wrapper around cache_t that handles initializing the buffer
cache for the purpose of storing a btree. There are many btree_slice_ts per
btree_key_value_store_t. */

class btree_slice_t :
    private cache_t::ready_callback_t,
    private cache_t::shutdown_callback_t,
    public cpu_message_t,  // For call_later_on_this_cpu()
    public home_cpu_mixin_t,
    public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, btree_slice_t>
{
    friend class initialize_superblock_fsm_t;
    
public:
    btree_slice_t(
        char *filename,
        size_t block_size,
        size_t max_size,
        bool wait_for_flush,
        unsigned int flush_timer_ms,
        unsigned int flush_threshold_percent);
    ~btree_slice_t();
    
public:
    struct ready_callback_t {
        virtual void on_slice_ready() = 0;
    };
    bool start(ready_callback_t *cb);
    
private:
    bool next_starting_up_step();
    void on_cache_ready();
    void on_initialize_superblock();
    ready_callback_t *ready_callback;

public:
    btree_value::cas_t gen_cas();
private:
    uint32_t cas_counter;

public:
    struct shutdown_callback_t {
        virtual void on_slice_shutdown() = 0;
    };
    bool shutdown(shutdown_callback_t *cb);
private:
    void on_cache_shutdown();
    void on_cpu_switch();
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
    
    initialize_superblock_fsm_t *sb_fsm;

public:
    cache_t cache;
};

// Other parts of the code refer to store_t instead of btree_key_value_store_t to
// facilitate the process of adding another type of store (such as a hashmap)
// later on.
typedef btree_key_value_store_t store_t;

#endif /* __BTREE_KEY_VALUE_STORE_HPP__ */
