#include "key_value_store.hpp"
#include "db_cpu_info.hpp"
#include "btree/get_fsm.hpp"
#include "btree/set_fsm.hpp"
#include "btree/incr_decr_fsm.hpp"
#include "btree/append_prepend_fsm.hpp"
#include "btree/delete_fsm.hpp"
#include "btree/get_cas_fsm.hpp"
#include "btree/replicate.hpp"

/* The key-value store slices up the serializers as follows:

- Each slice is assigned to a serializer. The formula is (slice_id % n_files)
- Block ID 0 on each serializer is for static btree configuration information.
- Each slice uses block IDs of the form (1 + (n * (number of slices on serializer) +
    (slice_id / n_files))).

*/

btree_key_value_store_t::btree_key_value_store_t(
        btree_key_value_store_dynamic_config_t *dynamic_config)
    : dynamic_config(dynamic_config),
      n_files(dynamic_config->serializer_private.size()),
      state(state_off)
{
    assert(n_files > 0);
    assert(n_files <= MAX_SERIALIZERS);
    for (int i = 0; i < n_files; i++) {
        serializers[i] = NULL;
    }
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

const block_magic_t serializer_config_block_t::expected_magic = { { 'c','f','g','_' } };

/* Process of creating a new key-value store */

bool btree_key_value_store_t::start_new(ready_callback_t *cb, btree_key_value_store_static_config_t *sc) {
    
    is_start_existing = false;
    btree_static_config = sc->btree;
    serializer_static_config = &sc->serializer;
    return start(cb);
}

bool btree_key_value_store_t::start_existing(ready_callback_t *cb) {
    
    is_start_existing = true;
    return start(cb);
}

bool btree_key_value_store_t::start(ready_callback_t *cb) {
    
    assert(state == state_off);
    state = state_starting_up;
    
    ready_callback = NULL;
    create_serializers();
    if (state == state_ready) {
        return true;
    } else {
        ready_callback = cb;
        return false;
    }
}

struct bkvs_start_new_serializer_fsm_t :
    public standard_serializer_t::ready_callback_t,
    public serializer_t::write_txn_callback_t
{
    btree_key_value_store_t *store;
    int i;
    
    void run() {
        do_on_cpu(i % get_num_db_cpus(), this, &bkvs_start_new_serializer_fsm_t::create_serializer);
    }
    
    void *config_block;
    
    bool create_serializer() {
        
        store->serializers[i] = new standard_serializer_t(&store->dynamic_config->serializer, &store->dynamic_config->serializer_private[i]);
        
        if (store->serializers[i]->start_new(store->serializer_static_config, this))
            on_serializer_ready(NULL);
        
        return true;
    }
    
    void on_serializer_ready(standard_serializer_t *ser) {
        
        config_block = store->serializers[i]->malloc();
        bzero(config_block, store->serializers[i]->get_block_size().value());
        serializer_config_block_t *c = (serializer_config_block_t *)config_block;
        c->magic = serializer_config_block_t::expected_magic;
        c->database_magic = store->creation_magic;
        c->n_files = store->n_files;
        c->this_serializer = i;
        c->btree_config = store->btree_static_config;

        serializer_t::write_t w(CONFIG_BLOCK_ID.ser_id, repl_timestamp::placeholder, config_block, NULL);
        if (store->serializers[i]->do_write(&w, 1, this)) on_serializer_write_txn();
    }
    
    void on_serializer_write_txn() {
        
        store->serializers[i]->free(config_block);
        do_on_cpu(store->home_cpu, this, &bkvs_start_new_serializer_fsm_t::done);
    }
    
    bool done() {
    
        store->have_created_a_serializer();
        delete this;
        return true;
    }
};

struct bkvs_start_existing_serializer_fsm_t :
    public standard_serializer_t::ready_callback_t,
    public serializer_t::read_callback_t
{
    btree_key_value_store_t *store;
    standard_serializer_t *serializer;
    int i;
    
    void run() {
        do_on_cpu(i % get_num_db_cpus(), this, &bkvs_start_existing_serializer_fsm_t::create_serializer);
    }
    
    void *config_block;
    
    bool create_serializer() {
        
        serializer = new standard_serializer_t(&store->dynamic_config->serializer, &store->dynamic_config->serializer_private[i]);
        
        if (serializer->start_existing(this)) on_serializer_ready(NULL);
        
        return true;
    }
    
    void on_serializer_ready(standard_serializer_t *ser) {
        
        config_block = serializer->malloc();
        if (serializer->do_read(CONFIG_BLOCK_ID.ser_id, config_block, this)) on_serializer_read();
    }
    
    void on_serializer_read() {
        
        serializer_config_block_t *c = (serializer_config_block_t *)config_block;
        if (c->n_files != store->n_files) {
            fail("File config block for file \"%s\" says there should be %d files, but we have %d.",
                store->dynamic_config->serializer_private[i].db_filename.c_str(), (int)c->n_files, (int)store->n_files);
        }

        assert(check_magic<serializer_config_block_t>(c->magic));
        assert(c->this_serializer >= 0 && c->this_serializer < store->n_files);

        store->serializer_magics[c->this_serializer] = c->database_magic;
        store->btree_static_config = c->btree_config;

        assert(!store->serializers[c->this_serializer]);

        store->serializers[c->this_serializer] = serializer;
        serializer->free(config_block);
        
        do_on_cpu(store->home_cpu, this, &bkvs_start_existing_serializer_fsm_t::done);
    }
    
    bool done() {
    
        store->have_created_a_serializer();
        delete this;
        return true;
    }
};

void btree_key_value_store_t::create_serializers() {
    
    messages_out = n_files;
    if (is_start_existing) {
        for (int i = 0; i < n_files; i++) {
            bkvs_start_existing_serializer_fsm_t *f = new bkvs_start_existing_serializer_fsm_t();
            f->store = this;
            f->i = i;
            f->run();
        }
    } else {
        creation_magic = time(NULL);
        for (int i = 0; i < n_files; i++) {
            bkvs_start_new_serializer_fsm_t *f = new bkvs_start_new_serializer_fsm_t();
            f->store = this;
            f->i = i;
            f->run();
        }
    }
}

bool btree_key_value_store_t::have_created_a_serializer() {
    
    messages_out--;
    if (messages_out == 0) {
    
        if (is_start_existing) {
            /* Make sure all the magics line up */
            for (int i = 1; i < n_files; i++) {
                if (serializer_magics[i] != serializer_magics[0]) {
                    fail("The files that the server was started with didn't all come from "
                         "the same database.");
                }
            }
        }
    
        create_pseudoserializers();
    }
    
    return true;
}

void btree_key_value_store_t::create_pseudoserializers() {
    
    messages_out = btree_static_config.n_slices;
    for (int i = 0; i < btree_static_config.n_slices; i++) {
        do_on_cpu(serializers[i % n_files]->home_cpu, this,
            &btree_key_value_store_t::create_a_pseudoserializer_on_this_core, i);
    }
}

int btree_key_value_store_t::compute_mod_count(int32_t file_number, int32_t n_files, int32_t n_slices) {
    return n_slices / n_files + (n_slices % n_files > file_number);
}

bool btree_key_value_store_t::create_a_pseudoserializer_on_this_core(int i) {
    
    /* How many other pseudoserializers are we sharing the serializer with? */
    int mod_count = compute_mod_count(i % n_files, n_files, btree_static_config.n_slices);
    
    pseudoserializers[i] = new translator_serializer_t(
        serializers[i % n_files],
        mod_count,
        i / n_files,
        CONFIG_BLOCK_ID   /* Reserve block ID 0 */
        );
    
    do_on_cpu(home_cpu, this, &btree_key_value_store_t::have_created_a_pseudoserializer);

    return true;
}

bool btree_key_value_store_t::have_created_a_pseudoserializer() {
    
    messages_out--;
    if (messages_out == 0) {
        create_slices();
    }
    
    return true;
}

void btree_key_value_store_t::create_slices() {
    
    /* Divvy up the memory available between the several slices. */
    cache_config = dynamic_config->cache;
    cache_config.max_size /= btree_static_config.n_slices;
    
    messages_out = btree_static_config.n_slices;
    for (int id = 0; id < btree_static_config.n_slices; id++) {
        do_on_cpu(id % get_num_db_cpus(), this, &btree_key_value_store_t::create_a_slice_on_this_core, id);
    }
}

bool btree_key_value_store_t::create_a_slice_on_this_core(int id) {
    
    // TODO try to align slices with serializers so that when possible, a slice is on the
    // same thread as its serializer
    
    slices[id] = new btree_slice_t(
        pseudoserializers[id],
        &cache_config);
    
    bool done;
    if (is_start_existing) {
        done = slices[id]->start_existing(this);
    } else {
        done = slices[id]->start_new(this);
    }
    if (done) on_slice_ready();
    
    return true;
}

void btree_key_value_store_t::on_slice_ready() {

    do_on_cpu(home_cpu, this, &btree_key_value_store_t::have_created_a_slice);
}

bool btree_key_value_store_t::have_created_a_slice() {

    assert_cpu();
    messages_out--;
    if (messages_out == 0) {
        finish_start();
    }
    
    return true;
}

void btree_key_value_store_t::finish_start() {
    
    assert_cpu();
    assert(state == state_starting_up);
    state = state_ready;
    
    if (ready_callback) {
        ready_callback->on_store_ready();
    }
}

/* Hashing keys and choosing a slice for each key */

#define get16bits(d) ((((uint32_t)(((const uint8_t *)(d))[1])) << 8)\
                       +(uint32_t)(((const uint8_t *)(d))[0]) )

uint32_t btree_key_value_store_t::hash(btree_key *key) {
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

btree_slice_t *btree_key_value_store_t::slice_for_key(btree_key *key) {
    return slices[hash(key) % btree_static_config.n_slices];
}

/* store_t interface */

void btree_key_value_store_t::get(store_key_t *key, get_callback_t *cb) {
    new btree_get_fsm_t(key, this, cb);
}

void btree_key_value_store_t::get_cas(store_key_t *key, get_callback_t *cb) {
    new btree_get_cas_fsm_t(key, this, cb);
}

void btree_key_value_store_t::set(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, set_callback_t *cb) {
    new btree_set_fsm_t(key, this, data, btree_set_fsm_t::set_type_set, flags, exptime, 0, cb);
}

void btree_key_value_store_t::add(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, set_callback_t *cb) {
    new btree_set_fsm_t(key, this, data, btree_set_fsm_t::set_type_add, flags, exptime, 0, cb);
}

void btree_key_value_store_t::replace(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, set_callback_t *cb) {
    new btree_set_fsm_t(key, this, data, btree_set_fsm_t::set_type_replace, flags, exptime, 0, cb);
}

void btree_key_value_store_t::cas(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, cas_t unique, set_callback_t *cb) {
    new btree_set_fsm_t(key, this, data, btree_set_fsm_t::set_type_cas, flags, exptime, unique, cb);
}

void btree_key_value_store_t::incr(store_key_t *key, unsigned long long amount, incr_decr_callback_t *cb) {
    new btree_incr_decr_fsm_t(key, this, true, amount, cb);
}

void btree_key_value_store_t::decr(store_key_t *key, unsigned long long amount, incr_decr_callback_t *cb) {
    new btree_incr_decr_fsm_t(key, this, false, amount, cb);
}

void btree_key_value_store_t::append(store_key_t *key, data_provider_t *data, append_prepend_callback_t *cb) {
    new btree_append_prepend_fsm_t(key, this, data, true, cb);
}

void btree_key_value_store_t::prepend(store_key_t *key, data_provider_t *data, append_prepend_callback_t *cb) {
    new btree_append_prepend_fsm_t(key, this, data, false, cb);
}

void btree_key_value_store_t::delete_key(store_key_t *key, delete_callback_t *cb) {
    new btree_delete_fsm_t(key, this, cb);
}

void btree_key_value_store_t::replicate(replicant_t *cb) {
    replicants.push_back(new btree_replicant_t(cb, this));
}

void btree_key_value_store_t::stop_replicating(replicant_t *cb) {
    std::vector<btree_replicant_t *>::iterator it;
    for (it = replicants.begin(); it != replicants.end(); it++) {
        if ((*it)->callback == cb) {
            (*it)->stop();
            replicants.erase(it);
            return;
        }
    }
    fail("stop_replicating() called on a replicant that isn't replicating.");
}

/* Process of shutting down */

bool btree_key_value_store_t::shutdown(shutdown_callback_t *cb) {
    
    assert(state == state_ready);
    state = state_shutting_down;
    
    shutdown_callback = NULL;
    shutdown_slices();
    if (state == state_off) {
        return true;
    } else {
        shutdown_callback = cb;
        return false;
    }
}

void btree_key_value_store_t::shutdown_slices() {
    
    messages_out = btree_static_config.n_slices;
    for (int id = 0; id < btree_static_config.n_slices; id++) {
        do_on_cpu(slices[id]->home_cpu, this, &btree_key_value_store_t::shutdown_a_slice, id);
    }
}

bool btree_key_value_store_t::shutdown_a_slice(int id) {
    
    if (slices[id]->shutdown(this)) on_slice_shutdown(slices[id]);
    return true;
}

void btree_key_value_store_t::on_slice_shutdown(btree_slice_t *slice) {
    
    delete slice;
    do_on_cpu(home_cpu, this, &btree_key_value_store_t::have_shutdown_a_slice);
}

bool btree_key_value_store_t::have_shutdown_a_slice() {
    
    messages_out--;
    if (messages_out == 0) {
        delete_pseudoserializers();
    }
    
    return true;
}

void btree_key_value_store_t::delete_pseudoserializers() {
    
    for (int id = 0; id < btree_static_config.n_slices; id++) {
        do_on_cpu(pseudoserializers[id]->home_cpu, this, &btree_key_value_store_t::delete_a_pseudoserializer, id);
    }
    
    /* Don't bother waiting for pseudoserializers to get completely deleted */
    
    shutdown_serializers();
}

bool btree_key_value_store_t::delete_a_pseudoserializer(int id) {

    delete pseudoserializers[id];
    
    return true;
}

void btree_key_value_store_t::shutdown_serializers() {
    
    messages_out = n_files;
    for (int id = 0; id < n_files; id++) {
        do_on_cpu(serializers[id]->home_cpu, this, &btree_key_value_store_t::shutdown_a_serializer, id);
    }
}

bool btree_key_value_store_t::shutdown_a_serializer(int id) {
    
    if (serializers[id]->shutdown(this)) on_serializer_shutdown(serializers[id]);
    return true;
}

void btree_key_value_store_t::on_serializer_shutdown(standard_serializer_t *serializer) {
    
    delete serializer;
    do_on_cpu(home_cpu, this, &btree_key_value_store_t::have_shutdown_a_serializer);
}

bool btree_key_value_store_t::have_shutdown_a_serializer() {
    
    messages_out--;
    if (messages_out == 0) {
        finish_shutdown();
    }
    
    return true;
}

void btree_key_value_store_t::finish_shutdown() {
    
    assert(state == state_shutting_down);
    state = state_off;
    
    if (shutdown_callback) shutdown_callback->on_store_shutdown();
}

btree_key_value_store_t::~btree_key_value_store_t() {
    
    assert(state == state_off);
}

// Stats

perfmon_duration_sampler_t
    pm_cmd_set("cmd_set", secs_to_ticks(1)),
    pm_cmd_get("cmd_get", secs_to_ticks(1));
