// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_HTTP_DISTRIBUTION_APP_HPP_
#define CLUSTERING_ADMINISTRATION_HTTP_DISTRIBUTION_APP_HPP_

#include "errors.hpp"
#include <boost/shared_ptr.hpp>

#include "clustering/administration/namespace_interface_repository.hpp"
#include "clustering/administration/namespace_metadata.hpp"
#include "http/http.hpp"
#include "rpc/semilattice/view.hpp"


class distribution_app_t : public http_app_t {
public:
    distribution_app_t(boost::shared_ptr<semilattice_read_view_t<cow_ptr_t<namespaces_semilattice_metadata_t> > >, namespace_repo_t *);
    void handle(const http_req_t &, http_res_t *result, signal_t *interruptor);

private:
    boost::shared_ptr<semilattice_read_view_t<cow_ptr_t<namespaces_semilattice_metadata_t> > > rdb_namespaces_sl_metadata;
    namespace_repo_t *rdb_ns_repo;

    DISABLE_COPYING(distribution_app_t);
};

#endif /* CLUSTERING_ADMINISTRATION_HTTP_DISTRIBUTION_APP_HPP_ */
