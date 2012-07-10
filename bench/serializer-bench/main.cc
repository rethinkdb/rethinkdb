#include "errors.hpp"
#include <boost/bind.hpp>

#include "arch/arch.hpp"
#include "serializer/log/log_serializer.hpp"

namespace stress {
    #include "../stress-client/utils.hpp"
    #include "../stress-client/utils.cc"
    #include "../stress-client/random.cc"
}

class timer_token_t;

struct txn_info_t {
    ticks_t start, end;
};

typedef std::deque<txn_info_t> log_t;

struct txn_callback_t {
    virtual void on_transaction_complete() = 0;
} *cb;

void transact(serializer_t *ser, log_t *log, unsigned inserts, unsigned updates, txn_callback_t *cb) {
    /* If there aren't enough blocks to update, then convert the updates into inserts */
    std::vector<serializer_write_t> writes;

    if (updates > ser->max_block_id()) {
        inserts += updates;
        updates = 0;
    }

    /* To save CPU time (from clearing many bufs) we malloc() one buf and clear it and then
    write all the blocks from that one. I hope this doesn't bias the test. */

    void *dummy_buf = ser->malloc();
    memset(dummy_buf, 0xDB, ser->get_block_size().value());

    writes.reserve(updates + inserts);

    /* As a simple way to avoid updating the same block twice in one transaction, select
    a contiguous range of blocks starting at a random offset within range */

    block_id_t begin = stress::random(0, ser->max_block_id() - updates);

    // We just need some value for this.
    repli_timestamp_t tstamp = repli_timestamp_t::distant_past;
    for (unsigned i = 0; i < updates; i++) {
        writes.push_back(serializer_write_t::make_update(begin + i, tstamp, dummy_buf));
    }

    /* Generate new IDs to insert by simply taking (highest ID + 1) */
    for (unsigned i = 0; i < inserts; i++) {
        writes.push_back(serializer_write_t::make_update(ser->max_block_id() + i, tstamp, dummy_buf));
    }

    do_writes(ser, writes, DEFAULT_DISK_ACCOUNT);

    ser->free(dummy_buf);

    cb->on_transaction_complete();
}

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
    public txn_callback_t
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
    boost::scoped_ptr<io_backender_t> io_backender;
    
    
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
        make_io_backender(aio_native, &io_backender);
        last_time = get_ticks();
        if(config->tps_log_file) {
            tps_log_fd = fopen(config->tps_log_file, "a");
        }
    }
    
    /* When on_thread_switch() is called, it could either be the start message telling us to run the
    test or the shutdown message from call_later_on_this_thread(). We differentiate by checking 'ser'. */
    void on_thread_switch() {
        if (!ser) coro_t::spawn(boost::bind(&tester_t::start, this));
        else coro_t::spawn(boost::bind(&tester_t::shutdown, this));
    }
    
    void start() {
        if (config->log_file) {
            log = new log_t();
        } else {
            log = NULL;
        }

        fprintf(stderr, "Creating a database...\n");
        log_serializer_t::create(config->ser_dynamic_config,
                                 io_backender.get(),
                                 config->ser_private_dynamic_config,
                                 config->ser_static_config,
                                 &get_global_perfmon_collection());
        
        fprintf(stderr, "Starting serializer...\n");
        ser = new log_serializer_t(config->ser_dynamic_config,
                                   io_backender.get(),
                                   config->ser_private_dynamic_config,
                                   &get_global_perfmon_collection());
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
            coro_t::spawn_sometime(boost::bind(transact, ser, log, config->inserts_per_txn, config->updates_per_txn, this));
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

        if (tps_log_fd)
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
    
    // config->ser_static_config uses its own defaults from constructor
    // config->ser_dynamic_config uses its own defaults from constructor
    
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
            config->ser_static_config.block_size_ = atoi(read_arg(argc, argv));
        } else if (strcmp(flag, "--extent-size") == 0) {
            config->ser_static_config.extent_size_ = atoi(read_arg(argc, argv));
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
    
    thread_pool_t thread_pool(1, true);
    tester_t tester(&config, &thread_pool);
    thread_pool.run(&tester);
    
    return 0;
}
