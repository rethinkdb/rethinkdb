
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
    
    virtual void connect(config_t *config) {
        // Setup the host/port data structures
        struct sockaddr_in sin;
        struct hostent *host = gethostbyname(config->host);
        memcpy(&sin.sin_addr.s_addr, host->h_addr, host->h_length);
        sin.sin_family = AF_INET;
        sin.sin_port = htons(config->port);

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
        expect(false);
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
        expect(false);
    }
    
    virtual void read(payload_t *keys, int count) {
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
        expect(true);
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

    void expect(bool fread) {
        int nread = 0;
        char *buf = buffer;
        do {
            // Read
            int res = ::read(sockfd, buf, 256);
            if(res <= 0) {
                int err = errno;
                fprintf(stderr, "Could not read from socket (%d)\n", errno);
                exit(-1);
            }
            nread += res;
            buf += res;

            // Search
            void *ptr = NULL;
            if(fread) {
                ptr = memmem(buffer, nread, "END\r\n", 5);
            } else {
                ptr = memmem(buffer, nread, "\r\n", 2);
            }
            
            if(ptr) {
                return;
            }
        } while(true);
    }

private:
    int sockfd;
    char buffer[1024 * 10];
};


#endif // __MEMCACHED_SOCK_PROTOCOL_HPP__

