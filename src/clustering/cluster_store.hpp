#ifndef __CLUSTERING_CLUSTER_STORE_HPP__
#define __CLUSTERING_CLUSTER_STORE_HPP__

#include "store.hpp"
#include "clustering/rpc.hpp"
#include "clustering/serialize_macros.hpp"

RDB_MAKE_SERIALIZABLE_1(get_cas_mutation_t, key)

/* Explicitly define serialization instead of using the RDB_MAKE_SERIALIZABLE_*() macro for
set_mutation_t because it contains a data_provider_t*, which means it needs a real format_t
instead of an unserialize(), and the macro can't handle that. This is temporary. */
template<>
struct format_t<set_mutation_t> {
    struct parser_t {
        format_t<data_provider_t *>::parser_t dp_parser;
        set_mutation_t mut;
        parser_t(cluster_inpipe_t *inpipe) : dp_parser(inpipe) {
            mut.data = dp_parser.value();
            ::unserialize(inpipe, &mut.key);
            ::unserialize(inpipe, &mut.flags);
            ::unserialize(inpipe, &mut.exptime);
            ::unserialize(inpipe, &mut.add_policy);
            ::unserialize(inpipe, &mut.replace_policy);
            ::unserialize(inpipe, &mut.old_cas);
        }
        const set_mutation_t &value() {
            return mut;
        }
    };
    static int get_size(const set_mutation_t &mut) {
        return format_t<data_provider_t *>::get_size(mut.data) +
            ::ser_size(mut.key) +
            ::ser_size(mut.flags) +
            ::ser_size(mut.exptime) +
            ::ser_size(mut.add_policy) +
            ::ser_size(mut.replace_policy) +
            ::ser_size(mut.old_cas);
    }
    static void write(cluster_outpipe_t *outpipe, const set_mutation_t &mut) {
        format_t<data_provider_t *>::write(outpipe, mut.data);
        ::serialize(outpipe, mut.key);
        ::serialize(outpipe, mut.flags);
        ::serialize(outpipe, mut.exptime);
        ::serialize(outpipe, mut.add_policy);
        ::serialize(outpipe, mut.replace_policy);
        ::serialize(outpipe, mut.old_cas);
    }
};

RDB_MAKE_SERIALIZABLE_1(delete_mutation_t, key)
RDB_MAKE_SERIALIZABLE_3(incr_decr_mutation_t, kind, key, amount)

template<>
struct format_t<append_prepend_mutation_t> {
    struct parser_t {
        format_t<data_provider_t *>::parser_t dp_parser;
        append_prepend_mutation_t mut;
        parser_t(cluster_inpipe_t *inpipe) : dp_parser(inpipe) {
            mut.data = dp_parser.value();
            ::unserialize(inpipe, &mut.key);
            ::unserialize(inpipe, &mut.kind);
        }
        const append_prepend_mutation_t &value() {
            return mut;
        }
    };
    static int get_size(const append_prepend_mutation_t &mut) {
        return format_t<data_provider_t *>::get_size(mut.data) +
            ::ser_size(mut.kind) +
            ::ser_size(mut.key);
    }
    static void write(cluster_outpipe_t *outpipe, const append_prepend_mutation_t &mut) {
        format_t<data_provider_t *>::write(outpipe, mut.data);
        ::serialize(outpipe, mut.kind);
        ::serialize(outpipe, mut.key);
    }
};

template<>
struct format_t<mutation_t> {
    struct parser_t {
        format_t<mutation_t::mutation_variant_t>::parser_t mut_parser;
        mutation_t mut;
        parser_t(cluster_inpipe_t *inpipe) : mut_parser(inpipe) {
            mut.mutation = mut_parser.value();
        }
        const mutation_t &value() {
            return mut;
        }
    };
    static int get_size(const mutation_t &mut) {
        return format_t<mutation_t::mutation_variant_t>::get_size(mut.mutation);
    }
    static void write(cluster_outpipe_t *outpipe, const mutation_t &mut) {
        format_t<mutation_t::mutation_variant_t>::write(outpipe, mut.mutation);
    }
};

template<>
struct format_t<mutation_result_t> {
    struct parser_t {
        format_t<mutation_result_t::result_variant_t>::parser_t res_parser;
        mutation_result_t res;
        parser_t(cluster_inpipe_t *inpipe) : res_parser(inpipe) {
            res.result = res_parser.value();
        }
        const mutation_result_t &value() {
            return res;
        }
    };
    static int get_size(const mutation_result_t &res) {
        return format_t<mutation_result_t::result_variant_t>::get_size(res.result);
    }
    static void write(cluster_outpipe_t *outpipe, const mutation_result_t &res) {
        format_t<mutation_result_t::result_variant_t>::write(outpipe, res.result);
    }
};

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
            return change_address.call(mut);
        }
        RDB_MAKE_ME_SERIALIZABLE_1(address_t, change_address)
    private:
        change_mailbox_t::address_t change_address;
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
            return change_address.call(mut, cs);
        }
        RDB_MAKE_ME_SERIALIZABLE_1(address_t, change_address)
    private:
        change_mailbox_t::address_t change_address;
    };
    friend class address_t;
};

#endif /* __CLUSTERING_CLUSTER_STORE_HPP__ */
