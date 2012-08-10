#ifndef PROTOB_PROTOB_HPP_
#define PROTOB_PROTOB_HPP_

#include "errors.hpp"
#include <boost/function.hpp>

#include "arch/arch.hpp"
#include "concurrency/auto_drainer.hpp"
#include "containers/archive/archive.hpp"
#include "rdb_protocol/stream_cache.hpp"

enum protob_server_callback_mode_t {
    INLINE, //protobs that arrive will be called inline
    CORO_ORDERED, //a coroutine is spawned for each request but responses are sent back in order
    CORO_UNORDERED //a coroutine is spawned for each request and responses are sent back as they are completed
};

template <class request_t, class response_t>
class protob_server_t {
public:
    protob_server_t(int port, boost::function<response_t(request_t *, stream_cache_t *)> _f, protob_server_callback_mode_t _cb_mode = CORO_ORDERED);
    ~protob_server_t();
private:

    void handle_conn(const scoped_ptr_t<nascent_tcp_conn_t> &nconn, auto_drainer_t::lock_t);
    void send(const response_t &, tcp_conn_t *conn, signal_t *closer) THROWS_ONLY(tcp_conn_write_closed_exc_t);

    auto_drainer_t auto_drainer;
    scoped_ptr_t<tcp_listener_t> tcp_listener;
    boost::function<response_t(request_t *, stream_cache_t *)> f;
    protob_server_callback_mode_t cb_mode;
};

//TODO figure out how to do 0 copy serialization with this.

#define RDB_MAKE_PROTOB_SERIALIZABLE_HELPER(pb_t, isinline) \
isinline write_message_t &operator<<(write_message_t &msg, const pb_t &p) { \
    int size = p.ByteSize(); \
    scoped_array_t<char> data(size); \
    p.SerializeToArray(data.data(), size); \
    int32_t size32 = size;                 \
    msg << size32;                         \
    msg.append(data.data(), data.size());  \
    return msg; \
} \
\
isinline MUST_USE archive_result_t deserialize(read_stream_t *s, pb_t *p) { \
    int32_t size;                                                       \
    archive_result_t res = deserialize(s, &size);                       \
    if (res) { return res; }                                            \
    scoped_array_t<char> data(size);                                    \
    int64_t read_res = force_read(s, data.data(), data.size());         \
    if (read_res != size) { return ARCHIVE_SOCK_ERROR; }                \
    p->ParseFromArray(data.data(), data.size()); \
    return ARCHIVE_SUCCESS;                      \
}

#define RDB_MAKE_PROTOB_SERIALIZABLE(pb_t) RDB_MAKE_PROTOB_SERIALIZABLE_HELPER(pb_t, inline)
#define RDB_IMPL_PROTOB_SERIALIZABLE(pb_t) RDB_MAKE_PROTOB_SERIALIZABLE_HELPER(pb_t, )

#include "protob/protob.tcc"

#endif /* PROTOB_PROTOB_HPP_ */
