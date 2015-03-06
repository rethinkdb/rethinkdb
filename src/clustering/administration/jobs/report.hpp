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

// `job_report_base_t` is a CRTP base class allowing static polymorphism without virtual
// functions, see http://en.wikipedia.org/wiki/Curiously_recurring_template_pattern.
template <typename T>
class job_report_base_t {
public:
    job_report_base_t();
    job_report_base_t(
            std::string const &type,
            uuid_u const &id,
            double duration,
            server_id_t const &server_id);

    void merge(T const & job_report);

    bool to_datum(
            admin_identifier_format_t identifier_format,
            server_config_client_t *server_config_client,
            cluster_semilattice_metadata_t const &metadata,
            ql::datum_t *row_out) const;

    // `type` can be derived from the derived type, but having it as a string is easier.
    std::string type;
    uuid_u id;
    double duration;
    std::set<server_id_t> servers;
};

class backfill_job_report_t : public job_report_base_t<backfill_job_report_t> {
public:
    backfill_job_report_t();
    backfill_job_report_t(
            uuid_u const &id,
            double duration,
            server_id_t const &server_id,
            namespace_id_t const &table,
            bool is_ready,
            double progress,
            server_id_t const &source_server,
            server_id_t const &destination_server);

    void merge_derived(backfill_job_report_t const &job_report);

    bool info_derived(
            admin_identifier_format_t identifier_format,
            server_config_client_t *server_config_client,
            cluster_semilattice_metadata_t const &metadata,
            ql::datum_object_builder_t *info_builder_out) const;

    namespace_id_t table;
    bool is_ready;
    double progress_numerator;
    double progress_denominator;
    server_id_t source_server;
    server_id_t destination_server;
};
RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(backfill_job_report_t);

class disk_compaction_job_report_t
    : public job_report_base_t<disk_compaction_job_report_t> {
public:
    disk_compaction_job_report_t();
    disk_compaction_job_report_t(
            uuid_u const &id,
            double duration,
            server_id_t const &server_id);

    void merge_derived(disk_compaction_job_report_t const &job_report);

    bool info_derived(
            admin_identifier_format_t identifier_format,
            server_config_client_t *server_config_client,
            cluster_semilattice_metadata_t const &metadata,
            ql::datum_object_builder_t *info_builder_out) const;
};
RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(disk_compaction_job_report_t);

class index_construction_job_report_t
    : public job_report_base_t<index_construction_job_report_t> {
public:
    index_construction_job_report_t();
    index_construction_job_report_t(
            uuid_u const &id,
            double duration,
            server_id_t const &server_id,
            namespace_id_t const &table,
            std::string const &index,
            bool is_ready,
            double progress);

    void merge_derived(index_construction_job_report_t const &job_report);

    bool info_derived(
            admin_identifier_format_t identifier_format,
            server_config_client_t *server_config_client,
            cluster_semilattice_metadata_t const &metadata,
            ql::datum_object_builder_t *info_builder_out) const;

    namespace_id_t table;
    std::string index;
    bool is_ready;
    double progress_numerator;
    double progress_denominator;
};
RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(index_construction_job_report_t);

class query_job_report_t : public job_report_base_t<query_job_report_t> {
public:
    query_job_report_t();
    query_job_report_t(
            uuid_u const &id,
            double duration,
            server_id_t const &server_id,
            ip_and_port_t const &client_addr_port,
            std::string const &query);

    void merge_derived(query_job_report_t const &job_report);

    bool info_derived(
            admin_identifier_format_t identifier_format,
            server_config_client_t *server_config_client,
            cluster_semilattice_metadata_t const &metadata,
            ql::datum_object_builder_t *info_builder_out) const;

    ip_and_port_t client_addr_port;
    std::string query;
};
RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(query_job_report_t);

#include "clustering/administration/jobs/report.tcc"

#endif /* CLUSTERING_ADMINISTRATION_JOBS_REPORT_HPP_ */
