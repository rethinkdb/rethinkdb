#ifndef PROTOB_PROTOB_HPP_
#define PROTOB_PROTOB_HPP_

#include <string>

#include "errors.hpp"
#include <boost/function.hpp>
#include <boost/ptr_container/ptr_map.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

#include "arch/arch.hpp"
#include "concurrency/auto_drainer.hpp"
#include "concurrency/cross_thread_signal.hpp"
#include "containers/archive/archive.hpp"
#include "http/http.hpp"

enum protob_server_callback_mode_t {
    INLINE, //protobs that arrive will be called inline
    CORO_ORDERED, //a coroutine is spawned for each request but responses are sent back in order
    CORO_UNORDERED //a coroutine is spawned for each request and responses are sent back as they are completed
};

template<class context_t>
class http_conn_cache_t : public repeating_timer_callback_t {
public:
    class http_conn_t {
    public:
        http_conn_t() : last_accessed(time(0)) {
            ctx.interruptor = &interruptor;
        }
        context_t *get_ctx() {
            last_accessed = time(0);
            return &ctx;
        }
        void pulse() {
            rassert(!interruptor.is_pulsed());
            interruptor.pulse();
        }
        bool is_expired() {
            return difftime(time(0), last_accessed) > TIMEOUT_SEC;
        }
    private:
        cond_t interruptor;
        context_t ctx;
        time_t last_accessed;
        DISABLE_COPYING(http_conn_t);
    };

    http_conn_cache_t() : next_id(0), http_timeout_timer(TIMEOUT_MS, this) { }
    ~http_conn_cache_t() {
        typename std::map<int32_t, boost::shared_ptr<http_conn_t> >::iterator it;
        for (it = cache.begin(); it != cache.end(); ++it) it->second->pulse();
    }

    boost::shared_ptr<http_conn_t> find(int32_t key) {
        typename std::map<int32_t, boost::shared_ptr<http_conn_t> >::iterator
            it = cache.find(key);
        if (it == cache.end()) return boost::shared_ptr<http_conn_t>();
        return it->second;
    }
    int32_t create() {
        int32_t key = next_id++;
        cache.insert(std::make_pair(key, boost::shared_ptr<http_conn_t>(new http_conn_t())));
        return key;
    }
    size_t erase(int32_t key) { return cache.erase(key); }

    void on_ring() {
        typename std::map<int32_t, boost::shared_ptr<http_conn_t> >::iterator it, tmp;
        for (it = cache.begin(); it != cache.end();) {
            tmp = it++;
            if (tmp->second->is_expired()) {
                tmp->second->pulse();
                cache.erase(tmp);
            }
        }
    }
private:
        static const int TIMEOUT_SEC = 5*60;
        static const int TIMEOUT_MS = TIMEOUT_SEC*1000;
    std::map<int32_t, boost::shared_ptr<http_conn_t> > cache;
    int32_t next_id;
    repeating_timer_t http_timeout_timer;
};

template <class request_t, class response_t, class context_t>
class protob_server_t : public http_app_t {
public:
    // TODO: Function pointers?  Really?
    protob_server_t(int port, boost::function<response_t(request_t *, context_t *)> _f, response_t (*_on_unparsable_query)(request_t *, std::string), protob_server_callback_mode_t _cb_mode = CORO_ORDERED);
    ~protob_server_t();
    static const int32_t magic_number;
private:

    void handle_conn(const scoped_ptr_t<tcp_conn_descriptor_t> &nconn, auto_drainer_t::lock_t);
    void send(const response_t &, tcp_conn_t *conn, signal_t *closer) THROWS_ONLY(tcp_conn_write_closed_exc_t);

    // For HTTP server
    http_res_t handle(const http_req_t &);

    boost::function<response_t(request_t *, context_t *)> f;
    response_t (*on_unparsable_query)(request_t *, std::string);
    protob_server_callback_mode_t cb_mode;

    /* WARNING: The order here is fragile. */
    cond_t main_shutting_down_cond;
    signal_t *shutdown_signal() { return &shutting_down_conds[get_thread_id()]; }
    boost::ptr_vector<cross_thread_signal_t> shutting_down_conds;
    auto_drainer_t auto_drainer;
    struct pulse_on_destruct_t {
        explicit pulse_on_destruct_t(cond_t *_cond) : cond(_cond) { }
        ~pulse_on_destruct_t() { cond->pulse(); }
        cond_t *cond;
    } pulse_sdc_on_shutdown;
    http_conn_cache_t<context_t> http_conn_cache;

    scoped_ptr_t<tcp_listener_t> tcp_listener;

    unsigned next_thread;
};

template<class request_t, class response_t, class context_t>
const int32_t protob_server_t<request_t, response_t, context_t>::magic_number
    = 0xaf61ba35;

//TODO figure out how to do 0 copy serialization with this.

#define RDB_MAKE_PROTOB_SERIALIZABLE_HELPER(pb_t, isinline)             \
    isinline write_message_t &operator<<(write_message_t &msg, const pb_t &p) { \
        CT_ASSERT(sizeof(int) == sizeof(int32_t));                      \
        int size = p.ByteSize();                                        \
        scoped_array_t<char> data(size);                                \
        p.SerializeToArray(data.data(), size);                          \
        int32_t size32 = size;                                          \
        msg << size32;                                                  \
        msg.append(data.data(), data.size());                           \
        return msg;                                                     \
    }                                                                   \
                                                                        \
    isinline MUST_USE archive_result_t deserialize(read_stream_t *s, pb_t *p) { \
        CT_ASSERT(sizeof(int) == sizeof(int32_t));                      \
        int32_t size;                                                   \
        archive_result_t res = deserialize(s, &size);                   \
        if (res) { return res; }                                        \
        if (size < 0) { return ARCHIVE_RANGE_ERROR; }                   \
        scoped_array_t<char> data(size);                                \
        int64_t read_res = force_read(s, data.data(), data.size());     \
        if (read_res != size) { return ARCHIVE_SOCK_ERROR; }            \
        p->ParseFromArray(data.data(), data.size());                    \
        return ARCHIVE_SUCCESS;                                         \
    }

#define RDB_MAKE_PROTOB_SERIALIZABLE(pb_t) RDB_MAKE_PROTOB_SERIALIZABLE_HELPER(pb_t, inline)
#define RDB_IMPL_PROTOB_SERIALIZABLE(pb_t) RDB_MAKE_PROTOB_SERIALIZABLE_HELPER(pb_t, )

#include "protob/protob.tcc"

#endif /* PROTOB_PROTOB_HPP_ */
