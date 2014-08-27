// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_ISSUES_OUTDATED_INDEX_HPP_
#define CLUSTERING_ADMINISTRATION_ISSUES_OUTDATED_INDEX_HPP_

#include <map>
#include <set>
#include <list>
#include <string>
#include <list>

#include "http/json.hpp"
#include "containers/incremental_lenses.hpp"
#include "rpc/mailbox/typed.hpp"
#include "rdb_protocol/store.hpp"
#include "clustering/administration/issues/global.hpp"
#include "clustering/administration/issues/json.hpp"

template <class> class semilattice_read_view_t;
typedef std::map<namespace_id_t, std::set<std::string> > outdated_index_map_t;

class outdated_index_issue_t : public global_issue_t {
public:
    explicit outdated_index_issue_t(outdated_index_map_t &&indexes);
    std::string get_description() const;
    cJSON *get_json_description();
    outdated_index_issue_t *clone() const;

    outdated_index_map_t outdated_indexes;

private:
    DISABLE_COPYING(outdated_index_issue_t);
};

class outdated_index_issue_client_t;
class outdated_index_report_impl_t;

class outdated_index_issue_server_t : public home_thread_mixin_t {
public:
    typedef mailbox_t<void(outdated_index_map_t)> result_mailbox_t;
    typedef mailbox_t<void(result_mailbox_t::address_t)> request_mailbox_t;
    typedef result_mailbox_t::address_t result_address_t;
    typedef request_mailbox_t::address_t request_address_t;

    explicit outdated_index_issue_server_t(mailbox_manager_t *_mailbox_manager);

    void attach_local_client(outdated_index_issue_client_t *client);
    void detach_local_client();

    request_address_t get_request_mailbox_address() const;

private:
    void handle_request(result_address_t result_address);

    mailbox_manager_t *mailbox_manager;
    request_mailbox_t request_mailbox;
    outdated_index_issue_client_t *local_client;

    DISABLE_COPYING(outdated_index_issue_server_t);
};

class outdated_index_issue_client_t :
    public global_issue_tracker_t,
    public home_thread_mixin_t {
public:
    outdated_index_issue_client_t(
        mailbox_manager_t *_mailbox_manager,
        const clone_ptr_t<watchable_t<change_tracking_map_t<peer_id_t,
                outdated_index_issue_server_t::request_address_t> > >
            &_outdated_index_mailboxes);
    ~outdated_index_issue_client_t();

    // Returns the set of strings for a given namespace that may be filled
    // by the store_t when it scans its secondary indexes
    std::set<std::string> *get_index_set(const namespace_id_t &ns_id);

    // Returns the set of outdated indexes for all tables hosted on this node
    outdated_index_map_t collect_local_indexes();

    std::list<clone_ptr_t<global_issue_t> > get_issues();

    outdated_index_report_t *create_report(const namespace_id_t &ns_id);

private:
    friend class outdated_index_report_impl_t;
    void destroy_report(outdated_index_report_impl_t *report);

    outdated_index_map_t collect_all_indexes();

    void log_outdated_indexes(namespace_id_t ns_id,
                              std::set<std::string> indexes,
                              auto_drainer_t::lock_t keepalive);

    mailbox_manager_t *mailbox_manager;
    std::set<namespace_id_t> logged_namespaces;
    one_per_thread_t<outdated_index_map_t> outdated_indexes;
    one_per_thread_t<std::set<outdated_index_report_impl_t *> > index_reports;
    clone_ptr_t<watchable_t<change_tracking_map_t<peer_id_t,
            outdated_index_issue_server_t::request_address_t> > >
        outdated_index_mailboxes;

    DISABLE_COPYING(outdated_index_issue_client_t);
};

#endif /* CLUSTERING_ADMINISTRATION_ISSUES_OUTDATED_INDEX_HPP_ */
