
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
#include "serializer/config.hpp"
#include "serializer/translator.hpp"

struct btree_slice_t;

/* btree_key_value_store_t represents a collection of slices, possibly distributed
across several cores, each of which holds a btree. Together with the btree_fsms, it
provides the abstraction of a key-value store. */

struct bkvs_start_new_serializer_fsm_t;
struct bkvs_start_existing_serializer_fsm_t;

class btree_key_value_store_t :
    public home_cpu_mixin_t,
    public standard_serializer_t::shutdown_callback_t
{
    friend class bkvs_start_new_serializer_fsm_t;
    friend class bkvs_start_existing_serializer_fsm_t;
    
public:
    btree_key_value_store_t(
        btree_key_value_store_dynamic_config_t *dynamic_config,
        int n_files,
        const char **db_filenames);
    ~btree_key_value_store_t();
    
    struct ready_callback_t {
        virtual void on_store_ready() = 0;
    };
    bool start_new(ready_callback_t *cb, btree_key_value_store_static_config_t *static_config);
    bool start_existing(ready_callback_t *cb);
    
    btree_slice_t *slice_for_key(btree_key *key);
    
    struct shutdown_callback_t {
        virtual void on_store_shutdown() = 0;
    };
    bool shutdown(shutdown_callback_t *cb);

public:
    btree_key_value_store_dynamic_config_t *dynamic_config;
    mirrored_cache_config_t cache_config;   /* Duplicate so we can modify it */
    int n_files;
    const char **db_filenames;
    
    btree_config_t btree_static_config;
    
    /* The key-value store typically has more slices than serializers. The slices share
    serializers via the "pseudoserializers": translator-serializers, one per slice, that
    multiplex requests onto the actual serializers. */
    standard_serializer_t *serializers[MAX_SERIALIZERS];
    translator_serializer_t *pseudoserializers[MAX_SLICES];
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
    bool is_start_existing;
    log_serializer_static_config_t *serializer_static_config;
    
    bool start(ready_callback_t *cb);
    
    void create_serializers();   // Called on home thread
    uint32_t creation_magic;   // Used in start-new mode
    uint32_t serializer_magics[MAX_SERIALIZERS];   // Used in start-existing mode
    bool have_created_a_serializer();   // Called on home thread
    
    void create_pseudoserializers();   // Called on home thread
    bool create_a_pseudoserializer_on_this_core(int i);   // Called on serializer thread
    bool have_created_a_pseudoserializer();   // Called on serializer thread
    
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
    
    void delete_pseudoserializers();   // Called on home thread
    bool delete_a_pseudoserializer(int id);   // Called on serializer thread
    
    void shutdown_serializers();   // Called on home thread
    bool shutdown_a_serializer(int id);   // Called on serializer thread
    void on_serializer_shutdown(standard_serializer_t *serializer);   // Called on serializer thread
    bool have_shutdown_a_serializer();   // Called on home thread
    
    void finish_shutdown();

private:
    DISABLE_COPYING(btree_key_value_store_t);
};



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
    btree_value::cas_t gen_cas();
private:
    uint32_t cas_counter;

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

// Stats

extern perfmon_counter_t
    pm_cmd_set,
    pm_cmd_get;

// Other parts of the code refer to store_t instead of btree_key_value_store_t to
// facilitate the process of adding another type of store (such as a hashmap)
// later on.
typedef btree_key_value_store_t store_t;

#endif /* __BTREE_KEY_VALUE_STORE_HPP__ */
