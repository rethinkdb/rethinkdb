#ifndef __REPLICATION_PROTOCOL_HPP__
#define __REPLICATION_PROTOCOL_HPP__

#include "errors.hpp"
#include <boost/function.hpp>

#include "arch/arch.hpp"
#include "concurrency/fifo_checker.hpp"
#include "concurrency/drain_semaphore.hpp"
#include "concurrency/mutex.hpp"
#include "containers/scoped_malloc.hpp"
#include "data_provider.hpp"
#include "replication/multistream.hpp"
#include "replication/net_structs.hpp"
#include "replication/heartbeat_manager.hpp"


namespace replication {

template <class T>
struct stream_pair {
    boost::shared_ptr<buffered_data_provider_t> stream;
    T *data;

    /* The `stream_pair()` constructor takes a buffer that contains the beginning of a multipart
    message. It's guaranteed to contain the entire header and the entire key and probably part of
    the value, but it's not guaranteed to contain the entire value. The network logic will later
    fill in the rest of the value. */
    // This uses key_size, which is completely crap.
    stream_pair(const char *beg, const char *end, ssize_t size = 0) : stream(), data(NULL) {
        void *p;
        size_t m = sizeof(T) + reinterpret_cast<const T *>(beg)->key_size;

        const char *cutpoint = beg + m;
        data = reinterpret_cast<T *>(malloc(m));
        memcpy(data, beg, m);

        stream.reset(new buffered_data_provider_t(size == 0 ? end - cutpoint : size, &p));

        memcpy(p, cutpoint, end - cutpoint);
    }

    stream_pair(const stream_pair& other) : data(NULL) {
        operator=(const_cast<stream_pair&>(other));
    }

    stream_pair& operator=(stream_pair& other) {
        if (this != &other) {
            stream = other.stream;
            free(data);
            data = other.data;
            other.data = NULL;
        }
        return *this;
    }

    ~stream_pair() {
        free(data);
    }

    T *operator->() { return data; }
};

class message_callback_t {
public:
    // These could call .swap on their parameter, taking ownership of the pointee.
    virtual void hello(net_hello_t message) = 0;
    virtual void send(scoped_malloc<net_introduce_t>& message) = 0;
    virtual void send(scoped_malloc<net_backfill_t>& message) = 0;
    virtual void send(scoped_malloc<net_backfill_complete_t>& message) = 0;
    virtual void send(scoped_malloc<net_backfill_delete_t>& message) = 0;
    virtual void send(stream_pair<net_backfill_set_t>& message) = 0;
    virtual void send(scoped_malloc<net_get_cas_t>& message) = 0;
    virtual void send(stream_pair<net_sarc_t>& message) = 0;
    virtual void send(scoped_malloc<net_incr_t>& message) = 0;
    virtual void send(scoped_malloc<net_decr_t>& message) = 0;
    virtual void send(stream_pair<net_append_t>& message) = 0;
    virtual void send(stream_pair<net_prepend_t>& message) = 0;
    virtual void send(scoped_malloc<net_delete_t>& message) = 0;
    virtual void send(scoped_malloc<net_timebarrier_t>& message) = 0;
    virtual void send(scoped_malloc<net_heartbeat_t>& message) = 0;
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
    replication_stream_handler_t(message_callback_t *receiver, heartbeat_receiver_t *hb_receiver) :
            receiver_(receiver), hb_receiver_(hb_receiver), saw_first_part_(false), tracker_obj_(NULL) { }
    ~replication_stream_handler_t() { if(tracker_obj_) delete tracker_obj_; }
    void stream_part(const char *buf, size_t size);
    void end_of_stream();

private:
    message_callback_t *const receiver_;
    heartbeat_receiver_t *hb_receiver_;
    bool saw_first_part_;
    tracker_obj_t *tracker_obj_;
};

struct replication_connection_handler_t : public connection_handler_t {
    void process_hello_message(net_hello_t msg);
    stream_handler_t *new_stream_handler() { return new replication_stream_handler_t(receiver_, hb_receiver_); }
    replication_connection_handler_t(message_callback_t *receiver, heartbeat_receiver_t *hb_receiver) :
            receiver_(receiver), hb_receiver_(hb_receiver) { }
    void conn_closed() { receiver_->conn_closed(); }
private:
    message_callback_t *receiver_;
    heartbeat_receiver_t *hb_receiver_;
};



}  // namespace internal

class repli_stream_t : public heartbeat_sender_t, public heartbeat_receiver_t, public home_thread_mixin_t {
public:
    repli_stream_t(boost::scoped_ptr<tcp_conn_t>& conn, message_callback_t *recv_callback, int heartbeat_timeout);
    ~repli_stream_t();

    // Call shutdown() when you want the repli_stream to stop. shutdown() causes
    // the connection to be closed and conn_closed() to be called.
    void shutdown() {
        drain_semaphore_t::lock_t keep_us_alive(&drain_semaphore_);
        unwatch_heartbeat();
        stop_sending_heartbeats();
        try {
            mutex_acquisition_t ak(&outgoing_mutex_); // flush_buffer() would interfere with active writes
            conn_->flush_buffer();
        } catch (tcp_conn_t::write_closed_exc_t &e) {
	    (void)e;
            // Ignore
        }
        if (conn_->is_read_open()) {
            conn_->shutdown_read();
        }
    }

    void send(net_introduce_t *msg);
    void send(net_backfill_t *msg);
    void send(net_backfill_complete_t *msg);
    void send(net_backfill_delete_t *msg);
    void send(net_backfill_set_t *msg, const char *key, boost::shared_ptr<data_provider_t> value);
    void send(net_get_cas_t *msg);
    void send(net_sarc_t *msg, const char *key, boost::shared_ptr<data_provider_t> value);
    void send(net_incr_t *msg);
    void send(net_decr_t *msg);
    void send(net_append_t *msg, const char *key, boost::shared_ptr<data_provider_t> value);
    void send(net_prepend_t *msg, const char *key, boost::shared_ptr<data_provider_t> value);
    void send(net_delete_t *msg);
    void send(net_timebarrier_t msg);
    void send(net_heartbeat_t msg);

    void flush();

protected:
    void send_heartbeat();
    void on_heartbeat_timeout();

private:

    template <class net_struct_type>
    void sendobj(uint8_t msgcode, net_struct_type *msg);

    template <class net_struct_type>
    void sendobj(uint8_t msgcode, net_struct_type *msg, const char *key, boost::shared_ptr<data_provider_t> data);

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
