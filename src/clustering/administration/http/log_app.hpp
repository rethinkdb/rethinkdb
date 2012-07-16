#ifndef CLUSTERING_ADMINISTRATION_HTTP_LOG_APP_HPP_
#define CLUSTERING_ADMINISTRATION_HTTP_LOG_APP_HPP_

#include <map>
#include <vector>

#include "clustering/administration/log_transfer.hpp"
#include "clustering/administration/machine_metadata.hpp"
#include "http/http.hpp"

class log_http_app_t : public http_app_t {
public:
    log_http_app_t(
            mailbox_manager_t *mm,
            const clone_ptr_t<watchable_t<std::map<peer_id_t, log_server_business_card_t> > > &log_mailbox_view,
            const clone_ptr_t<watchable_t<std::map<peer_id_t, machine_id_t> > > &machine_id_translation_table);

    http_res_t handle(const http_req_t &req);

private:
    void fetch_logs(int i,
            const std::vector<machine_id_t> &machines, const std::vector<peer_id_t> &peers,
            int max_messages, struct timespec min_timestamp, struct timespec max_timestamp,
            cJSON *map_to_fill,
            signal_t *interruptor) THROWS_NOTHING;

    mailbox_manager_t *mailbox_manager;
    clone_ptr_t<watchable_t<std::map<peer_id_t, log_server_business_card_t> > > log_mailbox_view;
    clone_ptr_t<watchable_t<std::map<peer_id_t, machine_id_t> > > machine_id_translation_table;

    DISABLE_COPYING(log_http_app_t);
};

#endif /* CLUSTERING_ADMINISTRATION_HTTP_LOG_APP_HPP_ */
