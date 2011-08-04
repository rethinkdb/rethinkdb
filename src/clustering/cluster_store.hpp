#ifndef __CLUSTERING_CLUSTER_STORE_HPP__
#define __CLUSTERING_CLUSTER_STORE_HPP__

#include "memcached/store.hpp"
#include "rpc/rpc.hpp"
#include "rpc/serialize/serialize_macros.hpp"
#include "rpc/serialize/others.hpp"
#include "rpc/serialize/variant.hpp"

RDB_MAKE_SERIALIZABLE_1(get_cas_mutation_t, key)
RDB_MAKE_SERIALIZABLE_7(sarc_mutation_t, key, data, flags, exptime, add_policy, replace_policy, old_cas)
RDB_MAKE_SERIALIZABLE_1(delete_mutation_t, key)
RDB_MAKE_SERIALIZABLE_3(incr_decr_mutation_t, kind, key, amount)
RDB_MAKE_SERIALIZABLE_3(append_prepend_mutation_t, key, kind, data)
RDB_MAKE_SERIALIZABLE_1(mutation_t, mutation)
RDB_MAKE_SERIALIZABLE_1(mutation_result_t, result)

struct set_store_interface_mailbox_t {
private:
    typedef sync_mailbox_t<mutation_result_t(mutation_t, order_token_t)> change_mailbox_t;
    change_mailbox_t change_mailbox;

public:
    set_store_interface_mailbox_t(set_store_interface_t *inner) :
        change_mailbox(boost::bind(&set_store_interface_t::change, inner, _1, _2))
    { }

    struct address_t : public set_store_interface_t {
        address_t(set_store_interface_mailbox_t *mb) :
            change_address(&mb->change_mailbox)
            { }
        address_t() { }
        mutation_result_t change(const mutation_t &mut, order_token_t tok) {
            try {
                return change_address.call(mut, tok);
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
    typedef sync_mailbox_t<get_result_t(store_key_t, order_token_t)> get_mailbox_t;
    get_mailbox_t get_mailbox;

public:
    get_store_mailbox_t(get_store_t *inner) :
        get_mailbox(boost::bind(&get_store_t::get, inner, _1, _2))
    { }

    struct address_t : public get_store_t {
        address_t(get_store_mailbox_t *mb) :
            get_address(&mb->get_mailbox)
        { }
        address_t() { }
        get_result_t get(const store_key_t &key, order_token_t tok) {
            try {
                return get_address.call(key, tok);
            } catch (rpc_peer_killed_exc_t) {}
            get_result_t res;
            return res;
        }
        rget_result_t rget(UNUSED rget_bound_mode_t left_mode, UNUSED const store_key_t &left_key,
                UNUSED rget_bound_mode_t right_mode, UNUSED const store_key_t &right_key, UNUSED order_token_t tok) {
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
    typedef sync_mailbox_t<mutation_result_t(mutation_t, castime_t, order_token_t)> change_mailbox_t;
    change_mailbox_t change_mailbox;

public:
    set_store_mailbox_t(set_store_t *inner) :
        change_mailbox(boost::bind(&set_store_t::change, inner, _1, _2, _3))
        { }

    struct address_t : public set_store_t {
        address_t(set_store_mailbox_t *mb) :
            change_address(&mb->change_mailbox)
            { }
        address_t() { }
        mutation_result_t change(const mutation_t &mut, castime_t cs, order_token_t tok) {
            try {
                return change_address.call(mut, cs, tok);
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

RDB_MAKE_SERIALIZABLE_1(repli_timestamp_t, time)
RDB_MAKE_SERIALIZABLE_2(castime_t, proposed_cas, timestamp)

/* If the incr/decr fails, then new_value is meaningless; garbage will be
written to the socket and faithfully reconstructed on the other side. This
isn't a big enough problem to justify not using the RDB_MAKE_SERIALIZABLE
macro. */
RDB_MAKE_SERIALIZABLE_2(incr_decr_result_t, res, new_value)

RDB_MAKE_SERIALIZABLE_3(get_result_t, value, flags, cas)

#endif /* __CLUSTERING_CLUSTER_STORE_HPP__ */
