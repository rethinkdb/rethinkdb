#include "server/key_value_store.hpp"
#include "btree/rget.hpp"
#include "server/slice_dispatching_to_master.hpp"
#include "concurrency/cond_var.hpp"
#include "concurrency/pmap.hpp"
#include "db_thread_info.hpp"

void prep_for_serializer(
        btree_key_value_store_dynamic_config_t *dynamic_config,
        btree_key_value_store_static_config_t *static_config,
        int i) {
    /* Prepare the file */
    standard_serializer_t::create(
        &dynamic_config->serializer,
        &dynamic_config->serializer_private[i],
        &static_config->serializer);
}

void create_existing_serializer(
        btree_key_value_store_dynamic_config_t *dynamic_config,
        standard_serializer_t **serializers,
        int i) {

    /* Go to an appropriate thread to run the serializer on */
    on_thread_t thread_switcher(i % get_num_db_threads());
    serializers[i] = new standard_serializer_t(
        &dynamic_config->serializer,
        &dynamic_config->serializer_private[i]);
}

void prep_for_btree(
        translator_serializer_t **pseudoserializers,
        mirrored_cache_config_t *dynamic_config,
        mirrored_cache_static_config_t *static_config,
        int i) {

    on_thread_t thread_switcher(i % get_num_db_threads());
    btree_slice_t::create(pseudoserializers[i], dynamic_config, static_config);
}

void destroy_serializer(standard_serializer_t **serializers, int i) {
    on_thread_t thread_switcher(serializers[i]->home_thread);
    delete serializers[i];
}

void btree_key_value_store_t::create(btree_key_value_store_dynamic_config_t *dynamic_config,
                                     btree_key_value_store_static_config_t *static_config) {

    int n_files = dynamic_config->serializer_private.size();
    rassert(n_files > 0);
    rassert(n_files <= MAX_SERIALIZERS);

    /* Wipe out contents of files and initialize with an empty serializer */
    pmap(n_files, boost::bind(&prep_for_serializer,
        dynamic_config, static_config, _1));

    /* Create serializers so we can initialize their contents */
    standard_serializer_t *serializers[n_files];
    pmap(n_files, boost::bind(&create_existing_serializer,
        dynamic_config, serializers, _1));

    {
        /* Prepare serializers for multiplexing */
        std::vector<serializer_t *> serializers_for_multiplexer(n_files);
        for (int i = 0; i < n_files; i++) serializers_for_multiplexer[i] = serializers[i];
        serializer_multiplexer_t::create(serializers_for_multiplexer, static_config->btree.n_slices);

        /* Create pseudoserializers */
        serializer_multiplexer_t multiplexer(serializers_for_multiplexer);

        /* Initialize the btrees. Don't bother splitting the memory between the slices since we're just
        creating, which takes almost no memory. */
        pmap(multiplexer.proxies.size(), boost::bind(&prep_for_btree, multiplexer.proxies.data(), &dynamic_config->cache, &static_config->cache, _1));
    }

    /* Shut down serializers */
    pmap(n_files, boost::bind(&destroy_serializer, serializers, _1));
}

void create_existing_btree(
        translator_serializer_t **pseudoserializers,
        btree_slice_t **btrees,
        btree_slice_dispatching_to_master_t **dispatchers,
        timestamping_set_store_interface_t **timestampers,
        mirrored_cache_config_t *dynamic_config,
        mirrored_cache_static_config_t *static_config,
        replication::master_t *master,
        int i) {

    // TODO try to align slices with serializers so that when possible, a slice is on the
    // same thread as its serializer
    on_thread_t thread_switcher(i % get_num_db_threads());

    btrees[i] = new btree_slice_t(pseudoserializers[i], dynamic_config, static_config);
    dispatchers[i] = new btree_slice_dispatching_to_master_t(btrees[i], master);
    timestampers[i] = new timestamping_set_store_interface_t(dispatchers[i]);
}

btree_key_value_store_t::btree_key_value_store_t(btree_key_value_store_dynamic_config_t *dynamic_config,
                                                 replication::master_t *master)
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
         multiplexer->proxies.data(), btrees, dispatchers, timestampers, &per_slice_config, &cache_static_config, master, _1));
}

void destroy_btree(
        btree_slice_t **btrees,
        btree_slice_dispatching_to_master_t **dispatchers,
        timestamping_set_store_interface_t **timestampers,
        int i) {

    on_thread_t thread_switcher(btrees[i]->home_thread);

    delete timestampers[i];
    delete dispatchers[i];
    delete btrees[i];
}

btree_key_value_store_t::~btree_key_value_store_t() {

    /* Shut down btrees */
    pmap(btree_static_config.n_slices, boost::bind(&destroy_btree, btrees, dispatchers, timestampers, _1));

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

/* The following hash function was developed by Paul Hsieh, its source
 * is taken from <http://www.azillionmonkeys.com/qed/hash.html>.
 * According to the site, the source is licensed under LGPL 2.1.
 */
#define get16bits(d) ((((uint32_t)(((const uint8_t *)(d))[1])) << 8)\
                       +(uint32_t)(((const uint8_t *)(d))[0]) )

uint32_t btree_key_value_store_t::hash(const store_key_t &key) {
    const char *data = key.contents;
    int len = key.size;
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

uint32_t btree_key_value_store_t::slice_num(const store_key_t &key) {
    return hash(key) % btree_static_config.n_slices;
}

set_store_interface_t *btree_key_value_store_t::slice_for_key_set_interface(const store_key_t &key) {
    return timestampers[slice_num(key)];
}

set_store_t *btree_key_value_store_t::slice_for_key_set(const store_key_t &key) {
    return dispatchers[slice_num(key)];
}

get_store_t *btree_key_value_store_t::slice_for_key_get(const store_key_t &key) {
    return btrees[slice_num(key)];
}

/* get_store_t interface */

get_result_t btree_key_value_store_t::get(const store_key_t &key) {
    return slice_for_key_get(key)->get(key);
}

typedef merge_ordered_data_iterator_t<key_with_data_provider_t,key_with_data_provider_t::less> merged_results_iterator_t;

template<class T>
struct pm_iterator_wrapper_t : one_way_iterator_t<T> {
    pm_iterator_wrapper_t(one_way_iterator_t<T> * wrapped, perfmon_duration_sampler_t *pm) :
        wrapped(wrapped), block_pm(pm) { }
    ~pm_iterator_wrapper_t() {
        delete wrapped;
    }
    typename boost::optional<T> next() {
        return wrapped->next();
    }
    void prefetch() {
        wrapped->prefetch();
    }
    one_way_iterator_t<T> *get_wrapped() {
        return wrapped;
    }
private:
    one_way_iterator_t<T> *wrapped;
    block_pm_duration block_pm;
};

rget_result_t btree_key_value_store_t::rget(rget_bound_mode_t left_mode, const store_key_t &left_key, rget_bound_mode_t right_mode, const store_key_t &right_key) {
    thread_saver_t thread_saver;

    /* TODO: Wrapped iterators are weird and we shouldn't have them. Instead we should do all of the
    perfmonning for different operations up at the memcached level. */
    unique_ptr_t<pm_iterator_wrapper_t<key_with_data_provider_t> > wrapped_iterator(new pm_iterator_wrapper_t<key_with_data_provider_t>(new merged_results_iterator_t(), &pm_cmd_rget));

    merged_results_iterator_t *merge_iterator = ptr_cast<merged_results_iterator_t>(wrapped_iterator->get_wrapped());
    for (int s = 0; s < btree_static_config.n_slices; s++) {
        merge_iterator->add_mergee(btrees[s]->rget(left_mode, left_key, right_mode, right_key).release());
    }
    return wrapped_iterator;
}

/* set_store_interface_t interface */

get_result_t btree_key_value_store_t::get_cas(const store_key_t &key) {
    return slice_for_key_set_interface(key)->get_cas(key);
}

set_result_t btree_key_value_store_t::sarc(const store_key_t &key, data_provider_t *data, mcflags_t flags, exptime_t exptime, add_policy_t add_policy, replace_policy_t replace_policy, cas_t old_cas) {
    return slice_for_key_set_interface(key)->sarc(key, data, flags, exptime, add_policy, replace_policy, old_cas);
}

incr_decr_result_t btree_key_value_store_t::incr_decr(incr_decr_kind_t kind, const store_key_t &key, uint64_t amount) {
    return slice_for_key_set_interface(key)->incr_decr(kind, key, amount);
}

append_prepend_result_t btree_key_value_store_t::append_prepend(append_prepend_kind_t kind, const store_key_t &key, data_provider_t *data) {
    return slice_for_key_set_interface(key)->append_prepend(kind, key, data);
}

delete_result_t btree_key_value_store_t::delete_key(const store_key_t &key) {
    return slice_for_key_set_interface(key)->delete_key(key);
}

/* set_store_t interface */

get_result_t btree_key_value_store_t::get_cas(const store_key_t &key, castime_t castime) {
    return slice_for_key_set(key)->get_cas(key, castime);
}

set_result_t btree_key_value_store_t::sarc(const store_key_t &key, data_provider_t *data, mcflags_t flags, exptime_t exptime, castime_t castime, add_policy_t add_policy, replace_policy_t replace_policy, cas_t old_cas) {
    return slice_for_key_set(key)->sarc(key, data, flags, exptime, castime, add_policy, replace_policy, old_cas);
}

incr_decr_result_t btree_key_value_store_t::incr_decr(incr_decr_kind_t kind, const store_key_t &key, uint64_t amount, castime_t castime) {
    return slice_for_key_set(key)->incr_decr(kind, key, amount, castime);
}

append_prepend_result_t btree_key_value_store_t::append_prepend(append_prepend_kind_t kind, const store_key_t &key, data_provider_t *data, castime_t castime) {
    return slice_for_key_set(key)->append_prepend(kind, key, data, castime);
}

delete_result_t btree_key_value_store_t::delete_key(const store_key_t &key, repli_timestamp timestamp) {
    return slice_for_key_set(key)->delete_key(key, timestamp);
}
