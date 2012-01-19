#ifndef __CLUSTERING_REACTORS_METADATA_HPP__
#define __CLUSTERING_REACTORS_METADATA_HPP__

#include "clustering/immediate_consistency/branch/metadata.hpp"

/* `reactor_business_card_t` is the way that each peer tells peers what's
currently happening namespace on this machine. Each `reactor_business_card_t`
only applies to a single namespace. */

template<class protocol_t>
class reactor_business_card_t {
public:
    /* `inactive_shard_t` indicates that we have data for that shard, but we're
    not currently offering any services related to it. We are never in this
    state unless the cluster is broken or in the middle of a restructuring. */
    class inactive_shard_t {
    };

    /* `cold_shard_t` indicates that we have data for the shard and are offering
    it for backfill, but we aren't serving queries on it or tracking a live
    broadcaster. Like `inactive_shard_t`, we are never in this state unless the
    cluster is broken or in the middle of a restructuring. */
    class cold_shard_t {
    public:
        region_map_t<protocol_t, version_range_t> version;
        backfiller_business_card_t<protocol_t> backfiller;
    };

    /* `primary_shard_t` indicates that we have a broadcaster, listener, and
    replier for the shard. */
    class primary_shard_t {
    public:
        branch_id_t branch_id;
        broadcaster_business_card_t<protocol_t> broadcaster;
        backfiller_business_card_t<protocol_t> backfiller;
    };

    /* `secondary_shard_t` indicates that we have a listener and replier for the
    shard. */
    class secondary_shard_t {
    public:
        branch_id_t branch_id;
        backfiller_business_card_t<protocol_t> backfiller;
    };

    typedef boost::variant<
            cold_shard_t,
            primary_shard_t,
            secondary_shard_t>
            shard_t;

    /* There is one more possible state, which is indicates by the absence of
    any entry in the `shards` map. This is used when we have no data for the
    region at all. */

    /* The confirmation mailbox is used for messages from other peers that want
    to confirm that their blueprint is compatible with ours. */
    typedef async_mailbox_t<void(
            typename protocol_t::region_t,
            peer_id_t peer,
            typename blueprint_t<protocol_t>::role_t,
            async_mailbox_t<void(bool)>::address_t
            )> confirmation_mailbox_t;

    /* The keys in the `shards` map should never overlap */
    std::map<typename protocol_t::region_t, shard_t> shards;

    typename confirmation_mailbox_t::address_t confirmation_mailbox;
};

#endif /* __CLUSTERING_REACTORS_METADATA_HPP__ */
