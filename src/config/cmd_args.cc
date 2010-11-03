
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config/cmd_args.hpp"
#include "utils.hpp"

void usage(const char *name) {
    printf("Usage:\n");
    printf("\t%s [OPTIONS] [FILE]\n", name);
    
    printf("\nOptions:\n");
    //     "                        24 characters start here.
    printf("  -h, --help            Print these usage options.\n");
    printf("      --create          Create a new database.\n");
    printf("  -f, --file            Path to file or block device where database goes. Can be\n"
           "                        specified multiple times to use multiple files.\n");
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
    }
    else {
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

void init_config(cmd_config_t *config) {

    bzero(config, sizeof(*config));
    
    config->port = DEFAULT_LISTEN_PORT;
    config->n_workers = get_cpu_count();
    
    config->log_file_name[0] = 0;
    config->log_file_name[MAX_LOG_FILE_NAME - 1] = 0;
    
    config->n_files = 0;
    
    config->store_dynamic_config.serializer.gc_low_ratio = DEFAULT_GC_LOW_RATIO;
    config->store_dynamic_config.serializer.gc_high_ratio = DEFAULT_GC_HIGH_RATIO;
    config->store_dynamic_config.serializer.num_active_data_extents = DEFAULT_ACTIVE_DATA_EXTENTS;
    config->store_dynamic_config.serializer.file_size = 0;   // Unlimited file size
    config->store_dynamic_config.serializer.file_zone_size = GIGABYTE;
    
    config->store_dynamic_config.cache.max_size = DEFAULT_MAX_CACHE_RATIO * get_available_ram();
    config->store_dynamic_config.cache.wait_for_flush = false;
    config->store_dynamic_config.cache.flush_timer_ms = DEFAULT_FLUSH_TIMER_MS;
    config->store_dynamic_config.cache.flush_threshold_percent = DEFAULT_FLUSH_THRESHOLD_PERCENT;
    
    config->create_store = false;
    
    config->store_static_config.serializer.extent_size = DEFAULT_EXTENT_SIZE;
    config->store_static_config.serializer.block_size = DEFAULT_BTREE_BLOCK_SIZE;
    
    config->store_static_config.btree.n_slices = DEFAULT_BTREE_SHARD_FACTOR;
}

enum {
    wait_for_flush = 256, // Start these values above the ASCII range.
    flush_timer,
    flush_threshold,
    gc_range,
    active_data_extents,
    block_size,
    extent_size,
    create_database
};

void parse_cmd_args(int argc, char *argv[], cmd_config_t *config)
{
    init_config(config);
    
    optind = 1; // reinit getopt
    while(1)
    {
        int do_help = 0;
        int do_create_database = 0;
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
                {"max-cache-size",       required_argument, 0, 'm'},
                {"log-file",             required_argument, 0, 'l'},
                {"port",                 required_argument, 0, 'p'},
                {"create",               no_argument, &do_create_database, 1},
                {"help",                 no_argument, &do_help, 1},
                {0, 0, 0, 0}
            };

        int option_index = 0;
        int c = getopt_long(argc, argv, "c:s:f:m:l:p:h", long_options, &option_index);

        if (do_help)
            c = 'h';
        if (do_create_database)
            c = create_database;
     
        /* Detect the end of the options. */
        if (c == -1)
            break;
     
        switch (c)
        {
        case 0:
            break;
        case 'p':
            config->port = atoi(optarg);
            break;
        case 'l':
            strncpy(config->log_file_name, optarg, MAX_LOG_FILE_NAME);
            break;
        case 'c':
            config->n_workers = atoi(optarg);
            // Subtract one because of utility cpu
            if(config->n_workers > MAX_CPUS - 1) {
                fail("Maximum number of CPUs is %d\n", MAX_CPUS - 1);
                abort();
            }
            break;
        case 's':
            config->store_static_config.btree.n_slices = atoi(optarg);
            if(config->store_static_config.btree.n_slices > MAX_SLICES) {
                fail("Maximum number of slices is %d\n", MAX_SLICES);
            }
            break;
        case 'f':
            if (config->n_files >= MAX_SERIALIZERS) {
                fail("Cannot use more than %d files.", MAX_SERIALIZERS);
            }
            config->files[config->n_files] = optarg;
            config->n_files++;
            break;
        case 'm':
            config->store_dynamic_config.cache.max_size = atoll(optarg) * 1024 * 1024;
            break;
        case wait_for_flush:
        	if (strcmp(optarg, "y")==0) config->store_dynamic_config.cache.wait_for_flush = true;
        	else if (strcmp(optarg, "n")==0) config->store_dynamic_config.cache.wait_for_flush = false;
        	else fail("wait-for-flush expects 'y' or 'n'");
            break;
        case flush_timer:
            if (strcmp(optarg, "disable")==0) config->store_dynamic_config.cache.flush_timer_ms = NEVER_FLUSH;
            else {
                config->store_dynamic_config.cache.flush_timer_ms = atoi(optarg);
                check("flush timer should not be negative; use 'disable' to allow changes "
                    "to sit in memory indefinitely",
                    config->store_dynamic_config.cache.flush_timer_ms < 0);
            }
            break;
        case flush_threshold:
            config->store_dynamic_config.cache.flush_threshold_percent = atoi(optarg);
            break;
        case gc_range: {
            float low = 0.0;
            float high = 0.0;
            int consumed = 0;
            if (3 != sscanf(optarg, "%f-%f%n", &low, &high, &consumed) || ((size_t)consumed) != strlen(optarg)) {
                usage(argv[0]);
            }
            if (!(MIN_GC_LOW_RATIO <= low && low < high && high <= MAX_GC_HIGH_RATIO)) {
                fail("gc-range expects \"low-high\", with %f <= low < high <= %f",
                     MIN_GC_LOW_RATIO, MAX_GC_HIGH_RATIO);
            }
            config->store_dynamic_config.serializer.gc_low_ratio = low;
            config->store_dynamic_config.serializer.gc_high_ratio = high;
            break;
        }
        case active_data_extents:
            config->store_dynamic_config.serializer.num_active_data_extents = atoi(optarg);
            if (config->store_dynamic_config.serializer.num_active_data_extents < 1 ||
                config->store_dynamic_config.serializer.num_active_data_extents > MAX_ACTIVE_DATA_EXTENTS) {
                fail("--active-data-extents must be less than or equal to %d", MAX_ACTIVE_DATA_EXTENTS);
            }
            break;
        case block_size:
            config->store_static_config.serializer.block_size = atoi(optarg);
            if (config->store_static_config.serializer.block_size % DEVICE_BLOCK_SIZE != 0) {
                fail("--block-size must be a multiple of %d", DEVICE_BLOCK_SIZE);
            }
            if (config->store_static_config.serializer.block_size <= 0 || config->store_static_config.serializer.block_size > DEVICE_BLOCK_SIZE * 1000) {
                fail("--block-size value is not reasonable.");
            }
            break;
        case extent_size:
            config->store_static_config.serializer.extent_size = atoi(optarg);
            if (config->store_static_config.serializer.extent_size <= 0 || config->store_static_config.serializer.extent_size > TERABYTE) {
                fail("--extent-size value is not reasonable.");
            }
            break;
        case create_database:
            config->create_store = true;
            break;
        case 'h':
            usage(argv[0]);
            break;
     
        default:
            /* getopt_long already printed an error message. */
            usage(argv[0]);
        }
    }

    if (optind < argc) {
        fail("Unexpected extra argument: \"%s\"", argv[optind]);
    }
    
    /* "Idiot mode" -- do something reasonable for novice users */
    
    if (config->n_files == 0 && !config->create_store) {        
        
        config->n_files = 1;
        config->files[0] = DEFAULT_DB_FILE_NAME;
        
        int res = access(DEFAULT_DB_FILE_NAME, F_OK);
        if (res == 0) {
            /* Found a database file -- try to load it */
            fprintf(stderr, "Database file was not specified explicitly; loading from \"%s\" by default.\n", DEFAULT_DB_FILE_NAME);
            config->create_store = false;   // This is redundant
        } else if (res == -1 && errno == ENOENT) {
            /* Create a new database */
            fprintf(stderr, "Database file was not specified explicitly; creating \"%s\" by default.\n", DEFAULT_DB_FILE_NAME);
            config->create_store = true;
        } else {
            fail("Could not access() path \"%s\": %s", DEFAULT_DB_FILE_NAME, strerror(errno));
        }
    }
    
    /* Sanity-check the input */
    
    if (config->n_files == 0) {
        fail("You must explicitly specify one or more paths with -f.");
    }
    
    if (config->store_dynamic_config.cache.wait_for_flush == true &&
        config->store_dynamic_config.cache.flush_timer_ms == NEVER_FLUSH &&
        config->store_dynamic_config.cache.flush_threshold_percent != 0) {
    	printf("WARNING: Server is configured to wait for data to be flushed\n"
               "to disk before returning, but also configured to wait\n"
               "indefinitely before flushing data to disk. Setting wait-for-flush\n"
               "to 'no'.\n\n");
    	config->store_dynamic_config.cache.wait_for_flush = false;
    }
    
    if (config->store_static_config.serializer.extent_size % config->store_static_config.serializer.block_size != 0) {
        fail("Extent size (%d) is not a multiple of block size (%d).", 
            config->store_static_config.serializer.extent_size,
            config->store_static_config.serializer.block_size);
    }
}

