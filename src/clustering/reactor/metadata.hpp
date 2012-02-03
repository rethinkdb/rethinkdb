#ifndef __CLUSTERING_REACTOR_METADATA_HPP__
#define __CLUSTERING_REACTOR_METADATA_HPP__

#include "clustering/immediate_consistency/branch/metadata.hpp"

/* `reactor_business_card_t` is the way that each peer tells peers what's
currently happening on this machine. Each `reactor_business_card_t` only applies
to a single namespace. */

template<class protocol_t>
class reactor_business_card_t {
public:
    class primary_soon_t {
    };

    class primary_t {
        broadcaster_business_card_t<protocol_t> broadcaster;
        backfiller_business_card_t<protocol_t> backfiller;
    };

    class secondary_t {
        branch_id_t branch_id;
        backfiller_business_card_t<protocol_t> backfiller;
    };

    class secondary_without_primary_t {
        region_map_t<protocol_t, version_range_t> current_state;
        backfiller_business_card_t<protocol_t> backfiller;
    };

    class listener_t {
    };

    class listener_ready_t {
    };

    class nothing_soon_t {
        region_map_t<protocol_t, version_range_t> current_state;
        backfiller_business_card_t<protocol_t> backfiller;
    };

    class nothing_t {
    };

    typedef boost::variant<
            primary_soon_t, primary_t,
            secondary_t, secondary_without_primary_t,
            listener_t, listener_ready_t,
            nothing_soon_t, nothing_t
            > activity_t;

    std::map<typename protocol_t::region_t, activity_t> activities;
};

#endif /* __CLUSTERING_REACTOR_METADATA_HPP__ */
