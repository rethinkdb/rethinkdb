// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "protob/protob.hpp"

#include <google/protobuf/stubs/common.h>

#include <set>
#include <string>
#include <limits>

#include "errors.hpp"
#include <boost/lexical_cast.hpp>

#include "arch/arch.hpp"
#include "arch/io/network.hpp"
#include "clustering/administration/metadata.hpp"
#include "concurrency/coro_pool.hpp"
#include "concurrency/cross_thread_signal.hpp"
#include "concurrency/queue/limited_fifo.hpp"
#include "containers/auth_key.hpp"
#include "perfmon/perfmon.hpp"
#include "protob/json_shim.hpp"
#include "rdb_protocol/env.hpp"
#include "rpc/semilattice/view.hpp"
#include "utils.hpp"

#include "rdb_protocol/ql2.pb.h"
#include "rdb_protocol/query_server.hpp"
#include "rdb_protocol/query_cache.hpp"

http_conn_cache_t::http_conn_t::http_conn_t(rdb_context_t *rdb_ctx,
                                            ip_and_port_t client_addr_port) :
    last_accessed(time(0)),
    // We always return empty normal batches after the timeout for HTTP
    // connections; I think we have to do this to keep the conn cache
    // from timing out.
    query_cache(new ql::query_cache_t(rdb_ctx, client_addr_port,
                                      ql::return_empty_normal_batches_t::YES)),
    counter(&rdb_ctx->stats.client_connections) { }

ql::query_cache_t *http_conn_cache_t::http_conn_t::get_query_cache() {
    last_accessed = time(0);
    return query_cache.get();
}

signal_t *http_conn_cache_t::http_conn_t::get_interruptor() {
    return &interruptor;
}

void http_conn_cache_t::http_conn_t::pulse() {
    rassert(!interruptor.is_pulsed());
    interruptor.pulse();
}

bool http_conn_cache_t::http_conn_t::is_expired() {
    return difftime(time(0), last_accessed) > TIMEOUT_SEC;
}

http_conn_cache_t::http_conn_cache_t() :
    next_id(0), http_timeout_timer(TIMER_RESOLUTION_MS, this) { }

http_conn_cache_t::~http_conn_cache_t() {
    for (auto &pair : cache) pair.second->pulse();
}

counted_t<http_conn_cache_t::http_conn_t> http_conn_cache_t::find(int32_t key) {
    assert_thread();
    auto conn_it = cache.find(key);
    if (conn_it == cache.end()) return counted_t<http_conn_t>();
    return conn_it->second;
}

int32_t http_conn_cache_t::create(rdb_context_t *rdb_ctx,
                                  ip_and_port_t client_addr_port) {
    assert_thread();
    int32_t key = next_id++;
    cache.insert(
        std::make_pair(key, make_counted<http_conn_t>(rdb_ctx, client_addr_port)));
    return key;
}

void http_conn_cache_t::erase(int32_t key) {
    assert_thread();
    auto it = cache.find(key);
    if (it != cache.end()) {
        it->second->pulse();
        cache.erase(it);
    }
}

void http_conn_cache_t::on_ring() {
    assert_thread();
    for (auto it = cache.begin(); it != cache.end();) {
        auto tmp = it++;
        if (tmp->second->is_expired()) {
            // We go through some rigmarole to make sure we erase from the
            // cache immediately and call the possibly-blocking destructor
            // in a separate coroutine to satisfy the
            // `ASSERT_FINITE_CORO_WAITING` in `call_ringer` in
            // `arch/timing.cc`.
            counted_t<http_conn_t> conn(std::move(tmp->second));
            conn->pulse();
            cache.erase(tmp);
            coro_t::spawn_now_dangerously(
                [&conn]() {
                    counted_t<http_conn_t> conn2;
                    conn2.swap(conn);
                });
            guarantee(!conn);
        }
    }
}

struct protob_server_exc_t : public std::exception {
public:
    explicit protob_server_exc_t(const std::string& data) : info(data) { }
    ~protob_server_exc_t() throw () { }
    const char *what() const throw () { return info.c_str(); }
private:
    std::string info;
};

const uint32_t MAX_QUERY_SIZE = 64 * MEGABYTE;
const size_t MAX_RESPONSE_SIZE = std::numeric_limits<uint32_t>::max();

class json_protocol_t {
public:
    static bool parse_query(tcp_conn_t *conn,
                            signal_t *interruptor,
                            query_handler_t *handler,
                            ql::protob_t<Query> *query_out) {
        int64_t token;
        uint32_t size;
        conn->read(&token, sizeof(token), interruptor);
        conn->read(&size, sizeof(size), interruptor);

        if (size > MAX_QUERY_SIZE) {
            Response error_response;
            handler->unparseable_query(token, &error_response,
                                       strprintf("Payload size (%" PRIu32 ") greater than maximum (%" PRIu32 ").",
                                                 size, MAX_QUERY_SIZE));
            send_response(error_response, handler, conn, interruptor);
            throw tcp_conn_read_closed_exc_t();
        } else {
            scoped_array_t<char> data(size + 1);
            conn->read(data.data(), size, interruptor);
            data[size] = 0; // Null terminate the string, which the json parser requires

            if (!json_shim::parse_json_pb(query_out->get(), token, data.data())) {
                Response error_response;
                handler->unparseable_query(
                    token, &error_response,
                    "Client is buggy (failed to deserialize query).");
                send_response(error_response, handler, conn, interruptor);
                return false;
            }
        }
        return true;
    }

    static void send_response(const Response &response,
                              query_handler_t *handler,
                              tcp_conn_t *conn,
                              signal_t *interruptor) {
        const int64_t token = response.token();

        uint32_t data_size_32; // filled in below
        const size_t prefix_size = sizeof(token) + sizeof(data_size_32);
        // Reserve space for the token and the size
        std::string str(prefix_size, '\0');

        json_shim::write_json_pb(response, &str);
        guarantee(str.size() >= prefix_size);
        const size_t data_size = str.size() - prefix_size;

        if (data_size > MAX_RESPONSE_SIZE) {
            Response error_response;
            handler->unparseable_query(token, &error_response,
                strprintf("Response size (%zu) is greater than maximum (%zu).",
                          data_size, MAX_RESPONSE_SIZE));
            send_response(error_response, handler, conn, interruptor);
            return;
        }

        // Fill in the prefix.
        // std::string::operator[] has unspecified complexity, but in practice
        // it should be fine.
        for (size_t i = 0; i < sizeof(token); ++i) {
            str[i] = reinterpret_cast<const char *>(&token)[i];
        }
        data_size_32 = static_cast<uint32_t>(data_size);
        for (size_t i = 0; i < sizeof(data_size_32); ++i) {
            str[i + sizeof(token)] = reinterpret_cast<const char *>(&data_size_32)[i];
        }

        conn->write(str.data(), str.size(), interruptor);
    }
};

class protobuf_protocol_t {
public:
    static bool parse_query(tcp_conn_t *conn,
                            signal_t *interruptor,
                            query_handler_t *handler,
                            ql::protob_t<Query> *query_out) {
        uint32_t size;
        conn->read(&size, sizeof(size), interruptor);

        if (size > MAX_QUERY_SIZE) {
            Response error_response;
            handler->unparseable_query(
                0,
                &error_response,
                strprintf("Payload size (%" PRIu32 ") greater than maximum (%" PRIu32 ").",
                          size, MAX_QUERY_SIZE));
            send_response(error_response, handler, conn, interruptor);
            return false;
        } else {
            scoped_array_t<char> data(size);
            conn->read(data.data(), size, interruptor);

            if (!query_out->get()->ParseFromArray(data.data(), size)) {
                Response error_response;
                int64_t token = query_out->get()->has_token() ? query_out->get()->token() : 0;
                handler->unparseable_query(token, &error_response,
                                           "Client is buggy (failed to deserialize query).");
                send_response(error_response, handler, conn, interruptor);
                return false;
            }
        }
        return true;
    }

    static void send_response(const Response &response,
                              query_handler_t *handler,
                              tcp_conn_t *conn,
                              signal_t *interruptor) {
        const size_t data_size = response.ByteSize();
        if (data_size > MAX_RESPONSE_SIZE) {
            Response error_response;
            handler->unparseable_query(
                response.token(),
                &error_response,
                strprintf("Response size (%zu) is greater than maximum (%zu).",
                          data_size, MAX_RESPONSE_SIZE));
            send_response(error_response, handler, conn, interruptor);
            return;
        }
        const uint32_t data_size_32 = static_cast<uint32_t>(data_size);
        const size_t prefix_size = sizeof(data_size_32);
        const size_t total_size = prefix_size + data_size;

        scoped_array_t<char> scoped_array(total_size);
        memcpy(scoped_array.data(), &data_size_32, sizeof(data_size_32));
        response.SerializeToArray(scoped_array.data() + prefix_size, data_size_32);

        conn->write(scoped_array.data(), total_size, interruptor);
    }
};

query_server_t::query_server_t(
    rdb_context_t *_rdb_ctx,
    const std::set<ip_address_t> &local_addresses,
    int port,
    query_handler_t *_handler)
    : rdb_ctx(_rdb_ctx),
      handler(_handler),
      next_thread(0) {
    rassert(rdb_ctx != NULL);
    try {
        tcp_listener.init(new tcp_listener_t(local_addresses, port,
            std::bind(&query_server_t::handle_conn,
                      this, ph::_1, auto_drainer_t::lock_t(&auto_drainer))));
    } catch (const address_in_use_exc_t &ex) {
        throw address_in_use_exc_t(
            strprintf("Could not bind to RDB protocol port: %s", ex.what()));
    }
}

query_server_t::~query_server_t() { }

int query_server_t::get_port() const {
    return tcp_listener->get_port();
}

std::string query_server_t::read_sized_string(tcp_conn_t *conn,
                                              size_t max_size,
                                              const std::string &length_error_msg,
                                              signal_t *interruptor) {
    uint32_t str_length;
    conn->read(&str_length, sizeof(uint32_t), interruptor);

    if (str_length > max_size) {
        throw protob_server_exc_t(length_error_msg);
    }

    scoped_array_t<char> buffer(max_size);
    conn->read(buffer.data(), str_length, interruptor);

    return std::string(buffer.data(), str_length);
}

auth_key_t query_server_t::read_auth_key(tcp_conn_t *conn,
                                         signal_t *interruptor) {
    const std::string length_error_msg("Client provided an authorization key that is too long.");
    std::string auth_key = read_sized_string(conn, auth_key_t::max_length,
                                             length_error_msg, interruptor);
    auth_key_t ret;
    if (!ret.assign_value(auth_key)) {
        // This should never happen, since we already checked above.
        rassert(false);
        throw protob_server_exc_t(length_error_msg);
    }
    return ret;
}

void query_server_t::handle_conn(const scoped_ptr_t<tcp_conn_descriptor_t> &nconn,
                                 auto_drainer_t::lock_t keepalive) {
    // This must be read here because of home threads and stuff
    auth_key_t auth_key = rdb_ctx->auth_metadata->get().auth_key.get_ref();

    threadnum_t chosen_thread = threadnum_t(next_thread);
    next_thread = (next_thread + 1) % get_num_db_threads();

    cross_thread_signal_t ct_keepalive(keepalive.get_drain_signal(), chosen_thread);
    on_thread_t rethreader(chosen_thread);

    scoped_ptr_t<tcp_conn_t> conn;
    nconn->make_overcomplicated(&conn);
    conn->enable_keepalive();

#ifdef __linux
    linux_event_watcher_t *ew = conn->get_event_watcher();
    linux_event_watcher_t::watch_t conn_interrupted(ew, poll_event_rdhup);
    wait_any_t interruptor(&conn_interrupted, &ct_keepalive);
#else
    wait_any_t interruptor(&ct_keepalive);
#endif  // __linux
    ip_and_port_t client_addr_port(ip_address_t::any(AF_INET), port_t(0));
    UNUSED bool peer_res = conn->getpeername(&client_addr_port);

    std::string init_error;

    try {
        int32_t client_magic_number;
        conn->read(&client_magic_number, sizeof(client_magic_number), &interruptor);

        bool pre_2 = client_magic_number == VersionDummy::V0_1;
        bool pre_3 = pre_2 || client_magic_number == VersionDummy::V0_2;
        bool pre_4 = pre_3 || client_magic_number == VersionDummy::V0_3;
        bool legal = pre_4 || client_magic_number == VersionDummy::V0_4;

        // With version 0_2 and up, the client drivers specifies the authorization key
        if (pre_2) {
            if (!auth_key.str().empty()) {
                throw protob_server_exc_t(
                    "Authorization required but client does not support it.");
            }
        } else if (legal) {
            auth_key_t provided_auth = read_auth_key(conn.get(), &interruptor);
            if (!timing_sensitive_equals(provided_auth, auth_key)) {
                throw protob_server_exc_t("Incorrect authorization key.");
            }
            const char *success_msg = "SUCCESS";
            conn->write(success_msg, strlen(success_msg) + 1, &interruptor);
        } else {
            throw protob_server_exc_t("Received an unsupported protocol version. "
                                      "This port is for RethinkDB queries. Does your "
                                      "client driver version not match the server?");
        }

        size_t max_concurrent_queries = pre_4 ? 1 : 1024;

        // With version 0_3, the client driver specifies which protocol to use
        int32_t wire_protocol = VersionDummy::PROTOBUF;
        if (!pre_3) {
            conn->read(&wire_protocol, sizeof(wire_protocol), &interruptor);
        }

        ql::query_cache_t query_cache(rdb_ctx, client_addr_port,
                                      pre_4 ? ql::return_empty_normal_batches_t::YES :
                                              ql::return_empty_normal_batches_t::NO);

        if (wire_protocol == VersionDummy::JSON) {
            connection_loop<json_protocol_t>(
                conn.get(), max_concurrent_queries, &query_cache, &interruptor);
        } else if (wire_protocol == VersionDummy::PROTOBUF) {
            connection_loop<protobuf_protocol_t>(
                conn.get(), max_concurrent_queries, &query_cache, &interruptor);
        } else {
            throw protob_server_exc_t(strprintf("Unrecognized protocol specified: '%d'",
                                                wire_protocol));
        }
    } catch (const protob_server_exc_t &ex) {
        // Can't write response here due to coro switching inside exception handler
        init_error = strprintf("ERROR: %s\n", ex.what());
    } catch (const interrupted_exc_t &ex) {
        // If we have been interrupted, we can't write a message to the client, as that
        // may block (and we would just be interrupted again anyway), just close.
        return;
    } catch (const tcp_conn_read_closed_exc_t &) {
        return;
    } catch (const tcp_conn_write_closed_exc_t &) {
        return;
    }

    try {
        if (!init_error.empty()) {
            conn->write(init_error.c_str(), init_error.length() + 1, &interruptor);
            conn->shutdown_write();
        }
    } catch (const tcp_conn_write_closed_exc_t &) {
        // Do nothing
    }
}

void query_server_t::make_error_response(const tcp_conn_t &conn,
                                         const ql::protob_t<Query> &query,
                                         Response *response_out) {
    response_out->Clear();
    response_out->set_type(Response::RUNTIME_ERROR);
    response_out->set_token(query->token());
    Datum *msg_datum = response_out->add_response(); // Only one response, the error string
    UNUSED Backtrace *bt = response_out->mutable_backtrace(); // Empty backtrace

    msg_datum->set_type(Datum::R_STR);
    std::string *msg = msg_datum->mutable_r_str();

    // Best guess at the error that occurred
    if (!conn.is_write_open()) {
        // The other side closed it's socket - it won't get this message
        msg->assign("Client closed the connection.");
    } else if (auto_drainer.is_draining()) {
        // The query_server_t is being destroyed - we can't even write this to the socket anyway
        msg->assign("Server is shutting down.");
    } else {
        // Sort of a catch-all - there could be other reasons for this
        msg->assign("Unhandled error on another query.");
    }
}

template <class Callable>
void save_exception(std::exception_ptr *err, cond_t *abort, Callable &&fn) {
    try {
        fn();
    } catch (...) {
        if (!(*err)) {
            (*err) = std::current_exception();
        }
        abort->pulse_if_not_already_pulsed();
    }
}

template <class protocol_t>
void query_server_t::connection_loop(tcp_conn_t *conn,
                                     size_t max_concurrent_queries,
                                     ql::query_cache_t *query_cache,
                                     signal_t *raw_interruptor) {
    std::exception_ptr err;
    cond_t abort;
    new_mutex_t send_mutex;
    scoped_perfmon_counter_t connection_counter(&rdb_ctx->stats.client_connections);

    // query_info_t and the nascent_query_list_t exist to guarantee that ql::query_id_ts
    // (which are RAII) are allocated and destroyed properly.  When we read a query off
    // of the wire, it needs to be allocated a query_id_t immediately, because we then
    // put it into the coro pool.  Once it is in the coro pool, all ordering guarantees
    // are lost, so it must be done as soon as we receive the query.
    //
    // The nascent_query_list_t holds ownership of the query_info_t until it is successfully
    // handed over to the coroutine that will run the query.  This allows us to guarantee
    // proper destruction of query_id_ts in exceptional interruption or error cases.
    //
    // A ql::query_id_t is used to provide an absolute ordering of queries, and is necessary
    // for proper NOREPLY_WAIT semantics.
    typedef std::list<std::pair<ql::query_id_t, ql::protob_t<Query> > > nascent_query_list_t;
    nascent_query_list_t query_list;

    std_function_callback_t<nascent_query_list_t::iterator> callback(
        [&](nascent_query_list_t::iterator query_it,
            signal_t *pool_interruptor) {

            ql::query_id_t query_id(std::move(query_it->first));
            ql::protob_t<Query> query_pb(std::move(query_it->second));
            query_list.erase(query_it);
            bool replied = false;
            wait_any_t cb_interruptor(raw_interruptor, &abort, pool_interruptor);
            Response response;

            save_exception(&err, &abort, [&]() {
                    if (handler->run_query(query_id, query_pb, &response,
                                           query_cache, &cb_interruptor)) {
                        new_mutex_acq_t send_lock(&send_mutex, &cb_interruptor);
                        protocol_t::send_response(response, handler, conn, &cb_interruptor);
                        replied = true;
                    }
                });

            if (!replied) {
                save_exception(&err, &abort, [&]() {
                        make_error_response(*conn, query_pb, &response);
                        new_mutex_acq_t send_lock(&send_mutex, &cb_interruptor);
                        protocol_t::send_response(response, handler, conn, &cb_interruptor);
                    });
            }
        });

    // Pick a small limit so queries back up on the TCP connection.
    limited_fifo_queue_t<nascent_query_list_t::iterator> coro_queue(4);
    coro_pool_t<nascent_query_list_t::iterator> coro_pool(max_concurrent_queries,
                                                          &coro_queue,
                                                          &callback);

    wait_any_t loop_interruptor(raw_interruptor, &abort);
    while (!err) {
        ql::protob_t<Query> query(ql::make_counted_query());
        save_exception(&err, &abort, [&]() {
                if (protocol_t::parse_query(conn, &loop_interruptor, handler, &query)) {
                    query_list.push_front(std::make_pair(ql::query_id_t(query_cache),
                                                         std::move(query)));
                    coro_queue.push(query_list.begin());
                }
            });
    }

    // Respond to any queries still in the run queue
    for (auto const &pair : query_list) {
        Response response;
        save_exception(&err, &abort, [&]() {
                make_error_response(*conn, pair.second, &response);
                new_mutex_acq_t send_lock(&send_mutex, raw_interruptor);
                protocol_t::send_response(response, handler, conn, raw_interruptor);
            });
    }

    if (err) {
        std::rethrow_exception(err);
    }
}

void query_server_t::handle(const http_req_t &req,
                            http_res_t *result,
                            signal_t *interruptor) {
    auto_drainer_t::lock_t auto_drainer_lock(&auto_drainer);
    if (req.method == http_method_t::POST &&
        req.resource.as_string().find("open-new-connection") != std::string::npos) {
        int32_t conn_id = http_conn_cache.create(rdb_ctx, req.peer);

        std::string body_data;
        body_data.assign(reinterpret_cast<char *>(&conn_id), sizeof(conn_id));
        result->set_body("application/octet-stream", body_data);
        result->code = http_status_code_t::OK;
        return;
    }

    boost::optional<std::string> optional_conn_id = req.find_query_param("conn_id");
    if (!optional_conn_id) {
        *result = http_res_t(http_status_code_t::BAD_REQUEST, "application/text",
                             "Required parameter \"conn_id\" missing\n");
        return;
    }

    std::string string_conn_id = *optional_conn_id;
    int32_t conn_id = boost::lexical_cast<int32_t>(string_conn_id);

    if (req.method == http_method_t::POST &&
        req.resource.as_string().find("close-connection") != std::string::npos) {
        http_conn_cache.erase(conn_id);
        result->code = http_status_code_t::OK;
        return;
    }

    ql::protob_t<Query> query(ql::make_counted_query());
    Response response;
    int64_t token;

    if (req.body.size() < sizeof(token)) {
        *result = http_res_t(http_status_code_t::BAD_REQUEST, "application/text",
                             "Client is buggy (request too small).");
        return;
    }

    // Parse the token out from the start of the request
    const char *data = req.body.c_str();
    token = *reinterpret_cast<const int64_t *>(data);
    data += sizeof(token);

    const bool parse_succeeded =
        json_shim::parse_json_pb(query.get(), token, data);

    if (!parse_succeeded) {
        handler->unparseable_query(token, &response,
                                   "Client is buggy (failed to deserialize query).");
    } else {
        counted_t<http_conn_cache_t::http_conn_t> conn = http_conn_cache.find(conn_id);
        if (!conn.has()) {
            handler->unparseable_query(token, &response,
                                       "This HTTP connection is not open.");
        } else {
            // Check for noreply, which we don't support here, as it causes
            // problems with interruption
            ql::datum_t noreply = static_optarg("noreply", query);
            bool response_needed = !(noreply.has() &&
                                     noreply.get_type() == ql::datum_t::type_t::R_BOOL &&
                                     noreply.as_bool());

            if (!response_needed) {
                *result = http_res_t(http_status_code_t::BAD_REQUEST,
                                     "application/text",
                                     "noreply queries are not supported over HTTP\n");
                return;
            }

            wait_any_t true_interruptor(interruptor, conn->get_interruptor());
            ql::query_id_t query_id(conn->get_query_cache());
            try {
                response_needed = handler->run_query(query_id,
                                                     query, &response,
                                                     conn->get_query_cache(),
                                                     &true_interruptor);
            } catch (const interrupted_exc_t &ex) {
                // This will only be sent back if this was interrupted by a http conn
                // cache timeout.
                ql::fill_error(&response, Response::RUNTIME_ERROR,
                               "Query timed out after 5 minutes.");
            }
            rassert(response_needed);
        }
    }

    uint32_t size;
    std::string str;

    json_shim::write_json_pb(response, &str);
    size = str.size();

    char header_buffer[sizeof(token) + sizeof(size)];
    memcpy(&header_buffer[0], &token, sizeof(token));
    memcpy(&header_buffer[sizeof(token)], &size, sizeof(size));

    std::string body_data;
    body_data.reserve(sizeof(header_buffer) + str.length());
    body_data.append(&header_buffer[0], sizeof(header_buffer));
    body_data.append(str);
    result->set_body("application/octet-stream", body_data);
    result->code = http_status_code_t::OK;
}
