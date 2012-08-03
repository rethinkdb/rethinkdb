#ifndef __STRESS_CLIENT_PROTOCOLS_RETHINKDB_PROTOCOL_HPP__
#define __STRESS_CLIENT_PROTOCOLS_RETHINKDB_PROTOCOL_HPP__

#include <assert.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/socket.h>

#include "query_language.pb.h"

#define MAX_PROTOBUF_SIZE (1024*1024)
#define RDB_TABLE_NAME "Welcome-rdb"
#define PRIMARY_KEY_NAME "id"

struct rethinkdb_protocol_t : protocol_t {
    rethinkdb_protocol_t(const char *conn_str) : sockfd(-1), outstanding_reads(0), token_index(0) {
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

    virtual void remove(const char *key, size_t key_size) {
        assert(!exist_outstanding_pipeline_reads());

        // generate query
        Query *query = new Query;
        query->set_type(Query::WRITE);
        query->set_token(token_index++);
        WriteQuery *write_query = query->mutable_write_query();
        write_query->set_type(WriteQuery::POINTDELETE);
        WriteQuery::PointDelete *point_delete = write_query->mutable_point_delete();
        TableRef *table_ref = point_delete->mutable_table_ref();
        table_ref->set_table_name(RDB_TABLE_NAME);
        point_delete->set_attrname(PRIMARY_KEY_NAME);
        Term *term_key = point_delete->mutable_key();
        term_key->set_type(Term::STRING);
        term_key->set_valuestring(std::string(key, key_size));

        send_query(query);

        // get response
        Response *response = new Response;
        get_response(response);
        if (response->token() != query->token()) {
            fprintf(stderr, "Delete response token %ld did not match query token %ld.\n", response->token(), query->token());
        }
        if (response->status_code() != Response::SUCCESS_JSON) {
            fprintf(stderr, "Failed to remove key %s: %s\n", key, response->error_message().c_str());
        }

        // temporary debug output
        printf("%s\n", query->DebugString().c_str());
        printf("%s\n", response->DebugString().c_str());
    }

    virtual void update(const char *key, size_t key_size,
                        const char *value, size_t value_size) {
        insert(key, key_size, value, value_size); // TODO: use PointUpdate or PointMutate instead?
    }

    virtual void insert(const char *key, size_t key_size,
                        const char *value, size_t value_size) {
        assert(!exist_outstanding_pipeline_reads());

        // generate query
        Query *query = new Query;
        query->set_type(Query::WRITE);
        query->set_token(token_index++);
        WriteQuery *write_query = query->mutable_write_query();
        write_query->set_type(WriteQuery::INSERT);
        WriteQuery::Insert *insert = write_query->mutable_insert();
        TableRef *table_ref = insert->mutable_table_ref();
        table_ref->set_table_name(RDB_TABLE_NAME);
        Term *term = insert->add_terms();
        term->set_type(Term::JSON);
        std::string json_insert = std::string("{\"") + std::string(PRIMARY_KEY_NAME) + std::string("\" : \"") + std::string(key, key_size) + std::string("\", \"val\" : \"") + std::string(value, value_size) + std::string("\"}");
        term->set_jsonstring(json_insert);
        printf("%s\n", query->DebugString().c_str());

        send_query(query);

        // get response
        Response *response = new Response;
        get_response(response);
        if (response->token() != query->token()) {
            fprintf(stderr, "Insert response token %ld did not match query token %ld.\n", response->token(), query->token());
        }
        if (response->status_code() != Response::SUCCESS_JSON) {
            fprintf(stderr, "Failed to insert key %s, value %s: %s\n", key, value, response->error_message().c_str());
        }

        // temporary debug output
        printf("%s\n", query->DebugString().c_str());
        printf("%s\n", response->DebugString().c_str());
    }

    // TODO: make this more efficient instead of just doing a bunch of reads in a row
    virtual void read(payload_t *keys, int count, payload_t *values = NULL) {
        assert(!exist_outstanding_pipeline_reads());

        for (int i = 0; i < count; i++) {
            // generate query
            Query *query = new Query;
            query->set_type(Query::READ);
            query->set_token(token_index++);
            ReadQuery *read_query = query->mutable_read_query();
            Term *term = read_query->mutable_term();
            term->set_type(Term::GETBYKEY);
            Term::GetByKey *get_by_key = term->mutable_get_by_key();
            TableRef *table_ref = get_by_key->mutable_table_ref();
            table_ref->set_table_name(RDB_TABLE_NAME);
            get_by_key->set_attrname(PRIMARY_KEY_NAME);
            Term *term_key = get_by_key->mutable_key();
            term_key->set_type(Term::STRING);
            term_key->set_valuestring(std::string(keys[i].first, keys[i].second));

            send_query(query);

            // get response
            Response *response = new Response;
            get_response(response);
            if (response->token() != query->token()) {
                fprintf(stderr, "Read response token %ld did not match query token %ld.\n", response->token(), query->token());
            }
            if (response->status_code() != Response::SUCCESS_JSON) {
                fprintf(stderr, "Failed to read key %s: %s\n", keys[i].first, response->error_message().c_str());
            }
            if (values) {
                // TODO: use some JSON parser instead of this
                int last_quote = (int) response->response(0).find_last_of('"');
                int second_to_last_quote = (int) response->response(0).find_last_of(last_quote - 1);
                assert(last_quote >= 0 && last_quote < response->response(0).length());
                assert(second_to_last_quote >= 0 && second_to_last_quote < response->response(0).length());
                std::string result = response->response(0).substr(second_to_last_quote + 1, last_quote - second_to_last_quote - 1);
                if (std::string(values[i].first, values[i].second) != result) {
                    fprintf(stderr, "Read failed: wanted %s but got %s for key %s.\n", values[i].first, result.c_str(), keys[i].first);
                }
            }

            // temporary debug output
            printf("%s\n", query->DebugString().c_str());
            printf("%s\n", response->DebugString().c_str());
        }
    }

    virtual void enqueue_read(payload_t *keys, int count, payload_t *values = NULL) {
        for (int i = 0; i < count; i++) {
            // generate query
            Query *query = new Query;
            query->set_type(Query::READ);
            query->set_token(token_index++);
            ReadQuery *read_query = query->mutable_read_query();
            Term *term = read_query->mutable_term();
            term->set_type(Term::GETBYKEY);
            Term::GetByKey *get_by_key = term->mutable_get_by_key();
            TableRef *table_ref = get_by_key->mutable_table_ref();
            table_ref->set_table_name(RDB_TABLE_NAME);
            get_by_key->set_attrname(PRIMARY_KEY_NAME);
            Term *term_key = get_by_key->mutable_key();
            term_key->set_type(Term::STRING);
            term_key->set_valuestring(std::string(keys[i].first, keys[i].second));

            send_query(query);

            // temporary debug output
            printf("%s\n", query->DebugString().c_str());

            outstanding_reads++;
        }
    }

    // TODO: implement this method properly
    virtual bool dequeue_read_maybe(payload_t *keys, int count, payload_t *values = NULL) { 
        dequeue_read(keys, count, values);
        return true;
    }

    virtual void dequeue_read(payload_t *keys, int count, payload_t *values = NULL) {
        for (int i = 0; i < count; i++) {
            // get response
            Response *response = new Response;
            get_response(response);
            if (response->status_code() != Response::SUCCESS_JSON) {
                fprintf(stderr, "Failed to read key %s: %s\n", keys[i].first, response->error_message().c_str());
            }
            if (values) {
                // TODO: use some JSON parser instead of this
                int last_quote = (int) response->response(0).find_last_of('"');
                int second_to_last_quote = (int) response->response(0).find_last_of(last_quote - 1);
                assert(last_quote >= 0 && last_quote < response->response(0).length());
                assert(second_to_last_quote >= 0 && second_to_last_quote < response->response(0).length());
                std::string result = response->response(0).substr(second_to_last_quote + 1, last_quote - second_to_last_quote - 1);
                if (std::string(values[i].first, values[i].second) != result) {
                    fprintf(stderr, "Read failed: wanted %s but got %s for key %s.\n", values[i].first, result.c_str(), keys[i].first);
                }
            }

            // temporary debug output
            printf("%s\n", response->DebugString().c_str());

            outstanding_reads--;
        }
    }

    virtual void range_read(char* lkey, size_t lkey_size, char* rkey, size_t rkey_size, int count_limit, payload_t *values = NULL) {
        fprintf(stderr, "RANGE READ NOT YET IMPLEMENTED\n");
    }

    virtual void append(const char *key, size_t key_size,
                        const char *value, size_t value_size) {
        fprintf(stderr, "APPEND NOT YET IMPLEMENTED\n");
    }

    virtual void prepend(const char *key, size_t key_size,
                          const char *value, size_t value_size) {
        fprintf(stderr, "PREPEND NOT YET IMPLEMENTED\n");
    }

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

    void get_response(Response *response) {
        // get message
        int size;
        recv_all((char *) &size, sizeof(size));
        recv_all(buffer, size);

        // unserialize
        if (!response->ParseFromString(std::string(buffer, size))) {
            fprintf(stderr, "failed to unserialize response\n");
            exit(-1);
        }
    }

    bool exist_outstanding_pipeline_reads() {
        return outstanding_reads != 0;
    }

private:
    int sockfd;
    int outstanding_reads;
    int token_index;
    char buffer[MAX_PROTOBUF_SIZE];
} ;

#endif // __STRESS_CLIENT_PROTOCOLS_RETHINKDB_PROTOCOL_HPP__
