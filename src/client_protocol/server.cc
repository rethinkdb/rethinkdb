// Copyright 2010-2015 RethinkDB, all rights reserved.

// We need to include `openssl/evp.h` first, since it declares a function with the
// name `final`.
// Because of missing support for the `final` annotation in older GCC versions,
// we redefine final to the empty string in `errors.hpp`. So we must make
// sure that we haven't included `errors.hpp` by the time we include `evp.h`.
#include <openssl/evp.h> // NOLINT(build/include_order)

#include "client_protocol/server.hpp" // NOLINT(build/include_order)

#include <google/protobuf/stubs/common.h> // NOLINT(build/include_order)

#include <array> // NOLINT(build/include_order)
#include <random> // NOLINT(build/include_order)
#include <set> // NOLINT(build/include_order)
#include <string> // NOLINT(build/include_order)

#include "arch/arch.hpp"
#include "arch/io/network.hpp"
#include "client_protocol/client_server_error.hpp"
#include "client_protocol/protocols.hpp"
#include "clustering/administration/auth/authentication_error.hpp"
#include "clustering/administration/auth/plaintext_authenticator.hpp"
#include "clustering/administration/auth/scram_authenticator.hpp"
#include "clustering/administration/auth/username.hpp"
#include "clustering/administration/metadata.hpp"
#include "concurrency/coro_pool.hpp"
#include "concurrency/cross_thread_signal.hpp"
#include "concurrency/queue/limited_fifo.hpp"
#include "crypto/error.hpp"
#include "perfmon/perfmon.hpp"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rdb_protocol/rdb_backtrace.hpp"
#include "rdb_protocol/base64.hpp"
#include "rdb_protocol/env.hpp"
#include "rpc/semilattice/view.hpp"
#include "time.hpp"

#include "rdb_protocol/ql2.pb.h"
#include "rdb_protocol/query_server.hpp"
#include "rdb_protocol/query_cache.hpp"
#include "rdb_protocol/response.hpp"

http_conn_cache_t::http_conn_t::http_conn_t(rdb_context_t *rdb_ctx,
                                            ip_and_port_t client_addr_port) :
    last_accessed(time(nullptr)),
    // We always return empty normal batches after the timeout for HTTP
    // connections; I think we have to do this to keep the conn cache
    // from timing out.
    query_cache(
        new ql::query_cache_t(
            rdb_ctx,
            client_addr_port,
            ql::return_empty_normal_batches_t::YES,
            auth::user_context_t(auth::username_t("admin")))),
    counter(&rdb_ctx->stats.client_connections) { }

ql::query_cache_t *http_conn_cache_t::http_conn_t::get_query_cache() {
    last_accessed = time(nullptr);
    return query_cache.get();
}

signal_t *http_conn_cache_t::http_conn_t::get_interruptor() {
    return &interruptor;
}

void http_conn_cache_t::http_conn_t::pulse() {
    rassert(!interruptor.is_pulsed());
    interruptor.pulse();
}

time_t http_conn_cache_t::http_conn_t::last_accessed_time() const {
    return last_accessed;
}

http_conn_cache_t::http_conn_cache_t(uint32_t _http_timeout_sec) :
    http_timeout_timer(TIMER_RESOLUTION_MS, this),
    http_timeout_sec(_http_timeout_sec) {

    // Seed the random number generator from a true random source.
    // Note1: On some platforms std::random_device might not actually be
    //   non-deterministic, and it seems that there is no reliable way to tell.
    //   On major platforms it should be fine.
    // Note2: std::random_device() might block for a while. Since we only create
    //   http_conn_cache_t once at startup, that should be fine. But it's something
    //   to keep in mind.

    // Seed with an amount of bits equal to the state size of key_generator.
    static_assert(std::mt19937::word_size == 32,
                  "std::mt19937's word size doesn't match what we expected.");
    std::array<uint32_t, std::mt19937::state_size> seed_data;
    std::random_device rd;
    for (size_t i = 0; i < seed_data.size(); ++i) {
        seed_data[i] = rd();
    }
    std::seed_seq seed_seq(seed_data.begin(), seed_data.end());
    key_generator.seed(seed_seq);
}

http_conn_cache_t::~http_conn_cache_t() {
    for (auto &pair : cache) pair.second->pulse();
}

std::string http_conn_cache_t::expired_error_message() const {
    return strprintf("HTTP ReQL query timed out after %" PRIu32 " seconds.",
                     http_timeout_sec);
}

bool http_conn_cache_t::is_expired(const http_conn_t &conn) const {
    return difftime(time(nullptr), conn.last_accessed_time()) > http_timeout_sec;
}

counted_t<http_conn_cache_t::http_conn_t> http_conn_cache_t::find(
        const conn_key_t &key) {
    assert_thread();
    auto conn_it = cache.find(key);
    if (conn_it == cache.end()) return counted_t<http_conn_t>();
    return conn_it->second;
}

http_conn_cache_t::conn_key_t http_conn_cache_t::create(
        rdb_context_t *rdb_ctx,
        ip_and_port_t client_addr_port) {
    assert_thread();
    // Generate a 128 bit random key to avoid XSS attacks where someone
    // could run queries by guessing the connection ID.
    // The same origin policy of browsers will stop attackers from seeing
    // the response of the connection setup, so the attacker will have no chance
    // of getting a valid connection ID.
    uint32_t key_buf[4];
    for(size_t i = 0; i < 4; ++i) {
        key_buf[i] = key_generator();
    }
    conn_key_t key = encode_base64(reinterpret_cast<const char *>(key_buf),
                                   sizeof(key_buf));

    cache.insert(
        std::make_pair(key, make_counted<http_conn_t>(rdb_ctx, client_addr_port)));
    return key;
}

void http_conn_cache_t::erase(const conn_key_t &key) {
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
        if (is_expired(*tmp->second)) {
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

size_t http_conn_cache_t::sha_hasher_t::operator()(const conn_key_t &x) const {
    EVP_MD_CTX c;
    EVP_DigestInit(&c, EVP_sha256());
    EVP_DigestUpdate(&c, x.data(), x.size());
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digest_size = 0;
    EVP_DigestFinal(&c, digest, &digest_size);
    rassert(digest_size >= sizeof(size_t));
    size_t res = 0;
    memcpy(&res, digest, std::min(sizeof(size_t), static_cast<size_t>(digest_size)));
    return res;
}

query_server_t::query_server_t(rdb_context_t *_rdb_ctx,
                               const std::set<ip_address_t> &local_addresses,
                               int port,
                               query_handler_t *_handler,
                               uint32_t http_timeout_sec,
                               tls_ctx_t *_tls_ctx) :
        tls_ctx(_tls_ctx),
        rdb_ctx(_rdb_ctx),
        handler(_handler),
        http_conn_cache(http_timeout_sec),
        next_thread(0) {
    rassert(rdb_ctx != nullptr);
    try {
        tcp_listener.init(new tcp_listener_t(local_addresses, port,
            std::bind(&query_server_t::handle_conn,
                      this, ph::_1, auto_drainer_t::lock_t(&drainer))));
    } catch (const address_in_use_exc_t &ex) {
        throw address_in_use_exc_t(
            strprintf("Could not bind to RDB protocol port: %s", ex.what()));
    }
}

query_server_t::~query_server_t() { }

int query_server_t::get_port() const {
    return tcp_listener->get_port();
}

void write_datum(tcp_conn_t *connection, ql::datum_t datum, signal_t *interruptor) {
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    datum.write_json(&writer);
    buffer.Put('\0');
    connection->write(buffer.GetString(), buffer.GetSize(), interruptor);
}

ql::datum_t read_datum(tcp_conn_t *connection, signal_t *interruptor) {
    std::array<char, 2048> buffer;

    std::size_t offset = 0;
    do {
        connection->read_buffered(buffer.data() + offset, 1, interruptor);
    } while (buffer[offset++] != '\0' && offset < buffer.max_size());
    if (offset == 2048) {
        throw client_protocol::client_server_error_t(7, "Limited read buffer size.");
    }

    rapidjson::Document document;
    document.Parse(buffer.data());
    if (document.HasParseError()) {
        throw client_protocol::client_server_error_t(8, "Invalid JSON object.");
    }

    try {
        return ql::to_datum(
            document,
            ql::configured_limits_t::unlimited,
            reql_version_t::LATEST);
    } catch (ql::base_exc_t const &) {
        throw client_protocol::client_server_error_t(9, "Failed to convert JSON to datum.");
    }
}

void query_server_t::handle_conn(const scoped_ptr_t<tcp_conn_descriptor_t> &nconn,
                                 auto_drainer_t::lock_t keepalive) {
    threadnum_t chosen_thread = threadnum_t(next_thread);
    next_thread = (next_thread + 1) % get_num_db_threads();

    cross_thread_signal_t ct_keepalive(keepalive.get_drain_signal(), chosen_thread);
    on_thread_t rethreader(chosen_thread);

    scoped_ptr_t<tcp_conn_t> conn;

    try {
        nconn->make_server_connection(tls_ctx, &conn, &ct_keepalive);
    } catch (const interrupted_exc_t &) {
        // TLS handshake was interrupted.
        return;
    } catch (const crypto::openssl_error_t &err) {
        // TLS handshake failed.
        logWRN("Driver connection TLS handshake failed: %s", err.what());
        return;
    }

    conn->enable_keepalive();

    uint8_t version = 0;
    std::unique_ptr<auth::base_authenticator_t> authenticator;
    uint32_t error_code = 0;
    std::string error_message;
    try {
        int32_t client_magic_number;
        conn->read_buffered(
            &client_magic_number, sizeof(client_magic_number), &ct_keepalive);

        switch (client_magic_number) {
            case VersionDummy::V0_1:
                version = 1;
                break;
            case VersionDummy::V0_2:
                version = 2;
                break;
            case VersionDummy::V0_3:
                version = 3;
                break;
            case VersionDummy::V0_4:
                version = 4;
                break;
            case VersionDummy::V1_0:
                version = 10;
                break;
            default:
                throw client_protocol::client_server_error_t(
                    -1,
                    "Received an unsupported protocol version. This port is for "
                    "RethinkDB queries. Does your client driver version not match the "
                    "server?");
        }
        if (version < 3) {
            // `V0_1` and `V0_2` only supported the PROTOBUF protocol
            throw client_protocol::client_server_error_t(
                -1, "The PROTOBUF client protocol is no longer supported");
        } else if (version < 10) {
            // We'll get std::make_unique in C++14
            authenticator.reset(
                new auth::plaintext_authenticator_t(rdb_ctx->get_auth_watchable()));

            uint32_t auth_key_size;
            conn->read_buffered(&auth_key_size, sizeof(uint32_t), &ct_keepalive);
            if (auth_key_size > 2048) {
                throw client_protocol::client_server_error_t(
                    -1, "Client provided an authorization key that is too long.");
            }

            scoped_array_t<char> auth_key_buffer(auth_key_size);
            conn->read_buffered(auth_key_buffer.data(), auth_key_size, &ct_keepalive);

            try {
                authenticator->next_message(
                    std::string(auth_key_buffer.data(), auth_key_size));
            } catch (auth::authentication_error_t const &) {
                // Note we must not change this message for backwards compatibility
                throw client_protocol::client_server_error_t(
                    -1, "Incorrect authorization key.");
            }

            int32_t wire_protocol;
            conn->read_buffered(&wire_protocol, sizeof(wire_protocol), &ct_keepalive);
            switch (wire_protocol) {
                case VersionDummy::JSON:
                    break;
                case VersionDummy::PROTOBUF:
                    throw client_protocol::client_server_error_t(
                        -1, "The PROTOBUF client protocol is no longer supported");
                    break;
                default:
                    throw client_protocol::client_server_error_t(
                        -1,
                        strprintf(
                            "Unrecognized protocol specified: '%d'", wire_protocol));
            }

            char const *success_msg = "SUCCESS";
            conn->write(success_msg, strlen(success_msg) + 1, &ct_keepalive);
        } else {
            authenticator.reset(
                new auth::scram_authenticator_t(rdb_ctx->get_auth_watchable()));

            {
                ql::datum_object_builder_t datum_object_builder;
                datum_object_builder.overwrite("success", ql::datum_t::boolean(true));
                datum_object_builder.overwrite("max_protocol_version", ql::datum_t(0.0));
                datum_object_builder.overwrite("min_protocol_version", ql::datum_t(0.0));
                datum_object_builder.overwrite(
                    "server_version", ql::datum_t(RETHINKDB_VERSION));

                write_datum(
                    conn.get(),
                    std::move(datum_object_builder).to_datum(),
                    &ct_keepalive);
            }

            {
                ql::datum_t datum = read_datum(conn.get(), &ct_keepalive);

                ql::datum_t protocol_version =
                    datum.get_field("protocol_version", ql::NOTHROW);
                if (protocol_version.get_type() != ql::datum_t::R_NUM) {
                    throw client_protocol::client_server_error_t(
                        1, "Expected a number for `protocol_version`.");
                }
                if (protocol_version.as_num() != 0.0) {
                    throw client_protocol::client_server_error_t(
                        2, "Unsupported `protocol_version`.");
                }

                ql::datum_t authentication_method =
                    datum.get_field("authentication_method", ql::NOTHROW);
                if (authentication_method.get_type() != ql::datum_t::R_STR) {
                    throw client_protocol::client_server_error_t(
                        3, "Expected a string for `authentication_method`.");
                }
                if (authentication_method.as_str() != "SCRAM-SHA-256") {
                    throw client_protocol::client_server_error_t(
                        4, "Unsupported `authentication_method`.");
                }

                ql::datum_t authentication =
                    datum.get_field("authentication", ql::NOTHROW);
                if (authentication.get_type() != ql::datum_t::R_STR) {
                    throw client_protocol::client_server_error_t(
                        5, "Expected a string for `authentication`.");
                }

                ql::datum_object_builder_t datum_object_builder;
                datum_object_builder.overwrite("success", ql::datum_t::boolean(true));
                datum_object_builder.overwrite(
                    "authentication",
                    ql::datum_t(
                        authenticator->next_message(authentication.as_str().to_std())));

                write_datum(
                    conn.get(),
                    std::move(datum_object_builder).to_datum(),
                    &ct_keepalive);
            }

            {
                ql::datum_t datum = read_datum(conn.get(), &ct_keepalive);

                ql::datum_t authentication =
                    datum.get_field("authentication", ql::NOTHROW);
                if (authentication.get_type() != ql::datum_t::R_STR) {
                    throw client_protocol::client_server_error_t(
                        5, "Expected a string for `authentication`.");
                }

                ql::datum_object_builder_t datum_object_builder;
                datum_object_builder.overwrite("success", ql::datum_t::boolean(true));
                datum_object_builder.overwrite(
                    "authentication",
                    ql::datum_t(
                        authenticator->next_message(authentication.as_str().to_std())));

                write_datum(
                    conn.get(),
                    std::move(datum_object_builder).to_datum(),
                    &ct_keepalive);
            }
        }

        ip_and_port_t client_addr_port(ip_address_t::any(AF_INET), port_t(0));
        UNUSED bool peer_res = conn->getpeername(&client_addr_port);

        guarantee(authenticator != nullptr);
        ql::query_cache_t query_cache(
            rdb_ctx,
            client_addr_port,
            (version < 4)
                ? ql::return_empty_normal_batches_t::YES
                : ql::return_empty_normal_batches_t::NO,
            auth::user_context_t(authenticator->get_authenticated_username()));

        connection_loop<json_protocol_t>(
            conn.get(),
            (version < 4)
                ? 1
                : 1024,
            &query_cache,
            &ct_keepalive);
    } catch (client_protocol::client_server_error_t const &error) {
        // We can't write the response here due to coroutine switching inside an
        // exception handler
        error_code = error.get_error_code();
        error_message = error.what();
    } catch (interrupted_exc_t const &) {
        // If we have been interrupted, we can't write a message to the client, as that
        // may block (and we would just be interrupted again anyway), just close.
    } catch (auth::authentication_error_t const &error) {
        // Note these have error codes 10 to 20
        error_code = error.get_error_code();
        error_message = error.what();
    } catch (crypto::error_t const &error) {
        error_code = 21;
        error_message = error.what();
    } catch (crypto::openssl_error_t const &error) {
        error_code = 22;
        error_message = error.code().message();
    } catch (const tcp_conn_read_closed_exc_t &) {
    } catch (const tcp_conn_write_closed_exc_t &) {
    } catch (const std::exception &ex) {
        logERR("Unexpected exception in client handler: %s", ex.what());
    }

    if (!error_message.empty()) {
        try {
            if (version < 10) {
                std::string error = "ERROR: " + error_message + "\n";
                conn->write(error.c_str(), error.length() + 1, &ct_keepalive);
            } else {
                ql::datum_object_builder_t datum_object_builder;
                datum_object_builder.overwrite("success", ql::datum_t::boolean(false));
                datum_object_builder.overwrite("error", ql::datum_t(error_message));
                datum_object_builder.overwrite(
                    "error_code", ql::datum_t(static_cast<double>(error_code)));

                write_datum(
                    conn.get(),
                    std::move(datum_object_builder).to_datum(),
                    &ct_keepalive);
            }

            conn->shutdown_write();
        } catch (client_protocol::client_server_error_t const &error) {
        } catch (interrupted_exc_t const &) {
        } catch (const tcp_conn_write_closed_exc_t &) {
            // Writing the error message failed, there is nothing we can do at this point
        }
    }
}

void query_server_t::make_error_response(bool is_draining,
                                         const tcp_conn_t &conn,
                                         const std::string &err_str,
                                         ql::response_t *response_out) {
    // Best guess at the error that occurred.
    if (!conn.is_write_open()) {
        // The other side closed it's socket - it won't get this message
        response_out->fill_error(Response::RUNTIME_ERROR,
                                 Response::OP_INDETERMINATE,
                                 "Client closed the connection.",
                                 ql::backtrace_registry_t::EMPTY_BACKTRACE);
    } else if (is_draining) {
        // The query_server_t is being destroyed so this won't actually be written
        response_out->fill_error(Response::RUNTIME_ERROR,
                                 Response::OP_INDETERMINATE,
                                 "Server is shutting down.",
                                 ql::backtrace_registry_t::EMPTY_BACKTRACE);
    } else {
        // Sort of a catch-all - there could be other reasons for this
        response_out->fill_error(
            Response::RUNTIME_ERROR, Response::OP_INDETERMINATE,
            strprintf("Fatal error on another query: %s", err_str.c_str()),
            ql::backtrace_registry_t::EMPTY_BACKTRACE);
    }
}

template <class Callable>
void save_exception(std::exception_ptr *err,
                    std::string *err_str,
                    cond_t *abort,
                    Callable &&fn) {
    try {
        fn();
    } catch (const std::exception &ex) {
        if (!(*err)) {
            *err = std::current_exception();
            err_str->assign(ex.what());
        }
        abort->pulse_if_not_already_pulsed();
    }
}

template <class protocol_t>
void query_server_t::connection_loop(tcp_conn_t *conn,
                                     size_t max_concurrent_queries,
                                     ql::query_cache_t *query_cache,
                                     signal_t *drain_signal) {
    std::exception_ptr err;
    std::string err_str;
    cond_t abort;
    new_mutex_t send_mutex;
    scoped_perfmon_counter_t connection_counter(&rdb_ctx->stats.client_connections);

#ifdef __linux
    linux_event_watcher_t *ew = conn->get_event_watcher();
    linux_event_watcher_t::watch_t conn_interrupted(ew, poll_event_rdhup);
    wait_any_t interruptor(drain_signal, &abort, &conn_interrupted);
#else
    wait_any_t interruptor(drain_signal, &abort);
#endif  // __linux

    new_semaphore_t sem(max_concurrent_queries);
    auto_drainer_t coro_drainer;
    while (!err) {
        scoped_ptr_t<ql::query_params_t> outer_query =
            protocol_t::parse_query(conn, &interruptor, query_cache);
        if (outer_query.has()) {
            outer_query->throttler.init(&sem, 1);
            wait_interruptible(outer_query->throttler.acquisition_signal(),
                               &interruptor);
            coro_t::spawn_now_dangerously([&]() {
                // We grab this right away while it's still valid.
                scoped_ptr_t<ql::query_params_t> query = std::move(outer_query);
                // Since we `spawn_now_dangerously` it's always safe to acquire this.
                auto_drainer_t::lock_t coro_drainer_lock(&coro_drainer);
                wait_any_t cb_interruptor(coro_drainer_lock.get_drain_signal(),
                                          &interruptor);
                ql::response_t response;
                bool replied = false;

                save_exception(&err, &err_str, &abort, [&]() {
                    handler->run_query(query.get(), &response, &cb_interruptor);
                    if (!query->noreply) {
                        new_mutex_acq_t send_lock(&send_mutex, &cb_interruptor);
                        protocol_t::send_response(&response, query->token,
                                                  conn, &cb_interruptor);
                        replied = true;
                    }
                });
                save_exception(&err, &err_str, &abort, [&]() {
                    if (!replied && !query->noreply) {
                        make_error_response(drain_signal->is_pulsed(), *conn,
                                            err_str, &response);
                        new_mutex_acq_t send_lock(&send_mutex, drain_signal);
                        protocol_t::send_response(&response, query->token,
                                                  conn, &cb_interruptor);
                    }
                });
            });
            guarantee(!outer_query.has());
            // Since we're using `spawn_now_dangerously` above, we need to yield
            // here to stop a client sending a constant stream of expensive queries
            // from stalling the thread.
            coro_t::yield();
        }
    }

    if (err) {
        std::rethrow_exception(err);
    }
}

void query_server_t::handle(const http_req_t &req,
                            http_res_t *result,
                            signal_t *interruptor) {
    auto_drainer_t::lock_t auto_drainer_lock(&drainer);
    if (req.method == http_method_t::POST &&
        req.resource.as_string().find("open-new-connection") != std::string::npos) {
        http_conn_cache_t::conn_key_t conn_id
            = http_conn_cache.create(rdb_ctx, req.peer);

        result->set_body("text/plain", conn_id);
        result->code = http_status_code_t::OK;
        return;
    }

    boost::optional<std::string> optional_conn_id = req.find_query_param("conn_id");
    if (!optional_conn_id) {
        *result = http_res_t(http_status_code_t::BAD_REQUEST, "application/text",
                             "Required parameter \"conn_id\" missing\n");
        return;
    }

    http_conn_cache_t::conn_key_t conn_id = *optional_conn_id;

    if (req.method == http_method_t::POST &&
        req.resource.as_string().find("close-connection") != std::string::npos) {
        http_conn_cache.erase(conn_id);
        result->code = http_status_code_t::OK;
        return;
    }

    int64_t token = 0;
    if (req.body.size() < sizeof(token)) {
        *result = http_res_t(http_status_code_t::BAD_REQUEST, "application/text",
                             "Client is buggy (request too small).");
        return;
    }

    // Copy the body into a mutable buffer so we can move it into parse_json_pb.
    scoped_array_t<char> body_buf(req.body.size() + 1);
    memcpy(body_buf.data(), req.body.data(), req.body.size());
    body_buf[req.body.size()] = '\0';

    // Parse the token out from the start of the request
    char *data = body_buf.data();
    token = *reinterpret_cast<const int64_t *>(data);
    data += sizeof(token);

    ql::response_t response;
    counted_t<http_conn_cache_t::http_conn_t> conn = http_conn_cache.find(conn_id);
    if (!conn.has()) {
        response.fill_error(Response::CLIENT_ERROR, Response::INTERNAL,
                            "This HTTP connection is not open.",
                            ql::backtrace_registry_t::EMPTY_BACKTRACE);
    } else {
        scoped_ptr_t<ql::query_params_t> query =
            json_protocol_t::parse_query_from_buffer(std::move(body_buf),
                                                     sizeof(token),
                                                     conn->get_query_cache(),
                                                     token,
                                                     &response);

        if (query.has()) {
            // Check for noreply, which we don't support here, as it causes
            // problems with interruption
            if (query->noreply) {
                *result = http_res_t(http_status_code_t::BAD_REQUEST,
                                     "application/text",
                                     "noreply queries are not supported over HTTP\n");
                return;
            }

            wait_any_t true_interruptor(interruptor, conn->get_interruptor(),
                                        drainer.get_drain_signal());

            try {
                ticks_t start = get_ticks();
                // We don't throttle HTTP queries.
                handler->run_query(query.get(), &response, &true_interruptor);
                ticks_t ticks = get_ticks() - start;

                if (!response.profile()) {
                    ql::datum_array_builder_t array_builder(
                        ql::configured_limits_t::unlimited);
                    ql::datum_object_builder_t object_builder;
                    object_builder.overwrite("duration(ms)",
                        ql::datum_t(static_cast<double>(ticks) / MILLION));
                    array_builder.add(std::move(object_builder).to_datum());
                    response.set_profile(std::move(array_builder).to_datum());
                }
            } catch (const interrupted_exc_t &ex) {
                if (http_conn_cache.is_expired(*conn)) {
                    response.fill_error(Response::RUNTIME_ERROR,
                                        Response::OP_INDETERMINATE,
                                        http_conn_cache.expired_error_message(),
                                        ql::backtrace_registry_t::EMPTY_BACKTRACE);
                } else if (interruptor->is_pulsed()) {
                    response.fill_error(Response::RUNTIME_ERROR,
                                        Response::OP_INDETERMINATE,
                                        "This ReQL connection has been terminated.",
                                        ql::backtrace_registry_t::EMPTY_BACKTRACE);
                } else if (drainer.is_draining()) {
                    response.fill_error(Response::RUNTIME_ERROR,
                                        Response::OP_INDETERMINATE,
                                        "Server is shutting down.",
                                        ql::backtrace_registry_t::EMPTY_BACKTRACE);
                } else if (conn->get_interruptor()->is_pulsed()) {
                    response.fill_error(Response::RUNTIME_ERROR,
                                        Response::OP_INDETERMINATE,
                                        "This ReQL connection has been terminated.",
                                        ql::backtrace_registry_t::EMPTY_BACKTRACE);
                } else {
                    throw;
                }
            }
        }
    }

    rapidjson::StringBuffer buffer;
    json_protocol_t::write_response_to_buffer(&response, &buffer);

    uint32_t size = static_cast<uint32_t>(buffer.GetSize());
    char header_buffer[sizeof(token) + sizeof(size)];
    memcpy(&header_buffer[0], &token, sizeof(token));
    memcpy(&header_buffer[sizeof(token)], &size, sizeof(size));

    std::string body_data;
    body_data.reserve(sizeof(header_buffer) + buffer.GetSize());
    body_data.append(&header_buffer[0], sizeof(header_buffer));
    body_data.append(buffer.GetString(), buffer.GetSize());
    result->set_body("application/octet-stream", body_data);
    result->code = http_status_code_t::OK;
}
