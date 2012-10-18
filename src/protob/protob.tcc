#include "protob/protob.hpp"

#include <google/protobuf/stubs/common.h>

#include <string>

#include "errors.hpp"
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>

#include "arch/arch.hpp"
#include "concurrency/cross_thread_signal.hpp"
#include "db_thread_info.hpp"

template <class request_t, class response_t, class context_t>
protob_server_t<request_t, response_t, context_t>::protob_server_t(
    int port, boost::function<response_t(request_t *, context_t *)> _f,
    response_t (*_on_unparsable_query)(request_t *, std::string),
    protob_server_callback_mode_t _cb_mode)
    : f(_f),
      on_unparsable_query(_on_unparsable_query),
      cb_mode(_cb_mode),
      shutting_down_conds(get_num_threads()),
      pulse_sdc_on_shutdown(&main_shutting_down_cond),
      next_thread(0) {

    for (int i = 0; i < get_num_threads(); ++i) {
        cross_thread_signal_t *s = new cross_thread_signal_t(&main_shutting_down_cond, i);
        shutting_down_conds.push_back(s);
        rassert(s == &shutting_down_conds[i]);
    }

    try {
        tcp_listener.init(new tcp_listener_t(port, 0, boost::bind(&protob_server_t<request_t, response_t, context_t>::handle_conn, this, _1, auto_drainer_t::lock_t(&auto_drainer))));
    } catch (address_in_use_exc_t e) {
        nice_crash("%s. Cannot bind to RDB protocol port. Exiting.\n", e.what());
    }
}

template <class request_t, class response_t, class context_t>
protob_server_t<request_t, response_t, context_t>::~protob_server_t() { }

template <class request_t, class response_t, class context_t>
void protob_server_t<request_t, response_t, context_t>::handle_conn(const scoped_ptr_t<tcp_conn_descriptor_t> &nconn, auto_drainer_t::lock_t keepalive) {
    int chosen_thread = (next_thread++) % get_num_db_threads();
    cross_thread_signal_t ct_keepalive(keepalive.get_drain_signal(), chosen_thread);
    on_thread_t rethreader(chosen_thread);
    context_t ctx;
    scoped_ptr_t<tcp_conn_t> conn;
    nconn->make_overcomplicated(&conn);

    try {
        int32_t client_magic_number;
        conn->read(&client_magic_number, sizeof(int32_t), &ct_keepalive);
        if (client_magic_number != magic_number) {
            const char *msg = "ERROR: This is the rdb protocol port! (bad magic number)\n";
            conn->write(msg, strlen(msg), &ct_keepalive);
            conn->shutdown_write();
            return;
        }
    } catch (const tcp_conn_read_closed_exc_t &) {
        return;
    } catch (const tcp_conn_write_closed_exc_t &) {
        return;
    }

    //TODO figure out how to do this with less copying
    for (;;) {
        request_t request;
        bool force_response = false;
        response_t forced_response;
        std::string err;
        try {
            int32_t size;
            conn->read(&size, sizeof(int32_t), &ct_keepalive);
            if (size < 0) {
                err = strprintf("Negative protobuf size (%d).", size);
                forced_response = on_unparsable_query(0, err);
                force_response = true;
            } else {
                scoped_array_t<char> data(size);
                conn->read(data.data(), size, &ct_keepalive);

                bool res = request.ParseFromArray(data.data(), size);
                if (!res) {
                    err = "Client is buggy (failed to deserialize protobuf).";
                    forced_response = on_unparsable_query(&request, err);
                    force_response = true;
                }
            }
        } catch (tcp_conn_read_closed_exc_t &) {
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
                        linux_event_watcher_t *ew = conn->get_event_watcher();
                        linux_event_watcher_t::watch_t conn_interrupted(ew, poll_event_rdhup);
                        wait_any_t interruptor(&conn_interrupted, shutdown_signal());
                        ctx.interruptor = &interruptor;
                        send(f(&request, &ctx), conn.get(), &ct_keepalive);
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
        } catch (tcp_conn_write_closed_exc_t &) {
            //TODO need to figure out what blocks us up here in non inline cb
            //mode
            return;
        }
    }
}

template <class request_t, class response_t, class context_t>
void protob_server_t<request_t, response_t, context_t>::send(const response_t &res, tcp_conn_t *conn, signal_t *closer) THROWS_ONLY(tcp_conn_write_closed_exc_t) {
    int size = res.ByteSize();
    conn->write(&size, sizeof(res.ByteSize()), closer);
    scoped_array_t<char> data(size);

    res.SerializeToArray(data.data(), size);
    conn->write(data.data(), size, closer);
}

template <class request_t, class response_t, class context_t>
http_res_t protob_server_t<request_t, response_t, context_t>::handle(const http_req_t &req) {
    auto_drainer_t::lock_t auto_drainer_lock(&auto_drainer);
    if (req.method == POST &&
        req.resource.as_string().find("close-connection") != std::string::npos) {

        boost::optional<std::string> optional_conn_id = req.find_query_param("conn_id");
        if (!optional_conn_id) {
            return http_res_t(HTTP_BAD_REQUEST, "application/text", "Required parameter \"conn_id\" missing\n");
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
            return http_res_t(HTTP_BAD_REQUEST, "application/text", "Required parameter \"conn_id\" missing\n");
        }
        std::string string_conn_id = *optional_conn_id;
        int32_t conn_id = boost::lexical_cast<int32_t>(string_conn_id);

        // Extract protobuf from http request body
        const char *data = req.body.data();
        int32_t req_size = *reinterpret_cast<const int32_t *>(data);
        data += sizeof(req_size);

        request_t request;
        bool parseSucceeded = request.ParseFromArray(data, req_size);

        response_t response;
        switch(cb_mode) {
        case INLINE: {
            boost::shared_ptr<typename http_conn_cache_t<context_t>::http_conn_t> conn =
                http_conn_cache.find(conn_id);
            if (!parseSucceeded) {
                std::string err = "Client is buggy (failed to deserialize protobuf).";
                response = on_unparsable_query(&request, err);
            } else if (!conn) {
                std::string err = "This HTTP connection not open.";
                response = on_unparsable_query(&request, err);
            } else {
                context_t *ctx = conn->get_ctx();
                response = f(&request, ctx);
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
