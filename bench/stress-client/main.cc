
#include <map>
#include <unistd.h>
#include <getopt.h>
#include <stdio.h>
#include <vector>
#include <stdio.h>
#include <sys/stat.h>
#include "client.hpp"
#include "args.hpp"
#include "read_ops.hpp"
#include "write_ops.hpp"
#ifdef USE_MYSQL
#include "mysql_protocol.hpp"   // For initialize_mysql_table()
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
    if (config.load.op_ratios.verifies > 0 && config.clients > 1) {
        printf("Automatically enabled per-client key suffixes\n");
        config.load.keys.append_client_suffix = true;
    }
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
                config.load.keys.max, config.load.values.max);
        }
    }
#endif

    /* Create client objects */

    struct client_stuff_t {

        protocol_t *protocol;
        sqlite_protocol_t *sqlite;
        client_t client;
        read_op_t read_op;
        delete_op_t delete_op;
        update_op_t update_op;
        insert_op_t insert_op;
        append_prepend_op_t append_op;
        append_prepend_op_t prepend_op;
        verify_op_t verify_op;

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
            sqlite(make_sqlite_if_necessary(config, i)),

            /* Construct the client object */
            client(i, config->clients, config->load.keys, config->load.distr, config->load.mu, sqlite),

            /* Set up some operations for the client to perform */
            read_op(&client,
                // Scale the read-ratio to take into account the batch factor for reads
                config->load.op_ratios.reads / ((config->load.batch_factor.min + config->load.batch_factor.max) / 2),
                protocol, config->load.batch_factor),
            delete_op(&client, config->load.op_ratios.deletes, protocol),
            update_op(&client, config->load.op_ratios.updates, protocol, config->load.values),
            insert_op(&client, config->load.op_ratios.inserts, protocol, config->load.values),
            append_op(&client, config->load.op_ratios.appends, protocol, true, config->load.values),
            prepend_op(&client, config->load.op_ratios.prepends, protocol, false, config->load.values),
            verify_op(&client, config->load.op_ratios.verifies, protocol)
        {
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
                clients[i]->client.ops[j]->enable_latency_samples = false;
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
            int id, min_seed, max_seed;
            res = fread(&id, sizeof(id), 1, in_file);
            res = fread(&min_seed, sizeof(min_seed), 1, in_file);
            res = fread(&max_seed, sizeof(max_seed), 1, in_file);

            clients[id]->client.min_seed = min_seed;
            clients[id]->client.max_seed = max_seed;
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
            c->client.spinlock.lock();

            /* We pool all the stats from different operations together into one pool for
            reporting. If you want to get stats for individual operations separately, use
            the Python interface. */
            for (int j = 0; j < (int)c->client.ops.size(); j++) {
                round_stats.aggregate(c->client.ops[j]->stats);
            }

            /* Count total number of keys inserted and deleted (we will use this if our
            duration is specified in inserts) */
            round_inserts_minus_deletes += c->insert_op.stats.queries - c->delete_op.stats.queries;

            /* Reset the stats, so that every time we poll we only get the stats from
            since we last polled. */
            for (int j = 0; j < (int)c->client.ops.size(); j++) {
                c->client.ops[j]->stats = query_stats_t();
            }

            c->client.spinlock.unlock();
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
            fprintf(worst_latencies_fd, "worst-latency\t%d\t%.2f\n",
                total_time, ticks_to_us(round_stats.worst_latency));
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
            total_stats.aggregate(c->client.ops[j]->stats);
        }
        total_inserts_minus_deletes += c->insert_op.stats.queries - c->delete_op.stats.queries;
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
            fwrite(&clients[i]->client.min_seed, sizeof(clients[i]->client.min_seed), 1, out_file);
            fwrite(&clients[i]->client.max_seed, sizeof(clients[i]->client.max_seed), 1, out_file);
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

