// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_AUTH_BASE_ARTIFICIAL_TABLE_BACKEND_HPP
#define CLUSTERING_ADMINISTRATION_AUTH_BASE_ARTIFICIAL_TABLE_BACKEND_HPP

#include <memory>
#include <string>
#include <vector>

#include "clustering/administration/metadata.hpp"
#include "clustering/table_manager/table_meta_client.hpp"
#include "rdb_protocol/artificial_table/caching_cfeed_backend.hpp"

namespace auth {

class base_artificial_table_backend_t :
    public caching_cfeed_artificial_table_backend_t
{
public:
    base_artificial_table_backend_t(
            std::shared_ptr<semilattice_readwrite_view_t<auth_semilattice_metadata_t>>
                auth_semilattice_view,
            std::shared_ptr<semilattice_read_view_t<cluster_semilattice_metadata_t>>
                cluster_semilattice_view)
        : m_auth_semilattice_view(std::move(auth_semilattice_view)),
          m_auth_subscription([this](){ notify_all(); }, m_auth_semilattice_view),
          m_cluster_semilattice_view(std::move(cluster_semilattice_view)) {
    }

    ~base_artificial_table_backend_t() {
        begin_changefeed_destruction();
    }

    std::string get_primary_key_name() {
        return "id";
    }

protected:
    std::shared_ptr<semilattice_readwrite_view_t<auth_semilattice_metadata_t>>
        m_auth_semilattice_view;
    semilattice_read_view_t<auth_semilattice_metadata_t>::subscription_t
        m_auth_subscription;
    std::shared_ptr<semilattice_read_view_t<cluster_semilattice_metadata_t>>
        m_cluster_semilattice_view;
};

}  // namespace auth

#endif  // CLUSTERING_ADMINISTRATION_AUTH_BASE_ARTIFICIAL_TABLE_BACKEND_HPP
