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

    // Put all of the outdated indexes in a map by their table's uuid, like so:
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
            this, ph::_1, ph::_2)),
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

void outdated_index_issue_server_t::handle_request(
        UNUSED signal_t *interruptor,
        result_address_t result_address) {
    outdated_index_map_t res;

    if (local_client != NULL) {
        res = local_client->collect_local_indexes();
    }

    send(mailbox_manager, result_address, std::move(res));
}

class outdated_index_report_impl_t :
        public outdated_index_report_t,
        public home_thread_mixin_t {
public:
    outdated_index_report_impl_t(outdated_index_issue_client_t *_parent,
                                 namespace_id_t _ns_id) :
        parent(_parent),
        ns_id(_ns_id) { }

    ~outdated_index_report_impl_t() { }

    void set_outdated_indexes(std::set<std::string> &&indexes) {
        assert_thread();
        coro_t::spawn_sometime(
            std::bind(&outdated_index_issue_client_t::log_outdated_indexes,
                      parent, ns_id, indexes, auto_drainer_t::lock_t(&drainer)));
        (*parent->outdated_indexes.get())[ns_id] = std::move(indexes);
    }

    void index_dropped(const std::string &index_name) {
        assert_thread();
        (*parent->outdated_indexes.get())[ns_id].erase(index_name);
    }

    void index_renamed(const std::string &old_name,
                       const std::string &new_name) {
        assert_thread();
        std::set<std::string> &ns_set = (*parent->outdated_indexes.get())[ns_id];

        if (ns_set.find(old_name) != ns_set.end()) {
            ns_set.erase(old_name);
            ns_set.insert(new_name);
        }
    }

    void destroy() {
        assert_thread();
        parent->outdated_indexes.get()->erase(ns_id);
        parent->destroy_report(this);
    }

private:
    outdated_index_issue_client_t *parent;
    namespace_id_t ns_id;
    auto_drainer_t drainer;

    DISABLE_COPYING(outdated_index_report_impl_t);
};

outdated_index_issue_client_t::outdated_index_issue_client_t(
        mailbox_manager_t *_mailbox_manager,
        const clone_ptr_t<watchable_t<change_tracking_map_t<peer_id_t,
                                                            request_address_t> > >
            &_outdated_index_mailboxes) :
    mailbox_manager(_mailbox_manager),
    outdated_index_mailboxes(_outdated_index_mailboxes)
{ }

outdated_index_issue_client_t::~outdated_index_issue_client_t() { }

outdated_index_report_t *outdated_index_issue_client_t::create_report(
        const namespace_id_t &ns_id) {
    outdated_index_report_impl_t *new_report = new outdated_index_report_impl_t(this, ns_id);
    index_reports.get()->insert(new_report);
    return new_report;
}

void outdated_index_issue_client_t::destroy_report(outdated_index_report_impl_t *report) {
    guarantee(index_reports.get()->erase(report) == 1);
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
    return res;
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
            [&](signal_t *, const outdated_index_map_t &resp) {
                maps_out->at(peer_offset) = resp;
                response_done.pulse();
            });

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

void outdated_index_issue_client_t::log_outdated_indexes(namespace_id_t ns_id,
        std::set<std::string> indexes,
        UNUSED auto_drainer_t::lock_t keepalive) {
    on_thread_t rethreader(home_thread());
    if (indexes.size() > 0 &&
        logged_namespaces.find(ns_id) == logged_namespaces.end()) {

        std::string index_list;
        for (auto const &s : indexes) {
            index_list += (index_list.empty() ? "'" : ", '") + s + "'";
        }

        logWRN("Namespace %s contains these outdated indexes which should be "
               "recreated: %s", uuid_to_str(ns_id).c_str(), index_list.c_str());

        logged_namespaces.insert(ns_id);
    }
}
