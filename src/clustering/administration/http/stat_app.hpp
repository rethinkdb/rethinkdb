#ifndef CLUSTERING_ADMINISTRATION_HTTP_STAT_APP_HPP_
#define CLUSTERING_ADMINISTRATION_HTTP_STAT_APP_HPP_

#include <map>
#include <vector>

#include "clustering/administration/metadata.hpp"
#include "containers/clone_ptr.hpp"
#include "http/http.hpp"

template <class> class watchable_t;

class stat_http_app_t : public http_app_t {
public:
    stat_http_app_t(mailbox_manager_t *_mbox_manager,
                    clone_ptr_t<watchable_t<std::map<peer_id_t, cluster_directory_metadata_t> > >& _directory,
                    boost::shared_ptr<semilattice_readwrite_view_t<cluster_semilattice_metadata_t> >& _semilattice);
    http_res_t handle(const http_req_t &req);

private:
    typedef std::map<peer_id_t, cluster_directory_metadata_t> peers_to_metadata_t;

    cJSON *prepare_machine_info(const std::vector<machine_id_t> &not_replied);

private:
    mailbox_manager_t *mbox_manager;
    clone_ptr_t<watchable_t<std::map<peer_id_t, cluster_directory_metadata_t> > > directory;
    boost::shared_ptr<semilattice_readwrite_view_t<cluster_semilattice_metadata_t> > semilattice;

    DISABLE_COPYING(stat_http_app_t);
};

#endif /* CLUSTERING_ADMINISTRATION_HTTP_STAT_APP_HPP_ */

