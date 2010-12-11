
#ifndef __BTREE_KEY_VALUE_STORE_HPP__
#define __BTREE_KEY_VALUE_STORE_HPP__

#include "btree/slice.hpp"
#include "btree/node.hpp"
#include "utils.hpp"
#include "concurrency/access.hpp"
#include "config/cmd_args.hpp"
#include "arch/arch.hpp"
#include "serializer/config.hpp"
#include "serializer/translator.hpp"
#include "store.hpp"


#define CONFIG_BLOCK_ID (config_block_id_t::make(0))

// TODO move serializer_config_block_t to separate file
/* This is the format that block ID 0 on each serializer takes. */

struct serializer_config_block_t {
    block_magic_t magic;
    
    /* What time the database was created. To help catch the case where files from two
    databases are mixed. */
    uint32_t database_magic;
    
    /* How many serializers the database is using (in case user creates the database with
    some number of serializers and then specifies less than that many on a subsequent
    run) */
    int32_t n_files;
    
    /* Which serializer this is, in case user specifies serializers in a different order from
    run to run */
    int32_t this_serializer;
    
    /* Static btree configuration information, like number of slices. Should be the same on
    each serializer. */
    btree_config_t btree_config;

    static const block_magic_t expected_magic;
};

/* btree_key_value_store_t represents a collection of slices, possibly distributed
across several cores, each of which holds a btree. Together with the btree_fsms, it
provides the abstraction of a key-value store. */

struct bkvs_start_new_serializer_fsm_t;
struct bkvs_start_existing_serializer_fsm_t;
struct btree_replicant_t;

class btree_key_value_store_t :
    public home_cpu_mixin_t,
    public store_t,
    public standard_serializer_t::shutdown_callback_t
{
    friend class bkvs_start_new_serializer_fsm_t;
    friend class bkvs_start_existing_serializer_fsm_t;
    
public:
    btree_key_value_store_t(
        btree_key_value_store_dynamic_config_t *dynamic_config);
    ~btree_key_value_store_t();
    
    struct check_callback_t {
        virtual void on_store_check(bool valid) = 0;
        virtual ~check_callback_t() {}
    };
    static void check_existing(const std::vector<std::string>& db_filenames, check_callback_t *cb);
    
    struct ready_callback_t {
        virtual void on_store_ready() = 0;
        virtual ~ready_callback_t() {}
    };
    bool start_new(ready_callback_t *cb, btree_key_value_store_static_config_t *static_config);
    bool start_existing(ready_callback_t *cb);
    
    struct shutdown_callback_t {
        virtual void on_store_shutdown() = 0;
        virtual ~shutdown_callback_t() {}
    };
    bool shutdown(shutdown_callback_t *cb);

public:
    /* store_t interface. */
    
    void get(store_key_t *key, get_callback_t *cb);
    void get_cas(store_key_t *key, get_callback_t *cb);
    void set(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, set_callback_t *cb);
    void add(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, set_callback_t *cb);
    void replace(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, set_callback_t *cb);
    void cas(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, cas_t unique, set_callback_t *cb);
    void incr(store_key_t *key, unsigned long long amount, incr_decr_callback_t *cb);
    void decr(store_key_t *key, unsigned long long amount, incr_decr_callback_t *cb);
    void append(store_key_t *key, data_provider_t *data, append_prepend_callback_t *cb);
    void prepend(store_key_t *key, data_provider_t *data, append_prepend_callback_t *cb);
    void delete_key(store_key_t *key, delete_callback_t *cb);
    void replicate(replicant_t *cb, repli_timestamp cutoff);
    void stop_replicating(replicant_t *cb);

public:
    btree_key_value_store_dynamic_config_t *dynamic_config;
    mirrored_cache_config_t cache_config;   /* Duplicate so we can modify it */
    int n_files;
    
    btree_config_t btree_static_config;
    
    /* The key-value store typically has more slices than serializers. The slices share
    serializers via the "pseudoserializers": translator-serializers, one per slice, that
    multiplex requests onto the actual serializers. */
    standard_serializer_t *serializers[MAX_SERIALIZERS];
    translator_serializer_t *pseudoserializers[MAX_SLICES];
    btree_slice_t *slices[MAX_SLICES];
    
    btree_slice_t *slice_for_key(btree_key *key);
    
    std::vector<btree_replicant_t *> replicants;
    
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
    
    static int compute_mod_count(int32_t file_number, int32_t n_files, int32_t n_slices);
    static uint32_t hash(const btree_key *key);

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

// Stats

extern perfmon_duration_sampler_t
    pm_cmd_set,
    pm_cmd_get;

#endif /* __BTREE_KEY_VALUE_STORE_HPP__ */
