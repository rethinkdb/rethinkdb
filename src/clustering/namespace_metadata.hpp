#ifndef __CLUSTERING_NAMESPACE_HPP__
#define __CLUSTERING_NAMESPACE_HPP__

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

    typename read_mailbox_t::address_t read_mailbox;
    typename write_mailbox_t::address_t write_mailbox;
};

template<class protocol_t>
class namespace_metadata_t {

public:
    typedef boost::uuid::uuid branch_id_t;

    std::map<branch_id_t, branch_metadata_t> branches;

    std::map<branch_id_t, resource_metadata_t<master_metadata_t> > masters;

    std::map<branch_id_t, mirror_dispatcher_metadata_t<protocol_t> > dispatchers;
};

#endif /* __CLUSTERING_NAMESPACE_HPP__ */
