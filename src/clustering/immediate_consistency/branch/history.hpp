// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_HISTORY_HPP_
#define CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_HISTORY_HPP_

#include <map>
#include <set>

#include "concurrency/signal.hpp"
#include "containers/uuid.hpp"
#include "protocol_api.hpp"
#include "rpc/serialize_macros.hpp"
#include "timestamps.hpp"
#include "utils.hpp"

/* The type `version_t` uniquely identifies the state of some region of a
RethinkDB table at some point in time. Every read operation that passes through
a `broadcaster_t` will get all its data from the version that the broadcaster is
at at the time that the read arrives. Every write operation that passes through
a `broadcaster_t` will transition the data from one `version_t` to the next.

`version_t` internally consists of two parts: a `branch_id_t` and a timestamp.
For the `version_t::zero()`, which is the version of the initial empty database,
the `branch_id_t` is nil and the timestamp is 0. Any other version than
`version_t::zero()` belongs to a `broadcaster_t`. The `branch_id_t` will be
a new UUID that the `broadcaster_t` generated when it was created, and the
timestamp will be a number that the `broadcaster_t` increments every time a
write operation passes through it. (Warning: The timestamp is usually not zero
for a new `broadcaster_t`.) */

class version_t {
public:
    version_t() { }
    version_t(branch_id_t bid, state_timestamp_t ts) :
        branch(bid), timestamp(ts) { }
    static version_t zero() {
        return version_t(nil_uuid(), state_timestamp_t::zero());
    }

    bool operator==(const version_t &v) const{
        return branch == v.branch && timestamp == v.timestamp;
    }
    bool operator!=(const version_t &v) const {
        return !(*this == v);
    }

    branch_id_t branch;
    state_timestamp_t timestamp;

    RDB_MAKE_ME_SERIALIZABLE_2(branch, timestamp);
};

inline void debug_print(printf_buffer_t *buf, const version_t& v) {
    buf->appendf("v{");
    debug_print(buf, v.branch);
    buf->appendf(", ");
    debug_print(buf, v.timestamp);
    buf->appendf("}");
}

/* `version_range_t` is a pair of `version_t`s. The meta-info, which is stored
in the superblock of the B-tree, records a `version_range_t` for each range of
keys. Each key's value is the value it had at some `version_t` in the recorded
`version_range_t`.

The reason we store `version_range_t` instead of `version_t` is that if a
backfill were interrupted, we wouldn't know which keys were up-to-date and
which were not. All that we would know is that each key's state lay at some
point between the version the B-tree was at before the backfill started and the
version that the backfiller is at. */

class version_range_t {
public:
    version_range_t() { }
    explicit version_range_t(const version_t &v) :
        earliest(v), latest(v) { }
    version_range_t(const version_t &e, const version_t &l) :
        earliest(e), latest(l) { }

    bool is_coherent() const {
        return earliest == latest;
    }
    bool operator==(const version_range_t &v) const {
        return earliest == v.earliest && latest == v.latest;
    }
    bool operator!=(const version_range_t &v) const {
        return !(*this == v);
    }

    version_t earliest, latest;

    RDB_MAKE_ME_SERIALIZABLE_2(earliest, latest);
};

inline void debug_print(printf_buffer_t *buf, const version_range_t& vr) {
    buf->appendf("vr{earliest=");
    debug_print(buf, vr.earliest);
    buf->appendf(", latest=");
    debug_print(buf, vr.latest);
    buf->appendf("}");
}

template <class protocol_t>
region_map_t<protocol_t, version_range_t> to_version_range_map(const region_map_t<protocol_t, binary_blob_t> &blob_map);

/* The state of the database at the time that the `broadcaster_t` was created
and the sequence of writes that pass through a `broadcaster_t` are collectively
referred to as a "branch". When a new broadcaster is created, it records the
meta-info of the store.

Thus, the branches form a directed acyclic graph. The branch created by the
first primary "descends" from `version_t::zero()`. When a primary is created as
a replacement for a dead primary, its branch descends from the branch of the
dead primary. When shards are split or merged, the newly-created branches may
descend from part of a single existing branch or from part of several existing
branches.

When a `broadcaster_t` is created, it publishes a `branch_birth_certificate_t`
that describes its newly-created branch. */

template<class protocol_t>
class branch_birth_certificate_t {
public:
    /* The region that the branch applies to. This is the same as the region of
    the `broadcaster_t` that created the branch. Every write to the branch must
    affect only some (non-proper) subset of this region. */
    typename protocol_t::region_t region;

    /* The timestamp of the first state on the branch. `version_t(branch_id,
    initial_timestamp)` refers to the state that the B-tree was in when it was
    passed to the `broadcaster_t` constructor. `initial_timestamp` is chosen to
    be greater than any timestamp that was already issued on that branch, so
    that backfilling doesn't have to be aware of branches. */
    state_timestamp_t initial_timestamp;

    /* The state of the meta-info of the B-tree when the `broadcaster_t` was
    constructed. `origin.get_region()` will be the same as `region`. */
    region_map_t<protocol_t, version_range_t> origin;

    RDB_MAKE_ME_SERIALIZABLE_3(region, initial_timestamp, origin);
};

/* `branch_history_manager_t` is a repository of the all the branches' birth
certificates. It's basically a map from `branch_id_t`s to
`branch_birth_certificate_t<protocol_t>`s, but with the added responsibility of
persisting the data to disk.

The `branch_history_manager_t` does not automatically synchronize the branch
history across the network. Automatic synchronization would be susceptible to
race conditions: if a `version_t` were sent across the network before the
birth certificate for the corresponding branch were sent, then the receiving end
might try to look up the birth certificate for that branch and not find it.
Instead, it is the responsibility of the `backfiller_t`, `broadcaster_t`, and so
on to send the appropriate branch history information along with any
`branch_id_t` they send across the network. They do this by calling
`export_branch_history()`, then passing the resulting `branch_history_t` object
along with the `branch_id_t`, then calling `import_branch_history()` at the
destination before trying to use the `branch_id_t` for anything. */

template<class protocol_t>
class branch_history_t {
public:
    std::map<branch_id_t, branch_birth_certificate_t<protocol_t> > branches;

    RDB_MAKE_ME_SERIALIZABLE_1(branches);
};

template <class protocol_t>
class branch_history_manager_t : public home_thread_mixin_t {
public:
    virtual ~branch_history_manager_t() { }

    /* Returns information about one specific branch. Crashes if we don't have a
    record for this branch. */
    virtual branch_birth_certificate_t<protocol_t> get_branch(branch_id_t branch) THROWS_NOTHING = 0;

    /* Rrturns which branchs this branch history manager knows about. This can
     * be used with get branch to prevent failures. */
    virtual std::set<branch_id_t> known_branches() THROWS_NOTHING = 0;

    /* Adds a new branch to the database. Blocks until it is safely on disk.
    Blocks to avoid a race condition where we write the branch ID to a B-tree's
    metainfo, crash before flushing the `branch_birth_certificate_t` to disk,
    and then cannot find the `branch_birth_certificate_t` upon restarting. */
    virtual void create_branch(branch_id_t branch_id, const branch_birth_certificate_t<protocol_t> &bc, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) = 0;

    /* Copies records related to the given branch and all its ancestors into
    `out`. The reason this mutates `out` instead of returning a
    `branch_history_t` is so that you can call it several times with different
    branches that share history; they will re-use records that they share. */
    virtual void export_branch_history(branch_id_t branch, branch_history_t<protocol_t> *out) THROWS_NOTHING = 0;

    /* Convenience function that finds all records related to the given version
    map and copies them into `out` */
    void export_branch_history(const region_map_t<protocol_t, version_range_t> &region_map, branch_history_t<protocol_t> *out) THROWS_NOTHING {
        for (typename region_map_t<protocol_t, version_range_t>::const_iterator it = region_map.begin();
                                                                                it != region_map.end();
                                                                                ++it) {
            if (!it->second.latest.branch.is_nil()) {
                export_branch_history(it->second.latest.branch, out);
            }
        }
    }

    /* Stores the given branch history records. Blocks until they are safely on
    disk. Blocking is important because if we record the `branch_id_t` in a
    B-tree's metainfo and then crash, we had better be able to find the
    `branch_birth_certificate_t` when we start back up. */
    virtual void import_branch_history(const branch_history_t<protocol_t> &new_records, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) = 0;
};

/* `version_is_ancestor()` returns `true` if every key in `relevant_region` of
the table passed through `ancestor` version on the way to `descendent` version.
Also returns true if `ancestor` and `descendent` are the same version. */

template <class protocol_t>
bool version_is_ancestor(
        branch_history_manager_t<protocol_t> *branch_history_manager,
        version_t ancestor,
        version_t descendent,
        typename protocol_t::region_t relevant_region);

/* `version_is_divergent()` returns true if neither `v1` nor `v2` is an ancestor
of the other. Because the reactor doesn't allow new `broadcaster_t`s to be
created unless we can access every living machine, this should never happen, but
the ICL still checks for it and correctly handles this case. */

template <class protocol_t>
bool version_is_divergent(
        branch_history_manager_t<protocol_t> *branch_history_manager,
        version_t v1,
        version_t v2,
        const typename protocol_t::region_t &relevant_region);

#endif /* CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_HISTORY_HPP_ */
