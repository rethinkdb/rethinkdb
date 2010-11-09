#include "serializer/log/log_serializer.hpp"

#undef __UTILS_HPP__   /* Hack because both RethinkDB and stress-client have a utils.hpp */
#include "../stress-client/utils.hpp"

struct transaction_t :
    public serializer_t::write_txn_callback_t
{
    serializer_t *ser;
    std::vector<serializer_t::write_t, gnew_alloc<serializer_t::write_t> > writes;
    
    struct callback_t {
        virtual void on_transaction_complete() = 0;
    } *cb;
    
    transaction_t(serializer_t *ser, unsigned inserts, unsigned updates, callback_t *cb)
        : ser(ser), cb(cb)
    {
        /* If there aren't enough blocks to update, then convert the updates into inserts */
        
        if (updates > ser->max_block_id()) {
            inserts += updates;
            updates = 0;
        }
        
        /* As a simple way to avoid updating the same block twice in one transaction, select
        a contiguous range of blocks starting at a random offset within range */
        
        ser_block_id_t begin = random(0, ser->max_block_id() - updates);
        
        for (unsigned i = 0; i < updates; i++) {
            serializer_t::write_t w;
            w.block_id = begin + i;
            w.buf = ser->malloc();
            memset(w.buf, 0xDD, ser->get_block_size());
            w.callback = NULL;
            writes.push_back(w);
        }
        
        /* Generate new IDs to insert by simply taking (highest ID + 1) */
        
        for (unsigned i = 0; i < inserts; i++) {
            serializer_t::write_t w;
            w.block_id = ser->max_block_id() + i;
            w.buf = ser->malloc();
            memset(w.buf, 0xDD, ser->get_block_size());
            w.callback = NULL;
            writes.push_back(w);
        }
        
        bool done = ser->do_write(&writes[0], writes.size(), this);
        if (done) on_serializer_write_txn();   // Probably never happens
    }
    
    void on_serializer_write_txn() {
        
        for (unsigned i = 0; i < writes.size(); i++) {
            ser->free(writes[i].buf);
        }
        
        cb->on_transaction_complete();
        gdelete(this);
    }
};

#define RUN_FOREVER (-1)

struct config_t {
    
    const char *filename;
    log_serializer_static_config_t ser_static_config;
    log_serializer_dynamic_config_t ser_dynamic_config;
    int duration;
    unsigned concurrent_txns;
    unsigned inserts_per_txn, updates_per_txn;
};

struct tester_t :
    public cpu_message_t,
    public log_serializer_t::ready_callback_t,
    public transaction_t::callback_t,
    public log_serializer_t::shutdown_callback_t
{
    log_serializer_t *ser;
    unsigned active_txns, total_txns;
    config_t *config;
    thread_pool_t *pool;
    time_t start_ticks;
    
    tester_t(config_t *config, thread_pool_t *pool)
        : active_txns(0), total_txns(0), config(config), pool(pool) { }
    
    void on_cpu_switch() {
        fprintf(stderr, "Starting serializer...\n");
        ser = gnew<log_serializer_t>(config->filename, &config->ser_dynamic_config);
        if (ser->start_new(&config->ser_static_config, this)) on_serializer_ready(ser);
    }
    
    void on_serializer_ready(log_serializer_t *ls) {
        fprintf(stderr, "Running test...\n");
        start_ticks = get_ticks();
        pump();
    }
    
    void pump() {
        
        while (active_txns < config->concurrent_txns && ((signed)total_txns < config->duration || config->duration == RUN_FOREVER)) {
            active_txns++;
            total_txns++;
            gnew<transaction_t>(ser, config->inserts_per_txn, config->updates_per_txn, this);
        }
        
        if (active_txns == 0 && (signed)total_txns == config->duration) {
            shutdown();
        }
    }
    
    void on_transaction_complete() {
        active_txns--;
        pump();
    }
    
    void shutdown() {
        ticks_t end_ticks = get_ticks();
        fprintf(stderr, "Test over.\n");
        fprintf(stderr, "The test took %.3f seconds.\n", ticks_to_secs(end_ticks - start_ticks));
        if (ser->shutdown(this)) on_serializer_shutdown(ser);
    }
    
    void on_serializer_shutdown(log_serializer_t *ser) {
        fprintf(stderr, "Serializer shut down.\n");
        gdelete(ser);
        pool->shutdown();
    }
};

const char *read_arg(int &argc, char **&argv) {
    if (argc == 0) {
        fail("Expected another argument at the end.");
    } else {
        argc--;
        return (argv++)[0];
    }
} 

void parse_config(int argc, char *argv[], config_t *config) {
    
    config->filename = "rethinkdb_data";
    
    config->ser_static_config.block_size = DEFAULT_BTREE_BLOCK_SIZE;
    config->ser_static_config.extent_size = DEFAULT_EXTENT_SIZE;
    config->ser_dynamic_config.gc_low_ratio = DEFAULT_GC_LOW_RATIO;
    config->ser_dynamic_config.gc_high_ratio = DEFAULT_GC_HIGH_RATIO;
    config->ser_dynamic_config.num_active_data_extents = DEFAULT_ACTIVE_DATA_EXTENTS;
    config->ser_dynamic_config.file_size = 0;
    config->ser_dynamic_config.file_zone_size = GIGABYTE;
    
    config->duration = 1000;
    config->concurrent_txns = 8;
    config->inserts_per_txn = 10;
    config->updates_per_txn = 10;
    
    read_arg(argc, argv);
    
    while (argc) {
        const char *flag = read_arg(argc, argv);
        
        if (strcmp(flag, "-f") == 0) {
            config->filename = read_arg(argc, argv);
        
        } else if (strcmp(flag, "--block-size") == 0) {
            config->ser_static_config.block_size = atoi(read_arg(argc, argv));
        } else if (strcmp(flag, "--extent-size") == 0) {
            config->ser_static_config.extent_size = atoi(read_arg(argc, argv));
        } else if (strcmp(flag, "--active-data-extents") == 0) {
            config->ser_dynamic_config.num_active_data_extents = atoi(read_arg(argc, argv));
        } else if (strcmp(flag, "--file-zone-size") == 0) {
            config->ser_dynamic_config.file_zone_size = atoi(read_arg(argc, argv));
            
        } else if (strcmp(flag, "--duration") == 0) {
            config->duration = atoi(read_arg(argc, argv));
        } else if (strcmp(flag, "--forever") == 0) {
            config->duration = RUN_FOREVER;
        } else if (strcmp(flag, "--concurrent") == 0) {
            config->concurrent_txns = atoi(read_arg(argc, argv));
        } else if (strcmp(flag, "--inserts-per-txn") == 0) {
            config->inserts_per_txn = atoi(read_arg(argc, argv));
        } else if (strcmp(flag, "--updates-per-txn") == 0) {
            config->updates_per_txn = atoi(read_arg(argc, argv));
        
        } else {
            fail("Don't know how to handle \"%s\"", flag);
        }
    }

    assert(config->ser_static_config.block_size > 0);
    assert(config->ser_static_config.extent_size > 0);
    assert(config->ser_dynamic_config.num_active_data_extents > 0);
    assert(config->ser_dynamic_config.file_zone_size > 0);
    assert(config->duration > 0 || config->duration == RUN_FOREVER);
    assert(config->concurrent_txns > 0);
}

int main(int argc, char *argv[]) {
    
    config_t config;
    parse_config(argc, argv, &config);
    
    thread_pool_t thread_pool(1);
    tester_t tester(&config, &thread_pool);
    thread_pool.run(&tester);
    
    return 0;
}
