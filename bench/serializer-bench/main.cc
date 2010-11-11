#include "serializer/log/log_serializer.hpp"

#undef __UTILS_HPP__   /* Hack because both RethinkDB and stress-client have a utils.hpp */
#include "../stress-client/utils.hpp"

struct txn_info_t {
    ticks_t start, end;
};

typedef std::deque<txn_info_t, gnew_alloc<txn_info_t> > log_t;

struct transaction_t :
    public serializer_t::write_txn_callback_t
{
    serializer_t *ser;
    ticks_t start_time;
    log_t *log;
    void *dummy_buf;
    std::vector<serializer_t::write_t, gnew_alloc<serializer_t::write_t> > writes;
    
    struct callback_t {
        virtual void on_transaction_complete() = 0;
    } *cb;
    
    transaction_t(serializer_t *ser, log_t *log, unsigned inserts, unsigned updates, callback_t *cb)
        : ser(ser), log(log), cb(cb)
    {
        /* If there aren't enough blocks to update, then convert the updates into inserts */
        
        if (updates > ser->max_block_id()) {
            inserts += updates;
            updates = 0;
        }
        
        /* To save CPU time (from clearing many bufs) we malloc() one buf and clear it and then
        write all the blocks from that one. I hope this doesn't bias the test. */
        
        dummy_buf = ser->malloc();
        memset(dummy_buf, 0xDB, ser->get_block_size());
        
        writes.reserve(updates + inserts);
        
        /* As a simple way to avoid updating the same block twice in one transaction, select
        a contiguous range of blocks starting at a random offset within range */
        
        ser_block_id_t begin = random(0, ser->max_block_id() - updates);
        
        for (unsigned i = 0; i < updates; i++) {
            serializer_t::write_t w;
            w.block_id = begin + i;
            w.buf = dummy_buf;
            w.callback = NULL;
            writes.push_back(w);
        }
        
        /* Generate new IDs to insert by simply taking (highest ID + 1) */
        
        for (unsigned i = 0; i < inserts; i++) {
            serializer_t::write_t w;
            w.block_id = ser->max_block_id() + i;
            w.buf = dummy_buf;
            w.callback = NULL;
            writes.push_back(w);
        }
        
        start_time = get_ticks();
        
        bool done = ser->do_write(&writes[0], writes.size(), this);
        if (done) on_serializer_write_txn();   // Probably never happens
    }
    
    void on_serializer_write_txn() {
        
        ticks_t end_time = get_ticks();
        if (log) {
            txn_info_t info;
            info.start = start_time;
            info.end = end_time;
            log->push_back(info);
        }
        
        ser->free(dummy_buf);
        
        cb->on_transaction_complete();
        gdelete(this);
    }
};

#define RUN_FOREVER (-1)

struct config_t {
    
    const char *filename, *log_file;
    log_serializer_static_config_t ser_static_config;
    log_serializer_dynamic_config_t ser_dynamic_config;
    int duration;   /* Seconds */
    unsigned concurrent_txns;
    unsigned inserts_per_txn, updates_per_txn;
};

struct tester_t :
    public cpu_message_t,
    public log_serializer_t::ready_callback_t,
    public transaction_t::callback_t,
    public log_serializer_t::shutdown_callback_t
{
    log_t *log;
    log_serializer_t *ser;
    unsigned active_txns, total_txns;
    config_t *config;
    thread_pool_t *pool;
    bool stop, interrupted;
    timer_token_t *timer;
    
    struct interrupt_msg_t :
        public cpu_message_t
    {
        tester_t *tester;
        interrupt_msg_t(tester_t *tester) : tester(tester) { }
        void on_cpu_switch() {
            fprintf(stderr, "Interrupted.\n");
            tester->stop_test();
            cancel_timer(tester->timer);
        }
    } interruptor;
    
    tester_t(config_t *config, thread_pool_t *pool)
        : ser(NULL), active_txns(0), total_txns(0), config(config), pool(pool), stop(false), interrupted(false), interruptor(this)
    {
    }
    
    /* When on_cpu_switch() is called, it could either be the start message telling us to run the
    test or the shutdown message from call_later_on_this_cpu(). We differentiate by checking 'ser'. */
    void on_cpu_switch() {
        if (!ser) start();
        else shutdown();
    }
    
    void start() {
    
        if (config->log_file) {
            log = gnew<log_t>();
        } else {
            log = NULL;
        }
        
        fprintf(stderr, "Starting serializer...\n");
        ser = gnew<log_serializer_t>(config->filename, &config->ser_dynamic_config);
        if (ser->start_new(&config->ser_static_config, this)) on_serializer_ready(ser);
    }
    
    void on_serializer_ready(log_serializer_t *ls) {
        fprintf(stderr, "Running test...\n");
        timer = fire_timer_once(config->duration * 1000, &tester_t::on_timer, this);
        
        pool->set_interrupt_message(&interruptor);
        
        pump();
    }
    
    static void on_timer(void *self) {
        tester_t *tester = (tester_t *)self;
        fprintf(stderr, "Time's up.\n");
        tester->stop_test();
    }
    
    void pump() {
        
        while (active_txns < config->concurrent_txns && !stop) {
            active_txns++;
            total_txns++;
            gnew<transaction_t>(ser, log, config->inserts_per_txn, config->updates_per_txn, this);
        }
        
        if (active_txns == 0 && stop) {
            /* Serializer doesn't like shutdown() to be called from within
            on_serializer_write_txn() */
            call_later_on_this_cpu(this);
        }
    }
    
    void stop_test() {
        
        if (stop) return;
        pool->set_interrupt_message(NULL);
        
        fprintf(stderr, "Started %d transactions and completed %d of them\n",
            total_txns, total_txns - active_txns);
        if (config->log_file) {
            FILE *log_file = fopen(config->log_file, "w");
            log_t::iterator it;
            for (it = log->begin(); it != log->end(); it++) {
                fprintf(log_file, "%.6f %.6f\n", ticks_to_secs(it->start), ticks_to_secs(it->end));
            }
            fclose(log_file);
            fprintf(stderr, "Wrote log to '%s'\n", config->log_file);
            gdelete(log);
        }
        
        stop = true;
        if (active_txns > 0) {
            fprintf(stderr, "Waiting for the remaining %d transactions...\n", active_txns);
        }
    }
    
    void on_transaction_complete() {
        active_txns--;
        pump();
    }
    
    void shutdown() {
        fprintf(stderr, "Waiting for serializer to shut down...\n");
        if (ser->shutdown(this)) on_serializer_shutdown(ser);
    }
    
    void on_serializer_shutdown(log_serializer_t *ser) {
        fprintf(stderr, "Done.\n");
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
    config->log_file = NULL;
    
    config->ser_static_config.block_size = DEFAULT_BTREE_BLOCK_SIZE;
    config->ser_static_config.extent_size = DEFAULT_EXTENT_SIZE;
    config->ser_dynamic_config.gc_low_ratio = DEFAULT_GC_LOW_RATIO;
    config->ser_dynamic_config.gc_high_ratio = DEFAULT_GC_HIGH_RATIO;
    config->ser_dynamic_config.num_active_data_extents = DEFAULT_ACTIVE_DATA_EXTENTS;
    config->ser_dynamic_config.file_size = 0;
    config->ser_dynamic_config.file_zone_size = GIGABYTE;
    
    config->duration = 10;   /* Seconds */
    config->concurrent_txns = 8;
    config->inserts_per_txn = 10;
    config->updates_per_txn = 10;
    
    read_arg(argc, argv);
    
    while (argc) {
        const char *flag = read_arg(argc, argv);
        
        if (strcmp(flag, "-f") == 0) {
            config->filename = read_arg(argc, argv);
        } else if (strcmp(flag, "--log") == 0) {
            config->log_file = read_arg(argc, argv);
        
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
