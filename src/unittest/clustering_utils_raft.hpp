// Copyright 2010-2013 RethinkDB, all rights reserved.
#include <map>

#include "unittest/gtest.hpp"

#include "clustering/administration/metadata.hpp"
#include "clustering/generic/raft_core.hpp"
#include "clustering/generic/raft_core.tcc"
#include "clustering/generic/raft_network.hpp"
#include "clustering/generic/raft_network.tcc"
#include "unittest/clustering_utils.hpp"
#include "unittest/dummy_metadata_controller.hpp"
#include "unittest/unittest_utils.hpp"

namespace unittest {

/* `dummy_raft_state_t` is meant to be used as the `state_t` parameter to
`raft_member_t`, with the `change_t` parameter set to `uuid_u`. It just records all the
changes it receives and their order. */
class dummy_raft_state_t {
public:
    typedef uuid_u change_t;
    std::vector<uuid_u> state;
    void apply_change(const change_t &uuid) {
        state.push_back(uuid);
    }
    bool operator==(const dummy_raft_state_t &other) const {
        return state == other.state;
    }
    bool operator!=(const dummy_raft_state_t &other) const {
        return state != other.state;
    }
};
RDB_MAKE_SERIALIZABLE_1(dummy_raft_state_t, state);

typedef raft_member_t<dummy_raft_state_t> dummy_raft_member_t;

/* `dummy_raft_cluster_t` manages a collection of `dummy_raft_member_t`s. It handles
passing RPCs between them, and it can simulate crashes and netsplits. It periodically
automatically calls `check_invariants()` on its members. */
class dummy_raft_cluster_t {
public:
    /* An `alive` member is a `dummy_raft_member_t` that can communicate with other alive
    members. An `isolated` member is a `dummy_raft_member_t` that cannot communicate with
    any other members. A `dead` member is just a stored `raft_persistent_state_t`. */
    enum class live_t { alive, isolated, dead };

    static const char *show_live(live_t l);

    void print_state();

    /* The constructor starts a cluster of `num` alive members with the given initial
    state. */
    dummy_raft_cluster_t(
        size_t num,
        const dummy_raft_state_t &initial_state,
        std::vector<raft_member_id_t> *member_ids_out);
    ~dummy_raft_cluster_t();

    /* `join()` adds a new member to the cluster. The caller is responsible for running a
    Raft transaction to modify the config to include the new member. */
    raft_member_id_t join();

    live_t get_live(const raft_member_id_t &member_id) const;
    void set_live(const raft_member_id_t &member_id, live_t live);

    raft_member_id_t find_leader(signal_t *interruptor);
    raft_member_id_t find_leader(int timeout);

    /* Tries to perform the given change on the member with the given ID. */
    bool try_change(
        raft_member_id_t id,
        const uuid_u &change,
        signal_t *interruptor);

    /* Like `try_change()` but for Raft configuration changes */
    bool try_config_change(
        raft_member_id_t id,
        const raft_config_t &new_config,
        signal_t *interruptor);

    /* `get_all_member_ids()` returns the member IDs of all the members of the cluster,
    alive or dead.  */
    std::set<raft_member_id_t> get_all_member_ids();

    /* `run_on_member()` calls the given function for the `dummy_raft_member_t *` with
    the given ID. If the member is currently dead, it calls the function with a NULL
    pointer. */
    void run_on_member(
        const raft_member_id_t &member_id,
        const std::function<void(dummy_raft_member_t *, signal_t *)> &fun);

private:
    class member_info_t :
        public raft_storage_interface_t<dummy_raft_state_t> {
    public:
        member_info_t(
                const raft_member_id_t &mid,
                const raft_persistent_state_t<dummy_raft_state_t> &ss) :
            member_id(mid), stored_state(ss), live(live_t::dead) { }
        member_info_t(member_info_t &&) = default;
        member_info_t &operator=(member_info_t &&) = default;

        const raft_persistent_state_t<dummy_raft_state_t> *get() {
            return &stored_state;
        }

        void write_current_term_and_voted_for(
            raft_term_t current_term,
            raft_member_id_t voted_for);
        void write_commit_index(raft_log_index_t commit_index);
        void write_log_replace_tail(
            const raft_log_t<dummy_raft_state_t> &log,
            raft_log_index_t first_replaced);
        void write_log_append_one(
            const raft_log_entry_t<dummy_raft_state_t> &entry);
        void write_snapshot(
            const dummy_raft_state_t &snapshot_state,
            const raft_complex_config_t &snapshot_config,
            bool clear_log,
            raft_log_index_t log_prev_index,
            raft_term_t log_prev_term,
            raft_log_index_t commit_index);

        void block();

        raft_member_id_t member_id;
        raft_persistent_state_t<dummy_raft_state_t> stored_state;
        watchable_map_var_t<raft_member_id_t, raft_business_card_t<dummy_raft_state_t> >
            member_directory;
        live_t live;
        scoped_ptr_t<raft_networked_member_t<dummy_raft_state_t> > member;
        scoped_ptr_t<auto_drainer_t> member_drainer;
        scoped_ptr_t<watchable_t<raft_business_card_t<dummy_raft_state_t> >
            ::subscription_t> bcard_subs;
    };

    void add_member(
        const raft_member_id_t &member_id,
        raft_persistent_state_t<dummy_raft_state_t> initial_state);

    void check_invariants(UNUSED auto_drainer_t::lock_t keepalive);

    connectivity_cluster_t connectivity_cluster;
    mailbox_manager_t mailbox_manager;
    heartbeat_semilattice_metadata_t heartbeat_semilattice_metadata;
    dummy_semilattice_controller_t<heartbeat_semilattice_metadata_t> heartbeat_manager;
    connectivity_cluster_t::run_t connectivity_cluster_run;

    std::map<raft_member_id_t, scoped_ptr_t<member_info_t> > members;
    auto_drainer_t drainer;
    repeating_timer_t check_invariants_timer;
};

/* `dummy_raft_traffic_generator_t` tries to send operations to the given Raft cluster at
a fixed rate. */
class dummy_raft_traffic_generator_t {
public:
    dummy_raft_traffic_generator_t(dummy_raft_cluster_t *_cluster, int num_threads);

    size_t get_num_changes();
    void check_changes_present();

private:
    void do_changes(auto_drainer_t::lock_t keepalive);

    std::set<uuid_u> committed_changes;
    dummy_raft_cluster_t *cluster;
    auto_drainer_t drainer;
};

void do_writes_raft(dummy_raft_cluster_t *cluster, int expect, int ms);

}   /* namespace unittest */

