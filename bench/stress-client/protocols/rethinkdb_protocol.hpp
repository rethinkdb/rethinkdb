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
#define DB_NAME "Welcome-db"
#define PRIMARY_KEY_NAME "id"
#define SECONDARY_KEY_NAME "val"

#define prepare_point_delete_query(Q) generate_point_delete_query(Q, NULL, 0, PREPARE)
#define finalize_point_delete_query(Q, K, KS) generate_point_delete_query(Q, K, KS, FINALIZE)
#define prepare_point_update_query(Q) generate_point_update_query(Q, NULL, 0, NULL, 0, PREPARE)
#define finalize_point_update_query(Q, K, KS, V, VS) generate_point_update_query(Q, K, KS, V, VS, FINALIZE)
#define prepare_insert_query(Q) generate_insert_query(Q, NULL, 0, NULL, 0, PREPARE)
#define finalize_insert_query(Q, K, KS, V, VS) generate_insert_query(Q, K, KS, V, VS, FINALIZE)
#define prepare_batched_read_query(Q) generate_batched_read_query(Q, NULL, 0, PREPARE)
#define finalize_batched_read_query(Q, K, KS) generate_batched_read_query(Q, K, KS, FINALIZE)
#define prepare_read_query(Q) generate_read_query(Q, NULL, 0, PREPARE)
#define finalize_read_query(Q, K, KS) generate_read_query(Q, K, KS, FINALIZE)
#define prepare_range_read_query(Q) generate_range_read_query(Q, NULL, 0, NULL, 0, 0, PREPARE)
#define finalize_range_read_query(Q, A, AS, B, BS, C) generate_range_read_query(Q, A, AS, B, BS, C, FINALIZE)
#define prepare_stop_query(Q) generate_stop_query(Q, PREPARE)
#define finalize_stop_query(Q) generate_stop_query(Q, FINALIZE)

struct rethinkdb_protocol_t : protocol_t {
    rethinkdb_protocol_t(const char *conn_str) : token_index(0), sockfd(-1), queued_read(-1) {
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

        // Write the initial magic number to identify us as a rethinkdb peer
        int32_t magic_number = 0xaf61ba35;
        send_all(reinterpret_cast<char *>(&magic_number), sizeof(magic_number));

        // set up readfds for socket_ready()
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);

        // set up base protocol buffers
        response = new Response;
        stop_response = new Response;
        base_point_delete_query = new Query;
        prepare_point_delete_query(base_point_delete_query);
        base_point_update_query = new Query;
        prepare_point_update_query(base_point_update_query);
        base_insert_query = new Query;
        prepare_insert_query(base_insert_query);
        base_batched_read_query = new Query;
        prepare_batched_read_query(base_batched_read_query);
        base_read_query = new Query;
        prepare_read_query(base_read_query);
        base_range_read_query = new Query;
        prepare_range_read_query(base_range_read_query);
        base_stop_query = new Query;
        prepare_stop_query(base_stop_query);

        // wait until started up (by inserting until one is successful)
        bool success = false;
        do {
            finalize_insert_query(base_insert_query, "0", 1, "0", 1);

            send_query(base_insert_query);

            get_response(response);
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
        finalize_point_delete_query(base_point_delete_query, key, key_size);

        send_query(base_point_delete_query);

        // get response
        get_response(response);

        if (response->token() != base_point_delete_query->token()) {
            fprintf(stderr, "Delete response token %ld did not match query token %ld.\n", response->token(), base_point_delete_query->token());
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
        finalize_point_update_query(base_point_update_query, key, key_size, value, value_size);

        send_query(base_point_update_query);

        // get response
        get_response(response);

        if (response->token() != base_point_update_query->token()) {
            fprintf(stderr, "Insert response token %ld did not match query token %ld.\n", response->token(), base_point_update_query->token());
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
        finalize_insert_query(base_insert_query, key, key_size, value, value_size);

        send_query(base_insert_query);

        // get response
        get_response(response);

        if (response->token() != base_insert_query->token()) {
            fprintf(stderr, "Insert response token %ld did not match query token %ld.\n", response->token(), base_insert_query->token());
            throw protocol_error_t("Insert response token mismatch.");
        }

        if (response->status_code() != Response::SUCCESS_JSON) {
            fprintf(stderr, "Failed to insert key %s, value %s: %s\n", key, value, response->error_message().c_str());
            throw protocol_error_t("Failed to successfully insert.");
        }
    }

    virtual void read(payload_t *keys, int count, payload_t *values = NULL) {
        assert(!exist_outstanding_pipeline_reads());

        finalize_batched_read_query(base_batched_read_query, keys, count);

        send_query(base_batched_read_query);

        get_response(response);

        if (response->token() != base_batched_read_query->token()) {
            fprintf(stderr, "Read response token %ld did not match query token %ld.\n", response->token(), base_batched_read_query->token());
            throw protocol_error_t("Read response token mismatch.");
        }

        if (response->status_code() != Response::SUCCESS_JSON) {
            fprintf(stderr, "Failed to read a batch of %d keys: %s\n", count, response->error_message().c_str());
            throw protocol_error_t("Failed to successfully read.");
        }

        if (values) {
            int last = response->response(0).find_last_of("}");
            for (int i = count - 1; i >= 0; i--) {
                assert(last >= 0 && last < response->response(0).length());
                std::string result = get_value(response->response(0), last);
                if (std::string(values[i].first, values[i].second) != result) {
                    fprintf(stderr, "Read failed: wanted %s but got %s for key %s.\n", values[i].first, result.c_str(), keys[i].first);
                }
                last = response->response(0).find_last_of("}", last - 1);
            }
        }
    }

    virtual void enqueue_read(payload_t *keys, int count, payload_t *values = NULL) {
        assert(!exist_outstanding_pipeline_reads());

        generate_batched_read_query(base_batched_read_query, keys, count);

        send_query(base_batched_read_query);

        queued_read = base_batched_read_query->token();
    }

    // returns whether all the reads were completed
    virtual bool dequeue_read_maybe(payload_t *keys, int count, payload_t *values = NULL) { 
        bool done = true;

        bool received = get_response_maybe(response);
        if (!received) {
            done = false;
        } else {
            if (response->token() != queued_read) {
                fprintf(stderr, "Read response token %ld did not match query token %ld.\n", response->token(), queued_read);
                throw protocol_error_t("Read response token mismatch.");
            }

            if (response->status_code() != Response::SUCCESS_JSON) {
                fprintf(stderr, "Failed to read a batch of %d keys: %s\n", count, response->error_message().c_str());
                throw protocol_error_t("Failed to successfully read.");
            }

            if (values) {
                int last = response->response(0).find_last_of("}");
                for (int i = count - 1; i >= 0; i--) {
                    assert(last >= 0 && last < response->response(0).length());
                    std::string result = get_value(response->response(0), last);
                    if (std::string(values[i].first, values[i].second) != result) {
                        fprintf(stderr, "Read failed: wanted %s but got %s for key %s.\n", values[i].first, result.c_str(), keys[i].first);
                    }
                    last = response->response(0).find_last_of("}", last);
                }
            }
            queued_read = -1;
        }

        return done;
    }

    virtual void dequeue_read(payload_t *keys, int count, payload_t *values = NULL) {
        get_response(response);

        if (response->token() != queued_read) {
            fprintf(stderr, "Read response token %ld did not match query token %ld.\n", response->token(), queued_read);
            throw protocol_error_t("Read response token mismatch.");
        }

        if (response->status_code() != Response::SUCCESS_JSON) {
            fprintf(stderr, "Failed to read a batch of %d keys: %s\n", count, response->error_message().c_str());
            throw protocol_error_t("Failed to successfully read.");
        }

        if (values) {
            int last = response->response(0).find_last_of("}");
            for (int i = count - 1; i >= 0; i--) {
                assert(last >= 0 && last < response->response(0).length());
                std::string result = get_value(response->response(0), last);
                if (std::string(values[i].first, values[i].second) != result) {
                    fprintf(stderr, "Read failed: wanted %s but got %s for key %s.\n", values[i].first, result.c_str(), keys[i].first);
                }
                last = response->response(0).find_last_of("}", last);
            }
        }

        queued_read = -1;
    }

    virtual void range_read(char* lkey, size_t lkey_size, char* rkey, size_t rkey_size, int count_limit, payload_t *values = NULL) {
        assert(!exist_outstanding_pipeline_reads());

        // generate query
        finalize_range_read_query(base_range_read_query, lkey, lkey_size, rkey, rkey_size, count_limit);

        send_query(base_range_read_query);

        // get response
        get_response(response);
        if (response->token() != base_range_read_query->token()) {
            fprintf(stderr, "Range read response token %ld did not match query token %ld.\n", response->token(), base_range_read_query->token());
            throw protocol_error_t("Range read response token mismatch.");
        }

        if (response->status_code() == Response::SUCCESS_PARTIAL) {
            finalize_stop_query(base_stop_query);
            send_query(base_stop_query, base_range_read_query->token());

            get_response(stop_response);

            if (stop_response->token() != base_stop_query->token()) {
                fprintf(stderr, "Stop response token %ld did not match query token %ld.\n", stop_response->token(), base_stop_query->token());
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
    /* There are three kinds of methods for generating queries. The most intuitive kind is
    the "generate" kind. It takes a blank query and fills in all the information necessary to create
    the desired query. However, this can be slow, as the query must be generated from scratch each
    time. The stress client holds "base" queries, which are partially formed queries. Each time
    a new query must be sent, only a minimal number of fields are filled in before sending. The
    base queries can be prepared using the "prepare" methods and then before being sent off,
    the "finalize" methods can be used to fill in the important information. The combination of
    "prepare" and "finalize" should do the same thing as a single call to "generate". */

    enum query_build_options_t {
        GENERATE = 0,
        PREPARE = 1,
        FINALIZE = 2
    };

    void generate_point_delete_query(Query *query, const char *key, size_t key_size, int option = GENERATE) {
        if (option == PREPARE || option == GENERATE) {
            query->set_type(Query::WRITE);
            WriteQuery *write_query = query->mutable_write_query();
            write_query->set_type(WriteQuery::POINTDELETE);
            WriteQuery::PointDelete *point_delete = write_query->mutable_point_delete();
            TableRef *table_ref = point_delete->mutable_table_ref();
            table_ref->set_table_name(RDB_TABLE_NAME);
            table_ref->set_db_name(DB_NAME);
            point_delete->set_attrname(PRIMARY_KEY_NAME);
        }
        if (option == FINALIZE || option == GENERATE) {
            Term *term_key = query->mutable_write_query()->mutable_point_delete()->mutable_key();
            term_key->set_type(Term::STRING);
            term_key->set_valuestring(std::string(key, key_size));
        }
    }

    void generate_point_update_query(Query *query, const char *key, size_t key_size,
                                                   const char *value, size_t value_size, int option = GENERATE) {
        if (option == PREPARE || option == GENERATE) {
            query->set_type(Query::WRITE);
            WriteQuery *write_query = query->mutable_write_query();
            write_query->set_type(WriteQuery::POINTUPDATE);
            WriteQuery::PointUpdate *point_update = write_query->mutable_point_update();
            TableRef *table_ref = point_update->mutable_table_ref();
            table_ref->set_table_name(RDB_TABLE_NAME);
            table_ref->set_db_name(DB_NAME);
            point_update->set_attrname(PRIMARY_KEY_NAME);
            Term *term_key = point_update->mutable_key();
            term_key->set_type(Term::STRING);
            Mapping *mapping = point_update->mutable_mapping();
            mapping->set_arg("row"); // unused
            Term *body = mapping->mutable_body();
            body->set_type(Term::OBJECT);
            VarTermTuple *object_key = body->add_object();
            object_key->set_var(PRIMARY_KEY_NAME);
            Term *key_term = object_key->mutable_term();
            key_term->set_type(Term::STRING);
            VarTermTuple *object_value = body->add_object();
            object_value->set_var(SECONDARY_KEY_NAME);
            Term *object_term = object_value->mutable_term();
            object_term->set_type(Term::STRING);
        }
        if (option == FINALIZE || option == GENERATE) {
            Term *term_key = query->mutable_write_query()->mutable_point_update()->mutable_key();
            term_key->set_valuestring(std::string(key, key_size));
            Term *key_term = query->mutable_write_query()->mutable_point_update()->mutable_mapping()->mutable_body()->mutable_object(0)->mutable_term();
            key_term->set_valuestring(std::string(key, key_size));
            Term *object_term = query->mutable_write_query()->mutable_point_update()->mutable_mapping()->mutable_body()->mutable_object(1)->mutable_term();
            object_term->set_valuestring(std::string(value, value_size));
        }
    }

    void generate_insert_query(Query *query, const char *key, size_t key_size,
                                             const char *value, size_t value_size, int option = GENERATE) {
        if (option == PREPARE || option == GENERATE) {
            query->set_type(Query::WRITE);
            WriteQuery *write_query = query->mutable_write_query();
            write_query->set_type(WriteQuery::INSERT);
            WriteQuery::Insert *insert = write_query->mutable_insert();
            TableRef *table_ref = insert->mutable_table_ref();
            table_ref->set_table_name(RDB_TABLE_NAME);
            table_ref->set_db_name(DB_NAME);
            Term *term = insert->add_terms();
            term->set_type(Term::OBJECT);
            VarTermTuple *object_key = term->add_object();
            object_key->set_var(PRIMARY_KEY_NAME);
            Term *key_term = object_key->mutable_term();
            key_term->set_type(Term::STRING);
            VarTermTuple *object_value = term->add_object();
            object_value->set_var(SECONDARY_KEY_NAME);
            Term *object_term = object_value->mutable_term();
            object_term->set_type(Term::STRING);
        }
        if (option == FINALIZE || option == GENERATE) {
            Term *key_term = query->mutable_write_query()->mutable_insert()->mutable_terms(0)->mutable_object(0)->mutable_term();
            key_term->set_valuestring(std::string(key, key_size));
            Term *object_term = query->mutable_write_query()->mutable_insert()->mutable_terms(0)->mutable_object(1)->mutable_term();
            object_term->set_valuestring(std::string(value, value_size));
        }
    }

    void generate_batched_read_query(Query *query, payload_t *keys, int count, int option = GENERATE) {
        if (option == PREPARE || option == GENERATE) {
            query->set_type(Query::READ);
            ReadQuery *read_query = query->mutable_read_query();
            Term *term = read_query->mutable_term();
            term->set_type(Term::ARRAY);
        }
        if (option == FINALIZE || option == GENERATE) {
            Term *term = query->mutable_read_query()->mutable_term();
            term->clear_array();
            for (int i = 0; i < count; i++) {
                Term *array_term = term->add_array();
                array_term->set_type(Term::GETBYKEY);
                Term::GetByKey *get_by_key = array_term->mutable_get_by_key();
                TableRef *table_ref = get_by_key->mutable_table_ref();
                table_ref->set_table_name(RDB_TABLE_NAME);
                table_ref->set_db_name(DB_NAME);
                get_by_key->set_attrname(PRIMARY_KEY_NAME);
                Term *term_key = get_by_key->mutable_key();
                term_key->set_type(Term::STRING);
                term_key->set_valuestring(std::string(keys[i].first, keys[i].second));
            }
        }
    }

    void generate_read_query(Query *query, const char *key, size_t key_size, int option = GENERATE) {
        if (option == PREPARE || option == GENERATE) {
            query->set_type(Query::READ);
            ReadQuery *read_query = query->mutable_read_query();
            Term *term = read_query->mutable_term();
            term->set_type(Term::GETBYKEY);
            Term::GetByKey *get_by_key = term->mutable_get_by_key();
            TableRef *table_ref = get_by_key->mutable_table_ref();
            table_ref->set_table_name(RDB_TABLE_NAME);
            table_ref->set_db_name(DB_NAME);
            get_by_key->set_attrname(PRIMARY_KEY_NAME);
            Term *term_key = get_by_key->mutable_key();
            term_key->set_type(Term::STRING);
        }
        if (option == FINALIZE || option == GENERATE) {
            Term *term_key = query->mutable_read_query()->mutable_term()->mutable_get_by_key()->mutable_key();
            term_key->set_valuestring(std::string(key, key_size));
        }
    }

    void generate_range_read_query(Query *query, char *lkey, size_t lkey_size,
                                                 char *rkey, size_t rkey_size, int count_limit, int option = GENERATE) {
        if (option == PREPARE || option == GENERATE) {
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
            Term *upperbound = range->mutable_upperbound();
            upperbound->set_type(Term::STRING);
            Term *args = call->add_args();
            args->set_type(Term::TABLE);
            Term::Table *table = args->mutable_table();
            TableRef *table_ref = table->mutable_table_ref();
            table_ref->set_table_name(RDB_TABLE_NAME);
            table_ref->set_db_name(DB_NAME);
        }
        if (option == FINALIZE || option == GENERATE) {
            Term *lowerbound = query->mutable_read_query()->mutable_term()->mutable_call()->mutable_builtin()->mutable_range()->mutable_lowerbound();
            lowerbound->set_valuestring(std::string(lkey, lkey_size));
            Term *upperbound = query->mutable_read_query()->mutable_term()->mutable_call()->mutable_builtin()->mutable_range()->mutable_upperbound();
            upperbound->set_valuestring(std::string(rkey, rkey_size));
            ReadQuery *read_query = query->mutable_read_query();
            read_query->set_max_chunk_size(count_limit);
        }
    }

    void generate_stop_query(Query *query, int option = GENERATE) {
        if (option == PREPARE || option == GENERATE) {
            query->set_type(Query::STOP);
        }
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

    void get_response(Response *response) {
        // get message
        int size;
        recv_all((char *) &size, sizeof(size));
        recv_all(buffer, size);

        // unserialize
        if (!response->ParseFromString(std::string(buffer, size))) {
            fprintf(stderr, "Failed to unserialize response.\n");
            throw protocol_error_t("Failed to unserialize response.");
        }
    }

    bool get_response_maybe(Response *response) {
        if (socket_ready()) {
            get_response(response);
        } else {
            return false;
        }
    }

    bool exist_outstanding_pipeline_reads() {
        return queued_read >= 0;
    }

    // takes a JSON string and returns the string in the last set of quotes
    // useful for retrieving the last value, and probably faster than using a JSON parser
    std::string get_value(const std::string &json_string, int last = -1) {
        if (json_string == "null" || json_string == "[null]") {
            return json_string;
        }
        if (last < 0) {
            last = json_string.length() - 1;
        }
        int last_quote = (int) json_string.find_last_of('"', last);
        int second_to_last_quote = (int) json_string.find_last_of('"', last_quote - 1);
        assert(last_quote >= 0 && last_quote < json_string.length());
        assert(second_to_last_quote >= 0 && second_to_last_quote < json_string.length());
        return json_string.substr(second_to_last_quote + 1, last_quote - second_to_last_quote - 1);
    }

private:
    int64_t token_index;
    int sockfd;
    int64_t queued_read; // used for enqueue/dequeue read (stores a token number)
    fd_set readfds;
    char buffer[MAX_PROTOBUF_SIZE];

    // the following are used to lower the extra time used to create each query before sending
    Response *response;
    Response *stop_response;
    Query *base_point_delete_query;
    Query *base_point_update_query;
    Query *base_insert_query;
    Query *base_batched_read_query;
    Query *base_read_query;
    Query *base_range_read_query;
    Query *base_stop_query;
} ;

#endif // __STRESS_CLIENT_PROTOCOLS_RETHINKDB_PROTOCOL_HPP__
