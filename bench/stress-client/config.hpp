
#ifndef __CONFIG_HPP__
#define __CONFIG_HPP__

#define MAX_HOST    255
#define MAX_FILE    255

enum protocol_enum_t {
    protocol_libmemcached, protocol_sockmemcached, protocol_mysql
};

/* Defines a client configuration, including sensible default
 * values.*/
struct config_t {
public:
    config_t()
        : port(11211), protocol(protocol_sockmemcached),
          clients(64), load(load_t()),
          keys(distr_t(8, 16)), values(distr_t(8, 128)),
          duration(10000000L), batch_factor(distr_t(1, 16))
        {
            strcpy(host, "localhost");
            latency_file[0] = 0;
            qps_file[0] = 0;
        }

    void print() {
        printf("--- Workload -----\n");
        printf("Duration..........%ldops\n", duration);

        printf("Protocol..........");
        if(protocol == protocol_libmemcached)
            printf("libmemcached");
        else if(protocol == protocol_sockmemcached)
            printf("sockmemcached");
        else if(protocol == protocol_mysql)
            printf("mysql");
        else
            printf("unknown");
        
        printf("\nClients...........%d\nLoad..............", clients);
        load.print();
        printf("\nKeys..............");
        keys.print();
        printf("\nValues............");
        values.print();
        printf("\nBatch factor......");
        batch_factor.print();
        printf("\n");

        printf("\n--- Networking ---\n");
        printf("Host..............%s\nPort..............%d\n", host, port);
        
        printf("\n--- Data files ---\n");
        printf("Latency file......");
        if(latency_file[0] != 0) {
            printf("%s\n", latency_file);
        } else {
            printf("[N/A]\n");
        }
        printf("QPS file..........");
        if(qps_file[0] != 0) {
            printf("%s\n", qps_file);
        } else {
            printf("[N/A]\n");
        }
    }
    
public:
    char host[MAX_HOST];
    int port;
    protocol_enum_t protocol;
    int clients;
    load_t load;
    distr_t keys;
    distr_t values;
    long duration;
    distr_t batch_factor;
    char latency_file[MAX_FILE];
    char qps_file[MAX_FILE];
};

#endif // __CONFIG_HPP__

