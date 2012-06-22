#ifndef PROTOB_PROTOB_HPP__
#define PROTOB_PROTOB_HPP__

#include "errors.hpp"
#include <boost/scoped_ptr.hpp>

#include "arch/arch.hpp"
#include "concurrency/auto_drainer.hpp"

enum protob_server_callback_mode_t {
    INLINE, //protobs that arrive will be called inline
    CORO_ORDERED, //a coroutine is spawned for each request but responses are sent back in order
    CORO_UNORDERED //a coroutine is spawned for each request and responses are sent back as they are completed
};

template <class request_t, class response_t>
class protob_server_t {
public:
    explicit protob_server_t(int port, boost::function<response_t(const request_t &)> _f, protob_server_callback_mode_t _cb_mode = CORO_ORDERED);
    ~protob_server_t();
private:

    void handle_conn(boost::scoped_ptr<nascent_tcp_conn_t> &nconn, auto_drainer_t::lock_t);
    void send(const response_t &, tcp_conn_t *conn);

    auto_drainer_t auto_drainer;
    boost::scoped_ptr<tcp_listener_t> tcp_listener;
    boost::function<response_t(const request_t &)> f;
    protob_server_callback_mode_t cb_mode;
};

#include "protob/protob.tcc"

#endif
