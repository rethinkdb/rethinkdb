
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config/cmd_args.hpp"
#include "config/code.hpp"
#include "utils.hpp"

static const char* default_db_file_name = "db_data/data.file";

void usage(const char *name) {
    printf("Usage:\n");
    printf("\t%s [OPTIONS] [FILE]\n", name);

    printf("\nArguments:\n");
    
    printf("  FILE\t\t\tDevice or file name to store the database file. If the\n");
    printf("\t\t\tname isn't provided, %s will use '%s' by\n", name, default_db_file_name);
    printf("\t\t\tdefault.\n");
    
    printf("\nOptions:\n");
    
    printf("  -h, --help\t\tPrint these usage options.\n");
    printf("  -c, --max-cores\tDo not use more than this number of cores for\n");
    printf("\t\t\thandling user requests.\n");

    printf("  -s, --slices\tShards per thread\n");
    
    printf("  -m, --max-cache-size\tMaximum amount of RAM to use for caching disk\n");
    printf("\t\t\tblocks, in megabytes.\n");
    
    printf("  -l, --log-file\tFile to log to. If not provided, messages will be printed to stderr.\n");
    printf("  -p, --port\t\tSocket port to listen on. Defaults to %d.\n", DEFAULT_LISTEN_PORT);
    printf("      --wait-for-flush\tDo not respond to commands until changes are durable. Expects\n"
            "\t\t\t'y' or 'n'.\n");
    printf("      --flush-timer\tTime in milliseconds that the server should allow changes to sit\n"
            "\t\t\tin memory before flushing it to disk. Pass 'disable' to allow modified data to\n"
            "\t\t\tsit in memory indefinitely.\n");
    if (DEFAULT_FLUSH_TIMER_MS == NEVER_FLUSH) printf("\t\t\tDefaults to 'disable'.\n");
    else printf("\t\t\tDefaults to %dms.\n", DEFAULT_FLUSH_TIMER_MS);
    printf("      --flush-threshold\tIf more than X%% of the server's maximum cache size is\n"
            "\t\t\tmodified data, the server will flush it all to disk. Pass 0 to flush\n"
            "\t\t\timmediately when changes are made.\n");
    
    exit(-1);
}

void init_config(cmd_config_t *config) {
    bzero(config, sizeof(*config));
    
    // Initialize default database name
    strncpy(config->db_file_name, default_db_file_name, MAX_DB_FILE_NAME);
    config->db_file_name[MAX_DB_FILE_NAME - 1] = 0;

    config->log_file_name[0] = 0;
    config->log_file_name[MAX_LOG_FILE_NAME - 1] = 0;

    config->max_cache_size = DEFAULT_MAX_CACHE_RATIO * get_available_ram();
    config->port = DEFAULT_LISTEN_PORT;

    config->wait_for_flush = false;
    config->flush_timer_ms = DEFAULT_FLUSH_TIMER_MS;
    config->flush_threshold_percent = DEFAULT_FLUSH_THRESHOLD_PERCENT;
}

enum {
    wait_for_flush = 256, // Start these values above the ASCII range.
    flush_timer,
    flush_threshold
};

void parse_cmd_args(int argc, char *argv[], cmd_config_t *config)
{
    init_config(config);
    
    optind = 1; // reinit getopt
    while(1)
    {
        int do_help = 0;
        struct option long_options[] =
            {
                {"wait-for-flush",       required_argument, 0, wait_for_flush},
                {"flush-timer",          required_argument, 0, flush_timer},
                {"flush-threshold",      required_argument, 0, flush_threshold},
                {"max-cores",            required_argument, 0, 'c'},
                {"slices",               required_argument, 0, 's'},
                {"max-cache-size",       required_argument, 0, 'm'},
                {"log-file",             required_argument, 0, 'l'},
                {"port",                 required_argument, 0, 'p'},
                {"help",                 no_argument, &do_help, 1},
                {0, 0, 0, 0}
            };

        int option_index = 0;
        int c = getopt_long(argc, argv, "c:s:m:l:p:h", long_options, &option_index);

        if(do_help)
            c = 'h';
     
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
            break;
        case 's':
            config->n_slices = atoi(optarg);
        case 'm':
            config->max_cache_size = atoi(optarg) * 1024 * 1024;
            break;
        case wait_for_flush:
        	if (strcmp(optarg, "y")==0) config->wait_for_flush = true;
        	else if (strcmp(optarg, "n")==0) config->wait_for_flush = false;
        	else check("wait-for-flush expects 'y' or 'n'", 1);
            break;
        case flush_timer:
            if (strcmp(optarg, "disable")==0) config->flush_timer_ms = NEVER_FLUSH;
            else {
                config->flush_timer_ms = atoi(optarg);
                check("flush timer should not be negative; use 'disable' to allow changes"
                    "to sit in memory indefinitely",
                    config->flush_timer_ms < 0);
                check("flush timer of 0 is broken at the moment",
                    config->flush_timer_ms == 0);
            }
            break;
        case flush_threshold:
            config->flush_threshold_percent = atoi(optarg);
            break;
        case 'h':
            usage(argv[0]);
            break;
     
        default:
            /* getopt_long already printed an error message. */
            usage(argv[0]);
        }
    }

    // Grab the db name
    if (optind < argc) {
        strncpy(config->db_file_name, argv[optind++], MAX_DB_FILE_NAME);
        config->db_file_name[MAX_DB_FILE_NAME - 1] = 0;
    }
    
    if (config->wait_for_flush == true &&
        config->flush_timer_ms == NEVER_FLUSH &&
        config->flush_threshold_percent != 0) {
    	printf("WARNING: Server is configured to wait for data to be flushed\n"
               "to disk before returning, but also configured to wait\n"
               "indefinitely before flushing data to disk. Setting wait-for-flush\n"
               "to 'no'.\n\n");
    	config->wait_for_flush = false;
    }
}

