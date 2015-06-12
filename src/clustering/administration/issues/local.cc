// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/issues/local.hpp"

#include "arch/runtime/runtime_utils.hpp"
#include "clustering/administration/metadata.hpp"
#include "utils.hpp"

RDB_MAKE_SERIALIZABLE_2_FOR_CLUSTER(
    local_issues_t, log_write_issues, outdated_index_issues);
RDB_MAKE_SERIALIZABLE_1_FOR_CLUSTER(
    local_issue_bcard_t, get_mailbox);

local_issue_server_t::local_issue_server_t(
        mailbox_manager_t *mm,
        log_write_issue_tracker_t *_log_write_issue_tracker,
        outdated_index_issue_tracker_t *_outdated_index_issue_tracker) :
    mailbox_manager(mm),
    log_write_issue_tracker(_log_write_issue_tracker),
    outdated_index_issue_tracker(_outdated_index_issue_tracker),
    get_mailbox(mailbox_manager,
        std::bind(&local_issue_server_t::on_get, this, ph::_1, ph::_2))
    { }

void local_issue_server_t::on_get(
        signal_t *, const mailbox_t<void(local_issues_t)>::address_t &reply) {
    local_issues_t issues;
    issues.log_write_issues = log_write_issue_tracker->get_issues();
    issues.outdated_index_issues = outdated_index_issue_tracker->get_issues();
    send(mailbox_manager, reply, issues);
}

local_issue_client_t::local_issue_client_t(
        mailbox_manager_t *_mailbox_manager,
        watchable_map_t<peer_id_t, cluster_directory_metadata_t> *_directory) :
    mailbox_manager(_mailbox_manager), directory(_directory)
    { }

std::vector<scoped_ptr_t<issue_t> > local_issue_client_t::get_issues(
        signal_t *interruptor) const {
    std::vector<std::pair<server_id_t, local_issue_bcard_t> > bcards;
    directory->read_all(
    [&](const peer_id_t &, const cluster_directory_metadata_t *md) {
        if (md->peer_type == SERVER_PEER) {
            bcards.push_back(std::make_pair(md->server_id, md->local_issue_bcard));
        }
    });
    local_issues_t aggregator;
    static const int max_concurrent_fetches = 16;
    throttled_pmap(bcards.size(), [&](size_t i) {
        try {
            cond_t got_reply;
            mailbox_t<void(local_issues_t)> reply_mailbox(
                mailbox_manager,
                [&](signal_t *, const local_issues_t &issues) {
                    for (const auto &issue : issues.log_write_issues) {
                        log_write_issue_t copy = issue;
                        copy.add_server(bcards[i].first);
                        aggregator.log_write_issues.push_back(copy);
                    }
                    for (const auto &issue : issues.outdated_index_issues) {
                        outdated_index_issue_t copy = issue;
                        copy.add_server(bcards[i].first);
                        aggregator.outdated_index_issues.push_back(copy);
                    }
                    got_reply.pulse();
                });
            disconnect_watcher_t disconnect_watcher(
                mailbox_manager, bcards[i].second.get_mailbox.get_peer());
            send(mailbox_manager, bcards[i].second.get_mailbox,
                reply_mailbox.get_address());
            wait_any_t waiter(&got_reply, &disconnect_watcher);
            wait_interruptible(&waiter, interruptor);
        } catch (const interrupted_exc_t &) {
            /* We'll check outside the `throttled_pmap()` */
        }
    }, max_concurrent_fetches);
    if (interruptor->is_pulsed()) {
        throw interrupted_exc_t();
    }
    std::vector<scoped_ptr_t<issue_t> > res;
    log_write_issue_tracker_t::combine(
        std::move(aggregator.log_write_issues), &res);
    outdated_index_issue_tracker_t::combine(
        std::move(aggregator.outdated_index_issues), &res);
    return res;
}

