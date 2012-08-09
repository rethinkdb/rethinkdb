#include "protob/protob.hpp"

#include "errors.hpp"
#include <boost/bind.hpp>

#include "arch/arch.hpp"

template <class request_t, class response_t>
protob_server_t<request_t, response_t>::protob_server_t(int port, boost::function<response_t(request_t *, stream_cache_t *)> _f, protob_server_callback_mode_t _cb_mode)
    : f(_f), cb_mode(_cb_mode) {
    tcp_listener.init(new tcp_listener_t(port, boost::bind(&protob_server_t<request_t, response_t>::handle_conn, this, _1, auto_drainer_t::lock_t(&auto_drainer))));
}

template <class request_t, class response_t>
protob_server_t<request_t, response_t>::~protob_server_t() { }

template <class request_t, class response_t>
void protob_server_t<request_t, response_t>::handle_conn(const scoped_ptr_t<nascent_tcp_conn_t> &nconn, auto_drainer_t::lock_t keepalive) {
    stream_cache_t stream_cache;
    scoped_ptr_t<tcp_conn_t> conn;
    nconn->ennervate(&conn);

    //TODO figure out how to do this with less copying
    for (;;) {
        request_t request;
        try {
            int size;
            conn->read(&size, sizeof(size), keepalive.get_drain_signal());

            scoped_array_t<char> data(size);
            conn->read(data.data(), size, keepalive.get_drain_signal());

            request.ParseFromArray(data.data(), size);
        } catch (tcp_conn_t::read_closed_exc_t &) {
            //TODO need to figure out what blocks us up here in non inline cb
            //mode
            return;
        }

        try {
            switch (cb_mode) {
                case INLINE:
                    send(f(&request, &stream_cache), conn.get(), keepalive.get_drain_signal());
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
        } catch (tcp_conn_t::write_closed_exc_t &) {
            //TODO need to figure out what blocks us up here in non inline cb
            //mode
            return;
        }
    }
}

template <class request_t, class response_t>
void protob_server_t<request_t, response_t>::send(const response_t &res, tcp_conn_t *conn, signal_t *closer) THROWS_ONLY(tcp_conn_t::write_closed_exc_t) {
    int size = res.ByteSize();
    conn->write(&size, sizeof(res.ByteSize()), closer);
    scoped_array_t<char> data(size);

    res.SerializeToArray(data.data(), size);
    conn->write(data.data(), size, closer);
}
