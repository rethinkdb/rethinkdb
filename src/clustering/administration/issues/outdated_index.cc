// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/issues/outdated_index.hpp"
#include "debug.hpp"

typedef outdated_index_issue_server_t::request_address_t request_address_t;
typedef outdated_index_issue_server_t::result_address_t result_address_t;

outdated_index_issue_t::outdated_index_issue_t(outdated_index_map_t &&indexes) :
    outdated_indexes(indexes) { }

std::string outdated_index_issue_t::get_description() const {
    std::string description;
    for (auto it = outdated_indexes.begin(); it != outdated_indexes.end(); ++it) {
        description += strprintf("\n\t%s\t", uuid_to_str(it->first).c_str());
        for (auto jt = it->second.begin(); jt != it->second.end(); ++jt) {
            description += strprintf("%s\"%s\"",
                                     jt != it->second.begin() ? ", " : "", jt->c_str());
        }
    }
    return strprintf("The cluster contains indexes that were created with a previous "
                     "version of the query language which contained some bugs.  These "
                     "should be remade to avoid relying on broken behavior.  See <url> "
                     "for details.\n"
                     "\tTable                               \tIndexes%s",
                     description.c_str());
}

cJSON *outdated_index_issue_t::get_json_description() {
    issue_json_t json;
    json.critical = false;
    json.description = get_description();
    json.type = "OUTDATED_INDEX_ISSUE";
    json.time = get_secs();

    // Put all of the outdated indexes in a map by their uuid, like so:
    // indexes: { '1234-5678': ['sindex_a', 'sindex_b'], '9284-af82': ['sindex_c'] }
    cJSON *res = render_as_json(&json);
    cJSON *obj = cJSON_CreateObject();
    for (auto it = outdated_indexes.begin(); it != outdated_indexes.end(); ++it) {
        cJSON *arr = cJSON_CreateArray();
        for (auto jt = it->second.begin(); jt != it->second.end(); ++jt) {
            cJSON_AddItemToArray(arr, cJSON_CreateString(jt->c_str()));
        }
        cJSON_AddItemToObject(obj, uuid_to_str(it->first).c_str(), arr);
    }
    cJSON_AddItemToObject(res, "indexes", obj);

    return res;
}

outdated_index_issue_t *outdated_index_issue_t::clone() const {
    return new outdated_index_issue_t(outdated_index_map_t(outdated_indexes));
}

outdated_index_issue_server_t::outdated_index_issue_server_t(
    mailbox_manager_t *_mailbox_manager) :
        mailbox_manager(_mailbox_manager),
        request_mailbox(mailbox_manager, std::bind(
            &outdated_index_issue_server_t::handle_request,
            this, ph::_1)),
        local_client(NULL) { }


void outdated_index_issue_server_t::attach_local_client(
        outdated_index_issue_client_t *client) {
    assert_thread();
    rassert(local_client == NULL);
    local_client = client;
}

void outdated_index_issue_server_t::detach_local_client() {
    assert_thread();
    rassert(local_client != NULL);
    local_client = NULL;
}

request_address_t outdated_index_issue_server_t::get_request_mailbox_address() const {
    return request_mailbox.get_address();
}

void outdated_index_issue_server_t::handle_request(result_address_t result_address) {
    outdated_index_map_t res;

    if (local_client != NULL) {
        res = local_client->collect_local_indexes();
    }

    send(mailbox_manager, result_address, std::move(res));
}

outdated_index_issue_client_t::outdated_index_issue_client_t(
        mailbox_manager_t *_mailbox_manager,
        const clone_ptr_t<watchable_t<change_tracking_map_t<peer_id_t,
                                                            request_address_t> > >
            &_outdated_index_mailboxes) :
    mailbox_manager(_mailbox_manager),
    outdated_index_mailboxes(_outdated_index_mailboxes)
{ }

outdated_index_issue_client_t::~outdated_index_issue_client_t() { }

std::set<std::string> *
outdated_index_issue_client_t::get_index_set(const namespace_id_t &ns_id) {
    auto thread_map = outdated_indexes.get();
    auto ns_it = thread_map->find(ns_id);

    if (ns_it == thread_map->end()) {
        ns_it = thread_map->insert(std::make_pair(ns_id, std::set<std::string>())).first;
        guarantee(ns_it != thread_map->end());
    }

    return &ns_it->second;
}

void copy_thread_map(
        one_per_thread_t<outdated_index_map_t> *maps_in,
        std::vector<outdated_index_map_t> *maps_out, int thread_id) {
    on_thread_t thread_switcher((threadnum_t(thread_id)));
    (*maps_out)[thread_id] = *maps_in->get();
}

outdated_index_map_t merge_maps(
        const std::vector<outdated_index_map_t> &maps) {
    outdated_index_map_t res;
    for (auto it = maps.begin(); it != maps.end(); ++it) {
        for (auto jt = it->begin(); jt != it->end(); ++jt) {
            if (!jt->second.empty()) {
                res[jt->first].insert(jt->second.begin(), jt->second.end());
            }
        }
    }
    return std::move(res);
}

void handle_response(cond_t *done_signal,
                     outdated_index_map_t *map_out,
                     outdated_index_map_t &&peer_map) {
    *map_out = peer_map;
    done_signal->pulse();
}

void make_peer_request(mailbox_manager_t *mailbox_manager,
                       const std::map<peer_id_t, request_address_t> &request_mailboxes,
                       std::vector<outdated_index_map_t> *maps_out,
                       int peer_offset,
                       signal_t *interruptor) {
    try {
        cond_t response_done;
        outdated_index_issue_server_t::result_mailbox_t result_mailbox(
            mailbox_manager,
            std::bind(&handle_response,
                      &response_done,
                      &maps_out->at(peer_offset),
                      ph::_1));

        auto peer_it = request_mailboxes.begin();
        for (int i = 0; i < peer_offset; ++i) {
            ++peer_it;
        }

        send(mailbox_manager, peer_it->second, result_mailbox.get_address());

        wait_interruptible(&response_done, interruptor);
    } catch (const interrupted_exc_t &ex) {
        // Disregard this - worst case some indexes aren't listed
    }
}

void get_peers_outdated_indexes(cond_t *done_signal,
                                mailbox_manager_t *mailbox_manager,
                                const std::map<peer_id_t, request_address_t> &request_mailboxes,
                                std::vector<outdated_index_map_t> *maps_out,
                                signal_t *interruptor) {
    pmap(request_mailboxes.size(),
         std::bind(&make_peer_request,
                   mailbox_manager,
                   std::cref(request_mailboxes),
                   maps_out,
                   ph::_1,
                   interruptor));
    done_signal->pulse();
}

outdated_index_map_t outdated_index_issue_client_t::collect_local_indexes() {
    std::vector<outdated_index_map_t> all_maps(get_num_threads());
    pmap(get_num_threads(), std::bind(&copy_thread_map,
                                      &outdated_indexes,
                                      &all_maps,
                                      ph::_1));
    return merge_maps(all_maps);
}

outdated_index_map_t outdated_index_issue_client_t::collect_all_indexes() {
    assert_thread();
    const std::map<peer_id_t, request_address_t> request_mailboxes =
        outdated_index_mailboxes->get().get_inner();
    std::vector<outdated_index_map_t> all_maps(request_mailboxes.size());

    cond_t peer_requests_done;
    signal_timer_t timeout_interruptor;

    // Allow 3s to collect peers' outdated indexes
    timeout_interruptor.start(3000);

    coro_t::spawn_sometime(std::bind(&get_peers_outdated_indexes,
                                     &peer_requests_done,
                                     mailbox_manager,
                                     std::cref(request_mailboxes),
                                     &all_maps,
                                     &timeout_interruptor));

    peer_requests_done.wait();

    return merge_maps(all_maps);
}

// We only create one issue (at most) to avoid spamming issues
std::list<clone_ptr_t<global_issue_t> > outdated_index_issue_client_t::get_issues() {
    std::list<clone_ptr_t<global_issue_t> > issues;
    outdated_index_map_t all_outdated_indexes = collect_all_indexes();
    if (!all_outdated_indexes.empty()) {
        issues.push_back(clone_ptr_t<global_issue_t>(
            new outdated_index_issue_t(std::move(all_outdated_indexes))));
    }
    return std::move(issues);
}
