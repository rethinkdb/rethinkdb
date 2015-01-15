// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/tables/debug_table_status.hpp"

// RSI(raft): Reimplement this once table IO works
#if 0

#include "clustering/administration/datum_adapter.hpp"
#include "clustering/administration/servers/config_client.hpp"

debug_table_status_artificial_table_backend_t::
            debug_table_status_artificial_table_backend_t(
        boost::shared_ptr< semilattice_readwrite_view_t<
            cluster_semilattice_metadata_t> > _semilattice_view,
        watchable_map_t<std::pair<peer_id_t, namespace_id_t>,
            namespace_directory_metadata_t> *_directory_view,
        server_config_client_t *_server_config_client) :
    common_table_artificial_table_backend_t(
        _semilattice_view,
        admin_identifier_format_t::uuid),
    directory_view(_directory_view),
    server_config_client(_server_config_client),
    directory_subs(directory_view,
        [this](const std::pair<peer_id_t, namespace_id_t> &key, 
               const namespace_directory_metadata_t *) {
            notify_row(convert_uuid_to_datum(key.second));
        }, false)
    { }

debug_table_status_artificial_table_backend_t::
        ~debug_table_status_artificial_table_backend_t() {
    begin_changefeed_destruction();
}

/* Names like `reactor_activity_entry_t::secondary_without_primary_t` are too long to
type without this */
using reactor_business_card_details::primary_when_safe_t;
using reactor_business_card_details::primary_t;
using reactor_business_card_details::secondary_up_to_date_t;
using reactor_business_card_details::secondary_without_primary_t;
using reactor_business_card_details::secondary_backfilling_t;
using reactor_business_card_details::nothing_when_safe_t;
using reactor_business_card_details::nothing_when_done_erasing_t;
using reactor_business_card_details::nothing_t;

ql::datum_t convert_debug_store_key_to_datum(
        const store_key_t &key) {
    return ql::datum_t::binary(datum_string_t(
        key.size(),
        reinterpret_cast<const char *>(key.contents())));
}

ql::datum_t convert_debug_region_to_datum(
        const region_t &region) {
    ql::datum_object_builder_t builder;
    builder.overwrite("hash_min", ql::datum_t(datum_string_t(
        strprintf("%016" PRIx64, region.beg))));
    builder.overwrite("hash_max", ql::datum_t(datum_string_t(
        strprintf("%016" PRIx64, region.end))));
    builder.overwrite("key_min",
        convert_debug_store_key_to_datum(region.inner.left));
    builder.overwrite("key_max",
        region.inner.right.unbounded
            ? ql::datum_t::null()
            : convert_debug_store_key_to_datum(region.inner.right.key));
    return std::move(builder).to_datum();
}

ql::datum_t convert_debug_version_range_to_datum(
        const version_range_t &range) {
    ql::datum_object_builder_t builder;
    if (range.is_coherent()) {
        builder.overwrite("branch", convert_uuid_to_datum(range.earliest.branch));
        builder.overwrite("timestamp", ql::datum_t(static_cast<double>(
            range.earliest.timestamp.to_repli_timestamp().longtime)));
    } else {
        builder.overwrite("branch_earliest",
            convert_uuid_to_datum(range.earliest.branch));
        builder.overwrite("timestamp_earliest", ql::datum_t(static_cast<double>(
            range.earliest.timestamp.to_repli_timestamp().longtime)));
        builder.overwrite("branch_latest",
            convert_uuid_to_datum(range.latest.branch));
        builder.overwrite("timestamp_latest", ql::datum_t(static_cast<double>(
            range.latest.timestamp.to_repli_timestamp().longtime)));
    }
    return std::move(builder).to_datum();
}

ql::datum_t convert_debug_version_map_to_datum(
        const region_map_t<version_range_t> &map) {
    ql::datum_array_builder_t builder(ql::configured_limits_t::unlimited);
    for (const auto &pair : map) {
        ql::datum_object_builder_t pair_builder;
        pair_builder.overwrite("region",
            convert_debug_region_to_datum(pair.first));
        pair_builder.overwrite("version",
            convert_debug_version_range_to_datum(pair.second));
        builder.add(std::move(pair_builder).to_datum());
    }
    return std::move(builder).to_datum();
}

ql::datum_t convert_debug_reactor_activity_to_datum(
        const reactor_activity_id_t &id,
        const reactor_activity_entry_t &entry) {
    ql::datum_object_builder_t builder;
    builder.overwrite("id", convert_uuid_to_datum(id));
    builder.overwrite("region", convert_debug_region_to_datum(entry.region));
    class visitor_t : public boost::static_visitor<const char *> {
    public:
        const char *operator()(const primary_when_safe_t &) {
            return "primary_when_safe";
        }
        const char *operator()(const primary_t &p) {
            b->overwrite("branch", convert_uuid_to_datum(p.broadcaster.branch_id));
            b->overwrite("has_replier", ql::datum_t::boolean(
                static_cast<bool>(p.replier)));
            b->overwrite("has_master", ql::datum_t::boolean(
                static_cast<bool>(p.master)));
            b->overwrite("has_direct_reader", ql::datum_t::boolean(
                static_cast<bool>(p.direct_reader)));
            return "primary";
        }
        const char *operator()(const secondary_up_to_date_t &s) {
            b->overwrite("branch", convert_uuid_to_datum(s.branch_id));
            return "secondary_up_to_date";
        }
        const char *operator()(const secondary_without_primary_t &s) {
            b->overwrite("version", convert_debug_version_map_to_datum(s.current_state));
            return "secondary_without_primary";
        }
        const char *operator()(const secondary_backfilling_t &) {
            return "secondary_backfilling";
        }
        const char *operator()(const nothing_when_safe_t &n) {
            b->overwrite("version", convert_debug_version_map_to_datum(n.current_state));
            return "nothing_when_safe";
        }
        const char *operator()(const nothing_when_done_erasing_t &) {
            return "nothing_when_done_erasing";
        }
        const char *operator()(const nothing_t &) {
            return "nothing";
        }
        ql::datum_object_builder_t *b;
    } visitor;
    visitor.b = &builder;
    builder.overwrite("state", ql::datum_t(
        boost::apply_visitor(visitor, entry.activity)));
    return std::move(builder).to_datum();
}

ql::datum_t convert_debug_table_status_to_datum(
        name_string_t table_name,
        const ql::datum_t &db_name_or_uuid,
        namespace_id_t uuid,
        const namespace_semilattice_metadata_t &metadata,
        watchable_map_t<std::pair<peer_id_t, namespace_id_t>,
                        namespace_directory_metadata_t> *dir,
        server_config_client_t *server_config_client) {
    ql::datum_object_builder_t builder;
    builder.overwrite("name", convert_name_to_datum(table_name));
    builder.overwrite("db", db_name_or_uuid);
    builder.overwrite("id", convert_uuid_to_datum(uuid));

    ql::datum_array_builder_t servers_builder(ql::configured_limits_t::unlimited);
    dir->read_all([&](const std::pair<peer_id_t, namespace_id_t> &key,
                      const namespace_directory_metadata_t *value) {
            if (key.second != uuid) {
                return;
            }
            boost::optional<server_id_t> server_id =
                server_config_client->get_server_id_for_peer_id(key.first);
            if (!static_cast<bool>(server_id)) {
                return;
            }
            boost::optional<name_string_t> name =
                server_config_client->get_name_for_server_id(*server_id);
            if (!static_cast<bool>(name)) {
                name = boost::optional<name_string_t>(
                    name_string_t::guarantee_valid("__no_valid_name__"));
            }
            ql::datum_object_builder_t server_builder;
            server_builder.overwrite("id", convert_uuid_to_datum(*server_id));
            server_builder.overwrite("name", convert_name_to_datum(*name));
            ql::datum_array_builder_t activities_builder(
                ql::configured_limits_t::unlimited);
            for (const auto &pair : value->internal->activities) {
                activities_builder.add(convert_debug_reactor_activity_to_datum(
                    pair.first, pair.second));
            }
            server_builder.overwrite("activities",
                std::move(activities_builder).to_datum());
            servers_builder.add(std::move(server_builder).to_datum());
        });
    builder.overwrite("servers", std::move(servers_builder).to_datum());

    builder.overwrite("split_points",
        convert_vector_to_datum<store_key_t>(
            &convert_debug_store_key_to_datum,
            metadata.replication_info.get_ref().shard_scheme.split_points));

    return std::move(builder).to_datum();
}

bool debug_table_status_artificial_table_backend_t::format_row(
        namespace_id_t table_id,
        name_string_t table_name,
        const ql::datum_t &db_name_or_uuid,
        const namespace_semilattice_metadata_t &metadata,
        UNUSED signal_t *interruptor,
        ql::datum_t *row_out,
        UNUSED std::string *error_out) {
    assert_thread();
    *row_out = convert_debug_table_status_to_datum(
        table_name,
        db_name_or_uuid,
        table_id,
        metadata,
        directory_view,
        server_config_client);
    return true;
}

bool debug_table_status_artificial_table_backend_t::write_row(
        UNUSED ql::datum_t primary_key,
        UNUSED bool pkey_was_autogenerated,
        UNUSED ql::datum_t *new_value_inout,
        UNUSED signal_t *interruptor,
        std::string *error_out) {
    *error_out = "It's illegal to write to the `rethinkdb._debug_table_status` table.";
    return false;
}

#endif

