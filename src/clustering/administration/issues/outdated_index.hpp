// Copyright 2014-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_ISSUES_OUTDATED_INDEX_HPP_
#define CLUSTERING_ADMINISTRATION_ISSUES_OUTDATED_INDEX_HPP_

#include <map>
#include <set>
#include <string>

#include "http/json.hpp"
#include "containers/incremental_lenses.hpp"
#include "rpc/mailbox/typed.hpp"
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

class outdated_index_issue_tracker_t : public global_issue_tracker_t, public home_thread_mixin_t {
public:
    typedef mailbox_t<void(outdated_index_map_t)> result_mailbox_t;
    typedef mailbox_t<void(result_mailbox_t::address_t)> request_mailbox_t;
    typedef result_mailbox_t::address_t result_address_t;
    typedef request_mailbox_t::address_t request_address_t;

    outdated_index_issue_tracker_t(
        mailbox_manager_t *_mailbox_manager,
        const clone_ptr_t<watchable_t<change_tracking_map_t<peer_id_t,
                                                            request_address_t> > >
            &_outdated_index_mailboxes);
    ~outdated_index_issue_tracker_t();

    std::set<std::string> *get_index_set(const namespace_id_t &ns_id);
    std::list<clone_ptr_t<global_issue_t> > get_issues();

    static request_address_t get_dummy_mailbox(mailbox_manager_t *mailbox_manager);
    request_address_t get_request_mailbox() {
        return request_mailbox.get_address();
    }

private:
    void handle_request(result_address_t result_address);

    outdated_index_map_t collect_and_merge_maps();

    mailbox_manager_t *mailbox_manager;
    request_mailbox_t request_mailbox;
    clone_ptr_t<watchable_t<change_tracking_map_t<peer_id_t, request_address_t> > >
        outdated_index_mailboxes;
    one_per_thread_t<outdated_index_map_t> outdated_indexes;

    DISABLE_COPYING(outdated_index_issue_tracker_t);
};

#endif /* CLUSTERING_ADMINISTRATION_ISSUES_OUTDATED_INDEX_HPP_ */
