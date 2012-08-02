#ifndef __STRESS_CLIENT_PROTOCOLS_RETHINKDB_PROTOCOL_HPP__
#define __STRESS_CLIENT_PROTOCOLS_RETHINKDB_PROTOCOL_HPP__

#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/socket.h>

#include "query_language.pb.h"

struct rethinkdb_protocol_t : protocol_t {
    rethinkdb_protocol_t(const char *conn_str) : sockfd(-1), outstanding_reads(0) {
        // initialize the socket
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            perror("socket() failed");
            exit(-1);
        }

        // parse the host string
        char _host[MAX_HOST];
        strncpy(_host, conn_str, MAX_HOST);

        int port;
        if (char *_port = strchr(_host, ':')) {
            *_port = '\0';
            _port++;
            port = atoi(_port);
            if (port == 0) {
                fprintf(stderr, "Cannot parse port string: \"%s\".\n", _port);
                exit(-1);
            }
        }

        // set up socket
        struct sockaddr_in sin;
        struct hostent *host = gethostbyname(_host);
        if (!host) {
            herror("gethostbyname() failed");
            exit(-1);
        }
        memcpy(&sin.sin_addr.s_addr, host->h_addr, host->h_length);
        sin.sin_family = AF_INET;
        sin.sin_port = htons(port);

        // connect to server
        int res = ::connect(sockfd, (struct sockaddr *)&sin, sizeof(sin));
        if (res < 0) {
            perror("connect() failed");
            exit(-1);
        }
    }

    virtual ~rethinkdb_protocol_t() {
        // close the socket
        if (sockfd != -1) {
            int res = close(sockfd);
            if (res != 0) {
                perror("close() failed");
                exit(-1);
            }
        }
    }

    virtual void remove(const char *key, size_t key_size) {};
    virtual void update(const char *key, size_t key_size,
                        const char *value, size_t value_size) {};
    virtual void insert(const char *key, size_t key_size,
                        const char *value, size_t value_size) {};

    virtual void read(payload_t *keys, int count, payload_t *values = NULL) {};

    virtual void enqueue_read(payload_t *keys, int count, payload_t *values = NULL) {
        read(keys, count, values);
    }

    virtual bool dequeue_read_maybe(payload_t *keys, int count, payload_t *values = NULL) { 
        return true;
    }

    virtual void dequeue_read(payload_t *keys, int count, payload_t *values = NULL) { }

    virtual void range_read(char* lkey, size_t lkey_size, char* rkey, size_t rkey_size, int count_limit, payload_t *values = NULL) {};

    virtual void append(const char *key, size_t key_size,
                        const char *value, size_t value_size) {};

    virtual void prepend(const char *key, size_t key_size,
                          const char *value, size_t value_size) {};

private:
    // TODO: add timeout to send_all and recv_all?
    void send_all(const char *buf, int size) {
        int total_sent = 0, sent;
        while (total_sent < size) {
            sent = send(sockfd, buf + total_sent, size - total_sent, 0);
            if (sent < 0) {
                perror("send() failed");
                exit(-1);
            }
            total_sent += sent;
        }
    }

    void recv_all(char *buf, int size) {
        int total_received = 0, received;
        while (total_received < size) {
            received = recv(sockfd, buf + total_received, size - total_received, 0);
            if (received < 0) {
                perror("recv() failed");
                exit(-1);
            }
            total_received += received;
        }
    }

    void send_query(Query *query) {
        // serialize query
        std::string query_serialized;
        if (!query->SerializeToString(&query_serialized)) {
            fprintf(stderr, "faild to serialize query\n");
            fprintf(stderr, "%s\n", query->DebugString().c_str());
            exit(-1);
        }

        // send message
        int size = int(query_serialized.length());
        send_all((char *) &size, sizeof(size));
        send_all(query_serialized.c_str(), size);
    }

    Response *get_response() {
        Response *response;

        // get message
        int size;
        recv_all((char *) &size, sizeof(size));
        char response_buffer[size];
        recv_all(response_buffer, size);

        // unserialize
        if (!response->ParseFromString(std::string(response_buffer))) {
            fprintf(stderr, "failed to unserialize response\n");
            exit(-1);
        }

        return response;
    }

private:
    int sockfd;
    int outstanding_reads;
} ;

#endif // __STRESS_CLIENT_PROTOCOLS_RETHINKDB_PROTOCOL_HPP__
