// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_GENERIC_RAFT_CORE_HPP_
#define CLUSTERING_GENERIC_RAFT_CORE_HPP_

#include <deque>
#include <set>
#include <map>

#include "errors.hpp"
#include <boost/optional.hpp>
#include <boost/variant.hpp>

#include "concurrency/auto_drainer.hpp"
#include "concurrency/new_mutex.hpp"
#include "concurrency/promise.hpp"
#include "concurrency/signal.hpp"
#include "concurrency/watchable.hpp"
#include "concurrency/watchable_map.hpp"
#include "concurrency/watchdog_timer.hpp"
#include "containers/archive/boost_types.hpp"
#include "containers/archive/stl_types.hpp"
#include "containers/empty_value.hpp"
#include "containers/uuid.hpp"
#include "time.hpp"

/* This file and the corresponding `raft_core.tcc` implement the Raft consensus
algorithm, as described in the paper "In Search of an Understandable Consensus Algorithm
(Extended Version)" (2014) by Diego Ongaro and John Ousterhout. Because of the complexity
and subtlety of the Raft algorithm, we follow the paper closely and refer back to it
regularly. You are advised to have a copy of the paper on hand when reading or modifying
this file. The comments also occasionally refer to Diego Ongaro's dissertation,
"Consensus: Bridging Theory and Practice" (2014), as the dissertation addresses a few
subtle points that the paper does not.

These files only contains the basic Raft algorithm itself; they don't contain any
networking or storage logic. Instead, the algorithm uses abstract interfaces to send and
receive network messages and write data to persistent storage. This both keeps these
files as simple as possible and makes it easy to test the Raft algorithm using mocked-up
network and storage systems.

We support both log compaction and configuration changes. Configuration changes use the
joint-configuration mechanism described in the original paper, rather than the
one-server-at-a-time mechanism described in the dissertation.

This implementation deviates significantly from the Raft paper in that we don't actually
send a continuous stream of heartbeats from the leader to the followers. Instead, we send
a "virtual heartbeat stream"; we send a message when a server becomes leader and another
when it ceases to be leader, and we rely on the underlying networking layer to detect if
the connection has failed.

The classes in this file are templatized on a types called `state_t`, which represents
the state machine that the Raft cluster manages. Operations on the state machine are
represented by a member type `state_t::change_t`. So `state_t::change_t` is the type that
is stored in the Raft log, and `state_t` is stored when taking a snapshot.

`state_t` and `state_t::change_t` must satisfy the following requirements:
  * Both must be default-constructable, destructable, copy- and move-constructable, copy-
    and move-assignable.
  * Both must support the `==` and `!=` operators.
  * `state_t` must have a method `void apply_change(const change_t &)` which applies the
    change to the `state_t`, mutating it in place. */

/* `raft_term_t` and `raft_log_index_t` are typedefs to improve the readability of the
code, by making it clearer what the meaning of a particular number is. */
typedef uint64_t raft_term_t;
typedef uint64_t raft_log_index_t;

/* `raft_start_election_immediately_t` is used as a hint to the `raft_member_t`
constructor, and is used to speed up new raft clusters' first elections.

On new clusters, it should be set to YES on precisely one of the raft members. If we're
joining an existing cluster, it should be set to NO to avoid disposing a running leader. */
enum class raft_start_election_immediately_t {
    NO,
    YES
};
ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(
    raft_start_election_immediately_t,
    int8_t,
    raft_start_election_immediately_t::NO,
    raft_start_election_immediately_t::YES);

/* Every member of the Raft cluster is identified by a `raft_member_id_t`. The Raft paper
uses integers for this purpose, but we use UUIDs because we have no reliable distributed
way of assigning integers. Note that `raft_member_id_t` is not a `server_id_t` or a
`peer_id_t`. If a single server leaves a Raft cluster and then joins again, it will use a
different `raft_member_id_t` the second time. */
class raft_member_id_t {
public:
    raft_member_id_t() : uuid(nil_uuid()) { }
    explicit raft_member_id_t(uuid_u u) : uuid(u) { }
    bool is_nil() const { return uuid.is_nil(); }
    bool operator==(const raft_member_id_t &other) const { return uuid == other.uuid; }
    bool operator!=(const raft_member_id_t &other) const { return uuid != other.uuid; }
    bool operator<(const raft_member_id_t &other) const { return uuid < other.uuid; }
    uuid_u uuid;
};
RDB_DECLARE_SERIALIZABLE(raft_member_id_t);
void debug_print(printf_buffer_t *buf, const raft_member_id_t &member_id);

/* `raft_config_t` describes the set of members that are involved in the Raft cluster. */
class raft_config_t {
public:
    /* Regular members of the Raft cluster go in `voting_members`. `non_voting_members`
    is for members that should receive updates, but that don't count for voting purposes.
    */
    std::set<raft_member_id_t> voting_members, non_voting_members;

    /* Returns a list of all members, voting and non-voting. */
    std::set<raft_member_id_t> get_all_members() const {
        std::set<raft_member_id_t> members;
        members.insert(voting_members.begin(), voting_members.end());
        members.insert(non_voting_members.begin(), non_voting_members.end());
        return members;
    }

    /* Returns `true` if `member` is a voting or non-voting member. */
    bool is_member(const raft_member_id_t &member) const {
        return voting_members.count(member) == 1 ||
            non_voting_members.count(member) == 1;
    }

    /* Returns `true` if `members` constitutes a majority. */
    bool is_quorum(const std::set<raft_member_id_t> &members) const {
        size_t votes = 0;
        for (const raft_member_id_t &m : members) {
            votes += voting_members.count(m);
        }
        return (votes * 2 > voting_members.size());
    }

    /* Returns `true` if the given member can act as a leader. (Mostly this exists for
    consistency with `raft_complex_config_t`.) */
    bool is_valid_leader(const raft_member_id_t &member) const {
        return voting_members.count(member) == 1;
    }

    /* The equality and inequality operators are mostly for debugging */
    bool operator==(const raft_config_t &other) const {
        return voting_members == other.voting_members &&
            non_voting_members == other.non_voting_members;
    }
    bool operator!=(const raft_config_t &other) const {
        return !(*this == other);
    }
};
RDB_DECLARE_SERIALIZABLE(raft_config_t);

/* `raft_complex_config_t` can represent either a `raft_config_t` or a joint consensus of
an old and a new `raft_config_t`. */
class raft_complex_config_t {
public:
    /* For a regular configuration, `config` holds the configuration and `new_config` is
    empty. For a joint consensus configuration, `config` holds the old configuration and
    `new_config` holds the new configuration. */
    raft_config_t config;
    boost::optional<raft_config_t> new_config;

    bool is_joint_consensus() const {
        return static_cast<bool>(new_config);
    }

    std::set<raft_member_id_t> get_all_members() const {
        std::set<raft_member_id_t> members = config.get_all_members();
        if (is_joint_consensus()) {
            /* Raft paper, Section 6: "Log entries are replicated to all servers in both
            configurations." */
            std::set<raft_member_id_t> members2 = new_config->get_all_members();
            members.insert(members2.begin(), members2.end());
        }
        return members;
    }

    bool is_member(const raft_member_id_t &member) const {
        return config.is_member(member) ||
            (is_joint_consensus() && new_config->is_member(member));
    }

    bool is_quorum(const std::set<raft_member_id_t> &members) const {
        /* Raft paper, Section 6: "Agreement (for elections and entry commitment)
        requires separate majorities from both the old and new configurations." */
        if (is_joint_consensus()) {
            return config.is_quorum(members) && new_config->is_quorum(members);
        } else {
            return config.is_quorum(members);
        }
    }

    bool is_valid_leader(const raft_member_id_t &member) const {
        /* Raft paper, Section 6: "Any server from either configuration may serve as
        leader." */
        return config.is_valid_leader(member) ||
            (is_joint_consensus() && new_config->is_valid_leader(member));
    }

    /* The equality and inequality operators are mostly for debugging */
    bool operator==(const raft_complex_config_t &other) const {
        return config == other.config &&
            new_config == other.new_config;
    }
    bool operator!=(const raft_complex_config_t &other) const {
        return !(*this == other);
    }
};
RDB_DECLARE_SERIALIZABLE(raft_complex_config_t);

enum class raft_log_entry_type_t {
    /* A `regular` log entry is one with a `change_t`. So if `type` is `regular`,
    then `change` has a value but `config` is empty. */
    regular,
    /* A `config` log entry has a `raft_complex_config_t`. They are used to change
    the cluster configuration. See Section 6 of the Raft paper. So if `type` is
    `config`, then `config` has a value but `change` is empty. */
    config,
    /* A `noop` log entry does nothing and carries niether a `change_t` nor a
    `raft_complex_config_t`. See Section 8 of the Raft paper. */
    noop
};
ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(raft_log_entry_type_t, int8_t,
    raft_log_entry_type_t::regular, raft_log_entry_type_t::noop);

/* `raft_log_entry_t` describes an entry in the Raft log. */
template<class state_t>
class raft_log_entry_t {
public:
    raft_log_entry_type_t type;
    raft_term_t term;
    /* Whether `change` and `config` are empty or not depends on the value of `type`. */
    boost::optional<typename state_t::change_t> change;
    boost::optional<raft_complex_config_t> config;

    /* The equality and inequality operators are for testing. */
    bool operator==(const raft_log_entry_t<state_t> &other) const {
        return type == other.type &&
            term == other.term &&
            change == other.change &&
            config == other.config;
    }
    bool operator!=(const raft_log_entry_t<state_t> &other) const {
        return !(*this == other);
    }

    RDB_MAKE_ME_SERIALIZABLE_4(raft_log_entry_t, type, term, change, config);
};

/* `raft_log_t` stores a slice of the Raft log. There are two situations where this shows
up in Raft: in an "AppendEntries RPC", and in each server's local state. The Raft paper
represents this as three separate variables, but grouping them together makes the
code clearer. */
template<class state_t>
class raft_log_t {
public:
    /* In an append-entries message, `prev_index` and `prev_term` correspond to the
    parameters that Figure 2 of the Raft paper calls `prevLogIndex` and `prevLogTerm`,
    and `entries` corresponds to the parameter that the Raft paper calls `entries`.

    In a server's local state, `prev_index` and `prev_term` correspond to the "last
    included index" and "last included term" variables as described in Section 7.
    `entries` corresponds to the `log` variable described in Figure 2. */

    raft_log_index_t prev_index;
    raft_term_t prev_term;
    std::deque<raft_log_entry_t<state_t> > entries;

    /* Return the latest index that is present in the log. If the log is empty, returns
    the index on which the log is based. */
    raft_log_index_t get_latest_index() const {
        return prev_index + entries.size();
    }

    /* Returns the term of the log entry at the given index. The index must either be
    present in the log or the last index before the log. */
    raft_term_t get_entry_term(raft_log_index_t index) const {
        guarantee(index >= prev_index, "the log doesn't go back this far");
        guarantee(index <= get_latest_index(), "the log doesn't go forward this far");
        if (index == prev_index) {
            return prev_term;
        } else {
            return get_entry_ref(index).term;
        }
    }

    /* Returns the entry in the log at the given index. */
    const raft_log_entry_t<state_t> &get_entry_ref(raft_log_index_t index) const {
        guarantee(index > prev_index, "the log doesn't go back this far");
        guarantee(index <= get_latest_index(), "the log doesn't go forward this far");
        return entries[index - prev_index - 1];
    }

    /* Deletes the log entry at the given index and all entries after it. */
    void delete_entries_from(raft_log_index_t index) {
        guarantee(index > prev_index, "the log doesn't go back this far");
        guarantee(index <= get_latest_index(), "the log doesn't go forward this far");
        entries.erase(entries.begin() + (index - prev_index - 1), entries.end());
    }

    /* Deletes the log entry at the given index and all entries before it. Note that
    `index` may be after the last log entry. */
    void delete_entries_to(raft_log_index_t index, raft_term_t index_term) {
        guarantee(index > prev_index, "the log doesn't go back this far");
        if (index <= get_latest_index()) {
            guarantee(index_term == get_entry_term(index));
            entries.erase(entries.begin(), entries.begin() + (index - prev_index));
        } else {
            entries.clear();
        }
        prev_index = index;
        prev_term = index_term;
    }

    /* Appends the given entry ot the log. */
    void append(const raft_log_entry_t<state_t> &entry) {
        entries.push_back(entry);
    }

    /* The equality and inequality operators are for testing. */
    bool operator==(const raft_log_t<state_t> &other) const {
        return prev_index == other.prev_index &&
            prev_term == other.prev_term &&
            entries == other.entries;
    }
    bool operator!=(const raft_log_t<state_t> &other) const {
        return !(*this == other);
    }

    RDB_MAKE_ME_SERIALIZABLE_3(raft_log_t, prev_index, prev_term, entries);
};

/* `raft_persistent_state_t` describes the information that each member of the Raft
cluster persists to stable storage. */
template<class state_t>
class raft_persistent_state_t {
public:
    /* `make_initial()` returns a `raft_persistent_state_t` for a member of a new Raft
    instance with starting state `initial_state` and configuration `initial_config`. The
    caller must ensure that every member of the new Raft cluster starts with the same
    values for these variables. */
    static raft_persistent_state_t make_initial(
        const state_t &initial_state,
        const raft_config_t &initial_config);

    /* The remaining members are public so that `raft_member_t` and implementations of
    `raft_storage_interface_t` can access them. In general nothing else besides those two
    should have to access these. */

    /* `current_term` and `voted_for` correspond to the variables with the same names in
    Figure 2 of the Raft paper. */
    raft_term_t current_term;
    raft_member_id_t voted_for;

    /* `snapshot_state` corresponds to the stored snapshotted state, as described in
    Section 7. */
    state_t snapshot_state;

    /* `snapshot_config` corresponds to the stored snapshotted configuration, as
    described in Section 7. */
    raft_complex_config_t snapshot_config;

    /* `log.prev_index` and `log.prev_term` correspond to the "last included index" and
    "last included term" as described in Section 7. `log.entries` corresponds to the
    `log` variable in Figure 2. */
    raft_log_t<state_t> log;

    /* This implementation deviates from the Raft paper by also persisting
    `commit_index`. This ensures that the Raft committed state doesn't revert to an
    earlier state if the member crashes and restarts. */
    raft_log_index_t commit_index;

    /* The equality and inequality operators are for testing. */
    bool operator==(const raft_persistent_state_t<state_t> &other) const {
        return current_term == other.current_term &&
            voted_for == other.voted_for &&
            snapshot_state == other.snapshot_state &&
            snapshot_config == other.snapshot_config &&
            log == other.log &&
            commit_index == other.commit_index;
    }
    bool operator!=(const raft_persistent_state_t<state_t> &other) const {
        return !(*this == other);
    }

    RDB_MAKE_ME_SERIALIZABLE_6(raft_persistent_state_t, current_term, voted_for,
        snapshot_state, snapshot_config, log, commit_index);
};

/* `raft_storage_interface_t` is an abstract class that `raft_member_t` uses to store
data on disk. */
template<class state_t>
class raft_storage_interface_t {
public:
    /* `get()` returns a stable pointer to the current state. The `raft_member_t` will
    take this pointer once and then hold it. It's a const pointer; the `raft_member_t`
    must use the other methods in order to change the state. */
    virtual const raft_persistent_state_t<state_t> *get() = 0;

    /* These methods perform various combinations of changes on the stored state. The
    changes will always be visible in the pointer returned by `get()` and also stored
    safely on disk by the time the method call returns. The changes will always be
    atomic. */

    /* Set `current_term` and `voted_for`, leaving everything else alone. */
    virtual void write_current_term_and_voted_for(
        raft_term_t current_term,
        raft_member_id_t voted_for) = 0;

    /* Set `commit_index`, leaving everything else alone. */
    virtual void write_commit_index(
        raft_log_index_t commit_index) = 0;

    /* Delete any existing log entries in the stored log after `first_replaced`, and then
    copy any log entries in `source` after `first_replaced` into the stored log. */
    virtual void write_log_replace_tail(
        const raft_log_t<state_t> &source,
        raft_log_index_t first_replaced) = 0;

    /* Append a single entry to the log. */
    virtual void write_log_append_one(
        const raft_log_entry_t<state_t> &entry) = 0;

    /* Overwrite `snapshot_state` and `snapshot_config`. If `erase_log` is `true`, it
    erase the entire log; otherwise, only erase log entries with indexes less than or
    equal to `log_prev_index`. Set `log.prev_index` and `log.prev_term` to
    `log_prev_index` and `log_prev_term`. Set `commit_index`. */
    virtual void write_snapshot(
        const state_t &snapshot_state,
        const raft_complex_config_t &snapshot_config,
        bool clear_log,
        raft_log_index_t log_prev_index,
        raft_term_t log_prev_term,
        raft_log_index_t commit_index) = 0;

protected:
    virtual ~raft_storage_interface_t() { }
};

/* `raft_rpc_request_t` describes a request that one Raft member sends over the network
to another Raft member. It actually can describe one of three request types,
corresponding to the three RPCs in the Raft paper, but they're bundled together into one
type for the convenience of code that uses Raft. */
template<class state_t>
class raft_rpc_request_t {
private:
    template<class state2_t> friend class raft_member_t;

    /* `request_vote_t` describes the parameters to the "RequestVote RPC" described
    in Figure 2 of the Raft paper. */
    class request_vote_t {
    public:
        /* `term`, `candidate_id`, `last_log_index`, and `last_log_term` correspond to
        the parameters with the same names in the Raft paper. */
        raft_term_t term;
        raft_member_id_t candidate_id;
        raft_log_index_t last_log_index;
        raft_term_t last_log_term;
        RDB_MAKE_ME_SERIALIZABLE_4(request_vote_t,
            term, candidate_id, last_log_index, last_log_term);
    };

    /* `install_snapshot_t` describes the parameters of the "InstallSnapshot RPC"
    described in Figure 13 of the Raft paper. */
    class install_snapshot_t {
    public:
        /* `term`, `leader_id`, `last_included_index`, and `last_included_term`
        correspond to the parameters with the same names in the Raft paper. In the Raft
        paper, the content of the snapshot is sent as a series of binary blobs, but we
        don't want to do that; instead, we send the `state_t` and `raft_complex_config_t`
        directly. So our `snapshot_state` and `snapshot_config` parameters replace the
        `offset`, `data`, and `done` parameters of the Raft paper. */
        raft_term_t term;
        raft_member_id_t leader_id;
        raft_log_index_t last_included_index;
        raft_term_t last_included_term;
        state_t snapshot_state;
        raft_complex_config_t snapshot_config;
        RDB_MAKE_ME_SERIALIZABLE_6(install_snapshot_t,
            term, leader_id, last_included_index, last_included_term, snapshot_state,
            snapshot_config);
    };

    /* `append_entries_t` describes the parameters of the "AppendEntries RPC" described
    in Figure 2 of the Raft paper. */
    class append_entries_t {
    public:
        /* `term`, `leader_id`, and `leader_commit` correspond to the parameters with the
        same names in the Raft paper. `entries` corresponds to three of the paper's
        variables: `prevLogIndex`, `prevLogTerm`, and `entries`. */
        raft_term_t term;
        raft_member_id_t leader_id;
        raft_log_t<state_t> entries;
        raft_log_index_t leader_commit;
        RDB_MAKE_ME_SERIALIZABLE_4(append_entries_t,
            term, leader_id, entries, leader_commit);
    };

    boost::variant<request_vote_t, install_snapshot_t, append_entries_t> request;

    RDB_MAKE_ME_SERIALIZABLE_1(raft_rpc_request_t, request);
};

/* `raft_rpc_reply_t` describes the reply to a `raft_rpc_request_t`. */
class raft_rpc_reply_t {
private:
    template<class state2_t> friend class raft_member_t;

    /* `request_vote_t` describes the information returned from the "RequestVote
    RPC" described in Figure 2 of the Raft paper. */
    class request_vote_t {
    public:
        raft_term_t term;
        bool vote_granted;
        RDB_MAKE_ME_SERIALIZABLE_2(request_vote_t, term, vote_granted);
    };

    /* `install_snapshot_t` describes in the information returned from the
    "InstallSnapshot RPC" described in Figure 13 of the Raft paper. */
    class install_snapshot_t {
    public:
        raft_term_t term;
        RDB_MAKE_ME_SERIALIZABLE_1(install_snapshot_t, term);
    };

    /* `append_entries_t` describes the information returned from the
    "AppendEntries RPC" described in Figure 2 of the Raft paper. */
    class append_entries_t {
    public:
        raft_term_t term;
        bool success;
        RDB_MAKE_ME_SERIALIZABLE_2(append_entries_t, term, success);
    };

    boost::variant<request_vote_t, install_snapshot_t, append_entries_t> reply;

    RDB_MAKE_ME_SERIALIZABLE_1(raft_rpc_reply_t, reply);
};

/* `raft_network_interface_t` is the abstract class that `raft_member_t` uses to send
messages over the network. */
template<class state_t>
class raft_network_interface_t {
public:
    /* `send_rpc()` sends a message to the Raft member indicated in the `dest` field. The
    message will be delivered by calling the `on_rpc()` method on the `raft_member_t` in
    question.
      * If the RPC is delivered successfully, `send_rpc()` will return `true`, and
        the reply will be stored in `*reply_out`.
      * If something goes wrong, `send_rpc()` will return `false`. The RPC may or may not
        have been delivered in this case. The caller should wait until the Raft member is
        present in `get_connected_members()` before trying again.
      * If the interruptor is pulsed, it throws `interrupted_exc_t`. The RPC may or may
        not have been delivered. */
    virtual bool send_rpc(
        const raft_member_id_t &dest,
        const raft_rpc_request_t<state_t> &request,
        signal_t *interruptor,
        raft_rpc_reply_t *reply_out) = 0;

    /* `send_virtual_heartbeats()` sends a virtual continuous stream of heartbeat
    messages to all other members. The stream is "virtual" in that it actually consists
    of a single start message and a single stop message rather than repeated actual
    messages. The other members can receive it by looking in `get_connected_members()`.
    Calling `send_virtual_heartbeats()` with an empty `boost::optional` stops the stream.
    */
    virtual void send_virtual_heartbeats(
        const boost::optional<raft_term_t> &term) = 0;

    /* `get_connected_members()` has an entry for every Raft member that we think we're
    currently connected to. If the member is sending virtual heartbeats, the values will
    be the term it is sending heartbeats for; otherwise, the values will be empty
    `boost::optional`s. */
    virtual watchable_map_t<raft_member_id_t, boost::optional<raft_term_t> >
        *get_connected_members() = 0;

protected:
    virtual ~raft_network_interface_t() { }
};

/* `raft_member_t` is responsible for managing the activity of a single member of the
Raft cluster. */
template<class state_t>
class raft_member_t : public home_thread_mixin_debug_only_t
{
public:
    raft_member_t(
        const raft_member_id_t &this_member_id,
        /* The state in `storage` must already be initialized */
        raft_storage_interface_t<state_t> *storage,
        raft_network_interface_t<state_t> *network,
        /* We'll print log messages of the form `<log_prefix>: <message>`. If
        `log_prefix` is empty, we won't print any messages. */
        const std::string &log_prefix,
        const raft_start_election_immediately_t start_election_immediately);

    ~raft_member_t();

    /* `state_and_config_t` describes the Raft cluster's current state, configuration,
    and log index all in the same struct. The reason for putting them in the same struct
    is so that they can be stored in a `watchable_t` and kept in sync. */
    class state_and_config_t {
    public:
        state_and_config_t(raft_log_index_t _log_index, const state_t &_state,
                           const raft_complex_config_t &_config) :
            log_index(_log_index), state(_state), config(_config) { }
        raft_log_index_t log_index;
        state_t state;
        raft_complex_config_t config;
    };

    /* `get_committed_state()` describes the state of the Raft cluster after all
    committed log entries have been applied. */
    clone_ptr_t<watchable_t<state_and_config_t> > get_committed_state() {
        assert_thread();
        return committed_state.get_watchable();
    }

    /* `get_latest_state()` describes the state of the Raft cluster if every log entry,
    including uncommitted entries, has been applied. */
    clone_ptr_t<watchable_t<state_and_config_t> > get_latest_state() {
        assert_thread();
        return latest_state.get_watchable();
    }

    /* `change_lock_t` freezes the Raft member state, for example in preparation for
    calling `propose_*()`. Only one `change_lock_t` can exist at a time, and while it
    exists, the Raft member will not process normal traffic; so don't keep the
    `change_lock_t` around longer than necessary. However, it is safe to block while
    holding the `change_lock_t` if you need to.

    The point of `change_lock_t` is that `get_latest_state()` will not change while the
    `change_lock_t` exists, unless the lock owner calls `propose_*()`. The state reported
    by `get_latest_state()` is guaranteed to be the state that the proposed change will
    be applied to. This makes it possible to atomically read the state and issue a change
    conditional on the state. */
    class change_lock_t {
    public:
        change_lock_t(raft_member_t *parent, signal_t *interruptor);
    private:
        friend class raft_member_t;
        new_mutex_acq_t mutex_acq;
    };

    /* `get_state_for_init()` returns a `raft_persistent_state_t` that could be used to
    initialize a new member joining the Raft cluster.
    A `change_lock_t` must be constructed on this `raft_member_t` before calling this.
    This is a separate step so that `get_state_for_init()` doesn't need to block in
    order to obtain a lock internally. */
    raft_persistent_state_t<state_t> get_state_for_init(
        const change_lock_t &change_lock_proof);

    /* Here's how to perform a Raft transaction:

    1. Find a `raft_member_t` in the cluster for which `get_readiness_for_change()`
    returns true. (For a config transaction, use `get_readiness_for_config_change()`
    instead.)

    2. Construct a `change_lock_t` on that `raft_member_t`.

    3. Call `propose_*()`. You can make multiple calls to `propose_change()` and
    `propose_noop()` with the same `change_lock_t`, but no more than one call to
    `propose_config_change()`.

    4. Destroy the `change_lock_t` so the Raft cluster can process your transaction.

    5. If you need to be notified of whether your transaction succeeds or not, wait on
    the `change_token_t` returned by `propose_*()`. */

    /* These watchables indicate whether this Raft member is ready to accept changes. In
    general, if these watchables are true, then `propose_*()` will probably succeed.
    (However, this is not guaranteed.) If these watchables are false, don't bother trying
    `propose_*()`.

    Under the hood, these are true if:
    - This member is currently the leader
    - This member is in contact with a quorum of followers
    - We are not currently in a reconfiguration (for `get_readiness_for_config_change()`)
   */
    clone_ptr_t<watchable_t<bool> > get_readiness_for_change() {
        return readiness_for_change.get_watchable();
    }
    clone_ptr_t<watchable_t<bool> > get_readiness_for_config_change() {
        return readiness_for_config_change.get_watchable();
    }

    /* `change_token_t` is a way to track the progress of a change to the Raft cluster.
    It's a promise that will be `true` if the change has been committed, and `false` if
    something went wrong. If it returns `false`, the change may or may not eventually be
    committed anyway. */
    class change_token_t : public promise_t<bool> {
    private:
        friend class raft_member_t;
        change_token_t(raft_member_t *parent, raft_log_index_t index, bool is_config);
        promise_t<bool> promise;
        bool is_config;
        multimap_insertion_sentry_t<raft_log_index_t, change_token_t *> sentry;
    };

    /* `propose_change()` tries to apply a `change_t` to the cluster.
    `propose_config_change()` tries to change the cluster's configuration.
    `propose_noop()` executes a transaction with no side effects; this can be used to
    test if this node is actually a functioning leader.

    `propose_*()` will block while the change is being initiated; this should be a
    relatively quick process.

    If the change is successfully initiated, `propose_*()` will return a `change_token_t`
    that you can use to monitor the progress of the change. If it is not successful, it
    will return `nullptr`. See `get_readiness_for_[config_]change()` for an explanation
    of when and why it will return `nullptr`. */
    scoped_ptr_t<change_token_t> propose_change(
        change_lock_t *change_lock,
        const typename state_t::change_t &change);
    scoped_ptr_t<change_token_t> propose_config_change(
        change_lock_t *change_lock,
        const raft_config_t &new_config);
    scoped_ptr_t<change_token_t> propose_noop(
        change_lock_t *change_lock);

    /* When a Raft member calls `send_rpc()` on its `raft_network_interface_t`, the RPC
    is sent across the network and delivered by calling `on_rpc()` at its destination. */
    void on_rpc(
        const raft_rpc_request_t<state_t> &request,
        raft_rpc_reply_t *reply_out);

#ifndef NDEBUG
    /* `check_invariants()` asserts that the given collection of Raft cluster members are
    in a valid, consistent state. This may block, because it needs to acquire each
    member's mutex, but it will not modify anything. Since this requires direct access to
    each member of the Raft cluster, it's only useful for testing. */
    static void check_invariants(
        const std::set<raft_member_t<state_t> *> &members);
#endif

private:
    enum class mode_t {
        follower,
        candidate,
        leader
    };

    /* These are the minimum and maximum election timeouts. In section 5.6, the Raft
    paper suggests that a typical election timeout should be somewhere between 10ms and
    500ms. We use somewhat larger numbers to reduce server traffic, at the cost of longer
    periods of unavailability when a master dies. */
    static const int32_t election_timeout_min_ms = 1000,
                         election_timeout_max_ms = 2000;

    /* When the number of committed entries in the log exceeds this number, we will take
    a snapshot to compress them. */
    static const size_t snapshot_threshold = 20;

    /* Note: Methods prefixed with `follower_`, `candidate_`, or `leader_` are methods
    that are only used when in that state. This convention will hopefully make the code
    slightly clearer. */

    /* `on_rpc()` calls one of these three methods depending on what type of RPC it
    received. */
    void on_request_vote_rpc(
        const typename raft_rpc_request_t<state_t>::request_vote_t &rpc,
        raft_rpc_reply_t::request_vote_t *reply_out);
    void on_install_snapshot_rpc(
        const typename raft_rpc_request_t<state_t>::install_snapshot_t &rpc,
        raft_rpc_reply_t::install_snapshot_t *reply_out);
    void on_append_entries_rpc(
        const typename raft_rpc_request_t<state_t>::append_entries_t &rpc,
        raft_rpc_reply_t::append_entries_t *reply_out);

#ifndef NDEBUG
    /* Asserts that all of the invariants that can be checked locally hold true. This
    doesn't block or modify anything. It should be safe to call it at any time (except
    when in between modifying two variables that should remain consistent with each
    other, of course). In general we call it whenever we acquire or release the mutex,
    because we know that the variables should be consistent at those times. */
    void check_invariants(const new_mutex_acq_t *mutex_acq);
#endif

    /* `on_connected_members_change()` is called whenever the contents of
    `network->get_connected_members()` changes. If we're a leader, we use it to run
    `update_readiness_for_change()`. We also use it to check for higher terms in virtual
    heartbeats. */
    void on_connected_members_change(
        const raft_member_id_t &member_id,
        const boost::optional<raft_term_t> *value);

    /* `on_rpc_from_leader()` is a helper function that we call in response to
    AppendEntries RPCs, InstallSnapshot RPCs, and virtual heartbeats. It returns `true`
    if the RPC is valid, and `false` if we should reject the RPC. */
    bool on_rpc_from_leader(
        const raft_member_id_t &request_leader_id,
        raft_term_t request_term,
        const new_mutex_acq_t *mutex_acq);

    /* `on_watchdog()` is called if we haven't heard from the leader within an election
    timeout. it starts a new election by spawning `candidate_and_leader_coro()`. */
    void on_watchdog();

    /* `apply_log_entries()` updates `state_and_config` with the entries from `log` with
    indexes `first <= index <= last`. */
    static void apply_log_entries(
        state_and_config_t *state_and_config,
        const raft_log_t<state_t> &log,
        raft_log_index_t first,
        raft_log_index_t last);

    /* `update_term()` sets the term to `new_term` and resets all per-term variables.
    Since we often want to set `ps().voted_for` to something immediately after starting a
    new term, it allows setting that to avoid an extra storage write operation. */
    void update_term(raft_term_t new_term,
                     raft_member_id_t new_voted_for,
                     const new_mutex_acq_t *mutex_acq);

    /* When we change the commit index we have to also apply changes to the state
    machine. `update_commit_index()` handles that automatically. */
    void update_commit_index(raft_log_index_t new_commit_index,
                             const new_mutex_acq_t *mutex_acq);

    /* When we change `match_index` we might have to update `commit_index` as well.
    `leader_update_match_index()` handles that automatically. */
    void leader_update_match_index(
        raft_member_id_t key,
        raft_log_index_t new_value,
        const new_mutex_acq_t *mutex_acq);

    /* `update_readiness_for_change()` should be called whenever any of the variables
    that are used to compute `readiness_for_change` or `readiness_for_config_change` are
    modified. */
    void update_readiness_for_change();

    /* `candidate_or_leader_become_follower()` moves us from the `candidate` or `leader`
    state to `follower` state. It kills `candidate_and_leader_coro()` and blocks until it
    exits. */
    void candidate_or_leader_become_follower(const new_mutex_acq_t *mutex_acq);

    /* `candidate_and_leader_coro()` contains most of the candidate- and leader-specific
    logic. It runs in a separate coroutine for as long as we are a candidate or leader.
    */
    void candidate_and_leader_coro(
        /* A `new_mutex_acq_t` allocated on the heap. `candidate_and_leader_coro()` takes
        ownership of it. */
        new_mutex_acq_t *mutex_acq_on_heap,
        /* To make sure that `candidate_and_leader_coro` stops before the `raft_member_t`
        is destroyed. This is also used by `candidate_or_leader_become_follower()` to
        interrupt `candidate_and_leader_coro()`. */
        auto_drainer_t::lock_t leader_keepalive);

    /* `candidate_run_election()` is a helper function for `candidate_and_leader_coro()`.
    It sends out request-vote RPCs and wait for us to get enough votes. It blocks until
    we are elected or `cancel_signal` is pulsed. The caller is responsible for detecting
    the case where another leader is elected and also for detecting the case where the
    election times out, and pulsing `cancel_signal`. It returns `true` if we were
    elected. */
    bool candidate_run_election(
        /* Note that `candidate_run_election()` may temporarily release `mutex_acq`, but
        it will always be holding the lock when `run_election()` returns. But if
        `interruptor` is pulsed it will throw `interrupted_exc_t` and not reacquire the
        lock. */
        scoped_ptr_t<new_mutex_acq_t> *mutex_acq,
        signal_t *cancel_signal,
        signal_t *interruptor);

    /* `leader_spawn_update_coros()` is a helper function for
    `candidate_and_leader_coro()` that spawns or kills instances of `run_updates()` as
    necessary to ensure that there is always one for each cluster member. */
    void leader_spawn_update_coros(
        /* The value of `nextIndex` to use for each newly connected peer. */
        raft_log_index_t initial_next_index,
        /* A map containing an `auto_drainer_t` for each running update coroutine. */
        std::map<raft_member_id_t, scoped_ptr_t<auto_drainer_t> > *update_drainers,
        const new_mutex_acq_t *mutex_acq);

    /* `leader_send_updates()` is a helper function for `candidate_and_leader_coro()`;
    `leader_spawn_update_coros()` spawns one in a new coroutine for each peer. It pushes
    install-snapshot RPCs and/or append-entry RPCs out to the given peer until
    `update_keepalive.get_drain_signal()` is pulsed. */
    void leader_send_updates(
        const raft_member_id_t &peer,
        raft_log_index_t initial_next_index,
        auto_drainer_t::lock_t update_keepalive);

    /* `leader_continue_reconfiguration()` is a helper function for
    `candidate_and_leader_coro()`. It checks if we have completed the first phase of a
    reconfiguration (by committing a joint consensus configuration) and if so, it starts
    the second phase by committing the new configuration. It also checks if we have
    completed the second phase and if so, it makes us step down. */
    void leader_continue_reconfiguration(
        const new_mutex_acq_t *mutex_acq);

    /* `candidate_or_leader_note_term()` is a helper function for
    `candidate_run_election()` and `leader_send_updates()`. If the given term is greater
    than the current term, it updates the current term and interrupts
    `candidate_and_leader_coro()`. It returns `true` if the term was changed. */
    bool candidate_or_leader_note_term(
        raft_term_t term,
        const new_mutex_acq_t *mutex_acq);

    /* `leader_append_log_entry()` is a helper for `propose_change_if_leader()` and
    `propose_config_change_if_leader()`. It adds an entry to the log but doesn't wait for
    the entry to be committed. */
    void leader_append_log_entry(
        const raft_log_entry_t<state_t> &log_entry,
        const new_mutex_acq_t *mutex_acq);

    /* This is because we end up needing to access the persistent state very frequently,
    and `storage->get()` is too verbose. */
    raft_persistent_state_t<state_t> const &ps() {
        return *storage->get();
    }

    /* The member ID of the member of the Raft cluster represented by this
    `raft_member_t`. */
    const raft_member_id_t this_member_id;

    raft_storage_interface_t<state_t> *const storage;
    raft_network_interface_t<state_t> *const network;

    const std::string log_prefix;

    /* `committed_state` describes the state after all committed log entries have been
    applied. The `state` field of `committed_state` is equivalent to the "state machine"
    in the Raft paper. The `log_index` field is equal to the `lastApplied` and
    `commitIndex` variables in Figure 2 of the Raft paper. This implementation deviates
    from the Raft paper in that the paper allows for a delay between when changes are
    committed and when they are applied to the state machine, so `lastApplied` may lag
    behind `commitIndex`. But we always apply changes to the state machine as soon as
    they are committed, so `lastApplied` and `commitIndex` are equivalent for us. */
    watchable_variable_t<state_and_config_t> committed_state;

    /* `latest_state` describes the state after all log entries, not only committed ones,
    have been applied. This is publicly exposed to the user, and it's also useful because
    "a server always uses the latest configuration in its log, regardless of whether the
    entry is committed" (Raft paper, Section 6). Whenever `ps.log` is modified,
    `latest_state` must be updated to keep in sync. */
    watchable_variable_t<state_and_config_t> latest_state;

    /* Only `candidate_and_leader_coro()` should ever change `mode` */
    mode_t mode;

    /* `current_term_leader_id` is the ID of the member that is leader during this term.
    If we haven't seen any node acting as leader this term, it's `nil_uuid()`. We use it
    to redirect clients as described in Figure 2 and Section 8. */
    raft_member_id_t current_term_leader_id;

    /* If any member is sending us valid virtual heartbeats for `ps.current_term`, then
    `virtual_heartbeat_sender` will be set to that member; otherwise it will be nil. */
    raft_member_id_t virtual_heartbeat_sender;

    /* If `virtual_heartbeat_sender` is non-empty, we'll set
    `virtual_heartbeat_watchdog_blockers` to make sure that neither `watchdog` nor
    `watchdog_leader` gets triggered while we're receiving virtual heartbeats. */
    scoped_ptr_t<watchdog_timer_t::blocker_t> virtual_heartbeat_watchdog_blockers[2];

    /* `match_indexes` corresponds to the `matchIndex` array described in Figure 2 of the
    Raft paper. Note that it is only used if we are the leader; if we are not the leader,
    then it must be empty. */
    std::map<raft_member_id_t, raft_log_index_t> match_indexes;

    /* `readiness_for_change` and `readiness_for_config_change` track whether this member
    is ready to accept changes. A member is ready for changes if it is leader and in
    contact with a quorum of followers; it is ready for config changes if those
    conditions are met and it is also not currently in a reconfiguration. Whenever any of
    those variables changes, `update_readiness_for_change()` must be called. */
    watchable_variable_t<bool> readiness_for_change;
    watchable_variable_t<bool> readiness_for_config_change;

    /* `propose_[config_]change()` inserts a `change_token_t *` into `change_tokens`. If
    we stop being leader or lose contact with a majority of the cluster nodes, then all
    of the change tokens will be notified that the changes they were waiting on have
    failed. Whenever we commit a transaction, we also notify change tokens for success if
    appropriate. If we are not leader, `change_tokens` will be empty. */
    std::multimap<raft_log_index_t, change_token_t *> change_tokens;

    /* This mutex ensures that operations don't interleave in confusing ways. Each RPC
    acquires this mutex when it begins and releases it when it returns. Also, if
    `candidate_and_leader_coro()` is running, it holds this mutex when actively
    manipulating state and releases it when waiting. In general we don't hold the mutex
    when responding to an interruptor. */
    new_mutex_t mutex;

    /* This mutex assertion controls writes to the Raft log and associated state.
    Specifically, anything writing to `ps().log`, `ps().snapshot_*`, `committed_state`,
    or `latest_state` should hold this mutex assertion.

    If we are follower, `on_append_entries_rpc()` and `on_install_snapshot_rpc()` acquire
    this temporarily; if we are candidate or leader, `candidate_and_leader_coro()` holds
    this at all times. */
    mutex_assertion_t log_mutex;

    /* This makes sure that `candidate_and_leader_coro()` stops when the `raft_member_t`
    is destroyed. It's in a `scoped_ptr_t` so that
    `candidate_or_leader_become_follower()` can destroy it to kill
    `candidate_and_leader_coro()`. */
    scoped_ptr_t<auto_drainer_t> leader_drainer;

    /* Occasionally we have to spawn miscellaneous coroutines. This makes sure that they
    all get stopped before the `raft_member_t` is destroyed. It's in a `scoped_ptr_t` so
    that the destructor can destroy it early. */
    scoped_ptr_t<auto_drainer_t> drainer;

    /* Whenever we get a valid RPC from a candidate, we notify `watchdog`. Whenever we
    get a valid RPC from a leader, we notify both watchdogs. If `watchdog` is triggered,
    we start a new election. The purpose of `watchdog_leader_only` is that to keep track
    of whether we have seen a valid leader recently, so that we know whether or not to
    accept RequestVote RPCs. */
    scoped_ptr_t<watchdog_timer_t> watchdog, watchdog_leader_only;

    /* This calls `update_readiness_for_change()` whenever a peer connects or
    disconnects. */
    scoped_ptr_t<watchable_map_t<raft_member_id_t, boost::optional<raft_term_t> >
        ::all_subs_t> connected_members_subs;
};

#endif /* CLUSTERING_GENERIC_RAFT_CORE_HPP_ */

