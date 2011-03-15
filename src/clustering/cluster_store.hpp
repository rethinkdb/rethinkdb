#ifndef __CLUSTERING_CLUSTER_STORE_HPP__
#define __CLUSTERING_CLUSTER_STORE_HPP__

#include "store.hpp"
#include "rpc/rpc.hpp"
#include "rpc/serialize/serialize_macros.hpp"
#include "rpc/serialize/others.hpp"
#include "rpc/serialize/variant.hpp"

RDB_MAKE_SERIALIZABLE_1(get_cas_mutation_t, key)
RDB_MAKE_SERIALIZABLE_7(set_mutation_t, key, data, flags, exptime, add_policy, replace_policy, old_cas)
RDB_MAKE_SERIALIZABLE_1(delete_mutation_t, key)
RDB_MAKE_SERIALIZABLE_3(incr_decr_mutation_t, kind, key, amount)
RDB_MAKE_SERIALIZABLE_3(append_prepend_mutation_t, key, kind, data)
RDB_MAKE_SERIALIZABLE_1(mutation_t, mutation)
RDB_MAKE_SERIALIZABLE_1(mutation_result_t, result)

struct set_store_interface_mailbox_t {
private:
    typedef sync_mailbox_t<mutation_result_t(mutation_t)> change_mailbox_t;
    change_mailbox_t change_mailbox;

public:
    set_store_interface_mailbox_t(set_store_interface_t *inner) :
        change_mailbox(boost::bind(&set_store_interface_t::change, inner, _1))
    { }

    struct address_t : public set_store_interface_t {
        address_t(set_store_interface_mailbox_t *mb) :
            change_address(&mb->change_mailbox)
            { }
        address_t() { }
        mutation_result_t change(const mutation_t &mut) {
            try {
                return change_address.call(mut);
            } catch (rpc_peer_killed_exc_t) {}
            mutation_result_t res;
            return res;
        }
        RDB_MAKE_ME_SERIALIZABLE_1(change_address)
    private:
        change_mailbox_t::address_t change_address;
    };
    friend class address_t;
};

struct get_store_mailbox_t {
private: 
    typedef sync_mailbox_t<get_result_t(store_key_t)> get_mailbox_t;
    get_mailbox_t get_mailbox;

public:
    get_store_mailbox_t(get_store_t *inner) :
        get_mailbox(boost::bind(&get_store_t::get, inner, _1))
    { }

    struct address_t : public get_store_t {
        address_t(get_store_mailbox_t *mb) :
            get_address(&mb->get_mailbox)
        { }
        address_t() { }
        get_result_t get(const store_key_t &key) {
            try {
                return get_address.call(key);
            } catch (rpc_peer_killed_exc_t) {}
            get_result_t res;
            return res;
        }
        rget_result_t rget(rget_bound_mode_t left_mode, const store_key_t &left_key,
                rget_bound_mode_t right_mode, const store_key_t &right_key) {
            not_implemented();
            rget_result_t res;
            return res;
        }
        RDB_MAKE_ME_SERIALIZABLE_1(get_address)
    private:
        get_mailbox_t::address_t get_address;
    };
    friend class address_t;
};

struct set_store_mailbox_t {

private:
    typedef sync_mailbox_t<mutation_result_t(mutation_t, castime_t)> change_mailbox_t;
    change_mailbox_t change_mailbox;

public:
    set_store_mailbox_t(set_store_t *inner) :
        change_mailbox(boost::bind(&set_store_t::change, inner, _1, _2))
        { }

    struct address_t : public set_store_t {
        address_t(set_store_mailbox_t *mb) :
            change_address(&mb->change_mailbox)
            { }
        address_t() { }
        mutation_result_t change(const mutation_t &mut, castime_t cs) {
            try {
                return change_address.call(mut, cs);
            } catch (rpc_peer_killed_exc_t) {}
            mutation_result_t res;
            return res;
        }
        RDB_MAKE_ME_SERIALIZABLE_1(change_address)
    private:
        change_mailbox_t::address_t change_address;
    };
    friend class address_t;
};

/* Various things we need to be able to serialize and unserialize */

RDB_MAKE_SERIALIZABLE_1(repli_timestamp, time)
RDB_MAKE_SERIALIZABLE_2(castime_t, proposed_cas, timestamp)

/* If the incr/decr fails, then new_value is meaningless; garbage will be
written to the socket and faithfully reconstructed on the other side. This
isn't a big enough problem to justify not using the RDB_MAKE_SERIALIZABLE
macro. */
RDB_MAKE_SERIALIZABLE_2(incr_decr_result_t, res, new_value)

inline void serialize(cluster_outpipe_t *conn, const get_result_t &res) {
    ::serialize(conn, res.value);
    ::serialize(conn, res.flags);
    ::serialize(conn, res.cas);
    if (res.to_signal_when_done) res.to_signal_when_done->pulse();
}

inline int ser_size(const get_result_t &res) {
    return ::ser_size(res.value) + ::ser_size(res.flags) + ::ser_size(res.cas);
}

inline void unserialize(cluster_inpipe_t *conn, unserialize_extra_storage_t *es, get_result_t *res) {
    ::unserialize(conn, es, &res->value);
    ::unserialize(conn, es, &res->flags);
    ::unserialize(conn, es, &res->cas);
    res->to_signal_when_done = NULL;
}

#endif /* __CLUSTERING_CLUSTER_STORE_HPP__ */
