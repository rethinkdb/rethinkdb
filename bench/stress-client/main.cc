
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

    // Create the shared structure
    shared_t shared(&config, make_protocol);
    client_data_t client_data;
    client_data.config = &config;
    client_data.shared = &shared;

    // Gotta run the shared init
    protocol_t *p = make_protocol(&config);
    p->connect(&config);
    p->shared_init();
    delete p;
    p = NULL;

    // Let's rock 'n roll
    int res;
    vector<pthread_t> threads(config.clients);
    
    // Create the threads
    for(int i = 0; i < config.clients; i++) {
        int res = pthread_create(&threads[i], NULL, run_client, &client_data);
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
    
    return 0;
}

