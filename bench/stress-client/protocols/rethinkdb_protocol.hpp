#ifndef __STRESS_CLIENT_PROTOCOLS_RETHINKDB_PROTOCOL_HPP__
#define __STRESS_CLIENT_PROTOCOLS_RETHINKDB_PROTOCOL_HPP__

#include <assert.h>
#include <errno.h>
#include <netdb.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/socket.h>

#include "query_language.pb.h"

#define MAX_PROTOBUF_SIZE (1024*1024)
#define RDB_TABLE_NAME "Welcome-rdb"
#define PRIMARY_KEY_NAME "id"

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

        // initialize send mutex
        pthread_mutex_init(&send_mutex, NULL);
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

        // destroy send mutex
        pthread_mutex_destroy(&send_mutex);
    }

    virtual void remove(const char *key, size_t key_size) {
        assert(!exist_outstanding_pipeline_reads());

        // generate query
        Query *query = new Query;
        query->set_type(Query::WRITE);
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
    }

    virtual void update(const char *key, size_t key_size,
                        const char *value, size_t value_size) {
        assert(!exist_outstanding_pipeline_reads());

        // generate query
        Query *query = new Query;
        query->set_type(Query::WRITE);
        WriteQuery *write_query = query->mutable_write_query();
        write_query->set_type(WriteQuery::POINTUPDATE);
        WriteQuery::PointUpdate *point_update = write_query->mutable_point_update();
        TableRef *table_ref = point_update->mutable_table_ref();
        table_ref->set_table_name(RDB_TABLE_NAME);
        point_update->set_attrname(PRIMARY_KEY_NAME);
        Term *term_key = point_update->mutable_key();
        term_key->set_type(Term::STRING);
        term_key->set_valuestring(std::string(key, key_size));
        Mapping *mapping = point_update->mutable_mapping();
        mapping->set_arg("row"); // unused
        Term *body = mapping->mutable_body();
        body->set_type(Term::JSON);
        std::string json_update = std::string("{\"") + std::string(PRIMARY_KEY_NAME) + std::string("\" : \"") + std::string(key, key_size) + std::string("\", \"val\" : \"") + std::string(value, value_size) + std::string("\"}");
        body->set_jsonstring(json_update);

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
    }

    virtual void insert(const char *key, size_t key_size,
                        const char *value, size_t value_size) {
        assert(!exist_outstanding_pipeline_reads());

        // generate query
        Query *query = new Query;
        query->set_type(Query::WRITE);
        WriteQuery *write_query = query->mutable_write_query();
        write_query->set_type(WriteQuery::INSERT);
        WriteQuery::Insert *insert = write_query->mutable_insert();
        TableRef *table_ref = insert->mutable_table_ref();
        table_ref->set_table_name(RDB_TABLE_NAME);
        Term *term = insert->add_terms();
        term->set_type(Term::JSON);
        std::string json_insert = std::string("{\"") + std::string(PRIMARY_KEY_NAME) + std::string("\" : \"") + std::string(key, key_size) + std::string("\", \"val\" : \"") + std::string(value, value_size) + std::string("\"}");
        term->set_jsonstring(json_insert);

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
    }

    // TODO: make this more efficient instead of just doing a bunch of reads in a row
    virtual void read(payload_t *keys, int count, payload_t *values = NULL) {
        assert(!exist_outstanding_pipeline_reads());

        for (int i = 0; i < count; i++) {
            // generate query
            Query *query = new Query;
            query->set_type(Query::READ);
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
        }
    }

    virtual void enqueue_read(payload_t *keys, int count, payload_t *values = NULL) {
        for (int i = 0; i < count; i++) {
            // generate query
            Query *query = new Query;
            query->set_type(Query::READ);
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

            outstanding_reads--;
        }
    }

    virtual void range_read(char* lkey, size_t lkey_size, char* rkey, size_t rkey_size, int count_limit, payload_t *values = NULL) {
        assert(!exist_outstanding_pipeline_reads());

        // generate query
        Query *query = new Query;
        query->set_type(Query::READ);
        ReadQuery *read_query = query->mutable_read_query();
        Term *term = read_query->mutable_term();
        term->set_type(Term::CALL);
        Term::Call *call = term->mutable_call();
        Builtin *builtin = call->mutable_builtin();
        builtin->set_type(Builtin::RANGE);
        Builtin::Range *range = builtin->mutable_range();
        range->set_attrname(PRIMARY_KEY_NAME);
        Term *lowerbound = range->mutable_lowerbound();
        lowerbound->set_type(Term::STRING);
        lowerbound->set_valuestring(std::string(lkey, lkey_size));
        Term *upperbound = range->mutable_upperbound();
        upperbound->set_type(Term::STRING);
        upperbound->set_valuestring(std::string(rkey, rkey_size));
        Term *args = call->add_args();
        args->set_type(Term::TABLE);
        Term::Table *table = args->mutable_table();
        TableRef *table_ref = table->mutable_table_ref();
        table_ref->set_table_name(RDB_TABLE_NAME);

        send_query(query);

        // get response
        Response *response = new Response;
        get_response(response);
        if (response->token() != query->token()) {
            fprintf(stderr, "Range read response token %ld did not match query token %ld.\n", response->token(), query->token());
        }
        if (response->status_code() != Response::SUCCESS_JSON) {
            fprintf(stderr, "Failed to range read key %s to key %s: %s\n", lkey, rkey, response->error_message().c_str());
        }
        if (values) {
            for (int i = 0; i < count_limit; i++) {
                // TODO: use some JSON parser instead of this
                int last_quote = (int) response->response(i).find_last_of('"');
                int second_to_last_quote = (int) response->response(i).find_last_of(last_quote - 1);
                assert(last_quote >= 0 && last_quote < response->response(i).length());
                assert(second_to_last_quote >= 0 && second_to_last_quote < response->response(i).length());
                std::string result = response->response(i).substr(second_to_last_quote + 1, last_quote - second_to_last_quote - 1);
                if (std::string(values[i].first, values[i].second) != result) {
                    fprintf(stderr, "Range read failed: wanted %s but got %s for some key.\n", values[i].first, result.c_str());
                }
            }
        }
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
        pthread_mutex_lock(&send_mutex);
        printf("got mutex for token %d\n", token_index);

        // set token
        query->set_token(token_index++);

        // serialize query
        std::string query_serialized;
        if (!query->SerializeToString(&query_serialized)) {
            fprintf(stderr, "faild to serialize query\n");
            fprintf(stderr, "%s\n", query->DebugString().c_str());
            exit(-1);
        }

        // TODO: remove debug output
        printf("About to send query.\n");
        printf("%s\n", query->DebugString().c_str());

        // send message
        int size = int(query_serialized.length());
        send_all((char *) &size, sizeof(size));
        send_all(query_serialized.c_str(), size);
        printf("about to release mutex for token %d\n", token_index - 1);
        pthread_mutex_unlock(&send_mutex);
    }

    // TODO: RethinkDB apparently does not guarantee that responses come back in the same order
    // that queries were sent. Thus, we should keep some storage of received Responses
    // and when we want to check the response to a particular query, we wait for the storage to
    // gain the response with the token we want.
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

        // TODO: remove debug output
        printf("Received response!\n");
        printf("%s\n", response->DebugString().c_str());
    }

    bool exist_outstanding_pipeline_reads() {
        return outstanding_reads != 0;
    }

private:
    static int token_index;
    static pthread_mutex_t send_mutex;
    int sockfd;
    int outstanding_reads;
    char buffer[MAX_PROTOBUF_SIZE];
} ;

int rethinkdb_protocol_t::token_index = 0;
pthread_mutex_t rethinkdb_protocol_t::send_mutex;

#endif // __STRESS_CLIENT_PROTOCOLS_RETHINKDB_PROTOCOL_HPP__
