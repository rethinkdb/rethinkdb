// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/tables/table_config.hpp"

json_adapter_if_t::json_adapter_map_t get_json_subfields(table_shard_config_t *config) {
    json_adapter_if_t::json_adapter_map_t res;
    res["region"] = boost::shared_ptr<json_adapter_if_t(
        new json_adapter_t<region_t>(&config->region));
    res["replicas"] = boost::shared_ptr<json_adapter_if_t(
        new json_adapter_t<std::set<name_string_t> >(&config->replica_names));
    res["directors"] = boost::shared_ptr<json_adapter_if_t(
        new json_adapter_t<std::set<name_string_t> >(&config->director_names));
    return res;
}

cJSON *render_as_json(table_shard_config_t *config) {
    return render_as_directory(config);
}

void apply_json_to(cJSON *change, table_shard_config_t *config) {
    apply_as_directory(change, config);
}

json_adapter_if_t::json_adapter_map_t get_json_subfields(
        namespace_semilattice_metadata_t *table) {
    json_adapter_if_t::json_adapter_map_t res;
    res["name"] = boost::shared_ptr<json_adapter_if_t>(
        new json_adapter_t<name_string_t>(&table->name));
    res["shards"] = boost::shared_ptr<json_adapter_if_t>(
        new json_adapter_t< std::vector<table_shard_config_t> >(&table->shards));
    return res;
}

void render_as_json(namespace_semilattice_metadata_t *table) {
    return render_as_directory(table);
}

void apply_json_to(cJSON *change, namespace_semilattice_metadata_t *table) {
    apply_as_directory(change, table);
    // TODO: Validate shard regions
    // TODO: Validate that replica and director sets are non-empty
    // TODO: Validate that replica set is a superset of director set
}

std::string table_config_artificial_table_backend_t::get_primary_key_name() {
    return "name";
}

bool table_config_artificial_table_backend_t::read_all_primary_keys(
        UNUSED signal_t *interruptor,
        std::vector<counted_t<const ql::datum_t> > *keys_out,
        std::string *error_out) {
    keys_out->clear();
    namespace_semilattice_metadata_t md = semilattice_view->get();
    for (auto it = md->namespaces.begin();
              it != md->namespaces.end();
            ++it) {
        if (it->second.is_deleted() || it->second.get_ref().in_conflict()) {
            /* TODO: Handle conflict differently */
            continue;
        }
        name_string_t name = it->second.get_ref().get_ref().name;
        /* TODO: How to handle table name collisions? */
        keys_out->push_back(make_counted<const ql::datum_t>(name.str()));
    }
    return true;
}

bool table_config_artificial_table_backend_t::read_row(
        counted_t<const ql::datum_t> primary_key,
        UNUSED signal_t *interruptor,
        counted_t<const ql::datum_t> *row_out,
        std::string *error_out) {
    if (primary_key->get_type() != ql::datum_t::R_STR) {
        /* All tables have a string primary key, so a get on a non-string primary key
        will never find a row. */
        *row_out = counted_t<const ql::datum_t>();
        return true;
    }
    namespace_semilattice_metadata_t md = semilattice_view->get();
    for (auto it = md->namespaces.begin();
              it != md->namespaces.end();
            ++it) {
        if (it->second.is_deleted() || it->second.get_ref().in_conflict()) {
            /* TODO: Handle conflict differently */
            continue;
        }
        name_string_t name = it->second.get_ref().get_ref().name;
        if (name.str() == primary_key->as_str().to_std()) {
            namespace_semilattice_metadata_t table = it->second.get_ref().get_copy();
            scoped_cJSON_t table_json(render_as_json(&table));
            return to_datum(table_json.get(), configured_limits_t());
        }
    }
    *row_out = counted_t<const ql::datum_t>();
    return true;
}

bool table_config_artificial_table_backend_t::write_row(
        counted_t<const ql::datum_t> primary_key,
        counted_t<const ql::datum_t> new_value,
        UNUSED signal_t *interruptor,
        std::string *error_out) {
    if (primary_key->get_type() != ql::datum_t::R_STR) {
        *error_out = "To create a table, you must use table_create() instead of "
            "inserting into the `rethinkdb.table_config` table.";
        return false;
    }
    namespace_semilattice_metadata_t md = semilattice_view->get();
    for (auto it = md->namespaces.begin();
              it != md->namespaces.end();
            ++it) {
        if (it->second.is_deleted() || it->second.get_ref().in_conflict()) {
            continue;
        }
        name_string_t name = it->second.get_ref().get_ref().name;
        if (name.str() == primary_key->as_str().to_std()) {
            namespace_semilattice_metadata_t new_value;
            scoped_cJSON_t new_json = new_value.as_json();
            apply_json_to(new_json.get(), &new_value);
            *it->second.get_mutable() = it->second.get_ref().make_new_version(
                new_value, my_machine_id);
            semilattice_view->join(md);
            return true;
        }
    }
    *error_out = "To create a table, you must use table_create() instead of inserting "
        "into the `rethinkdb.table_config` table.";
    return true;
}

publisher_t<std::function<void(counted_t<const ql::datum_t>)> > *
table_config_artificial_table_backend_t::get_publisher() {
    return NULL;
}

