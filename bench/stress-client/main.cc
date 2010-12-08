
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
#include "memcached_sock_protocol.hpp"
#include "memcached_protocol.hpp"
#include "mysql_protocol.hpp"
#include "sqlite_protocol.hpp"
#include "sys/stat.h"

using namespace std;

protocol_t* make_protocol(protocol_enum_t protocol) {
    switch (protocol) {
        case protocol_mysql:
            return (protocol_t*) new mysql_protocol_t();
            break;
        case protocol_sockmemcached:
            return (protocol_t*) new memcached_sock_protocol_t();
            break;
        case protocol_libmemcached:
            return (protocol_t*) new memcached_protocol_t();
            break;
        case protocol_sqlite:
            return (protocol_t*) new sqlite_protocol_t();
            break;
        default:
            fprintf(stderr, "Unknown protocol\n");
            exit(-1);
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
    config.print();

    /* make a directory for our sqlite files */
    if (config.db_file[0]) {
        mkdir(BACKUP_FOLDER, 0777);
    }

    // Gotta run the shared init for each server.
    for (int i = 0; i < config.servers.size(); i++) {
        protocol_t *p = make_protocol(config.servers[i].protocol);
        p->connect(&config, &config.servers[i]);
        p->shared_init();
        delete p;
    }

    for (int i = 0; i < config.clients; i++) {
        if (config.db_file[0]) {
            sqlite_protocol_t *sqlite = (sqlite_protocol_t *) make_protocol(protocol_sqlite);
            sqlite->set_id(i);
            sqlite->connect(&config, &config.servers[0]);
            sqlite->shared_init();
            delete sqlite;
        };
    }

    // Let's rock 'n roll
    int res;
    vector<pthread_t> threads(config.clients);

    // Create the shared structure
    shared_t shared(&config);

    client_data_t client_data[config.clients];
    for(int i = 0; i < config.clients; i++) {
        client_data[i].config = &config;
        client_data[i].server = &config.servers[i % config.servers.size()];
        client_data[i].shared = &shared;
        client_data[i].id = i;
        client_data[i].min_seed = 0;
        client_data[i].max_seed = 0;

        // Create and connect all protocols first to avoid weird TCP
        // timeout bugs
        client_data[i].proto = (*make_protocol)(client_data[i].server->protocol);
        client_data[i].proto->connect(&config, client_data[i].server);
        if (config.db_file[0]) {
            client_data[i].sqlite = (sqlite_protocol_t*)(*make_protocol)(protocol_sqlite);
            client_data[i].sqlite->set_id(i);
            client_data[i].sqlite->connect(&config, client_data[i].server);
        } else {
            client_data[i].sqlite = NULL;
        }
    }

    // If input keys are provided, read them in
    if(config.in_file[0] != 0) {
        FILE *in_file = fopen(config.in_file, "r");

        if(in_file == NULL) {
            fprintf(stderr, "Could not open output key file");
            exit(-1);
        }

        while(feof(in_file) == 0) {
            int id, min_seed, max_seed;
            size_t res __attribute__((unused)) = fread(&id, sizeof(id), 1, in_file);
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
            fprintf(stderr, "Can't create thread");
            exit(-1);
        }
    }

    // Wait for the threads to finish
    for(int i = 0; i < config.clients; i++) {
        res = pthread_join(threads[i], NULL);
        if(res != 0) {
            fprintf(stderr, "Can't join on the thread");
            exit(-1);
        }
    }

    // Disconnect everyone
    for(int i = 0; i < config.clients; i++) {
        delete client_data[i].proto;
    }
    
    // Dump key vectors if we have an out file
    if(config.out_file[0] != 0) {
        FILE *out_file = fopen(config.out_file, "aw");

        // Dump the keys
        for(int i = 0; i < config.clients; i++) {
            client_data_t *cd = &client_data[i];
            fwrite(&cd->id, sizeof(cd->id), 1, out_file);
            fwrite(&cd->min_seed, sizeof(cd->min_seed), 1, out_file);
            fwrite(&cd->max_seed, sizeof(cd->max_seed), 1, out_file);
        }

        fclose(out_file);
    }

    return 0;
}

