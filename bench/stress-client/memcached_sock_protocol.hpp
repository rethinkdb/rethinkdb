
#ifndef __MEMCACHED_SOCK_PROTOCOL_HPP__
#define __MEMCACHED_SOCK_PROTOCOL_HPP__

#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include "protocol.hpp"

struct memcached_sock_protocol_t : public protocol_t {
    memcached_sock_protocol_t()
        : sockfd(-1)
        {
            // init the socket
            sockfd = socket(AF_INET, SOCK_STREAM, 0);
            if(sockfd < 0) {
                int err = errno;
                fprintf(stderr, "Could not create socket\n");
                exit(-1);
            }
        }

    virtual ~memcached_sock_protocol_t() {
        if(sockfd != -1) {
            int res = close(sockfd);
            int err = errno;
            if(res != 0) {
                fprintf(stderr, "Could not close socket\n");
                exit(-1);
            }
        }
    }

    virtual void connect(config_t *config, server_t *server) {
        // Parse the host string
        char _host[MAX_HOST];
        strncpy(_host, server->host, MAX_HOST);

        char *_port = NULL;

        _port = strchr(_host, ':');
        if(_port) {
            *_port = '\0';
            _port++;
        } else {
            fprintf(stderr, "Please use host string of the form host:port.\n");
            exit(-1);
        }

        int port = atoi(_port);

        // Setup the host/port data structures
        struct sockaddr_in sin;
        struct hostent *host = gethostbyname(_host);
        memcpy(&sin.sin_addr.s_addr, host->h_addr, host->h_length);
        sin.sin_family = AF_INET;
        sin.sin_port = htons(port);

        // Connect to server
        int res = ::connect(sockfd, (struct sockaddr *)&sin, sizeof(sin));
        if(res < 0) {
            int err = errno;
            fprintf(stderr, "Could not connect to server (%d)\n", err);
            exit(-1);
        }
    }

    virtual void remove(const char *key, size_t key_size) {
        // Setup the text command
        char *buf = buffer;
        buf += sprintf(buf, "delete ");
        memcpy(buf, key, key_size);
        buf += key_size;
        buf += sprintf(buf, "\r\n");

        // Send it on its merry way to the server
        send_command(buf - buffer);

        // Check the result
        expect(NULL, 0);
    }

    virtual void update(const char *key, size_t key_size,
                        const char *value, size_t value_size) {
        insert(key, key_size, value, value_size);
    }

    virtual void insert(const char *key, size_t key_size,
                        const char *value, size_t value_size) {
        // Setup the text command
        char *buf = buffer;
        buf += sprintf(buf, "set ");
        memcpy(buf, key, key_size);
        buf += key_size;
        buf += sprintf(buf, " 0 0 %d\r\n", (int)value_size);
        memcpy(buf, value, value_size);
        buf += value_size;
        buf += sprintf(buf, "\r\n");

        // Send it on its merry way to the server
        send_command(buf - buffer);

        // Check the result
        expect(NULL, 0);
    }

    virtual void read(payload_t *keys, int count, payload_t *values = NULL) {
        // Setup the text command
        char *buf = buffer;
        buf += sprintf(buf, "get ");
        for(int i = 0; i < count; i++) {
            memcpy(buf, keys[i].first, keys[i].second);
            buf += keys[i].second;
            if(i != count - 1) {
                buf += sprintf(buf, " ");
            }
        }
        buf += sprintf(buf, "\r\n");

        // Send it on its merry way to the server
        send_command(buf - buffer);

        // Check the result
        if (values) {
            char *expect_str = cmp_buff;
            for (int i = 0; i < count; i++)
                expect_str += sprintf(expect_str, "VALUE %.*s 0 %lu\r\n%.*s\r\nEND\r\n", (int) keys[i].second, keys[i].first, values[i].second,  (int) values[i].second, values[i].first);

            expect(cmp_buff, expect_str - cmp_buff);
        } else {
            expect(NULL, 0);
        }
    }

    virtual void append(const char *key, size_t key_size,
                        const char *value, size_t value_size) {
        // Setup the text command
        char *buf = buffer;
        buf += sprintf(buf, "append ");
        memcpy(buf, key, key_size);
        buf += key_size;
        buf += sprintf(buf, " 0 0 %d\r\n", (int)value_size);
        memcpy(buf, value, value_size);
        buf += value_size;
        buf += sprintf(buf, "\r\n");

        // Send it on its merry way to the server
        send_command(buf - buffer);

        // Check the result
        expect(NULL, 0);
    }

    virtual void prepend(const char *key, size_t key_size,
                          const char *value, size_t value_size) {
        // Setup the text command
        char *buf = buffer;
        buf += sprintf(buf, "prepend ");
        memcpy(buf, key, key_size);
        buf += key_size;
        buf += sprintf(buf, " 0 0 %d\r\n", (int)value_size);
        memcpy(buf, value, value_size);
        buf += value_size;
        buf += sprintf(buf, "\r\n");

        // Send it on its merry way to the server
        send_command(buf - buffer);

        // Check the result
        expect(NULL, 0);
    }

private:
    void send_command(int total) {
        int count = 0, res = 0;
        do {
            res = write(sockfd, buffer + count, total - count);
            if(res < 0) {
                int err = errno;
                fprintf(stderr, "Could not send command (%d)\n", errno);
                exit(-1);
            }
            count += res;
        } while(count < total);
    }

    void expect(char *str, int len) {
        int nread = 0;
        char *buf = buffer;
        char *strhd = str;
        bool done = false;
        do {
            // Read
            int res = ::read(sockfd, buf, 256);
            if(res <= 0) {
                int err = errno;
                fprintf(stderr, "Could not read from socket (%d)\n", errno);
                exit(-1);
            }

            // Search
            if (str) {
                if (res + nread > len || memcmp(buf, strhd, res) != 0) {
                    fprintf(stderr, "Incorrect data in the socket:\nReceived:%.*s\nExpected:%.*s\n", res + nread, buffer, len, str);
                    exit(-1);
                }
            }
            else {
                void *ptr = memmem(buf, res, "\r\n", 2);
                if(ptr)
                    done = true;
            }

            nread += res;
            buf += res;
            strhd += res;

            if (nread == len)
                done = true;

        } while(!done);
    }

private:
    int sockfd;
    char buffer[1024 * 10], cmp_buff[1024 * 10];
};


#endif // __MEMCACHED_SOCK_PROTOCOL_HPP__

