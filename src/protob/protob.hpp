#ifndef PROTOB_PROTOB_HPP_
#define PROTOB_PROTOB_HPP_

#include "errors.hpp"
#include <boost/function.hpp>

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

    void handle_conn(scoped_ptr_t<nascent_tcp_conn_t> &nconn, auto_drainer_t::lock_t);
    void send(const response_t &, tcp_conn_t *conn);

    auto_drainer_t auto_drainer;
    scoped_ptr_t<tcp_listener_t> tcp_listener;
    boost::function<response_t(const request_t &)> f;
    protob_server_callback_mode_t cb_mode;
};

#include "protob/protob.tcc"

#endif /* PROTOB_PROTOB_HPP_ */
