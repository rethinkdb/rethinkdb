#ifndef __CLUSTERING_NAMESPACE_HPP__
#define __CLUSTERING_NAMESPACE_HPP__

#include <map>

#include "errors.hpp"
#include <boost/serialization/variant.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/variant.hpp>

#include "clustering/resource.hpp"
#include "clustering/mirror_metadata.hpp"
#include "concurrency/fifo_checker.hpp"
#include "rpc/mailbox/typed.hpp"

typedef boost::uuids::uuid branch_id_t;

template<class protocol_t>
class master_metadata_t {

public:
    typedef async_mailbox_t<void(
        typename protocol_t::read_t,
        order_token_t,
        typename async_mailbox_t<void(boost::variant<
            typename protocol_t::read_response_t,
            std::string
            >)>::address_t
        )> read_mailbox_t;

    typedef async_mailbox_t<void(
        typename protocol_t::write_t,
        order_token_t,
        typename async_mailbox_t<void(boost::variant<
            typename protocol_t::write_response_t,
            std::string
            >)>::address_t
        )> write_mailbox_t;

    master_metadata_t() { }
    master_metadata_t(
            const typename protocol_t::region_t &r,
            const typename read_mailbox_t::address_t &rm,
            const typename write_mailbox_t::address_t &wm) :
        region(r), read_mailbox(rm), write_mailbox(wm) { }

    typename protocol_t::region_t region;
    typename read_mailbox_t::address_t read_mailbox;
    typename write_mailbox_t::address_t write_mailbox;

    RDB_MAKE_ME_SERIALIZABLE_3(region, read_mailbox, write_mailbox);
};

template<class protocol_t>
void semilattice_join(master_metadata_t<protocol_t> *a, const master_metadata_t<protocol_t> &b) {
    rassert(a->region == b.region);
    /* The mailboxes should be equal as well, but we can't test that */
}

template<class protocol_t>
class namespace_metadata_t {

public:
    std::map<branch_id_t, resource_metadata_t<master_metadata_t<protocol_t> > > masters;

    std::map<branch_id_t, mirror_dispatcher_metadata_t<protocol_t> > dispatchers;

    RDB_MAKE_ME_SERIALIZABLE_2(masters, dispatchers);
};

template<class protocol_t>
void semilattice_join(namespace_metadata_t<protocol_t> *a, const namespace_metadata_t<protocol_t> &b) {
    semilattice_join(&a->masters, b.masters);
    semilattice_join(&a->dispatchers, b.dispatchers);
}

#endif /* __CLUSTERING_NAMESPACE_HPP__ */
