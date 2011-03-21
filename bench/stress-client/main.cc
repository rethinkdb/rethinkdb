
#include <map>
#include <unistd.h>
#include <getopt.h>
#include <stdio.h>
#include <vector>
#include <stdio.h>
#include "utils.hpp"
#include "load.hpp"
#include "distr.hpp"
#include "client.hpp"
#include "args.hpp"
#include "sys/stat.h"

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

    /* Create client objects */
    protocol_t *protocols[config.clients];
    sqlite_protocol_t *sqlites[config.clients];
    client_t *clients[config.clients];
    for (int i = 0; i < config.clients; i++) {

        protocols[i] = config.servers[i % config.servers.size()].connect(&config.load);

        if (config.db_file[0]) {
            char buffer[2048];
            sprintf(buffer, "%s/%d_%s", BACKUP_FOLDER, i, config.db_file);
            sqlites[i] = new sqlite_protocol_t(buffer, &config.load);
        } else {
            sqlites[i] = NULL;
        }

        clients[i] = new client_t(&config.load, protocols[i], sqlites[i], i, config.clients);

        /* Collecting latency samples is a potentially expensive operation, so we disable it
        if the user does not ask for them. */
        if (latencies_fd == NULL) clients[i]->enable_latency_samples = false;
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

            clients[id]->min_seed = min_seed;
            clients[id]->max_seed = max_seed;
        }

        fclose(in_file);
    }

    // Start everything running
    for(int i = 0; i < config.clients; i++) {
        clients[i]->start();
    }

    /* The main loop polls the threads for stats once a second, and issues the order to stop when
    it decides it is time. */

    // Used to sum up total number of queries and total inserts_minus_deletes. Its worst_latency
    // is valid but not used. Its latency sample pool is not valid.
    query_stats_t total_stats;

    ticks_t start_time = get_ticks();
    int seconds_of_run = 0;

    // TODO: If an workload contains contains no inserts and there are no keys available for a
    // particular client (and the duration is specified in q/i), it'll just loop forever.

    bool keep_running = true;
    while(keep_running) {

        /* Delay an integer number of seconds, preferably 1. */
        int delay_seconds = 1;
        ticks_t now = get_ticks();
        while (now > start_time + (seconds_of_run + delay_seconds) * secs_to_ticks(1)) {
            delay_seconds++;
        }
        sleep_ticks(start_time + (seconds_of_run + delay_seconds) * secs_to_ticks(1) - now);

        /* Poll all the clients for stats */
        query_stats_t stats_for_this_second;
        for(int i = 0; i < config.clients; i++) {
            query_stats_t buffer;
            clients[i]->poll(&buffer);
            stats_for_this_second.aggregate(buffer);
        }

        /* Report the stats we got from the clients */
        if (qps_fd) {
            fprintf(qps_fd, "%d\t\t%d\n", seconds_of_run, stats_for_this_second.queries / delay_seconds);
            fflush(qps_fd);
        }
        if (latencies_fd) {
            for (int i = 0; i < stats_for_this_second.latency_samples.size(); i++) {
                fprintf(latencies_fd, "%d\t\t%.2f\n",
                    seconds_of_run,
                    ticks_to_us(stats_for_this_second.latency_samples.samples[i]));
            }
            fflush(latencies_fd);
        }
        if (worst_latencies_fd) {
            fprintf(worst_latencies_fd, "worst-latency\t%d\t%.2f\n",
                seconds_of_run, ticks_to_us(stats_for_this_second.worst_latency));
            fflush(worst_latencies_fd);
        }

        /* Update the aggregate total stats, and check if we are done running */
        total_stats.aggregate(stats_for_this_second);
        seconds_of_run += delay_seconds;
        if (config.duration.duration != -1) {
            switch (config.duration.units) {
            case duration_t::queries_t:
                keep_running = total_stats.queries < config.duration.duration;
                break;
            case duration_t::seconds_t:
                keep_running = seconds_of_run < config.duration.duration;
                break;
            case duration_t::inserts_t:
                keep_running = total_stats.inserts_minus_deletes < config.duration.duration;
                break;
            default:
                fprintf(stderr, "Unknown duration unit\n");
                exit(-1);
            }
        }
    }

    // Stop the threads
    for (int i = 0; i < config.clients; i++) {
        clients[i]->stop();
    }

    /* Collect information about the last few operations performed, then print totals. */
    for (int i = 0; i < config.clients; i++) {
        query_stats_t buffer;
        clients[i]->poll(&buffer);
        total_stats.aggregate(buffer);
    }
    printf("Total running time: %f seconds\n", ticks_to_secs(get_ticks() - start_time));
    printf("Total operations: %d\n", total_stats.queries);
    printf("Total keys inserted minus keys deleted: %d\n", total_stats.inserts_minus_deletes);

    // Dump key vectors if we have an out file
    if(config.out_file[0] != 0) {
        FILE *out_file = fopen(config.out_file, "w");

        fwrite(&config.clients, sizeof(config.clients), 1, out_file);

        // Dump the keys
        for(int i = 0; i < config.clients; i++) {
            fwrite(&i, sizeof(i), 1, out_file);
            fwrite(&clients[i]->min_seed, sizeof(clients[i]->min_seed), 1, out_file);
            fwrite(&clients[i]->max_seed, sizeof(clients[i]->max_seed), 1, out_file);
        }

        fclose(out_file);
    }

    // Clean up
    for(int i = 0; i < config.clients; i++) {
        delete clients[i];
        if (sqlites[i]) delete sqlites[i];
        delete protocols[i];
    }

    if (qps_fd && qps_fd != stdout) fclose(qps_fd);
    if (latencies_fd && latencies_fd != stdout) fclose(latencies_fd);
    if (worst_latencies_fd && worst_latencies_fd != stdout) fclose(worst_latencies_fd);

    return 0;
}

