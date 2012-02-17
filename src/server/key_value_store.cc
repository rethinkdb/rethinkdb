#include "server/key_value_store.hpp"

#include <math.h>

#include "errors.hpp"
#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>

#include "arch/timing.hpp"
#include "btree/rget.hpp"
#include "concurrency/cond_var.hpp"
#include "concurrency/pmap.hpp"
#include "concurrency/signal.hpp"
#include "concurrency/side_coro.hpp"
#include "containers/iterators.hpp"
#include "db_thread_info.hpp"
#include "btree/backfill.hpp"
#include "serializer/config.hpp"
#include "serializer/translator.hpp"
#include "server/cmd_args.hpp"
#include "stats/persist.hpp"
#include "buffer_cache/sequence_group.hpp"

/* shard_store_t */

shard_store_t::shard_store_t(serializer_t *serializer,
                             mirrored_cache_config_t *dynamic_config,
                             int bucket) :
    cache(serializer, dynamic_config, bucket),
    btree(&cache),
    dispatching_store(&btree),
    timestamper(&dispatching_store)
    { }

get_result_t shard_store_t::get(const store_key_t &key, sequence_group_t *seq_group, order_token_t token) {
    on_thread_t th(home_thread());
    // We need to let gets reorder themselves, and haven't implemented that yet.
    return dispatching_store.get(key, seq_group, token);
}

rget_result_t shard_store_t::rget(sequence_group_t *seq_group, rget_bound_mode_t left_mode, const store_key_t &left_key, rget_bound_mode_t right_mode, const store_key_t &right_key, order_token_t token) {
    on_thread_t th(home_thread());

    // We need to let gets reorder themselves, and haven't implemented that yet.
    return dispatching_store.rget(seq_group, left_mode, left_key, right_mode, right_key, token);
}

mutation_result_t shard_store_t::change(sequence_group_t *seq_group, const mutation_t &m, order_token_t token) {
    on_thread_t th(home_thread());
    return timestamper.change(seq_group, m, token);
}

mutation_result_t shard_store_t::change(sequence_group_t *seq_group, const mutation_t &m, castime_t ct, order_token_t token) {
    /* Bypass the timestamper because we already have a castime_t */
    on_thread_t th(home_thread());
    return dispatching_store.change(seq_group, m, ct, token);
}

void shard_store_t::backfill_delete_range(sequence_group_t *seq_group, int hash_value, int hashmod,
                                          bool left_key_supplied, const store_key_t& left_key_exclusive,
                                          bool right_key_supplied, const store_key_t& right_key_inclusive,
                                          order_token_t token) {
    on_thread_t th(home_thread());
    return dispatching_store.backfill_delete_range(seq_group, hash_value, hashmod, left_key_supplied, left_key_exclusive, right_key_supplied, right_key_inclusive, token);
}

void shard_store_t::set_replication_clock(sequence_group_t *seq_group, repli_timestamp_t t, order_token_t token) {
    on_thread_t th(home_thread());
    dispatching_store.set_replication_clock(seq_group, t, token);
}

/* btree_store_helpers */
void btree_store_helpers::prep_for_serializer(
        btree_key_value_store_dynamic_config_t *dynamic_config,
        btree_key_value_store_static_config_t *static_config,
        int i) {
    /* Prepare the file */
    standard_serializer_t::create(
        dynamic_config->serializer,
        dynamic_config->serializer_private[i],
        static_config->serializer);
}

void btree_store_helpers::create_existing_serializer(standard_serializer_t **serializer, int i,
                                log_serializer_dynamic_config_t *config,
                                log_serializer_private_dynamic_config_t *privconfig) {
    on_thread_t switcher(i % get_num_db_threads());
    *serializer = new standard_serializer_t(*config, *privconfig);
}

void btree_store_helpers::create_existing_shard_serializer(
        btree_key_value_store_dynamic_config_t *dynamic_config,
        standard_serializer_t **serializers,
        int i) {
    btree_store_helpers::create_existing_serializer(&serializers[i], i,
                               &dynamic_config->serializer,
                               &dynamic_config->serializer_private[i]);
}

void btree_store_helpers::prep_serializer(
        serializer_t *serializer,
        mirrored_cache_static_config_t *static_config,
        int i) {

    on_thread_t thread_switcher(i % get_num_db_threads());

    /* Prepare the serializer to hold a cache */
    cache_t::create(serializer, static_config);\

    /* Construct a cache so that the btree code can write its superblock */

    /* The values we pass here are almost totally irrelevant. The cache-size parameter must
be big enough to hold the patch log so we don't trip an assert, though. */
    mirrored_cache_config_t startup_dynamic_config;
    int size = static_config->n_patch_log_blocks * serializer->get_block_size().ser_value() + MEGABYTE;
    startup_dynamic_config.max_size = size * 2;
    startup_dynamic_config.wait_for_flush = false;
    startup_dynamic_config.flush_timer_ms = NEVER_FLUSH;
    startup_dynamic_config.max_dirty_size = size;
    startup_dynamic_config.flush_dirty_size = size;
    startup_dynamic_config.flush_waiting_threshold = INT_MAX;
    startup_dynamic_config.max_concurrent_flushes = 1;
    startup_dynamic_config.io_priority_reads = 100;
    startup_dynamic_config.io_priority_writes = 100;

    /* Cache is in a scoped pointer because it may be too big to allocate on the coroutine stack */
    boost::scoped_ptr<cache_t> cache(new cache_t(serializer, &startup_dynamic_config, i));

    /* Ask the btree code to write its superblock */
    btree_slice_t::create(cache.get());
}
int compute_shard_home_thread(int shard_number, int num_db_threads) {
    return shard_number % num_db_threads;
}

void btree_store_helpers::prep_serializer_for_shard(
        translator_serializer_t **pseudoserializers,
        mirrored_cache_static_config_t *static_config,
        int i) {
    btree_store_helpers::prep_serializer(pseudoserializers[i], static_config, i);
}

void btree_store_helpers::destroy_serializer(standard_serializer_t *serializer) {
    on_thread_t thread_switcher(serializer->home_thread());
    delete serializer;
}

void btree_store_helpers::destroy_shard_serializer(standard_serializer_t **serializers, int i) {
    btree_store_helpers::destroy_serializer(serializers[i]);
}

void btree_store_helpers::create_existing_shard(
        shard_store_t **shard,
        int i,
        serializer_t *serializer,
        mirrored_cache_config_t *dynamic_config)
{
    on_thread_t thread_switcher(compute_shard_home_thread(i, get_num_db_threads()));
    *shard = new shard_store_t(serializer, dynamic_config, i);
}

void btree_store_helpers::create_existing_data_shard(
        shard_store_t **shards,
        int i,
        translator_serializer_t **pseudoserializers,
        mirrored_cache_config_t *dynamic_config)
{
    // TODO try to align slices with serializers so that when possible, a slice is on the
    // same thread as its serializer
    btree_store_helpers::create_existing_shard(&shards[i], i, pseudoserializers[i], dynamic_config);
}

/* btree_key_value_store_t */
void btree_key_value_store_t::create(btree_key_value_store_dynamic_config_t *dynamic_config,
                                     btree_key_value_store_static_config_t *static_config) {

    int n_files = dynamic_config->serializer_private.size();
    rassert(n_files > 0);
    rassert(n_files <= MAX_SERIALIZERS);

    /* Wipe out contents of files and initialize with an empty serializer */
    pmap(n_files, boost::bind(&btree_store_helpers::prep_for_serializer,
        dynamic_config, static_config, _1));

    /* Create serializers so we can initialize their contents */
    std::vector<standard_serializer_t *> serializers(n_files);
    pmap(n_files, boost::bind(&btree_store_helpers::create_existing_shard_serializer,
                              dynamic_config, serializers.data(), _1));
    {
        /* Prepare serializers for multiplexing */
        std::vector<standard_serializer_t *> serializers_for_multiplexer(n_files);
        for (int i = 0; i < n_files; i++) {
            serializers_for_multiplexer[i] = serializers[i];
        }
        serializer_multiplexer_t::create(serializers_for_multiplexer, static_config->btree.n_slices);

        /* Create pseudoserializers */
        serializer_multiplexer_t multiplexer(serializers_for_multiplexer);

        /* Initialize the btrees. */
        pmap(multiplexer.proxies.size(), boost::bind(
                 &btree_store_helpers::prep_serializer_for_shard, multiplexer.proxies.data(), &static_config->cache, _1));
    }

    /* Shut down serializers */
    pmap(n_files, boost::bind(&btree_store_helpers::destroy_shard_serializer, serializers.data(), _1));
}

static mirrored_cache_config_t partition_cache_config(const mirrored_cache_config_t &orig, float share) {
    mirrored_cache_config_t shard = orig;
    shard.max_size = std::max((long long) floorf(orig.max_size * share), 1LL);
    shard.max_dirty_size = std::max((long long) floorf(orig.max_dirty_size * share), 1LL);
    shard.flush_dirty_size = std::max((long long) floorf(orig.flush_dirty_size * share), 1LL);
    shard.io_priority_reads = std::max((int) floorf(orig.io_priority_reads * share), 1);
    shard.io_priority_writes = std::max((int) floorf(orig.io_priority_writes * share), 1);
    return shard;
}

btree_key_value_store_t::btree_key_value_store_t(const btree_key_value_store_dynamic_config_t &dynamic_config) : shards(NULL) {
// TODO: Re-enable the hash control
// : hash_control(this) {

    /* Keep a local copy of the dynamic configuration */
    store_dynamic_config = dynamic_config;

    /* Start data shard serializers */
    n_files = store_dynamic_config.serializer_private.size();
    rassert(n_files > 0);
    rassert(n_files <= MAX_SERIALIZERS);

    for (int i = 0; i < n_files; i++) serializers[i] = NULL;
    pmap(n_files, boost::bind(&btree_store_helpers::create_existing_shard_serializer, &store_dynamic_config, serializers, _1));
    for (int i = 0; i < n_files; i++) rassert(serializers[i]);

    /* Multiplex serializers so we have enough proxy-serializers for our slices */
    std::vector<standard_serializer_t *> serializers_for_multiplexer(n_files);
    for (int i = 0; i < n_files; i++) {
        serializers_for_multiplexer[i] = serializers[i];
    }
    multiplexer = new serializer_multiplexer_t(serializers_for_multiplexer);

    btree_static_config.n_slices = multiplexer->proxies.size();
    shards = new shard_store_t*[btree_static_config.n_slices];

    // calculate what share of the resources we have give to each shard
    float shard_share = 1.0f / static_cast<float>(btree_static_config.n_slices);

    /* Divide resources among the several slices */
    mirrored_cache_config_t per_slice_config = partition_cache_config(store_dynamic_config.cache, shard_share);

    /* Load btrees */
    translator_serializer_t **pseudoserializers = multiplexer->proxies.data();
    pmap(btree_static_config.n_slices,
         boost::bind(&btree_store_helpers::create_existing_data_shard, shards, _1,
                     pseudoserializers, &per_slice_config));

    /* Initialize the timestampers to the timestamp value on disk */
    sequence_group_t seq_group(btree_static_config.n_slices);
    repli_timestamp_t t = get_replication_clock(&seq_group);
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

void destroy_shard(shard_store_t **shards, int i) {
    on_thread_t thread_switcher(shards[i]->home_thread());
    delete shards[i];
}

btree_key_value_store_t::~btree_key_value_store_t() {
    /* Shut down btrees */
    pmap(btree_static_config.n_slices, boost::bind(&destroy_shard, shards, _1));

    /* Destroy proxy-serializers */
    delete multiplexer;

    /* Shut down serializers */
    pmap(n_files, boost::bind(&btree_store_helpers::destroy_shard_serializer, serializers, _1));

    delete[] shards;
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


void btree_key_value_store_t::set_replication_clock(sequence_group_t *seq_group, repli_timestamp_t t, order_token_t token) {
    shards[0]->set_replication_clock(seq_group, t, token);
}

repli_timestamp_t btree_key_value_store_t::get_replication_clock(sequence_group_t *seq_group) {
    return shards[0]->btree.get_replication_clock(seq_group);   /* Read the value from disk */
}

void btree_key_value_store_t::set_last_sync(sequence_group_t *seq_group, repli_timestamp_t t, order_token_t token) {
    shards[0]->btree.set_last_sync(seq_group, t, token);   /* Write the value to disk */
}

repli_timestamp_t btree_key_value_store_t::get_last_sync(sequence_group_t *seq_group) {
    return shards[0]->btree.get_last_sync(seq_group);   /* Read the value from disk */
}

void btree_key_value_store_t::set_replication_master_id(sequence_group_t *seq_group, uint32_t t) {
    shards[0]->btree.set_replication_master_id(seq_group, t);
}

uint32_t btree_key_value_store_t::get_replication_master_id(sequence_group_t *seq_group) {
    return shards[0]->btree.get_replication_master_id(seq_group);
}

void btree_key_value_store_t::set_replication_slave_id(sequence_group_t *seq_group, uint32_t t) {
    shards[0]->btree.set_replication_slave_id(seq_group, t);
}

uint32_t btree_key_value_store_t::get_replication_slave_id(sequence_group_t *seq_group) {
    return shards[0]->btree.get_replication_slave_id(seq_group);
}

/* Hashing keys and choosing a slice for each key */

/* The following hash function was developed by Paul Hsieh, its source
* is taken from <http://www.azillionmonkeys.com/qed/hash.html>.
* According to the site, the source is licensed under LGPL 2.1.
*/
#define get16bits(d) (uint32_t(*reinterpret_cast<const uint16_t *>(d)))

uint32_t btree_key_value_store_t::hash(const store_key_t& key) {
    return hash(key.contents, key.size);
}

uint32_t btree_key_value_store_t::hash(const char *data, int len) {
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

creation_timestamp_t btree_key_value_store_t::get_creation_timestamp() const {
    return multiplexer->creation_timestamp;
}

uint32_t btree_key_value_store_t::slice_num(const store_key_t &key) {
    return hash(key) % btree_static_config.n_slices;
}

get_result_t btree_key_value_store_t::get(const store_key_t &key, sequence_group_t *seq_group, order_token_t token) {
    return shards[slice_num(key)]->get(key, seq_group, token);
}

typedef merge_ordered_data_iterator_t<key_with_data_buffer_t,key_with_data_buffer_t::less> merged_results_iterator_t;

rget_result_t btree_key_value_store_t::rget(sequence_group_t *seq_group, rget_bound_mode_t left_mode, const store_key_t &left_key, rget_bound_mode_t right_mode, const store_key_t &right_key, order_token_t token) {

    boost::shared_ptr<merged_results_iterator_t> merge_iterator(new merged_results_iterator_t());
    for (int s = 0; s < btree_static_config.n_slices; s++) {
        merge_iterator->add_mergee(shards[s]->rget(seq_group, left_mode, left_key, right_mode, right_key, token));
    }
    return merge_iterator;
}

/* set_store_interface_t interface */

perfmon_duration_sampler_t pm_store_change_1("store_change_1", secs_to_ticks(1.0));

mutation_result_t btree_key_value_store_t::change(sequence_group_t *seq_group, const mutation_t &m, order_token_t token) {
    block_pm_duration timer(&pm_store_change_1);
    return shards[slice_num(m.get_key())]->change(seq_group, m, token);
}

/* set_store_t interface */

perfmon_duration_sampler_t pm_store_change_2("store_change_2", secs_to_ticks(1.0));

mutation_result_t btree_key_value_store_t::change(sequence_group_t *seq_group, const mutation_t &m, castime_t ct, order_token_t token) {
    block_pm_duration timer(&pm_store_change_2);
    return shards[slice_num(m.get_key())]->change(seq_group, m, ct, token);
}





void btree_key_value_store_t::backfill_delete_range(sequence_group_t *seq_group, int hash_value, int hashmod, bool left_key_supplied, const store_key_t& left_key_exclusive, bool right_key_supplied, const store_key_t& right_key_inclusive, order_token_t token) {
    // This has to be a bit fancy because we only want to call the
    // change function on shards where hash(key) % hashmod ==
    // hash_value is possible.  Visit slices where hash(slice_num) is
    // congruent to hash_value, modulo gcd(hashmod, n_slices).

    int g = gcd(hashmod, btree_static_config.n_slices);

    int h = hash_value % g;

    for (int i = h; i < btree_static_config.n_slices; i += g) {
        shards[i]->backfill_delete_range(seq_group, hash_value, hashmod, left_key_supplied, left_key_exclusive, right_key_supplied, right_key_inclusive, token);
    }
}
