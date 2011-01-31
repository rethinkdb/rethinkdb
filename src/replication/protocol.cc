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
template <> struct stream_type<net_set_t> { typedef stream_pair<net_set_t> type; };
template <> struct stream_type<net_prepend_t> { typedef stream_pair<net_prepend_t> type; };
template <> struct stream_type<net_append_t> { typedef stream_pair<net_append_t> type; };

template <class T> size_t objsize(const T *buf) { return sizeof(T); }
template <> size_t objsize<net_get_cas_t>(const net_get_cas_t *buf) { return sizeof(net_get_cas_t) + buf->key_size; }
template <> size_t objsize<net_set_t>(const net_set_t *buf) { return sizeof(net_set_t) + buf->key_size + buf->value_size; }
template <> size_t objsize<net_append_t>(const net_append_t *buf) { return sizeof(net_append_t) + buf->key_size + buf->value_size; }
template <> size_t objsize<net_prepend_t>(const net_prepend_t *buf) { return sizeof(net_prepend_t) + buf->key_size + buf->value_size; }

template <class T>
void check_pass(message_callback_t *receiver, shared_buf_t& shared_buf, size_t realoffset, size_t realsize) {
    if (realsize <= sizeof(T) && objsize<T>(shared_buf.get<T>(realoffset)) == realsize) {
        typename stream_type<T>::type buf(shared_buf, realoffset, realsize);
        receiver->send(buf);
    } else {
        throw protocol_exc_t("message wrong length for message code");
    }
}

template <class T>
void check_first_size(message_callback_t *receiver, shared_buf_t& shared_buf, size_t realbegin, size_t realsize, uint32_t ident, thick_list<value_stream_t *, uint32_t>& streams) {
    if (sizeof(T) >= realsize
        && sizeof(T) + shared_buf.get<T>(realbegin)->key_size <= realsize) {

        stream_pair<T> spair(shared_buf, realbegin, realsize);
        if (!streams.add(ident, spair.stream)) {
            throw protocol_exc_t("reused live ident code");
        }
        receiver->send(spair);

    } else {
        throw protocol_exc_t("message too short for message code and key size");
    }
}

size_t message_parser_t::handle_message(message_callback_t *receiver, shared_buf_t& shared_buf, size_t offset, size_t num_read, thick_list<value_stream_t *, uint32_t>& streams) {
    // Returning 0 means not enough bytes; returning >0 means "I consumed <this many> bytes."

    if (num_read < sizeof(net_multipart_header_t)) {
        return 0;
    }

    const net_header_t *hdr = shared_buf.get<net_header_t>(offset);
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
        case BACKFILL: check_pass<net_backfill_t>(receiver, shared_buf, realbegin, realsize); break;
        case ANNOUNCE: check_pass<net_announce_t>(receiver, shared_buf, realbegin, realsize); break;
        case NOP: check_pass<net_nop_t>(receiver, shared_buf, realbegin, realsize); break;
        case ACK: check_pass<net_ack_t>(receiver, shared_buf, realbegin, realsize); break;
        case SHUTTING_DOWN: check_pass<net_shutting_down_t>(receiver, shared_buf, realbegin, realsize); break;
        case GOODBYE: check_pass<net_goodbye_t>(receiver, shared_buf, realbegin, realsize); break;
        case SET: check_pass<net_set_t>(receiver, shared_buf, realbegin, realsize); break;
        case APPEND: check_pass<net_append_t>(receiver, shared_buf, realbegin, realsize); break;
        case PREPEND: check_pass<net_prepend_t>(receiver, shared_buf, realbegin, realsize); break;
        default: throw protocol_exc_t("invalid message code");
        }
    } else {
        const net_multipart_header_t *multipart_hdr = shared_buf.get<net_multipart_header_t>(offset);
        uint32_t ident = multipart_hdr->ident;
        size_t realbegin = offset + sizeof(multipart_hdr);
        size_t realsize = msgsize - sizeof(multipart_hdr);

        if (hdr->message_multipart_aspect == FIRST) {
            switch (hdr->msgcode) {
            case SET: check_first_size<net_set_t>(receiver, shared_buf, realbegin, realsize, ident, streams); break;
            case APPEND: check_first_size<net_append_t>(receiver, shared_buf, realbegin, realsize, ident, streams); break;
            case PREPEND: check_first_size<net_prepend_t>(receiver, shared_buf, realbegin, realsize, ident, streams); break;
            default: throw protocol_exc_t("invalid message code for multipart message");
            }
        } else if (hdr->message_multipart_aspect == MIDDLE || hdr->message_multipart_aspect == LAST) {
            value_stream_t *stream = streams[ident];

            if (stream == NULL) {
                throw protocol_exc_t("inactive stream identifier");
            }

            write_charslice(stream, const_charslice(shared_buf.get<char>(realbegin), shared_buf.get<char>(realbegin + realsize)));

            if (hdr->message_multipart_aspect == LAST) {
                streams[ident]->shutdown_write();
                streams.drop(ident);
            }
        } else {
            throw protocol_exc_t("invalid message multipart aspect code");
        }
    }

    return msgsize;
}

void message_parser_t::do_parse_normal_messages(tcp_conn_t *conn, message_callback_t *receiver, thick_list<value_stream_t *, uint32_t>& streams) {

    // This is slightly inefficient: we do excess copying since
    // handle_message is forced to accept a contiguous message, even
    // the _value_ part of the message (which could very well be
    // discontiguous and we wouldn't really care).  Worst case
    // scenario: we copy everything over the network one extra time.
    const size_t shbuf_size = 0x10000;
    shared_buf_t shared_buf(shbuf_size);
    size_t offset = 0;
    size_t num_read = 0;

    keep_going = true;
    while (keep_going) {
        // Try handling the message.
        size_t handled = handle_message(receiver, shared_buf, offset, num_read, streams);
        if (handled) {
            rassert(handled <= num_read);
            offset += handled;
            break;
        }

        if (offset + num_read == shbuf_size) {
            shared_buf_t new_shared_buf(shbuf_size);
            memcpy(new_shared_buf.get(), shared_buf.get() + offset, num_read);
            offset = 0;
            shared_buf.swap(new_shared_buf);
        }

        num_read += conn->read_some(shared_buf.get() + offset + num_read, shbuf_size - (offset + num_read));
    }

    /* we only get out of this loop when we've been shutdown, if the connection
     * closes then we catch an exception and never reach here */
    _cb->on_parser_shutdown();
}


void message_parser_t::do_parse_messages(tcp_conn_t *conn, message_callback_t *receiver) {
    try {
        do_parse_hello_message(conn, receiver);

        thick_list<value_stream_t *, uint32_t> streams;
        do_parse_normal_messages(conn, receiver, streams);
    } catch (tcp_conn_t::read_closed_exc_t& e) {
        if (!shutdown_asked_for)
            receiver->conn_closed();
    }
}

void message_parser_t::parse_messages(tcp_conn_t *conn, message_callback_t *receiver) {
    coro_t::spawn(&message_parser_t::do_parse_messages, this, conn, receiver);
}

bool message_parser_t::shutdown(message_parser_shutdown_callback_t *cb) {
    if (!keep_going) return true;
    shutdown_asked_for = true;

    _cb = cb;
    keep_going = false;
    return false;
}

}  // namespace replication
