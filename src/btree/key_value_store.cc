#include "btree/key_value_store.hpp"
#include "btree/rget.hpp"
#include "btree/serializer_config_block.hpp"
#include "btree/slice_dispatching_to_masterstore.hpp"
#include "concurrency/cond_var.hpp"
#include "concurrency/pmap.hpp"
#include "db_thread_info.hpp"

/* The key-value store slices up the serializers as follows:

- Each slice is assigned to a serializer. The formula is (slice_id % n_files)
- Block ID 0 on each serializer is for static btree configuration information.
- Each slice uses block IDs of the form (1 + (n * (number of slices on serializer) +
    (slice_id / n_files))).

*/

const block_magic_t serializer_config_block_t::expected_magic = { { 'c','f','g','_' } };

void create_new_serializer(
        btree_key_value_store_dynamic_config_t *dynamic_config,
        btree_key_value_store_static_config_t *static_config,
        standard_serializer_t **serializers,
        uint32_t creation_magic,
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

    serializer_config_block_t *c = reinterpret_cast<serializer_config_block_t *>(
        serializers[i]->malloc());

    /* Prepare an initial configuration block */
    bzero(c, serializers[i]->get_block_size().value());
    c->magic = serializer_config_block_t::expected_magic;
    c->database_magic = creation_magic;
    c->n_files = dynamic_config->serializer_private.size();
    c->this_serializer = i;
    c->btree_config = static_config->btree;
    serializer_t::write_t w = serializer_t::write_t::make(CONFIG_BLOCK_ID.ser_id, repli_timestamp::invalid, c, NULL);

    /* Write the initial configuration block */
    struct : public serializer_t::write_txn_callback_t, public cond_t {
        void on_serializer_write_txn() { pulse(); }
    } write_cb;
    if (!serializers[i]->do_write(&w, 1, &write_cb)) write_cb.wait();

    serializers[i]->free(c);
}

void create_existing_serializer(
        btree_key_value_store_dynamic_config_t *dynamic_config,
        btree_config_t *static_config_out,
        standard_serializer_t **serializers,
        uint32_t *magics,
        int i) {

    /* Go to an appropriate thread to run the serializer on */
    on_thread_t thread_switcher(i % get_num_db_threads());

    /* Create the serializer */
    standard_serializer_t *serializer = new standard_serializer_t(
        &dynamic_config->serializer,
        &dynamic_config->serializer_private[i]);

    /* Start the serializer up */
    struct : public standard_serializer_t::ready_callback_t, public cond_t {
        void on_serializer_ready(standard_serializer_t *) { pulse(); }
    } ready_cb;
    if (!serializer->start_existing(&ready_cb)) ready_cb.wait();

    /* Load the config block from the serializer */
    serializer_config_block_t *c = reinterpret_cast<serializer_config_block_t *>(
        serializer->malloc());
    struct : public serializer_t::read_callback_t, public cond_t {
        void on_serializer_read() { pulse(); }
    } read_cb;
    serializer->do_read(CONFIG_BLOCK_ID.ser_id, c, &read_cb);
    read_cb.wait();

    /* Check that the config block is valid */
    if (c->n_files != (int)dynamic_config->serializer_private.size()) {
        fail_due_to_user_error("File config block for file \"%s\" says there should be %d files, but we have %d.",
            dynamic_config->serializer_private[i].db_filename.c_str(), (int)c->n_files,
            (int)dynamic_config->serializer_private.size());
    }
    rassert(check_magic<serializer_config_block_t>(c->magic));
    rassert(c->this_serializer >= 0 &&
        c->this_serializer < (int)dynamic_config->serializer_private.size());
    magics[c->this_serializer] = c->database_magic;

    if (i == 0) *static_config_out = c->btree_config;
    rassert(!serializers[c->this_serializer]);
    serializers[c->this_serializer] = serializer;

    serializer->free(c);
}

void create_pseudoserializer(
        btree_key_value_store_dynamic_config_t *dynamic_config,
        btree_config_t *btree_config,
        standard_serializer_t **serializers,
        translator_serializer_t **pseudoserializers,
        int i) {

    /* Which real serializer will this pseudoserializer map to? */
    int n_files = dynamic_config->serializer_private.size();
    serializer_t *real_serializer = serializers[i % n_files];

    /* How many other pseudoserializers are we sharing the serializer with? */
    int mod_count = btree_key_value_store_t::compute_mod_count(
        i % n_files, n_files, btree_config->n_slices);

    on_thread_t thread_switcher(real_serializer->home_thread);

    pseudoserializers[i] = new translator_serializer_t(
        real_serializer,
        mod_count,
        i / n_files,
        CONFIG_BLOCK_ID /* Reserve block ID 0 */
        );
}

void prep_for_btree(
        translator_serializer_t **pseudoserializers,
        mirrored_cache_config_t *config,
        int i) {

    on_thread_t thread_switcher(i % get_num_db_threads());

    btree_slice_t::create(pseudoserializers[i], config);
}

void create_existing_btree(
        translator_serializer_t **pseudoserializers,
        slice_store_t **slices,
        mirrored_cache_config_t *config,
        replication::masterstore_t *masterstore,
        int i) {

    // TODO try to align slices with serializers so that when possible, a slice is on the
    // same thread as its serializer
    on_thread_t thread_switcher(i % get_num_db_threads());

    btree_slice_t *sl = new btree_slice_t(pseudoserializers[i], config);
    //    if (masterstore) {  /* commented out to avoid temporarily breaking master.  btree_slice_dispatching_to_masterstore_t handles NULL masterstore gracefully, for now */
        slices[i] = new btree_slice_dispatching_to_masterstore_t(sl, masterstore);
        //    } else {
        //        slices[i] = sl;
        //    }
}

void destroy_btree(
        slice_store_t **slices,
        int i) {

    on_thread_t thread_switcher(slices[i]->slice_home_thread());

    delete slices[i];
}

void destroy_pseudoserializer(
        translator_serializer_t **pseudoserializers,
        int i) {

    on_thread_t thread_switcher(pseudoserializers[i]->home_thread);

    delete pseudoserializers[i];
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
    uint32_t creation_magic = time(NULL);
    standard_serializer_t *serializers[n_files];
    pmap(n_files, boost::bind(&create_new_serializer,
        dynamic_config, static_config, serializers, creation_magic, _1));

    /* Create pseudoserializers */
    translator_serializer_t *pseudoserializers[static_config->btree.n_slices];
    pmap(static_config->btree.n_slices, boost::bind(&create_pseudoserializer,
        dynamic_config, &static_config->btree, serializers, pseudoserializers, _1));

    /* Initialize the btrees. Don't bother splitting the memory between the slices since we're just
    creating, which takes almost no memory. */

    pmap(static_config->btree.n_slices, boost::bind(&prep_for_btree, pseudoserializers, &dynamic_config->cache, _1));

    /* Destroy pseudoserializers */
    pmap(static_config->btree.n_slices, boost::bind(&destroy_pseudoserializer, pseudoserializers, _1));

    /* Shut down serializers */
    pmap(n_files, boost::bind(&destroy_serializer, serializers, _1));
}

btree_key_value_store_t::btree_key_value_store_t(btree_key_value_store_dynamic_config_t *dynamic_config,
                                                 replication::masterstore_t *masterstore)
    : hash_control(this) {
    /* Start serializers */
    n_files = dynamic_config->serializer_private.size();
    rassert(n_files > 0);
    rassert(n_files <= MAX_SERIALIZERS);
    uint32_t magics[n_files];
    for (int i = 0; i < n_files; i++) serializers[i] = NULL;
    pmap(n_files, boost::bind(&create_existing_serializer,
        dynamic_config, &btree_static_config, serializers, magics, _1));
    for (int i = 1; i < n_files; i++) {
        if (magics[i] != magics[0]) {
            fail_due_to_user_error("The files that the server was started with didn't all come from the same database.");
        }
    }

    /* Create pseudoserializers */
    pmap(btree_static_config.n_slices, boost::bind(&create_pseudoserializer,
         dynamic_config, &btree_static_config, serializers, pseudoserializers, _1));

    /* Load btrees */
    mirrored_cache_config_t per_slice_config = dynamic_config->cache;
    /* Divvy up the memory available between the several slices. */
    per_slice_config.max_size /= btree_static_config.n_slices;
    per_slice_config.max_dirty_size /= btree_static_config.n_slices;
    per_slice_config.flush_dirty_size /= btree_static_config.n_slices;
    pmap(btree_static_config.n_slices, boost::bind(&create_existing_btree,
         pseudoserializers, slices, &per_slice_config, masterstore, _1));
}

btree_key_value_store_t::~btree_key_value_store_t() {

    /* Shut down btrees */
    pmap(btree_static_config.n_slices, boost::bind(&destroy_btree, slices, _1));

    /* Destroy pseudoserializers */
    pmap(btree_static_config.n_slices, boost::bind(&destroy_pseudoserializer, pseudoserializers, _1));

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

int btree_key_value_store_t::compute_mod_count(int32_t file_number, int32_t n_files, int32_t n_slices) {
    return n_slices / n_files + (n_slices % n_files > file_number);
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

store_t::set_result_t btree_key_value_store_t::set(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, castime_t castime) {
    return slice_for_key(key)->set(key, data, flags, exptime, castime);
}

store_t::set_result_t btree_key_value_store_t::add(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, castime_t castime) {
    return slice_for_key(key)->add(key, data, flags, exptime, castime);
}

store_t::set_result_t btree_key_value_store_t::replace(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, castime_t castime) {
    return slice_for_key(key)->replace(key, data, flags, exptime, castime);
}

store_t::set_result_t btree_key_value_store_t::cas(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, cas_t unique, castime_t castime) {
    return slice_for_key(key)->cas(key, data, flags, exptime, unique, castime);
}

store_t::incr_decr_result_t btree_key_value_store_t::incr(store_key_t *key, unsigned long long amount, castime_t castime) {
    return slice_for_key(key)->incr(key, amount, castime);
}

store_t::incr_decr_result_t btree_key_value_store_t::decr(store_key_t *key, unsigned long long amount, castime_t castime) {
    return slice_for_key(key)->decr(key, amount, castime);
}

store_t::append_prepend_result_t btree_key_value_store_t::append(store_key_t *key, data_provider_t *data, castime_t castime) {
    return slice_for_key(key)->append(key, data, castime);
}

store_t::append_prepend_result_t btree_key_value_store_t::prepend(store_key_t *key, data_provider_t *data, castime_t castime) {
    return slice_for_key(key)->prepend(key, data, castime);
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

