#include "clustering/administration/issues/table.hpp"

#include <map>

#include "clustering/administration/datum_adapter.hpp"
#include "clustering/administration/tables/calculate_status.hpp"
#include "clustering/administration/tables/table_metadata.hpp"
#include "clustering/administration/tables/table_status.hpp"
#include "clustering/table_manager/table_meta_client.hpp"

class table_availability_issue_t : public issue_t {
public:
    table_availability_issue_t(const namespace_id_t &_table_id,
                               table_readiness_t _readiness,
                               std::vector<shard_status_t> _shard_statuses);
    ~table_availability_issue_t();

    bool is_critical() const { return readiness < table_readiness_t::writes; };
    const datum_string_t &get_name() const { return table_availability_issue_type; }

private:
    static const datum_string_t table_availability_issue_type;
    static const uuid_u base_issue_id;

    bool build_info_and_description(
        const metadata_t &metadata,
        server_config_client_t *server_config_client,
        table_meta_client_t *table_meta_client,
        admin_identifier_format_t identifier_format,
        ql::datum_t *info_out,
        datum_string_t *description_out) const;

    const namespace_id_t table_id;
    const table_readiness_t readiness;
    const std::vector<shard_status_t> shard_statuses;
};

table_availability_issue_t::table_availability_issue_t(
        const namespace_id_t &_table_id,
        table_readiness_t _readiness,
        std::vector<shard_status_t> _shard_statuses) :
    issue_t(from_hash(base_issue_id, _table_id)),
    table_id(_table_id),
    readiness(_readiness),
    shard_statuses(std::move(_shard_statuses)) { }

table_availability_issue_t::~table_availability_issue_t() { }

bool table_availability_issue_t::build_info_and_description(
        const metadata_t &metadata,
        server_config_client_t *server_config_client,
        table_meta_client_t *table_meta_client,
        admin_identifier_format_t identifier_format,
        ql::datum_t *info_out,
        datum_string_t *description_out) const {
    ql::datum_t db_id_datum;
    name_string_t db_name;
    ql::datum_t table_id_datum;
    name_string_t table_name;
    if (!convert_table_id_to_datums(table_id, identifier_format, metadata,
                                    table_meta_client, &table_id_datum, &table_name,
                                    &db_id_datum, &db_name)) {
        return false;
    }

    ql::datum_t table_status_datum =
        convert_table_status_to_datum(table_id, table_name, db_id_datum,
                                      readiness, shard_statuses,
                                      identifier_format, server_config_client);

    // Convert table id/name into the requested identifier format
    ql::datum_object_builder_t builder(table_status_datum);
    UNUSED bool b = builder.delete_field("id");
    UNUSED bool c = builder.delete_field("name");
    builder.overwrite("table", table_id_datum);

    *info_out = std::move(builder).to_datum();

    // Provide a textual description of the problem as well
    std::string text = strprintf("Table `%s.%s` is ",
                                 db_name.c_str(), table_name.c_str());
    switch (readiness) {
    case table_readiness_t::unavailable:
        text += "not available for any operations.";
        break;
    case table_readiness_t::outdated_reads:
        text += "available for outdated reads, but not up-to-date reads or writes.";
        break;
    case table_readiness_t::reads:
        text += "available for outdated and up-to-date reads, but not writes.";
        break;
    case table_readiness_t::writes:
        text += "available for all operations, but some replicas are not ready.";
        break;
    case table_readiness_t::finished: // fallthrough intentional
    default:
        // This shouldn't happen, but it's easy to ignore
        return false;
    }

    // Collect a set of servers that have replicas for this table that aren't ready
    std::map<server_id_t, bool> servers;
    for (auto const &shard : shard_statuses) {
        for (auto const &pair : shard.replicas) {
            if (pair.second != server_status_t::READY) {
                bool is_primary = (shard.primary_replicas.count(pair.first) != 0);
                auto it = servers.find(pair.first);
                if (it == servers.end()) {
                    servers.insert(std::make_pair(pair.first, is_primary));
                } else {
                    it->second = it->second || is_primary;
                }
            }
        }
    }

    // Print out a list of servers with affected shards and their most important role
    if (servers.size() > 0) {
        text += "\nThe servers that are not ready are:";
        for (auto const &pair : servers) {
            name_string_t server_name;
            if (!convert_server_id_to_datum(pair.first, identifier_format,
                                            server_config_client, nullptr, &server_name)) {
                continue;
            }
            text += strprintf("\n %s (%s replica)",
                              server_name.c_str(),
                              pair.second ? "primary" : "secondary");
        }
    }
    *description_out = datum_string_t(text);
    return true;
}

const datum_string_t table_availability_issue_t::table_availability_issue_type =
    datum_string_t("table_availability");
const uuid_u table_availability_issue_t::base_issue_id =
    str_to_uuid("4ae8c575-b103-3335-a79a-121089e34339");

table_issue_tracker_t::table_issue_tracker_t(
        server_config_client_t *_server_config_client,
        table_meta_client_t *_table_meta_client) :
    server_config_client(_server_config_client),
    table_meta_client(_table_meta_client) { }

table_issue_tracker_t::~table_issue_tracker_t() { }

std::vector<scoped_ptr_t<issue_t> > table_issue_tracker_t::get_issues(
        signal_t *interruptor) const {
    std::map<namespace_id_t, table_basic_config_t> table_names;
    table_meta_client->list_names(&table_names);

    // Convert to a vector of ids so we can index it in pmap
    std::vector<namespace_id_t> table_ids;
    table_ids.reserve(table_names.size());
    for (auto const &pair : table_names) {
        table_ids.push_back(pair.first);
    }

    std::vector<scoped_ptr_t<issue_t> > issues;
    throttled_pmap(table_ids.size(), [&] (int64_t i) {
                       check_table(table_ids[i], &issues, interruptor);
                   }, 16);
    return issues;
}

void table_issue_tracker_t::check_table(const namespace_id_t &table_id,
                                        std::vector<scoped_ptr_t<issue_t> > *issues_out,
                                        signal_t *interruptor) const {
    table_readiness_t readiness = table_readiness_t::finished;
    std::vector<shard_status_t> shard_statuses;

    try {
        calculate_status(table_id, interruptor, table_meta_client,
                         server_config_client, &readiness, &shard_statuses);
    } catch (const no_such_table_exc_t &) {
        // Ignore any tables that have stopped existing
    }

    if (readiness != table_readiness_t::finished) {
        issues_out->emplace_back(make_scoped<table_availability_issue_t>(
            table_id, readiness, std::move(shard_statuses)));
    }
}
