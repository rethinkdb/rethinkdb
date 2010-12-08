
#ifndef __ARGS_HPP__
#define __ARGS_HPP__
#include <getopt.h>

/* Usage */
void usage(const char *name) {
    // Create a default config
    config_t _d;
    server_t _d_server;

    // Print usage
    printf("Usage:\n");
    printf("\t%s [OPTIONS]\n", name);

    printf("\nOptions:\n");
    printf("\t-s, --server\n\t\tServer to connect to, in the form [PROTOCOL,]HOSTSTRING. This option\n");
    printf("\t\tcan be specified more than once to distribute clients over several\n");
    printf("\t\tservers. If no servers are specified, defaults to [");
    _d_server.print();
    printf("].\n");
    printf("\t-c, --clients\n\t\tNumber of concurrent clients. Defaults to [%d].\n", _d.clients);
    printf("\t-w, --workload\n\t\tTarget load to generate. Expects a value in format D/U/I/R/A/P/V, where\n" \
           "\t\t\tD - number of deletes\n" \
           "\t\t\tU - number of updates\n" \
           "\t\t\tI - number of inserts\n" \
           "\t\t\tR - number of reads\n" \
           "\t\t\tA - number of appends\n" \
           "\t\t\tP - number of prepends\n" \
           "\t\t\tV - number of verifications\n" \
           "\t\tDefaults to [");
    _d.load.print();
    printf("]\n");
    printf("\t-k, --keys\n\t\tKey distribution in DISTR format (see below). Defaults to [");
    _d.keys.print();
    printf("].\n");
    printf("\t-v, --values\n\t\tValue distribution in DISTR format (see below). Maximum possible\n");
    printf("\t\tvalue size is [%d]. Defaults to [", MAX_VALUE_SIZE);
    _d.values.print();
    printf("].\n");
    printf("\t-d, --duration\n\t\tDuration of the run. Defaults to [");
    _d.duration.print();
    printf("].\n");
    printf("\t-b, --batch-factor\n\t\tA range in DISTR format for average number of reads\n" \
           "\t\tto perform in one shot. Defaults to [");
    _d.batch_factor.print();
    printf("].\n");
    printf("\t-l, --latency-file\n\t\tFile name to output individual latency information (in us).\n" \
           "\t\tThe information is not outputted if this argument is skipped.\n");
    printf("\t-q, --qps-file\n\t\tFile name to output QPS information.\n" \
           "\t\tThe information is not outputted if this argument is skipped.\n");
    printf("\t-o, --out-file\n\t\tIf present, dump all inserted keys to this file.\n");
    printf("\t-i, --in-file\n\t\tIf present, populate initial keys from this file\n"\
           "\t\tand don't drop the database (for relevant protocols).\n");
    printf("\t-f, --db-file\n\t\tIf present drop kv pairs into sqlite and verify correctness on read.\n");

    printf("\nAdditional information:\n");
    printf("\t\tDISTR format describes a range and can be specified in as MIN-MAX.\n\n");
    printf("\t\tPossible protocols are libmemcached, sockmemcached, and mysql. Protocol\n");
    printf("\t\tis optional; if not specified, will default to [");
    _d_server.print_protocol();
    printf("].\n\n");

    printf("\t\tFor memcached protocols the host argument should be in the form host:port.\n");
    printf("\t\tFor mysql protocol the host argument should be in the following\n" \
           "\t\tformat: username/password@host:port+database.\n\n");

    printf("\t\tDuration can be specified as a number of queries (e.g. 5000 or 5000q),\n" \
           "\t\ta number of rows inserted (e.g. 5000i), or a number of seconds (e.g. 5000s).\n");

    exit(-1);
}

/* Parse the args */
void parse(config_t *config, int argc, char *argv[]) {
    optind = 1; // reinit getopt
    while(1)
    {
        int do_help = 0;
        struct option long_options[] =
            {
                {"server",         required_argument, 0, 's'},
                {"clients",        required_argument, 0, 'c'},
                {"workload",       required_argument, 0, 'w'},
                {"keys",           required_argument, 0, 'k'},
                {"values",         required_argument, 0, 'v'},
                {"duration",       required_argument, 0, 'd'},
                {"batch-factor",   required_argument, 0, 'b'},
                {"latency-file",   required_argument, 0, 'l'},
                {"qps-file",       required_argument, 0, 'q'},
                {"out-file",       required_argument, 0, 'o'},
                {"in-file",        required_argument, 0, 'i'},
                {"db-file",        required_argument, 0, 'f'},
                {"help",           no_argument, &do_help, 1},
                {0, 0, 0, 0}
            };

        int option_index = 0;
        int c = getopt_long(argc, argv, "s:n:p:r:c:w:k:v:d:b:l:q:o:i:h:f:", long_options, &option_index);

        if(do_help)
            c = 'h';

        /* Detect the end of the options. */
        if (c == -1)
            break;

        switch (c)
        {
        case 0:
            break;
        case 's': {
            server_t server;
            server.parse(optarg);
            config->servers.push_back(server);
            break;
        }
        case 'c':
            config->clients = atoi(optarg);
            break;
        case 'w':
            config->load.parse(optarg);
            break;
        case 'k':
            config->keys.parse(optarg);
            break;
        case 'v':
            config->values.parse(optarg);
            if (config->values.min > MAX_VALUE_SIZE || config->values.max > MAX_VALUE_SIZE) {
                fprintf(stderr, "Invalid value distribution (maximum value size exceeded).\n");
                exit(-1);
            }
            break;
        case 'd':
            config->duration.parse(optarg);
            break;
        case 'b':
            config->batch_factor.parse(optarg);
            break;
        case 'l':
            strncpy(config->latency_file, optarg, MAX_FILE);
            break;
        case 'q':
            strncpy(config->qps_file, optarg, MAX_FILE);
            break;
        case 'o':
            strncpy(config->out_file, optarg, MAX_FILE);
            break;
        case 'i':
            strncpy(config->in_file, optarg, MAX_FILE);
            break;
        case 'f':
            strncpy(config->db_file, optarg, MAX_FILE);
            break;
        case 'h':
            usage(argv[0]);
            break;

        default:
            /* getopt_long already printed an error message. */
            usage(argv[0]);
        }
    }

    if (config->servers.size() == 0) { // No server specified -- add one to be used as the default.
        config->servers.push_back(server_t());
    }
}

#endif // __ARGS_HPP__

