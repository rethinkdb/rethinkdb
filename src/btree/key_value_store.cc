#include "btree/key_value_store.hpp"
#include "btree/rget.hpp"
#include "btree/slice_dispatching_to_master.hpp"
#include "concurrency/cond_var.hpp"
#include "concurrency/pmap.hpp"
#include "db_thread_info.hpp"

void create_new_serializer(
        btree_key_value_store_dynamic_config_t *dynamic_config,
        btree_key_value_store_static_config_t *static_config,
        standard_serializer_t **serializers,
        int i) {

    /* Go to an appropriate thread to run the serializer on */
    on_thread_t thread_switcher(i % get_num_db_threads());

    /* Create the serializer */
    serializers[i] = new standard_serializer_t(
        &dynamic_config->serializer,
        &dynamic_config->serializer_private[i]);

    /* Start the serializer up */
    struct : public standard_serializer_t::ready_callback_t, public cond_t {
        void on_serializer_ready(standard_serializer_t *) { pulse(); }
    } ready_cb;
    if (!serializers[i]->start_new(&static_config->serializer, &ready_cb)) ready_cb.wait();
}

void create_existing_serializer(
        btree_key_value_store_dynamic_config_t *dynamic_config,
        standard_serializer_t **serializers,
        int i) {

    /* Go to an appropriate thread to run the serializer on */
    on_thread_t thread_switcher(i % get_num_db_threads());

    /* Create the serializer */
    serializers[i] = new standard_serializer_t(
        &dynamic_config->serializer,
        &dynamic_config->serializer_private[i]);

    /* Start the serializer up */
    struct : public standard_serializer_t::ready_callback_t, public cond_t {
        void on_serializer_ready(standard_serializer_t *) { pulse(); }
    } ready_cb;
    if (!serializers[i]->start_existing(&ready_cb)) ready_cb.wait();
}

void prep_for_btree(
        translator_serializer_t **pseudoserializers,
        mirrored_cache_config_t *config,
        int i) {

    on_thread_t thread_switcher(i % get_num_db_threads());

    btree_slice_t::create(pseudoserializers[i], config);
}

void destroy_serializer(
        standard_serializer_t **serializers,
        int i) {

    on_thread_t thread_switcher(serializers[i]->home_thread);

    struct : public standard_serializer_t::shutdown_callback_t, public cond_t {
        void on_serializer_shutdown(standard_serializer_t *) { pulse(); }
    } shutdown_cb;
    if (!serializers[i]->shutdown(&shutdown_cb)) shutdown_cb.wait();

    delete serializers[i];
}

void btree_key_value_store_t::create(btree_key_value_store_dynamic_config_t *dynamic_config,
                                     btree_key_value_store_static_config_t *static_config) {

    /* Create serializers */
    int n_files = dynamic_config->serializer_private.size();
    rassert(n_files > 0);
    rassert(n_files <= MAX_SERIALIZERS);

    standard_serializer_t *serializers[n_files];
    pmap(n_files, boost::bind(&create_new_serializer,
        dynamic_config, static_config, serializers, _1));

    {
        /* Prepare serializers for multiplexing */
        std::vector<serializer_t *> serializers_for_multiplexer(n_files);
        for (int i = 0; i < n_files; i++) serializers_for_multiplexer[i] = serializers[i];
        serializer_multiplexer_t::create(serializers_for_multiplexer, static_config->btree.n_slices);

        /* Create pseudoserializers */
        serializer_multiplexer_t multiplexer(serializers_for_multiplexer);

        /* Initialize the btrees. Don't bother splitting the memory between the slices since we're just
        creating, which takes almost no memory. */
        pmap(multiplexer.proxies.size(), boost::bind(&prep_for_btree, multiplexer.proxies.data(), &dynamic_config->cache, _1));
    }

    /* Shut down serializers */
    pmap(n_files, boost::bind(&destroy_serializer, serializers, _1));
}

void create_existing_btree(
        translator_serializer_t **pseudoserializers,
        slice_store_t **slices,
        mirrored_cache_config_t *config,
        replication::master_t *masterstore,
        int i) {

    // TODO try to align slices with serializers so that when possible, a slice is on the
    // same thread as its serializer
    on_thread_t thread_switcher(i % get_num_db_threads());

    btree_slice_t *sl = new btree_slice_t(pseudoserializers[i], config);
    //    if (masterstore) {  /* commented out to avoid temporarily breaking master.  btree_slice_dispatching_to_master_t handles NULL masterstore gracefully, for now */
        slices[i] = new btree_slice_dispatching_to_master_t(sl, masterstore);
        //    } else {
        //        slices[i] = sl;
        //    }
}

btree_key_value_store_t::btree_key_value_store_t(btree_key_value_store_dynamic_config_t *dynamic_config,
                                                 replication::master_t *masterstore)
    : hash_control(this) {

    /* Start serializers */
    n_files = dynamic_config->serializer_private.size();
    rassert(n_files > 0);
    rassert(n_files <= MAX_SERIALIZERS);

    for (int i = 0; i < n_files; i++) serializers[i] = NULL;
    pmap(n_files, boost::bind(&create_existing_serializer, dynamic_config, serializers, _1));
    for (int i = 0; i < n_files; i++) rassert(serializers[i]);

    /* Multiplex serializers so we have enough proxy-serializers for our slices */
    std::vector<serializer_t *> serializers_for_multiplexer(n_files);
    for (int i = 0; i < n_files; i++) serializers_for_multiplexer[i] = serializers[i];
    multiplexer = new serializer_multiplexer_t(serializers_for_multiplexer);

    btree_static_config.n_slices = multiplexer->proxies.size();

    /* Load btrees */
    mirrored_cache_config_t per_slice_config = dynamic_config->cache;
    /* Divvy up the memory available between the several slices. */
    per_slice_config.max_size /= btree_static_config.n_slices;
    per_slice_config.max_dirty_size /= btree_static_config.n_slices;
    per_slice_config.flush_dirty_size /= btree_static_config.n_slices;
    pmap(btree_static_config.n_slices, boost::bind(&create_existing_btree,
         multiplexer->proxies.data(), slices, &per_slice_config, masterstore, _1));
}

void destroy_btree(
        slice_store_t **slices,
        int i) {

    on_thread_t thread_switcher(slices[i]->slice_home_thread());

    delete slices[i];
}

btree_key_value_store_t::~btree_key_value_store_t() {

    /* Shut down btrees */
    pmap(btree_static_config.n_slices, boost::bind(&destroy_btree, slices, _1));

    /* Destroy proxy-serializers */
    delete multiplexer;

    /* Shut down serializers */
    pmap(n_files, boost::bind(&destroy_serializer, serializers, _1));
}

/* Function to check if any of the files seem to contain existing databases */

struct check_existing_fsm_t
    : public standard_serializer_t::check_callback_t
{
    int n_unchecked;
    btree_key_value_store_t::check_callback_t *callback;
    bool is_ok;
    check_existing_fsm_t(const std::vector<std::string>& filenames, btree_key_value_store_t::check_callback_t *cb)
        : callback(cb)
    {
        int n_files = filenames.size();
        n_unchecked = n_files;
        is_ok = true;
        for (int i = 0; i < n_files; i++)
            standard_serializer_t::check_existing(filenames[i].c_str(), this);
    }
    void on_serializer_check(bool ok) {
        is_ok = is_ok && ok;
        n_unchecked--;
        if (n_unchecked == 0) {
            callback->on_store_check(is_ok);
            delete this;
        }
    }
};

void btree_key_value_store_t::check_existing(const std::vector<std::string>& filenames, check_callback_t *cb) {
    new check_existing_fsm_t(filenames, cb);
}

/* Hashing keys and choosing a slice for each key */

#define get16bits(d) ((((uint32_t)(((const uint8_t *)(d))[1])) << 8)\
                       +(uint32_t)(((const uint8_t *)(d))[0]) )

uint32_t btree_key_value_store_t::hash(const btree_key *key) {
    const char *data = key->contents;
    int len = key->size;
    uint32_t hash = len, tmp;
    int rem;
    if (len <= 0 || data == NULL) return 0;

    rem = len & 3;
    len >>= 2;

    for (;len > 0; len--) {
        hash  += get16bits (data);
        tmp    = (get16bits (data+2) << 11) ^ hash;
        hash   = (hash << 16) ^ tmp;
        data  += 2*sizeof (uint16_t);
        hash  += hash >> 11;
    }

    switch (rem) {
        case 3: hash += get16bits (data);
                hash ^= hash << 16;
                hash ^= data[sizeof (uint16_t)] << 18;
                hash += hash >> 11;
                break;
        case 2: hash += get16bits (data);
                hash ^= hash << 11;
                hash += hash >> 17;
                break;
        case 1: hash += *data;
                hash ^= hash << 10;
                hash += hash >> 1;
                break;
        default: { }    // this space intentionally left blank
    }

    /* Force "avalanching" of final 127 bits */
    hash ^= hash << 3;
    hash += hash >> 5;
    hash ^= hash << 4;
    hash += hash >> 17;
    hash ^= hash << 25;
    hash += hash >> 6;

    return hash;
}

uint32_t btree_key_value_store_t::slice_num(const btree_key *key) {
    return hash(key) % btree_static_config.n_slices;
}

slice_store_t *btree_key_value_store_t::slice_for_key(const btree_key *key) {
    return slices[slice_num(key)];
}

/* store_t interface */

store_t::get_result_t btree_key_value_store_t::get(store_key_t *key) {
    return slice_for_key(key)->get(key);
}

store_t::get_result_t btree_key_value_store_t::get_cas(store_key_t *key, castime_t castime) {
    return slice_for_key(key)->get_cas(key, castime);
}

store_t::rget_result_t btree_key_value_store_t::rget(store_key_t *start, store_key_t *end, bool left_open, bool right_open) {
    return btree_rget(this, start, end, left_open, right_open);
}
store_t::set_result_t btree_key_value_store_t::sarc(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, castime_t castime, add_policy_t add_policy, replace_policy_t replace_policy, cas_t old_cas) {
    return slice_for_key(key)->sarc(key, data, flags, exptime, castime, add_policy, replace_policy, old_cas);
}


store_t::incr_decr_result_t btree_key_value_store_t::incr_decr(incr_decr_kind_t kind, store_key_t *key, uint64_t amount, castime_t castime) {
    return slice_for_key(key)->incr_decr(kind, key, amount, castime);
}

store_t::append_prepend_result_t btree_key_value_store_t::append_prepend(append_prepend_kind_t kind, store_key_t *key, data_provider_t *data, castime_t castime) {
    return slice_for_key(key)->append_prepend(kind, key, data, castime);
}

store_t::delete_result_t btree_key_value_store_t::delete_key(store_key_t *key, repli_timestamp timestamp) {
    return slice_for_key(key)->delete_key(key, timestamp);
}

// Stats

perfmon_duration_sampler_t
    pm_cmd_set("cmd_set", secs_to_ticks(1)),
    pm_cmd_get_without_threads("cmd_get_without_threads", secs_to_ticks(1)),
    pm_cmd_get("cmd_get", secs_to_ticks(1)),
    pm_cmd_rget("cmd_rget", secs_to_ticks(1));

