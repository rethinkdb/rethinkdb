// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_JOBS_REPORT_HPP_
#define CLUSTERING_ADMINISTRATION_JOBS_REPORT_HPP_

#include <string>

#include "btree/secondary_operations.hpp"
#include "clustering/administration/datum_adapter.hpp"
#include "containers/archive/stl_types.hpp"
#include "containers/uuid.hpp"
#include "rdb_protocol/datum.hpp"
#include "rpc/serialize_macros.hpp"
#include "time.hpp"

class job_report_t {
public:
    job_report_t();
    job_report_t(
            uuid_u const &id,
            std::string const &type,
            double duration,
            namespace_id_t const &table = nil_uuid(),
            std::string const &index = "");

    bool to_datum(
            admin_identifier_format_t identifier_format,
            server_config_client_t *server_config_client,
            cluster_semilattice_metadata_t const &metadata,
            ql::datum_t *row_out) const;

    uuid_u id;
    std::string type;
    double duration;
    namespace_id_t table;
    std::string index;

    // `servers` is used in `backend.cc` to aggregate the same job running on multiple
    // machines.
    std::set<server_id_t> servers;
};
RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(job_report_t);

#endif /* CLUSTERING_ADMINISTRATION_JOBS_REPORT_HPP_ */
