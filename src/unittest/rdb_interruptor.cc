// Copyright 2010-2014 RethinkDB, all rights reserved.
#include <functional>
#include <stdexcept>

#include "errors.hpp"
#include <boost/optional.hpp>

#include "protob/protob.hpp"
#include "rdb_protocol/counted_term.hpp"
#include "rdb_protocol/env.hpp"
#include "rdb_protocol/func.hpp"
#include "rdb_protocol/minidriver.hpp"
#include "unittest/gtest.hpp"
#include "unittest/rdb_env.hpp"
#include "unittest/unittest_utils.hpp"

namespace unittest {

class count_callback_t : public ql::env_t::eval_callback_t {
public:
    explicit count_callback_t(uint32_t *_count_out) : count_out(_count_out) {
        *count_out = 0;
    }

    virtual ~count_callback_t() { }

    void eval_callback() {
        ++(*count_out);
    }

private:
    uint32_t *count_out;
};

class interrupt_callback_t : public ql::env_t::eval_callback_t {
public:
    interrupt_callback_t(uint32_t _delay,
                         test_rdb_env_t::instance_t *_test_env_instance) :
        delay(_delay),
        test_env_instance(_test_env_instance)
    {
        if (delay == 0) {
            test_env_instance->interrupt();
        }
    }

    virtual ~interrupt_callback_t() { }

    void eval_callback() {
        --delay;
        if (delay == 0) {
            test_env_instance->interrupt();
        }
    }

private:
    uint32_t delay;
    test_rdb_env_t::instance_t *test_env_instance;
};

class verify_callback_t {
public:
    virtual ~verify_callback_t() { }
    virtual bool verify(test_rdb_env_t::instance_t *) = 0;
};

void count_evals(test_rdb_env_t *test_env,
                 ql::protob_t<const Term> term,
                 uint32_t *count_out,
                 verify_callback_t *verify_callback) {
    scoped_ptr_t<test_rdb_env_t::instance_t> env_instance = test_env->make_env();

    count_callback_t callback(count_out);
    env_instance->get_env()->set_eval_callback(&callback);

    ql::compile_env_t compile_env((ql::var_visibility_t()));
    counted_t<const ql::term_t> compiled_term = ql::compile_term(&compile_env, term);

    ql::scope_env_t scope_env(env_instance->get_env(), ql::var_scope_t());
    UNUSED scoped_ptr_t<ql::val_t> result = compiled_term->eval(&scope_env);
    guarantee(*count_out > 0);
    guarantee(verify_callback->verify(env_instance.get()));
}

void interrupt_test(test_rdb_env_t *test_env,
                    ql::protob_t<const Term> term,
                    uint32_t interrupt_phase,
                    verify_callback_t *verify_callback) {
    scoped_ptr_t<test_rdb_env_t::instance_t> env_instance = test_env->make_env();

    interrupt_callback_t callback(interrupt_phase, env_instance.get());
    env_instance->get_env()->set_eval_callback(&callback);

    ql::compile_env_t compile_env((ql::var_visibility_t()));
    counted_t<const ql::term_t> compiled_term = ql::compile_term(&compile_env, term);

    try {
        ql::scope_env_t scope_env(env_instance->get_env(), ql::var_scope_t());
        UNUSED scoped_ptr_t<ql::val_t> result = compiled_term->eval(&scope_env);
        guarantee(false);
    } catch (const interrupted_exc_t &ex) {
        // Do nothing
    }
    guarantee(verify_callback->verify(env_instance.get()));
}

class exists_verify_callback_t : public verify_callback_t {
public:
    exists_verify_callback_t(const std::string &_db_name,
                             const std::string &_table_name,
                             const ql::datum_t &_key,
                             bool _should_exist) :
        key(_key.print_primary()),
        db_name(name_string_t::guarantee_valid(_db_name.c_str())),
        table_name(name_string_t::guarantee_valid(_table_name.c_str())),
        should_exist(_should_exist) { }

    virtual ~exists_verify_callback_t() { }

    bool verify(test_rdb_env_t::instance_t *env_instance) {
        const std::map<store_key_t, ql::datum_t> *data =
            env_instance->get_data(db_name, table_name);
        bool exists = data->find(key) != data->end();
        return should_exist == exists;
    }

private:
    const store_key_t key;
    const name_string_t db_name;
    const name_string_t table_name;
    const bool should_exist;
};

TEST(RDBInterrupt, InsertOp) {
    uint32_t eval_count;

    ql::datum_object_builder_t row;
    row.overwrite("id", ql::datum_t(datum_string_t("key")));
    row.overwrite("value", ql::datum_t(datum_string_t("stuff")));

    ql::protob_t<const Term> insert_proto =
        ql::r::db("db").table("table")
                       .insert(std::move(row).to_datum()).release_counted();

    {
        test_rdb_env_t test_env;
        test_env.add_database("db");
        test_env.add_table("db", "table", "id");
        exists_verify_callback_t verify_callback(
            "db", "table", ql::datum_t(datum_string_t("key")), true);
        unittest::run_in_thread_pool(std::bind(count_evals,
                                               &test_env,
                                               insert_proto,
                                               &eval_count,
                                               &verify_callback));
    }
    for (uint64_t i = 0; i <= eval_count; ++i) {
        test_rdb_env_t test_env;
        test_env.add_database("db");
        test_env.add_table("db", "table", "id");
        exists_verify_callback_t verify_callback(
            "db", "table", ql::datum_t(datum_string_t("key")), false);
        unittest::run_in_thread_pool(std::bind(interrupt_test,
                                               &test_env,
                                               insert_proto,
                                               i,
                                               &verify_callback));
    }
}

class dummy_callback_t : public verify_callback_t {
public:
    virtual ~dummy_callback_t() { }
    bool verify(UNUSED test_rdb_env_t::instance_t *env_instance) {
        return true;
    }
};

TEST(RDBInterrupt, GetOp) {
    uint32_t eval_count;
    std::set<ql::datum_t, latest_version_optional_datum_less_t> initial_data;

    ql::datum_object_builder_t row;
    row.overwrite("id", ql::datum_t(datum_string_t("key")));
    row.overwrite("value", ql::datum_t(datum_string_t("stuff")));
    initial_data.insert(std::move(row).to_datum());

    ql::protob_t<const Term> get_proto =
        ql::r::db("db").table("table").get_("key").release_counted();

    {
        test_rdb_env_t test_env;
        dummy_callback_t dummy_callback;
        test_env.add_database("db");
        test_env.add_table("db", "table", "id", initial_data);
        unittest::run_in_thread_pool(std::bind(count_evals,
                                               &test_env,
                                               get_proto,
                                               &eval_count,
                                               &dummy_callback));
    }
    for (uint64_t i = 0; i <= eval_count; ++i) {
        test_rdb_env_t test_env;
        dummy_callback_t dummy_callback;
        test_env.add_database("db");
        test_env.add_table("db", "table", "id", initial_data);
        unittest::run_in_thread_pool(std::bind(interrupt_test, &test_env, get_proto, i,
                                               &dummy_callback));
    }
}

TEST(RDBInterrupt, DeleteOp) {
    uint32_t eval_count;
    std::set<ql::datum_t, latest_version_optional_datum_less_t> initial_data;

    ql::datum_object_builder_t row;
    row.overwrite("id", ql::datum_t(datum_string_t("key")));
    row.overwrite("value", ql::datum_t(datum_string_t("stuff")));
    initial_data.insert(std::move(row).to_datum());

    ql::protob_t<const Term> delete_proto =
        ql::r::db("db").table("table").get_("key").delete_().release_counted();

    {
        test_rdb_env_t test_env;
        test_env.add_database("db");
        test_env.add_table("db", "table", "id", initial_data);
        exists_verify_callback_t verify_callback(
            "db", "table", ql::datum_t(datum_string_t("key")), false);
        unittest::run_in_thread_pool(std::bind(count_evals,
                                               &test_env,
                                               delete_proto,
                                               &eval_count,
                                               &verify_callback));
    }
    for (uint64_t i = 0; i <= eval_count; ++i) {
        test_rdb_env_t test_env;
        test_env.add_database("db");
        test_env.add_table("db", "table", "id", initial_data);
        exists_verify_callback_t verify_callback(
            "db", "table", ql::datum_t(datum_string_t("key")), true);
        unittest::run_in_thread_pool(std::bind(interrupt_test,
                                               &test_env,
                                               delete_proto,
                                               i,
                                               &verify_callback));
    }
}

// This is a simple drop-in mock of query_server_t - it will not handle
// concurrent queries for the same token - aside from a STOP query.
class query_hanger_t : public query_handler_t, public home_thread_mixin_t {
public:
    static const std::string stop_query_message;

    void run_query(UNUSED const ql::query_id_t &query_id,
                   const ql::protob_t<Query> &query,
                   Response *res,
                   UNUSED ql::query_cache_t *query_cache,
                   signal_t *interruptor) {
        assert_thread();

        if (query->type() != Query::STOP) {
            cond_t dummy_cond;
            cond_t local_interruptor;
            wait_any_t final_interruptor(&local_interruptor, interruptor);
            map_insertion_sentry_t<int64_t, cond_t *> sentry(&interruptors,
                                                             query->token(),
                                                             &local_interruptor);

            try {
                wait_interruptible(&dummy_cond, &final_interruptor);
            } catch (const interrupted_exc_t &ex) {
                if (!local_interruptor.is_pulsed()) {
                    throw;
                }
            }
        } else {
            auto interruptor_it = interruptors.find(query->token());
            guarantee(interruptor_it != interruptors.end());
            interruptor_it->second->pulse_if_not_already_pulsed();
        }

        // The real server sends a SUCCESS_SEQUENCE, but this makes the test simpler
        res->set_token(query->token());
        ql::fill_error(res, Response::RUNTIME_ERROR, stop_query_message);
    }
private:
    std::map<int64_t, cond_t *> interruptors;
};

const std::string query_hanger_t::stop_query_message =
    "Query terminated by a STOP query.";

// Arbitrary non-zero to check that responses are correct
const int64_t test_token = 54;
const int64_t unparsable_query_token = 99;
const std::string r_uuid_json(strprintf("[%d,[%d,[]],{}]", Query::START, Term::UUID));
const std::string stop_json(strprintf("[%d,%" PRIi64 "]", Query::STOP, test_token));
const std::string invalid_json(",");

template <class T>
void append_to_message(const T& item, std::string *message) {
    message->append(reinterpret_cast<const char *>(&item), sizeof(item));
}

void append_to_message(const std::string &item, std::string *message) {
    message->append(item);
}

template <class... Args>
void send_tcp_message(tcp_conn_stream_t *conn, Args... args) {
    std::string message;
    UNUSED int _[] = { (append_to_message(args, &message), 1)... };
    int64_t res = conn->write(message.data(), message.size());
    guarantee(static_cast<size_t>(res) == message.size());
}

scoped_ptr_t<tcp_conn_stream_t> connect_client(int port) {
    cond_t dummy_interruptor;
    scoped_ptr_t<tcp_conn_stream_t> conn(
        new tcp_conn_stream_t(ip_address_t("127.0.0.1"), port, &dummy_interruptor));

    send_tcp_message(conn.get(),
        static_cast<int32_t>(VersionDummy::V0_4),    // Protocol version
        static_cast<uint32_t>(0),                             // Auth key length
        static_cast<uint32_t>(VersionDummy::JSON)); // Wire protocol

    std::string auth_response;
    while (true) {
        char next;
        guarantee(conn->read(&next, sizeof(next)) == sizeof(next));
        if (next == '\0') {
            break;
        }
        auth_response += next;
    }
    guarantee(auth_response == "SUCCESS");

    return conn;
}

void send_query(int64_t token, const std::string &query_json, tcp_conn_stream_t *conn) {
    send_tcp_message(conn, token, static_cast<uint32_t>(query_json.size()), query_json);
}

std::string parse_json_error_message(const char *json,
                                     int32_t expected_type) {
    scoped_cJSON_t response(cJSON_Parse(json));
    guarantee(response.get() != nullptr);

    json_object_iterator_t it(response.get());
    // The `make_optional` call works around an incorrect warning in old versions of GCC
    boost::optional<Response::ResponseType> type =
        boost::make_optional(false, Response::COMPILE_ERROR);
    boost::optional<std::string> msg = boost::make_optional(false, std::string());

    while (!type || !msg) {
        cJSON *item = it.next();
        std::string item_name(item->string);

        if (item_name == "t") {
            guarantee(item->type == cJSON_Number);
            type = static_cast<Response::ResponseType>(item->valueint);
        } else if (item_name == "r") {
            json_array_iterator_t rit(item);
            item = rit.next();
            guarantee(item->type == cJSON_String);
            msg = std::string(item->valuestring);
            guarantee(rit.next() == nullptr);
        }
    }
    guarantee(type.get() == expected_type);
    guarantee(msg->length() != 0);
    return msg.get();
}

std::string get_query_response(tcp_conn_stream_t *conn) {
    int64_t res;
    int64_t token;
    uint32_t response_size;
    res = conn->read(&token, sizeof(token));
    if (res == 0) {
        return std::string();
    }
    guarantee(res == sizeof(token));
    guarantee(token == unparsable_query_token || token == test_token);

    res = conn->read(&response_size, sizeof(response_size));
    guarantee(res == sizeof(response_size));

    scoped_array_t<char> response_data(response_size + 1);
    res = conn->read(response_data.data(), response_size);
    guarantee(res == response_size);
    response_data[response_size] = '\0';

    return parse_json_error_message(response_data.data(),
        token == test_token ?  Response::RUNTIME_ERROR : Response::CLIENT_ERROR);
}

void tcp_interrupt_test(
        test_rdb_env_t *test_env,
        const std::string &expected_msg,
        std::function<void(scoped_ptr_t<query_server_t> *,
                           scoped_ptr_t<tcp_conn_stream_t> *)> interrupt_fn) {
    scoped_ptr_t<test_rdb_env_t::instance_t> env_instance(test_env->make_env());

    query_hanger_t hanger; // Causes all queries to hang until interrupted
    scoped_ptr_t<query_server_t> server(
        new query_server_t(env_instance->get_rdb_context(),
                           std::set<ip_address_t>({ip_address_t("127.0.0.1")}),
                           0, &hanger, 2));

    scoped_ptr_t<tcp_conn_stream_t> conn = connect_client(server->get_port());
    send_query(test_token, r_uuid_json, conn.get());

    let_stuff_happen();

    interrupt_fn(&server, &conn);
    if (conn.has()) {
        ASSERT_EQ(expected_msg, get_query_response(conn.get()));
    } else {
        ASSERT_EQ(expected_msg, std::string());
    }
}

TEST(RDBInterrupt, TcpInterrupt) {
    // Test different types of TCP session interruptions:
    // 1. TCP session is closed remotely
    // 2. any drainers?

    {
        // Test destroying the server
        test_rdb_env_t test_env;
        unittest::run_in_thread_pool(std::bind(tcp_interrupt_test, &test_env,
            std::string(), // Server shouldn't be allowed to respond
            [](scoped_ptr_t<query_server_t> *serv,
               UNUSED scoped_ptr_t<tcp_conn_stream_t> *conn) {
                serv->reset();
            }));
    }

    {
        // Test closing the connection
        test_rdb_env_t test_env;
        unittest::run_in_thread_pool(std::bind(tcp_interrupt_test, &test_env,
            std::string(), // No way to get a response on a closed connection
            [](UNUSED scoped_ptr_t<query_server_t> *serv,
               scoped_ptr_t<tcp_conn_stream_t> *conn) {
                conn->reset();
            }));
    }

    {
        // Test sending a STOP query
        test_rdb_env_t test_env;
        unittest::run_in_thread_pool(std::bind(tcp_interrupt_test, &test_env,
            query_hanger_t::stop_query_message,
            [](UNUSED scoped_ptr_t<query_server_t> *serv,
               scoped_ptr_t<tcp_conn_stream_t> *conn) {
                send_query(test_token, stop_json, conn->get());
                get_query_response(conn->get());
            }));
    }
}

http_res_t run_http_req(const http_req_t &req,
                        http_app_t *query_app,
                        cond_t *interruptor) {
    http_res_t result;
    query_app->handle(req, &result, interruptor);
    return result;
}

int32_t create_http_session(http_app_t *query_app) {
    http_req_t create_req("/query/open-new-connection");
    create_req.method = http_method_t::POST;

    cond_t dummy_interruptor;
    http_res_t result = run_http_req(create_req, query_app, &dummy_interruptor);
    guarantee(result.body.size() == sizeof(int32_t));
    return *reinterpret_cast<const int32_t *>(result.body.data());
}

http_req_t make_http_close(int32_t conn_id) {
    http_req_t close_req("/query/close-connection");
    close_req.method = http_method_t::POST;
    close_req.query_params.insert(std::make_pair("conn_id", strprintf("%d", conn_id)));
    return close_req;
}

http_req_t make_http_query(int32_t conn_id, const std::string &query_json) {
    http_req_t query_req("/query");
    query_req.method = http_method_t::POST;
    query_req.query_params.insert(std::make_pair("conn_id", strprintf("%d", conn_id)));
    query_req.body.append(reinterpret_cast<const char *>(&test_token),
                          sizeof(test_token));
    query_req.body.append(query_json);
    return query_req;
}

std::string parse_http_result(const http_res_t &http_res, int32_t expected_type) {
    guarantee(http_res.body.size() > sizeof(int64_t));
    const char *data = http_res.body.data();

    int64_t token = *reinterpret_cast<const int64_t *>(data);
    data += sizeof(token);

    uint32_t data_size = *reinterpret_cast<const uint32_t *>(data);
    data += sizeof(data_size);

    return parse_json_error_message(data, expected_type);
}

void http_interrupt_test(test_rdb_env_t *test_env,
                         const std::string &expected_msg,
                         std::function<void(scoped_ptr_t<query_server_t> *,
                                            cond_t *, int32_t)> interrupt_fn) {
    scoped_ptr_t<test_rdb_env_t::instance_t> env_instance = test_env->make_env();

    query_hanger_t hanger; // Causes all queries to hang until interrupted
    scoped_ptr_t<query_server_t> server(
        new query_server_t(env_instance->get_rdb_context(),
                           std::set<ip_address_t>({ip_address_t("127.0.0.1")}),
                           0, &hanger, 2));

    cond_t http_app_interruptor;
    http_res_t result;
    cond_t http_done;

    int32_t conn_id = create_http_session(server.get());

    coro_t::spawn_now_dangerously([&]() {
            result = run_http_req(make_http_query(conn_id, r_uuid_json),
                                  server.get(), &http_app_interruptor);
            http_done.pulse();
        });

    let_stuff_happen(); // Let the query get into its 'run' phase
    interrupt_fn(&server, &http_app_interruptor, conn_id);
    http_done.wait();

    std::string msg = parse_http_result(result, Response::RUNTIME_ERROR);
    ASSERT_EQ(expected_msg, msg);
}

TEST(RDBInterrupt, HttpInterrupt) {
    // Test different types of HTTP session interruptions
    {
        // HTTP socket closed by the client
        test_rdb_env_t test_env;
        unittest::run_in_thread_pool(std::bind(http_interrupt_test, &test_env,
            "This ReQL connection has been terminated.",
            [](UNUSED scoped_ptr_t<query_server_t> *server,
               cond_t *interruptor, UNUSED int32_t conn_id) {
                // We don't have a real socket here, use the fake interruptor
                interruptor->pulse_if_not_already_pulsed();
            }));
    }

    {
        // HTTP session closed by another request
        test_rdb_env_t test_env;
        unittest::run_in_thread_pool(std::bind(http_interrupt_test, &test_env,
            "This ReQL connection has been terminated.",
            [](scoped_ptr_t<query_server_t> *server,
               cond_t *interruptor, int32_t conn_id) {
                http_res_t result = run_http_req(make_http_close(conn_id),
                                                 server->get(), interruptor);
            }));
    }

    {
        // Query server destroyed
        test_rdb_env_t test_env;
        unittest::run_in_thread_pool(std::bind(http_interrupt_test, &test_env,
            "Server is shutting down.", // Technically we can't send this message back
            [](scoped_ptr_t<query_server_t> *server,
               UNUSED cond_t *interruptor, UNUSED int32_t conn_id) {
                server->reset();
            }));
    }

    {
        // STOP query
        test_rdb_env_t test_env;
        unittest::run_in_thread_pool(std::bind(http_interrupt_test, &test_env,
            query_hanger_t::stop_query_message,
            [](scoped_ptr_t<query_server_t> *server,
               cond_t *interruptor, int32_t conn_id) {
                http_res_t result = run_http_req(make_http_query(conn_id, stop_json),
                                                 server->get(), interruptor);
            }));
    }

    {
        // HTTP session expires
        test_rdb_env_t test_env;
        unittest::run_in_thread_pool(std::bind(http_interrupt_test, &test_env,
            "HTTP ReQL query timed out after 2 seconds.",
            [](UNUSED scoped_ptr_t<query_server_t> *server,
               UNUSED cond_t *interruptor, UNUSED int32_t conn_id) {
                // Don't actually have to do anything here, the test should
                // wait for the timeout
            }));
    }
}

}  // namespace unittest
