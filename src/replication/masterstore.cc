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
    message->data.proposed_cas = castime.proposed_cas;
    message->data.timestamp = castime.timestamp;
    memcpy(message->data.key, key->contents, key->size);

    slave_->write_buffered(message.get(), n);
}

void masterstore_t::set(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, castime_t castime) {



}





}  // namespace replication

