#ifndef __REPLICATION_PROTOCOL_HPP__
#define __REPLICATION_PROTOCOL_HPP__

#include "errors.hpp"
#include <boost/function.hpp>

#include "arch/arch.hpp"
#include "concurrency/fifo_checker.hpp"
#include "concurrency/drain_semaphore.hpp"
#include "concurrency/mutex.hpp"
#include "containers/scoped_malloc.hpp"
#include "containers/thick_list.hpp"
#include "data_provider.hpp"
#include "replication/multistream.hpp"
#include "replication/net_structs.hpp"


namespace replication {

template <class T>
struct stream_pair {
    unique_ptr_t<buffered_data_provider_t> stream;
    unique_malloc_t<T> data;

    // This uses key_size, which is completely crap.
    stream_pair(const char *beg, const char *end, size_t size = 0) : stream(), data() {
        void *p;
        size_t m = sizeof(T) + reinterpret_cast<const T *>(beg)->key_size;

        const char *cutpoint = beg + m;
        {
            unique_malloc_t<T> tmp(beg, cutpoint);
            data = tmp;
        }

        stream.reset(new buffered_data_provider_t(size == 0 ? end - cutpoint : size, &p));

        memcpy(p, cutpoint, end - cutpoint);
    }

    T *operator->() { return data.get(); }
};

class message_callback_t {
public:
    // These could call .swap on their parameter, taking ownership of the pointee.
    virtual void hello(net_hello_t message) = 0;
    virtual void send(scoped_malloc<net_introduce_t>& message) = 0;
    virtual void send(scoped_malloc<net_backfill_t>& message) = 0;
    virtual void send(scoped_malloc<net_backfill_complete_t>& message) = 0;
    virtual void send(scoped_malloc<net_backfill_delete_everything_t>& message) = 0;
    virtual void send(scoped_malloc<net_backfill_delete_t>& message) = 0;
    virtual void send(stream_pair<net_backfill_set_t>& message) = 0;
    virtual void send(scoped_malloc<net_get_cas_t>& message) = 0;
    virtual void send(stream_pair<net_sarc_t>& message) = 0;
    virtual void send(scoped_malloc<net_incr_t>& message) = 0;
    virtual void send(scoped_malloc<net_decr_t>& message) = 0;
    virtual void send(stream_pair<net_append_t>& message) = 0;
    virtual void send(stream_pair<net_prepend_t>& message) = 0;
    virtual void send(scoped_malloc<net_delete_t>& message) = 0;
    virtual void send(scoped_malloc<net_nop_t>& message) = 0;
    virtual void conn_closed() = 0;
    virtual ~message_callback_t() {}
};

struct tracker_obj_t {
    boost::function<void ()> function;
    char *buf;
    size_t bufsize;

    tracker_obj_t(const boost::function<void ()>& _function, char * _buf, size_t _bufsize)
        : function(_function), buf(_buf), bufsize(_bufsize) { }
};

namespace internal {

struct replication_stream_handler_t : public stream_handler_t {
    replication_stream_handler_t(message_callback_t *receiver) : receiver_(receiver), saw_first_part_(false), tracker_obj_(NULL) { }
    ~replication_stream_handler_t() { delete tracker_obj_; }
    void stream_part(const char *buf, size_t size);
    void end_of_stream();

private:
    message_callback_t *const receiver_;
    bool saw_first_part_;
    tracker_obj_t *tracker_obj_;
};

struct replication_connection_handler_t : public connection_handler_t {
    void process_hello_message(net_hello_t msg);
    stream_handler_t *new_stream_handler() { return new replication_stream_handler_t(receiver_); }
    replication_connection_handler_t(message_callback_t *receiver) : receiver_(receiver) { }
    void conn_closed() { receiver_->conn_closed(); }
private:
    message_callback_t *receiver_;
};



}  // namespace internal

class repli_stream_t : public home_thread_mixin_t {
public:
    repli_stream_t(boost::scoped_ptr<tcp_conn_t>& conn, message_callback_t *recv_callback) : conn_handler_(recv_callback) {
        conn_.swap(conn);
        parse_messages(conn_.get(), &conn_handler_);
        mutex_acquisition_t ak(&outgoing_mutex_);
        send_hello(ak);
    }

    ~repli_stream_t() {
        drain_semaphore_.drain();   // Wait for any active send()s to finish
        rassert(!conn_->is_read_open());
    }

    // Call shutdown() when you want the repli_stream to stop. shutdown() will return
    // immediately but cause the connection to be closed and cause conn_closed() to
    // be called.
    void shutdown() {
        conn_->shutdown_read();
    }

    void send(net_introduce_t *msg);
    void send(net_backfill_t *msg);
    void send(net_backfill_complete_t *msg);
    void send(net_backfill_delete_everything_t msg);
    void send(net_backfill_delete_t *msg);
    void send(net_backfill_set_t *msg, const char *key, unique_ptr_t<data_provider_t> value);
    void send(net_get_cas_t *msg);
    void send(net_sarc_t *msg, const char *key, unique_ptr_t<data_provider_t> value);
    void send(net_incr_t *msg);
    void send(net_decr_t *msg);
    void send(net_append_t *msg, const char *key, unique_ptr_t<data_provider_t> value);
    void send(net_prepend_t *msg, const char *key, unique_ptr_t<data_provider_t> value);
    void send(net_delete_t *msg);
    void send(net_nop_t msg);

    void flush();

private:

    template <class net_struct_type>
    void sendobj(uint8_t msgcode, net_struct_type *msg);

    template <class net_struct_type>
    void sendobj(uint8_t msgcode, net_struct_type *msg, const char *key, unique_ptr_t<data_provider_t> data);

    void send_hello(const mutex_acquisition_t& proof_of_acquisition);

    void try_write(const void *data, size_t size);

    internal::replication_connection_handler_t conn_handler_;

    /* outgoing_mutex_ is used to prevent messages from being interlaced on the wire */
    mutex_t outgoing_mutex_;

    /* drain_semaphore_ is used to prevent the repli_stream_t from being destroyed while there
    are active calls to send(). */
    drain_semaphore_t drain_semaphore_;

    boost::scoped_ptr<tcp_conn_t> conn_;
};


}  // namespace replication


#endif  // __REPLICATION_PROTOCOL_HPP__
