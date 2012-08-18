#include "protob/protob.hpp"

#include "errors.hpp"
#include <boost/bind.hpp>
#include <google/protobuf/stubs/common.h>

#include "arch/arch.hpp"

template <class request_t, class response_t, class context_t>
protob_server_t<request_t, response_t, context_t>::protob_server_t(int port, boost::function<response_t(request_t *, context_t *)> _f, response_t (*on_unparsable_query)(request_t *, const std::string&), protob_server_callback_mode_t _cb_mode)
    : f(_f), cb_mode(_cb_mode) {
    tcp_listener.init(new tcp_listener_t(port, boost::bind(&protob_server_t<request_t, response_t, context_t>::handle_conn, this, _1, auto_drainer_t::lock_t(&auto_drainer), on_unparsable_query)));
}

template <class request_t, class response_t, class context_t>
protob_server_t<request_t, response_t, context_t>::~protob_server_t() { }

template <class request_t, class response_t, class context_t>
void protob_server_t<request_t, response_t, context_t>::handle_conn(const scoped_ptr_t<nascent_tcp_conn_t> &nconn, auto_drainer_t::lock_t keepalive, response_t (*on_unparsable_query)(request_t *, const std::string&)) {
    google::protobuf::SetLogHandler(handler_log_protobuf_error);

    context_t ctx;
    scoped_ptr_t<tcp_conn_t> conn;
    nconn->ennervate(&conn);

    //TODO figure out how to do this with less copying
    for (;;) {
        request_t request;
        bool force_response = false;
        response_t forced_response;
        try {
            int size;
            conn->read(&size, sizeof(size), keepalive.get_drain_signal());

            scoped_array_t<char> data(size);
            conn->read(data.data(), size, keepalive.get_drain_signal());

            bool res = request.ParseFromArray(data.data(), size);
            if (!res) {
                forced_response = (*on_unparsable_query)(&request, "bad protocol buffer (failed to deserialize); client is buggy");
                force_response = true;
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
                        send(forced_response, conn.get(), keepalive.get_drain_signal());
                    } else {
                        send(f(&request, &ctx), conn.get(), keepalive.get_drain_signal());
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
