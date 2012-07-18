#include <string>
#include <set>

#include "errors.hpp"
#include <boost/ptr_container/ptr_map.hpp>

#include "stl_utils.hpp"
#include "arch/timing.hpp"
#include "clustering/administration/http/stat_app.hpp"
#include "clustering/administration/stat_manager.hpp"
#include "http/json.hpp"
#include "perfmon/perfmon.hpp"
#include "perfmon/archive.hpp"
#include "rpc/mailbox/mailbox.hpp"
#include "clustering/administration/main/watchable_fields.hpp"

static const char * STAT_REQ_TIMEOUT_PARAM = "timeout";
static const uint64_t DEFAULT_STAT_REQ_TIMEOUT_MS = 1000;
static const uint64_t MAX_STAT_REQ_TIMEOUT_MS = 60*1000;

class stats_request_record_t {
public:
    explicit stats_request_record_t(mailbox_manager_t *mbox_manager)
        : response_mailbox(mbox_manager, boost::bind(&promise_t<perfmon_result_t>::pulse, &stats, _1), mailbox_callback_mode_inline)
    { }
    promise_t<perfmon_result_t> stats;
    mailbox_t<void(perfmon_result_t)> response_mailbox;
};

stat_http_app_t::stat_http_app_t(mailbox_manager_t *_mbox_manager,
                                 clone_ptr_t<watchable_t<std::map<peer_id_t, cluster_directory_metadata_t> > >& _directory,
                                 boost::shared_ptr<semilattice_readwrite_view_t<cluster_semilattice_metadata_t> >& _semilattice
                                 )
    : mbox_manager(_mbox_manager), directory(_directory), semilattice(_semilattice)
{ }

template <class ctx_t>
cJSON *render_as_json(perfmon_result_t *target, ctx_t ctx) {
    if (target->is_map()) {
        cJSON *res = cJSON_CreateObject();

        for (perfmon_result_t::iterator it  = target->begin(); it != target->end(); ++it) {
            cJSON_AddItemToObject(res, it->first.c_str(), render_as_json(it->second, ctx));
        }

        return res;
    } else if (target->is_string()) {
        return render_as_json(target->get_string(), ctx);
    } else {
        crash("Unknown perfmon_result_type\n");
    }
}

cJSON *stat_http_app_t::prepare_machine_info(const std::vector<machine_id_t> &not_replied) {
    scoped_cJSON_t machines(cJSON_CreateObject());

    scoped_cJSON_t all_known(cJSON_CreateArray());
    scoped_cJSON_t dead(cJSON_CreateArray());
    scoped_cJSON_t ghosts(cJSON_CreateArray());
    scoped_cJSON_t timed_out(cJSON_CreateArray());

    std::map<peer_id_t,machine_id_t> peer_id_to_machine_id(directory->subview(
            field_getter_t<machine_id_t, cluster_directory_metadata_t>(
                &cluster_directory_metadata_t::machine_id
            ))->get());
    std::map<machine_id_t,peer_id_t> machine_id_to_peer_id(invert_bijection_map(peer_id_to_machine_id));

    machines_semilattice_metadata_t::machine_map_t machines_ids = semilattice->get().machines.machines;
    for (machines_semilattice_metadata_t::machine_map_t::const_iterator it = machines_ids.begin(); it != machines_ids.end(); it++) {
        const machine_id_t &machine_id = it->first;
        bool peer_exists = machine_id_to_peer_id.count(machine_id) != 0;

        if (!it->second.is_deleted() && !peer_exists) {
            // machine is dead
            cJSON_AddItemToArray(dead.get(), cJSON_CreateString(uuid_to_str(machine_id).c_str()));
        } else if (it->second.is_deleted() && peer_exists) {
            // machine is a ghost
            cJSON_AddItemToArray(ghosts.get(), cJSON_CreateString(uuid_to_str(machine_id).c_str()));
        }
        cJSON_AddItemToArray(all_known.get(), cJSON_CreateString(uuid_to_str(machine_id).c_str()));
    }

    for (std::vector<machine_id_t>::const_iterator it = not_replied.begin(); it != not_replied.end(); ++it) {
        cJSON_AddItemToArray(timed_out.get(), cJSON_CreateString(uuid_to_str(*it).c_str()));
    }

    cJSON_AddItemToObject(machines.get(), "known", all_known.release());
    cJSON_AddItemToObject(machines.get(), "dead", dead.release());
    cJSON_AddItemToObject(machines.get(), "ghosts", ghosts.release());
    cJSON_AddItemToObject(machines.get(), "timed_out", timed_out.release());
    return machines.release();
}

http_res_t stat_http_app_t::handle(const http_req_t &req) {
    scoped_cJSON_t body(cJSON_CreateObject());

    peers_to_metadata_t peers_to_metadata = directory->get();

    typedef boost::ptr_map<machine_id_t, stats_request_record_t> stats_promises_t;
    stats_promises_t stats_promises;

    // Parse the 'timeout' query parameter
    boost::optional<std::string> timeout_param = req.find_query_param(STAT_REQ_TIMEOUT_PARAM);
    uint64_t timeout = DEFAULT_STAT_REQ_TIMEOUT_MS;
    if (timeout_param) {
        if (!strtou64_strict(timeout_param.get(), 10, &timeout) || timeout == 0 || timeout > MAX_STAT_REQ_TIMEOUT_MS) {
            http_res_t res(400);
            res.set_body("application/text", "Invalid timeout value");
            return res;
        }
    }

    /* If a machine has disconnected, or the mailbox for the
     * get_stat function  has gone out of existence we'll never get a response.
     * Thus we need to have a time out.
     */
    signal_timer_t timer(static_cast<int>(timeout)); // WTF? why is it accepting an int? negative milliseconds, anyone?

    for (peers_to_metadata_t::iterator it  = peers_to_metadata.begin();
                                       it != peers_to_metadata.end();
                                       ++it) {
        machine_id_t machine = it->second.machine_id; //due to boost bug with not accepting const keys for insert
        stats_request_record_t *req_record = new stats_request_record_t(mbox_manager);
        stats_promises.insert(machine, req_record);
        send(mbox_manager, it->second.get_stats_mailbox_address, req_record->response_mailbox.get_address(), std::set<stat_manager_t::stat_id_t>());
    }

    std::vector<machine_id_t> not_replied;
    for (stats_promises_t::iterator it = stats_promises.begin(); it != stats_promises.end(); ++it) {
        machine_id_t machine = it->first;

        signal_t * stats_ready = it->second->stats.get_ready_signal();
        wait_any_t waiter(&timer, stats_ready);
        waiter.wait();

        if (stats_ready->is_pulsed()) {
            perfmon_result_t stats = it->second->stats.wait();
            body.AddItemToObject(uuid_to_str(machine).c_str(), render_as_json(&stats, 0));
        } else {
            not_replied.push_back(machine);
        }
    }

    cJSON_AddItemToObject(body.get(), "machines", prepare_machine_info(not_replied));

    http_res_t res(200);
    res.set_body("application/json", body.Print());
    return res;
}
