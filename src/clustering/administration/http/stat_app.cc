#include <string>
#include <set>

#include "errors.hpp"
#include <boost/ptr_container/ptr_map.hpp>

#include "clustering/administration/http/stat_app.hpp"
#include "clustering/administration/stat_manager.hpp"
#include "http/json.hpp"
#include "perfmon.hpp"
#include "rpc/mailbox/mailbox.hpp"

class stats_request_record_t {
public:
    stats_request_record_t(mailbox_manager_t *mbox_manager)
        : response_mailbox(mbox_manager, boost::bind(&promise_t<stat_manager_t::stats_t>::pulse, &stats, _1))
    { }
    promise_t<stat_manager_t::stats_t> stats;
    mailbox_t<void(stat_manager_t::stats_t)> response_mailbox;
};

typedef std::map<peer_id_t, cluster_directory_metadata_t> peers_to_metadata_t;

stat_http_app_t::stat_http_app_t(mailbox_manager_t *_mbox_manager,
                                 clone_ptr_t<watchable_t<std::map<peer_id_t, cluster_directory_metadata_t> > >& _directory)
    : mbox_manager(_mbox_manager), directory(_directory) 
{ }

http_res_t stat_http_app_t::handle(const http_req_t &) {
    scoped_cJSON_t body(cJSON_CreateObject());

   peers_to_metadata_t peers_to_metadata = directory->get();

    typedef boost::ptr_map<machine_id_t, stats_request_record_t> stats_promises_t;
    stats_promises_t stats_promises;

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
        stat_manager_t::stats_t stats = it->second->stats.wait();
        cJSON_AddItemToObject(body.get(), uuid_to_str(machine).c_str(), render_as_json(&stats, 0));
    }

    http_res_t res(200);
    res.set_body("application/json", cJSON_print_std_string(body.get()));
    return res;
}
