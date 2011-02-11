#ifndef __BTREE_KEY_VALUE_STORE_HPP__
#define __BTREE_KEY_VALUE_STORE_HPP__

#include "btree/slice.hpp"
#include "server/slice_dispatching_to_master.hpp"
#include "btree/node.hpp"
#include "utils.hpp"
#include "concurrency/access.hpp"
#include "server/cmd_args.hpp"
#include "arch/arch.hpp"
#include "serializer/config.hpp"
#include "serializer/translator.hpp"
#include "store.hpp"
#include "control.hpp"

namespace replication {
class master_t;
}  // namespace replication


/* btree_key_value_store_t represents a collection of serializers and slices, possibly distributed
across several cores. */

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
    btree_key_value_store_t(btree_key_value_store_dynamic_config_t *dynamic_config,
                            snag_ptr_t<replication::master_t> master);

    // Blocks
    ~btree_key_value_store_t();

    struct check_callback_t {
        virtual void on_store_check(bool valid) = 0;
        virtual ~check_callback_t() {}
    };
    static void check_existing(const std::vector<std::string>& db_filenames, check_callback_t *cb);

public:
    /* get_store_t interface */

    get_result_t get(store_key_t *key);
    rget_result_t rget(store_key_t *start, store_key_t *end, bool left_open, bool right_open);

    /* set_store_interface_t interface */

    get_result_t get_cas(store_key_t *key);
    set_result_t sarc(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, add_policy_t add_policy, replace_policy_t replace_policy, cas_t old_cas);

    incr_decr_result_t incr_decr(incr_decr_kind_t kind, store_key_t *key, uint64_t amount);
    append_prepend_result_t append_prepend(append_prepend_kind_t kind, store_key_t *key, data_provider_t *data);

    delete_result_t delete_key(store_key_t *key);

    /* set_store_t interface */

    get_result_t get_cas(store_key_t *key, castime_t castime);
    
    set_result_t sarc(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, castime_t castime,
                      add_policy_t add_policy, replace_policy_t replace_policy, cas_t old_cas);
    delete_result_t delete_key(store_key_t *key, repli_timestamp timestamp);

    incr_decr_result_t incr_decr(incr_decr_kind_t kind, store_key_t *key, uint64_t amount, castime_t castime);
    append_prepend_result_t append_prepend(append_prepend_kind_t kind, store_key_t *key, data_provider_t *data, castime_t castime);

public:
    int n_files;
    btree_config_t btree_static_config;
    mirrored_cache_static_config_t cache_static_config;

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
    btree_slice_t *btrees[MAX_SLICES];
    btree_slice_dispatching_to_master_t *dispatchers[MAX_SLICES];
    timestamping_set_store_interface_t *timestampers[MAX_SLICES];

    uint32_t slice_num(const btree_key *key);
    set_store_interface_t *slice_for_key_set_interface(const btree_key *key);
    set_store_t *slice_for_key_set(const btree_key *key);
    get_store_t *slice_for_key_get(const btree_key *key);

    static uint32_t hash(const btree_key *key);

private:
    DISABLE_COPYING(btree_key_value_store_t);

private:
    /* slice debug control_t which allows us to see slice and hash for a key */
    class hash_control_t :
        public control_t
    {
    public:
        hash_control_t(btree_key_value_store_t *btkvs)
#ifndef NDEBUG  //No documentation if we're in release mode.
            : control_t("hash", std::string("Get hash, slice, and thread of a key. Syntax: rdb hash: key")), btkvs(btkvs)
#else
            : control_t("hash", std::string("")), btkvs(btkvs)
#endif
        {}
        ~hash_control_t() {};

    private:
        btree_key_value_store_t *btkvs;
    public:
        std::string call(std::string keys) {
            std::string result;
            std::stringstream str(keys);

            while (!str.eof()) {
                std::string key;
                str >> key;

                char store_key[MAX_KEY_SIZE+sizeof(store_key_t)];
                str_to_key(key.c_str(), (store_key_t *) store_key);

                uint32_t hash = btkvs->hash((store_key_t*) store_key);
                uint32_t slice = btkvs->slice_num((store_key_t *) store_key);
                int thread = btkvs->btrees[slice]->home_thread;

                result += strprintf("%*s: %08x [slice: %03u, thread: %03d]\r\n", int(key.length()), key.c_str(), hash, slice, thread);
            }
            return result;
        }
    };

    hash_control_t hash_control;
};

#endif /* __BTREE_KEY_VALUE_STORE_HPP__ */
