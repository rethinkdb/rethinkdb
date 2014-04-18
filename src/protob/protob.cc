// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "protob/protob.hpp"

#include <google/protobuf/stubs/common.h>

#include <set>
#include <string>

#include "errors.hpp"
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>

#include "arch/arch.hpp"
#include "arch/io/network.hpp"
#include "clustering/administration/metadata.hpp"
#include "concurrency/cross_thread_signal.hpp"
#include "containers/auth_key.hpp"
#include "protob/json_shim.hpp"
#include "rdb_protocol/env.hpp"
#include "rpc/semilattice/joins/vclock.hpp"
#include "rpc/semilattice/view.hpp"
#include "utils.hpp"

#include "rdb_protocol/ql2.pb.h"
#include "rdb_protocol/query_server.hpp"

struct protob_server_exc_t : public std::exception {
public:
    explicit protob_server_exc_t(const std::string& data) : info(data) { }
    ~protob_server_exc_t() throw () { }
    const char *what() const throw () { return info.c_str(); }
private:
    std::string info;
};

class json_protocol_t {
public:
    static bool parse_query(ql::protob_t<Query> *query, const char *data, UNUSED int32_t size) {
        return json_shim::parse_json_pb(underlying_protob_value(query), data);
    }

    static void send_response(const Response &response, tcp_conn_t *conn, signal_t *interruptor) {
        int64_t token = response.token();
        int32_t size;
        std::string str;

        json_shim::write_json_pb(response, &str);
        size = str.size();

        conn->write(&token, sizeof(token), interruptor);
        conn->write(&size, sizeof(size), interruptor);
        conn->write(str.data(), size, interruptor);
    }
};

class protobuf_protocol_t {
public:
    static bool parse_query(ql::protob_t<Query> *query, const char *data, int32_t size) {
        return underlying_protob_value(query)->ParseFromArray(data, size);
    }

    static void send_response(const Response &response, tcp_conn_t *conn, signal_t *interruptor) {
        scoped_array_t<char> scoped_array;
        const char *data;
        int32_t size;
        size = response.ByteSize();
        scoped_array.init(size);
        response.SerializeToArray(scoped_array.data(), size);
        data = scoped_array.data();
        conn->write(&size, sizeof(size), interruptor);
        conn->write(data, size, interruptor);
    }
};

query_server_t::query_server_t(const std::set<ip_address_t> &local_addresses,
                               int port,
                               query_handler_t *_handler,
                               boost::shared_ptr<semilattice_readwrite_view_t<auth_semilattice_metadata_t> > _auth_metadata) :
        handler(_handler),
        auth_metadata(_auth_metadata),
        shutting_down_conds(get_num_threads()),
        pulse_sdc_on_shutdown(&main_shutting_down_cond),
        next_thread(0)
{
    for (int i = 0; i < get_num_threads(); ++i) {
        cross_thread_signal_t *s =
            new cross_thread_signal_t(&main_shutting_down_cond, threadnum_t(i));
        shutting_down_conds.push_back(s);
        rassert(s == &shutting_down_conds[i]);
    }

    try {
        tcp_listener.init(new tcp_listener_t(local_addresses, port,
            boost::bind(&query_server_t::handle_conn,
                        this, _1, auto_drainer_t::lock_t(&auto_drainer))));
    } catch (const address_in_use_exc_t &ex) {
        throw address_in_use_exc_t(strprintf("Could not bind to RDB protocol port: %s", ex.what()));
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
    const vclock_t<auth_key_t> auth_vclock = auth_metadata->get().auth_key;

    threadnum_t chosen_thread = threadnum_t((next_thread++) % get_num_db_threads());
    cross_thread_signal_t ct_keepalive(keepalive.get_drain_signal(), chosen_thread);
    on_thread_t rethreader(chosen_thread);

    scoped_ptr_t<tcp_conn_t> conn;
    nconn->make_overcomplicated(&conn);

#ifdef __linux
    linux_event_watcher_t *ew = conn->get_event_watcher();
    linux_event_watcher_t::watch_t conn_interrupted(ew, poll_event_rdhup);
    wait_any_t interruptor(&conn_interrupted, shutdown_signal(), &ct_keepalive);
#else
    wait_any_t interruptor(shutdown_signal(), &ct_keepalive);
#endif  // __linux
    client_context_t client_ctx(&interruptor);

    std::string init_error;
    int32_t client_magic_number = -1;

    try {
        if (auth_vclock.in_conflict()) {
            throw protob_server_exc_t(
                "Authorization key is in conflict, "
                "resolve it through the admin UI before connecting clients.");
        }

        conn->read(&client_magic_number, sizeof(client_magic_number), &interruptor);

        // With version 0_2 and up, the client drivers specifies the authorization key
        if (client_magic_number == VersionDummy::V0_1) {
            if (!auth_vclock.get().str().empty()) {
                throw protob_server_exc_t(
                    "Authorization required but client does not support it.");
            }
        } else if (client_magic_number == VersionDummy::V0_2 ||
                   client_magic_number == VersionDummy::V0_3) {
            auth_key_t provided_auth = read_auth_key(conn.get(), &interruptor);
            if (!timing_sensitive_equals(provided_auth, auth_vclock.get())) {
                throw protob_server_exc_t("Incorrect authorization key.");
            }
            const char *success_msg = "SUCCESS";
            conn->write(success_msg, strlen(success_msg) + 1, &interruptor);
        } else {
            throw protob_server_exc_t("This is the rdb protocol port (bad magic number).");
        }

        // With version 0_3, the client driver specifies which protocol to use
        int32_t wire_protocol = VersionDummy::PROTOBUF;
        if (client_magic_number == VersionDummy::V0_3) {
            conn->read(&wire_protocol, sizeof(wire_protocol), &interruptor);
        }

        if (wire_protocol == VersionDummy::JSON) {
            connection_loop<json_protocol_t>(conn.get(), &client_ctx);
        } else if (wire_protocol == VersionDummy::PROTOBUF) {
            connection_loop<protobuf_protocol_t>(conn.get(), &client_ctx);
        } else {
            throw protob_server_exc_t(strprintf("Unrecognized protocol specified: '%d'",
                                                wire_protocol));
        }
    } catch (const protob_server_exc_t &ex) {
        // Can't write response here due to coro switching inside exception handler
        init_error = strprintf("ERROR: %s\n", ex.what());
    } catch (const tcp_conn_read_closed_exc_t &) {
        return;
    } catch (const tcp_conn_write_closed_exc_t &) {
        return;
    }

    guarantee(client_magic_number != -1);

    try {
        if (!init_error.empty()) {
            conn->write(init_error.c_str(), init_error.length() + 1, &interruptor);
            conn->shutdown_write();
        }
    } catch (const tcp_conn_write_closed_exc_t &) {
        // Do nothing
    }
}

template <class protocol_t>
void query_server_t::connection_loop(tcp_conn_t *conn,
                                     client_context_t *client_ctx) {
    for (;;) {
        ql::protob_t<Query> query;
        make_empty_protob_bearer(&query);
        bool send_response = false;
        Response response;

        int32_t size;
        conn->read(&size, sizeof(int32_t), client_ctx->interruptor);
        if (size < 0) {
            handler->unparseable_query(query, &response,
                                       strprintf("Negative payload size (%d).", size));
            send_response = true;
        } else {
            scoped_array_t<char> data(size + 1);
            conn->read(data.data(), size, client_ctx->interruptor);
            data[size] = 0; // Null terminate the string, which some protocols require

            if (!protocol_t::parse_query(&query, data.data(), size)) {
                handler->unparseable_query(query, &response,
                                           "Client is buggy (failed to deserialize query).");
                send_response = true;
            }
        }

        if (!send_response) {
            send_response = handler->run_query(query, &response, client_ctx);
        }

        if (send_response) {
            protocol_t::send_response(response, conn, client_ctx->interruptor);
        }
    }
}

// Used in protob_server_t::handle(...) below to combine the interruptor from the
// http_conn_cache_t with the interruptor from the http_server_t in an exception-safe
// manner, and return it to how it was once handle(...) is complete.
class interruptor_mixer_t {
public:
    interruptor_mixer_t(client_context_t *_client_ctx, signal_t *new_interruptor) :
        client_ctx(_client_ctx), old_interruptor(client_ctx->interruptor),
        combined_interruptor(old_interruptor, new_interruptor) {
        client_ctx->interruptor = &combined_interruptor;
    }

    ~interruptor_mixer_t() {
        client_ctx->interruptor = old_interruptor;
    }

private:
    client_context_t *client_ctx;
    signal_t *old_interruptor;
    wait_any_t combined_interruptor;
};

class conn_acq_t {
public:
    conn_acq_t() : conn(NULL) { }

    bool init(typename http_conn_cache_t::http_conn_t *_conn) {
        guarantee(conn == NULL);
        if (_conn->acquire()) {
            conn = _conn;
            return true;
        }
        return false;
    }

    ~conn_acq_t() {
        if (conn != NULL) {
            conn->release();
        }
    }
private:
    typename http_conn_cache_t::http_conn_t *conn;
};

void query_server_t::handle(const http_req_t &req,
                            http_res_t *result,
                            signal_t *interruptor) {
    auto_drainer_t::lock_t auto_drainer_lock(&auto_drainer);
    if (req.method == GET &&
        req.resource.as_string().find("open-new-connection")) {
        int32_t conn_id = http_conn_cache.create();

        std::string body_data;
        body_data.assign(reinterpret_cast<char *>(&conn_id), sizeof(conn_id));
        result->set_body("application/octet-stream", body_data);
        result->code = HTTP_OK;
    } else {
        boost::optional<std::string> optional_conn_id = req.find_query_param("conn_id");
        if (!optional_conn_id) {
            *result = http_res_t(HTTP_BAD_REQUEST, "application/text",
                                 "Required parameter \"conn_id\" missing\n");
            return;
        }

        std::string string_conn_id = *optional_conn_id;
        int32_t conn_id = boost::lexical_cast<int32_t>(string_conn_id);

        if (req.method == POST &&
            req.resource.as_string().find("close-connection") != std::string::npos) {
            http_conn_cache.erase(conn_id);
            result->code = HTTP_OK;
        } else {
            // Extract protobuf from http request body
            const char *data = req.body.data();
            int32_t req_size = *reinterpret_cast<const int32_t *>(data);
            data += sizeof(req_size);

            ql::protob_t<Query> query;
            make_empty_protob_bearer(&query);
            const bool parse_succeeded =
                json_protocol_t::parse_query(&query, data, req_size);

            Response response;
            if (!parse_succeeded) {
                handler->unparseable_query(query, &response,
                                           "Client is buggy (failed to deserialize query).");
            } else {
                boost::shared_ptr<http_conn_cache_t::http_conn_t> conn =
                    http_conn_cache.find(conn_id);
                if (!conn) {
                    handler->unparseable_query(query, &response,
                                               "This HTTP connection is not open.");
                } else {
                    // Make sure the connection is not already running a query
                    conn_acq_t conn_acq;

                    if (!conn_acq.init(conn.get())) {
                        *result = http_res_t(HTTP_BAD_REQUEST, "application/text",
                                             "Session is already running a query");
                        return;
                    }

                    // Check for noreply, which we don't support here, as it causes
                    // problems with interruption
                    counted_t<const ql::datum_t> noreply = static_optarg("noreply", query);
                    bool response_needed = !(noreply.has() &&
                         noreply->get_type() == ql::datum_t::type_t::R_BOOL &&
                         noreply->as_bool());

                    if (!response_needed) {
                        *result = http_res_t(HTTP_BAD_REQUEST, "application/text",
                                             "Noreply writes unsupported over HTTP\n");
                        return;
                    }

                    client_context_t *client_ctx = conn->get_ctx();
                    interruptor_mixer_t interruptor_mixer(client_ctx, interruptor);
                    response_needed = handler->run_query(query, &response, client_ctx);
                    rassert(response_needed);
                }
            }

            int64_t token = response.token();
            int32_t size;
            std::string str;

            json_shim::write_json_pb(response, &str);
            size = str.size();

            std::string body_data;
            body_data.reserve(sizeof(token) + sizeof(size) + str.length());
            body_data.append(&token, &token + 1);
            body_data.append(&size, &size + 1);
            body_data.append(str);
            result->set_body("application/octet-stream", body_data);
            result->code = HTTP_OK;
        }
    }
}
