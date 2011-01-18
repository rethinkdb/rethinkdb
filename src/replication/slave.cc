#include "slave.hpp"

#include <stdint.h>

#include "net_structs.hpp"

using boost::scoped_ptr;

namespace replication {

// The 16 bytes include the \0 at the end.
// 13 is the length of the string.
const char parser_t::STANDARD_HELLO_MAGIC[16] = "13rethinkdbrepl";

// TODO unit test offsets

bool valid_role(uint32_t val) {
    return val == master || val == new_slave || val == slave;
}




parser_t::parser_t(net_conn_t *conn, message_callback_t *receiver)
    : conn_(conn), receiver_(receiver), hello_message_received_(false) {
    // Kick things off...
    conn->read_buffered(this);
}

void parser_t::on_net_conn_close() {
    receiver_->conn_closed();
}

void parser_t::on_net_conn_read_external() {


}

void parser_t::on_net_conn_read_buffered(const char *buffer, size_t size) {
    if (hello_message_received_) {
        handle_nonhello_message(buffer, size);
    } else {
        handle_hello_message(buffer, size);
    }
}

void parser_t::handle_hello_message(const char *buffer, size_t size) {
    if (size >= sizeof(net_hello_t)) {
        const net_hello_t *p = reinterpret_cast<const net_hello_t *>(buffer);

        assert(16 == sizeof(p->hello_magic));
        if (0 != memcmp(p->hello_magic, STANDARD_HELLO_MAGIC, sizeof(p->hello_magic))) {
            report_protocol_error("hello magic");
        } else {
            if (p->replication_protocol_version != ACCEPTED_PROTOCOL_VERSION) {
                report_protocol_error("protocol version");
            } else {
                if (!valid_role(p->role)) {
                    report_protocol_error("bad role");
                } else {
                    assert(32 == sizeof(p->informal_name));
                    scoped_ptr<hello_message_t> msg(new hello_message_t(role_t(p->role), p->database_magic, p->informal_name, p->informal_name + sizeof(p->informal_name)));
                    receiver_->hello(msg);

                    conn_->accept_buffer(sizeof(net_hello_t));
                    ask_for_a_message();
                }
            }
        }
    }
}

void parser_t::ask_for_a_message() {
    conn_->read_buffered(this);
}

template <class struct_type>
bool fits(const char *buffer, size_t size) {
    return size >= sizeof(struct_type);
}

template <>
bool fits<net_small_set_t>(const char *buffer, size_t size) {
    const net_small_set_t *p = reinterpret_cast<const net_small_set_t *>(buffer);

    return sizeof(net_small_set_t) < size && leaf_pair_fits(p->leaf_pair(), size - sizeof(net_small_set_t));
}

template <class message_type>
void parser_t::try_parsing(const char *buffer, size_t size) {
    typedef typename message_type::net_struct_type net_struct_type;
    if (fits<net_struct_type>(buffer, size)) {
        const net_struct_type *p = reinterpret_cast<const net_struct_type *>(buffer);

        scoped_ptr<message_type> msg(new message_type(p));
        receiver_->send(msg);
        conn_->accept_buffer(sizeof(net_struct_type));
        ask_for_a_message();
    }
}

void parser_t::handle_nonhello_message(const char *buffer, size_t size) {
    if (size > sizeof(net_header_t)) {
        const net_header_t *hdr = reinterpret_cast<const net_header_t *>(buffer);

        // TODO: handle non-small messages.

        int multipart_aspect = hdr->message_multipart_aspect;

        if (multipart_aspect == SMALL) {
            switch (hdr->msgcode) {
            case BACKFILL: {
                try_parsing<backfill_message_t>(buffer, size);
            } break;

            case ANNOUNCE: {
                try_parsing<announce_message_t>(buffer, size);
            } break;

            case NOP: {
                try_parsing<nop_message_t>(buffer, size);
            } break;

            case ACK: {
                try_parsing<ack_message_t>(buffer, size);
            } break;

            case SHUTTING_DOWN: {
                try_parsing<shutting_down_message_t>(buffer, size);
            } break;

            case GOODBYE: {
                try_parsing<goodbye_message_t>(buffer, size);
            } break;

            case SET: {
                try_parsing<set_message_t>(buffer, size);
            } break;

            case APPEND: {
                try_parsing<append_message_t>(buffer, size);
            } break;

            case PREPEND: {
                try_parsing<prepend_message_t>(buffer, size);
            } break;

            default: {
                report_protocol_error("invalid message code");
            }
            }
        }
    }
}






}  // namespace replication
