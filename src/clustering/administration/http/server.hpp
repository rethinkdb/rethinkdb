// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_HTTP_SERVER_HPP_
#define CLUSTERING_ADMINISTRATION_HTTP_SERVER_HPP_

#include <map>
#include <set>
#include <string>

#include "arch/io/openssl.hpp"
#include "clustering/administration/metadata.hpp"
#include "clustering/administration/namespace_interface_repository.hpp"
#include "http/http.hpp"
#include "rpc/semilattice/view.hpp"

class http_server_t;
class routing_http_app_t;
class file_http_app_t;
class cyanide_http_app_t;

class real_reql_cluster_interface_t;

class administrative_http_server_manager_t {

public:
    administrative_http_server_manager_t(
        const std::set<ip_address_t> &local_addresses,
        int port,
        http_app_t *reql_app,
        std::string _path,
        tls_ctx_t *tls_ctx);
    ~administrative_http_server_manager_t();

    int get_port() const;
private:

    scoped_ptr_t<file_http_app_t> file_app;
#ifndef NDEBUG
    scoped_ptr_t<cyanide_http_app_t> cyanide_app;
#endif
    scoped_ptr_t<routing_http_app_t> ajax_routing_app;
    scoped_ptr_t<routing_http_app_t> root_routing_app;
    scoped_ptr_t<http_server_t> server;

    DISABLE_COPYING(administrative_http_server_manager_t);  // kind of redundant with the scoped_ptrs but too bad.
};

#endif /* CLUSTERING_ADMINISTRATION_HTTP_SERVER_HPP_ */
