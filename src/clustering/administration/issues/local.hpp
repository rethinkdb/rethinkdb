// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_ISSUES_LOCAL_HPP_
#define CLUSTERING_ADMINISTRATION_ISSUES_LOCAL_HPP_

#include <string>
#include <vector>

#include "clustering/administration/issues/log_write.hpp"
#include "clustering/administration/issues/memory.hpp"
#include "concurrency/watchable.hpp"
#include "containers/clone_ptr.hpp"
#include "rpc/semilattice/joins/macros.hpp"
#include "rpc/serialize_macros.hpp"

class cluster_directory_metadata_t;

/* "Local issues" are issues that originate on a particular server. When the user queries
the `rethinkdb.current_issues` system table, each server polls the other servers to
retrieve their local issues. */
class local_issues_t {
public:
    std::vector<log_write_issue_t> log_write_issues;
    std::vector<memory_issue_t> memory_issues;
};

RDB_DECLARE_SERIALIZABLE(local_issues_t);

class local_issue_bcard_t {
public:
    typedef mailbox_t<void(mailbox_t<void(local_issues_t)>::address_t)> get_mailbox_t;
    get_mailbox_t::address_t get_mailbox;
};

RDB_DECLARE_SERIALIZABLE(local_issue_bcard_t);

class local_issue_server_t : public home_thread_mixin_t {
public:
    local_issue_server_t(
        mailbox_manager_t *mm,
        log_write_issue_tracker_t *log_write_issue_tracker,
        memory_issue_tracker_t *memory_issue_tracker);

    local_issue_bcard_t get_bcard() {
        return local_issue_bcard_t { get_mailbox.get_address() };
    }

private:
    void on_get(signal_t *, const mailbox_t<void(local_issues_t)>::address_t &reply);

    mailbox_manager_t *const mailbox_manager;
    log_write_issue_tracker_t *const log_write_issue_tracker;
    memory_issue_tracker_t *const memory_issue_tracker;
    local_issue_bcard_t::get_mailbox_t get_mailbox;
    DISABLE_COPYING(local_issue_server_t);
};

class local_issue_client_t : public issue_tracker_t {
public:
    local_issue_client_t(
        mailbox_manager_t *_mailbox_manager,
        watchable_map_t<peer_id_t, cluster_directory_metadata_t> *_directory);
    std::vector<scoped_ptr_t<issue_t> > get_issues(signal_t *interruptor) const;

private:
    mailbox_manager_t *const mailbox_manager;
    watchable_map_t<peer_id_t, cluster_directory_metadata_t> *const directory;
};

#endif /* CLUSTERING_ADMINISTRATION_ISSUES_LOCAL_HPP_ */
