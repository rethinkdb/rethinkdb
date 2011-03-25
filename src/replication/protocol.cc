#include "replication/protocol.hpp"

namespace replication {

class protocol_exc_t : public std::exception {
public:
    protocol_exc_t(const char *msg) : msg_(msg) { }
    const char *what() throw() { return msg_; }
private:
    const char *msg_;
};

// The 16 bytes conveniently include a \0 at the end.
// 13 is the length of the text.
const char STANDARD_HELLO_MAGIC[16] = "13rethinkdbrepl";

bool valid_role(uint32_t val) {
    return val == role_master || val == role_new_slave || val == role_slave;
}

void do_parse_hello_message(tcp_conn_t *conn, message_callback_t *receiver) {
    net_hello_t buf;
    conn->read(&buf, sizeof(buf));

    rassert(16 == sizeof(STANDARD_HELLO_MAGIC));
    if (0 != memcmp(buf.hello_magic, STANDARD_HELLO_MAGIC, sizeof(STANDARD_HELLO_MAGIC))) {
        throw protocol_exc_t("bad hello magic");  // TODO details
    }

    if (buf.replication_protocol_version != 1) {
        throw protocol_exc_t("bad protocol version");  // TODO details
    }

    if (!valid_role(buf.role)) {
        throw protocol_exc_t("bad protocol role");  // TODO details
    }

    rassert(32 == sizeof(buf.informal_name));

    receiver->hello(buf);
}

template <class T> struct stream_type { typedef buffed_data_t<T> type; };
template <> struct stream_type<net_sarc_t> { typedef stream_pair<net_sarc_t> type; };
template <> struct stream_type<net_append_t> { typedef stream_pair<net_append_t> type; };
template <> struct stream_type<net_prepend_t> { typedef stream_pair<net_prepend_t> type; };
template <> struct stream_type<net_backfill_set_t> { typedef stream_pair<net_backfill_set_t> type; };

template <class T> size_t objsize(UNUSED const T *buf) { return sizeof(T); }
template <> size_t objsize<net_get_cas_t>(const net_get_cas_t *buf) { return sizeof(net_get_cas_t) + buf->key_size; }
template <> size_t objsize<net_sarc_t>(const net_sarc_t *buf) { return sizeof(net_sarc_t) + buf->key_size + buf->value_size; }
template <> size_t objsize<net_append_t>(const net_append_t *buf) { return sizeof(net_append_t) + buf->key_size + buf->value_size; }
template <> size_t objsize<net_prepend_t>(const net_prepend_t *buf) { return sizeof(net_prepend_t) + buf->key_size + buf->value_size; }
template <> size_t objsize<net_backfill_set_t>(const net_backfill_set_t *buf) { return sizeof(net_backfill_set_t) + buf->key_size + buf->value_size; }
template <> size_t objsize<net_backfill_delete_t>(const net_backfill_delete_t *buf) { return sizeof(net_backfill_delete_t) + buf->key_size; }

template <class T>
void check_pass(message_callback_t *receiver, weak_buf_t buffer, size_t realoffset, size_t realsize) {
    if (sizeof(T) <= realsize && objsize<T>(buffer.get<T>(realoffset)) == realsize) {
        typename stream_type<T>::type buf(buffer, realoffset, realsize);
        receiver->send(buf);
    } else {
        debugf("realsize: %zu sizeof(T): %zu objsize: %zu\n", realsize, sizeof(T), objsize<T>(buffer.get<T>(realoffset)));
        throw protocol_exc_t("message wrong length for message code");
    }
}

template <class T>
void check_first_size(message_callback_t *receiver, weak_buf_t& buffer, size_t realbegin, size_t realsize, uint32_t ident, tracker_t& streams) {
    if (sizeof(T) >= realsize
        && sizeof(T) + buffer.get<T>(realbegin)->key_size <= realsize) {

        stream_pair<T> spair(buffer, realbegin, realsize, buffer.get<T>(realbegin)->value_size);
        size_t m = realsize - sizeof(T) - buffer.get<T>(realbegin)->key_size;

        void (message_callback_t::*fn)(typename stream_type<T>::type&) = &message_callback_t::send;

        if (!streams.add(ident, new std::pair<boost::function<void()>, std::pair<char *, size_t> >(boost::bind(fn, receiver, boost::ref(spair)), std::make_pair(spair.stream->peek() + m, buffer.get<T>(realbegin)->value_size - m)))) {
            throw protocol_exc_t("reused live ident code");
        }

    } else {
        throw protocol_exc_t("message too short for message code and key size");
    }
}

size_t message_parser_t::handle_message(message_callback_t *receiver, weak_buf_t buffer, size_t offset, size_t num_read, tracker_t& streams) {
    // Returning 0 means not enough bytes; returning >0 means "I consumed <this many> bytes."

    if (num_read < sizeof(net_multipart_header_t)) {
        return 0;
    }

    const net_header_t *hdr = buffer.get<net_header_t>(offset);
    size_t msgsize = hdr->msgsize;

    if (msgsize < sizeof(net_multipart_header_t)) {
        throw protocol_exc_t("invalid msgsize");
    }

    if (num_read < msgsize) {
        return 0;
    }

    if (hdr->message_multipart_aspect == SMALL) {
        size_t realbegin = offset + sizeof(net_header_t);
        size_t realsize = msgsize - sizeof(net_header_t);

        switch (hdr->msgcode) {
        case BACKFILL: check_pass<net_backfill_t>(receiver, buffer, realbegin, realsize); break;
        case BACKFILL_COMPLETE: check_pass<net_backfill_complete_t>(receiver, buffer, realbegin, realsize); break;
        case ANNOUNCE: check_pass<net_announce_t>(receiver, buffer, realbegin, realsize); break;
        case NOP: check_pass<net_nop_t>(receiver, buffer, realbegin, realsize); break;
        case ACK: check_pass<net_ack_t>(receiver, buffer, realbegin, realsize); break;
        case SHUTTING_DOWN: check_pass<net_shutting_down_t>(receiver, buffer, realbegin, realsize); break;
        case GOODBYE: check_pass<net_goodbye_t>(receiver, buffer, realbegin, realsize); break;
        case GET_CAS: check_pass<net_get_cas_t>(receiver, buffer, realbegin, realsize); break;
        case SARC: check_pass<net_sarc_t>(receiver, buffer, realbegin, realsize); break;
        case INCR: check_pass<net_incr_t>(receiver, buffer, realbegin, realsize); break;
        case DECR: check_pass<net_decr_t>(receiver, buffer, realbegin, realsize); break;
        case APPEND: check_pass<net_append_t>(receiver, buffer, realbegin, realsize); break;
        case PREPEND: check_pass<net_prepend_t>(receiver, buffer, realbegin, realsize); break;
        case DELETE: check_pass<net_delete_t>(receiver, buffer, realbegin, realsize); break;
        case BACKFILL_SET: check_pass<net_backfill_set_t>(receiver, buffer, realbegin, realsize); break;
        case BACKFILL_DELETE: check_pass<net_backfill_delete_t>(receiver, buffer, realbegin, realsize); break;
        default: throw protocol_exc_t("invalid message code");
        }
    } else {
        const net_multipart_header_t *multipart_hdr = buffer.get<net_multipart_header_t>(offset);
        uint32_t ident = multipart_hdr->ident;
        size_t realbegin = offset + sizeof(multipart_hdr);
        size_t realsize = msgsize - sizeof(multipart_hdr);

        if (hdr->message_multipart_aspect == FIRST) {
            switch (hdr->msgcode) {
            case SARC: check_first_size<net_sarc_t>(receiver, buffer, realbegin, realsize, ident, streams); break;
            case APPEND: check_first_size<net_append_t>(receiver, buffer, realbegin, realsize, ident, streams); break;
            case PREPEND: check_first_size<net_prepend_t>(receiver, buffer, realbegin, realsize, ident, streams); break;
            case BACKFILL_SET: check_first_size<net_backfill_set_t>(receiver, buffer, realbegin, realsize, ident, streams); break;
            default: throw protocol_exc_t("invalid message code for multipart message");
            }
        } else if (hdr->message_multipart_aspect == MIDDLE || hdr->message_multipart_aspect == LAST) {
            std::pair<boost::function<void ()>, std::pair<char *, size_t> > *pair = streams[ident];

            if (pair == NULL) {
                throw protocol_exc_t("inactive stream identifier");
            }

            if (realsize > pair->second.second) {
                throw protocol_exc_t("buffer overflows value size");
            }
            memcpy(pair->second.first, buffer.get<char>(realbegin), realsize);
            pair->second.first += realsize;
            pair->second.second -= realsize;

            if (hdr->message_multipart_aspect == LAST) {
                if (pair->second.second != 0) {
                    throw protocol_exc_t("buffer left unfilled at LAST message");
                }
                pair->first();
                delete pair;
                streams.drop(ident);
            }
        } else {
            throw protocol_exc_t("invalid message multipart aspect code");
        }
    }

    return msgsize;
}

void message_parser_t::do_parse_normal_messages(tcp_conn_t *conn, message_callback_t *receiver, tracker_t& streams) {

    // This is slightly inefficient: we do excess copying since
    // handle_message is forced to accept a contiguous message, even
    // the _value_ part of the message (which could very well be
    // discontiguous and we wouldn't really care).  Worst case
    // scenario: we copy everything over the network one extra time.
    const size_t shbuf_size = 0x10000;
    shared_buf_t shared_buf(shbuf_size);
    size_t offset = 0;
    size_t num_read = 0;

    is_live = true;

    struct mark_unlive {
        bool *p;
        ~mark_unlive() { *p = false; }
    } marker;

    marker.p = &is_live;

    while (is_live) {
        // Try handling the message.
        size_t handled = handle_message(receiver, weak_buf_t(shared_buf), offset, num_read, streams);
        if (handled > 0) {
            rassert(handled <= num_read);
            offset += handled;
            num_read -= handled;
        } else {
            if (offset + num_read == shbuf_size) {
                shared_buf_t new_shared_buf(shbuf_size);
                memcpy(new_shared_buf.get(), shared_buf.get() + offset, num_read);
                offset = 0;
                shared_buf.swap(new_shared_buf);
            }

            num_read += conn->read_some(shared_buf.get() + offset + num_read, shbuf_size - (offset + num_read));
        }
    }

    /* we only get out of this loop when we've been shutdown, if the connection
     * closes then we catch an exception and never reach here */
    _cb->on_parser_shutdown();

    // marker destructor sets is_live to false
}


void message_parser_t::do_parse_messages(tcp_conn_t *conn, message_callback_t *receiver) {
    try {
        do_parse_hello_message(conn, receiver);

        tracker_t streams;
        do_parse_normal_messages(conn, receiver, streams);
    } catch (tcp_conn_t::read_closed_exc_t& e) {
        if (shutdown_asked_for) {
            _cb->on_parser_shutdown();
        }
        else {
            receiver->conn_closed();
        }
#ifndef NDEBUG
    } catch (protocol_exc_t& e) {
        debugf("catch 'n throwing protocol_exc_t: %s\n", e.what());
        throw;
#endif
    }
}

void message_parser_t::parse_messages(tcp_conn_t *conn, message_callback_t *receiver) {
    rassert(!is_live);

    coro_t::spawn(boost::bind(&message_parser_t::do_parse_messages, this, conn, receiver));
}

void message_parser_t::shutdown(message_parser_shutdown_callback_t *cb) {
    if (!is_live) {
        cb->on_parser_shutdown();
    } else {
        shutdown_asked_for = true;
        _cb = cb;

        is_live = false;
    }
}


void message_parser_t::co_shutdown() {
    struct : public message_parser_shutdown_callback_t {
        cond_t cond;
        void on_parser_shutdown() {
            cond.pulse();
        }
    } cb;

    shutdown(&cb);
    cb.cond.wait();
}


// REPLI_STREAM_T

template <class net_struct_type>
void repli_stream_t::sendobj(uint8_t msgcode, net_struct_type *msg) {
    size_t obsize = objsize(msg);

    if (obsize + sizeof(net_header_t) <= 0xFFFF) {
        net_header_t hdr;
        hdr.message_multipart_aspect = SMALL;
        hdr.msgcode = msgcode;
        hdr.msgsize = sizeof(net_header_t) + obsize;

        mutex_acquisition_t ak(&outgoing_mutex_);
        conn_->write(&hdr, sizeof(net_header_t));
        conn_->write(msg, obsize);
    } else {
        net_multipart_header_t hdr;
        hdr.message_multipart_aspect = FIRST;
        hdr.msgcode = msgcode;
        hdr.msgsize = 0xFFFF;
        // Right now we send every message contiguously.
        hdr.ident = 1;

        size_t offset = 0xFFFF - sizeof(net_multipart_header_t);

        {
            mutex_acquisition_t ak(&outgoing_mutex_);
            conn_->write(&hdr, sizeof(net_multipart_header_t));
            conn_->write(msg, offset);
        }

        char *buf = reinterpret_cast<char *>(msg);

        while (offset + 0xFFFF < obsize) {
            mutex_acquisition_t ak(&outgoing_mutex_);
            hdr.message_multipart_aspect = MIDDLE;
            conn_->write(&hdr, sizeof(net_multipart_header_t));
            // TODO change protocol so that 0 means 0x10000 mmkay?
            conn_->write(buf + offset, 0xFFFF);
            offset += 0xFFFF;
        }

        {
            rassert(obsize - offset <= 0xFFFF);
            mutex_acquisition_t ak(&outgoing_mutex_);
            hdr.message_multipart_aspect = LAST;
            conn_->write(&hdr, sizeof(net_multipart_header_t));
            conn_->write(buf + offset, obsize - offset);
        }
    }
}

template <class net_struct_type>
void repli_stream_t::sendobj(uint8_t msgcode, net_struct_type *msg, const char *key, data_provider_t *data) {
    rassert(msg->value_size == data->get_size());

    size_t bufsize = objsize(msg);
    scoped_malloc<char> buf(bufsize);
    memcpy(buf.get(), msg, sizeof(net_struct_type));
    memcpy(buf.get() + sizeof(net_struct_type), key, msg->key_size);

    buffer_group_t group;
    group.add_buffer(data->get_size(), buf.get() + sizeof(net_struct_type) + msg->key_size);
    data->get_data_into_buffers(&group);

    sendobj(msgcode, reinterpret_cast<net_struct_type *>(buf.get()));
}

void repli_stream_t::send(net_backfill_t *msg) {
    sendobj(BACKFILL, msg);
}

void repli_stream_t::send(net_backfill_complete_t *msg) {
    sendobj(BACKFILL_COMPLETE, msg);
}

void repli_stream_t::send(net_announce_t *msg) {
    sendobj(ANNOUNCE, msg);
}

void repli_stream_t::send(net_get_cas_t *msg) {
    sendobj(GET_CAS, msg);
}

void repli_stream_t::send(net_sarc_t *msg, const char *key, data_provider_t *value) {
    sendobj(SARC, msg, key, value);
}

void repli_stream_t::send(net_backfill_set_t *msg, const char *key, data_provider_t *value) {
    sendobj(BACKFILL_SET, msg, key, value);
}

void repli_stream_t::send(net_incr_t *msg) {
    sendobj(INCR, msg);
}

void repli_stream_t::send(net_decr_t *msg) {
    sendobj(DECR, msg);
}

void repli_stream_t::send(net_append_t *msg, const char *key, data_provider_t *value) {
    sendobj(APPEND, msg, key, value);
}

void repli_stream_t::send(net_prepend_t *msg, const char *key, data_provider_t *value) {
    sendobj(PREPEND, msg, key, value);
}

void repli_stream_t::send(net_delete_t *msg) {
    sendobj(DELETE, msg);
}

void repli_stream_t::send(net_backfill_delete_t *msg) {
    sendobj(BACKFILL_DELETE, msg);
}

void repli_stream_t::send(net_nop_t msg) {
    sendobj(NOP, &msg);
}

void repli_stream_t::send(net_ack_t msg) {
    sendobj(ACK, &msg);
}

void repli_stream_t::send_hello(UNUSED const mutex_acquisition_t& evidence_of_acquisition) {
    net_hello_t msg;
    rassert(sizeof(msg.hello_magic) == 16);
    // TODO make a #define for this.
    memcpy(msg.hello_magic, "13rethinkdbrepl", 16);
    msg.replication_protocol_version = 1;
    msg.role = role_master;
    // TODO have this use actual database_magic!  Or die!
    msg.database_creation_timestamp = 0;
    rassert(sizeof(msg.informal_name) == 32);
    // TODO possibly have a user configurable name.  Or decide not to.
    char informal_name[32] = "master";
    memcpy(msg.informal_name, informal_name, 32);

    conn_->write(&msg, sizeof(msg));
}


}  // namespace replication
