#ifndef __BTREE_KEY_VALUE_STORE_HPP__
#define __BTREE_KEY_VALUE_STORE_HPP__

#include "utils.hpp"
#include <boost/scoped_ptr.hpp>

#include "buffer_cache/buffer_cache.hpp"
#include "btree/slice.hpp"
#include "btree/node.hpp"
#include "concurrency/access.hpp"
#include "concurrency/signal.hpp"
#include "server/key_value_store_config.hpp"
#include "stats/control.hpp"
#include "server/dispatching_store.hpp"
#include "serializer/config.hpp"
#include "serializer/translator.hpp"
#include "memcached/store.hpp"

namespace replication {
    class backfill_and_streaming_manager_t;
}  // namespace replication

// If the database is not a slave, then get_replication_creation_timestamp() is NOT_A_SLAVE.
// If the database is a slave, then get_replication_creation_timestamp() returns the creation
// timestamp of the master.
#define NOT_A_SLAVE uint32_t(0xFFFFFFFF)

/* sharded_key_value_store_t represents a collection of serializers and slices, possibly distributed
across several cores. */

struct shard_store_t :
    public home_thread_mixin_t,
    public set_store_interface_t,
    public set_store_t,
    public get_store_t
{
    shard_store_t(
        serializer_t *serializer,
        mirrored_cache_config_t *dynamic_config);

    get_result_t get(const store_key_t &key, order_token_t token);
    rget_result_t rget(rget_bound_mode_t left_mode, const store_key_t &left_key, rget_bound_mode_t right_mode, const store_key_t &right_key, order_token_t token);
    mutation_result_t change(const mutation_t &m, order_token_t token);
    mutation_result_t change(const mutation_t &m, castime_t ct, order_token_t token);
    void set_replication_clock(repli_timestamp_t t, order_token_t token);

    cache_t cache;
    btree_slice_t btree;
    dispatching_store_t dispatching_store;   // For replication
    timestamping_set_store_interface_t timestamper;
};

namespace btree_store_helpers {
    // These helper functions are currently used by both btree_key_value_store_t
    // and btree_metadata_store_t
    void create_existing_shard(
        shard_store_t **shard,
        int i,
        serializer_t *serializer,
        mirrored_cache_config_t *dynamic_config);

    void create_existing_serializer(standard_serializer_t **serializer, int i,
        log_serializer_dynamic_config_t *config,
        log_serializer_private_dynamic_config_t *privconfig);

    void prep_serializer(
        serializer_t *serializer,
        mirrored_cache_static_config_t *static_config,
        int i);

    void prep_serializer_for_shard(
        translator_serializer_t **pseudoserializers,
        mirrored_cache_static_config_t *static_config,
        int i);

    void prep_for_serializer(
        btree_key_value_store_dynamic_config_t *dynamic_config,
        btree_key_value_store_static_config_t *static_config,
        int i);

    void create_existing_shard_serializer(
        btree_key_value_store_dynamic_config_t *dynamic_config,
        standard_serializer_t **serializers,
        int i);

    void create_existing_data_shard(
        shard_store_t **shards,
        int i,
        translator_serializer_t **pseudoserializers,
        mirrored_cache_config_t *dynamic_config);

    void destroy_serializer(standard_serializer_t *serializer);

    void destroy_shard_serializer(standard_serializer_t **serializers, int i);
}

class btree_key_value_store_t :
    public home_thread_mixin_t,
    public get_store_t,
    /* The btree_key_value_store_t can take changes with or without castimes. It takes changes with
    castimes if they are coming from a replication slave; it takes changes without castimes if they
    are coming directly from a memcached handler. This is kind of hacky. */
    public set_store_interface_t,
    public set_store_t
{
public:
    // Blocks
    static void create(btree_key_value_store_dynamic_config_t *dynamic_config,
                       btree_key_value_store_static_config_t *static_config);

    // Blocks
    btree_key_value_store_t(const btree_key_value_store_dynamic_config_t &dynamic_config);

    // Blocks
    ~btree_key_value_store_t();

    struct check_callback_t {
        virtual void on_store_check(bool valid) = 0;
        virtual ~check_callback_t() {}
    };
    static void check_existing(const std::vector<std::string>& db_filenames, check_callback_t *cb);

public:
    get_result_t get(const store_key_t &key, order_token_t token);
    rget_result_t rget(rget_bound_mode_t left_mode, const store_key_t &left_key, rget_bound_mode_t right_mode, const store_key_t &right_key, order_token_t token);

    mutation_result_t change(const mutation_t &m, order_token_t order_token);

    mutation_result_t change(const mutation_t &m, castime_t ct, order_token_t order_token);

    // Deletes the keys in the inclusive range [low_key, high_key] for
    // which hash(hash_value) % hashmod == slice.
    void backfill_delete_range(int hash_value, int hashmod, store_key_t low_key, store_key_t high_key, order_token_t token);

    /* The value passed to `set_timestampers()` is the value that will be used as the
    timestamp for all new operations. When the key-value store starts up, it is
    initialized to the value of `get_replication_clock()`. */
    void set_timestampers(repli_timestamp_t t);

    /* These values are persisted to disk, but except for initializing the value of
    `set_timestampers()`, changing them doesn't change anything in the
    `btree_key_value_store_t` itself. They are used by the higher-level code to persist
    metadata to disk. */
    void set_replication_clock(repli_timestamp_t t, order_token_t token);
    repli_timestamp_t get_replication_clock();
    void set_last_sync(repli_timestamp_t t, order_token_t token);
    repli_timestamp_t get_last_sync();
    void set_replication_master_id(uint32_t ts);
    uint32_t get_replication_master_id();
    void set_replication_slave_id(uint32_t ts);
    uint32_t get_replication_slave_id();

    static uint32_t hash(const store_key_t& key);

    creation_timestamp_t get_creation_timestamp() const { return multiplexer->creation_timestamp; }

private:
    friend class replication::backfill_and_streaming_manager_t;

    int n_files;
    btree_config_t btree_static_config;
    mirrored_cache_static_config_t cache_static_config;
    btree_key_value_store_dynamic_config_t store_dynamic_config;

    /* These five things are declared in reverse order from how they are used. (The order they are
    declared is the order in which they are created.)

    Sets are sent to the appropriate timestamper, which gives it a castime_t, puts it on the
    correct core, and sends it to the appropriate dispatcher. The dispatcher sends a copy to any
    replication slaves and then forwards it to the appropriate btree. The btree makes whatever
    changes are appropriate in its local cache, and eventually flushes it to disk via the
    multiplexer and the serializers.

    Gets go straight to the btree.

    Sets that come from a master during replication come via the set_store_t interface, and they
    are treated the same as normal sets except that they bypass the timestamper because they already
    have timestamps. */

    standard_serializer_t *serializers[MAX_SERIALIZERS];
    serializer_multiplexer_t *multiplexer;   // Helps us split the serializers among the slices

    shard_store_t *shards[MAX_SLICES];
    uint32_t slice_num(const store_key_t &key);

    /* slice debug control_t which allows us to see slice and hash for a key */
    class hash_control_t :
        public control_t
    {
    public:
        hash_control_t(btree_key_value_store_t *btkvs)
            : control_t("hash", std::string("Get hash, slice, and thread of a key. Syntax: rdb hash key"), true), btkvs(btkvs)
        {}
        virtual ~hash_control_t() {};

    private:
        btree_key_value_store_t *btkvs;
    public:
        std::string call(int argc, char **argv) {
            std::string result;
            store_key_t key;
            for (int i = 1; i < argc; i++) {
                if (strlen(argv[i]) > MAX_KEY_SIZE) {
                    result += strprintf("%*s: invalid key (too long)\r\n", int(strlen(argv[i])), argv[i]);
                    continue;
                }

                str_to_key(argv[i], &key);
                uint32_t hash = btkvs->hash(key);
                uint32_t slice = btkvs->slice_num(key);
                int thread = btkvs->shards[slice]->home_thread();

                result += strprintf("%*s: %08x [slice: %03u, thread: %03d]\r\n", int(strlen(argv[i])), argv[i], hash, slice, thread);
            }
            return result;
        }
    };

    // TODO: Re-enable the hash control
// hash_control_t hash_control;

    DISABLE_COPYING(btree_key_value_store_t);
};

#endif /* __BTREE_KEY_VALUE_STORE_HPP__ */
