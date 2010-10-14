
#ifndef __CONFIG_HPP__
#define __CONFIG_HPP__

#define MAX_HOST    255
#define MAX_FILE    255

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

#endif // __CONFIG_HPP__

