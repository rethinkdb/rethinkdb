
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

class initialize_superblock_fsm_t;

/* btree_slice_t is a thin wrapper around cache_t that handles initializing the buffer
cache for the purpose of storing a btree. There are many btree_slice_ts per
btree_key_value_store_t. */

class btree_slice_t :
    private cache_t::ready_callback_t,
    private cache_t::shutdown_callback_t,
    public home_cpu_mixin_t,
    public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, btree_slice_t>
{
    friend class initialize_superblock_fsm_t;
    
public:
    btree_slice_t(
        serializer_t *serializer,
        int id_on_this_serializer,
        int count_on_this_serializer,
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
        virtual void on_slice_shutdown(btree_slice_t *) = 0;
    };
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
    
public:
    /* Statistics */
    int total_set_operations;
    perfmon_var_t<int> pm_total_set_operations;
};

/* btree_key_value_store_t represents a collection of slices, possibly distributed
across several cores, each of which holds a btree. Together with the btree_fsms, it
provides the abstraction of a key-value store. */

class btree_key_value_store_t :
    public home_cpu_mixin_t,
    public serializer_t::ready_callback_t,
    public btree_slice_t::ready_callback_t,
    public btree_slice_t::shutdown_callback_t,
    public serializer_t::shutdown_callback_t
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
    
    serializer_t *serializers[MAX_SERIALIZERS];
    btree_slice_t *slices[MAX_SLICES];
    
    enum state_t {
        state_off,
        state_starting_up,
        state_ready,
        state_shutting_down
    } state;
    
    int messages_out;
    
    /* Startup process */
    
    ready_callback_t *ready_callback;

    void create_serializers();   // Called on home thread
    bool create_a_serializer_on_this_core(int);   // Called for each serializer on its thread
    void on_serializer_ready(serializer_t *ser);   // Called on serializer thread
    bool have_created_a_serializer();   // Called on home thread
    
    void create_slices();   // Called on home thread
    bool create_a_slice_on_this_core(int);   // Called for each slice on its thread
    void on_slice_ready();   // Called on slice thread
    bool have_created_a_slice();   // Called on home thread
    
    void finish_start();
    
    /* Shutdown process */
    
    shutdown_callback_t *shutdown_callback;
    
    void shutdown_slices();   // Called on home thread
    bool shutdown_a_slice(int id);   // Called on slice thread
    void on_slice_shutdown(btree_slice_t *slice);   // Called on slice thread
    bool have_shutdown_a_slice();   // Called on home thread
    
    void shutdown_serializers();   // Called on home thread
    bool shutdown_a_serializer(int id);   // Called on serializer thread
    void on_serializer_shutdown(serializer_t *serializer);   // Called on serializer thread
    bool have_shutdown_a_serializer();   // Called on home thread
    
    void finish_shutdown();
};

// Other parts of the code refer to store_t instead of btree_key_value_store_t to
// facilitate the process of adding another type of store (such as a hashmap)
// later on.
typedef btree_key_value_store_t store_t;

#endif /* __BTREE_KEY_VALUE_STORE_HPP__ */
