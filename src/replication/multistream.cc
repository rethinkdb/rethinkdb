#include "replication/multistream.hpp"

#include "errors.hpp"
#include <boost/bind.hpp>

#include "arch/arch.hpp"
#include "containers/scoped_malloc.hpp"
#include "containers/thick_list.hpp"
#include "logger.hpp"
#include "perfmon.hpp"

namespace replication {

perfmon_duration_sampler_t
    pm_replication_slave_reading("replication_slave_reading", secs_to_ticks(1.0)),
    pm_replication_slave_handling("replication_slave_handling", secs_to_ticks(1.0));

const uint32_t MAX_MESSAGE_SIZE = 65535;

typedef thick_list<stream_handler_t *, uint32_t> tracker_t;

void do_parse_hello_message(tcp_conn_t *conn, connection_handler_t *h) {
    net_hello_t buf;
    {
        block_pm_duration set_timer(&pm_replication_slave_reading);
        conn->read(&buf, sizeof(buf));
    }

    h->process_hello_message(buf);
}

size_t handle_message(connection_handler_t *connection_handler, const char *buf, size_t num_read, tracker_t& streams) {
    // Returning 0 means not enough bytes; returning >0 means "I consumed <this many> bytes."

    block_pm_duration timer(&pm_replication_slave_handling);

    if (num_read < sizeof(net_header_t)) {
        return 0;
    }

    const net_header_t *hdr = reinterpret_cast<const net_header_t *>(buf);
    if (hdr->message_multipart_aspect == SMALL) {
        size_t msgsize = hdr->msgsize;
        if (msgsize < sizeof(net_header_t) + 1) {
            throw protocol_exc_t("invalid msgsize");
        }

        if (num_read < msgsize) {
            return 0;
        }

        size_t realbegin = sizeof(net_header_t);
        size_t realsize = msgsize - sizeof(net_header_t);

        boost::scoped_ptr<stream_handler_t> h(connection_handler->new_stream_handler());
        h->stream_part(buf + realbegin, realsize);
        h->end_of_stream();

        return msgsize;
    } else {
        const net_multipart_header_t *multipart_hdr = reinterpret_cast<const net_multipart_header_t *>(buf);
        size_t msgsize = multipart_hdr->msgsize;
        if (msgsize < sizeof(net_multipart_header_t)) {
            throw protocol_exc_t("invalid msgsize");
        }

        if (num_read < msgsize) {
            return 0;
        }

        uint32_t ident = multipart_hdr->ident;

        if (multipart_hdr->message_multipart_aspect == FIRST) {
#ifdef REPLICATION_DEBUG
            debugf("FIRST for ident %u\n", ident);
#endif
            if (!streams.add(ident, connection_handler->new_stream_handler())) {
                throw protocol_exc_t("reused live ident code");
            }

            streams[ident]->stream_part(buf + sizeof(net_multipart_header_t), msgsize - sizeof(net_multipart_header_t));

        } else if (multipart_hdr->message_multipart_aspect == MIDDLE || multipart_hdr->message_multipart_aspect == LAST) {
#ifdef REPLICATION_DEBUG
            debugf("MIDDLE or LAST for ident %u\n", ident);
#endif
            stream_handler_t *h = streams[ident];
            if (h == NULL) {
                throw protocol_exc_t("inactive stream identifier");
            }

            h->stream_part(buf + sizeof(net_multipart_header_t), msgsize - sizeof(net_multipart_header_t));
            if (multipart_hdr->message_multipart_aspect == LAST) {
#ifdef REPLICATION_DEBUG
                debugf("WAS LAST for ident %u\n", ident);
#endif
                h->end_of_stream();
                delete h;
                streams.drop(ident);
            }
        } else {
            throw protocol_exc_t(strprintf("invalid message multipart aspect code, it was %d", multipart_hdr->message_multipart_aspect).c_str());
        }

        return msgsize;
    }
}

void do_parse_normal_messages(tcp_conn_t *conn, connection_handler_t *conn_handler, tracker_t& streams) {

    // This is slightly inefficient: we do excess copying since
    // handle_message is forced to accept a contiguous message, even
    // the _value_ part of the message (which could very well be
    // discontiguous and we wouldn't really care).  Worst case
    // scenario: we copy everything over the network one extra time.
    const size_t shbuf_size = MAX_MESSAGE_SIZE;
    scoped_malloc<char> buffer(shbuf_size);
    size_t offset = 0;
    size_t num_read = 0;

    // We break out of this loop when we get a tcp_conn_t::read_closed_exc_t.
    while (true) {
        // Try handling the message.
        size_t handled = handle_message(conn_handler, buffer.get() + offset, num_read, streams);
        if (handled > 0) {
            rassert(handled <= num_read);
            offset += handled;
            num_read -= handled;
        } else {
            if (offset + num_read == shbuf_size) {
                scoped_malloc<char> new_buffer(shbuf_size);
                memcpy(new_buffer.get(), buffer.get() + offset, num_read);
                offset = 0;
                buffer.swap(new_buffer);
            }

            {
                block_pm_duration set_timer(&pm_replication_slave_reading);
                num_read += conn->read_some(buffer.get() + offset + num_read, shbuf_size - (offset + num_read));
            }
        }
    }
}

class tracker_cleaner_t {
public:
    explicit tracker_cleaner_t(tracker_t *tracker) : tracker_(tracker) { }
    ~tracker_cleaner_t() {
        for (uint32_t i = 0, e = tracker_->end_index(); i < e; ++i) {
            stream_handler_t *h = (*tracker_)[i];
            if (h) {
                delete h;
                tracker_->drop(i);
            }
        }
    }
private:
    tracker_t *tracker_;
};


void do_parse_messages(tcp_conn_t *conn, connection_handler_t *conn_handler) {

    try {
        do_parse_hello_message(conn, conn_handler);

        tracker_t streams;
        tracker_cleaner_t streams_cleaner(&streams);

        do_parse_normal_messages(conn, conn_handler, streams);

    } catch (tcp_conn_t::read_closed_exc_t& e) {
        (void) e;   // clang has problems with UNUSED in catch clause
        if (conn->is_write_open()) {
            conn->shutdown_write();
        }
        logERR("Replication connection was closed.\n");
        debugf("Reading end closed on replication connection.\n");
    } catch (protocol_exc_t& e) {
        if (conn->is_write_open()) {
            conn->shutdown_write();
        }
        if (conn->is_read_open()) {
            conn->shutdown_read();
        }
        logERR("Bad data was sent on the replication connection:  %s\n", e.what());

        debugf("Bad data was sent on the replication connection:  %s\n", e.what());
    }

    conn_handler->conn_closed();
}

void parse_messages(tcp_conn_t *conn, connection_handler_t *conn_handler) {

    coro_t::spawn(boost::bind(&do_parse_messages, conn, conn_handler));
}

}  // namespace replication
