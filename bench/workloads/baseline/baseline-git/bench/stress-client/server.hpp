
#ifndef __SERVER_HPP__
#define __SERVER_HPP__

#include <stdio.h>
#include <string>

#define MAX_HOST 255

enum protocol_enum_t {
    protocol_libmemcached, protocol_sockmemcached, protocol_mysql, protocol_sqlite,
};

struct server_t {
    server_t() : protocol(protocol_sockmemcached) {
        strcpy(host, "localhost:11211");
    }

    protocol_enum_t parse_protocol(const char *name) {
        if(strcmp(name, "mysql") == 0)
            return protocol_mysql;
        else if(strcmp(name, "libmemcached") == 0)
            return protocol_libmemcached;
        else if(strcmp(name, "sockmemcached") == 0)
            return protocol_sockmemcached;
        else if(strcmp(name, "sqlite") == 0)
            return protocol_sqlite;
        else {
            fprintf(stderr, "Unknown protocol\n");
            exit(-1);
        }
    }

    void parse(char *str) {
        char *_host = NULL;
        if (_host = strchr(str, ',')) {
            *_host = '\0';
            _host++;
            protocol = parse_protocol(str);
        } else {
            _host = str;
        }
        strncpy(host, _host, MAX_HOST);
    }

    void print_protocol() {
        if(protocol == protocol_libmemcached)
            printf("libmemcached");
        else if(protocol == protocol_sockmemcached)
            printf("sockmemcached");
        else if(protocol == protocol_mysql)
            printf("mysql");
        else if(protocol == protocol_sqlite)
            printf("sqlite");
        else
            printf("unknown");
    }

    void print() {
        print_protocol();
        printf(",%s", host);
    }


    protocol_enum_t protocol;
    char host[MAX_HOST];
};

#endif // __SERVER_HPP__

