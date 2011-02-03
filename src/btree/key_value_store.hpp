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
    get_result_t get_cas(store_key_t *key, castime_t castime);
    rget_result_t rget(store_key_t *start, store_key_t *end, bool left_open, bool right_open, uint64_t max_results);
    set_result_t set(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, castime_t castime);
    set_result_t add(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, castime_t castime);
    set_result_t replace(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, castime_t castime);
    set_result_t cas(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, cas_t unique, castime_t castime);
    incr_decr_result_t incr(store_key_t *key, unsigned long long amount, castime_t castime);
    incr_decr_result_t decr(store_key_t *key, unsigned long long amount, castime_t castime);
    append_prepend_result_t append(store_key_t *key, data_provider_t *data, castime_t castime);
    append_prepend_result_t prepend(store_key_t *key, data_provider_t *data, castime_t castime);
    delete_result_t delete_key(store_key_t *key, repli_timestamp timestamp);

public:
    int n_files;
    btree_config_t btree_static_config;

    /* The key-value store typically has more slices than serializers. The slices share
    serializers via the "pseudoserializers": translator-serializers, one per slice, that
    multiplex requests onto the actual serializers. */
    standard_serializer_t *serializers[MAX_SERIALIZERS];
    translator_serializer_t *pseudoserializers[MAX_SLICES];
    btree_slice_t *slices[MAX_SLICES];

    uint32_t slice_num(const btree_key *key);
    btree_slice_t *slice_for_key(const btree_key *key);

    static int compute_mod_count(int32_t file_number, int32_t n_files, int32_t n_slices);
    static uint32_t hash(const btree_key *key);

private:
    DISABLE_COPYING(btree_key_value_store_t);

private:
    /* slice debug control_t which allows us to grab, slice and hash for a key */
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
                int thread = btkvs->slice_for_key((store_key_t *) store_key)->home_thread;

                result += strprintf("%*s: %08lx [slice: %03lu, thread: %03d]\r\n", key.length(), key.c_str(), hash, slice, thread);
            }
            return result;
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
