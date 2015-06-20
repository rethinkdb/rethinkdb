#include "clustering/administration/issues/table.hpp"

#include <map>

#include "clustering/administration/datum_adapter.hpp"
#include "clustering/administration/tables/table_metadata.hpp"
#include "clustering/administration/tables/table_status.hpp"
#include "clustering/table_manager/table_meta_client.hpp"

class table_availability_issue_t : public issue_t {
public:
    table_availability_issue_t(
            const namespace_id_t &_table_id,
            table_status_t &&_status) :
        issue_t(from_hash(base_issue_id, _table_id)),
        table_id(_table_id), status(std::move(_status)) { }

    bool is_critical() const { return status.readiness < table_readiness_t::writes; }
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
    const table_status_t status;
};

bool table_availability_issue_t::build_info_and_description(
        const metadata_t &metadata,
        UNUSED server_config_client_t *server_config_client,
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

    ql::datum_t status_datum = convert_table_status_to_datum(status, identifier_format);
    ql::datum_object_builder_t builder(status_datum);
    builder.overwrite("table", table_id_datum);
    *info_out = std::move(builder).to_datum();

    // Provide a textual description of the problem as well
    if (!status.total_loss) {
        const char *blurb;
        switch (status.readiness) {
        case table_readiness_t::unavailable:
            blurb = "not available for any operations";
            break;
        case table_readiness_t::outdated_reads:
            blurb = "available for outdated reads, but not up-to-date reads or writes";
            break;
        case table_readiness_t::reads:
            blurb = "available for outdated and up-to-date reads, but not writes";
            break;
        case table_readiness_t::writes:
            blurb = "available for all operations, but some replicas are not ready";
            break;
        case table_readiness_t::finished: // fallthrough intentional
        default:
            unreachable();
        }
        std::string text = strprintf(
            "Table `%s.%s` is %s. The following servers are not reachable: ",
            db_name.c_str(), table_name.c_str(), blurb);
        bool first = true;
        for (const server_id_t &server : status.disconnected) {
            if (first) {
                first = false;
            } else {
                text += ", ";
            }
            text += status.server_names.get(server).c_str();
        }
        *description_out = datum_string_t(text);
    } else {
        *description_out = datum_string_t(strprintf(
            "Table `%s.%s` is not available for any operations. No servers are "
            "reachable for this table.",
            db_name.c_str(), table_name.c_str()));
    }

    return true;
}

const datum_string_t table_availability_issue_t::table_availability_issue_type =
    datum_string_t("table_availability");
const uuid_u table_availability_issue_t::base_issue_id =
    str_to_uuid("4ae8c575-b103-3335-a79a-121089e34339");

table_issue_tracker_t::table_issue_tracker_t(
        server_config_client_t *_server_config_client,
        table_meta_client_t *_table_meta_client,
        namespace_repo_t *_namespace_repo) :
    server_config_client(_server_config_client),
    table_meta_client(_table_meta_client),
    namespace_repo(_namespace_repo) { }

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
    if (interruptor->is_pulsed()) {
        throw interrupted_exc_t();
    }

    return issues;
}

void table_issue_tracker_t::check_table(const namespace_id_t &table_id,
                                        std::vector<scoped_ptr_t<issue_t> > *issues_out,
                                        signal_t *interruptor) const {
    table_status_t status;
    try {
        table_config_and_shards_t config;
        table_meta_client->get_config(table_id, interruptor, &config);
        get_table_status(table_id, config, namespace_repo, table_meta_client,
            server_config_client, interruptor, &status);
    } catch (const interrupted_exc_t &) {
        // Ignore interruption - can't be passed through pmap
        return;
    } catch (const no_such_table_exc_t &) {
        // Ignore any tables that have stopped existing
        return;
    } catch (const failed_table_op_exc_t &) {
        status.readiness = table_readiness_t::unavailable;
        status.total_loss = true;
    }

    if (status.readiness != table_readiness_t::finished) {
        issues_out->emplace_back(make_scoped<table_availability_issue_t>(
            table_id, std::move(status)));
    }
}
