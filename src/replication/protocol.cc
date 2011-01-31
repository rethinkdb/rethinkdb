#include "replication/protocol.hpp"

namespace replication {

// TODO refactor this darned thing.  TODO make it easier to parse and
// document the binary protocol we have here.



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

    scoped_malloc<net_hello_t> msg(reinterpret_cast<char *>(&buf), reinterpret_cast<char *>(&buf + 1));
    receiver->hello(msg);
}

template <class T> struct stream_type { typedef scoped_malloc<T> type; };
template <> struct stream_type<net_set_t> { typedef stream_pair<net_set_t> type; };
template <> struct stream_type<net_prepend_t> { typedef stream_pair<net_prepend_t> type; };
template <> struct stream_type<net_append_t> { typedef stream_pair<net_append_t> type; };

template <class T> int objsize(const T *buf) { return sizeof(T); }
template <> int objsize<net_get_cas_t>(const net_get_cas_t *buf) { return sizeof(net_get_cas_t) + buf->key_size; }
template <> int objsize<net_set_t>(const net_set_t *buf) { return sizeof(net_set_t) + buf->key_size + buf->value_size; }
template <> int objsize<net_append_t>(const net_append_t *buf) { return sizeof(net_append_t) + buf->key_size + buf->value_size; }
template <> int objsize<net_prepend_t>(const net_prepend_t *buf) { return sizeof(net_prepend_t) + buf->key_size + buf->value_size; }

template <class T>
void check_pass(message_callback_t *receiver, const char *realbegin, const char *realend) {
    if (ptrdiff_t(sizeof(T)) >= realend - realbegin && objsize<T>(reinterpret_cast<const T *>(realbegin)) == realend - realbegin) {
        typename stream_type<T>::type buf(realbegin, realend);
        receiver->send(buf);
    } else {
        throw protocol_exc_t("message wrong length for message code");
    }
}

template <class T>
void check_first_size(message_callback_t *receiver, const char *realbegin, const char *realend, uint32_t ident, thick_list<value_stream_t *, uint32_t>& streams) {
    if (ptrdiff_t(sizeof(T)) >= realend - realbegin
        && ptrdiff_t(sizeof(T) + reinterpret_cast<const T *>(realbegin)->key_size) <= realend - realbegin) {

        stream_pair<T> spair(realbegin, realend);
        if (!streams.add(ident, spair.stream)) {
            throw protocol_exc_t("reused live ident code");
        }
        receiver->send(spair);

    } else {
        throw protocol_exc_t("message too short for message code and key size");
    }
}


void message_parser_t::handle_message(message_callback_t *receiver, const char *msgbegin, const char *msgend, thick_list<value_stream_t *, uint32_t>& streams) {
    const net_header_t *hdr = reinterpret_cast<const net_header_t *>(msgbegin);

    if (hdr->message_multipart_aspect == SMALL) {
        const char *realbegin = msgbegin + sizeof(net_header_t);
        const char *realend = msgend;

        switch (hdr->msgcode) {
        case BACKFILL: check_pass<net_backfill_t>(receiver, realbegin, realend); break;
        case ANNOUNCE: check_pass<net_announce_t>(receiver, realbegin, realend); break;
        case NOP: check_pass<net_nop_t>(receiver, realbegin, realend); break;
        case ACK: check_pass<net_ack_t>(receiver, realbegin, realend); break;
        case SHUTTING_DOWN: check_pass<net_shutting_down_t>(receiver, realbegin, realend); break;
        case GOODBYE: check_pass<net_goodbye_t>(receiver, realbegin, realend); break;
        case SET: check_pass<net_set_t>(receiver, realbegin, realend); break;
        case APPEND: check_pass<net_append_t>(receiver, realbegin, realend); break;
        case PREPEND: check_pass<net_prepend_t>(receiver, realbegin, realend); break;
        default: throw protocol_exc_t("invalid message code");
        }
    } else {
        const net_multipart_header_t *multipart_hdr = reinterpret_cast<const net_multipart_header_t *>(msgbegin);
        uint32_t ident = multipart_hdr->ident;
        const char *realbegin = msgbegin + sizeof(multipart_hdr);
        const char *realend = msgend;

        if (hdr->message_multipart_aspect == FIRST) {
            switch (hdr->msgcode) {
            case SET: check_first_size<net_set_t>(receiver, realbegin, realend, ident, streams); break;
            case APPEND: check_first_size<net_append_t>(receiver, realbegin, realend, ident, streams); break;
            case PREPEND: check_first_size<net_prepend_t>(receiver, realbegin, realend, ident, streams); break;
            default: throw protocol_exc_t("invalid message code for multipart message");
            }
        } else if (hdr->message_multipart_aspect == MIDDLE || hdr->message_multipart_aspect == LAST) {
            value_stream_t *stream = streams[ident];

            if (stream == NULL) {
                throw protocol_exc_t("inactive stream identifier");
            }

            write_charslice(stream, const_charslice(realbegin, realend));

            if (hdr->message_multipart_aspect == LAST) {
                streams[ident]->shutdown_write();
                streams.drop(ident);
            }
        } else {
            throw protocol_exc_t("invalid message multipart aspect code");
        }
    }

}

void message_parser_t::do_parse_normal_message(tcp_conn_t *conn, message_callback_t *receiver, thick_list<value_stream_t *, uint32_t>& streams) {

    keep_going = true;
    while (keep_going) {
        const_charslice sl = conn->peek();
        if (sl.end - sl.beg >= ptrdiff_t(sizeof(net_multipart_header_t))) {
            const net_header_t *hdr = reinterpret_cast<const net_header_t *>(sl.beg);

            if (hdr->msgsize < sizeof(net_multipart_header_t)) {
                throw protocol_exc_t("message declares too-small size");
            }

            // Just read the whole message.  It's at most 65535 bytes.
            if (sl.end - sl.beg >= ptrdiff_t(hdr->msgsize)) {
                handle_message(receiver, sl.beg, sl.beg + hdr->msgsize, streams);
                conn->pop(hdr->msgsize);
            }
        }

        conn->read_more_buffered();
    }

    /* we only get out of this loop when we've been shutdown, if the connection
     * closes then we catch an exception and never reach here */
    _cb->on_parser_shutdown();
}


void message_parser_t::do_parse_messages(tcp_conn_t *conn, message_callback_t *receiver) {
    try {
        do_parse_hello_message(conn, receiver);

        thick_list<value_stream_t *, uint32_t> streams;
        do_parse_normal_message(conn, receiver, streams);
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
