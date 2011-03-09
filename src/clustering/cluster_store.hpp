#ifndef __CLUSTERING_CLUSTER_STORE_HPP__
#define __CLUSTERING_CLUSTER_STORE_HPP__

#include "store.hpp"
#include "clustering/rpc.hpp"
#include "clustering/serialize_macros.hpp"

struct set_store_interface_mailbox_t {

public:
friend class master_map_t;
    typedef sync_mailbox_t<get_result_t(store_key_t)> get_cas_mailbox_t;
    typedef sync_mailbox_t<set_result_t(store_key_t, data_provider_t*, mcflags_t, exptime_t,
        add_policy_t, replace_policy_t, cas_t)> sarc_mailbox_t;
    typedef sync_mailbox_t<delete_result_t(store_key_t)> delete_mailbox_t;
    typedef sync_mailbox_t<append_prepend_result_t(append_prepend_kind_t, store_key_t,
        data_provider_t*)> append_prepend_mailbox_t;
    typedef sync_mailbox_t<incr_decr_result_t(incr_decr_kind_t, store_key_t,
        uint64_t)> incr_decr_mailbox_t;

    get_cas_mailbox_t get_cas_mailbox;
    sarc_mailbox_t sarc_mailbox;
    delete_mailbox_t delete_mailbox;
    append_prepend_mailbox_t append_prepend_mailbox;
    incr_decr_mailbox_t incr_decr_mailbox;

public:
    set_store_interface_mailbox_t(set_store_interface_t *inner) :
        get_cas_mailbox(boost::bind(&set_store_interface_t::get_cas, inner, _1)),
        sarc_mailbox(boost::bind(&set_store_interface_t::sarc, inner, _1, _2, _3, _4, _5, _6, _7)),
        delete_mailbox(boost::bind(&set_store_interface_t::delete_key, inner, _1)),
        append_prepend_mailbox(boost::bind(&set_store_interface_t::append_prepend, inner, _1, _2, _3)),
        incr_decr_mailbox(boost::bind(&set_store_interface_t::incr_decr, inner, _1, _2, _3))
        { }

    struct address_t : public set_store_interface_t {
        address_t(set_store_interface_mailbox_t *mb) :
            get_cas_address(&mb->get_cas_mailbox),
            sarc_address(&mb->sarc_mailbox),
            delete_address(&mb->delete_mailbox),
            append_prepend_address(&mb->append_prepend_mailbox),
            incr_decr_address(&mb->incr_decr_mailbox)
            { }
        address_t() { }
        get_result_t get_cas(const store_key_t &key) {
            return get_cas_address.call(key);
        }
        set_result_t sarc(const store_key_t &key, data_provider_t *data, mcflags_t flags, exptime_t exptime, add_policy_t add_policy, replace_policy_t replace_policy, cas_t old_cas) {
            return sarc_address.call(key, data, flags, exptime, add_policy, replace_policy, old_cas);
        }
        delete_result_t delete_key(const store_key_t &key) {
            return delete_address.call(key);
        }
        incr_decr_result_t incr_decr(incr_decr_kind_t kind, const store_key_t &key, uint64_t amount) {
            return incr_decr_address.call(kind, key, amount);
        }
        append_prepend_result_t append_prepend(append_prepend_kind_t kind, const store_key_t &key, data_provider_t *data) {
            return append_prepend_address.call(kind, key, data);
        }
        RDB_MAKE_ME_SERIALIZABLE_5(address_t, get_cas_address, sarc_address, delete_address, append_prepend_address, incr_decr_address)
    private:
        get_cas_mailbox_t::address_t get_cas_address;
        sarc_mailbox_t::address_t sarc_address;
        delete_mailbox_t::address_t delete_address;
        append_prepend_mailbox_t::address_t append_prepend_address;
        incr_decr_mailbox_t::address_t incr_decr_address;
    };
    friend class address_t;
};

struct set_store_mailbox_t {

private:
    typedef sync_mailbox_t<get_result_t(store_key_t, castime_t)> get_cas_mailbox_t;
    typedef sync_mailbox_t<set_result_t(store_key_t, data_provider_t*, mcflags_t, exptime_t,
        castime_t, add_policy_t, replace_policy_t, cas_t)> sarc_mailbox_t;
    typedef sync_mailbox_t<delete_result_t(store_key_t, repli_timestamp)> delete_mailbox_t;
    typedef sync_mailbox_t<append_prepend_result_t(append_prepend_kind_t, store_key_t,
        data_provider_t*, castime_t)> append_prepend_mailbox_t;
    typedef sync_mailbox_t<incr_decr_result_t(incr_decr_kind_t, store_key_t,
        uint64_t, castime_t)> incr_decr_mailbox_t;

    get_cas_mailbox_t get_cas_mailbox;
    sarc_mailbox_t sarc_mailbox;
    delete_mailbox_t delete_mailbox;
    append_prepend_mailbox_t append_prepend_mailbox;
    incr_decr_mailbox_t incr_decr_mailbox;

public:
    set_store_mailbox_t(set_store_t *inner) :
        get_cas_mailbox(boost::bind(&set_store_t::get_cas, inner, _1, _2)),
        sarc_mailbox(boost::bind(&set_store_t::sarc, inner, _1, _2, _3, _4, _5, _6, _7, _8)),
        delete_mailbox(boost::bind(&set_store_t::delete_key, inner, _1, _2)),
        append_prepend_mailbox(boost::bind(&set_store_t::append_prepend, inner, _1, _2, _3, _4)),
        incr_decr_mailbox(boost::bind(&set_store_t::incr_decr, inner, _1, _2, _3, _4))
        { }

    struct address_t : public set_store_t {
        address_t(set_store_mailbox_t *mb) :
            get_cas_address(&mb->get_cas_mailbox),
            sarc_address(&mb->sarc_mailbox),
            delete_address(&mb->delete_mailbox),
            append_prepend_address(&mb->append_prepend_mailbox),
            incr_decr_address(&mb->incr_decr_mailbox)
            { }
        address_t() { }
        get_result_t get_cas(const store_key_t &key, castime_t castime) {
            return get_cas_address.call(key, castime);
        }
        set_result_t sarc(const store_key_t &key, data_provider_t *data, mcflags_t flags, exptime_t exptime, castime_t castime, add_policy_t add_policy, replace_policy_t replace_policy, cas_t old_cas) {
            return sarc_address.call(key, data, flags, exptime, castime, add_policy, replace_policy, old_cas);
        }
        delete_result_t delete_key(const store_key_t &key, repli_timestamp ts) {
            return delete_address.call(key, ts);
        }
        incr_decr_result_t incr_decr(incr_decr_kind_t kind, const store_key_t &key, uint64_t amount, castime_t castime) {
            return incr_decr_address.call(kind, key, amount, castime);
        }
        append_prepend_result_t append_prepend(append_prepend_kind_t kind, const store_key_t &key, data_provider_t *data, castime_t castime) {
            return append_prepend_address.call(kind, key, data, castime);
        }
        RDB_MAKE_ME_SERIALIZABLE_5(address_t, get_cas_address, sarc_address, delete_address, append_prepend_address, incr_decr_address)
    private:
        get_cas_mailbox_t::address_t get_cas_address;
        sarc_mailbox_t::address_t sarc_address;
        delete_mailbox_t::address_t delete_address;
        append_prepend_mailbox_t::address_t append_prepend_address;
        incr_decr_mailbox_t::address_t incr_decr_address;
    };
    friend class address_t;
};

#endif /* __CLUSTERING_CLUSTER_STORE_HPP__ */
