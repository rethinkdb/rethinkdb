#ifndef __CLUSTERING_REACTOR_METADATA_HPP__
#define __CLUSTERING_REACTOR_METADATA_HPP__

#include "clustering/immediate_consistency/branch/metadata.hpp"

/* `reactor_business_card_t` is the way that each peer tells peers what's
currently happening namespace on this machine. Each `reactor_business_card_t`
only applies to a single namespace. */

template<class protocol_t>
class reactor_business_card_t {
public:
    typedef int activity_id_t;

    class primary_t {
    public:
        typename protocol_t::region_t region;

        /* This is `boost::optional()` if we are a primary-in-waiting. */
        boost::optional<broadcaster_business_card_t<protocol_t> > broadcaster;
    };

    class secondary_t {
    public:
        typename protocol_t::region_t region;
        branch_id_t branch;
        backfiller_business_card_t<protocol_t> backfiller;
        bool waiting_to_shut_down;
    };

    class backfiller_t {
    public:
        region_map_t<protocol_t, version_range_t> version;
        backfiller_business_card_t<protocol_t> backfiller;
    };

    class nothing_t {
        typename protocol_t::region_t region;
    };

    std::map<activity_id_t, primary_t> primaries;
    std::map<activity_id_t, secondary_t> secondaries;
    std::map<activity_id_t, backfiller_t> backfillers;
    std::map<activity_id_t, nothing_t> nothings;
};

#endif /* __CLUSTERING_REACTOR_METADATA_HPP__ */
