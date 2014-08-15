// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/artificial_reql_cluster_interface.hpp"

#include "rdb_protocol/artificial_table/artificial_table.hpp"

bool artificial_reql_cluster_interface_t::db_create(const name_string_t &name,
            signal_t *interruptor, std::string *error_out) {
    if (name == database) {
        *error_out = strprintf("Database `%s` already exists.", database.c_str());
        return false;
    }
    return next->db_create(name, interruptor, error_out);
}

bool artificial_reql_cluster_interface_t::db_drop(const name_string_t &name,
        signal_t *interruptor, std::string *error_out) {
    if (name == database) {
        *error_out = strprintf("Database `%s` is special; you can't delete it.",
            database.c_str());
        return false;
    }
    return next->db_drop(name, interruptor, error_out);
}

bool artificial_reql_cluster_interface_t::db_list(
        signal_t *interruptor,
        std::set<name_string_t> *names_out, std::string *error_out) {
    if (!next->db_list(interruptor, names_out, error_out)) {
        return false;
    }
    guarantee(names_out->count(database) == 0);
    names_out->insert(database);
    return true;
}

bool artificial_reql_cluster_interface_t::db_find(const name_string_t &name,
        signal_t *interruptor,
        counted_t<const ql::db_t> *db_out, std::string *error_out) {
    if (name == database) {
        *db_out = make_counted<const ql::db_t>(nil_uuid(), database.str());
        return true;
    }
    return next->db_find(name, interruptor, db_out, error_out);
}

bool artificial_reql_cluster_interface_t::table_create(const name_string_t &name,
        counted_t<const ql::db_t> db, const boost::optional<name_string_t> &primary_dc,
        bool hard_durability, const std::string &primary_key, signal_t *interruptor,
        std::string *error_out) {
    if (db->name == database.str()) {
        *error_out = strprintf("Database `%s` is special; you can't create new tables "
            "in it.", database.c_str());
        return false;
    }
    return next->table_create(name, db, primary_dc, hard_durability, primary_key,
        interruptor, error_out);
}

bool artificial_reql_cluster_interface_t::table_drop(const name_string_t &name,
        counted_t<const ql::db_t> db, signal_t *interruptor, std::string *error_out) {
    if (db->name == database.str()) {
        *error_out = strprintf("Database `%s` is special; you can't drop tables in it.",
            database.c_str());
        return false;
    }
    return next->table_drop(name, db, interruptor, error_out);
}

bool artificial_reql_cluster_interface_t::table_list(counted_t<const ql::db_t> db,
        signal_t *interruptor,
        std::set<name_string_t> *names_out, std::string *error_out) {
    if (db->name == database.str()) {
        for (auto it = tables.begin(); it != tables.end(); ++it) {
            names_out->insert(it->first);
        }
        return true;
    }
    return next->table_list(db, interruptor, names_out, error_out);
}

bool artificial_reql_cluster_interface_t::table_find(const name_string_t &name,
        counted_t<const ql::db_t> db, signal_t *interruptor,
        scoped_ptr_t<base_table_t> *table_out, std::string *error_out) {
    if (db->name == database.str()) {
        auto it = tables.find(name);
        if (it != tables.end()) {
            table_out->init(new artificial_table_t(it->second));
            return true;
        } else {
            *error_out = strprintf("Table `%s.%s` does not exist.",
                database.c_str(), name.c_str());
            return false;
        }
    }
    return next->table_find(name, db, interruptor, table_out, error_out);
}

bool artificial_reql_cluster_interface_t::server_rename(const name_string_t &old_name,
        const name_string_t &new_name, signal_t *interruptor, std::string *error_out) {
    return next->server_rename(old_name, new_name, interruptor, error_out);
}

bool artificial_reql_cluster_interface_t::table_reconfigure(
        counted_t<const ql::db_t> db, const name_string_t &name,
        const table_generate_config_params_t &params,
        bool dry_run,
        signal_t *interruptor,
        counted_t<const ql::datum_t> *new_config_out,
        std::string *error_out) {
    if (db->name == database.str()) {
        *error_out = strprintf("Database `%s` is special; you can't configure the "
            "tables in it.", database.c_str());
        return false;
    }
    return next->table_reconfigure(db, name, params, dry_run, interruptor,
        new_config_out, error_out);
}

