#ifndef CLUSTERING_IMMEDIATE_CONSISTENCY_QUERY_DIRECT_READER_HPP_
#define CLUSTERING_IMMEDIATE_CONSISTENCY_QUERY_DIRECT_READER_HPP_

#include "clustering/immediate_consistency/branch/multistore.hpp"
#include "clustering/immediate_consistency/query/direct_reader_metadata.hpp"
#include "concurrency/fifo_checker.hpp"

/* For each primary and secondary of each shard, there is a `direct_reader_t`.
The `direct_reader_t` allows the `cluster_namespace_interface_t` to bypass the
`broadcaster_t` and read directly from the B-tree itself. This reduces network
traffic and is possible even when the primary is down, but the data it returns
might be out of date. */

template <class protocol_t>
class direct_reader_t {
public:
    direct_reader_t(
            mailbox_manager_t *mm,
            store_view_t<protocol_t> *svs);

    direct_reader_business_card_t<protocol_t> get_business_card();

private:
    void on_read(
            const typename protocol_t::read_t &,
            const mailbox_addr_t<void(typename protocol_t::read_response_t)> &);
    void perform_read(
            const typename protocol_t::read_t &,
            const mailbox_addr_t<void(typename protocol_t::read_response_t)> &,
            auto_drainer_t::lock_t);

    mailbox_manager_t *mailbox_manager;
    store_view_t<protocol_t> *svs;

    order_source_t order_source;  // TODO: order_token_t::ignore
    auto_drainer_t drainer;

    typename direct_reader_business_card_t<protocol_t>::read_mailbox_t read_mailbox;
};

#endif  // CLUSTERING_IMMEDIATE_CONSISTENCY_QUERY_DIRECT_READER_HPP_
