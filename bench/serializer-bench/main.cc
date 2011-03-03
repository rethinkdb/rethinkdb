#include "errors.hpp"
#include "serializer/log/log_serializer.hpp"

#undef __UTILS_HPP__   /* Hack because both RethinkDB and stress-client have a utils.hpp */
namespace stress {
    #include "../stress-client/utils.hpp"
    #include "../stress-client/utils.cc"
    #include "../stress-client/random.cc"
}

struct txn_info_t {
    ticks_t start, end;
};

typedef std::deque<txn_info_t> log_t;

struct transaction_t :
    public serializer_t::write_txn_callback_t
{
    serializer_t *ser;
    ticks_t start_time;
    log_t *log;
    void *dummy_buf;
    std::vector<serializer_t::write_t> writes;
    
    struct callback_t {
        virtual void on_transaction_complete() = 0;
    } *cb;
    
    transaction_t(serializer_t *ser, log_t *log, unsigned inserts, unsigned updates, callback_t *cb)
        : ser(ser), log(log), cb(cb)
    {
        /* If there aren't enough blocks to update, then convert the updates into inserts */
        
        if (updates > ser->max_block_id().value) {
            inserts += updates;
            updates = 0;
        }
        
        /* To save CPU time (from clearing many bufs) we malloc() one buf and clear it and then
        write all the blocks from that one. I hope this doesn't bias the test. */
        
        dummy_buf = ser->malloc();
        memset(dummy_buf, 0xDB, ser->get_block_size().value());
        
        writes.reserve(updates + inserts);
        
        /* As a simple way to avoid updating the same block twice in one transaction, select
        a contiguous range of blocks starting at a random offset within range */
        
        ser_block_id_t begin = ser_block_id_t::make(stress::random(0, ser->max_block_id().value - updates));

        repli_timestamp tstamp = current_time();
        for (unsigned i = 0; i < updates; i++) {
            writes.push_back(serializer_t::write_t::make(ser_block_id_t::make(begin.value + i), tstamp, dummy_buf, true, NULL));
        }

        /* Generate new IDs to insert by simply taking (highest ID + 1) */

        for (unsigned i = 0; i < inserts; i++) {
            writes.push_back(serializer_t::write_t::make(ser_block_id_t::make(ser->max_block_id().value + i), tstamp, dummy_buf, true, NULL));
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
            //log->push_back(info);
        }
        
        ser->free(dummy_buf);
        
        cb->on_transaction_complete();
        delete(this);
    }
};

#define RUN_FOREVER (-1)

struct config_t {
    
    const char *log_file, *tps_log_file;
    log_serializer_static_config_t ser_static_config;
    log_serializer_dynamic_config_t ser_dynamic_config;
    log_serializer_private_dynamic_config_t ser_private_dynamic_config;
    int duration;   /* Seconds */
    unsigned concurrent_txns;
    unsigned inserts_per_txn, updates_per_txn;
};

struct tester_t :
    public thread_message_t,
    public transaction_t::callback_t
{
    log_t *log;
    FILE *tps_log_fd;
    log_serializer_t *ser;
    unsigned active_txns, total_txns;
    config_t *config;
    thread_pool_t *pool;
    bool stop, interrupted;
    timer_token_t *timer;
    ticks_t last_time;
    unsigned txns_last_sec;
    unsigned secs_so_far;
    
    struct interrupt_msg_t :
        public thread_message_t
    {
        tester_t *tester;
        interrupt_msg_t(tester_t *tester) : tester(tester) { }
        void on_thread_switch() {
            fprintf(stderr, "Interrupted.\n");
            tester->stop_test();
            cancel_timer(tester->timer);
        }
    } interruptor;
    
    tester_t(config_t *config, thread_pool_t *pool)
        : tps_log_fd(NULL), ser(NULL), active_txns(0), total_txns(0), config(config), pool(pool), stop(false), interrupted(false), last_time(0), txns_last_sec(0), secs_so_far(0), interruptor(this)
    {
        last_time = get_ticks();
        if(config->tps_log_file) {
            tps_log_fd = fopen(config->tps_log_file, "a");
        }
    }
    
    /* When on_thread_switch() is called, it could either be the start message telling us to run the
    test or the shutdown message from call_later_on_this_thread(). We differentiate by checking 'ser'. */
    void on_thread_switch() {
        if (!ser) start();
        else coro_t::spawn(boost::bind(&tester_t::shutdown, this));
    }
    
    void start() {
    
        if (config->log_file) {
            log = new log_t();
        } else {
            log = NULL;
        }
        
        fprintf(stderr, "Starting serializer...\n");
        ser = new log_serializer_t(&config->ser_dynamic_config, &config->ser_private_dynamic_config);
        on_serializer_ready(ser);
    }
    
    void on_serializer_ready(log_serializer_t *ls) {
        fprintf(stderr, "Running test...\n");
        if(config->duration != RUN_FOREVER) {
            timer = fire_timer_once(config->duration * 1000, &tester_t::on_timer, this);
        }
        
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
            new transaction_t(ser, log, config->inserts_per_txn, config->updates_per_txn, this);

        }
        
        // See if we need to report the TPS
        ticks_t cur_time = get_ticks();
        if(ticks_to_secs(cur_time - last_time) >= 1.0f) {
            if(tps_log_fd) {
                fprintf(tps_log_fd, "%d\n", txns_last_sec);
            }
                
            last_time = cur_time;
            txns_last_sec = 0;
            secs_so_far++;
            
            // Flush every five seconds in case of a crash
            if(secs_so_far % 5 == 0) {
                fflush(tps_log_fd);
            }
        }

        if (active_txns == 0 && stop) {
            /* Serializer doesn't like shutdown() to be called from within
               on_serializer_write_txn() */
            call_later_on_this_thread(this);
        }
    }
    
    void stop_test() {
        
        if (stop) return;
        pool->set_interrupt_message(NULL);
        
        fprintf(stderr, "Started %d transactions and completed %d of them\n",
            total_txns, total_txns - active_txns);

        fclose(tps_log_fd);
        
        if (config->log_file) {
            FILE *log_file = fopen(config->log_file, "w");
            log_t::iterator it;
            for (it = log->begin(); it != log->end(); it++) {
                fprintf(log_file, "%.6f %.6f\n", ticks_to_secs(it->start), ticks_to_secs(it->end));
            }
            fclose(log_file);
            fprintf(stderr, "Wrote log to '%s'\n", config->log_file);
            delete(log);
        }
        
        stop = true;
        if (active_txns > 0) {
            fprintf(stderr, "Waiting for the remaining %d transactions...\n", active_txns);
        }
    }
    
    void on_transaction_complete() {
        active_txns--;
        txns_last_sec++;
        pump();
    }
    
    void shutdown() {
        fprintf(stderr, "Waiting for serializer to shut down...\n");
        delete ser;
        on_serializer_shutdown();
    }
    
    void on_serializer_shutdown() {
        fprintf(stderr, "Done.\n");
        pool->shutdown();
    }
};

const char *read_arg(int &argc, char **&argv) {
    if (argc == 0) {
        fail_due_to_user_error("Expected another argument at the end.");
    } else {
        argc--;
        return (argv++)[0];
    }
} 

void parse_config(int argc, char *argv[], config_t *config) {
    
    config->ser_private_dynamic_config.db_filename = "rethinkdb_data";
#ifdef SEMANTIC_SERIALIZER_CHECK
    config->ser_private_dynamic_config.semantic_filename = "rethinkdb_data.semantic";
#endif
    config->log_file = NULL;
    config->tps_log_file = NULL;
    
    config->ser_static_config.unsafe_block_size() = DEFAULT_BTREE_BLOCK_SIZE;
    config->ser_static_config.unsafe_extent_size() = DEFAULT_EXTENT_SIZE;
    config->ser_dynamic_config.gc_low_ratio = DEFAULT_GC_LOW_RATIO;
    config->ser_dynamic_config.gc_high_ratio = DEFAULT_GC_HIGH_RATIO;
    config->ser_dynamic_config.num_active_data_extents = DEFAULT_ACTIVE_DATA_EXTENTS;
    config->ser_dynamic_config.file_size = 0;
    config->ser_dynamic_config.file_zone_size = GIGABYTE;
    
    config->duration = 10;   /* Seconds */
    config->concurrent_txns = 8;
    config->inserts_per_txn = 10;
    config->updates_per_txn = 2;
    
    read_arg(argc, argv);
    
    while (argc) {
        const char *flag = read_arg(argc, argv);
        
        if (strcmp(flag, "-f") == 0) {
            config->ser_private_dynamic_config.db_filename = read_arg(argc, argv);
#ifdef SEMANTIC_SERIALIZER_CHECK
            config->ser_private_dynamic_config.semantic_filename = config->ser_private_dynamic_config.db_filename + ".semantic";
#endif
        } else if (strcmp(flag, "--log") == 0) {
            config->log_file = read_arg(argc, argv);
        } else if (strcmp(flag, "--tps-log") == 0) {
            config->tps_log_file = read_arg(argc, argv);
        } else if (strcmp(flag, "--block-size") == 0) {
            config->ser_static_config.unsafe_block_size() = atoi(read_arg(argc, argv));
        } else if (strcmp(flag, "--extent-size") == 0) {
            config->ser_static_config.unsafe_extent_size() = atoi(read_arg(argc, argv));
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
            fail_due_to_user_error("Don't know how to handle \"%s\"", flag);
        }
    }

    rassert(config->ser_static_config.block_size().ser_value() > 0);
    rassert(config->ser_static_config.extent_size() > 0);
    rassert(config->ser_dynamic_config.num_active_data_extents > 0);
    rassert(config->ser_dynamic_config.file_zone_size > 0);
    rassert(config->duration > 0 || config->duration == RUN_FOREVER);
    rassert(config->concurrent_txns > 0);
}

int main(int argc, char *argv[]) {
    
    config_t config;
    parse_config(argc, argv, &config);
    
    thread_pool_t thread_pool(1);
    tester_t tester(&config, &thread_pool);
    thread_pool.run(&tester);
    
    return 0;
}
