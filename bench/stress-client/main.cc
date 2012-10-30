// Copyright 2010-2012 RethinkDB, all rights reserved.

#include <map>
#include <unistd.h>
#include <getopt.h>
#include <stdio.h>
#include <vector>
#include <stdio.h>
#include <sys/stat.h>
#include "args.hpp"
#include "client.hpp"
#include "ops/consecutive_seed_model.hpp"
#include "ops/simple_ops.hpp"
#include "ops/range_read_ops.hpp"
#include "ops/sqlite_mirror.hpp"
#ifdef USE_MYSQL
#include "protocols/mysql_protocol.hpp"   // For initialize_mysql_table()
#endif

using namespace std;

FILE *get_out_file(const char *name, const char *what) {
    if (strcmp(name, "-") == 0) {
        return stdout;
    } else if (name[0] != '\0') {
        FILE *f = fopen(name, "wa");
        if (!f) {
            fprintf(stderr, "Warning: Could not open \"%s\". Will not record %s.\n", name, what);
        }
        return f;
    } else {
        return NULL;
    }
}

#define BACKUP_FOLDER "sqlite_backup"

/* Tie it all together */
int main(int argc, char *argv[])
{
    // Initialize randomness
    srand(time(NULL));

    // Parse the arguments
    config_t config;
    parse(&config, argc, argv);
    config.print();

    /* Open output files */
    FILE *qps_fd = get_out_file(config.qps_file, "QPS");
    FILE *latencies_fd = get_out_file(config.latency_file, "latencies");
    FILE *worst_latencies_fd = get_out_file(config.worst_latency_file, "worst latencies");

    /* make a directory for our sqlite files */
    if (config.db_file[0]) {
        mkdir(BACKUP_FOLDER, 0777);
    }

#ifdef USE_MYSQL
    /* Initialize any MySQL servers */
    for (int i = 0; i < config.servers.size(); i++) {
        if (config.servers[i].protocol == protocol_mysql) {
            initialize_mysql_table(config.servers[i].host,
                config.keys.max, config.values.max);
        }
    }
#endif

    /* Create client objects */

    struct client_stuff_t {

        protocol_t *protocol;

        seed_key_generator_t kg;

        consecutive_seed_model_t model;

        sqlite_protocol_t *sqlite;
        sqlite_mirror_t sqlite_mirror;

        consecutive_seed_model_t::insert_chooser_t insert_chooser;
        insert_op_generator_t insert_op_generator;

        consecutive_seed_model_t::delete_chooser_t delete_chooser;
        delete_op_generator_t delete_op_generator;

        consecutive_seed_model_t::live_chooser_t live_chooser;
        read_op_generator_t read_op_generator;
        update_op_generator_t update_op_generator;
        append_prepend_op_generator_t append_op_generator;
        append_prepend_op_generator_t prepend_op_generator;

        sqlite_mirror_verify_op_generator_t verify_op_generator;

        percentage_range_read_op_generator_t range_read_op_generator;

        client_t client;

        static sqlite_protocol_t *make_sqlite_if_necessary(config_t *config, int i) {
            if (config->db_file[0]) {
                char buffer[2048];
                sprintf(buffer, "%s/%d_%s", BACKUP_FOLDER, i, config->db_file);
                return new sqlite_protocol_t(buffer);
            } else {
                return NULL;
            }
        }

        client_stuff_t(config_t *config, int i) :

            /* Connect to the server */
            protocol(config->servers[i % config->servers.size()].connect()),

            /* Make a key-generator so we can translate from seeds to keys */
            kg(i, config->clients, config->key_prefix, config->keys),

            /* Set up the consecutive-seed model, which keeps track of which keys exist */
            model(),

            /* Set up the mirroring to a SQLite database for verification */
            sqlite(make_sqlite_if_necessary(config, i)),
            sqlite_mirror(&kg, &model, &model, sqlite),

            /* Set up the various operations */
            insert_chooser(&model),
            insert_op_generator(config->pipeline_limit + 1, &kg, &insert_chooser, &sqlite_mirror, protocol, config->values),

            delete_chooser(&model),
            delete_op_generator(config->pipeline_limit + 1, &kg, &delete_chooser, &sqlite_mirror, protocol),

            live_chooser(&model, config->distr, config->mu),
            read_op_generator(config->pipeline_limit + 1, &kg, &live_chooser, protocol, config->batch_factor),
            update_op_generator(config->pipeline_limit + 1, &kg, &live_chooser, &sqlite_mirror, protocol, config->values),
            append_op_generator(config->pipeline_limit + 1, &kg, &live_chooser, &sqlite_mirror, protocol, true, config->values),
            prepend_op_generator(config->pipeline_limit + 1, &kg, &live_chooser, &sqlite_mirror, protocol, false, config->values),

            verify_op_generator(config->pipeline_limit + 1, &sqlite_mirror, protocol),

            range_read_op_generator(config->pipeline_limit + 1, protocol, distr_t(50, 50), config->range_size, config->key_prefix),

            /* Construct the client object */
            client(config->pipeline_limit, config->ignore_protocol_errors)
        {
            int expected_batch_factor = (config->batch_factor.min + config->batch_factor.max) / 2;

            /* We multiply the ratio by expected_batch_factor to get nicer rounding for reads (instead of dividing the reads frequency) */
            client.add_op(config->op_ratios.inserts * expected_batch_factor, &insert_op_generator);

            client.add_op(config->op_ratios.deletes * expected_batch_factor, &delete_op_generator);

            client.add_op(config->op_ratios.reads, &read_op_generator);
            client.add_op(config->op_ratios.updates * expected_batch_factor, &update_op_generator);
            client.add_op(config->op_ratios.appends * expected_batch_factor, &append_op_generator);
            client.add_op(config->op_ratios.prepends * expected_batch_factor, &prepend_op_generator);

            client.add_op(config->op_ratios.verifies * expected_batch_factor, &verify_op_generator);

            client.add_op(config->op_ratios.range_reads * expected_batch_factor, &range_read_op_generator);
        }

        ~client_stuff_t() {
            if (sqlite) delete sqlite;
            delete protocol;
        }
    };

    client_stuff_t *clients[config.clients];

    for (int i = 0; i < config.clients; i++) {
        clients[i] = new client_stuff_t(&config, i);

        /* Collecting latency samples is a potentially expensive operation, so we disable it
        if the user does not ask for them. */
        if (latencies_fd == NULL) {
            for (int j = 0; j < (int)clients[i]->client.ops.size(); j++) {
                clients[i]->client.ops[j]->query_stats.set_enable_latency_samples(false);
            }
        }
    }

    // If input keys are provided, read them in
    if(config.in_file[0] != 0) {
        FILE *in_file = fopen(config.in_file, "r");

        if(in_file == NULL) {
            fprintf(stderr, "Could not open output key file\n");
            exit(-1);
        }
        
        int check_clients = 0;
        size_t res __attribute__((unused));
        res = fread(&check_clients, sizeof(check_clients), 1, in_file);
        
        if (check_clients != config.clients) {
            fprintf(stderr, "Client number mismatch. Input file is for %d clients, attempted to run with %d.\n", check_clients, config.clients);
            exit(-1);
        }

        while(feof(in_file) == 0) {
            int id;
            seed_t min_seed, max_seed;
            res = fread(&id, sizeof(id), 1, in_file);
            res = fread(&min_seed, sizeof(min_seed), 1, in_file);
            res = fread(&max_seed, sizeof(max_seed), 1, in_file);

            clients[id]->model.min_seed = min_seed;
            clients[id]->model.max_seed = max_seed;
        }

        fclose(in_file);
    }

    // Start everything running
    for(int i = 0; i < config.clients; i++) {
        clients[i]->client.start();
    }

    /* The main loop polls the threads for stats once a second, and issues the order to stop when
    it decides it is time. */

    ticks_t start_time = get_ticks();

    query_stats_t total_stats;
    int total_time = 0, total_inserts_minus_deletes = 0;

    // TODO: If an workload contains contains no inserts and there are no keys available for a
    // particular client (and the duration is specified in q/i), it'll just loop forever.

    bool keep_running = true;
    while(keep_running) {

        /* Delay an integer number of seconds, preferably 1. */
        int round_time = 1;
        ticks_t now = get_ticks();
        while (now > start_time + (total_time + round_time) * secs_to_ticks(1)) {
            round_time++;
        }
        sleep_ticks(start_time + (total_time + round_time) * secs_to_ticks(1) - now);

        /* Poll all the clients for stats. We mix the results from all the clients together;
        if you want to get stats for individual clients separately, use the Python
        interface. */
        query_stats_t round_stats;
        int round_inserts_minus_deletes = 0;
        for(int i = 0; i < config.clients; i++) {
            client_stuff_t *c = clients[i];

            for (int j = 0; j < (int)c->client.ops.size(); j++) {
                c->client.ops[j]->query_stats.lock.lock();
            }

            /* We pool all the stats from different operations together into one pool for
            reporting. If you want to get stats for individual operations separately, use
            the Python interface. */
            for (int j = 0; j < (int)c->client.ops.size(); j++) {
                round_stats.aggregate(c->client.ops[j]->query_stats);
            }

            /* Count total number of keys inserted and deleted (we will use this if our
            duration is specified in inserts) */
            round_inserts_minus_deletes += c->insert_op_generator.query_stats.queries - c->delete_op_generator.query_stats.queries;

            /* Reset the stats, so that every time we poll we only get the stats from
            since we last polled. */
            for (int j = 0; j < (int)c->client.ops.size(); j++) {
                c->client.ops[j]->query_stats.reset();
            }

            for (int j = 0; j < (int)c->client.ops.size(); j++) {
                c->client.ops[j]->query_stats.lock.unlock();
            }
        }

        /* Report the stats we got from the clients */
        if (qps_fd) {
            fprintf(qps_fd, "%d\t\t%d\n", total_time, round_stats.queries / round_time);
            fflush(qps_fd);
        }
        if (latencies_fd) {
            for (int i = 0; i < round_stats.latency_samples.size(); i++) {
                fprintf(latencies_fd, "%d\t\t%.2f\n",
                    total_time,
                    ticks_to_us(round_stats.latency_samples.samples[i]));
            }
            fflush(latencies_fd);
        }
        if (worst_latencies_fd) {
            fprintf(worst_latencies_fd, "worst-latency\t%d\t%.6f\n",
                total_time, ticks_to_secs(round_stats.worst_latency));
            fflush(worst_latencies_fd);
        }

        /* Update the aggregate total stats, and check if we are done running */
        total_stats.aggregate(round_stats);
        total_inserts_minus_deletes += round_inserts_minus_deletes;
        total_time += round_time;

        if (config.duration.duration != -1) {
            switch (config.duration.units) {
            case duration_t::queries_t:
                keep_running = total_stats.queries < config.duration.duration;
                break;
            case duration_t::seconds_t:
                keep_running = total_time < config.duration.duration;
                break;
            case duration_t::inserts_t:
                keep_running = total_inserts_minus_deletes < config.duration.duration;
                break;
            default:
                fprintf(stderr, "Unknown duration unit\n");
                exit(-1);
            }
        }
    }

    // Stop the threads
    for (int i = 0; i < config.clients; i++) {
        clients[i]->client.stop();
    }

    /* Collect information about the last few operations performed, then print totals. */
    for (int i = 0; i < config.clients; i++) {
        client_stuff_t *c = clients[i];
        for (int j = 0; j < (int)c->client.ops.size(); j++) {
            total_stats.aggregate(c->client.ops[j]->query_stats);
        }
        total_inserts_minus_deletes += c->insert_op_generator.query_stats.queries - c->delete_op_generator.query_stats.queries;
    }

    printf("Total running time: %f seconds\n", ticks_to_secs(get_ticks() - start_time));
    printf("Total operations: %d\n", total_stats.queries);
    printf("Total keys inserted minus keys deleted: %d\n", total_inserts_minus_deletes);

    // Dump key vectors if we have an out file
    if(config.out_file[0] != 0) {
        FILE *out_file = fopen(config.out_file, "w");

        fwrite(&config.clients, sizeof(config.clients), 1, out_file);

        // Dump the keys
        for(int i = 0; i < config.clients; i++) {
            fwrite(&i, sizeof(i), 1, out_file);
            fwrite(&clients[i]->model.min_seed, sizeof(clients[i]->model.min_seed), 1, out_file);
            fwrite(&clients[i]->model.max_seed, sizeof(clients[i]->model.max_seed), 1, out_file);
        }

        fclose(out_file);
    }

    // Clean up
    for(int i = 0; i < config.clients; i++) {
        delete clients[i];
    }

    if (qps_fd && qps_fd != stdout) fclose(qps_fd);
    if (latencies_fd && latencies_fd != stdout) fclose(latencies_fd);
    if (worst_latencies_fd && worst_latencies_fd != stdout) fclose(worst_latencies_fd);

    return 0;
}

