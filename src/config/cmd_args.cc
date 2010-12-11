
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "config/cmd_args.hpp"
#include "utils.hpp"

void usage(const char *name) {
    // TODO: Make all messages break such that they fit into an 80-characters screen
    printf("Usage:\n");
    printf("\t%s [OPTIONS] [FILE]\n", name);
    
    printf("\nOptions:\n");

    //     "                        24 characters start here.
    printf("  -h, --help            Print these usage options.\n");
    printf("  -v, --verbose         Print extra information to standard output.\n");
    printf("      --create          Create a new database.\n");
    printf("      --force           Used with the --create flag to create a new database\n"
           "                        even if there already is one.\n");

    printf("  -f, --file            Path to file or block device where database goes. Can be\n"
           "                        specified multiple times to use multiple files.\n");
#ifdef SEMANTIC_SERIALIZER_CHECK
    printf("  -S, --semantic-file   Path to the semantic file for the previously specified database file.\n"
           "                        Can only be specified after the path to the database file.\n"
           "                        Default is the name of the database file with '%s' appended.\n", DEFAULT_SEMANTIC_EXTENSION);
#endif

    printf("  -c, --cores           Number of cores to use for handling requests.\n");
    printf("  -m, --max-cache-size  Maximum amount of RAM to use for caching disk\n");
    printf("                        blocks, in megabytes.\n");
    printf("  -l, --log-file        File to log to. If not provided, messages will be printed to stderr.\n");
    printf("  -p, --port            Socket port to listen on. Defaults to %d.\n", DEFAULT_LISTEN_PORT);
    printf("      --wait-for-flush  Do not respond to commands until changes are durable. Expects\n"
           "                        'y' or 'n'.\n");
    printf("      --flush-timer     Time in milliseconds that the server should allow changes to sit\n"
           "                        in memory before flushing it to disk. Pass 'disable' to allow modified data to\n"
           "                        sit in memory indefinitely.\n");
    if (DEFAULT_FLUSH_TIMER_MS == NEVER_FLUSH) {
        printf("                        Defaults to 'disable'.\n");
    } else {
        printf("                        Defaults to %dms.\n", DEFAULT_FLUSH_TIMER_MS);
    }
    printf("      --flush-threshold If more than X%% of the server's maximum cache size is\n"
           "                        modified data, the server will flush it all to disk. Pass 0 to flush\n"
           "                        immediately when changes are made.\n");
    printf("      --gc-range low-high  (e.g. --gc-range 0.5-0.75)\n"
           "                        The proportion of garbage maintained by garbage collection.\n");
    printf("      --active-data-extents\n"
           "                        How many places in the file to write to at once.\n");
    printf("\nOptions for new databases:\n");
    printf("  -s, --slices          Shards total.\n");
    printf("      --block-size      Size of a block, in bytes.\n");
    printf("      --extent-size     Size of an extent, in bytes.\n");
    
    exit(-1);
}

enum {
    wait_for_flush = 256, // Start these values above the ASCII range.
    flush_timer,
    flush_threshold,
    gc_range,
    active_data_extents,
    block_size,
    extent_size,
    create_database,
    force_create
};

cmd_config_t parse_cmd_args(int argc, char *argv[]) {
    parsing_cmd_config_t config;

    // TODO: Overhaul private config parsing!
    std::vector<log_serializer_private_dynamic_config_t>& private_configs = config.store_dynamic_config.serializer_private;
    
    optind = 1; // reinit getopt
    while(1)
    {
        int do_help = 0;
        int do_create_database = 0;
        int do_force_create = 0;
        struct option long_options[] =
            {
                {"wait-for-flush",       required_argument, 0, wait_for_flush},
                {"flush-timer",          required_argument, 0, flush_timer},
                {"flush-threshold",      required_argument, 0, flush_threshold},
                {"gc-range",             required_argument, 0, gc_range},
                {"block-size",           required_argument, 0, block_size},
                {"extent-size",          required_argument, 0, extent_size},
                {"active-data-extents",  required_argument, 0, active_data_extents},
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
                {"create",               no_argument, &do_create_database, 1},
                {"force",                no_argument, &do_force_create, 1},
                {"help",                 no_argument, &do_help, 1},
                {0, 0, 0, 0}
            };

        int option_index = 0;
        int c = getopt_long(argc, argv, "vc:s:f:S:m:l:p:h", long_options, &option_index);

        if (do_help)
            c = 'h';
        if (do_create_database)
            c = create_database;
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
                config.set_flush_threshold(optarg); break;
            case gc_range:
                config.set_gc_range(optarg); break;
            case active_data_extents: 
                config.set_active_data_extents(optarg); break;
            case block_size:
                config.set_block_size(optarg); break;
            case extent_size:
                config.set_extent_size(optarg); break;
            case create_database:
                config.create_store = true;
                config.shutdown_after_creation = true;
                break;
            case force_create:
                config.force_create = true; break;
            case 'h':
                usage(argv[0]);
                break;
         
            default:
                /* getopt_long already printed an error message. */
                usage(argv[0]);
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
    }
    
    /* Sanity-check the input */
    
    if (private_configs.empty()) {
        fail_due_to_user_error("You must explicitly specify one or more paths with -f.");
    }
    
    if (config.store_dynamic_config.cache.wait_for_flush == true &&
        config.store_dynamic_config.cache.flush_timer_ms == NEVER_FLUSH &&
        config.store_dynamic_config.cache.flush_threshold_percent != 0) {
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
        if (parsing_failed || !is_at_most(target, 0))
            fail_due_to_user_error("flush timer should not be negative; use 'disable' to allow changes to sit in memory indefinitely.");
    }
}

void parsing_cmd_config_t::set_extent_size(const char* value) {
    long long int target;
    const long long int minimum_value = 1ll;
    const long long int maximum_value = TERABYTE;
    
    target = parse_longlong(optarg);
    if (parsing_failed || !is_in_range(target, minimum_value, maximum_value))
        fail_due_to_user_error("Extent size must be a number from %d to %d.", minimum_value, maximum_value);
        
    store_static_config.serializer.unsafe_extent_size() = static_cast<long long unsigned int>(target);
}

void parsing_cmd_config_t::set_block_size(const char* value) {
    int target;
    const int minimum_value = 1;
    const int maximum_value = DEVICE_BLOCK_SIZE * 1000;
    
    target = parse_int(optarg);
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
    
    target = parse_int(optarg);
    if (parsing_failed || !is_in_range(target, minimum_value, maximum_value))
        fail_due_to_user_error("Active data extents must be a number from %d to %d.", minimum_value, maximum_value);
    
    store_dynamic_config.serializer.num_active_data_extents = static_cast<unsigned int>(target);
}

void parsing_cmd_config_t::set_gc_range(const char* value) {
    float low = 0.0;
    float high = 0.0;
    int consumed = 0;
    if (2 != sscanf(value, "%f-%f%n", &low, &high, &consumed) || ((size_t)consumed) != strlen(optarg)) {
        fail_due_to_user_error("gc-range expects \"low-high\"");
    }
    if (!(MIN_GC_LOW_RATIO <= low && low < high && high <= MAX_GC_HIGH_RATIO)) {
        fail_due_to_user_error("gc-range expects \"low-high\", with %f <= low < high <= %f",
             MIN_GC_LOW_RATIO, MAX_GC_HIGH_RATIO);
    }
    store_dynamic_config.serializer.gc_low_ratio = low;
    store_dynamic_config.serializer.gc_high_ratio = high;
}

void parsing_cmd_config_t::set_flush_threshold(const char* value) {
    int& target = store_dynamic_config.cache.flush_threshold_percent;
    const int minimum_value = 0;
    const int maximum_value = 99;
    
    target = parse_int(optarg);
    if (parsing_failed || !is_in_range(target, minimum_value, maximum_value))
        fail_due_to_user_error("Flush threshold must be a number from %d to %d.", minimum_value, maximum_value);
}

void parsing_cmd_config_t::set_wait_for_flush(const char* value) {
    if (strlen(optarg) != 1 || !(optarg[0] == 'y' || optarg[0] == 'n'))
        fail_due_to_user_error("Wait-for-flush expects 'y' or 'n'.");
    
    store_dynamic_config.cache.wait_for_flush = optarg[0] == 'y';
}

void parsing_cmd_config_t::set_max_cache_size(const char* value) {
    int int_value = parse_int(optarg);
    if (parsing_failed || !is_positive(int_value))
        fail_due_to_user_error("Cache size must be a positive number.");
    if (is_at_most(int_value, static_cast<int>(get_total_ram() / 1024 / 1024)))
        fail_due_to_user_error("Cache size is larger than this machine's total memory (%s MB).", get_total_ram() / 1024 / 1024);
        
    store_dynamic_config.cache.max_size = (long long)(int_value) * 1024ll * 1024ll;
}

void parsing_cmd_config_t::set_slices(const char* value) {
    int& target = store_static_config.btree.n_slices;
    const int minimum_value = 1;
    const int maximum_value = MAX_SLICES;
    
    target = parse_int(optarg);
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
    
    strncpy(log_file_name, optarg, MAX_LOG_FILE_NAME);
}

void parsing_cmd_config_t::set_port(const char* value) {
    int& target = port;
    const int minimum_value = 0;
    const int maximum_value = 65535;
    
    target = parse_int(optarg);
    if (parsing_failed || !is_in_range(target, minimum_value, maximum_value))
        fail_due_to_user_error("Invalid TCP port (must be a number from %d to %d).", minimum_value, maximum_value);
}

void parsing_cmd_config_t::set_cores(const char* value) {
    int& target = n_workers;
    const int minimum_value = 1;
    // Subtract one because of utility cpu
    const int maximum_value = MAX_CPUS - 1;
    
    target = parse_int(optarg);
    if (parsing_failed || !is_in_range(target, minimum_value, maximum_value))
        fail_due_to_user_error("Number of CPUs must be a number from %d to %d.", minimum_value, maximum_value);
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
    bzero(this, sizeof(*this));
    
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
    
    store_dynamic_config.cache.max_size = DEFAULT_MAX_CACHE_RATIO * get_available_ram();
    store_dynamic_config.cache.wait_for_flush = false;
    store_dynamic_config.cache.flush_timer_ms = DEFAULT_FLUSH_TIMER_MS;
    store_dynamic_config.cache.flush_threshold_percent = DEFAULT_FLUSH_THRESHOLD_PERCENT;
    
    create_store = false;
    force_create = false;
    shutdown_after_creation = false;
    
    store_static_config.serializer.unsafe_extent_size() = DEFAULT_EXTENT_SIZE;
    store_static_config.serializer.unsafe_block_size() = DEFAULT_BTREE_BLOCK_SIZE;
    
    store_static_config.btree.n_slices = DEFAULT_BTREE_SHARD_FACTOR;
}

