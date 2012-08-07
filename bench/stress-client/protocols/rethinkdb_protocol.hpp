#ifndef __STRESS_CLIENT_PROTOCOLS_RETHINKDB_PROTOCOL_HPP__
#define __STRESS_CLIENT_PROTOCOLS_RETHINKDB_PROTOCOL_HPP__

#include <assert.h>
#include <time.h>
#include <errno.h>
#include <map>
#include <netdb.h>
#include <queue>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/socket.h>

#include "query_language.pb.h"

#define MAX_PROTOBUF_SIZE (1024*1024)
#define RDB_TABLE_NAME "Welcome-rdb"
#define PRIMARY_KEY_NAME "id"

struct rethinkdb_protocol_t : protocol_t {
    rethinkdb_protocol_t(const char *conn_str) : sockfd(-1) {
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

        // set up readfds for socket_ready()
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);

        // wait until started up (by inserting until one is successful)
        bool success = false;
        do {
            Query *query = new Query;
            generate_insert_query(query, "0", 1, "0", 1);

            send_query(query);

            Response *response = new Response;
            get_response(response, query->token());
            success = response->status_code() == Response::SUCCESS_JSON;
        } while (!success);
        remove("0", 1);
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
        generate_point_delete_query(query, key, key_size);

        send_query(query);

        // get response
        Response *response = new Response;
        get_response(response, query->token());
        if (response->token() != query->token()) {
            fprintf(stderr, "Delete response token %ld did not match query token %ld.\n", response->token(), query->token());
            throw protocol_error_t("Delete response token mismatch.");
        }
        if (response->status_code() != Response::SUCCESS_JSON) {
            fprintf(stderr, "Failed to remove key %s: %s\n", key, response->error_message().c_str());
            throw protocol_error_t("Failed to successfully delete.");
        }
    }

    virtual void update(const char *key, size_t key_size,
                        const char *value, size_t value_size) {
        assert(!exist_outstanding_pipeline_reads());

        // generate query
        Query *query = new Query;
        Mapping *mapping = new Mapping;
        mapping->set_arg("row"); // unused
        Term *body = mapping->mutable_body();
        body->set_type(Term::JSON);
        std::string json_update = std::string("{\"") + std::string(PRIMARY_KEY_NAME) + std::string("\" : \"") + std::string(key, key_size) + std::string("\", \"val\" : \"") + std::string(value, value_size) + std::string("\"}");
        body->set_jsonstring(json_update);
        generate_point_update_query(query, key, key_size, value, value_size, *mapping);

        send_query(query);

        // get response
        Response *response = new Response;
        get_response(response, query->token());
        if (response->token() != query->token()) {
            fprintf(stderr, "Insert response token %ld did not match query token %ld.\n", response->token(), query->token());
            throw protocol_error_t("Update response token mismatch.");
        }
        if (response->status_code() != Response::SUCCESS_JSON) {
            fprintf(stderr, "Failed to insert key %s, value %s: %s\n", key, value, response->error_message().c_str());
            throw protocol_error_t("Failed to successfully update.");
        }
    }

    virtual void insert(const char *key, size_t key_size,
                        const char *value, size_t value_size) {
        assert(!exist_outstanding_pipeline_reads());

        // generate query
        Query *query = new Query;
        generate_insert_query(query, key, key_size, value, value_size);

        send_query(query);

        // get response
        Response *response = new Response;
        get_response(response, query->token());
        if (response->token() != query->token()) {
            fprintf(stderr, "Insert response token %ld did not match query token %ld.\n", response->token(), query->token());
            throw protocol_error_t("Insert response token mismatch.");
        }
        if (response->status_code() != Response::SUCCESS_JSON) {
            fprintf(stderr, "Failed to insert key %s, value %s: %s\n", key, value, response->error_message().c_str());
            throw protocol_error_t("Failed to successfully insert.");
        }
    }

    virtual void read(payload_t *keys, int count, payload_t *values = NULL) {
        assert(!exist_outstanding_pipeline_reads());

        for (int i = 0; i < count; i++) {
            // generate query
            Query *query = new Query;
            generate_read_query(query, keys[i].first, keys[i].second);

            send_query(query);

            // get response
            Response *response = new Response;
            get_response(response, query->token());
            if (response->token() != query->token()) {
                fprintf(stderr, "Read response token %ld did not match query token %ld.\n", response->token(), query->token());
                throw protocol_error_t("Read response token mismatch.");
            }
            if (response->status_code() != Response::SUCCESS_JSON) {
                fprintf(stderr, "Failed to read key %s: %s\n", keys[i].first, response->error_message().c_str());
                throw protocol_error_t("Failed to successfully read.");
            }
            if (values) {
                std::string result = get_value(response->response(0));
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
            generate_read_query(query, keys[i].first, keys[i].second);

            send_query(query);

            read_tokens.push(query->token());
        }
    }

    // returns whether all the reads were completed
    virtual bool dequeue_read_maybe(payload_t *keys, int count, payload_t *values = NULL) { 
        bool done = true;
        for (int i = 0; i < count; i++) {
            if (read_tokens.front() == -1) {
                read_tokens.push(-1);
                read_tokens.pop();
            }

            // get response
            Response *response = new Response;
            bool received = get_response_maybe(response, read_tokens.front());
            if (!received) {
                done = false;
                read_tokens.push(read_tokens.front());
                read_tokens.pop();
                continue;
            }

            if (response->status_code() != Response::SUCCESS_JSON) {
                fprintf(stderr, "Failed to read key %s: %s\n", keys[i].first, response->error_message().c_str());
                throw protocol_error_t("Failed to successfully read.");
            }
            if (values) {
                std::string result = get_value(response->response(0));
                if (std::string(values[i].first, values[i].second) != result) {
                    fprintf(stderr, "Read failed: wanted %s but got %s for key %s.\n", values[i].first, result.c_str(), keys[i].first);
                }
            }

            read_tokens.push(-1); // signifies a completed read
            read_tokens.pop();
        }

        // empty the read queue if done
        if (done) {
            while (read_tokens.size()) {
                read_tokens.pop();
            }
        }

        return done;
    }

    virtual void dequeue_read(payload_t *keys, int count, payload_t *values = NULL) {
        for (int i = 0; i < count; i++) {
            if (read_tokens.front() == -1) {
                continue;
            }

            // get response
            Response *response = new Response;
            get_response(response, read_tokens.front());
            if (response->status_code() != Response::SUCCESS_JSON) {
                fprintf(stderr, "Failed to read key %s: %s\n", keys[i].first, response->error_message().c_str());
                throw protocol_error_t("Failed to successfully read.");
            }
            if (values) {
                std::string result = get_value(response->response(0));
                if (std::string(values[i].first, values[i].second) != result) {
                    fprintf(stderr, "Read failed: wanted %s but got %s for key %s.\n", values[i].first, result.c_str(), keys[i].first);
                }
            }

            read_tokens.pop();
        }
    }

    virtual void range_read(char* lkey, size_t lkey_size, char* rkey, size_t rkey_size, int count_limit, payload_t *values = NULL) {
        assert(!exist_outstanding_pipeline_reads());

        // generate query
        Query *query = new Query;
        generate_range_read_query(query, lkey, lkey_size, rkey, rkey_size, count_limit);

        send_query(query);

        // get response
        Response *response = new Response;
        get_response(response, query->token());
        if (response->token() != query->token()) {
            fprintf(stderr, "Range read response token %ld did not match query token %ld.\n", response->token(), query->token());
            throw protocol_error_t("Range read response token mismatch.");
        }
        if (response->status_code() == Response::SUCCESS_PARTIAL) {
            Query *stop_query = new Query;
            generate_stop_query(stop_query);
            send_query(stop_query, query->token());

            Response *stop_response = new Response;
            get_response(stop_response, stop_query->token());
            if (stop_response->token() != stop_query->token()) {
                fprintf(stderr, "Stop response token %ld did not match query token %ld.\n", stop_response->token(), stop_query->token());
                throw protocol_error_t("Stop response token mismatch.");
            }
            if (stop_response->status_code() != Response::SUCCESS_EMPTY) {
                fprintf(stderr, "Failed to stop partial stream.");
                throw protocol_error_t("Failed to successfully stop.");
            }
        } else if (response->status_code() != Response::SUCCESS_STREAM) {
            fprintf(stderr, "Failed to range read key %s to key %s: %s\n", lkey, rkey, response->error_message().c_str());
            throw protocol_error_t("Failed to successfully range read.");
        }

        if (values) {
            for (int i = 0; i < count_limit; i++) {
                std::string result = get_value(response->response(i));
                if (std::string(values[i].first, values[i].second) != result) {
                    fprintf(stderr, "Range read failed: wanted %s but got %s for some key.\n", values[i].first, result.c_str());
                }
            }
        }
    }

    virtual void append(const char *key, size_t key_size,
                        const char *value, size_t value_size) {
        fprintf(stderr, "APPEND NOT YET IMPLEMENTED. EXITING.\n");
        exit(-1);
    }

    virtual void prepend(const char *key, size_t key_size,
                          const char *value, size_t value_size) {
        fprintf(stderr, "PREPEND NOT YET IMPLEMENTED. EXITING.\n");
        exit(-1);
    }

private:
    void generate_stop_query(Query *query) {
        query->set_type(Query::STOP);
    }

    void generate_point_delete_query(Query *query, const char *key, size_t key_size) {
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
    }

    void generate_point_update_query(Query *query, const char *key, size_t key_size,
                                                   const char *value, size_t value_size, const Mapping &input_mapping) {
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
        *mapping = input_mapping;
    }

    void generate_insert_query(Query *query, const char *key, size_t key_size,
                                             const char *value, size_t value_size) {
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
    }

    void generate_read_query(Query *query, const char *key, size_t key_size) {
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
        term_key->set_valuestring(std::string(key, key_size));
    }

    void generate_range_read_query(Query *query, char *lkey, size_t lkey_size,
                                                 char *rkey, size_t rkey_size, int count_limit) {
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
        read_query->set_max_chunk_size(count_limit);
    }

    // Returns true if there is something to be read
    bool socket_ready() {
        int res = select(sockfd + 1, &readfds, NULL, NULL, NULL);
        if (res < 0) {
            perror("select() failed");
            exit(-1);
        }
        return FD_ISSET(sockfd, &readfds);
    }
    
    void send_all(const char *buf, int size, double timeout = 1.0) {
        double start_time = clock();
        int total_sent = 0, sent;
        while (total_sent < size && (clock() - start_time) / CLOCKS_PER_SEC < timeout) {
            sent = send(sockfd, buf + total_sent, size - total_sent, 0);
            if (sent < 0) {
                perror("send() failed");
                exit(-1);
            }
            total_sent += sent;
        }
        if (total_sent < size) {
            fprintf(stderr, "Failed to send_all within time limit of %.3lf seconds.\n", timeout);
            throw protocol_error_t("Failed to send_all within time limit.");
        }
    }

    void recv_all(char *buf, int size, double timeout = 1.0) {
        double start_time = clock();
        int total_received = 0, received;
        while (total_received < size && (clock() - start_time) / CLOCKS_PER_SEC < timeout) {
            received = recv(sockfd, buf + total_received, size - total_received, 0);
            if (received < 0) {
                perror("recv() failed");
                exit(-1);
            }
            total_received += received;
        }
        if (total_received < size) {
            fprintf(stderr, "Failed to recv_all within time limit of %.3lf seconds.\n", timeout);
            throw protocol_error_t("Failed to recv_all within time limit.");
        }
    }

    void send_query(Query *query, int set_token_index = -1) {
        // set token
        if (set_token_index < 0) {
            query->set_token(token_index++);
        } else {
            query->set_token(set_token_index);
        }

        // serialize query
        std::string query_serialized;
        if (!query->SerializeToString(&query_serialized)) {
            fprintf(stderr, "Failed to serialize query.\n");
            fprintf(stderr, "%s\n", query->DebugString().c_str());
            throw protocol_error_t("Failed to serialize query.");
        }

        // send message
        int size = int(query_serialized.length());
        send_all((char *) &size, sizeof(size));
        send_all(query_serialized.c_str(), size);
    }

    void get_response(Response *response, int64_t target_token) {
        map<int64_t, Response>::iterator iterator;
        while ((iterator = response_bucket.find(target_token)) == response_bucket.end()) {
            recv_response(response);
        }
        *response = iterator->second;
        response_bucket.erase(iterator);
    }

    bool get_response_maybe(Response *response, int64_t target_token) {
        map<int64_t, Response>::iterator iterator;
        while ((iterator = response_bucket.find(target_token)) == response_bucket.end()) {
            if (socket_ready()) {
                recv_response(response);
            } else {
                return false;
            }
        }
        *response = iterator->second;
        response_bucket.erase(iterator);
    }

    void recv_response(Response *response) {
        // get message
        int size;
        recv_all((char *) &size, sizeof(size));
        recv_all(buffer, size);

        // unserialize
        if (!response->ParseFromString(std::string(buffer, size))) {
            fprintf(stderr, "Failed to unserialize response.\n");
            throw protocol_error_t("Failed to unserialize response.");
        }

        response_bucket[response->token()] = *response;
    }

    bool exist_outstanding_pipeline_reads() {
        return read_tokens.size() != 0;
    }

    // takes a JSON string and returns the string in the last set of quotes
    // useful for retrieving the last value, and probably faster than using a JSON parser
    std::string get_value(const std::string &json_string) {
        if (json_string == "null") {
            return json_string;
        }
        int last_quote = (int) json_string.find_last_of('"');
        int second_to_last_quote = (int) json_string.find_last_of('"', last_quote - 1);
        assert(last_quote >= 0 && last_quote < json_string.length());
        assert(second_to_last_quote >= 0 && second_to_last_quote < json_string.length());
        return json_string.substr(second_to_last_quote + 1, last_quote - second_to_last_quote - 1);
    }

private:
    int64_t token_index;
    int sockfd;
    fd_set readfds;
    char buffer[MAX_PROTOBUF_SIZE];
    queue<int64_t> read_tokens; // used for keeping track of enqueued reads
    map<int64_t, Response> response_bucket; // maps the token number to the corresponding response
} ;

#endif // __STRESS_CLIENT_PROTOCOLS_RETHINKDB_PROTOCOL_HPP__
