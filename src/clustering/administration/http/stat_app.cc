#include <string>
#include <set>

#include "errors.hpp"
#include <boost/ptr_container/ptr_map.hpp>

#include "arch/timing.hpp"
#include "clustering/administration/http/stat_app.hpp"
#include "clustering/administration/stat_manager.hpp"
#include "http/json.hpp"
#include "perfmon/perfmon.hpp"
#include "perfmon/perfmon_archive.hpp"
#include "rpc/mailbox/mailbox.hpp"

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

typedef std::map<peer_id_t, cluster_directory_metadata_t> peers_to_metadata_t;

stat_http_app_t::stat_http_app_t(mailbox_manager_t *_mbox_manager,
                                 clone_ptr_t<watchable_t<std::map<peer_id_t, cluster_directory_metadata_t> > >& _directory)
    : mbox_manager(_mbox_manager), directory(_directory)
{ }

template <class ctx_t>
cJSON *render_as_json(perfmon_result_t *target, ctx_t ctx) {
    if (target->is_map()) {
        cJSON *res = cJSON_CreateObject();
        
        for (perfmon_result_t::iterator it  = target->begin();
                                        it != target->end();
                                        ++it) {
            cJSON_AddItemToObject(res, it->first.c_str(), render_as_json(it->second, ctx));
        }

        return res;
    } else if (target->is_string()) {
        return render_as_json(target->get_string(), ctx);
    } else {
        crash("Unknown perfmon_result_type\n");
    }
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
        machine_id_t tmp = it->second.machine_id; //due to boost bug with not accepting const keys for insert
        stats_request_record_t *req_record = new stats_request_record_t(mbox_manager);
        stats_promises.insert(tmp, req_record);
        send(mbox_manager, it->second.get_stats_mailbox_address, req_record->response_mailbox.get_address(), std::set<stat_manager_t::stat_id_t>());
    }

    for (stats_promises_t::iterator it = stats_promises.begin(); it != stats_promises.end(); ++it) {
        machine_id_t machine = it->first;

        signal_t * stats_ready = it->second->stats.get_ready_signal();
        wait_any_t waiter(&timer, stats_ready);
        waiter.wait();

        if (stats_ready->is_pulsed()) {
            perfmon_result_t stats = it->second->stats.wait();
            cJSON_AddItemToObject(body.get(), uuid_to_str(machine).c_str(), render_as_json(&stats, 0));
        }
    }

    http_res_t res(200);
    res.set_body("application/json", cJSON_print_std_string(body.get()));
    return res;
}
