#include "key_value_store.hpp"
#include "db_cpu_info.hpp"

/* The key-value store slices up the serializers as follows:

- Each slice is assigned to a serializer. The formula is (slice_id % n_files)
- Block ID 0 on each serializer is for static btree configuration information.
- Each slice uses block IDs of the form (1 + (n * (number of serializers on slice) +
    (slice_id / n_files))).

*/

btree_key_value_store_t::btree_key_value_store_t(
        btree_key_value_store_dynamic_config_t *dynamic_config,
        int n_files,
        const char **db_filenames)
    : dynamic_config(dynamic_config),
      n_files(n_files),
      db_filenames(db_filenames),
      state(state_off)
{
    assert(n_files > 0);
    for (int i = 0; i < n_files; i++) {
        serializers[i] = NULL;
    }
}

/* This is the format that block ID 0 on each serializer takes. */

#define CONFIG_BLOCK_ID (ser_block_id_t(0))

static const char serializer_config_block_magic[] = {'b', 't', 'r', 'e', 'e', 'c', 'f', 'g'};

struct serializer_config_block_t {
    
    block_magic_t magic;
    
    /* What time the database was created. To help catch the case where files from two
    databases are mixed. */
    uint32_t database_magic;
    
    /* How many serializers the database is using (in case user creates the database with
    some number of serializers and then specifies less than that many on a subsequent
    run) */
    int n_files;
    
    /* Which serializer this is, in case user specifies serializers in a different order from
    run to run */
    int this_serializer;
    
    /* Static btree configuration information, like number of slices. Should be the same on
    each serializer. */
    btree_config_t btree_config;

    static const block_magic_t expected_magic;
};

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
        
        store->serializers[i] = gnew<standard_serializer_t>(store->db_filenames[i], &store->dynamic_config->serializer);
        
        if (store->serializers[i]->start_new(store->serializer_static_config, this))
            on_serializer_ready(NULL);
        
        return true;
    }
    
    void on_serializer_ready(standard_serializer_t *ser) {
        
        config_block = store->serializers[i]->malloc();
        bzero(config_block, store->serializers[i]->get_block_size());
        serializer_config_block_t *c = (serializer_config_block_t *)config_block;
        c->magic = serializer_config_block_t::expected_magic;
        c->database_magic = store->creation_magic;
        c->n_files = store->n_files;
        c->this_serializer = i;
        c->btree_config = store->btree_static_config;
        
        serializer_t::write_t w;
        w.buf = config_block;
        w.block_id = CONFIG_BLOCK_ID;
        w.callback = NULL;
        if (store->serializers[i]->do_write(&w, 1, this)) on_serializer_write_txn();
    }
    
    void on_serializer_write_txn() {
        
        store->serializers[i]->free(config_block);
        do_on_cpu(store->home_cpu, this, &bkvs_start_new_serializer_fsm_t::done);
    }
    
    bool done() {
    
        store->have_created_a_serializer();
        gdelete(this);
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
        
        serializer = gnew<standard_serializer_t>(store->db_filenames[i], &store->dynamic_config->serializer);
        
        if (serializer->start_existing(this)) on_serializer_ready(NULL);
        
        return true;
    }
    
    void on_serializer_ready(standard_serializer_t *ser) {
        
        config_block = serializer->malloc();
        if (serializer->do_read(CONFIG_BLOCK_ID, config_block, this)) on_serializer_read();
    }
    
    void on_serializer_read() {
        
        serializer_config_block_t *c = (serializer_config_block_t *)config_block;
        if (c->n_files != store->n_files) {
            fail("File config block for file \"%s\" says there should be %d files, but we have %d.",
                store->db_filenames[i], (int)c->n_files, (int)store->n_files);
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
        gdelete(this);
        return true;
    }
};

void btree_key_value_store_t::create_serializers() {
    
    messages_out = n_files;
    if (is_start_existing) {
        for (int i = 0; i < n_files; i++) {
            bkvs_start_existing_serializer_fsm_t *f = gnew<bkvs_start_existing_serializer_fsm_t>();
            f->store = this;
            f->i = i;
            f->run();
        }
    } else {
        creation_magic = time(NULL);
        for (int i = 0; i < n_files; i++) {
            bkvs_start_new_serializer_fsm_t *f = gnew<bkvs_start_new_serializer_fsm_t>();
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

bool btree_key_value_store_t::create_a_pseudoserializer_on_this_core(int i) {
    
    /* How many other pseudoserializers are we sharing the serializer with? */
    int mod_count = btree_static_config.n_slices / n_files +
        (btree_static_config.n_slices % n_files > i % n_files);
    
    pseudoserializers[i] = gnew<translator_serializer_t>(
        serializers[i % n_files],
        mod_count,
        i / n_files,
        CONFIG_BLOCK_ID + 1   /* Reserve block ID 0 */
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

uint32_t hash(btree_key *key) {
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

    gdelete<translator_serializer_t>(pseudoserializers[id]);
    
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
    
    gdelete(serializer);
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


/* When the serializer starts up, it will create an initial superblock and initialize it to zero.
This isn't quite the behavior we want. The job of initialize_superblock_fsm is to initialize the
superblock to contain NULL_BLOCK_ID rather than zero as the root node. */

class initialize_superblock_fsm_t :
    private block_available_callback_t,
    private transaction_begin_callback_t,
    private transaction_commit_callback_t,
    public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, initialize_superblock_fsm_t>{

public:
    initialize_superblock_fsm_t(cache_t *cache)
        : state(state_unstarted), cache(cache), sb_buf(NULL), txn(NULL)
        {}
    ~initialize_superblock_fsm_t() {
        assert(state == state_unstarted || state == state_done);
    }
    
    bool initialize_superblock_if_necessary(btree_slice_t *cb) {
        assert(state == state_unstarted);
        state = state_begin_transaction;
        callback = NULL;
        if (next_initialize_superblock_step()) {
            return true;
        } else {
            callback = cb;
            return false;
        }
    }

private:
    enum state_t {
        state_unstarted,
        state_begin_transaction,
        state_beginning_transaction,
        state_acquire_superblock,
        state_acquiring_superblock,
        state_make_change,
        state_commit_transaction,
        state_committing_transaction,
        state_finish,
        state_done
    } state;
    
    cache_t *cache;
    buf_t *sb_buf;
    transaction_t *txn;
    btree_slice_t *callback;
    
    bool next_initialize_superblock_step() {
        
        if (state == state_begin_transaction) {
            txn = cache->begin_transaction(rwi_write, this);
            if (txn) {
                state = state_acquire_superblock;
            } else {
                state = state_beginning_transaction;
                return false;
            }
        }
        
        if (state == state_acquire_superblock) {
            sb_buf = txn->acquire(SUPERBLOCK_ID, rwi_write, this);
            if (sb_buf) {
                state = state_make_change;
            } else {
                state = state_acquiring_superblock;
                return false;
            }
        }
        
        if (state == state_make_change) {
            btree_superblock_t *sb = (btree_superblock_t*)(sb_buf->get_data_write());
            sb->magic = btree_superblock_t::expected_magic;
            sb->root_block = NULL_BLOCK_ID;
            sb_buf->release();
            state = state_commit_transaction;
        }
        
        if (state == state_commit_transaction) {
            if (txn->commit(this)) {
                state = state_finish;
            } else {
                state = state_committing_transaction;
                return false;
            }
        }
        
        if (state == state_finish) {
            state = state_done;
            if (callback) callback->on_initialize_superblock();
            return true;
        }
        
        fail("Unexpected state");
    }
    
    void on_txn_begin(transaction_t *t) {
        assert(state == state_beginning_transaction);
        txn = t;
        state = state_acquire_superblock;
        next_initialize_superblock_step();
    }
    
    void on_txn_commit(transaction_t *t) {
        assert(state == state_committing_transaction);
        state = state_finish;
        next_initialize_superblock_step();
    }
    
    void on_block_available(buf_t *buf) {
        assert(state == state_acquiring_superblock);
        sb_buf = buf;
        state = state_make_change;
        next_initialize_superblock_step();
    }
};

btree_slice_t::btree_slice_t(
    serializer_t *serializer,
    mirrored_cache_config_t *config)
    : cas_counter(0),
      state(state_unstarted),
      cache(serializer, config)
    { }

btree_slice_t::~btree_slice_t() {
    assert(state == state_unstarted || state == state_shut_down);
}

bool btree_slice_t::start_new(ready_callback_t *cb) {
    is_start_existing = false;
    return start(cb);
}

bool btree_slice_t::start_existing(ready_callback_t *cb) {
    is_start_existing = true;
    return start(cb);
}

bool btree_slice_t::start(ready_callback_t *cb) {
    assert(state == state_unstarted);
    state = state_starting_up_start_cache;
    ready_callback = NULL;
    if (next_starting_up_step()) {
        return true;
    } else {
        ready_callback = cb;
        return false;
    }
}

bool btree_slice_t::next_starting_up_step() {
    
    if (state == state_starting_up_start_cache) {
        
        /* For now, the cache's startup process is the same whether it is starting a new
        database or an existing one. This could change in the future, though. */
        
        if (cache.start(this)) {
            state = state_starting_up_initialize_superblock;
        } else {
            state = state_starting_up_waiting_for_cache;
            return false;
        }
    }
    
    if (state == state_starting_up_initialize_superblock) {
    
        if (is_start_existing) {
            state = state_starting_up_finish;
    
        } else {
            sb_fsm = new initialize_superblock_fsm_t(&cache);
            if (sb_fsm->initialize_superblock_if_necessary(this)) {
                state = state_starting_up_finish;
            } else {
                state = state_starting_up_waiting_for_superblock;
                return false;
            }
        }
    }
    
    if (state == state_starting_up_finish) {
        
        if (!is_start_existing) delete sb_fsm;
        
        state = state_ready;
        if (ready_callback) ready_callback->on_slice_ready();
        
        return true;
    }
    
    fail("Unexpected state");
}

void btree_slice_t::on_cache_ready() {
    assert(state == state_starting_up_waiting_for_cache);
    state = state_starting_up_initialize_superblock;
    next_starting_up_step();
}

void btree_slice_t::on_initialize_superblock() {
    assert(state == state_starting_up_waiting_for_superblock);
    state = state_starting_up_finish;
    next_starting_up_step();
}

btree_value::cas_t btree_slice_t::gen_cas() {
    // A CAS value is made up of both a timestamp and a per-slice counter,
    // which should be enough to guarantee that it'll be unique.
    return (time(NULL) << 32) | (++cas_counter);
}

bool btree_slice_t::shutdown(shutdown_callback_t *cb) {
    assert(state == state_ready);
    state = state_shutting_down_shutdown_cache;
    shutdown_callback = NULL;
    if (next_shutting_down_step()) {
        return true;
    } else {
        shutdown_callback = cb;
        return false;
    }
}

bool btree_slice_t::next_shutting_down_step() {
    
    if (state == state_shutting_down_shutdown_cache) {
        if (cache.shutdown(this)) {
            state = state_shutting_down_finish;
        } else {
            state = state_shutting_down_waiting_for_cache;
            return false;
        }
    }
    
    if (state == state_shutting_down_finish) {
        state = state_shut_down;
        if (shutdown_callback) shutdown_callback->on_slice_shutdown(this);
        return true;
    }
    
    fail("Invalid state.");
}

void btree_slice_t::on_cache_shutdown() {
    assert(state == state_shutting_down_waiting_for_cache);
    state = state_shutting_down_finish;
    next_shutting_down_step();
}

// Stats

perfmon_counter_t
    pm_cmd_set("cmd_set"),
    pm_cmd_get("cmd_get");
