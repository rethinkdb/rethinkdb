
#include <stdio.h>
#include <vector>
#include <sstream>
#include <libmemcached/memcached.h>

#define MAX_HOST    255

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

    void print() {
        printf("%d/%d/%d/%d", deletes, updates, inserts, reads);
    }
        
public:
    int deletes;
    int updates;
    int inserts;
    int reads;
};

struct distr_t {
public:
    distr_t(int _min, int _max)
        : min(_min), max(_max)
        {}
    
    distr_t()
        : min(8), max(16)
        {}
    
    void print() {
        printf("%d-%d", min, max);
    }
        
public:
    int min, max; 
};

struct config_t {
public:
    config_t()
        : port(11211),
          clients(512), load(load_t()),
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
    
    exit(0);
}

void* run_client(void* data) {
    // Connect to the server
    memcached_st memcached;
    memcached_create(&memcached);
    memcached_server_add(&memcached, "localhost", 11213);    

    // Perform the ops
    for(int i = 0; i < 100; i++) {
        // Convert i to string
        std::string s;
        std::stringstream out;
        out << i;
        s = out.str();
        const char *v = s.c_str();
        int len = strlen(v);
        memcached_set(&memcached, v, len, v, len, 0, 0);
    }
    
    // Disconnect
    memcached_free(&memcached);
}

int main(int argv, char *argc[])
{
    usage(argc[0]);
    int res;
    int nthreads = 25;
    std::vector<pthread_t> threads(nthreads);
    
    // Create the threads
    for(int i = 0; i < nthreads; i++) {
        int res = pthread_create(&threads[i], NULL, run_client, NULL);
        if(res != 0) {
            fprintf(stderr, "Can't create thread");
            exit(-1);
        }
    }

    // Wait for the threads to finish
    for(int i = 0; i < nthreads; i++) {
        res = pthread_join(threads[i], NULL);
        if(res != 0) {
            fprintf(stderr, "Can't join on the thread");
            exit(-1);
        }
    }
    
    return 0;
}

