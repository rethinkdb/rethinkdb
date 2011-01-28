#include "masterstore.hpp"

namespace replication {

void masterstore_t::add_slave(tcp_conn_t *conn) {
    if (slave_ != NULL) {
        throw masterstore_exc_t("We already have a slave.");
    }
}

void masterstore_t::get_cas(store_key_t *key, castime_t castime) {
    


}






}  // namespace replication

