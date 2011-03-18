#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "server/cmd_args.hpp"
#include "utils.hpp"
#include "help.hpp"
#include "arch/arch.hpp"
#include "cmd_args.hpp"

/* Note that this file only parses arguments for the 'serve' and 'create' subcommands. */

void usage_serve() {
    Help_Pager *help = Help_Pager::instance();
    help->pagef("Usage:\n"
                "        rethinkdb serve [OPTIONS] [-f <file_1> -f <file_2> ...]\n"
                "        Serve a database with one or more storage files.\n"
                "\n"
                "Options:\n"

    //          "                        24 characters start here.                              | < last character
                "  -f, --file            Path to file or block device where database goes.\n"
                "                        Can be specified multiple times to use multiple files.\n");
    help->pagef("  -c, --cores           Number of cores to use for handling requests.\n"
                "  -m, --max-cache-size  Maximum amount of RAM to use for caching disk\n"
                "                        blocks, in megabytes. This should be ~80%% of\n" 
                "                        the RAM you want RethinkDB to use.\n"
                "  -p, --port            Socket port to listen on. Defaults to %d.\n", DEFAULT_LISTEN_PORT);
    help->pagef("\n"
                "Flushing options:\n");
    help->pagef("      --wait-for-flush  Do not respond to commands until changes are durable.\n"
                "                        Expects 'y' or 'n'.\n"
                "      --flush-timer     Time in milliseconds that the server should allow\n"
                "                        changes to sit in memory before flushing it to disk.\n"
                "                        Pass \"disable\" to allow modified data to sit in memory\n"
                "                        indefinitely.");
    if (DEFAULT_FLUSH_TIMER_MS == NEVER_FLUSH) {
        help->pagef(" Defaults to \"disable\".\n");
    } else {
        help->pagef(" Defaults to %dms.\n", DEFAULT_FLUSH_TIMER_MS);
    }
    help->pagef("      --flush-threshold Number of transactions waiting for a flush on any slice\n"
                "                        at which a flush is automatically triggered. In\n"
                "                        combination with --wait-for-flush this option can be used\n"
                "                        to optimize the write latency for concurrent strong\n"
                "                        durability workloads. Defaults to %d\n"
                "      --flush-concurrency   Maximal number of concurrently active flushes per\n"
                "                        slice. Defaults to %d\n",
                                DEFAULT_FLUSH_WAITING_THRESHOLD, DEFAULT_MAX_CONCURRENT_FLUSHES);
    help->pagef("      --unsaved-data-limit\n" 
                "                        The maximum amount (in MB) of dirty data (data which is\n"
                "                        held in memory but has not yet been serialized to disk.)\n"
                "                        ");
    if (DEFAULT_UNSAVED_DATA_LIMIT == 0) {
        help->pagef("Defaults to %1.1f times the max cache size.\n", MAX_UNSAVED_DATA_LIMIT_FRACTION);
    }
    else {
        help->pagef("Defaults to %ld MB.\n", DEFAULT_UNSAVED_DATA_LIMIT / MEGABYTE);
    }
    help->pagef("\n"
                "Disk options:\n");
    help->pagef("      --gc-range low-high  (e.g. --gc-range 0.5-0.75)\n"
                "                        The proportion of garbage maintained by garbage\n"
                "                        collection.\n"
                "      --active-data-extents\n"
                "                        How many places in the file to write to at once.\n"
                "      --io-backend      Possible options are 'native' (the default) and 'pool'.\n"
                "                        The native backend is most efficient, but may have\n"
                "                        performance problems in some environments.\n"
                "      --read-ahead      Enable or disable read ahead during cache warmup. Read\n"
                "                        ahead can significantly speed up the cache warmup time\n"
                "                        for disks which have high costs for random access.\n"
                "                        Expects 'y' or 'n'.\n"
                "\n"
                "Output options:\n"
                "  -v, --verbose         Print extra information to standard output.\n");
    help->pagef("  -l, --log-file        File to log to. If not provided, messages will be\n"
                "                        printed to stderr.\n");
#ifdef SEMANTIC_SERIALIZER_CHECK
    help->pagef("  -S, --semantic-file   Path to the semantic file for the previously specified\n"
                "                        database file. Can only be specified after the path to\n"
                "                        the database file. Default is the name of the database\n"
                "                        file with \"%s\" appended.\n", DEFAULT_SEMANTIC_EXTENSION);
#endif
    //TODO move this in to an advanced options help file
    /* help->pagef("      --coroutine-stack-size\n"
                "                        How much space is allocated for the stacks of coroutines.\n"
                "                        Defaults to %d\n", COROUTINE_STACK_SIZE); */ 
    help->pagef("\n"
                "Replication & Failover options:\n"
                "      --slave-of host:port Run this server as a slave of a master server. As a\n"
                "                        slave it will be replica of the master and will respond\n"
                "                        only to gets. When the master goes down it will begin\n"
                "                        responding to writes.\n"
                "      --failover-script Used in conjunction with --slave-of to specify a script\n"
                "                        that will be run when the master fails and comes back up\n"
                "                        see manual for an example script.\n" //TODO @jdoliner add this to the manual slava where is the manual?
                "      --run-behind-elb  Used in conjunction with --slave-of makes the server\n"
                "                        compatible with Amazon Elastic Load Balancer (using TCP as\n"
                "                        the protocol.) See manual for a detailed description.\n"); //TODO add this to manual
    help->pagef("\n"
                "Serve can be called with no arguments to run a server with default parameters.\n"
                "For best performance RethinkDB should be run with one --file per device and a\n"
                "--max-cache-size no more than 80%% of the RAM it will have available to it\n");
    help->pagef("\n"
                "In general how you flush is a trade-off between performance and how much data\n" 
                "you risk losing to a crash. With --wait-for-flush enabled no data is ever at risk.\n"
                "Specifying --flush-timer means that data sent more than --flush-timer is guaranteed\n" //TODO @slava this guarantee isn't quite true but is easy to explain is it okay?
                "to be on disk. Warning: when IO reaches saturation this guarantee no longer exists.\n"
                "--unsaved-data-limit allows you to limit how much data could be lost with a crash.\n"
                "Unlike --flush-timer this flag is a hard limit and will throttle connections when it\n"
                "is reached.\n");
    help->pagef("\n"
                "The --gc-range defines the proportion of the database that can be garbage.\n"
                "A high value will result in better performance at the cost of higher disk usage.\n"
                "The --active-data-extents should be based on the devices being used.\n"
                "For values known to maximize performance, consult RethinkDB support.\n");
    exit(0);
}

void usage_create() {
    Help_Pager *help = Help_Pager::instance();
    help->pagef("Usage:\n"
                "        rethinkdb create [OPTIONS] -f <file_1> [-f <file_2> ...]\n"
                "        Create an empty database with one or more storage files.\n");
    help->pagef("\n"
                "On disk format options:\n"
                "  -f, --file            Path to file or block device where database goes. Can be\n"
                "                        specified multiple times to use multiple files.\n"
                "  -s, --slices          Total number of slices.\n"
                "      --block-size      Size of a block, in bytes.\n"
                "      --extent-size     Size of an extent, in bytes.\n"
                "      --diff-log-size   Size of the differential log, in megabytes\n"
                "                        (Default: %d)\n", (int)(DEFAULT_PATCH_LOG_SIZE / MEGABYTE));
    help->pagef("\n"
                "Output options:\n"
                "  -l, --log-file        File to log to. If not provided, messages will be\n"
                "                        printed to stderr.\n"
           );
    help->pagef("\n"
                "Behaviour options:\n"
                "      --force           Create a new database even if there already is one.\n"
           );
    help->pagef("\n"
                "Create makes an empty RethinkDB database. This files must be served together\n"
                "using the serve command and. For best performance RethinkDB should be run with\n"
                "one -file per device\n");
    help->pagef("\n"
                "--block-size and --extent-size should be based on the devices being used.\n"
                "For values known to maximize performance, consult RethinkDB support.\n");
    exit(0);
}

enum {
    wait_for_flush = 256, // Start these values above the ASCII range.
    flush_timer,
    unsaved_data_limit,
    gc_range,
    active_data_extents,
    io_backend,
    block_size,
    extent_size,
    read_ahead,
    diff_log_size,
    force_create,
    coroutine_stack_size,
    slave_of,
    failover_script,
    heartbeat_timeout,
    flush_concurrency,
    flush_threshold,
    run_behind_elb,
    failover
};

cmd_config_t parse_cmd_args(int argc, char *argv[]) {
    parsing_cmd_config_t config;

    std::vector<log_serializer_private_dynamic_config_t>& private_configs = config.store_dynamic_config.serializer_private;

    /* main() will have automatically inserted "serve" if no argument was specified */
    rassert(!strcmp(argv[0], "serve") || !strcmp(argv[0], "create"));

    if (!strcmp(argv[0], "create")) {
        if (argc >= 2 && !strcmp(argv[1], "help")) {
            usage_create();
        }
        config.create_store = true;
        config.shutdown_after_creation = true;
    }

    bool slices_set_by_user = false;
    long long override_diff_log_size = -1;
    optind = 1; // reinit getopt
    while(1)
    {
        int do_help = 0;
        int do_force_create = 0;
        struct option long_options[] =
            {
                {"wait-for-flush",       required_argument, 0, wait_for_flush},
                {"flush-timer",          required_argument, 0, flush_timer},
                {"flush-concurrency",    required_argument, 0, flush_concurrency},
                {"flush-threshold",      required_argument, 0, flush_threshold},
                {"unsaved-data-limit",   required_argument, 0, unsaved_data_limit},
                {"gc-range",             required_argument, 0, gc_range},
                {"block-size",           required_argument, 0, block_size},
                {"extent-size",          required_argument, 0, extent_size},
                {"read-ahead",           required_argument, 0, read_ahead},
                {"diff-log-size",        required_argument, 0, diff_log_size},
                {"active-data-extents",  required_argument, 0, active_data_extents},
                {"io-backend",           required_argument, 0, io_backend},
                {"coroutine-stack-size", required_argument, 0, coroutine_stack_size},
                {"cores",                required_argument, 0, 'c'},
                {"slices",               required_argument, 0, 's'},
                {"file",                 required_argument, 0, 'f'},
#ifdef SEMANTIC_SERIALIZER_CHECK
                {"semantic-file",        required_argument, 0, 'S'},
#endif
                {"max-cache-size",       required_argument, 0, 'm'},
                {"log-file",             required_argument, 0, 'l'},
                {"port",                 required_argument, 0, 'p'},
                {"verbose",              no_argument, (int*)&config.verbose, 1},
                {"force",                no_argument, &do_force_create, 1},
                {"help",                 no_argument, &do_help, 1},
                {"slave_of",             required_argument, 0, slave_of},
                {"failover-script",      required_argument, 0, failover_script},
                {"heartbeat-timeout",    required_argument, 0, heartbeat_timeout}, //TODO @sam push this through to where you want it
                {"run-behind-elb",       required_argument, 0, run_behind_elb},
                {"failover",             no_argument, 0, failover}, //TODO @sam @jdoliner push this through when we know where it goes
                {0, 0, 0, 0}
            };

        int option_index = 0;
        int c = getopt_long(argc, argv, "vc:s:f:S:m:l:p:h", long_options, &option_index);

        if (do_help)
            c = 'h';
        if (do_force_create)
            c = force_create;
     
        /* Detect the end of the options. */
        if (c == -1)
            break;

        switch (c)
        {
            case 0:
                break;
            case 'v':
                config.verbose = true; break;
            case 'p':
                config.set_port(optarg); break;
            case 'l':
                config.set_log_file(optarg); break;
            case 'c':
                config.set_cores(optarg); break;
            case 's':
                slices_set_by_user = true;
                config.set_slices(optarg); break;
            case 'f':
                config.push_private_config(optarg); break;
#ifdef SEMANTIC_SERIALIZER_CHECK
            case 'S':
                config.set_last_semantic_file(optarg); break;
#endif
            case 'm':
                config.set_max_cache_size(optarg); break;
            case wait_for_flush:
                config.set_wait_for_flush(optarg); break;
            case flush_timer:
                config.set_flush_timer(optarg); break;
            case flush_threshold:
                config.set_flush_waiting_threshold(optarg); break;
            case flush_concurrency:
                config.set_max_concurrent_flushes(optarg); break;
            case unsaved_data_limit:
                config.set_unsaved_data_limit(optarg); break;
            case gc_range:
                config.set_gc_range(optarg); break;
            case active_data_extents: 
                config.set_active_data_extents(optarg); break;
            case io_backend:
                config.set_io_backend(optarg); break;
            case block_size:
                config.set_block_size(optarg); break;
            case extent_size:
                config.set_extent_size(optarg); break;
            case read_ahead:
                config.set_read_ahead(optarg); break;
            case diff_log_size:
                override_diff_log_size = config.parse_diff_log_size(optarg); break;
            case coroutine_stack_size:
                config.set_coroutine_stack_size(optarg); break;
            case force_create:
                config.force_create = true; break;
            case slave_of:
                config.set_master_addr(optarg); break;
            case failover_script:
                config.set_failover_file(optarg); break;
            case heartbeat_timeout:
                not_implemented();
                break;
            case run_behind_elb:
                config.set_elb_port(optarg); break;
            case 'h':
            default:
                /* getopt_long already printed an error message. */
                if (config.create_store) {
                    usage_create();
                } else {
                    usage_serve();
                }
        }
    }

    if (optind < argc) {
        fail_due_to_user_error("Unexpected extra argument: \"%s\"", argv[optind]);
    }
    
    /* "Idiot mode" -- do something reasonable for novice users */
    
    if (private_configs.empty() && !config.create_store) {        
        struct log_serializer_private_dynamic_config_t db_info;
        db_info.db_filename = DEFAULT_DB_FILE_NAME;
#ifdef SEMANTIC_SERIALIZER_CHECK
        db_info.semantic_filename = std::string(DEFAULT_DB_FILE_NAME) + DEFAULT_SEMANTIC_EXTENSION;
#endif
        private_configs.push_back(db_info);
        
        int res = access(DEFAULT_DB_FILE_NAME, F_OK);
        if (res == 0) {
            /* Found a database file -- try to load it */
            config.create_store = false;   // This is redundant
        } else if (res == -1 && errno == ENOENT) {
            /* Create a new database */
            config.create_store = true;
            config.shutdown_after_creation = false;
        } else {
            fail_due_to_user_error("Could not access() path \"%s\": %s", DEFAULT_DB_FILE_NAME, strerror(errno));
        }
    } else if (private_configs.empty() && config.create_store) {
        fprintf(stderr, "You must explicitly specify one or more paths with -f.\n");
        usage_create();
    }
    
    /* Sanity-check the input */
    
    if (config.store_dynamic_config.cache.max_dirty_size == 0 ||
        config.store_dynamic_config.cache.max_dirty_size >
        config.store_dynamic_config.cache.max_size * MAX_UNSAVED_DATA_LIMIT_FRACTION) {
        
        /* The page replacement algorithm won't work properly if the number of dirty bufs
        is allowed to be more than about half of the total number of bufs. */
        config.store_dynamic_config.cache.max_dirty_size =
            (long long int)(config.store_dynamic_config.cache.max_size * MAX_UNSAVED_DATA_LIMIT_FRACTION);
    }
    
    if (config.store_dynamic_config.cache.wait_for_flush == true &&
        config.store_dynamic_config.cache.flush_timer_ms == NEVER_FLUSH &&
        config.store_dynamic_config.cache.flush_dirty_size != 0) {
        printf("WARNING: Server is configured to wait for data to be flushed\n"
               "to disk before returning, but also configured to wait\n"
               "indefinitely before flushing data to disk. Setting wait-for-flush\n"
               "to 'no'.\n\n");
        config.store_dynamic_config.cache.wait_for_flush = false;
    }
    
    if (config.store_static_config.serializer.extent_size() % config.store_static_config.serializer.block_size().ser_value() != 0) {
        fail_due_to_user_error("Extent size (%d) is not a multiple of block size (%d).", 
             config.store_static_config.serializer.extent_size(),
             config.store_static_config.serializer.block_size().ser_value());
    }
    
    if (config.store_static_config.serializer.extent_size() == config.store_dynamic_config.serializer.file_zone_size) {
        printf("WARNING: You made the extent size the same as the file zone size.\n"
               "This is not a big problem, but it is better to use a huge or\n"
               "unlimited zone size to get the effect you probably want.\n");
    }
    
    // It's probably not necessary for this parameter to be independently configurable
    config.store_dynamic_config.cache.flush_dirty_size =
        (long long int)(config.store_dynamic_config.cache.max_dirty_size * FLUSH_AT_FRACTION_OF_UNSAVED_DATA_LIMIT);

    //slices divisable by the number of files
    if ((config.store_static_config.btree.n_slices % config.store_dynamic_config.serializer_private.size()) != 0) {
        if (slices_set_by_user)
            fail_due_to_user_error("Slices must be divisable by the number of files\n");
        else {
            config.store_static_config.btree.n_slices -= config.store_static_config.btree.n_slices % config.store_dynamic_config.serializer_private.size();
            if (config.store_static_config.btree.n_slices <= 0)
                fail_due_to_user_error("Failed to set number of slices automatically. Please specify it manually by using the -s option.\n");
        }
    }

    /* Convert values which depends on others to be set first */
    if (override_diff_log_size >= 0) {
        config.store_static_config.cache.n_patch_log_blocks = override_diff_log_size / config.store_static_config.serializer.block_size().ser_value() / config.store_static_config.btree.n_slices;
    } else {
        config.store_static_config.cache.n_patch_log_blocks = DEFAULT_PATCH_LOG_SIZE / config.store_static_config.serializer.block_size().ser_value() / config.store_static_config.btree.n_slices;
    }
    
    return config;
}

parsing_cmd_config_t::parsing_cmd_config_t() {
    parsing_failed = false;
}

void parsing_cmd_config_t::push_private_config(const char* value) {
    if (store_dynamic_config.serializer_private.size() >= MAX_SERIALIZERS)
        fail_due_to_user_error("Cannot use more than %d files.", MAX_SERIALIZERS);
    
    // Unfortunately, we cannot really check that here. Have to check accessibility later.
    /*if (!create_flag) {
        // Check if we have write and read permissions
        int res = access(value, W_OK | R_OK);
        if (res == 0) {
            // Everything good...
        }
        else
            fail_due_to_user_error("Inaccessible or invalid database file: \"%s\": %s", value, strerror(errno));
    }*/
    
    struct log_serializer_private_dynamic_config_t db_info;
    db_info.db_filename = std::string(value);
#ifdef SEMANTIC_SERIALIZER_CHECK
    db_info.semantic_filename = std::string(value) + DEFAULT_SEMANTIC_EXTENSION;
#endif
    store_dynamic_config.serializer_private.push_back(db_info);
}

#ifdef SEMANTIC_SERIALIZER_CHECK
void parsing_cmd_config_t::set_last_semantic_file(const char* value) {    
    if (store_dynamic_config.serializer_private.size() == 0)
        fail_due_to_user_error("You can specify the semantic file name only after specifying a database file name.");

    // Check if we can create and access the file at this path
    FILE* logfile = fopen(value, "a+");
    if (logfile == NULL)
        fail_due_to_user_error("Inaccessible or invalid semantic file: \"%s\": %s", value, strerror(errno));
    else
        fclose(logfile);
        
    store_dynamic_config.serializer_private.back().semantic_filename = std::string(value);
    
}
#endif

void parsing_cmd_config_t::set_flush_timer(const char* value) {
    int& target = store_dynamic_config.cache.flush_timer_ms;
    
    if (strcmp(value, "disable") == 0)
        target = NEVER_FLUSH;
    else {
        target = parse_int(value);
        if (parsing_failed || !is_at_least(target, 0))
            fail_due_to_user_error("flush timer should not be negative; use 'disable' to allow changes to sit in memory indefinitely.");
    }
}

void parsing_cmd_config_t::set_flush_waiting_threshold(const char* value) {
    int& target = store_dynamic_config.cache.flush_waiting_threshold;

    target = parse_int(value);
    if (parsing_failed || !is_at_least(target, 1))
        fail_due_to_user_error("Flush threshold must be a positive number.");
}

void parsing_cmd_config_t::set_max_concurrent_flushes(const char* value) {
    int& target = store_dynamic_config.cache.max_concurrent_flushes;

    target = parse_int(value);
    if (parsing_failed || !is_at_least(target, 1))
        fail_due_to_user_error("Max concurrent flushes must be a positive number.");
}

void parsing_cmd_config_t::set_extent_size(const char* value) {
    long long int target;
    const long long int minimum_value = 1ll;
    const long long int maximum_value = TERABYTE;
    
    target = parse_longlong(value);
    if (parsing_failed || !is_in_range(target, minimum_value, maximum_value))
        fail_due_to_user_error("Extent size must be a number from %d to %d.", minimum_value, maximum_value);
        
    store_static_config.serializer.unsafe_extent_size() = static_cast<long long unsigned int>(target);
}

void parsing_cmd_config_t::set_read_ahead(const char* value) {
    if (strlen(value) != 1 || !(value[0] == 'y' || value[0] == 'n'))
        fail_due_to_user_error("Read-ahead expects 'y' or 'n'.");

    store_dynamic_config.serializer.read_ahead = (value[0] == 'y');
}

long long parsing_cmd_config_t::parse_diff_log_size(const char* value) {
    long long int result;
    const long long int minimum_value = 0ll;
    const long long int maximum_value = TERABYTE;

    result = parse_longlong(value) * MEGABYTE;
    if (parsing_failed || !is_in_range(result, minimum_value, maximum_value))
        fail_due_to_user_error("Diff log size must be a number from %d to %d.", minimum_value, maximum_value);

    return result;
}

void parsing_cmd_config_t::set_block_size(const char* value) {
    int target;
    const int minimum_value = 1;
    const int maximum_value = DEVICE_BLOCK_SIZE * 1000;
    
    target = parse_int(value);
    if (parsing_failed || !is_in_range(target, minimum_value, maximum_value))
        fail_due_to_user_error("Block size must be a number from %d to %d.", minimum_value, maximum_value);
    if (target % DEVICE_BLOCK_SIZE != 0)
        fail_due_to_user_error("Block size must be a multiple of %d.", DEVICE_BLOCK_SIZE);
        
    store_static_config.serializer.unsafe_block_size() = static_cast<unsigned int>(target);
}

void parsing_cmd_config_t::set_active_data_extents(const char* value) {
    int target;
    const int minimum_value = 1;
    const int maximum_value = MAX_ACTIVE_DATA_EXTENTS;

    target = parse_int(value);
    if (parsing_failed || !is_in_range(target, minimum_value, maximum_value))
        fail_due_to_user_error("Active data extents must be a number from %d to %d.", minimum_value, maximum_value);
    
    store_dynamic_config.serializer.num_active_data_extents = static_cast<unsigned int>(target);
}

void parsing_cmd_config_t::set_gc_range(const char* value) {
    float low = 0.0;
    float high = 0.0;
    int consumed = 0;
    if (2 != sscanf(value, "%f-%f%n", &low, &high, &consumed) || ((size_t)consumed) != strlen(value)) {
        fail_due_to_user_error("gc-range expects \"low-high\"");
    }
    if (!(MIN_GC_LOW_RATIO <= low && low < high && high <= MAX_GC_HIGH_RATIO)) {
        fail_due_to_user_error("gc-range expects \"low-high\", with %.2f <= low < high <= %.2f",
             MIN_GC_LOW_RATIO, MAX_GC_HIGH_RATIO);
    }
    store_dynamic_config.serializer.gc_low_ratio = low;
    store_dynamic_config.serializer.gc_high_ratio = high;
}

// TODO: Take all these functions that take a value parameter, and put
// them in some source file for which optarg is not in scope.  This is
// necessary for safety.

void parsing_cmd_config_t::set_unsaved_data_limit(const char* value) {
    int int_value = parse_int(value);
    if (parsing_failed || !is_positive(int_value))
        fail_due_to_user_error("Unsaved data limit must be a positive number.");
        
    store_dynamic_config.cache.max_dirty_size = (long long)(int_value) * MEGABYTE;
}

void parsing_cmd_config_t::set_wait_for_flush(const char* value) {
    // TODO: duplicated 'y' 'n' code.
    if (strlen(value) != 1 || !(value[0] == 'y' || value[0] == 'n'))
        fail_due_to_user_error("Wait-for-flush expects 'y' or 'n'.");
    
    store_dynamic_config.cache.wait_for_flush = value[0] == 'y';
}

void parsing_cmd_config_t::set_max_cache_size(const char* value) {
    int int_value = parse_int(value);
    if (parsing_failed || !is_positive(int_value))
        fail_due_to_user_error("Cache size must be a positive number.");

    // TODO: Explain why this code is commented out, for wtf is there commented out code without explanation?

    //if (is_at_most(int_value, static_cast<int>(get_total_ram() / 1024 / 1024)))
    //    fail_due_to_user_error("Cache size is larger than this machine's total memory (%s MB).", get_total_ram() / 1024 / 1024);
        
    store_dynamic_config.cache.max_size = (long long)(int_value) * MEGABYTE;
}

void parsing_cmd_config_t::set_slices(const char* value) {
    int& target = store_static_config.btree.n_slices;
    const int minimum_value = 1;
    const int maximum_value = MAX_SLICES;
    
    target = parse_int(value);
    if (parsing_failed || !is_in_range(target, minimum_value, maximum_value))
        fail_due_to_user_error("Number of slices must be a number from %d to %d.", minimum_value, maximum_value);
}

void parsing_cmd_config_t::set_log_file(const char* value) {
    if (strlen(value) > MAX_LOG_FILE_NAME - 1)
        fail_due_to_user_error("The name of the specified log file is too long (must not be more than %d).", MAX_LOG_FILE_NAME - 1);

    // See if we can open or create the file at this path with write permissions
    FILE* logfile = fopen(value, "a");
    if (logfile == NULL)
        fail_due_to_user_error("Inaccessible or invalid log file: \"%s\": %s", value, strerror(errno));
    else
        fclose(logfile);
    
    strncpy(log_file_name, value, MAX_LOG_FILE_NAME);
}

void parsing_cmd_config_t::set_port(const char* value) {
    int& target = port;
    const int minimum_value = 0;
    const int maximum_value = 65535;
    
    target = parse_int(value);
    if (parsing_failed || !is_in_range(target, minimum_value, maximum_value))
        fail_due_to_user_error("Invalid TCP port (must be a number from %d to %d).", minimum_value, maximum_value);
}

void parsing_cmd_config_t::set_cores(const char* value) {
    int& target = n_workers;
    const int minimum_value = 1;
    // Subtract one because of utility cpu
    const int maximum_value = MAX_THREADS - 1;
    
    target = parse_int(value);
    if (parsing_failed || !is_in_range(target, minimum_value, maximum_value))
        fail_due_to_user_error("Number of CPUs must be a number from %d to %d.", minimum_value, maximum_value);
}

void parsing_cmd_config_t::set_coroutine_stack_size(const char* value) {
    const int minimum_value = 8126;
    const int maximum_value = MAX_COROUTINE_STACK_SIZE;
    
    int target = parse_int(value);
    if (parsing_failed || !is_in_range(target, minimum_value, maximum_value))
        fail_due_to_user_error("Coroutine stack size must be a number from %d to %d.", minimum_value, maximum_value);

    coro_t::set_coroutine_stack_size(target);
}

void parsing_cmd_config_t::set_master_addr(char *value) {
    char *token = strtok(value, ":");
    if (token == NULL || strlen(token) > MAX_HOSTNAME_LEN - 1)
        fail_due_to_user_error("Invalid master address, address should be of the form hostname:port");

    strcpy(replication_config.hostname, token);

    token = strtok(NULL, ":");
    if (token == NULL)
        fail_due_to_user_error("Invalid master address, address should be of the form hostname:port");

    replication_config.port = parse_int(token);

    replication_config.active = true;
}

void parsing_cmd_config_t::set_failover_file(const char* value) {
    if (strlen(value) > MAX_PATH_LEN)
        fail_due_to_user_error("Failover script path is too long");

    strcpy(failover_config.failover_script_path, value);
}
void parsing_cmd_config_t::set_heartbeat_timeout(const char* value) {
    failover_config.heartbeat_timeout = parse_int(value);

    if (failover_config.heartbeat_timeout < 0)
        fail_due_to_user_error("Heartbeat cannot be negative");
}

void parsing_cmd_config_t::set_elb_port(const char *value) {
    int& target = failover_config.elb_port;
    const int minimum_value = 0;
    const int maximum_value = 65535;
    
    target = parse_int(value);
    if (parsing_failed || !is_in_range(target, minimum_value, maximum_value))
        fail_due_to_user_error("Invalid TCP port (must be a number from %d to %d).", minimum_value, maximum_value);
}

void parsing_cmd_config_t::set_io_backend(const char* value) {
    /* #if WE_ARE_ON_LINUX */
    if(strcmp(value, "native") == 0) {
        store_dynamic_config.serializer.io_backend = aio_native;
    } else if(strcmp(value, "pool") == 0) {
        store_dynamic_config.serializer.io_backend = aio_pool;
    } else {
        fail_due_to_user_error("Possible options for IO backend are 'native' and 'pool'.");
    }
    /* #endif */
}

long long int parsing_cmd_config_t::parse_longlong(const char* value) {
    char* endptr;
    const long long int result = strtoll(value, &endptr, 10);
    
    parsing_failed = *endptr != '\0' // Tests for invalid characters (or empty string)
            || errno == ERANGE; // Tests for range problems (too small / too large values)
    
    return result;
}

int parsing_cmd_config_t::parse_int(const char* value) {
    char* endptr;
    const int result = strtol(value, &endptr, 10);
    
    parsing_failed = *endptr != '\0' // Tests for invalid characters (or empty string)
            || errno == ERANGE; // Tests for range problems (too small / too large values)
    
    return result;
}

template<typename T> bool parsing_cmd_config_t::is_positive(const T value) const {
    return value > 0;
}
template<typename T> bool parsing_cmd_config_t::is_in_range(const T value, const T minimum_value, const T maximum_value) const {
    return is_at_least(value, minimum_value) && is_at_most(value, maximum_value);
}
template<typename T> bool parsing_cmd_config_t::is_at_least(const T value, const T minimum_value) const {
    return value >= minimum_value;
}
template<typename T> bool parsing_cmd_config_t::is_at_most(const T value, const T maximum_value) const {
    return value <= maximum_value;
}

/* Printing the configuration */
void cmd_config_t::print_runtime_flags() {
    printf("--- Runtime ----\n");
    printf("Threads............%d\n", n_workers);
    
    printf("Block cache........%lldMB\n", store_dynamic_config.cache.max_size / 1024 / 1024);
    printf("Wait for flush.....");
    if(store_dynamic_config.cache.wait_for_flush) {
        printf("Y\n");
    } else {
        printf("N\n");
    }
    printf("Flush timer........");
    if(store_dynamic_config.cache.flush_timer_ms == NEVER_FLUSH) {
        printf("Never\n");
    } else {
        printf("%dms\n", store_dynamic_config.cache.flush_timer_ms);
    }
    printf("Flush concurrency..%d\n", store_dynamic_config.cache.max_concurrent_flushes);
    printf("Flush threshold....%d\n", store_dynamic_config.cache.flush_waiting_threshold);

    printf("Read ahead.........");
    if(store_dynamic_config.serializer.read_ahead)
        printf("enabled\n");
    else
        printf("disabled\n");
    printf("Active writers.....%d\n", store_dynamic_config.serializer.num_active_data_extents);
    printf("GC range...........%g - %g\n",
           store_dynamic_config.serializer.gc_low_ratio,
           store_dynamic_config.serializer.gc_high_ratio);
    
    printf("Port...............%d\n", port);
}

void cmd_config_t::print_database_flags() {
    printf("--- Database ---\n");
    printf("Slices.............%d\n", store_static_config.btree.n_slices);
    printf("Block size.........%ldKB\n", store_static_config.serializer.block_size().ser_value() / KILOBYTE);
    printf("Extent size........%ldKB\n", store_static_config.serializer.extent_size() / KILOBYTE);
    
    const std::vector<log_serializer_private_dynamic_config_t>& private_configs = store_dynamic_config.serializer_private;
    
    for (size_t i = 0; i != private_configs.size(); i++) {
        const log_serializer_private_dynamic_config_t& db_info = private_configs[i];
        printf("File %.2u............%s\n", (uint) i + 1, db_info.db_filename.c_str());
#ifdef SEMANTIC_SERIALIZER_CHECK
        printf("Semantic file %.2u...%s\n", (uint) i + 1, db_info.semantic_filename.c_str());
#endif
    }
}

void cmd_config_t::print_system_spec() {
    printf("--- Hardware ---\n");
    // CPU and RAM
    printf("CPUs...............%d\n" \
           "Total RAM..........%ldMB\nFree RAM...........%ldMB (%.2f%%)\n",
           get_cpu_count(),
           get_total_ram() / 1024 / 1024,
           get_available_ram() / 1024 / 1024,
           (double)get_available_ram() / (double)get_total_ram() * 100.0f);
    // TODO: print CPU topology
    // TODO: print disk and filesystem information
}

void cmd_config_t::print() {
    if(!verbose)
        return;
    
    print_runtime_flags();
    printf("\n");
    print_database_flags();
    printf("\n");
    print_system_spec();
}

cmd_config_t::cmd_config_t() {
    bzero(&replication_config, sizeof(replication_config));
    bzero(&failover_config, sizeof(failover_config));

    verbose = false;
    port = DEFAULT_LISTEN_PORT;
    n_workers = get_cpu_count();
    
    log_file_name[0] = 0;
    log_file_name[MAX_LOG_FILE_NAME - 1] = 0;
    
    store_dynamic_config.serializer.gc_low_ratio = DEFAULT_GC_LOW_RATIO;
    store_dynamic_config.serializer.gc_high_ratio = DEFAULT_GC_HIGH_RATIO;
    store_dynamic_config.serializer.num_active_data_extents = DEFAULT_ACTIVE_DATA_EXTENTS;
    store_dynamic_config.serializer.file_size = 0;   // Unlimited file size
    store_dynamic_config.serializer.file_zone_size = GIGABYTE;
    store_dynamic_config.serializer.read_ahead = false;
    /* #if WE_ARE_ON_LINUX */
    store_dynamic_config.serializer.io_backend = aio_native;
    /* #endif */
    
    store_dynamic_config.cache.max_size = (long long int)(DEFAULT_MAX_CACHE_RATIO * get_available_ram());
    store_dynamic_config.cache.wait_for_flush = false;
    store_dynamic_config.cache.flush_timer_ms = DEFAULT_FLUSH_TIMER_MS;
    store_dynamic_config.cache.max_dirty_size = DEFAULT_UNSAVED_DATA_LIMIT;
    store_dynamic_config.cache.flush_dirty_size = 0;
    store_dynamic_config.cache.flush_waiting_threshold = DEFAULT_FLUSH_WAITING_THRESHOLD;
    store_dynamic_config.cache.max_concurrent_flushes = DEFAULT_MAX_CONCURRENT_FLUSHES;
    
    create_store = false;
    force_create = false;
    shutdown_after_creation = false;
    
    store_static_config.serializer.unsafe_extent_size() = DEFAULT_EXTENT_SIZE;
    store_static_config.serializer.unsafe_block_size() = DEFAULT_BTREE_BLOCK_SIZE;
    
    store_static_config.btree.n_slices = DEFAULT_BTREE_SHARD_FACTOR;

    store_static_config.cache.n_patch_log_blocks = DEFAULT_PATCH_LOG_SIZE / store_static_config.serializer.block_size().ser_value() / store_static_config.btree.n_slices;

    store_static_config.padding = 0;
}

