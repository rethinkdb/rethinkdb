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
                            replication::master_t *master);

    // Blocks
    ~btree_key_value_store_t();

    struct check_callback_t {
        virtual void on_store_check(bool valid) = 0;
        virtual ~check_callback_t() {}
    };
    static void check_existing(const std::vector<std::string>& db_filenames, check_callback_t *cb);

public:
    /* get_store_t interface */

    get_result_t get(const store_key_t &key);
    rget_result_t rget(rget_bound_mode_t left_mode, const store_key_t &left_key, rget_bound_mode_t right_mode, const store_key_t &right_key);

    /* set_store_interface_t interface */

    mutation_result_t change(const mutation_t &m);

    /* set_store_t interface */

    mutation_result_t change(const mutation_t &m, castime_t ct);

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

    uint32_t slice_num(const store_key_t &key);
    set_store_interface_t *slice_for_key_set_interface(const store_key_t &key);
    set_store_t *slice_for_key_set(const store_key_t &key);
    get_store_t *slice_for_key_get(const store_key_t &key);

    static uint32_t hash(const store_key_t &key);

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
            : control_t("hash", std::string("Get hash, slice, and thread of a key. Syntax: rdb hash key")), btkvs(btkvs)
#else
            : control_t("hash", std::string("")), btkvs(btkvs)
#endif
        {}
        ~hash_control_t() {};

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
                int thread = btkvs->btrees[slice]->home_thread;

                result += strprintf("%*s: %08x [slice: %03u, thread: %03d]\r\n", int(strlen(argv[i])), argv[i], hash, slice, thread);
            }
            return result;
        }
    };

    hash_control_t hash_control;
};

#endif /* __BTREE_KEY_VALUE_STORE_HPP__ */
