#ifndef __CLUSTERING_REACTOR_METADATA_HPP__
#define __CLUSTERING_REACTOR_METADATA_HPP__

#include "clustering/immediate_consistency/branch/metadata.hpp"

/* `reactor_business_card_t` is the way that each peer tells peers what's
currently happening on this machine. Each `reactor_business_card_t` only applies
to a single namespace. */

template<class protocol_t>
class reactor_business_card_t {
public:
    /* This peer would like to become a primary but can't for 1 or more of the
     * following reasons:
     *  - the peer is backfilling
     *  - another peer is a primary
     */
    class primary_when_safe_t{
    };

    /* This peer is currently a primary in working order. */
    class primary_t {
    public:
        primary_t(broadcaster_business_card_t<protocol_t> _broadcaster, backfiller_business_card_t<protocol_t> _backfiller)
            : broadcaster(_broadcaster), backfiller(_backfiller)
        { }

        broadcaster_business_card_t<protocol_t> broadcaster;
        backfiller_business_card_t<protocol_t> backfiller;
    };

    /* This peer is currently a secondary in working order. */
    class secondary_up_to_date_t {
    public:
        secondary_up_to_date_t(branch_id_t _branch_id, backfiller_business_card_t<protocol_t> _backfiller)
            : branch_id(_branch_id), backfiller(_backfiller)
        { }

        branch_id_t branch_id;
        backfiller_business_card_t<protocol_t> backfiller;
    };

    /* This peer would like to be a secondary but cannot because it failed to
     * find a primary. It may or may not have ever seen a primary. */
    class secondary_without_primary_t {
    public:
        secondary_without_primary_t(region_map_t<protocol_t, version_range_t> _current_state, backfiller_business_card_t<protocol_t> _backfiller)
            : current_state(_current_state), backfiller(_backfiller)
        { }

        region_map_t<protocol_t, version_range_t> current_state;
        backfiller_business_card_t<protocol_t> backfiller;
    };

    /* This peer is in the process of becoming a secondary, barring failures it
     * will become a secondary when it completes backfilling. */
    class secondary_backfilling_t {
    };

    /* This peer has up to date data and is tracking from the primary
     * (receiving changes as they happen) and can be efficiently switch to a
     * role in which it will serve queries. */
    class listener_up_to_date_t {
    };

    /* This peer is backfilling from the current primary but is not intending
     * to serve queries. When it's finished it will transition to the
     * listener_ready_t state and from there can efficiently be switched to a
     * different role (one in which it actually serves queries). */
    class listener_backfilling_t {
    };


    /* This peer would like to be a listener but can't find a primary to track.
     * It may or may not have seen a primary in the past. */
    class listener_without_primary_t {
    };

    /* This peer would like to erase its data and not do any job for this
     * shard, however it must stay up until every other peer is ready for it to
     * go away (to avoid risk of data loss). */
    class nothing_when_safe_t{
    public:
        nothing_soon_t(region_map_t<protocol_t, version_range_t> _current_state, backfiller_business_card_t<protocol_t> _backfiller)
            : current_state(_current_state), backfiller(_backfiller)
        { }

        region_map_t<protocol_t, version_range_t> current_state;
        backfiller_business_card_t<protocol_t> backfiller;
    };

    /* This peer is in the process of erasing data that it previously held,
     * this is identical to nothing in terms of cluster behavior but is a state
     * the we would like to display in the ui. */
    class nothing_when_done_erasing_t {
    };

    /* This peer has no data for the shard, is not backfilling and is not a
     * primary or a secondary. */
    class nothing_t {
    };

    typedef boost::variant<
            primary_soon_t, primary_t,
            secondary_up_to_date_t, secondary_without_primary_t,
            secondary_backfilling_t,
            listener_backfilling_t, listener_up_to_date_t,
            listener_without_primary_t,
            nothing_soon_t, nothing_t
            > activity_t;

    std::map<typename protocol_t::region_t, activity_t> activities;
};

#endif /* __CLUSTERING_REACTOR_METADATA_HPP__ */
