
#include <stdio.h>
#include <vector>
#include <time.h>
#include <libmemcached/memcached.h>

using namespace std;

#define MAX_HOST    255

/* Returns random number between [min, max] */
size_t random(size_t min, size_t max) {
    return rand() % (max - min + 1) + min;
}

/* Helpful typedefs */
typedef pair<char*, size_t> payload_t;

/* Defines a load in terms of ratio of deletes, updates, inserts, and
 * reads. */
struct load_t {
public:
    load_t()
        : deletes(1), updates(4),
          inserts(8), reads(64)
        {}
    
    load_t(int d, int u, int i, int r)
        : deletes(d), updates(u),
          inserts(i), reads(r)
        {}

    enum load_op_t {
        delete_op, update_op, insert_op, read_op
    };
    
    // Generates an operation to perform using the ratios as
    // probabilities.
    load_op_t toss() {
        int total = deletes + updates + inserts + reads;
        int rand_op = random(0, total - 1);
        int acc = 0;

        if(rand_op >= acc && rand_op < (acc += deletes))
            return delete_op;
        if(rand_op >= acc && rand_op < (acc += updates))
            return update_op;
        if(rand_op >= acc && rand_op < (acc += inserts))
            return insert_op;
        if(rand_op >= acc && rand_op < (acc += reads))
            return read_op;
        
        fprintf(stderr, "Something horrible has happened with the toss\n");
        exit(-1);
    }

    void print() {
        printf("%d/%d/%d/%d", deletes, updates, inserts, reads);
    }
        
public:
    int deletes;
    int updates;
    int inserts;
    int reads;
};

/* Defines a distribution of values, from min to max. */
struct distr_t {
public:
    distr_t(int _min, int _max)
        : min(_min), max(_max)
        {}
    
    distr_t()
        : min(8), max(16)
        {}

    // Generates a random payload within given bounds. It is the
    // user's responsibility to clean up the payload.
    void toss(payload_t *payload) {
        // generate the size
        int s = random(min, max);
        
        // malloc and fill memory
        char *l = (char*)malloc(s);
        for(int i = 0; i < s; i++) {
            l[i] = random(65, 90);
        }
        
        // fill the payload
        payload->first = l;
        payload->second = s;
    }
    
    void print() {
        printf("%d-%d", min, max);
    }
        
public:
    int min, max; 
};

/* Defines a client configuration, including sensible default
 * values.*/
struct config_t {
public:
    config_t()
        : port(11211),
          clients(64), load(load_t()),
          keys(distr_t(8, 16)), values(distr_t(8, 128)),
          duration(10000000)
        {
            strcpy(host, "localhost");
        }

    void print() {
        printf("[host: %s, port: %d, clients: %d, load: ",
               host, port, clients);
        load.print();
        printf(", keys: ");
        keys.print();
        printf(", values: ");
        values.print();
        printf(" , duration: %ld]\n", duration);
    }
    
public:
    char host[MAX_HOST];
    int port;
    int clients;
    load_t load;
    distr_t keys;
    distr_t values;
    long duration;
};

/* Usage */
void usage(const char *name) {
    // Create a default config
    config_t _d;

    // Print usage
    printf("Usage:\n");
    printf("\t%s [OPTIONS]\n", name);

    printf("\nOptions:\n");
    printf("\t-h, --host\n\t\tServer host to connect to. Defaults to [%s].\n", _d.host);
    printf("\t-p, --port\n\t\tServer port to connect to. Defaults to [%d].\n", _d.port);
    printf("\t-c, --clients\n\t\tNumber of concurrent clients. Defaults to [%d].\n", _d.clients);
    printf("\t-l, --load\n\t\tTarget load to generate. Expects a value in format D:U:I:R, where\n" \
           "\t\t\tD - number of deletes\n" \
           "\t\t\tU - number of updates\n" \
           "\t\t\tI - number of inserts\n" \
           "\t\t\tR - number of reads\n" \
           "\t\tDefaults to [");
    _d.load.print();
    printf("]\n");
    printf("\t-k, --keys\n\t\tKey distribution in DISTR format (see below). Defaults to [");
    _d.keys.print();
    printf("].\n");
    printf("\t-v, --values\n\t\tValue distribution in DISTR format (see below). Defaults to [");
    _d.values.print();
    printf("].\n");
    printf("\t-d, --duration\n\t\tDuration of the run specified in number of operations.\n" \
           "\t\tDefaults to [%ld].\n", _d.duration);

    printf("\nAdditional information:\n");
    printf("\t\tDISTR format describes a range and can be specified in as MIN-MAX\n");
    
    exit(-1);
}

void* run_client(void* data) {
    // Grab the config
    config_t *config = (config_t*)data;
    
    // Connect to the server
    memcached_st memcached;
    memcached_create(&memcached);
    memcached_server_add(&memcached, config->host, config->port);

    // Store the keys so we can run updates and deletes.
    vector<payload_t> keys;

    // Perform the ops
    for(int i = 0; i < config->duration; i++) {
        // Generate the command
        load_t::load_op_t cmd = config->load.toss();

        int _val;
        payload_t key, value;
        
        char *_value;
        size_t _value_length;
        uint32_t _flags;
        memcached_return_t _error;
        
        switch(cmd) {
        case load_t::delete_op:
            // Find the key
            if(keys.empty())
                break;
            _val = random(0, keys.size() - 1);
            key = keys[_val];
            // Delete it from the server
            _error = memcached_delete(&memcached, key.first, key.second, 0);
            if(_error != MEMCACHED_SUCCESS) {
                fprintf(stderr, "Error performing delete operation (%d)\n", _error);
                exit(-1);
            }
            // Our own bookkeeping
            free(key.first);
            keys[_val] = keys[keys.size() - 1];
            keys.erase(keys.begin() + _val);
            break;
            
        case load_t::update_op:
            // Find the key and generate the payload
            if(keys.empty())
                break;
            key = keys[random(0, keys.size() - 1)];
            config->values.toss(&value);
            // Send it to server
            _error = memcached_set(&memcached, key.first, key.second, value.first, value.second, 0, 0);
            if(_error != MEMCACHED_SUCCESS) {
                fprintf(stderr, "Error performing update operation (%d)\n", _error);
                exit(-1);
            }
            // Free the value
            free(value.first);
            break;
            
        case load_t::insert_op:
            // Generate the payload
            config->keys.toss(&key);
            config->values.toss(&value);
            // Send it to server
            _error = memcached_set(&memcached, key.first, key.second, value.first, value.second, 0, 0);
            if(_error != MEMCACHED_SUCCESS) {
                fprintf(stderr, "Error performing insert operation (%d)\n", _error);
                exit(-1);
            }
            // Free the value and save the key
            free(value.first);
            keys.push_back(key);
            break;
            
        case load_t::read_op:
            // Find the key
            if(keys.empty())
                break;
            key = keys[random(0, keys.size() - 1)];
            // Read it from the server
            _value = memcached_get(&memcached, key.first, key.second,
                                   &_value_length, &_flags, &_error);
            if(_error != MEMCACHED_SUCCESS) {
                fprintf(stderr, "Error performing read operation (%d)\n", _error);
                exit(-1);
            }
            // Our own bookkeeping
            free(_value);
            break;
        };
    }
    
    // Disconnect
    memcached_free(&memcached);

    // Free all the keys
    for(vector<payload_t>::iterator i = keys.begin(); i != keys.end(); i++) {
        free(i->first);
    }
}

int main(int argv, char *argc[])
{
    // Initialize randomness
    srand(time(NULL));
    
    // Parse the arguments
    config_t config;
    int res;
    vector<pthread_t> threads(config.clients);
    
    // Create the threads
    for(int i = 0; i < config.clients; i++) {
        int res = pthread_create(&threads[i], NULL, run_client, &config);
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

