#include "masterstore.hpp"

#include <boost/scoped_ptr.hpp>

#include "replication/net_structs.hpp"
#include "replication/protocol.hpp"

namespace replication {

void masterstore_t::add_slave(tcp_conn_t *conn) {
    if (slave_ != NULL) {
        throw masterstore_exc_t("We already have a slave.");
    }
    slave_ = conn;
}

void masterstore_t::get_cas(store_key_t *key, castime_t castime) {
    size_t n = sizeof(headed<net_get_cas_t>) + key->size;
    scoped_malloc<headed<net_get_cas_t> > message(n);
    message->hdr.message_multipart_aspect = SMALL;
    message->hdr.msgcode = GET_CAS;
    message->hdr.msgsize = n;
    message->data.proposed_cas = castime.proposed_cas;
    message->data.timestamp = castime.timestamp;
    memcpy(message->data.key, key->contents, key->size);

    slave_->write_buffered(message.get(), n);
}

void masterstore_t::set(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, castime_t castime) {
    size_t n = sizeof(headed<net_set_t>) + key->size + data->get_size();

    if (n < 0xFFFF) {
        scoped_malloc<headed<net_set_t> > message(n);
        message->hdr.message_multipart_aspect = SMALL;
        message->hdr.msgcode = SET;
        message->hdr.msgsize = n;
        message->data.proposed_cas = castime.proposed_cas;
        message->data.timestamp = castime.timestamp;
        message->data.flags = flags;
        message->data.exptime = exptime;
        message->data.key_size = key->size;
        message->data.value_size = data->get_size();
        memcpy(message->data.keyvalue, key->contents, key->size);

        struct buffer_group_t group;
        group.add_buffer(data->get_size(), message->data.keyvalue + key->size);
        data->get_data_into_buffers(&group);

        slave_->write_buffered(message.get(), n);
    } else {
        // For now, we have no way to get the first chunk of all the
        // data, so the first chunk has size zero, and we send all the
        // other chunks contiguously.  Later, data_provider_t might
        // stream.

        n = sizeof(multipart_headed<net_set_t>) + key->size;

        scoped_malloc<multipart_headed<net_set_t> > message(n);
        message->hdr.message_multipart_aspect = FIRST;
        message->hdr.msgcode = SET;
        message->hdr.msgsize = n;
        message->data.proposed_cas = castime.proposed_cas;
        message->data.timestamp = castime.timestamp;
        message->data.flags = flags;
        message->data.exptime = exptime;
        message->data.key_size = key->size;
        message->data.value_size = data->get_size();

        uint32_t ident = sources_.add(data);

        message->hdr.ident = ident;

        slave_->write_buffered(message.get(), n);

        // TODO: make sure we aren't taking liberties with the data_provider_t lifetime.
        coro_t::spawn(boost::bind(&masterstore_t::send_data_with_ident, this, data, ident));
    }
}

void masterstore_t::send_data_with_ident(data_provider_t *data, uint32_t ident) {
    size_t size = data->get_size();

    const const_buffer_group_t *group = data->get_data_as_buffers();

    ssize_t chunksize = 0xFFFF - sizeof(net_multipart_header_t);

    ssize_t n = size;

    net_multipart_header_t multi_header;
    multi_header.msgcode = MSGCODE_NIL;
    multi_header.ident = ident;

    std::vector<const_buffer_group_t::buffer_t>::size_type bufnum = 0;
    ssize_t off = 0;
    while (n > 0) {

        multi_header.message_multipart_aspect = n <= chunksize ? LAST : MIDDLE;
        ssize_t m = std::min(n, chunksize);
        n -= m;
        multi_header.msgsize = sizeof(net_multipart_header_t) + m;

        mutex_acquisition_t lock(&message_contiguity_);

        slave_->write(&multi_header, sizeof(multi_header));
        while (m > 0) {
            rassert(bufnum < group->buffers.size());
            const_buffer_group_t::buffer_t buf = group->buffers[bufnum];
            size_t write_size = std::min(m, buf.size - off);
            slave_->write(reinterpret_cast<const char *>(buf.data) + off, write_size);
            m -= write_size;
            off += write_size;

            if (buf.size == off) {
                bufnum++;
                off = 0;
            }
        }
    }

    // TODO: How do we declaim ownership of the data_provider_t?

    sources_.drop(ident);
}



}  // namespace replication

