#ifndef CLUSTERING_ADMINISTRATION_HTTP_STAT_APP_HPP_
#define CLUSTERING_ADMINISTRATION_HTTP_STAT_APP_HPP_

#include <map>

#include "clustering/administration/metadata.hpp"
#include "concurrency/watchable.hpp"
#include "containers/clone_ptr.hpp"
#include "http/http.hpp"

class stat_http_app_t : public http_app_t {
public:
    stat_http_app_t(mailbox_manager_t *_mbox_manager,
                    clone_ptr_t<watchable_t<std::map<peer_id_t, cluster_directory_metadata_t> > >& _directory);
    http_res_t handle(const http_req_t &req);

private:
    mailbox_manager_t *mbox_manager;
    clone_ptr_t<watchable_t<std::map<peer_id_t, cluster_directory_metadata_t> > > directory;

    DISABLE_COPYING(stat_http_app_t);
};

#endif /* CLUSTERING_ADMINISTRATION_HTTP_STAT_APP_HPP_ */

