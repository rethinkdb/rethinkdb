// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_ISSUES_LOCAL_HPP_
#define CLUSTERING_ADMINISTRATION_ISSUES_LOCAL_HPP_

#include <string>
#include <vector>

#include "clustering/administration/issues/issue.hpp"
#include "concurrency/watchable.hpp"
#include "containers/clone_ptr.hpp"
#include "rpc/semilattice/joins/macros.hpp"
#include "rpc/serialize_macros.hpp"

class local_issues_t {
public:
    std::vector<log_write_issue_t> log_write_issues;
    std::vector<outdated_index_issue_t> outdated_index_issues;
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
    local_issue_server_t(mailbox_manager_t *mm);

    local_issue_bcard_t get_bcard() {
        return local_issue_bcard_t { get_mailbox.get_address() };
    }

    template <typename local_t>
    class subscription_t {
    public:
        subscription_t(local_issue_aggregator_t *parent,
                       const clone_ptr_t<watchable_t<std::vector<local_t> > > &source,
                       std::vector<local_t> local_issues_t::*field) :
            copier(field, source, &parent->issues_watchable) { }
    private:
        watchable_field_copier_t<std::vector<local_t>, local_issues_t> copier;
    };

private:
    void on_get(signal_t *, const mailbox_t<void(local_issues_t)>::address_t &reply);

    mailbox_manager_t *const mailbox_manager;
    watchable_variable_t<local_issues_t> issues_watchable;
    local_issue_bcard_t::get_mailbox_t get_mailbox;
    DISABLE_COPYING(local_issue_aggregator_t);
};

class local_issue_client_t {
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
