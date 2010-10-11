
#include <map>
#include <unistd.h>
#include <getopt.h>
#include <stdio.h>
#include <vector>
#include <time.h>
#include <stdio.h>
#include <libmemcached/memcached.h>

using namespace std;

#define MAX_HOST    255
#define MAX_FILE    255

/* Returns random number between [min, max] */
size_t random(size_t min, size_t max) {
    return rand() % (max - min + 1) + min;
}

/* Timing related functions */
typedef unsigned long long ticks_t;
ticks_t secs_to_ticks(float secs) {
    return (unsigned long long)secs * 1000000000L;
}

ticks_t get_ticks() {
    timespec tv;
    clock_gettime(CLOCK_MONOTONIC, &tv);
    return secs_to_ticks(tv.tv_sec) + tv.tv_nsec;
}

long get_ticks_res() {
    timespec tv;
    clock_getres(CLOCK_MONOTONIC, &tv);
    return secs_to_ticks(tv.tv_sec) + tv.tv_nsec;
}

float ticks_to_secs(ticks_t ticks) {
    return ticks / 1000000000.0f;
}

float ticks_to_ms(ticks_t ticks) {
    return ticks / 1000000.0f;
}

float ticks_to_us(ticks_t ticks) {
    return ticks / 1000.0f;
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
            default:
                fprintf(stderr, "Invalid load format (use D/U/I/R)\n");
                exit(-1);
                break;
            }
            tok = strtok(NULL, "/");
            c++;
        }
        if(c < 4) {
            fprintf(stderr, "Invalid load format (use D/U/I/R)\n");
            exit(-1);
        }
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

    void parse(char *str) {
        char *tok = strtok(str, "-");
        int c = 0;
        while(tok != NULL) {
            switch(c) {
            case 0:
                min = atoi(tok);
                break;
            case 1:
                max = atoi(tok);
                break;
            default:
                fprintf(stderr, "Invalid distr format (use MIN-MAX)\n");
                exit(-1);
                break;
            }
            tok = strtok(NULL, "-");
            c++;
        }
        if(c < 2) {
            fprintf(stderr, "Invalid distr format (use MIN-MAX)\n");
            exit(-1);
        }
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
          duration(10000000L)
        {
            strcpy(host, "localhost");
            latency_file[0] = 0;
            qps_file[0] = 0;
        }

    void print() {
        printf("[host: %s, port: %d, clients: %d, load: ",
               host, port, clients);
        load.print();
        printf(", keys: ");
        keys.print();
        printf(", values: ");
        values.print();
        printf(" , duration: %ld", duration);

        if(latency_file[0] != 0) {
            printf(", latency file: %s", latency_file);
        }
        
        if(qps_file[0] != 0) {
            printf(", QPS file: %s", qps_file);
        }
        
        printf("]\n", duration);
    }
    
public:
    char host[MAX_HOST];
    int port;
    int clients;
    load_t load;
    distr_t keys;
    distr_t values;
    long duration;
    char latency_file[MAX_FILE];
    char qps_file[MAX_FILE];
};

/* Information shared between clients */
struct shared_t {
public:
    shared_t(config_t *_config)
        : config(_config),
          qps_offset(0), latencies_offset(0),
          qps_fd(NULL), latencies_fd(NULL)
        {
            pthread_mutex_init(&mutex, NULL);
            
            if(config->qps_file[0] != 0) {
                qps_fd = fopen(config->qps_file, "wa");
            }
            if(config->latency_file[0] != 0) {
                latencies_fd = fopen(config->latency_file, "wa");
            }
        }

    ~shared_t() {
        if(qps_fd) {
            fwrite(qps, 1, qps_offset, qps_fd);
            fclose(qps_fd);
        }
        if(latencies_fd) {
            fwrite(latencies, 1, latencies_offset, latencies_fd);
            fclose(latencies_fd);
        }

        pthread_mutex_destroy(&mutex);
    }
    
    void push_qps(int _qps, int tick) {
        if(!qps_fd)
            return;
        
        lock();

        // Collect qps info from all clients before attempting to print
        int qps_count = 0, agg_qps = 0;
        map<int, pair<int, int> >::iterator op = qps_map.find(tick);
        if(op != qps_map.end()) {
            qps_count = op->second.first;
            agg_qps = op->second.second;
        }

        qps_count += 1;
        agg_qps += _qps;
        
        if(qps_count == config->clients) {
            _qps = agg_qps;
            qps_map.erase(op);
        } else {
            qps_map[tick] = pair<int, int>(qps_count, agg_qps);
            unlock();
            return;
        }
            
        int _off = snprintf(qps + qps_offset, sizeof(qps) - qps_offset, "%d\n", _qps);
        if(_off >= sizeof(qps) - qps_offset) {
            // Couldn't write everything, flush
            fwrite(qps, 1, qps_offset, qps_fd);
            
            // Write again
            qps_offset = 0;
            _off = snprintf(qps + qps_offset, sizeof(qps) - qps_offset, "%d\n", _qps);
        }
        qps_offset += _off;
        
        unlock();
    }

    void push_latency(float latency) {
        if(!latencies_fd)
            return;

        lock();
        
        int _off = snprintf(latencies + latencies_offset, sizeof(latencies) - latencies_offset, "%.2f\n", latency);
        if(_off >= sizeof(latencies) - latencies_offset) {
            // Couldn't write everything, flush
            fwrite(latencies, 1, latencies_offset, latencies_fd);
            
            // Write again
            latencies_offset = 0;
            _off = snprintf(latencies + latencies_offset, sizeof(latencies) - latencies_offset, "%.2f\n", latency);
        }
        latencies_offset += _off;

        unlock();
    }

private:
    config_t *config;
    map<int, pair<int, int> > qps_map;
    char qps[40960], latencies[40960];
    int qps_offset, latencies_offset;
    FILE *qps_fd, *latencies_fd;
    pthread_mutex_t mutex;

private:
    void lock() {
        pthread_mutex_lock(&mutex);
    }
    void unlock() {
        pthread_mutex_unlock(&mutex);
    }

};

/* Communication structure for main thread and clients */
struct client_data_t {
    config_t *config;
    shared_t *shared;
};

/* Usage */
void usage(const char *name) {
    // Create a default config
    config_t _d;

    // Print usage
    printf("Usage:\n");
    printf("\t%s [OPTIONS]\n", name);

    printf("\nOptions:\n");
    printf("\t-n, --host\n\t\tServer host to connect to. Defaults to [%s].\n", _d.host);
    printf("\t-p, --port\n\t\tServer port to connect to. Defaults to [%d].\n", _d.port);
    printf("\t-c, --clients\n\t\tNumber of concurrent clients. Defaults to [%d].\n", _d.clients);
    printf("\t-w, --workload\n\t\tTarget load to generate. Expects a value in format D/U/I/R, where\n" \
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
    printf("\t-l, --latency-file\n\t\tFile name to output individual latency information (in us).\n" \
           "\t\tThe information is not outputted if this argument is skipped.\n");
    printf("\t-q, --qps-file\n\t\tFile name to output QPS information.\n" \
           "\t\tThe information is not outputted if this argument is skipped.\n");

    printf("\nAdditional information:\n");
    printf("\t\tDISTR format describes a range and can be specified in as MIN-MAX\n");
    
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
                {"host",           required_argument, 0, 'n'},
                {"port",           required_argument, 0, 'p'},
                {"clients",        required_argument, 0, 'c'},
                {"workload",       required_argument, 0, 'w'},
                {"keys",           required_argument, 0, 'k'},
                {"values",         required_argument, 0, 'v'},
                {"duration",       required_argument, 0, 'd'},
                {"latency-file",   required_argument, 0, 'l'},
                {"qps-file",       required_argument, 0, 'q'},
                {"help",           no_argument, &do_help, 1},
                {0, 0, 0, 0}
            };

        int option_index = 0;
        int c = getopt_long(argc, argv, "n:p:c:w:k:v:d:l:q:h", long_options, &option_index);

        if(do_help)
            c = 'h';
     
        /* Detect the end of the options. */
        if (c == -1)
            break;
     
        switch (c)
        {
        case 0:
            break;
        case 'n':
            strncpy(config->host, optarg, MAX_HOST);
            break;
        case 'p':
            config->port = atoi(optarg);
            break;
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
            break;
        case 'd':
            config->duration = atol(optarg);
            break;
        case 'l':
            strncpy(config->latency_file, optarg, MAX_FILE);
            break;
        case 'q':
            strncpy(config->qps_file, optarg, MAX_FILE);
            break;
        case 'h':
            usage(argv[0]);
            break;
     
        default:
            /* getopt_long already printed an error message. */
            usage(argv[0]);
        }
    }
}

/* The function that does the work */
void* run_client(void* data) {
    // Grab the config
    client_data_t *client_data = (client_data_t*)data;
    config_t *config = client_data->config;
    shared_t *shared = client_data->shared;
    
    // Connect to the server
    memcached_st memcached;
    memcached_create(&memcached);
    memcached_server_add(&memcached, config->host, config->port);

    // Store the keys so we can run updates and deletes.
    vector<payload_t> keys;

    // Perform the ops
    ticks_t last_time = get_ticks(), last_qps_time = last_time, now_time;
    int qps = 0, tick = 0;
    for(int i = 0; i < config->duration / config->clients; i++) {
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
        now_time = get_ticks();
        qps++;

        // Deal with individual op latency
        ticks_t latency = now_time - last_time;
        shared->push_latency(ticks_to_us(latency));
        last_time = now_time;

        // Deal with QPS
        if(ticks_to_secs(now_time - last_qps_time) >= 1.0f) {
            shared->push_qps(qps, tick);

            last_qps_time = now_time;
            qps = 0;
            tick++;
        }
    }
    
    // Disconnect
    memcached_free(&memcached);

    // Free all the keys
    for(vector<payload_t>::iterator i = keys.begin(); i != keys.end(); i++) {
        free(i->first);
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
    shared_t shared(&config);
    client_data_t client_data;
    client_data.config = &config;
    client_data.shared = &shared;

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

