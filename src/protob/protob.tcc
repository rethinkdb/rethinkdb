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
#include "protob/json_shim.hpp"
#include "rdb_protocol/env.hpp"
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
    int32_t client_magic_number = -1;

    try {
        if (auth_vclock.in_conflict()) {
            throw protob_server_exc_t(
                "Authorization key is in conflict, "
                "resolve it through the admin UI before connecting clients.");
        }

        conn->read(&client_magic_number, sizeof(int32_t), &ct_keepalive);

        if (client_magic_number == context_t::no_auth_magic_number) {
            if (!auth_vclock.get().str().empty()) {
                throw protob_server_exc_t(
                    "Authorization required but client does not support it.");
            }
        } else if (client_magic_number == context_t::auth_magic_number
                   || client_magic_number == context_t::json_magic_number) {
            auth_key_t provided_auth = read_auth_key(conn.get(), &ct_keepalive);
            if (!timing_sensitive_equals(provided_auth, auth_vclock.get())) {
                throw protob_server_exc_t("Incorrect authorization key.");
            }
            const char *success_msg = "SUCCESS";
            conn->write(success_msg, strlen(success_msg) + 1, &ct_keepalive);
        } else {
            throw protob_server_exc_t(
                "This is the rdb protocol port (bad magic number).");
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
        bool use_true_json = (client_magic_number == context_t::json_magic_number);
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
                scoped_array_t<char> data(size + 1);
                conn->read(data.data(), size, &ct_keepalive);
                data[size] = 0; // Null terminate in case we have JSON data.

                Query *q = underlying_protob_value(&request);
                bool res = use_true_json
                    ? json_shim::parse_json_pb(q, data.data())
                    : q->ParseFromArray(data.data(), size);
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
                    send(forced_response, use_true_json, conn.get(), &ct_keepalive);
                } else {
                    response_t response;
                    bool response_needed = f(request, &response, &ctx);
                    if (response_needed) {
                        send(response, use_true_json, conn.get(), &ct_keepalive);
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
    bool use_true_json,
    tcp_conn_t *conn,
    signal_t *closer) THROWS_ONLY(tcp_conn_write_closed_exc_t) {

    scoped_array_t<char> scoped_array;
    std::string str;
    const char *data;
    int32_t sz;
    if (use_true_json) {
        str = json_shim::write_json_pb(&res);
        data = str.data();
        sz = str.size();
    } else {
        sz = res.ByteSize();
        scoped_array.init(sz);
        res.SerializeToArray(scoped_array.data(), sz);
        data = scoped_array.data();
    }
    conn->write(&sz, sizeof(sz), closer);
    conn->write(data, sz, closer);
}

// Used in protob_server_t::handle(...) below to combine the interruptor from the
// http_conn_cache_t with the interruptor from the http_server_t in an exception-safe
// manner, and return it to how it was once handle(...) is complete.
template <class context_t>
class interruptor_mixer_t {
public:
    interruptor_mixer_t(context_t *_ctx, signal_t *new_interruptor) :
        ctx(_ctx), old_interruptor(ctx->interruptor),
        combined_interruptor(old_interruptor, new_interruptor) {
        ctx->interruptor = &combined_interruptor;
    }

    ~interruptor_mixer_t() {
        ctx->interruptor = old_interruptor;
    }

private:
    context_t *ctx;
    signal_t *old_interruptor;
    wait_any_t combined_interruptor;
};

template <class context_t>
class conn_acq_t {
public:
    conn_acq_t() : conn(NULL) { }

    bool init(typename http_conn_cache_t<context_t>::http_conn_t *_conn) {
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
    typename http_conn_cache_t<context_t>::http_conn_t *conn;
};

template <class request_t, class response_t, class context_t>
void protob_server_t<request_t, response_t, context_t>::handle(const http_req_t &req,
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

            request_t request;
            make_empty_protob_bearer(&request);
            const bool parseSucceeded
                = underlying_protob_value(&request)->ParseFromArray(data, req_size);

            response_t response;
            switch (cb_mode) {
            case INLINE:
                {
                    boost::shared_ptr<typename http_conn_cache_t<context_t>::http_conn_t> conn =
                        http_conn_cache.find(conn_id);
                    if (!parseSucceeded) {
                        std::string err = "Client is buggy (failed to deserialize protobuf).";
                        response = on_unparsable_query(request, err);
                    } else if (!conn) {
                        std::string err = "This HTTP connection is not open.";
                        response = on_unparsable_query(request, err);
                    } else {
                        // Make sure the connection is not already running a query
                        conn_acq_t<context_t> conn_acq;

                        if (!conn_acq.init(conn.get())) {
                            *result = http_res_t(HTTP_BAD_REQUEST, "application/text",
                                                 "Session is already running a query");
                            return;
                        }

                        // Check for noreply, which we don't support here, as it causes
                        // problems with interruption
                        counted_t<const ql::datum_t> noreply = static_optarg("noreply", request);
                        bool response_needed = !(noreply.has() &&
                             noreply->get_type() == ql::datum_t::type_t::R_BOOL &&
                             noreply->as_bool());

                        if (!response_needed) {
                            *result = http_res_t(HTTP_BAD_REQUEST, "application/text",
                                                 "Noreply writes unsupported over HTTP\n");
                            return;
                        }

                        context_t *ctx = conn->get_ctx();
                        interruptor_mixer_t<context_t> interruptor_mixer(ctx, interruptor);
                        response_needed = f(request, &response, ctx);
                        guarantee(response_needed);
                    }
                }
                break;
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

            std::string body_data;
            body_data.assign(res_data.data(), res_size + sizeof(res_size));
            result->set_body("application/octet-stream", body_data);
            result->code = HTTP_OK;
        }
    }
}

#endif  // PROTOB_PROTOB_TCC_
