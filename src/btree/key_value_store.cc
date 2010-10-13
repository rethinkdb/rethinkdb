#include "key_value_store.hpp"
#include "db_cpu_info.hpp"

btree_key_value_store_t::btree_key_value_store_t(cmd_config_t *cmd_config)
    : cmd_config(cmd_config), state(state_off)
{
    
    assert(cmd_config->n_slices > 0);
    for (int i = 0; i < cmd_config->n_slices; i++) {
        slices[i] = NULL;
    }
}

/* Process of starting individual slices on different cores */

bool btree_key_value_store_t::start(ready_callback_t *cb) {
    
    assert(state == state_off);
    state = state_starting_up;
    
    ready_callback = NULL;
    messages_out = cmd_config->n_slices;
    for (int id = 0; id < cmd_config->n_slices; id++) {
        do_on_cpu(id % get_num_db_cpus(), this, &btree_key_value_store_t::create_a_slice_on_this_core, id);
    }
    if (messages_out == 0) {
        return true;
    } else {
        ready_callback = cb;
        return false;
    }
}

bool btree_key_value_store_t::create_a_slice_on_this_core(int id) {
    
    char name[MAX_DB_FILE_NAME];
    int len = snprintf(name, MAX_DB_FILE_NAME, "%s_%d", cmd_config->db_file_name, id);
    // TODO: the below line is currently the only way to write to a block device,
    // we need a command line way to do it, this also requires consolidating to one
    // file
    //     int len = snprintf(name, MAX_DB_FILE_NAME, "/dev/sdb");
    check("Name too long", len == MAX_DB_FILE_NAME);
    
    slices[id] = new btree_slice_t(
        name,
        BTREE_BLOCK_SIZE,
        cmd_config->max_cache_size / cmd_config->n_slices,
        cmd_config->wait_for_flush,
        cmd_config->flush_timer_ms,
        cmd_config->flush_threshold_percent);
    
    if (slices[id]->start(this)) return a_slice_is_ready();
    else return false;
}

void btree_key_value_store_t::on_slice_ready() {

    a_slice_is_ready();
}

bool btree_key_value_store_t::a_slice_is_ready() {

    return do_on_cpu(home_cpu, this, &btree_key_value_store_t::have_created_a_slice);
}

bool btree_key_value_store_t::have_created_a_slice() {

    messages_out--;
    if (messages_out == 0) {
    
        assert(state == state_starting_up);
        state = state_ready;
        
        if (ready_callback) {
            ready_callback->on_store_ready();
        }
    }
    
    return true;
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
    return slices[hash(key) % cmd_config->n_slices];
}

/* Process of shutting down */

bool btree_key_value_store_t::shutdown(shutdown_callback_t *cb) {
    
    assert(state == state_ready);
    state = state_shutting_down;
    
    shutdown_callback = NULL;
    messages_out = cmd_config->n_slices;
    for (int id = 0; id < cmd_config->n_slices; id++) {
        do_on_cpu(slices[id]->home_cpu, this, &btree_key_value_store_t::shutdown_a_slice, id);
    }
    if (messages_out == 0) {
        return true;
    } else {
        shutdown_callback = cb;
        return false;
    }
}

bool btree_key_value_store_t::shutdown_a_slice(int id) {
    
    if (slices[id]->shutdown(this)) return a_slice_has_shutdown(slices[id]);
    else return false;
}

void btree_key_value_store_t::on_slice_shutdown(btree_slice_t *slice) {

    a_slice_has_shutdown(slice);
}

bool btree_key_value_store_t::a_slice_has_shutdown(btree_slice_t *slice) {
    
    delete slice;
    return do_on_cpu(home_cpu, this, &btree_key_value_store_t::have_shutdown_a_slice);
}

bool btree_key_value_store_t::have_shutdown_a_slice() {
    
    messages_out--;
    if (messages_out == 0) {
        
        assert(state == state_shutting_down);
        state = state_off;
        
        if (shutdown_callback) shutdown_callback->on_store_shutdown();
    }
    
    return true;
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
            const btree_superblock_t *sb = (const btree_superblock_t*)(sb_buf->get_data_read());
            // The serializer will init the superblock to zeroes if the database is newly created.
            if (!sb->database_exists) {
                btree_superblock_t *sb = (btree_superblock_t*)(sb_buf->get_data_write());
                sb->database_exists = 1;
                sb->root_block = NULL_BLOCK_ID;
            }
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
    char *filename,
    size_t block_size,
    size_t max_size,
    bool wait_for_flush,
    unsigned int flush_timer_ms,
    unsigned int flush_threshold_percent)
    : cas_counter(0),
      state(state_unstarted),
      cache(filename, block_size, max_size, wait_for_flush, flush_timer_ms, flush_threshold_percent),
      total_set_operations(0), pm_total_set_operations("cmd_set", &total_set_operations, &perfmon_combiner_sum)
    { }

btree_slice_t::~btree_slice_t() {
    assert(state == state_unstarted || state == state_shut_down);
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
        if (cache.start(this)) {
            state = state_starting_up_initialize_superblock;
        } else {
            state = state_starting_up_waiting_for_cache;
            return false;
        }
    }
    
    if (state == state_starting_up_initialize_superblock) {
        sb_fsm = new initialize_superblock_fsm_t(&cache);
        if (sb_fsm->initialize_superblock_if_necessary(this)) {
            state = state_starting_up_finish;
        } else {
            state = state_starting_up_waiting_for_superblock;
            return false;
        }
    }
    
    if (state == state_starting_up_finish) {
        
        delete sb_fsm;
        
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
    // A CAS value is made up of both a timestamp and a per-worker counter,
    // which should be enough to guarantee that it'll be unique.
    return (time(NULL) << 32) | (++cas_counter);
}

bool btree_slice_t::shutdown(shutdown_callback_t *cb) {
    assert(state == state_ready);
    if (cache.shutdown(this)) {
        state = state_shut_down;
        return true;
    } else {
        state = state_shutting_down;
        shutdown_callback = cb;
        return false;
    }
}

void btree_slice_t::on_cache_shutdown() {
    state = state_shut_down;
    // It's not safe to call the cache's destructor from within on_cache_shutdown()
    call_later_on_this_cpu(this);
}

void btree_slice_t::on_cpu_switch() {
    if (shutdown_callback) shutdown_callback->on_slice_shutdown(this);
}
