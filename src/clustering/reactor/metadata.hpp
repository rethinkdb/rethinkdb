// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CLUSTERING_REACTOR_METADATA_HPP_
#define CLUSTERING_REACTOR_METADATA_HPP_

#include <map>
#include <vector>
#include <utility>

#include "errors.hpp"
#include <boost/optional.hpp>

#include "clustering/immediate_consistency/branch/metadata.hpp"
#include "clustering/immediate_consistency/query/direct_reader_metadata.hpp"
#include "clustering/immediate_consistency/query/master_metadata.hpp"
#include "containers/archive/boost_types.hpp"
#include "http/json/json_adapter.hpp"
#include "rpc/semilattice/joins/macros.hpp"
#include "rpc/serialize_macros.hpp"

/* `reactor_business_card_t` is the way that each peer tells peers what's
currently happening on this server. Each `reactor_business_card_t` only applies
to a single namespace. */

namespace reactor_business_card_details {
/* This peer would like to become a primary but can't for 1 or more of the
 * following reasons:
 *  - the peer is backfilling
 *  - another peer is a primary
 */
class backfill_location_t {
public:
    backfill_location_t() { }
    backfill_location_t(backfill_session_id_t _backfill_session_id, peer_id_t _peer_id, reactor_activity_id_t _activity_id)
        : backfill_session_id(_backfill_session_id), peer_id(_peer_id), activity_id(_activity_id)
    { }

    backfill_session_id_t backfill_session_id;
    peer_id_t peer_id;
    reactor_activity_id_t activity_id;
    RDB_MAKE_ME_EQUALITY_COMPARABLE_3(backfill_location_t,
        backfill_session_id, peer_id, activity_id);
};

RDB_MAKE_SERIALIZABLE_3(backfill_location_t,
                        backfill_session_id, peer_id, activity_id);

class primary_when_safe_t {
public:
    primary_when_safe_t() { }

    explicit primary_when_safe_t(const std::vector<backfill_location_t> &_backfills_waited_on)
        : backfills_waited_on(_backfills_waited_on)
    { }
    std::vector<backfill_location_t> backfills_waited_on;
    RDB_MAKE_ME_EQUALITY_COMPARABLE_1(primary_when_safe_t, backfills_waited_on);
};

RDB_MAKE_SERIALIZABLE_1(primary_when_safe_t, backfills_waited_on);

/* This peer is currently a primary in working order. */
class primary_t {
public:
    explicit primary_t(broadcaster_business_card_t _broadcaster)
        : broadcaster(_broadcaster)
    { }

    primary_t(broadcaster_business_card_t _broadcaster,
            replier_business_card_t _replier,
            master_business_card_t _master,
            direct_reader_business_card_t _direct_reader)
        : broadcaster(_broadcaster), replier(_replier), master(_master), direct_reader(_direct_reader)
    { }

    primary_t() { }

    broadcaster_business_card_t broadcaster;

    /* Backfiller is optional because of an awkward circular dependency we
     * run in to where we have to put the broadcaster in the directory in
     * order to construct a listener however thats the listener that we
     * will put in the directory as the replier. Thus these entries must
     * be put in successively and for a brief period the replier will be
     * unset.
     */
    boost::optional<replier_business_card_t> replier;

    boost::optional<master_business_card_t> master;
    boost::optional<direct_reader_business_card_t> direct_reader;

    RDB_MAKE_ME_EQUALITY_COMPARABLE_4(primary_t,
        broadcaster, replier, master, direct_reader);
};

RDB_MAKE_SERIALIZABLE_4(primary_t,
                        broadcaster, replier, master, direct_reader);

/* This peer is currently a secondary in working order. */
class secondary_up_to_date_t {
public:
    secondary_up_to_date_t(branch_id_t _branch_id,
            replier_business_card_t _replier,
            direct_reader_business_card_t _direct_reader)
        : branch_id(_branch_id), replier(_replier), direct_reader(_direct_reader)
    { }

    secondary_up_to_date_t() { }

    branch_id_t branch_id;
    replier_business_card_t replier;
    direct_reader_business_card_t direct_reader;

    RDB_MAKE_ME_EQUALITY_COMPARABLE_3(secondary_up_to_date_t,
        branch_id, replier, direct_reader);
};

RDB_MAKE_SERIALIZABLE_3(secondary_up_to_date_t, branch_id, replier, direct_reader);

/* This peer would like to be a secondary but cannot because it failed to
 * find a primary. It may or may not have ever seen a primary. */
class secondary_without_primary_t {
public:
    secondary_without_primary_t(region_map_t<version_range_t> _current_state, backfiller_business_card_t _backfiller,
                                direct_reader_business_card_t _direct_reader, branch_history_t _branch_history)
        : current_state(_current_state), backfiller(_backfiller), direct_reader(_direct_reader), branch_history(_branch_history)
    { }

    secondary_without_primary_t() { }

    region_map_t<version_range_t> current_state;
    backfiller_business_card_t backfiller;
    direct_reader_business_card_t direct_reader;
    branch_history_t branch_history;

    RDB_MAKE_ME_EQUALITY_COMPARABLE_4(secondary_without_primary_t,
        current_state, backfiller, direct_reader, branch_history);
};

RDB_MAKE_SERIALIZABLE_4(secondary_without_primary_t,
                        current_state, backfiller, direct_reader, branch_history);

/* This peer is in the process of becoming a secondary, barring failures it
 * will become a secondary when it completes backfilling. */
class secondary_backfilling_t {
public:
    secondary_backfilling_t() { }

    explicit secondary_backfilling_t(backfill_location_t  _backfill)
        : backfill(_backfill)
    { }

    backfill_location_t backfill;
    RDB_MAKE_ME_EQUALITY_COMPARABLE_1(secondary_backfilling_t, backfill);
};

RDB_MAKE_SERIALIZABLE_1(secondary_backfilling_t, backfill);

/* This peer would like to erase its data and not do any job for this
 * shard, however it must stay up until every other peer is ready for it to
 * go away (to avoid risk of data loss). */
class nothing_when_safe_t {
public:
    nothing_when_safe_t(region_map_t<version_range_t> _current_state,
                        backfiller_business_card_t _backfiller,
                        branch_history_t _branch_history)
        : current_state(_current_state), backfiller(_backfiller),
          branch_history(_branch_history)
    { }

    nothing_when_safe_t() { }

    region_map_t<version_range_t> current_state;
    backfiller_business_card_t backfiller;
    branch_history_t branch_history;

    RDB_MAKE_ME_EQUALITY_COMPARABLE_3(nothing_when_safe_t,
        current_state, backfiller, branch_history);
};

RDB_MAKE_SERIALIZABLE_3(nothing_when_safe_t,
                        current_state, backfiller, branch_history);

/* This peer is in the process of erasing data that it previously held,
 * this is identical to nothing in terms of cluster behavior but is a state
 * the we would like to display in the ui. */
class nothing_when_done_erasing_t {
public:
    RDB_MAKE_ME_EQUALITY_COMPARABLE_0(nothing_when_done_erasing_t);
};

RDB_MAKE_SERIALIZABLE_0(nothing_when_done_erasing_t);

/* This peer has no data for the shard, is not backfilling and is not a
 * primary or a secondary. */
class nothing_t {
public:
    RDB_MAKE_ME_EQUALITY_COMPARABLE_0(nothing_t);
};

RDB_MAKE_SERIALIZABLE_0(nothing_t);

} //namespace reactor_business_card_details

struct reactor_activity_entry_t {
    typedef reactor_business_card_details::primary_when_safe_t primary_when_safe_t;
    typedef reactor_business_card_details::primary_t primary_t;
    typedef reactor_business_card_details::secondary_up_to_date_t secondary_up_to_date_t;
    typedef reactor_business_card_details::secondary_without_primary_t secondary_without_primary_t;
    typedef reactor_business_card_details::secondary_backfilling_t secondary_backfilling_t;
    typedef reactor_business_card_details::nothing_when_safe_t nothing_when_safe_t;
    typedef reactor_business_card_details::nothing_t nothing_t;
    typedef reactor_business_card_details::nothing_when_done_erasing_t nothing_when_done_erasing_t;

    typedef boost::variant<
        primary_when_safe_t, primary_t,
        secondary_up_to_date_t, secondary_without_primary_t,
        secondary_backfilling_t,
        nothing_when_safe_t, nothing_t, nothing_when_done_erasing_t
        > activity_t;

    region_t region;
    activity_t activity;

    reactor_activity_entry_t(const region_t &_region, activity_t _activity)
        : region(_region), activity(_activity) { }
    reactor_activity_entry_t() { }

    RDB_MAKE_ME_EQUALITY_COMPARABLE_2(reactor_activity_entry_t, region, activity);
};

RDB_MAKE_SERIALIZABLE_2(reactor_activity_entry_t, region, activity);

class reactor_business_card_t {
public:
    typedef reactor_activity_entry_t activity_entry_t;
    typedef activity_entry_t::primary_when_safe_t primary_when_safe_t;
    typedef activity_entry_t::primary_t primary_t;
    typedef activity_entry_t::secondary_up_to_date_t secondary_up_to_date_t;
    typedef activity_entry_t::secondary_without_primary_t secondary_without_primary_t;
    typedef activity_entry_t::secondary_backfilling_t secondary_backfilling_t;
    typedef activity_entry_t::nothing_when_safe_t nothing_when_safe_t;
    typedef activity_entry_t::nothing_t nothing_t;
    typedef activity_entry_t::nothing_when_done_erasing_t nothing_when_done_erasing_t;
    typedef activity_entry_t::activity_t activity_t;

    typedef std::map<reactor_activity_id_t, activity_entry_t> activity_map_t;
    activity_map_t activities;

    RDB_MAKE_ME_EQUALITY_COMPARABLE_1(reactor_business_card_t, activities);
};

RDB_MAKE_SERIALIZABLE_1(reactor_business_card_t, activities);

#endif /* CLUSTERING_REACTOR_METADATA_HPP_ */
