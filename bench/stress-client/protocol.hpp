#ifndef __STRESS_CLIENT_PROTOCOL_HPP__
#define __STRESS_CLIENT_PROTOCOL_HPP__

#include <exception>
#include <string>
#include "distr.hpp"

class protocol_error_t : public std::exception, public std::string {
public:
    protocol_error_t(const std::string& message) : std::string(message) { }
    virtual ~protocol_error_t() throw () { }
};

struct protocol_t {
    virtual ~protocol_t() {}

    virtual void remove(const char *key, size_t key_size) = 0;
    virtual void update(const char *key, size_t key_size,
                        const char *value, size_t value_size) = 0;
    virtual void insert(const char *key, size_t key_size,
                        const char *value, size_t value_size) = 0;

    virtual void read(payload_t *keys, int count, payload_t *values = NULL) = 0;


    /* These functions allow reads to be pipelined to the server meaning there
     * will be several reads sent over the socket and the client will not block
     * on their return. It is the users job to make sure the queue doesn't get
     * bigger than they want it to. */

    /* By default these functions are just stubbed out in terms of read which
     * is sementically correct but offers no performance improvements. */

    /* add a read to the pipeline */
public:
    virtual void enqueue_read(payload_t *keys, int count, payload_t *values = NULL) {
        read(keys, count, values);
    }

    /* remove as many reads as have been returned from the pipeline */
    virtual bool dequeue_read_maybe(payload_t *keys, int count, payload_t *values = NULL) { 
        return true;
    }

    /* Wait until all of the pipelined reads have been returned */
    virtual void dequeue_read(payload_t *keys, int count, payload_t *values = NULL) { }

    virtual void range_read(char* lkey, size_t lkey_size, char* rkey, size_t rkey_size, int count_limit, payload_t *values = NULL) = 0;

    virtual void append(const char *key, size_t key_size,
                        const char *value, size_t value_size) = 0;

    virtual void prepend(const char *key, size_t key_size,
                          const char *value, size_t value_size) = 0;
};

#define MAX_HOST 255

enum protocol_enum_t {
    protocol_rethinkdb,
    protocol_sockmemcached,
#ifdef USE_MYSQL
    protocol_mysql,
#endif
#ifdef USE_LIBMEMCACHED
    protocol_libmemcached,
#endif
    protocol_sqlite,
};

struct server_t {
    server_t() : protocol(protocol_rethinkdb) {
        strcpy(host, "localhost:28015");
    }

    protocol_enum_t parse_protocol(const char *name) {
        if (strcmp(name, "rethinkdb") == 0) {
            return protocol_rethinkdb;
        } else if (strcmp(name, "sockmemcached") == 0) {
            return protocol_sockmemcached;
#ifdef USE_MYSQL
        } else if (strcmp(name, "mysql") == 0) {
            return protocol_mysql;
#endif
#ifdef USE_LIBMEMCACHED
        } else if (strcmp(name, "libmemcached") == 0) {
            return protocol_libmemcached;
#endif
        } else if(strcmp(name, "sqlite") == 0) {
            return protocol_sqlite;
        } else {
            fprintf(stderr, "Unknown protocol\n");
            exit(-1);
        }
    }

    void parse(const char *const_str) {
        char str[500];
        strncpy(str, const_str, sizeof(str));
        char *_host = NULL;
        if (_host = strchr(str, ',')) {
            *_host = '\0';
            _host++;
            protocol = parse_protocol(str);
        } else {
            _host = str;
            protocol = protocol_rethinkdb;
        }
        strncpy(host, _host, MAX_HOST);
    }

    void print_protocol() {
        if (protocol == protocol_rethinkdb) {
            printf("rethinkdb");
        } else if (protocol == protocol_sockmemcached) {
            printf("sockmemcached");
#ifdef USE_MYSQL
        } else if (protocol == protocol_mysql) {
            printf("mysql");
#endif
#ifdef USE_LIBMEMCACHED
        } else if (protocol == protocol_libmemcached) {
            printf("libmemcached");
#endif
        } else if (protocol == protocol_sqlite) {
            printf("sqlite");
        } else {
            printf("unknown");
        }
    }

    void print() {
        print_protocol();
        printf(",%s", host);
    }

    protocol_t *connect();

    protocol_enum_t protocol;
    char host[MAX_HOST];
};

#endif // __STRESS_CLIENT_PROTOCOL_HPP__

