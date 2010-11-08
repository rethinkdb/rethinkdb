
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

using namespace std;

protocol_t* make_protocol(config_t *config) {
    if(config->protocol == protocol_mysql)
        return (protocol_t*) new mysql_protocol_t();
    else if(config->protocol == protocol_sockmemcached)
        return (protocol_t*) new memcached_sock_protocol_t();
    else if(config->protocol == protocol_libmemcached)
        return (protocol_t*) new memcached_protocol_t();
    else {
        printf("Unknown protocol\n");
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

    // Gotta run the shared init
    protocol_t *p = make_protocol(&config);
    p->connect(&config);
    p->shared_init();
    delete p;
    p = NULL;

    // Let's rock 'n roll
    int res;
    vector<pthread_t> threads(config.clients);

    // Create the shared structure
    shared_t shared(&config, make_protocol);

    // Create key vectors
    client_data_t client_data[config.clients];
    for(int i = 0; i < config.clients; i++) {
        client_data[i].config = &config;
        client_data[i].shared = &shared;
        client_data[i].keys = new vector<payload_t>();
    }

    // Populate key vectors if we have an in file
    vector<payload_t> loaded_keys;
    if(config.in_file[0] != 0) {
        FILE *in_file = fopen(config.in_file, "r");

        // Load the keys
        while(feof(in_file) == 0) {
            size_t key_size;
            fread(&key_size, sizeof(size_t), 1, in_file);
            char *key = (char*)malloc(key_size);
            fread(key, key_size, sizeof(char), in_file);
            loaded_keys.push_back(payload_t(key, key_size));
        }

        fclose(in_file);
    }
    
    // Create the threads
    int keys_per_client = loaded_keys.size() / config.clients;
    for(int i = 0; i < config.clients; i++) {
        // If we preloaded the keys, distribute them across the
        // clients
        if(!loaded_keys.empty()) {
            client_data[i].keys->reserve(keys_per_client);
            for(int j = 0; j < keys_per_client; j++) {
                int idx = i * keys_per_client + j;
                if(idx < loaded_keys.size()) {
                    client_data[i].keys->push_back(loaded_keys[idx]);
                }
            }
        }
        
        int res = pthread_create(&threads[i], NULL, run_client, &client_data[i]);
        if(res != 0) {
            fprintf(stderr, "Can't create thread");
            exit(-1);
        }
    }
    loaded_keys.clear();

    // Wait for the threads to finish
    for(int i = 0; i < config.clients; i++) {
        res = pthread_join(threads[i], NULL);
        if(res != 0) {
            fprintf(stderr, "Can't join on the thread");
            exit(-1);
        }
    }

    // Dump key vectors if we have an out file
    if(config.out_file[0] != 0) {
        FILE *out_file = fopen(config.out_file, "aw");

        // Dump the keys
        for(int i = 0; i < config.clients; i++) {
            client_data_t *cd = &client_data[i];
            for(int j = 0; j < cd->keys->size(); j++) {
                char *key = cd->keys->operator[](j).first;
                size_t key_size = cd->keys->operator[](j).second;
                fwrite(&key_size, sizeof(size_t), 1, out_file);
                fwrite(key, key_size, sizeof(char), out_file);
            }
        }
        
        fclose(out_file);
    }
    
    // Free keys and delete key vectors
    for(int i = 0; i < config.clients; i++) {
        vector<payload_t> &keys = *(client_data[i].keys);
        for(vector<payload_t>::iterator j = keys.begin(); j != keys.end(); j++) {
            free(j->first);
        }
        delete client_data[i].keys;
    }
    
    return 0;
}

