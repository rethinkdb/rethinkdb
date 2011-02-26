
#include <map>
#include <unistd.h>
#include <getopt.h>
#include <stdio.h>
#include <vector>
#include <stdio.h>
#include "utils.hpp"
#include "load.hpp"
#include "distr.hpp"
#include "config.hpp"
#include "client.hpp"
#include "args.hpp"
#include "sys/stat.h"
#include "memcached_sock_protocol.hpp"
#ifdef USE_LIBMEMCACHED
#  include "memcached_protocol.hpp"
#endif
#ifdef USE_MYSQL
#  include "mysql_protocol.hpp"
#endif
#include "sqlite_protocol.hpp"


using namespace std;

protocol_t *make_protocol(protocol_enum_t protocol, config_t *config) {
    switch (protocol) {
        case protocol_sockmemcached:
            return (protocol_t*) new memcached_sock_protocol_t(config);
            break;
#ifdef USE_MYSQL
        case protocol_mysql:
            return (protocol_t*) new mysql_protocol_t(config);
            break;
#endif
#ifdef USE_LIBMEMCACHED
        case protocol_libmemcached:
            return (protocol_t*) new memcached_protocol_t(config);
            break;
#endif
        case protocol_sqlite:
            return (protocol_t*) new sqlite_protocol_t(config);
            break;
        default:
            fprintf(stderr, "Unknown protocol\n");
            exit(-1);
    }
}

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

/* Tie it all together */
int main(int argc, char *argv[])
{
    // Initialize randomness
    srand(time(NULL));

    // Parse the arguments
    config_t config;
    parse(&config, argc, argv);
    if (config.load.verifies > 0 && config.clients > 1) {
        printf("Automatically enabled per-client key suffixes\n");
        config.keys.append_client_suffix = true;
    }
    if (config.load.verifies > 0) {
        config.mock_parse = false;
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

    // Gotta run the shared init for each server.
    for (int i = 0; i < config.servers.size(); i++) {
        protocol_t *p = make_protocol(config.servers[i].protocol, &config);
        p->connect(&config.servers[i]);
        p->shared_init();
        delete p;
    }

    for (int i = 0; i < config.clients; i++) {
        if (config.db_file[0]) {
            sqlite_protocol_t *sqlite = new sqlite_protocol_t(&config);
            sqlite->set_id(i);
            sqlite->connect(&config.servers[0]);
            sqlite->shared_init();
            delete sqlite;
        };
    }

    // Let's rock 'n roll
    int res;
    vector<pthread_t> threads(config.clients);

    client_data_t client_data[config.clients];
    for(int i = 0; i < config.clients; i++) {
        client_data[i].config = &config;
        client_data[i].server = &config.servers[i % config.servers.size()];
        client_data[i].id = i;
        client_data[i].min_seed = 0;
        client_data[i].max_seed = 0;

        // Sampling latencies can be expensive because it makes many calls to random(), so we
        // disable it if we aren't going to give latencies to the user
        client_data[i].enable_latency_samples = latencies_fd != NULL ? true : false;

        // Create and connect all protocols first to avoid weird TCP
        // timeout bugs
        client_data[i].proto = (*make_protocol)(client_data[i].server->protocol, &config);
        client_data[i].proto->connect(client_data[i].server);
        if (config.db_file[0]) {
            client_data[i].sqlite = new sqlite_protocol_t(&config);
            client_data[i].sqlite->set_id(i);
            client_data[i].sqlite->connect(client_data[i].server);
        } else {
            client_data[i].sqlite = NULL;
        }

        client_data[i].keep_running = true;
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

            client_data[id].min_seed = min_seed;
            client_data[id].max_seed = max_seed;
        }

        fclose(in_file);
    }

    // Create the threads
    for(int i = 0; i < config.clients; i++) {
        int res = pthread_create(&threads[i], NULL, run_client, &client_data[i]);
        if(res != 0) {
            fprintf(stderr, "Can't create thread\n");
            exit(-1);
        }
    }

    /* The main loop polls the threads for stats once a second, and issues the order to stop when
    it decides it is time. */

    query_stats_t total_stats;

    ticks_t start_time = get_ticks();
    int seconds_of_run = 0, latency_record_counter = 0;

    // TODO: If an workload contains contains no inserts and there are no keys available for a
    // particular client (and the duration is specified in q/i), it'll just loop forever.

    bool keep_running = true;
    while(keep_running) {

        /* Delay approximately one second, making sure we don't drift as time passes */
        ticks_t now = get_ticks(), target = start_time + (seconds_of_run + 1) * secs_to_ticks(1);
        if (now > target) {
            fprintf(stderr, "Reporter thread way too slow for some reason\n");
            //exit(-1);
        }
        else {
            sleep_ticks(target - now);
        }

        /* Poll all the clients for stats */
        query_stats_t stats_for_this_second;
        for(int i = 0; i < config.clients; i++) {
            /* We don't want to hold the spinlock for very long, so we copy the client stats into
            a temporary buffer at first, and then do the (potentially expensive) aggregation
            operation after we have released the spinlock */
            client_data[i].spinlock.lock();
            query_stats_t stat_buffer(client_data[i].stats);
            client_data[i].stats = query_stats_t();
            client_data[i].spinlock.unlock();
            stats_for_this_second.aggregate(&stat_buffer);
        }

        /* Report the stats we got from the clients */
        if (qps_fd) {
            fprintf(qps_fd, "%d\t\t%d\n", seconds_of_run, stats_for_this_second.queries);
            fflush(qps_fd);
        }
        if (latencies_fd) {
            for (int i = 0; i < stats_for_this_second.latency_samples.size(); i++) {
                fprintf(latencies_fd, "%d\t\t%.2f\n",
                    latency_record_counter++,
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
        total_stats.aggregate(&stats_for_this_second);
        seconds_of_run++;
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

    // Notify the threads to shut down
    for(int i = 0; i < config.clients; i++) {
        client_data[i].spinlock.lock();
        client_data[i].keep_running = false;
        client_data[i].spinlock.unlock();
    }

    // Wait for the threads to finish
    for(int i = 0; i < config.clients; i++) {
        res = pthread_join(threads[i], NULL);
        if(res != 0) {
            fprintf(stderr, "Can't join on the thread\n");
            exit(-1);
        }
    }

    // Disconnect everyone
    for(int i = 0; i < config.clients; i++) {
        delete client_data[i].proto;
    }
    
    // Dump key vectors if we have an out file
    if(config.out_file[0] != 0) {
        FILE *out_file = fopen(config.out_file, "w");

        fwrite(&config.clients, sizeof(config.clients), 1, out_file);

        // Dump the keys
        for(int i = 0; i < config.clients; i++) {
            client_data_t *cd = &client_data[i];
            fwrite(&cd->id, sizeof(cd->id), 1, out_file);
            fwrite(&cd->min_seed, sizeof(cd->min_seed), 1, out_file);
            fwrite(&cd->max_seed, sizeof(cd->max_seed), 1, out_file);
        }

        fclose(out_file);
    }

    if (qps_fd && qps_fd != stdout) fclose(qps_fd);
    if (latencies_fd && latencies_fd != stdout) fclose(latencies_fd);
    if (worst_latencies_fd && worst_latencies_fd != stdout) fclose(worst_latencies_fd);

    return 0;
}

