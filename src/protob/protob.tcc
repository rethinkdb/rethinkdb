// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef PROTOB_PROTOB_TCC_
#define PROTOB_PROTOB_TCC_

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
#include "rpc/semilattice/joins/vclock.hpp"
#include "rpc/semilattice/view.hpp"
#include "utils.hpp"

template <class request_t, class response_t, class context_t>
protob_server_t<request_t, response_t, context_t>::protob_server_t(
    const std::set<ip_address_t> &local_addresses,
    int port,
    boost::function<bool(request_t, response_t *, context_t *)> _f,  // NOLINT(readability/casting)
    response_t (*_on_unparsable_query)(request_t, std::string),
    boost::shared_ptr<semilattice_readwrite_view_t<auth_semilattice_metadata_t> > _auth_metadata,
    protob_server_callback_mode_t _cb_mode)
    : f(_f),
      on_unparsable_query(_on_unparsable_query),
      auth_metadata(_auth_metadata),
      cb_mode(_cb_mode),
      shutting_down_conds(get_num_threads()),
      pulse_sdc_on_shutdown(&main_shutting_down_cond),
      next_thread(0) {

    for (int i = 0; i < get_num_threads(); ++i) {
        cross_thread_signal_t *s =
            new cross_thread_signal_t(&main_shutting_down_cond, threadnum_t(i));
        shutting_down_conds.push_back(s);
        rassert(s == &shutting_down_conds[i]);
    }

    try {
        tcp_listener.init(new tcp_listener_t(
            local_addresses,
            port,
            boost::bind(&protob_server_t<request_t, response_t, context_t>::handle_conn,
                        this, _1, auto_drainer_t::lock_t(&auto_drainer))));
    } catch (const address_in_use_exc_t &ex) {
        throw address_in_use_exc_t(strprintf("Could not bind to RDB protocol port: %s", ex.what()));
    }
}
template <class request_t, class response_t, class context_t>
protob_server_t<request_t, response_t, context_t>::~protob_server_t() { }

template <class request_t, class response_t, class context_t>
int protob_server_t<request_t, response_t, context_t>::get_port() const {
    return tcp_listener->get_port();
}

struct protob_server_exc_t : public std::exception {
public:
    explicit protob_server_exc_t(const std::string& data) : info(data) { }
    ~protob_server_exc_t() throw () { }
    const char *what() const throw () { return info.c_str(); }
private:
    std::string info;
};

template <class request_t, class response_t, class context_t>
auth_key_t protob_server_t<request_t, response_t, context_t>::read_auth_key(tcp_conn_t *conn, signal_t *interruptor) {
    char buffer[auth_key_t::max_length];
    uint32_t auth_key_length;
    conn->read(&auth_key_length, sizeof(uint32_t), interruptor);

    const char *const too_long_error_message = "client provided an authorization key that is too long";

    if (auth_key_length > sizeof(buffer)) {
        throw protob_server_exc_t(too_long_error_message);
    }

    conn->read(buffer, auth_key_length, interruptor);
    auth_key_t ret;
    if (!ret.assign_value(std::string(buffer, auth_key_length))) {
        // This should never happen, since we already checked above.
        rassert(false);
        throw protob_server_exc_t(too_long_error_message);
    }
    return ret;
}

template <class request_t, class response_t, class context_t>
void protob_server_t<request_t, response_t, context_t>::handle_conn(
    const scoped_ptr_t<tcp_conn_descriptor_t> &nconn,
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
    wait_any_t interruptor(&conn_interrupted, shutdown_signal());
    context_t ctx;
    ctx.interruptor = &interruptor;
#else
    context_t ctx;
    ctx.interruptor = shutdown_signal();
#endif  // __linux

    std::string init_error;

    try {
        if (auth_vclock.in_conflict()) {
            throw protob_server_exc_t("authorization key is in conflict, resolve it through the admin UI before connecting clients");
        }

        int32_t client_magic_number;
        conn->read(&client_magic_number, sizeof(int32_t), &ct_keepalive);

        if (client_magic_number == context_t::no_auth_magic_number) {
            if (!auth_vclock.get().str().empty()) {
                throw protob_server_exc_t("authorization required, client does not support it");
            }
        } else if (client_magic_number == context_t::auth_magic_number) {
            auth_key_t provided_auth = read_auth_key(conn.get(), &ct_keepalive);
            if (!timing_sensitive_equals(provided_auth, auth_vclock.get())) {
                throw protob_server_exc_t("incorrect authorization key");
            }
            const char *success_msg = "SUCCESS";
            conn->write(success_msg, strlen(success_msg) + 1, &ct_keepalive);
        } else {
            throw protob_server_exc_t("this is the rdb protocol port (bad magic number)");
        }

    } catch (const protob_server_exc_t &ex) {
        // Can't write response here due to coro switching inside exception handler
        init_error = strprintf("ERROR: %s\n", ex.what());
    } catch (const tcp_conn_read_closed_exc_t &) {
        return;
    } catch (const tcp_conn_write_closed_exc_t &) {
        return;
    }

    try {
        if (!init_error.empty()) {
            conn->write(init_error.c_str(), init_error.length() + 1, &ct_keepalive);
            conn->shutdown_write();
            return;
        }
    } catch (const tcp_conn_write_closed_exc_t &) {
        return;
    }

    //TODO figure out how to do this with less copying
    for (;;) {
        request_t request;
        make_empty_protob_bearer(&request);
        bool force_response = false;
        response_t forced_response;
        std::string err;
        try {
            int32_t size;
            conn->read(&size, sizeof(int32_t), &ct_keepalive);
            if (size < 0) {
                err = strprintf("Negative protobuf size (%d).", size);
                forced_response = on_unparsable_query(request_t(), err);
                force_response = true;
            } else {
                scoped_array_t<char> data(size);
                conn->read(data.data(), size, &ct_keepalive);

                const bool res
                    = underlying_protob_value(&request)->ParseFromArray(data.data(), size);
                if (!res) {
                    err = "Client is buggy (failed to deserialize protobuf).";
                    forced_response = on_unparsable_query(request, err);
                    force_response = true;
                }
            }
        } catch (const tcp_conn_read_closed_exc_t &) {
            //TODO need to figure out what blocks us up here in non inline cb
            //mode
            return;
        }

        try {
            switch (cb_mode) {
            case INLINE:
                if (force_response) {
                    send(forced_response, conn.get(), &ct_keepalive);
                } else {
                    response_t response;
                    bool response_needed = f(request, &response, &ctx);
                    if (response_needed) {
                        send(response, conn.get(), &ct_keepalive);
                    }
                }
                break;
            case CORO_ORDERED:
                crash("unimplemented");
                break;
            case CORO_UNORDERED:
                crash("unimplemented");
                break;
            default:
                crash("unreachable");
                break;
            }
        } catch (const tcp_conn_write_closed_exc_t &) {
            //TODO need to figure out what blocks us up here in non inline cb
            //mode
            return;
        }
    }
}

template <class request_t, class response_t, class context_t>
void protob_server_t<request_t, response_t, context_t>::send(
    const response_t &res,
    tcp_conn_t *conn,
    signal_t *closer) THROWS_ONLY(tcp_conn_write_closed_exc_t) {
    int size = res.ByteSize();
    conn->write(&size, sizeof(res.ByteSize()), closer);
    scoped_array_t<char> data(size);

    res.SerializeToArray(data.data(), size);
    conn->write(data.data(), size, closer);
}

template <class request_t, class response_t, class context_t>
http_res_t protob_server_t<request_t, response_t, context_t>::handle(
    const http_req_t &req) {
    auto_drainer_t::lock_t auto_drainer_lock(&auto_drainer);
    if (req.method == POST &&
        req.resource.as_string().find("close-connection") != std::string::npos) {

        boost::optional<std::string> optional_conn_id = req.find_query_param("conn_id");
        if (!optional_conn_id) {
            return http_res_t(HTTP_BAD_REQUEST, "application/text",
                              "Required parameter \"conn_id\" missing\n");
        }

        std::string string_conn_id = *optional_conn_id;
        int32_t conn_id = boost::lexical_cast<int32_t>(string_conn_id);
        http_conn_cache.erase(conn_id);

        http_res_t res(HTTP_OK);
        res.version = "HTTP/1.1";
        res.add_header_line("Access-Control-Allow-Origin", "*");

        return res;
    } else if (req.method == GET &&
               req.resource.as_string().find("open-new-connection")) {
        int32_t conn_id = http_conn_cache.create();

        http_res_t res(HTTP_OK);
        res.version = "HTTP/1.1";
        res.add_header_line("Access-Control-Allow-Origin", "*");
        std::string body_data;
        body_data.assign(reinterpret_cast<char *>(&conn_id), sizeof(conn_id));
        res.set_body("application/octet-stream", body_data);

        return res;
    } else {
        boost::optional<std::string> optional_conn_id = req.find_query_param("conn_id");
        if (!optional_conn_id) {
            return http_res_t(HTTP_BAD_REQUEST, "application/text",
                              "Required parameter \"conn_id\" missing\n");
        }
        std::string string_conn_id = *optional_conn_id;
        int32_t conn_id = boost::lexical_cast<int32_t>(string_conn_id);

        // Extract protobuf from http request body
        const char *data = req.body.data();
        int32_t req_size = *reinterpret_cast<const int32_t *>(data);
        data += sizeof(req_size);

        request_t request;
        make_empty_protob_bearer(&request);
        const bool parseSucceeded
            = underlying_protob_value(&request)->ParseFromArray(data, req_size);

        bool response_needed;
        response_t response;
        switch (cb_mode) {
        case INLINE: {
            boost::shared_ptr<typename http_conn_cache_t<context_t>::http_conn_t> conn =
                http_conn_cache.find(conn_id);
            if (!parseSucceeded) {
                std::string err = "Client is buggy (failed to deserialize protobuf).";
                response = on_unparsable_query(request, err);
            } else if (!conn) {
                std::string err = "This HTTP connection not open.";
                response = on_unparsable_query(request, err);
            } else {
                context_t *ctx = conn->get_ctx();
                response_needed = f(request, &response, ctx);
                if (!response_needed) {
                    return http_res_t(HTTP_BAD_REQUEST, "application/text",
                                      "Noreply writes unsupported over HTTP\n");
                }
            }
        } break;
        case CORO_ORDERED:
        case CORO_UNORDERED:
            crash("unimplemented");
        default:
            crash("unreachable");
            break;
        }

        int32_t res_size = response.ByteSize();
        scoped_array_t<char> res_data(sizeof(res_size) + res_size);
        *reinterpret_cast<int32_t *>(res_data.data()) = res_size;
        response.SerializeToArray(res_data.data() + sizeof(res_size), res_size);

        http_res_t res(HTTP_OK);
        res.version = "HTTP/1.1";
        res.add_header_line("Access-Control-Allow-Origin", "*");

        std::string body_data;
        body_data.assign(res_data.data(), res_size + sizeof(res_size));
        res.set_body("application/octet-stream", body_data);

        return res;
    }
}

#endif  // PROTOB_PROTOB_TCC_
