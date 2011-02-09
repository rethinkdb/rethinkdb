#ifndef __REPLICATION_MASTERSTORE_HPP__
#define __REPLICATION_MASTERSTORE_HPP__

#include "store.hpp"
#include "arch/arch.hpp"
#include "concurrency/mutex.hpp"
#include "containers/thick_list.hpp"
#include "replication/net_structs.hpp"

namespace replication {

// TODO: Consider a good value for this port.
const int REPLICATION_PORT = 11212;

class masterstore_exc_t : public std::runtime_error {
public:
    masterstore_exc_t(const char *what_arg) : std::runtime_error(what_arg) { }
};


class masterstore_t : public home_thread_mixin_t, public linux_tcp_listener_callback_t {
public:
    masterstore_t() : message_contiguity_(), slave_(NULL), sources_(), listener_(REPLICATION_PORT) {
        listener_.set_callback(this);
    }

    bool has_slave() { return slave_ != NULL; }

    void hello();

    void get_cas(store_key_t *key, castime_t castime);

    void sarc(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, castime_t castime, store_t::add_policy_t add_policy, store_t::replace_policy_t replace_policy, cas_t old_cas);

    void incr_decr(store_t::incr_decr_kind_t kind, store_key_t *key, uint64_t amount, castime_t castime);

    void append_prepend(store_t::append_prepend_kind_t kind, store_key_t *key, data_provider_t *data, castime_t castime);

    void delete_key(store_key_t *key, repli_timestamp timestamp);

    // Listener callback functions
    void on_tcp_listener_accept(boost::scoped_ptr<linux_tcp_conn_t>& conn);


private:
    // TODO get rid of this.. someday.
    void cas(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, cas_t unique, castime_t castime);


    // Spawns a coroutine.
    void send_data_with_ident(data_provider_t *data, uint32_t ident);

    template <class net_struct_type>
    void setlike(int msgcode, store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, castime_t castime);

    template <class net_struct_type>
    void incr_decr_like(uint8_t msgcode, store_key_t *key, uint64_t amount, castime_t castime);

    template <class net_struct_type>
    void stereotypical(int msgcode, store_key_t *key, data_provider_t *data, net_struct_type netstruct);


    mutex_t message_contiguity_;
    boost::scoped_ptr<tcp_conn_t> slave_;
    thick_list<data_provider_t *, uint32_t> sources_;

    tcp_listener_t listener_;

    DISABLE_COPYING(masterstore_t);
};





}  // namespace replication

#endif  // __REPLICATION_MASTERSTORE_HPP__
