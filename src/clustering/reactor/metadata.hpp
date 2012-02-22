#ifndef __CLUSTERING_REACTOR_METADATA_HPP__
#define __CLUSTERING_REACTOR_METADATA_HPP__

#include "errors.hpp"
#include <boost/optional.hpp>

#include "clustering/immediate_consistency/branch/metadata.hpp"

/* `reactor_business_card_t` is the way that each peer tells peers what's
currently happening on this machine. Each `reactor_business_card_t` only applies
to a single namespace. */

typedef boost::uuids::uuid reactor_activity_id_t;

namespace reactor_business_card_details {
/* This peer would like to become a primary but can't for 1 or more of the
 * following reasons:
 *  - the peer is backfilling
 *  - another peer is a primary
 */
template <class protocol_t>
class primary_when_safe_t {
    RDB_MAKE_ME_SERIALIZABLE_0();
};

/* This peer is currently a primary in working order. */
template <class protocol_t>
class primary_t {
public:
    explicit primary_t(broadcaster_business_card_t<protocol_t> _broadcaster)
        : broadcaster(_broadcaster)
    { }

    primary_t(broadcaster_business_card_t<protocol_t> _broadcaster, backfiller_business_card_t<protocol_t> _backfiller)
        : broadcaster(_broadcaster), backfiller(_backfiller)
    { }

    primary_t() { }

    broadcaster_business_card_t<protocol_t> broadcaster;

    /* Backfiller is optional because of an awkward circular dependency we
     * run in to where we have to put the broadcaster in the directory in
     * order to construct a listener however thats the listener that we
     * will put in the directory as the backfiller. Thus these entries must
     * be put in successively and for a brief period the backfiller will be
     * unset.
     */
    boost::optional<backfiller_business_card_t<protocol_t> > backfiller;

    RDB_MAKE_ME_SERIALIZABLE_2(broadcaster, backfiller);
};

/* This peer is currently a secondary in working order. */
template <class protocol_t>
class secondary_up_to_date_t {
public:
    secondary_up_to_date_t(branch_id_t _branch_id, backfiller_business_card_t<protocol_t> _backfiller)
        : branch_id(_branch_id), backfiller(_backfiller)
    { }

    secondary_up_to_date_t() { }

    branch_id_t branch_id;
    backfiller_business_card_t<protocol_t> backfiller;

    RDB_MAKE_ME_SERIALIZABLE_2(branch_id, backfiller);
};

/* This peer would like to be a secondary but cannot because it failed to
 * find a primary. It may or may not have ever seen a primary. */
template <class protocol_t>
class secondary_without_primary_t {
public:
    secondary_without_primary_t(region_map_t<protocol_t, version_range_t> _current_state, backfiller_business_card_t<protocol_t> _backfiller)
        : current_state(_current_state), backfiller(_backfiller)
    { }

    secondary_without_primary_t() { }

    region_map_t<protocol_t, version_range_t> current_state;
    backfiller_business_card_t<protocol_t> backfiller;

    RDB_MAKE_ME_SERIALIZABLE_2(current_state, backfiller);
};

/* This peer is in the process of becoming a secondary, barring failures it
 * will become a secondary when it completes backfilling. */
template <class protocol_t>
class secondary_backfilling_t {
    RDB_MAKE_ME_SERIALIZABLE_0();
};

/* This peer would like to erase its data and not do any job for this
 * shard, however it must stay up until every other peer is ready for it to
 * go away (to avoid risk of data loss). */
template <class protocol_t>
class nothing_when_safe_t{
public:
    nothing_when_safe_t(region_map_t<protocol_t, version_range_t> _current_state, backfiller_business_card_t<protocol_t> _backfiller)
        : current_state(_current_state), backfiller(_backfiller)
    { }

    nothing_when_safe_t() { }

    region_map_t<protocol_t, version_range_t> current_state;
    backfiller_business_card_t<protocol_t> backfiller;

    RDB_MAKE_ME_SERIALIZABLE_2(current_state, backfiller);
};

/* This peer is in the process of erasing data that it previously held,
 * this is identical to nothing in terms of cluster behavior but is a state
 * the we would like to display in the ui. */
template <class protocol_t>
class nothing_when_done_erasing_t {
    RDB_MAKE_ME_SERIALIZABLE_0();
};

/* This peer has no data for the shard, is not backfilling and is not a
 * primary or a secondary. */
template <class protocol_t>
class nothing_t {
    RDB_MAKE_ME_SERIALIZABLE_0();
};

template <class protocol_t>
std::ostream &operator<<(std::ostream &stream, const primary_when_safe_t<protocol_t> &) {
    stream << "primary_when_safe_t";
    return stream;
}

template <class protocol_t>
std::ostream &operator<<(std::ostream &stream, const primary_t<protocol_t> &) {
    stream << "primary_t";
    return stream;
}

template <class protocol_t>
std::ostream &operator<<(std::ostream &stream, const secondary_up_to_date_t<protocol_t> &) {
    stream << "secondary_up_to_date_t";
    return stream;
}

template <class protocol_t>
std::ostream &operator<<(std::ostream &stream, const secondary_without_primary_t<protocol_t> &) {
    stream << "secondary_without_primary_t";
    return stream;
}

template <class protocol_t>
std::ostream &operator<<(std::ostream &stream, const secondary_backfilling_t<protocol_t> &) {
    stream << "secondary_backfilling_t";
    return stream;
}

template <class protocol_t>
std::ostream &operator<<(std::ostream &stream, const nothing_when_safe_t<protocol_t> &) {
    stream << "nothing_when_safe_t";
    return stream;
}

template <class protocol_t>
std::ostream &operator<<(std::ostream &stream, const nothing_t<protocol_t> &) {
    stream << "nothing_t";
    return stream;
}

template <class protocol_t>
std::ostream &operator<<(std::ostream &stream, const nothing_when_done_erasing_t<protocol_t> &) {
    stream << "nothing_when_done_erasing_t";
    return stream;
}
} //namespace reactor_business_card_details

template<class protocol_t>
class reactor_business_card_t {
public:
    typedef reactor_business_card_details::primary_when_safe_t<protocol_t> primary_when_safe_t;
    typedef reactor_business_card_details::primary_t<protocol_t> primary_t;
    typedef reactor_business_card_details::secondary_up_to_date_t<protocol_t> secondary_up_to_date_t;
    typedef reactor_business_card_details::secondary_without_primary_t<protocol_t> secondary_without_primary_t;
    typedef reactor_business_card_details::secondary_backfilling_t<protocol_t> secondary_backfilling_t;
    typedef reactor_business_card_details::nothing_when_safe_t<protocol_t> nothing_when_safe_t;
    typedef reactor_business_card_details::nothing_t<protocol_t> nothing_t;
    typedef reactor_business_card_details::nothing_when_done_erasing_t<protocol_t> nothing_when_done_erasing_t;

    typedef boost::variant<
            primary_when_safe_t, primary_t,
            secondary_up_to_date_t, secondary_without_primary_t,
            secondary_backfilling_t,
            nothing_when_safe_t, nothing_t, nothing_when_done_erasing_t
        > activity_t;

    typedef std::map<reactor_activity_id_t, std::pair<typename protocol_t::region_t, activity_t> > activity_map_t;
    activity_map_t activities;

    RDB_MAKE_ME_SERIALIZABLE_1(activities);
};

template <class protocol_t>
std::ostream & operator<<(std::ostream &stream, const reactor_business_card_t<protocol_t> &bcard) {
    stream << "{ ";
    typedef reactor_business_card_t<protocol_t> rb_t;
    for (typename rb_t::activity_map_t::const_iterator it =  bcard.activities.begin();
                                                       it != bcard.activities.end();
                                                       it++) {
        stream << it->first << " -> (" << it->second.first << ", " << it->second.second << ")";
    }
    stream << "}";

    return stream;
}

#endif /* __CLUSTERING_REACTOR_METADATA_HPP__ */
