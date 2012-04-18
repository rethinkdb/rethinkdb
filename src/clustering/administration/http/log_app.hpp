#ifndef CLUSTERING_ADMINISTRATION_HTTP_LOG_APP_HPP_
#define CLUSTERING_ADMINISTRATION_HTTP_LOG_APP_HPP_

#include "clustering/administration/logger.hpp"
#include "clustering/administration/machine_metadata.hpp"
#include "http/http.hpp"

class log_http_app_t : public http_app_t {
public:
    log_http_app_t(
            mailbox_manager_t *mm,
            const clone_ptr_t<watchable_t<std::map<peer_id_t, mailbox_addr_t<void(mailbox_addr_t<void(std::vector<std::string>)>)> > > > &log_mailbox_view,
            const clone_ptr_t<watchable_t<std::map<peer_id_t, machine_id_t> > > &machine_id_translation_table);

    http_res_t handle(const http_req_t &req);

private:
    mailbox_manager_t *mailbox_manager;
    clone_ptr_t<watchable_t<std::map<peer_id_t, mailbox_addr_t<void(mailbox_addr_t<void(std::vector<std::string>)>)> > > > log_mailbox_view;
    clone_ptr_t<watchable_t<std::map<peer_id_t, machine_id_t> > > machine_id_translation_table;
};

#endif /* CLUSTERING_ADMINISTRATION_HTTP_LOG_APP_HPP_ */
