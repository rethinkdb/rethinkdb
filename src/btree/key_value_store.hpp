#ifndef __BTREE_KEY_VALUE_STORE_HPP__
#define __BTREE_KEY_VALUE_STORE_HPP__

#include "btree/slice.hpp"
#include "btree/node.hpp"
#include "utils.hpp"
#include "concurrency/access.hpp"
#include "server/cmd_args.hpp"
#include "arch/arch.hpp"
#include "serializer/config.hpp"
#include "serializer/translator.hpp"
#include "store.hpp"
#include "control.hpp"


#define CONFIG_BLOCK_ID (config_block_id_t::make(0))

// TODO move serializer_config_block_t to separate file
/* This is the format that block ID 0 on each serializer takes. */

typedef uint32_t database_magic_t;

struct serializer_config_block_t {
    block_magic_t magic;
    
    /* What time the database was created. To help catch the case where files from two
    databases are mixed. */
    database_magic_t database_magic;
    
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
across several cores, each of which holds a btree. Together with the btree functions,
it provides the abstraction of a key-value store. */

class btree_key_value_store_t :
    public home_thread_mixin_t,
    public store_t
{
public:
    // Blocks
    static void create(
        btree_key_value_store_dynamic_config_t *dynamic_config,
        btree_key_value_store_static_config_t *static_config);

    // Blocks
    btree_key_value_store_t(
        btree_key_value_store_dynamic_config_t *dynamic_config);

    // Blocks
    ~btree_key_value_store_t();

    struct check_callback_t {
        virtual void on_store_check(bool valid) = 0;
        virtual ~check_callback_t() {}
    };
    static void check_existing(const std::vector<std::string>& db_filenames, check_callback_t *cb);

public:
    /* store_t interface. */

    get_result_t get(store_key_t *key);
    get_result_t get_cas(store_key_t *key, cas_t proposed_cas);
    set_result_t set(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, cas_t proposed_cas);
    set_result_t add(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, cas_t proposed_cas);
    set_result_t replace(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, cas_t proposed_cas);
    set_result_t cas(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, cas_t unique, cas_t proposed_cas);
    incr_decr_result_t incr(store_key_t *key, unsigned long long amount, cas_t proposed_cas);
    incr_decr_result_t decr(store_key_t *key, unsigned long long amount, cas_t proposed_cas);
    append_prepend_result_t append(store_key_t *key, data_provider_t *data, cas_t proposed_cas);
    append_prepend_result_t prepend(store_key_t *key, data_provider_t *data, cas_t proposed_cas);
    delete_result_t delete_key(store_key_t *key);

public:
    int n_files;
    btree_config_t btree_static_config;

    /* The key-value store typically has more slices than serializers. The slices share
    serializers via the "pseudoserializers": translator-serializers, one per slice, that
    multiplex requests onto the actual serializers. */
    standard_serializer_t *serializers[MAX_SERIALIZERS];
    translator_serializer_t *pseudoserializers[MAX_SLICES];
    btree_slice_t *slices[MAX_SLICES];

    uint32_t slice_nr(const btree_key *key);
    btree_slice_t *slice_for_key(const btree_key *key);

    static int compute_mod_count(int32_t file_number, int32_t n_files, int32_t n_slices);
    static uint32_t hash(const btree_key *key);

private:
    DISABLE_COPYING(btree_key_value_store_t);

private:
    /* slice debug control_t which allows us to grab, slice and hash for a key */
    struct hash_control_t :
        public control_t
    {
    public:
        hash_control_t(btree_key_value_store_t *btkvs) 
#ifndef NDEBUG  //No documentation if we're in release mode.
            : control_t("hash", std::string("Get hash and slice of a key. Syntax: rdb hash: key")), btkvs(btkvs)
#else
            : control_t("hash", std::string("")), btkvs(btkvs)
#endif
        {}
        ~hash_control_t() {};
        
    private: 
        btree_key_value_store_t *btkvs;
    public:
        std::string call(std::string key) {
            char store_key[MAX_KEY_SIZE+sizeof(store_key_t)];
            str_to_key(key.c_str(), (store_key_t *) store_key);
            uint32_t hash = btkvs->hash((store_key_t*) store_key), slice = btkvs->slice_nr((store_key_t *) store_key);

            char res[200]; /* man would my life be pleasant if I actually knew c++ */
            snprintf(res, 200, "Hash: %X, Slice: %d\n", hash, slice);

            return std::string(res);
        }
    };

    hash_control_t hash_control;
};

// Stats

extern perfmon_duration_sampler_t
    pm_cmd_set,
    pm_cmd_get,
    pm_cmd_get_without_threads;

#endif /* __BTREE_KEY_VALUE_STORE_HPP__ */
