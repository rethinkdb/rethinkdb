// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "unittest/rdb_env.hpp"

#include "rdb_protocol/func.hpp"
#include "rdb_protocol/real_table.hpp"

namespace unittest {

mock_namespace_interface_t::mock_namespace_interface_t(ql::env_t *_env) :
    env(_env) {
    ready_cond.pulse();
}

mock_namespace_interface_t::~mock_namespace_interface_t() {
    while (!data.empty()) {
        delete data.begin()->second;
        data.erase(data.begin());
    }
}

void mock_namespace_interface_t::read(const read_t &query,
                                      read_response_t *response,
                                      UNUSED order_token_t tok,
                                      signal_t *interruptor) THROWS_ONLY(interrupted_exc_t, cannot_perform_query_exc_t) {
    read_outdated(query, response, interruptor);
}

void mock_namespace_interface_t::read_outdated(const read_t &query,
                                               read_response_t *response,
                                               signal_t *interruptor) THROWS_ONLY(interrupted_exc_t, cannot_perform_query_exc_t) {
    if (interruptor->is_pulsed()) {
        throw interrupted_exc_t();
    }
    read_visitor_t v(&data, response);
    boost::apply_visitor(v, query.read);
}

void mock_namespace_interface_t::write(const write_t &query,
                                       write_response_t *response,
                                       UNUSED order_token_t tok,
                                       signal_t *interruptor) THROWS_ONLY(interrupted_exc_t, cannot_perform_query_exc_t) {
    if (interruptor->is_pulsed()) {
        throw interrupted_exc_t();
    }
    write_visitor_t v(&data, env, response);
    boost::apply_visitor(v, query.write);
}

std::map<store_key_t, scoped_cJSON_t *> *mock_namespace_interface_t::get_data() {
    return &data;
}

std::set<region_t> mock_namespace_interface_t::get_sharding_scheme()
    THROWS_ONLY(cannot_perform_query_exc_t) {
    std::set<region_t> s;
    s.insert(region_t::universe());
    return s;
}

bool mock_namespace_interface_t::check_readiness(table_readiness_t, signal_t *) {
    throw cannot_perform_query_exc_t("unimplemented");
}

void mock_namespace_interface_t::read_visitor_t::operator()(const point_read_t &get) {
    ql::configured_limits_t limits;
    response->response = point_read_response_t();
    point_read_response_t &res = boost::get<point_read_response_t>(response->response);

    if (data->find(get.key) != data->end()) {
        res.data = ql::to_datum(data->at(get.key)->get(), limits,
                                reql_version_t::LATEST);
    } else {
        res.data = ql::datum_t::null();
    }
}

void mock_namespace_interface_t::read_visitor_t::operator()(const dummy_read_t &) {
    response->response = dummy_read_response_t();
}

void NORETURN mock_namespace_interface_t::read_visitor_t::operator()(
        const changefeed_subscribe_t &) {
    throw cannot_perform_query_exc_t("unimplemented");
}

void NORETURN mock_namespace_interface_t::read_visitor_t::operator()(
        const changefeed_limit_subscribe_t &) {
    throw cannot_perform_query_exc_t("unimplemented");
}

void NORETURN mock_namespace_interface_t::read_visitor_t::operator()(
        const changefeed_stamp_t &) {
    throw cannot_perform_query_exc_t("unimplemented");
}

void NORETURN mock_namespace_interface_t::read_visitor_t::operator()(
        UNUSED const rget_read_t &rget) {
    throw cannot_perform_query_exc_t("unimplemented");
}

void NORETURN mock_namespace_interface_t::read_visitor_t::operator()(
        const changefeed_point_stamp_t &) {
    throw cannot_perform_query_exc_t("unimplemented");
}

void NORETURN mock_namespace_interface_t::read_visitor_t::operator()(
        UNUSED const intersecting_geo_read_t &gr) {
    throw cannot_perform_query_exc_t("unimplemented");
}

void NORETURN mock_namespace_interface_t::read_visitor_t::operator()(
        UNUSED const nearest_geo_read_t &gr) {
    throw cannot_perform_query_exc_t("unimplemented");
}

void NORETURN mock_namespace_interface_t::read_visitor_t::operator()(
        UNUSED const distribution_read_t &dg) {
    throw cannot_perform_query_exc_t("unimplemented");
}

void NORETURN mock_namespace_interface_t::read_visitor_t::operator()(
        UNUSED const sindex_list_t &sinner) {
    throw cannot_perform_query_exc_t("unimplemented");
}

void NORETURN mock_namespace_interface_t::read_visitor_t::operator()(
        UNUSED const sindex_status_t &ss) {
    throw cannot_perform_query_exc_t("unimplemented");
}

mock_namespace_interface_t::read_visitor_t::read_visitor_t(
        std::map<store_key_t, scoped_cJSON_t *> *_data,
        read_response_t *_response) :
    data(_data), response(_response) {
    // Do nothing
}

void mock_namespace_interface_t::write_visitor_t::operator()(
    const batched_replace_t &r) {
    ql::configured_limits_t limits;
    ql::datum_t stats = ql::datum_t::empty_object();
    std::set<std::string> conditions;
    for (auto it = r.keys.begin(); it != r.keys.end(); ++it) {
        ql::datum_object_builder_t resp;
        ql::datum_t old_val;
        if (data->find(*it) != data->end()) {
            old_val = ql::to_datum(data->at(*it)->get(), limits, reql_version_t::LATEST);
        } else {
            old_val = ql::datum_t::null();
        }

        ql::datum_t new_val
            = r.f.compile_wire_func()->call(env, old_val)->as_datum();
        data->erase(*it);

        bool err;
        if (new_val.get_type() == ql::datum_t::R_OBJECT) {
            data->insert(std::make_pair(*it, new scoped_cJSON_t(new_val.as_json())));
            if (old_val.get_type() == ql::datum_t::R_NULL) {
                err = resp.add("inserted", ql::datum_t(1.0));
            } else {
                if (old_val == new_val) {
                    err = resp.add("unchanged", ql::datum_t(1.0));
                } else {
                    err = resp.add("replaced", ql::datum_t(1.0));
                }
            }
        } else if (new_val.get_type() == ql::datum_t::R_NULL) {
            if (old_val.get_type() == ql::datum_t::R_NULL) {
                err = resp.add("skipped", ql::datum_t(1.0));
            } else {
                err = resp.add("deleted", ql::datum_t(1.0));
            }
        } else {
            throw cannot_perform_query_exc_t(
                "value being inserted is neither an object nor an empty value");
        }
        guarantee(!err);
        stats = stats.merge(std::move(resp).to_datum(), ql::stats_merge,
                            limits, &conditions);
    }
    ql::datum_object_builder_t result(std::move(stats));
    result.add_warnings(conditions, limits);
    response->response = std::move(result).to_datum();
}

void mock_namespace_interface_t::write_visitor_t::operator()(
    const batched_insert_t &bi) {
    ql::configured_limits_t limits;
    ql::datum_t stats = ql::datum_t::empty_object();
    std::set<std::string> conditions;
    for (auto it = bi.inserts.begin(); it != bi.inserts.end(); ++it) {
        store_key_t key((*it).get_field(datum_string_t(bi.pkey)).print_primary());
        ql::datum_object_builder_t resp;
        ql::datum_t old_val;
        if (data->find(key) != data->end()) {
            old_val = ql::to_datum(data->at(key)->get(), limits, reql_version_t::LATEST);
        } else {
            old_val = ql::datum_t::null();
        }

        ql::datum_t new_val = *it;
        data->erase(key);

        bool err;
        if (new_val.get_type() == ql::datum_t::R_OBJECT) {
            data->insert(std::make_pair(key, new scoped_cJSON_t(new_val.as_json())));
            if (old_val.get_type() == ql::datum_t::R_NULL) {
                err = resp.add("inserted", ql::datum_t(1.0));
            } else {
                if (old_val == new_val) {
                    err = resp.add("unchanged", ql::datum_t(1.0));
                } else {
                    err = resp.add("replaced", ql::datum_t(1.0));
                }
            }
        } else if (new_val.get_type() == ql::datum_t::R_NULL) {
            if (old_val.get_type() == ql::datum_t::R_NULL) {
                err = resp.add("skipped", ql::datum_t(1.0));
            } else {
                err = resp.add("deleted", ql::datum_t(1.0));
            }
        } else {
            throw cannot_perform_query_exc_t(
                "value being inserted is neither an object nor an empty value");
        }
        guarantee(!err);
        stats = stats.merge(std::move(resp).to_datum(), ql::stats_merge, limits, &conditions);
    }
    ql::datum_object_builder_t result(stats);
    result.add_warnings(conditions, limits);
    response->response = std::move(result).to_datum();
}

void mock_namespace_interface_t::write_visitor_t::operator()(const dummy_write_t &) {
    response->response = dummy_write_response_t();
}

void NORETURN mock_namespace_interface_t::write_visitor_t::operator()(const point_write_t &) {
    throw cannot_perform_query_exc_t("unimplemented");
}

void NORETURN mock_namespace_interface_t::write_visitor_t::operator()(const point_delete_t &) {
    throw cannot_perform_query_exc_t("unimplemented");
}

void NORETURN mock_namespace_interface_t::write_visitor_t::operator()(const sindex_create_t &) {
    throw cannot_perform_query_exc_t("unimplemented");
}

void NORETURN mock_namespace_interface_t::write_visitor_t::operator()(const sindex_drop_t &) {
    throw cannot_perform_query_exc_t("unimplemented");
}

void NORETURN mock_namespace_interface_t::write_visitor_t::operator()(const sindex_rename_t &) {
    throw cannot_perform_query_exc_t("unimplemented");
}

void NORETURN mock_namespace_interface_t::write_visitor_t::operator()(const sync_t &) {
    throw cannot_perform_query_exc_t("unimplemented");
}

mock_namespace_interface_t::write_visitor_t::write_visitor_t(std::map<store_key_t, scoped_cJSON_t*> *_data,
                                                             ql::env_t *_env,
                                                             write_response_t *_response) :
    data(_data), env(_env), response(_response) {
    // Do nothing
}

test_rdb_env_t::test_rdb_env_t() { }

test_rdb_env_t::~test_rdb_env_t() {
    // Clean up initial datas (if there was no instance constructed, this may happen
    for (auto it = initial_datas.begin(); it != initial_datas.end(); ++it) {
        delete it->second;
    }
}

void test_rdb_env_t::add_table(const std::string &table_name,
                               const uuid_u &db_id,
                               const std::string &primary_key,
                               const std::set<std::map<std::string, std::string> > &initial_data) {
    name_string_t table_name_string;
    if (!table_name_string.assign_value(table_name)) throw invalid_name_exc_t(table_name);

    primary_keys.insert(std::make_pair(std::make_pair(db_id, table_name_string),
                                       primary_key));

    // Set up initial data
    std::map<store_key_t, scoped_cJSON_t*> *data = new std::map<store_key_t, scoped_cJSON_t*>();

    for (auto it = initial_data.begin(); it != initial_data.end(); ++it) {
        guarantee(it->find(primary_key) != it->end());
        store_key_t key("S" + it->at(primary_key));
        scoped_cJSON_t *item = new scoped_cJSON_t(cJSON_CreateObject());

        for (auto jt = it->begin(); jt != it->end(); ++jt) {
            cJSON* strvalue = cJSON_CreateString(jt->second.c_str());
            item->AddItemToObject(jt->first.c_str(), strvalue);
        }
        data->insert(std::make_pair(key, item));
    }

    initial_datas.insert(std::make_pair(std::make_pair(db_id, table_name_string), data));
}

database_id_t test_rdb_env_t::add_database(const std::string &db_name) {
    name_string_t db_name_string;
    if (!db_name_string.assign_value(db_name)) throw invalid_name_exc_t(db_name);
    database_id_t id = generate_uuid();
    databases[db_name_string] = id;
    return id;
}

scoped_ptr_t<test_rdb_env_t::instance_t> test_rdb_env_t::make_env() {
    return make_scoped<instance_t>(this);
}

test_rdb_env_t::instance_t::instance_t(test_rdb_env_t *test_env) :
    extproc_pool(2),
    rdb_ctx(&extproc_pool, this)
{
    env.init(new ql::env_t(&rdb_ctx,
                           false,
                           &interruptor,
                           std::map<std::string, ql::wire_func_t>(),
                           nullptr /* no profile trace */));

    // Set up any initial datas
    databases = test_env->databases;
    primary_keys = test_env->primary_keys;
    for (auto it = test_env->initial_datas.begin(); it != test_env->initial_datas.end(); ++it) {
        scoped_ptr_t<mock_namespace_interface_t> storage(
            new mock_namespace_interface_t(env.get()));
        storage->get_data()->swap(*it->second);
        auto res = tables.insert(std::make_pair(it->first, std::move(storage)));
        guarantee(res.second == true);
        delete it->second;
    }
    test_env->initial_datas.clear();
}

ql::env_t *test_rdb_env_t::instance_t::get() {
    return env.get();
}

std::map<store_key_t, scoped_cJSON_t *> *test_rdb_env_t::instance_t::get_data(
        database_id_t db, name_string_t table) {
    guarantee(tables.count(std::make_pair(db, table)) == 1);
    return tables.at(std::make_pair(db, table))->get_data();
}

void test_rdb_env_t::instance_t::interrupt() {
    interruptor.pulse();
}

bool test_rdb_env_t::instance_t::db_create(UNUSED const name_string_t &name,
        UNUSED signal_t *local_interruptor, UNUSED ql::datum_t *result_out,
        std::string *error_out) {
    *error_out = "test_rdb_env_t::instance_t doesn't support mutation";
    return false;
}

bool test_rdb_env_t::instance_t::db_drop(UNUSED const name_string_t &name,
        UNUSED signal_t *local_interruptor, UNUSED ql::datum_t *result_out,
        std::string *error_out) {
    *error_out = "test_rdb_env_t::instance_t doesn't support mutation";
    return false;
}

bool test_rdb_env_t::instance_t::db_list(
        UNUSED signal_t *local_interruptor, std::set<name_string_t> *names_out,
        UNUSED std::string *error_out) {
    for (auto pair : databases) {
        names_out->insert(pair.first);
    }
    return true;
}

bool test_rdb_env_t::instance_t::db_find(const name_string_t &name,
        UNUSED signal_t *local_interruptor, counted_t<const ql::db_t> *db_out,
        std::string *error_out) {
    auto it = databases.find(name);
    if (it == databases.end()) {
        *error_out = "No database with that name";
        return false;
    } else {
        *db_out = make_counted<const ql::db_t>(it->second, name);
        return true;
    }
}

bool test_rdb_env_t::instance_t::db_config(
        UNUSED const counted_t<const ql::db_t> &db,
        UNUSED const ql::protob_t<const Backtrace> &bt,
        UNUSED ql::env_t *local_env,
        UNUSED scoped_ptr_t<ql::val_t> *selection_out,
        std::string *error_out) {
    *error_out = "test_db_env_t::instance_t doesn't support db_config()";
    return false;
}

bool test_rdb_env_t::instance_t::table_create(UNUSED const name_string_t &name,
        UNUSED counted_t<const ql::db_t> db,
        UNUSED const table_generate_config_params_t &config_params,
        UNUSED const std::string &primary_key,
        UNUSED write_durability_t durability,
        UNUSED signal_t *local_interruptor,
        UNUSED ql::datum_t *result_out,
        std::string *error_out) {
    *error_out = "test_rdb_env_t::instance_t doesn't support mutation";
    return false;
}

bool test_rdb_env_t::instance_t::table_drop(UNUSED const name_string_t &name,
        UNUSED counted_t<const ql::db_t> db,
        UNUSED signal_t *local_interruptor,
        UNUSED ql::datum_t *result_out,
        std::string *error_out) {
    *error_out = "test_rdb_env_t::instance_t doesn't support mutation";
    return false;
}

bool test_rdb_env_t::instance_t::table_list(counted_t<const ql::db_t> db,
        UNUSED signal_t *local_interruptor, std::set<name_string_t> *names_out,
        UNUSED std::string *error_out) {
    for (auto it = tables.begin(); it != tables.end(); it++) {
        if (it->first.first == db->id) {
            names_out->insert(it->first.second);
        }
    }
    return true;
}

class fake_ref_tracker_t : public namespace_interface_access_t::ref_tracker_t {
    void add_ref() { }
    void release() { }
};

bool test_rdb_env_t::instance_t::table_find(const name_string_t &name,
        counted_t<const ql::db_t> db,
        boost::optional<admin_identifier_format_t> identifier_format,
        UNUSED signal_t *local_interruptor, counted_t<base_table_t> *table_out,
        std::string *error_out) {
    auto it = tables.find(std::make_pair(db->id, name));
    if (it == tables.end()) {
        *error_out = "No table with that name";
        return false;
    } else {
        if (static_cast<bool>(identifier_format)) {
            *error_out = "identifier_format doesn't make sense for "
                "test_rdb_env_t::instance_t";
            return false;
        }
        static fake_ref_tracker_t fake_ref_tracker;
        namespace_interface_access_t table_access(
            it->second.get(), &fake_ref_tracker, get_thread_id());
        table_out->reset(new real_table_t(nil_uuid(), table_access,
            primary_keys.at(std::make_pair(db->id, name)), NULL));
        return true;
    }
}

bool test_rdb_env_t::instance_t::table_estimate_doc_counts(
        UNUSED counted_t<const ql::db_t> db,
        UNUSED const name_string_t &name,
        UNUSED ql::env_t *local_env,
        UNUSED std::vector<int64_t> *doc_counts_out,
        std::string *error_out) {
    *error_out = "test_rdb_env_t::instance_t doesn't support info()";
    return false;
}

bool test_rdb_env_t::instance_t::table_config(
        UNUSED counted_t<const ql::db_t> db,
        UNUSED const name_string_t &name,
        UNUSED const ql::protob_t<const Backtrace> &bt,
        UNUSED ql::env_t *local_env,
        UNUSED scoped_ptr_t<ql::val_t> *selection_out,
        std::string *error_out) {
    *error_out = "test_rdb_env_t::instance_t doesn't support table_config()";
    return false;
}

bool test_rdb_env_t::instance_t::table_status(
        UNUSED counted_t<const ql::db_t> db,
        UNUSED const name_string_t &name,
        UNUSED const ql::protob_t<const Backtrace> &bt,
        UNUSED ql::env_t *local_env,
        UNUSED scoped_ptr_t<ql::val_t> *selection_out,
        std::string *error_out) {
    *error_out = "test_rdb_env_t::instance_t doesn't support table_status()";
    return false;
}

bool test_rdb_env_t::instance_t::table_wait(
        UNUSED counted_t<const ql::db_t> db,
        UNUSED const name_string_t &name,
        UNUSED table_readiness_t readiness,
        UNUSED signal_t *local_interruptor,
        UNUSED ql::datum_t *result_out,
        std::string *error_out) {
    *error_out = "test_rdb_env_t::instance_t doesn't support table_wait()";
    return false;
}

bool test_rdb_env_t::instance_t::db_wait(
        UNUSED counted_t<const ql::db_t> db,
        UNUSED table_readiness_t readiness,
        UNUSED signal_t *local_interruptor,
        UNUSED ql::datum_t *result_out,
        std::string *error_out) {
    *error_out = "test_rdb_env_t::instance_t doesn't support db_wait()";
    return false;
}

bool test_rdb_env_t::instance_t::table_reconfigure(
        UNUSED counted_t<const ql::db_t> db,
        UNUSED const name_string_t &name,
        UNUSED const table_generate_config_params_t &params,
        UNUSED bool dry_run,
        UNUSED signal_t *local_interruptor,
        UNUSED ql::datum_t *result_out,
        std::string *error_out) {
    *error_out = "test_rdb_env_t::instance_t doesn't support reconfigure()";
    return false;
}

bool test_rdb_env_t::instance_t::db_reconfigure(
        UNUSED counted_t<const ql::db_t> db,
        UNUSED const table_generate_config_params_t &params,
        UNUSED bool dry_run,
        UNUSED signal_t *local_interruptor,
        UNUSED ql::datum_t *result_out,
        std::string *error_out) {
    *error_out = "test_rdb_env_t::instance_t doesn't support reconfigure()";
    return false;
}

bool test_rdb_env_t::instance_t::table_rebalance(
        UNUSED counted_t<const ql::db_t> db,
        UNUSED const name_string_t &name,
        UNUSED signal_t *local_interruptor,
        UNUSED ql::datum_t *result_out,
        std::string *error_out) {
    *error_out = "test_rdb_env_t::instance_t doesn't support rebalance()";
    return false;
}

bool test_rdb_env_t::instance_t::db_rebalance(
        UNUSED counted_t<const ql::db_t> db,
        UNUSED signal_t *local_interruptor,
        UNUSED ql::datum_t *result_out,
        std::string *error_out) {
    *error_out = "test_rdb_env_t::instance_t doesn't support rebalance()";
    return false;
}

}  // namespace unittest
