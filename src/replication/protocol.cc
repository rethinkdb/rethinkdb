#include "replication/protocol.hpp"

using boost::scoped_ptr;

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
    return val == master || val == new_slave || val == slave;
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
    scoped_ptr<hello_message_t> msg(new hello_message_t(role_t(buf.role), buf.database_magic, buf.informal_name, buf.informal_name + sizeof(buf.informal_name)));
    receiver->hello(msg);
}

template <class struct_type>
ssize_t fitsize(const_charslice sl) {
    return sl.end - sl.beg >= ptrdiff_t(sizeof(struct_type)) ? sizeof(struct_type) : -1;
}

template <>
ssize_t fitsize<net_small_set_t>(const_charslice sl) {
    const net_small_set_t *p = reinterpret_cast<const net_small_set_t *>(sl.beg);
    size_t size = sl.end - sl.beg;

    if (sizeof(net_small_set_t) < size && leaf_pair_fits(p->leaf_pair(), size - sizeof(net_small_set_t))) {
        return sizeof(net_small_set_t) + p->leaf_pair()->key.full_size() + p->leaf_pair()->value()->full_size();
    } else {
        return -1;
    }
}

template <>
ssize_t fitsize<net_small_append_prepend_t>(const_charslice sl) {
    const net_small_append_prepend_t *p = reinterpret_cast<const net_small_append_prepend_t *>(sl.beg);
    size_t size = sl.end - sl.beg;

    if (sizeof(net_small_append_prepend_t) <= size && sizeof(net_small_append_prepend_t) + p->size <= size) {
        return sizeof(net_small_append_prepend_t) + p->size;
    } else {
        return -1;
    }
}

template <>
ssize_t fitsize<net_large_set_t>(const_charslice sl) {
    const net_large_set_t *p = reinterpret_cast<const net_large_set_t *>(sl.beg);
    size_t size = sl.end - sl.beg;

    if (net_large_set_t::prefitsize() <= size) {
        size_t fs = p->fitsize();
        if (fs <= size) {
            return fs;
        }
    }
    return -1;
}

template <>
ssize_t fitsize<net_large_append_prepend_t>(const_charslice sl) {
    const net_large_append_prepend_t *p = reinterpret_cast<const net_large_append_prepend_t *>(sl.beg);
    size_t size = sl.end - sl.beg;

    if (net_large_append_prepend_t::prefitsize() <= size) {
        size_t fs = p->fitsize();
        if (fs <= size) {
            return fs;
        }
    }
    return -1;
}

template <class message_type>
ssize_t try_parsing(message_callback_t *receiver, const_charslice sl) {
    typedef typename message_type::net_struct_type net_struct_type;
    ssize_t sz = fitsize<net_struct_type>(sl);
    if (sz != -1) {
        scoped_ptr<message_type> msg(new message_type(reinterpret_cast<const net_struct_type *>(sl.beg)));
        receiver->send(msg);
    }
    return sz;
}

template <class message_type>
bool parse_and_pop(tcp_conn_t *conn, message_callback_t *receiver, const_charslice sl) {
    typedef typename message_type::net_struct_type net_struct_type;
    ssize_t sz = fitsize<net_struct_type>(sl);
    if (sz == -1) {
        return false;
    }
    scoped_ptr<message_type> msg(new message_type(reinterpret_cast<const net_struct_type *>(sl.beg)));
    receiver->send(msg);
    conn->pop(sz);
    return true;
}

void send_conn_and_popslice_to_stream(tcp_conn_t *conn, value_stream_t *stream, const_charslice sl, ssize_t skip, ssize_t packet_size) {
    ssize_t write_end = std::min(packet_size, sl.end - sl.beg);
    write_charslice(stream, const_charslice(sl.beg + skip, sl.beg + write_end));
    conn->pop(write_end);

    ssize_t remainder = packet_size - write_end;

    while (remainder > 0) {
        charslice new_sl = stream->buf_for_filling(remainder);
        ssize_t w_size = std::min(new_sl.end - new_sl.beg, remainder);
        conn->read(new_sl.beg, w_size);
        stream->data_written(w_size);
        remainder -= w_size;
    }
}

template <class message_type>
bool parse_and_stream(tcp_conn_t *conn, message_callback_t *receiver, const_charslice sl, thick_list<value_stream_t *, uint32_t>& streams) {
    typedef typename message_type::first_struct_type first_struct_type;
    ssize_t sz = fitsize<first_struct_type>(sl);
    if (sz == -1) {
        return false;
    }
    const first_struct_type *p = reinterpret_cast<const first_struct_type *>(sl.beg);

    uint32_t stream_identifier = p->op_header.identifier;


    value_stream_t *stream = new value_stream_t();
    if (!streams.add(stream_identifier, stream)) {
        throw protocol_exc_t("bad stream identifier");
    }

    send_conn_and_popslice_to_stream(conn, stream, sl, sz, p->op_header.header.size);

    return true;
}

void message_parser_t::do_parse_normal_message(tcp_conn_t *conn, message_callback_t *receiver, thick_list<value_stream_t *, uint32_t>& streams) {

    keep_going = true;
    while (keep_going) {
        const_charslice sl = conn->peek();
        if (sl.end - sl.beg >= ptrdiff_t(sizeof(net_header_t))) {
            const net_header_t *hdr = reinterpret_cast<const net_header_t *>(sl.beg);

            int multipart_aspect = hdr->message_multipart_aspect;

            if (multipart_aspect == SMALL) {

                switch (hdr->msgcode) {
                case BACKFILL: { if (parse_and_pop<backfill_message_t>(conn, receiver, sl)) return; } break;
                case ANNOUNCE: { if (parse_and_pop<announce_message_t>(conn, receiver, sl)) return; } break;
                case NOP: { if (parse_and_pop<nop_message_t>(conn, receiver, sl)) return; } break;
                case ACK: { if (parse_and_pop<ack_message_t>(conn, receiver, sl)) return; } break;
                case SHUTTING_DOWN: { if (parse_and_pop<shutting_down_message_t>(conn, receiver, sl)) return; } break;
                case GOODBYE: { if (parse_and_pop<goodbye_message_t>(conn, receiver, sl)) return; } break;
                case SET: { if (parse_and_pop<set_message_t>(conn, receiver, sl)) return; } break;
                case APPEND: { if (parse_and_pop<append_message_t>(conn, receiver, sl)) return; } break;
                case PREPEND: { if (parse_and_pop<prepend_message_t>(conn, receiver, sl)) return; } break;
                default: { throw protocol_exc_t("invalid message code"); }
                }

            } else if (multipart_aspect == FIRST) {

                if (sl.end - sl.beg >= ptrdiff_t(sizeof(net_large_operation_first_t))) {
                    const net_large_operation_first_t *firstheader
                        = reinterpret_cast<const net_large_operation_first_t *>(sl.beg);

                    switch (firstheader->header.header.msgcode) {
                    case SET: { if (parse_and_stream<set_message_t>(conn, receiver, sl, streams)) return; } break;
                    case APPEND: { if (parse_and_stream<append_message_t>(conn, receiver, sl, streams)) return; } break;
                    case PREPEND: { if (parse_and_stream<prepend_message_t>(conn, receiver, sl, streams)) return; } break;
                    default: { throw protocol_exc_t("invalid message code on FIRST message"); }
                    }

                }


            } else if (multipart_aspect == MIDDLE) {

                if (sl.end - sl.beg >= ptrdiff_t(sizeof(net_large_operation_middle_t))) {
                    const net_large_operation_middle_t *middle
                        = reinterpret_cast<const net_large_operation_middle_t *>(sl.beg);

                    uint32_t stream_identifier = middle->identifier;
                    ssize_t packet_size = middle->header.size;

                    value_stream_t *stream = streams[stream_identifier];

                    if (stream != NULL) {
                        send_conn_and_popslice_to_stream(conn, stream, sl, sizeof(net_large_operation_middle_t), packet_size);
                    } else {
                        throw protocol_exc_t("negative stream identifier");
                    }

                }


            } else if (multipart_aspect == LAST) {

                // CopyPastaz
                if (sl.end - sl.beg >= ptrdiff_t(sizeof(net_large_operation_last_t))) {
                    const net_large_operation_middle_t *middle
                        = reinterpret_cast<const net_large_operation_middle_t *>(sl.beg);

                    uint32_t stream_identifier = middle->identifier;
                    ssize_t packet_size = middle->header.size;

                    value_stream_t *stream = streams[stream_identifier];
                    if (stream != NULL) {
                        send_conn_and_popslice_to_stream(conn, streams[stream_identifier], sl, sizeof(net_large_operation_middle_t), packet_size);
                    } else {
                        throw protocol_exc_t("negative stream identifier");
                    }

                    streams[stream_identifier]->shutdown_write();
                    streams.drop(stream_identifier);
                }


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
