// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_JOBS_REPORT_HPP_
#define CLUSTERING_ADMINISTRATION_JOBS_REPORT_HPP_

#include <string>

#include "arch/address.hpp"
#include "btree/secondary_operations.hpp"
#include "clustering/administration/datum_adapter.hpp"
#include "concurrency/signal.hpp"
#include "containers/archive/stl_types.hpp"
#include "containers/uuid.hpp"
#include "rdb_protocol/datum.hpp"
#include "rpc/serialize_macros.hpp"
#include "time.hpp"

bool convert_job_type_and_id_from_datum(ql::datum_t primary_key,
                                        std::string *type_out,
                                        uuid_u *id_out);

ql::datum_t convert_job_type_and_id_to_datum(std::string const &type, uuid_u const &id);

class job_report_t {
public:
    job_report_t();

    // For `"query"` jobs, taking a `ip_and_port_t` of the client issuing the query
    job_report_t(
            uuid_u const &id,
            std::string const &type,
            double duration,
            ip_and_port_t const &client_addr_port);

    // For `"backfill"` jobs, taking extra variables for progress, source, and
    // destination servers
    job_report_t(
            uuid_u const &id,
            std::string const &type,
            double duration,
            namespace_id_t const &table,
            bool is_ready,
            double progress,
            peer_id_t const &source_peer,
            server_id_t const &destination_server);

    // For `"disk_compation"` and `"index_construction"` jobs
    job_report_t(
            uuid_u const &id,
            std::string const &type,
            double duration,
            namespace_id_t const &table = nil_uuid(),
            std::string const &index = "");

    bool to_datum(
            admin_identifier_format_t identifier_format,
            server_config_client_t *server_config_client,
            table_meta_client_t *table_meta_client,
            cluster_semilattice_metadata_t const &metadata,
            ql::datum_t *row_out) const;

    uuid_u id;
    std::string type;
    double duration;
    ip_and_port_t client_addr_port;
    namespace_id_t table;
    std::string index;
    bool is_ready;
    double progress_numerator, progress_denominator;
    peer_id_t source_peer;
    server_id_t destination_server;

    // `servers` is used in `backend.cc` to aggregate the same job running on multiple
    // machines.
    std::set<server_id_t> servers;
};
RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(job_report_t);

class query_job_t {
public:
    query_job_t(
            microtime_t _start_time,
            ip_and_port_t const &_client_addr_port,
            cond_t *interruptor);

    microtime_t start_time;
    ip_and_port_t client_addr_port;
    cond_t *interruptor;
};

#endif /* CLUSTERING_ADMINISTRATION_JOBS_REPORT_HPP_ */
