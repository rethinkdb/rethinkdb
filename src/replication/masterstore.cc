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
        n = sizeof(multipart_headed<net_set_t>) + key->size + data->get_size();

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

        size_t val_remaining __attribute__((__unused__)) = data->get_size();

        // TODO implement.

    }
}





}  // namespace replication

