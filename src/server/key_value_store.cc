#include "server/key_value_store.hpp"

#include "btree/rget.hpp"
#include "concurrency/cond_var.hpp"
#include "concurrency/pmap.hpp"
#include "db_thread_info.hpp"
#include "replication/backfill.hpp"
#include "replication/master.hpp"
#include "cmd_args.hpp"

/* shard_store_t */

shard_store_t::shard_store_t(
    translator_serializer_t *translator_serializer,
    mirrored_cache_config_t *dynamic_config,
    int64_t delete_queue_limit) :
    btree(translator_serializer, dynamic_config, delete_queue_limit),
    dispatching_store(&btree),
    timestamper(&dispatching_store)
    { }

get_result_t shard_store_t::get(const store_key_t &key, order_token_t token) {
    on_thread_t th(home_thread());
    sink.check_out(token);
    order_token_t substore_token = substore_order_source.check_in().with_read_mode();
    // We need to let gets reorder themselves, and haven't implemented that yet.
    return btree.get(key, substore_token);
}

rget_result_t shard_store_t::rget(rget_bound_mode_t left_mode, const store_key_t &left_key, rget_bound_mode_t right_mode, const store_key_t &right_key, order_token_t token) {
    on_thread_t th(home_thread());
    sink.check_out(token);
    order_token_t substore_token = substore_order_source.check_in().with_read_mode();
    // We need to let gets reorder themselves, and haven't implemented that yet.
    return btree.rget(left_mode, left_key, right_mode, right_key, substore_token);
}

mutation_result_t shard_store_t::change(const mutation_t &m, order_token_t token) {
    on_thread_t th(home_thread());
    sink.check_out(token);
    order_token_t substore_token = substore_order_source.check_in();
    return timestamper.change(m, substore_token);
}

mutation_result_t shard_store_t::change(const mutation_t &m, castime_t ct, order_token_t token) {
    /* Bypass the timestamper because we already have a castime_t */
    on_thread_t th(home_thread());
    sink.check_out(token);
    order_token_t substore_token = substore_order_source.check_in();
    return dispatching_store.change(m, ct, substore_token);
}

void shard_store_t::delete_all_keys_for_backfill(order_token_t token) {
    on_thread_t th(home_thread());
    sink.check_out(token);
    order_token_t substore_token = substore_order_source.check_in();
    btree.delete_all_keys_for_backfill(substore_token);
}

/* btree_key_value_store_t */

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

void prep_for_shard(
        translator_serializer_t **pseudoserializers,
        mirrored_cache_static_config_t *static_config,
        int i) {

    on_thread_t thread_switcher(i % get_num_db_threads());
    btree_slice_t::create(pseudoserializers[i], static_config);
}

void destroy_serializer(standard_serializer_t **serializers, int i) {
    on_thread_t thread_switcher(serializers[i]->home_thread());
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

        /* Initialize the btrees. */
        pmap(multiplexer.proxies.size(), boost::bind(&prep_for_shard, multiplexer.proxies.data(), &static_config->cache, _1));
    }

    /* Shut down serializers */
    pmap(n_files, boost::bind(&destroy_serializer, serializers, _1));
}

void create_existing_shard(
        translator_serializer_t **pseudoserializers,
        shard_store_t **shards,
        mirrored_cache_config_t *dynamic_config,
        int64_t delete_queue_limit,
        int i) {

    // TODO try to align slices with serializers so that when possible, a slice is on the
    // same thread as its serializer
    on_thread_t thread_switcher(i % get_num_db_threads());

    shards[i] = new shard_store_t(pseudoserializers[i], dynamic_config, delete_queue_limit);
}

btree_key_value_store_t::btree_key_value_store_t(btree_key_value_store_dynamic_config_t *dynamic_config)
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

    /* Divide resources among the several slices */
    mirrored_cache_config_t per_slice_config = dynamic_config->cache;
    per_slice_config.max_size = std::max(dynamic_config->cache.max_size / btree_static_config.n_slices, 1LL);
    per_slice_config.max_dirty_size = std::max(dynamic_config->cache.max_dirty_size / btree_static_config.n_slices, 1LL);
    per_slice_config.flush_dirty_size = std::max(dynamic_config->cache.flush_dirty_size / btree_static_config.n_slices, 1LL);
    per_slice_config.io_priority_reads = std::max(dynamic_config->cache.io_priority_reads / btree_static_config.n_slices, 1);
    per_slice_config.io_priority_writes = std::max(dynamic_config->cache.io_priority_writes / btree_static_config.n_slices, 1);
    int64_t per_slice_delete_queue_limit = dynamic_config->total_delete_queue_limit / btree_static_config.n_slices;

    /* Load btrees */
    pmap(btree_static_config.n_slices,
         boost::bind(&create_existing_shard,
                     multiplexer->proxies.data(), shards,
                     &per_slice_config, per_slice_delete_queue_limit, _1));

    /* Initialize the timestampers to the timestamp value on disk */
    repli_timestamp_t t = get_replication_clock();
    set_timestampers(t);
}

static void set_one_timestamper(shard_store_t **shards, int i, repli_timestamp_t t) {
    // TODO: Do we really need to wait for the operation to finish before returning?
    on_thread_t th(shards[i]->timestamper.home_thread());
    shards[i]->timestamper.set_timestamp(t);
}

void btree_key_value_store_t::set_timestampers(repli_timestamp_t t) {
    pmap(btree_static_config.n_slices, boost::bind(&set_one_timestamper, shards, _1, t));
}

void destroy_shard(
        shard_store_t **shards,
        int i) {

    on_thread_t thread_switcher(shards[i]->home_thread());

    delete shards[i];
}

btree_key_value_store_t::~btree_key_value_store_t() {
    /* Shut down btrees */
    pmap(btree_static_config.n_slices, boost::bind(&destroy_shard, shards, _1));

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


void btree_key_value_store_t::set_replication_clock(repli_timestamp_t t, order_token_t token) {

    /* Update the value on disk */
    shards[0]->btree.set_replication_clock(t, token);
}

repli_timestamp btree_key_value_store_t::get_replication_clock() {
    return shards[0]->btree.get_replication_clock();   /* Read the value from disk */
}

void btree_key_value_store_t::set_last_sync(repli_timestamp_t t, order_token_t token) {
    shards[0]->btree.set_last_sync(t, token);   /* Write the value to disk */
}

repli_timestamp btree_key_value_store_t::get_last_sync() {
    return shards[0]->btree.get_last_sync();   /* Read the value from disk */
}

void btree_key_value_store_t::set_replication_master_id(uint32_t t) {
    shards[0]->btree.set_replication_master_id(t);
}

uint32_t btree_key_value_store_t::get_replication_master_id() {
    return shards[0]->btree.get_replication_master_id();
}

void btree_key_value_store_t::set_replication_slave_id(uint32_t t) {
    shards[0]->btree.set_replication_slave_id(t);
}

uint32_t btree_key_value_store_t::get_replication_slave_id() {
    return shards[0]->btree.get_replication_slave_id();
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

get_result_t btree_key_value_store_t::get(const store_key_t &key, order_token_t token) {
    return shards[slice_num(key)]->get(key, token);
}

typedef merge_ordered_data_iterator_t<key_with_data_provider_t,key_with_data_provider_t::less> merged_results_iterator_t;

rget_result_t btree_key_value_store_t::rget(rget_bound_mode_t left_mode, const store_key_t &left_key, rget_bound_mode_t right_mode, const store_key_t &right_key, order_token_t token) {

    boost::shared_ptr<merged_results_iterator_t> merge_iterator(new merged_results_iterator_t());
    for (int s = 0; s < btree_static_config.n_slices; s++) {
        merge_iterator->add_mergee(shards[s]->rget(left_mode, left_key, right_mode, right_key, token));
    }
    return merge_iterator;
}

/* set_store_interface_t interface */

perfmon_duration_sampler_t pm_store_change_1("store_change_1", secs_to_ticks(1.0));

mutation_result_t btree_key_value_store_t::change(const mutation_t &m, order_token_t token) {
    block_pm_duration timer(&pm_store_change_1);
    return shards[slice_num(m.get_key())]->change(m, token);
}

/* set_store_t interface */

perfmon_duration_sampler_t pm_store_change_2("store_change_2", secs_to_ticks(1.0));

mutation_result_t btree_key_value_store_t::change(const mutation_t &m, castime_t ct, order_token_t token) {
    block_pm_duration timer(&pm_store_change_2);
    return shards[slice_num(m.get_key())]->change(m, ct, token);
}

/* btree_key_value_store_t interface */

void btree_key_value_store_t::delete_all_keys_for_backfill(order_token_t token) {
    for (int i = 0; i < btree_static_config.n_slices; ++i) {
        coro_t::spawn_now(boost::bind(&shard_store_t::delete_all_keys_for_backfill, shards[i], token));
    }
}
