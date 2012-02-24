#include "replication/protocol.hpp"
#include "replication/debug.hpp"
#include "concurrency/coro_fifo.hpp"

/* If `REPLICATION_DEBUG` is defined, then every time a network message is sent
or received, the contents of the message will be printed to `stderr`. */
#ifndef NDEBUG
#define REPLICATION_DEBUG
#endif

namespace replication {

// The 16 bytes conveniently include a \0 at the end.
// 13 is the length of the text.
const char STANDARD_HELLO_MAGIC[16] = "13rethinkdbrepl";

template <class T> struct stream_type { typedef scoped_malloc<T> type; };
template <> struct stream_type<net_sarc_t> { typedef stream_pair<net_sarc_t> type; };
template <> struct stream_type<net_append_t> { typedef stream_pair<net_append_t> type; };
template <> struct stream_type<net_prepend_t> { typedef stream_pair<net_prepend_t> type; };
template <> struct stream_type<net_backfill_set_t> { typedef stream_pair<net_backfill_set_t> type; };

size_t objsize(UNUSED const net_introduce_t *buf) { return sizeof(net_introduce_t); }
size_t objsize(UNUSED const net_backfill_t *buf) { return sizeof(net_backfill_t); }
size_t objsize(UNUSED const net_backfill_complete_t *buf) { return sizeof(net_backfill_complete_t); }
size_t objsize(UNUSED const net_backfill_delete_everything_t *buf) { return sizeof(net_backfill_delete_everything_t); }
size_t objsize(UNUSED const net_timebarrier_t *buf) { return sizeof(net_timebarrier_t); }
size_t objsize(UNUSED const net_heartbeat_t *buf) { return sizeof(net_heartbeat_t); }
size_t objsize(const net_get_cas_t *buf) { return sizeof(net_get_cas_t) + buf->key_size; }
size_t objsize(const net_incr_t *buf) { return sizeof(net_incr_t) + buf->key_size; }
size_t objsize(const net_decr_t *buf) { return sizeof(net_decr_t) + buf->key_size; }
size_t objsize(const net_delete_t *buf) { return sizeof(net_delete_t) + buf->key_size; }
size_t objsize(const net_sarc_t *buf) { return sizeof(net_sarc_t) + buf->key_size + buf->value_size; }
size_t objsize(const net_append_t *buf) { return sizeof(net_append_t) + buf->key_size + buf->value_size; }
size_t objsize(const net_prepend_t *buf) { return sizeof(net_prepend_t) + buf->key_size + buf->value_size; }
size_t objsize(const net_backfill_set_t *buf) { return sizeof(net_backfill_set_t) + buf->key_size + buf->value_size; }
size_t objsize(const net_backfill_delete_t *buf) { return sizeof(net_backfill_delete_t) + buf->key_size; }



namespace internal {

template <class T>
void check_pass(message_callback_t *receiver, const char *buf, size_t realsize) {
    if (sizeof(T) <= realsize && objsize(reinterpret_cast<const T *>(buf)) == realsize) {
#ifdef REPLICATION_DEBUG
        debugf("recv %s\n", debug_format(reinterpret_cast<const T *>(buf)).c_str());
#endif
        typename stream_type<T>::type b(buf, buf + realsize);
        receiver->send(b);
    } else {
        debugf("realsize: %zu sizeof(T): %zu objsize: %zu\n", realsize, sizeof(T), objsize(reinterpret_cast<const T *>(buf)));
        throw protocol_exc_t("message wrong length for message code");
    }
}

template <class T>
void print_and_pass_message(message_callback_t *receiver, stream_pair<T> &spair) {
#ifdef REPLICATION_DEBUG
    debugf("recv %s\n", debug_format(spair.data.get(), spair.stream->peek()).c_str());
#endif
    receiver->send(spair);
}

template <class T>
tracker_obj_t *check_value_streamer(message_callback_t *receiver, const char *buf, size_t size) {
    if (sizeof(T) <= size
        && sizeof(T) + reinterpret_cast<const T *>(buf)->key_size <= size) {

        stream_pair<T> spair(buf, buf + size, reinterpret_cast<const T *>(buf)->value_size);
        size_t m = size - sizeof(T) - reinterpret_cast<const T *>(buf)->key_size;

        char *p = spair.stream->peek() + m;

        return new tracker_obj_t(
            boost::bind(&print_and_pass_message<T>, receiver, spair),
            p, reinterpret_cast<const T *>(buf)->value_size - m);

    } else {
        throw protocol_exc_t("message too short for message code and key size");
    }
}

void replication_stream_handler_t::stream_part(const char *buf, size_t size) {
    // Suspend watching for the heartbeat while we are handling the message.
    // In case handling it takes very long, we might time out otherwise just
    // due to the fact that we are unable to handle incoming heartbeat messages
    // as we are still busy handling this one.
    heartbeat_receiver_t::pause_watching_heartbeat_t hb_pauser = hb_receiver_->pause_watching_heartbeat();

    // Notify the heartbeat receiver that we got something (which means that the connection is alive)
    // (Note: this might be a no-op while watching the heartbeat is paused, depending
    // on the implementation of the heartbeat receiver. It certainly doesn't hurt though.)
    hb_receiver_->note_heartbeat();
    
    if (!saw_first_part_) {
        uint8_t msgcode = *reinterpret_cast<const uint8_t *>(buf);
        ++buf;
        --size;

        switch (msgcode) {
        case INTRODUCE: check_pass<net_introduce_t>(receiver_, buf, size); break;
        case BACKFILL: check_pass<net_backfill_t>(receiver_, buf, size); break;
        case BACKFILL_COMPLETE: check_pass<net_backfill_complete_t>(receiver_, buf, size); break;
        case BACKFILL_DELETE_EVERYTHING: check_pass<net_backfill_delete_everything_t>(receiver_, buf, size); break;
        case TIMEBARRIER: check_pass<net_timebarrier_t>(receiver_, buf, size); break;
        case HEARTBEAT: check_pass<net_heartbeat_t>(receiver_, buf, size); break;
        case GET_CAS: check_pass<net_get_cas_t>(receiver_, buf, size); break;
        case SARC: tracker_obj_ = check_value_streamer<net_sarc_t>(receiver_, buf, size); break;
        case INCR: check_pass<net_incr_t>(receiver_, buf, size); break;
        case DECR: check_pass<net_decr_t>(receiver_, buf, size); break;
        case APPEND: tracker_obj_ = check_value_streamer<net_append_t>(receiver_, buf, size); break;
        case PREPEND: tracker_obj_ = check_value_streamer<net_prepend_t>(receiver_, buf, size); break;
        case DELETE: check_pass<net_delete_t>(receiver_, buf, size); break;
        case BACKFILL_SET: tracker_obj_ = check_value_streamer<net_backfill_set_t>(receiver_, buf, size); break;
        case BACKFILL_DELETE: check_pass<net_backfill_delete_t>(receiver_, buf, size); break;
        default: throw protocol_exc_t("invalid message code");
        }

        saw_first_part_ = true;
    } else {
        if (tracker_obj_ == NULL) {
            throw protocol_exc_t("multipart message received for an inappropriate message type");
        }
        if (size > tracker_obj_->bufsize) {
            throw protocol_exc_t("buffer overflows value size");
        }
        memcpy(tracker_obj_->buf, buf, size);
        tracker_obj_->buf += size;
        tracker_obj_->bufsize -= size;
    }
}

void replication_stream_handler_t::end_of_stream() {
    rassert(saw_first_part_);
    if (tracker_obj_) {
        // Suspend watching for the heartbeat while we are handling the message.
        // (see above for further description)
        heartbeat_receiver_t::pause_watching_heartbeat_t hb_pauser = hb_receiver_->pause_watching_heartbeat();
        tracker_obj_->function();
        delete tracker_obj_;
        tracker_obj_ = NULL;
    }
}

void replication_connection_handler_t::process_hello_message(net_hello_t buf) {
    rassert(16 == sizeof(STANDARD_HELLO_MAGIC));
    if (0 != memcmp(buf.hello_magic, STANDARD_HELLO_MAGIC, sizeof(STANDARD_HELLO_MAGIC))) {
        throw protocol_exc_t("bad hello magic");
    }

    if (buf.replication_protocol_version != 1) {
        throw protocol_exc_t("bad protocol version");
    }

    receiver_->hello(buf);
}

}  // namespace internal

perfmon_rate_monitor_t pm_replication_network_write_rate("replication_network_write_rate", secs_to_ticks(2.0), false);

repli_stream_t::repli_stream_t(boost::scoped_ptr<tcp_conn_t>& conn, message_callback_t *recv_callback, int heartbeat_timeout) :
            heartbeat_sender_t(REPLICATION_HEARTBEAT_INTERVAL),
            heartbeat_receiver_t(heartbeat_timeout),
            conn_handler_(recv_callback, this) {
    conn_.swap(conn);
    conn_->get_raw_connection().write_perfmon = &pm_replication_network_write_rate;
    parse_messages(conn_.get(), &conn_handler_);
    watch_heartbeat();
    {
        mutex_acquisition_t ak(&outgoing_mutex_);
        send_hello(ak);
    }
    start_sending_heartbeats();
}

repli_stream_t::~repli_stream_t() {
    unwatch_heartbeat();
    stop_sending_heartbeats();
    drain_semaphore_.drain();   // Wait for any active send()s to finish
    rassert(!conn_->is_read_open());

    debugf("Closing repli_stream_t()\n");
}

template <class net_struct_type>
void repli_stream_t::sendobj(uint8_t msgcode, net_struct_type *msg) {

#ifdef REPLICATION_DEBUG
    debugf("send %s\n", debug_format(msg).c_str());
#endif

    size_t obsize = objsize(msg);

    if (obsize + sizeof(net_header_t) + 1 <= MAX_MESSAGE_SIZE) {
        net_header_t hdr;
        hdr.msgsize = sizeof(net_header_t) + 1 + obsize;
        hdr.message_multipart_aspect = SMALL;

        mutex_acquisition_t ak(&outgoing_mutex_, true);

        try_write(&hdr, sizeof(net_header_t));
        rassert(1 == sizeof(msgcode));
        try_write(&msgcode, sizeof(msgcode));
        try_write(msg, obsize);
    } else {
        // Right now we don't really split up messages into
        // submessages, even though the other end of the protocol
        // supports that.
        mutex_acquisition_t ak(&outgoing_mutex_, true);

        net_multipart_header_t hdr;
        hdr.msgsize = MAX_MESSAGE_SIZE;
        hdr.message_multipart_aspect = FIRST;
        // TODO: This is an obvious bug, when we can split up our
        // messages (but right now it is fine because we hold the
        // mutex_acquisition_t the whole time).  Note that the value
        // we use here _must_ be zero, since the set of previously
        // used identifier codes must be a contiguous set whose
        // minimal element must be zero.
        hdr.ident = 0;


        size_t offset = MAX_MESSAGE_SIZE - (sizeof(net_multipart_header_t) + 1);

        {
            // Commented because we have a global mutex for all the messages.
            //            mutex_acquisition_t ak(&outgoing_mutex_, true);
            try_write(&hdr, sizeof(net_multipart_header_t));
            rassert(sizeof(msgcode) == 1);
            try_write(&msgcode, sizeof(msgcode));
            try_write(msg, offset);
        }

        char *buf = reinterpret_cast<char *>(msg);

        const size_t midlast_message_size = MAX_MESSAGE_SIZE - sizeof(net_multipart_header_t);

        while (offset + midlast_message_size < obsize) {
            // Commented because we have a global mutex for all the messages.
            //            mutex_acquisition_t ak(&outgoing_mutex_, true);
            hdr.message_multipart_aspect = MIDDLE;
            try_write(&hdr, sizeof(net_multipart_header_t));
            try_write(buf + offset, midlast_message_size);
            offset += midlast_message_size;
        }

        {
            rassert(obsize - offset <= midlast_message_size);
            // Commented because we have a global mutex for all the messages.
            //            mutex_acquisition_t ak(&outgoing_mutex_, true);
            hdr.message_multipart_aspect = LAST;
            hdr.msgsize = sizeof(net_multipart_header_t) + (obsize - offset);
            try_write(&hdr, sizeof(net_multipart_header_t));
            try_write(buf + offset, obsize - offset);
        }
    }
}

template <class net_struct_type>
void repli_stream_t::sendobj(uint8_t msgcode, net_struct_type *msg, const char *key, unique_ptr_t<data_provider_t> data) {
    rassert(msg->value_size == data->get_size());

    size_t bufsize = objsize(msg);
    scoped_malloc<char> buf(bufsize);
    memcpy(buf.get(), msg, sizeof(net_struct_type));
    memcpy(buf.get() + sizeof(net_struct_type), key, msg->key_size);

    /* We guarantee that if data->get_data_into_buffers() fails, we rethrow the exception without
    sending anything over the wire. */
    buffer_group_t group;
    group.add_buffer(data->get_size(), buf.get() + sizeof(net_struct_type) + msg->key_size);

    // This could theoretically block and that could cause reordering
    // of sets, for real time operations.  The fact that it doesn't
    // block is just a function of whatever data provider which we
    // happen to use.  (It definitely blocks for backfilling
    // operations but we don't care.)
    data->get_data_into_buffers(&group);

    sendobj(msgcode, reinterpret_cast<net_struct_type *>(buf.get()));
}

void repli_stream_t::send(net_introduce_t *msg) {
    drain_semaphore_t::lock_t keep_us_alive(&drain_semaphore_);
    sendobj(INTRODUCE, msg);
    flush();
}

void repli_stream_t::send(net_backfill_t *msg) {
    drain_semaphore_t::lock_t keep_us_alive(&drain_semaphore_);
    sendobj(BACKFILL, msg);
    flush();
}

void repli_stream_t::send(net_backfill_complete_t *msg) {
    drain_semaphore_t::lock_t keep_us_alive(&drain_semaphore_);
    sendobj(BACKFILL_COMPLETE, msg);
    flush();
}

void repli_stream_t::send(net_backfill_delete_everything_t msg) {
    drain_semaphore_t::lock_t keep_us_alive(&drain_semaphore_);
    sendobj(BACKFILL_DELETE_EVERYTHING, &msg);
}

void repli_stream_t::send(net_get_cas_t *msg) {
    drain_semaphore_t::lock_t keep_us_alive(&drain_semaphore_);
    sendobj(GET_CAS, msg);
}

void repli_stream_t::send(net_sarc_t *msg, const char *key, unique_ptr_t<data_provider_t> value) {
    drain_semaphore_t::lock_t keep_us_alive(&drain_semaphore_);
    sendobj(SARC, msg, key, value);
}

void repli_stream_t::send(net_backfill_set_t *msg, const char *key, unique_ptr_t<data_provider_t> value) {
    drain_semaphore_t::lock_t keep_us_alive(&drain_semaphore_);
    sendobj(BACKFILL_SET, msg, key, value);
}

void repli_stream_t::send(net_incr_t *msg) {
    drain_semaphore_t::lock_t keep_us_alive(&drain_semaphore_);
    sendobj(INCR, msg);
}

void repli_stream_t::send(net_decr_t *msg) {
    drain_semaphore_t::lock_t keep_us_alive(&drain_semaphore_);
    sendobj(DECR, msg);
}

void repli_stream_t::send(net_append_t *msg, const char *key, unique_ptr_t<data_provider_t> value) {
    drain_semaphore_t::lock_t keep_us_alive(&drain_semaphore_);
    sendobj(APPEND, msg, key, value);
}

void repli_stream_t::send(net_prepend_t *msg, const char *key, unique_ptr_t<data_provider_t> value) {
    drain_semaphore_t::lock_t keep_us_alive(&drain_semaphore_);
    sendobj(PREPEND, msg, key, value);
}

void repli_stream_t::send(net_delete_t *msg) {
    drain_semaphore_t::lock_t keep_us_alive(&drain_semaphore_);
    sendobj(DELETE, msg);
}

void repli_stream_t::send(net_backfill_delete_t *msg) {
    drain_semaphore_t::lock_t keep_us_alive(&drain_semaphore_);
    sendobj(BACKFILL_DELETE, msg);
}

void repli_stream_t::send(net_timebarrier_t msg) {
    drain_semaphore_t::lock_t keep_us_alive(&drain_semaphore_);
    sendobj(TIMEBARRIER, &msg);
    flush();
}

void repli_stream_t::send(net_heartbeat_t msg) {
    drain_semaphore_t::lock_t keep_us_alive(&drain_semaphore_);
    sendobj(HEARTBEAT, &msg);
    flush();
}

void repli_stream_t::send_hello(UNUSED const mutex_acquisition_t& evidence_of_acquisition) {

    drain_semaphore_t::lock_t keep_us_alive(&drain_semaphore_);

    net_hello_t msg;
    rassert(sizeof(msg.hello_magic) == 16);
    rassert(sizeof(STANDARD_HELLO_MAGIC) == 16);
    memcpy(msg.hello_magic, STANDARD_HELLO_MAGIC, 16);
    msg.replication_protocol_version = 1;

    try_write(&msg, sizeof(msg));
}

perfmon_duration_sampler_t master_write("master_write", secs_to_ticks(1.0));

void repli_stream_t::try_write(const void *data, size_t size) {
    try {
        block_pm_duration set_timer(&master_write);
        conn_->write(data, size);
    } catch (tcp_conn_t::write_closed_exc_t &e) {
	(void)e;
#ifndef REPLICATION_DEBUG
        debugf("try_write failed!\n");
#endif
        debugf("Writing end closed on network connection.\n");

        /* Master died; we happened to be mid-write at the time. A tcp_conn_t::read_closed_exc_t
        will be thrown somewhere and that will cause us to shut down. So we can ignore this
        exception. */
    }
}

void repli_stream_t::flush() {
    try {
        mutex_acquisition_t ak(&outgoing_mutex_, true);
        conn_->flush_write_buffer();
    } catch (tcp_conn_t::write_closed_exc_t &e) {
	(void)e;
        /* Ignore; see `repli_stream_t::try_write()`. */
    }
}

}  // namespace replication
