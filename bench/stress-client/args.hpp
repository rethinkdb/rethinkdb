// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef __STRESS_CLIENT_ARGS_HPP__
#define __STRESS_CLIENT_ARGS_HPP__
#include <getopt.h>

#include "protocol.hpp"

#define MAX_FILE    255
#define MAX_KEY_SIZE (250)
#define MAX_VALUE_SIZE (10*1024*1024)

struct duration_t {
public:
    enum duration_units_t {
        queries_t, seconds_t, inserts_t
    };

public:
    duration_t(long _duration, duration_units_t _units)
        : duration(_duration), units(_units)
        {}

    void print() {
        printf("%ld", duration);
        switch(units) {
        case queries_t:
            printf("q");
            break;
        case seconds_t:
            printf("s");
            break;
        case inserts_t:
            printf("i");
            break;
        default:
            fprintf(stderr, "Unknown duration unit\n");
            exit(-1);
        }
    }

    void parse(char *duration) {
        if (strcmp(duration, "infinity") == 0) {
            this->duration = -1;
            units = queries_t;
        } else {
            int len = strlen(duration);
            switch(duration[len - 1]) {
            case 'q':
                units = queries_t;
                break;
            case 's':
                units = seconds_t;
                break;
            case 'i':
                units = inserts_t;
                break;
            default:
                if(duration[len - 1] >= '0' && duration[len - 1] <= '9')
                    units = queries_t;
                else {
                    fprintf(stderr, "Unknown duration unit\n");
                    exit(-1);
                }
                break;
            }

            this->duration = atol(duration);
        }
    }

public:
    long duration;
    duration_units_t units;
};

/* Defines a load in terms of ratio of deletes, updates, inserts, and
 * reads. */
struct op_ratios_t {
public:
    op_ratios_t()
        : deletes(1), updates(4),
          inserts(8), reads(64),
          appends(0), prepends(0),
          verifies(0), range_reads(0)
        {}

    op_ratios_t(int d, int u, int i, int r, int a, int p, int v, int rr)
        : deletes(d), updates(u),
          inserts(i), reads(r),
          appends(a), prepends(p),
          verifies(v), range_reads(rr)
        {}

    enum load_op_t {
        delete_op, update_op, insert_op, read_op, range_read_op, append_op, prepend_op, verify_op,
    };

    void parse(char *str) {
        char *tok = strtok(str, "/");
        int c = 0;
        while(tok != NULL) {
            switch(c) {
            case 0:
                deletes = atoi(tok);
                break;
            case 1:
                updates = atoi(tok);
                break;
            case 2:
                inserts = atoi(tok);
                break;
            case 3:
                reads = atoi(tok);
                break;
            case 4:
                appends = atoi(tok);
                break;
            case 5:
                prepends = atoi(tok);
                break;
            case 6:
                verifies = atoi(tok);
                break;
            case 7:
                range_reads = atoi(tok);
                break;
            default:
                fprintf(stderr, "Invalid load format (use D/U/I/R/A/P/V/RR)\n");
                exit(-1);
                break;
            }
            tok = strtok(NULL, "/");
            c++;
        }
        if(c < 4) {
            fprintf(stderr, "Invalid load format (use D/U/I/R/A/P/V/RR)\n");
            exit(-1);
        }
    }

    void print() {
        printf("%d/%d/%d/%d/%d/%d/%d/%d", deletes, updates, inserts, reads,appends, prepends, verifies, range_reads);
    }

public:
    int deletes;
    int updates;
    int inserts;
    int reads;
    int range_reads;
    int appends;
    int prepends;
    int verifies;
};

/* Defines a client configuration, including sensible default
 * values.*/
struct config_t {
public:
    config_t()
        : clients(64), duration(10000000L, duration_t::queries_t), op_ratios(op_ratios_t()),
            keys(distr_t(8, 16)), values(distr_t(8, 128)),
            batch_factor(distr_t(1, 16)), range_size(distr_t(16, 128)),
            distr(rnd_uniform_t), mu(1), pipeline_limit(0), ignore_protocol_errors(0)
        {
            latency_file[0] = 0;
            worst_latency_file[0] = 0;
            qps_file[0] = 0;
            out_file[0] = 0;
            in_file[0] = 0;
            db_file[0] = 0;
        }

    void print() {
        printf("---- Workload ----\n");
        printf("Duration..........");
        duration.print();
        printf("\n");
        for (int i = 0; i < servers.size(); i++) {
            printf("Server............");
            servers[i].print();
            printf("\n");
        }
        printf("Clients...........%d\n", clients);
        printf("Load..............");
        op_ratios.print();
        printf("\nKeys..............");
        keys.print();
        printf("\nValues............");
        values.print();
        printf("\nBatch factor......");
        batch_factor.print();
        printf("\nRange size........");
        range_size.print();
        printf("\nDistribution......");
        if(distr == rnd_uniform_t)
            printf("uniform\n");
        if(distr == rnd_normal_t) {
            printf("normal\n");
            printf("MU................%d\n", mu);
        }
        // Adding one because users are 1-based, unlike our code for
        // pipelines, which is 0-based
        printf("Pipeline-limit....%d\n", pipeline_limit + 1);
        printf("\n");
    }

public:
    std::vector<server_t> servers;
    int clients;
    duration_t duration;
    op_ratios_t op_ratios;
    distr_t keys;
    std::string key_prefix;
    distr_t values;
    distr_t batch_factor;
    distr_t range_size;
    rnd_distr_t distr;
    int mu;
    char latency_file[MAX_FILE];
    char worst_latency_file[MAX_FILE];
    char qps_file[MAX_FILE];
    char out_file[MAX_FILE];
    char in_file[MAX_FILE];
    char db_file[MAX_FILE];
    int pipeline_limit;
    int ignore_protocol_errors;
};

/* List supported protocols. */
void list_protocols() {
    // I'll just cheat here.
    printf("rethinkdb");
    printf("sockmemcached,");
#ifdef USE_MYSQL
    printf("mysql,");
#endif
#ifdef USE_LIBMEMCACHED
    printf("libmemcached,");
#endif
    printf("sqlite");
}

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
    printf("\t--client-suffix\n\t\tAppend a per-client id to key names.\n");
    printf("\t--ignore-protocol-errors\n\t\tDo not quit if the protocol throws errors.\n");
    printf("\t-w, --workload\n\t\tTarget load to generate. Expects a value in format D/U/I/R/A/P/V/RR, where\n" \
           "\t\t\tD - number of deletes\n" \
           "\t\t\tU - number of updates\n" \
           "\t\t\tI - number of inserts\n" \
           "\t\t\tR - number of reads\n" \
           "\t\t\tA - number of appends\n" \
           "\t\t\tP - number of prepends\n" \
           "\t\t\tV - number of verifications\n" \
           "\t\t\tRR - number of range reads\n" \
           "\t\tDefaults to [");
    _d.op_ratios.print();
    printf("]\n");
    printf("\t-k, --keys\n\t\tKey distribution in DISTR format (see below). Defaults to [");
    _d.keys.print();
    printf("].\n");
    printf("\t-K, --keys-prefix\n\t\tPrefix every key with the following string. Defaults to [");
    printf("%s", _d.key_prefix.c_str());
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
    printf("\t-R, --range-size\n\t\tA range in DISTR format for average number of values\n" \
           "\t\tto retrieve per range get. Defaults to [");
    _d.range_size.print();
    printf("].\n");
    printf("\t-l, --latency-file\n\t\tFile name to output individual latency information (in us).\n" \
           "\t\tThe information is not outputted if this argument is skipped.\n");
    printf("\t-L, --worst-latency-file\n\t\tFile name to output worst latency each second.\n" \
           "\t\tThe information is not outputted if this argument is skipped.\n");
    printf("\t-q, --qps-file\n\t\tFile name to output QPS information. '-' for stdout.\n" \
           "\t\tThe information is not outputted if this argument is skipped.\n");
    printf("\t-o, --out-file\n\t\tIf present, dump all inserted keys to this file.\n");
    printf("\t-i, --in-file\n\t\tIf present, populate initial keys from this file\n"\
           "\t\tand don't drop the database (for relevant protocols).\n");
    printf("\t-f, --db-file\n\t\tIf present drop kv pairs into sqlite and verify correctness on read.\n");
    printf("\t-r, --distr\n\t\tA key access distrubution. Possible values: 'uniform' (default), and 'normal'.\n");
    printf("\t-m, --mu\n\t\tControl normal distribution. Percent of the database size within one standard\n\t\tdistribution (defaults to 1%%).\n");
    printf("\t-p, --pipeline\n\t\tMaximum number of operations that may be queued to server (defaults to 1).\n");

    printf("\nAdditional information:\n");
    printf("\t\tDISTR format describes a range and can be specified in as NUM or MIN-MAX.\n\n");
    printf("\t\tPossible protocols are [");
    list_protocols();
    printf("]. Protocol\n");
    printf("\t\tis optional; if not specified, will default to [");
    _d_server.print_protocol();
    printf("].\n\n");

    printf("\t\tFor memcached and rethinkdb protocols the host argument should be in the form host:port.\n");
#ifdef USE_MYSQL
    printf("\t\tFor mysql protocol the host argument should be in the following\n" \
           "\t\tformat: username/password@host:port+database.\n\n");
#endif

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
                {"server",             required_argument, 0, 's'},
                {"clients",            required_argument, 0, 'c'},
                {"workload",           required_argument, 0, 'w'},
                {"keys",               required_argument, 0, 'k'},
                {"keys-prefix",        required_argument, 0, 'K'},
                {"values",             required_argument, 0, 'v'},
                {"duration",           required_argument, 0, 'd'},
                {"batch-factor",       required_argument, 0, 'b'},
                {"range-size",         required_argument, 0, 'R'},
                {"latency-file",       required_argument, 0, 'l'},
                {"worst-latency-file", required_argument, 0, 'L'},
                {"qps-file",           required_argument, 0, 'q'},
                {"out-file",           required_argument, 0, 'o'},
                {"in-file",            required_argument, 0, 'i'},
                {"db-file",            required_argument, 0, 'f'},
                {"distr",              required_argument, 0, 'r'},
                {"mu",                 required_argument, 0, 'm'},
                {"pipeline",           required_argument, 0, 'p'},
                {"client-suffix",      no_argument, 0, 'a'},
                {"ignore-protocol-errors", no_argument, &config->ignore_protocol_errors, 1},
                {"help",               no_argument, &do_help, 1},
                {0, 0, 0, 0}
            };

        int option_index = 0;
        int c = getopt_long(argc, argv, "s:n:p:r:c:w:k:K:v:d:b:R:l:L:q:o:i:h:f:m:", long_options, &option_index);

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
            config->op_ratios.parse(optarg);
            break;
        case 'k':
            config->keys.parse(optarg);
            break;
        case 'K':
            config->key_prefix = optarg;
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
        case 'R':
            config->range_size.parse(optarg);
            break;
        case 'l':
            strncpy(config->latency_file, optarg, MAX_FILE);
            break;
        case 'L':
            strncpy(config->worst_latency_file, optarg, MAX_FILE);
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
        case 'a':
            fprintf(stderr, "Warning: The \"--client-suffix\" (alias \"-a\") flag is deprecated. "
                "Per-client suffixes are always enabled now.\n");
            break;
        case 'r':
            if(strcmp(optarg, "uniform") == 0) {
                config->distr = rnd_uniform_t;
            } else if(strcmp(optarg, "normal") == 0) {
                config->distr = rnd_normal_t;
            }
            break;
        case 'm':
            config->mu = atoi(optarg);
            break;
        case 'p':
            config->pipeline_limit = atoi(optarg);
            if(config->pipeline_limit < 1) {
                fprintf(stderr, "Minimum pipeline value is 1.\n");
                usage(argv[0]);
            }
            // the code is structured in a way where zero means no pipelining, so we subtract one
            config->pipeline_limit--;
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

    //validation:
    bool only_sockmemcached = true;
    for(int i = 0; i < config->servers.size(); i++) {
        if(config->servers[i].protocol != protocol_sockmemcached) {
            only_sockmemcached = false;
            break;
        }
    }
    if (config->pipeline_limit > 0) {
        if (config->op_ratios.deletes > 0 ||
            config->op_ratios.updates > 0 ||
            config->op_ratios.inserts > 0 ||
            config->op_ratios.range_reads > 0 ||
            config->op_ratios.appends > 0 ||
            config->op_ratios.prepends > 0 ||
            config->op_ratios.verifies > 0 ||
            !only_sockmemcached)
        {
            fprintf(stderr, "Pipelining can only be used with read operations on a sockmemcached protocol.\n");
            usage(argv[0]);
        }
    }
}

#endif  // __STRESS_CLIENT_ARGS_HPP__

