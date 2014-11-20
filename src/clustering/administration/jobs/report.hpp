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
            uuid_u const &table = nil_uuid(),
            std::string const &index = "");

    ql::datum_t to_datum(
            admin_identifier_format_t identifier_format,
            server_name_client_t *name_client,
            cluster_semilattice_metadata_t const &metadata) const;

    // Unfortunately these a required to be public in order for them to be serialized.
    uuid_u id;
    std::string type;
    double duration;
    uuid_u table;
    std::string index;

    // `servers` is used in `backend.cc` to aggregate the same job running on multiple
    // machines.
    std::set<uuid_u> servers;
};
RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(job_report_t);

class sindex_job_t {
public:
    sindex_job_t(microtime_t start_time);

    microtime_t start_time;
};

#endif /* CLUSTERING_ADMINISTRATION_JOBS_REPORT_HPP_ */
