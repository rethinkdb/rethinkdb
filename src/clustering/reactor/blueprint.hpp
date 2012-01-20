#ifndef __CLUSTERING_REACTOR_BLUEPRINT_HPP__
#define __CLUSTERING_REACTOR_BLUEPRINT_HPP__

#include "utils.hpp"
#include <boost/optional.hpp>

#include "rpc/connectivity/connectivity.hpp"

template<class protocol_t>
class blueprint_t {
public:
    enum role_t {
        role_nothing,
        role_primary,
        role_secondary
    };

    class shard_t {
    public:
        role_t get_role(peer_id_t) THROWS_NOTHING;

        peer_id_t primary;
        std::set<peer_id_t> secondaries;
    };

    /* Returns the role that this blueprint assigns to the given peer for the
    given shard. If the shard is not a shard of the blueprint, returns
    `boost::optional<role_t>()`. If the given peer is not in the blueprint's
    scope, crashes. */
    boost::optional<role_t> get_role(peer_id_t, typename protocol_t::region_t) THROWS_NOTHING;

    void assert_valid() THROWS_NOTHING;

    std::set<peer_id_t> scope;
    std::map<typename protocol_t::region_t, shard_t> shards;
};

#endif /* __CLUSTERING_REACTOR_BLUEPRINT_HPP__ */
