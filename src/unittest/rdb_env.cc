// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "unittest/rdb_env.hpp"

#include "rdb_protocol/func.hpp"

namespace unittest {

mock_namespace_repo_t::mock_namespace_repo_t() : env(NULL) { }
mock_namespace_repo_t::~mock_namespace_repo_t() {
    while (!cache.empty()) {
        delete cache.begin()->second;
        cache.erase(cache.begin());
    }
}

void mock_namespace_repo_t::set_env(ql::env_t *_env) {
    env = _env;
}

ql::env_t *mock_namespace_repo_t::get_env() {
    return env;
}

mock_namespace_interface_t *mock_namespace_repo_t::get_ns_if(const namespace_id_t &ns_id) {
    get_cache_entry(ns_id); // This will create it if it doesn't already exist
    if (cache.find(ns_id) != cache.end()) {
        return &cache[ns_id]->mock_ns_if;
    }
    return NULL;
}

mock_namespace_repo_t::namespace_cache_entry_t *mock_namespace_repo_t::get_cache_entry(const namespace_id_t &ns_id) {
    if (cache.find(ns_id) == cache.end()) {
        mock_namespace_cache_entry_t *entry = new mock_namespace_cache_entry_t(this);
        entry->entry.namespace_if.pulse(&entry->mock_ns_if);
        entry->entry.ref_count = 0;
        entry->entry.pulse_when_ref_count_becomes_zero = NULL;
        entry->entry.pulse_when_ref_count_becomes_nonzero = NULL;
        cache.insert(std::make_pair(ns_id, entry));
    }
    return &cache[ns_id]->entry;
}


mock_namespace_interface_t::mock_namespace_interface_t(mock_namespace_repo_t *_parent) :
    parent(_parent) {
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
    write_visitor_t v(&data, parent->get_env(), response);
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

void mock_namespace_interface_t::read_visitor_t::operator()(const point_read_t &get) {
    response->response = point_read_response_t();
    point_read_response_t &res = boost::get<point_read_response_t>(response->response);

    if (data->find(get.key) != data->end()) {
        res.data = make_counted<ql::datum_t>(scoped_cJSON_t(data->at(get.key)->DeepCopy()));
    } else {
        res.data = make_counted<ql::datum_t>(ql::datum_t::R_NULL);
    }
}

void NORETURN mock_namespace_interface_t::read_visitor_t::operator()(UNUSED const rget_read_t &rget) {
    throw cannot_perform_query_exc_t("unimplemented");
}

void NORETURN mock_namespace_interface_t::read_visitor_t::operator()(UNUSED const distribution_read_t &dg) {
    throw cannot_perform_query_exc_t("unimplemented");
}

void NORETURN mock_namespace_interface_t::read_visitor_t::operator()(UNUSED const sindex_list_t &sinner) {
    throw cannot_perform_query_exc_t("unimplemented");
}

void NORETURN mock_namespace_interface_t::read_visitor_t::operator()(UNUSED const sindex_status_t &ss) {
    throw cannot_perform_query_exc_t("unimplemented");
}

mock_namespace_interface_t::read_visitor_t::read_visitor_t(std::map<store_key_t, scoped_cJSON_t *> *_data,
                                                           read_response_t *_response) :
    data(_data), response(_response) {
    // Do nothing
}

void mock_namespace_interface_t::write_visitor_t::operator()(
    const batched_replace_t &r) {
    counted_t<const ql::datum_t> stats(new ql::datum_t(ql::datum_t::R_OBJECT));
    for (auto it = r.keys.begin(); it != r.keys.end(); ++it) {
        ql::datum_ptr_t resp(ql::datum_t::R_OBJECT);
        counted_t<const ql::datum_t> old_val;
        if (data->find(*it) != data->end()) {
            old_val = make_counted<ql::datum_t>(data->at(*it)->get());
        } else {
            old_val = make_counted<ql::datum_t>(ql::datum_t::R_NULL);
        }

        counted_t<const ql::datum_t> new_val
            = r.f.compile_wire_func()->call(env, old_val)->as_datum();
        data->erase(*it);

        bool err;
        if (new_val->get_type() == ql::datum_t::R_OBJECT) {
            data->insert(std::make_pair(*it, new scoped_cJSON_t(new_val->as_json())));
            if (old_val->get_type() == ql::datum_t::R_NULL) {
                err = resp.add("inserted", make_counted<const ql::datum_t>(1.0));
            } else {
                if (*old_val == *new_val) {
                    err = resp.add("unchanged", make_counted<const ql::datum_t>(1.0));
                } else {
                    err = resp.add("replaced", make_counted<const ql::datum_t>(1.0));
                }
            }
        } else if (new_val->get_type() == ql::datum_t::R_NULL) {
            if (old_val->get_type() == ql::datum_t::R_NULL) {
                err = resp.add("skipped", make_counted<const ql::datum_t>(1.0));
            } else {
                err = resp.add("deleted", make_counted<const ql::datum_t>(1.0));
            }
        } else {
            throw cannot_perform_query_exc_t(
                "value being inserted is neither an object nor an empty value");
        }
        guarantee(!err);
        stats = stats->merge(resp.to_counted(), ql::stats_merge);
    }
    response->response = stats;
}

void mock_namespace_interface_t::write_visitor_t::operator()(
    const batched_insert_t &bi) {
    counted_t<const ql::datum_t> stats(new ql::datum_t(ql::datum_t::R_OBJECT));
    for (auto it = bi.inserts.begin(); it != bi.inserts.end(); ++it) {
        store_key_t key((*it)->get(bi.pkey)->print_primary());
        ql::datum_ptr_t resp(ql::datum_t::R_OBJECT);
        counted_t<const ql::datum_t> old_val;
        if (data->find(key) != data->end()) {
            old_val = make_counted<ql::datum_t>(data->at(key)->get());
        } else {
            old_val = make_counted<ql::datum_t>(ql::datum_t::R_NULL);
        }

        counted_t<const ql::datum_t> new_val = *it;
        data->erase(key);

        bool err;
        if (new_val->get_type() == ql::datum_t::R_OBJECT) {
            data->insert(std::make_pair(key, new scoped_cJSON_t(new_val->as_json())));
            if (old_val->get_type() == ql::datum_t::R_NULL) {
                err = resp.add("inserted", make_counted<const ql::datum_t>(1.0));
            } else {
                if (*old_val == *new_val) {
                    err = resp.add("unchanged", make_counted<const ql::datum_t>(1.0));
                } else {
                    err = resp.add("replaced", make_counted<const ql::datum_t>(1.0));
                }
            }
        } else if (new_val->get_type() == ql::datum_t::R_NULL) {
            if (old_val->get_type() == ql::datum_t::R_NULL) {
                err = resp.add("skipped", make_counted<const ql::datum_t>(1.0));
            } else {
                err = resp.add("deleted", make_counted<const ql::datum_t>(1.0));
            }
        } else {
            throw cannot_perform_query_exc_t(
                "value being inserted is neither an object nor an empty value");
        }
        guarantee(!err);
        stats = stats->merge(resp.to_counted(), ql::stats_merge);
    }
    response->response = stats;
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

void NORETURN mock_namespace_interface_t::write_visitor_t::operator()(const sync_t &) {
    throw cannot_perform_query_exc_t("unimplemented");
}

mock_namespace_interface_t::write_visitor_t::write_visitor_t(std::map<store_key_t, scoped_cJSON_t*> *_data,
                                                             ql::env_t *_env,
                                                             write_response_t *_response) :
    data(_data), env(_env), response(_response) {
    // Do nothing
}

test_rdb_env_t::test_rdb_env_t() :
    machine_id(generate_uuid()) // Not like we actually care
{
    machine_semilattice_metadata_t machine;
    name_string_t machine_name;
    if (!machine_name.assign_value("test_machine")) throw invalid_name_exc_t("test_machine");
    machine.name = vclock_t<name_string_t>(machine_name, machine_id);
    machine.datacenter = vclock_t<datacenter_id_t>(nil_uuid());
    metadata.machines.machines.insert(std::make_pair(generate_uuid(), make_deletable(machine)));
}

test_rdb_env_t::~test_rdb_env_t() {
    // Clean up initial datas (if there was no instance constructed, this may happen
    for (auto it = initial_datas.begin(); it != initial_datas.end(); ++it) {
        delete it->second;
    }
}

namespace_id_t test_rdb_env_t::add_table(const std::string &table_name,
                                         const uuid_u &db_id,
                                         const std::string &primary_key,
                                         const std::set<std::map<std::string, std::string> > &initial_data) {
    name_string_t table_name_string;
    cow_ptr_t<namespaces_semilattice_metadata_t>::change_t change(&metadata.rdb_namespaces);
    if (!table_name_string.assign_value(table_name)) throw invalid_name_exc_t(table_name);
    namespace_id_t namespace_id = generate_uuid();
    *change.get()->namespaces[namespace_id].get_mutable() =
        new_namespace(machine_id,
                      db_id,
                      nil_uuid(),
                      table_name_string,
                      primary_key);

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

    initial_datas.insert(std::make_pair(namespace_id, data));

    return namespace_id;
}

database_id_t test_rdb_env_t::add_database(const std::string &db_name) {
    name_string_t db_name_string;
    database_semilattice_metadata_t db;
    if (!db_name_string.assign_value(db_name)) throw invalid_name_exc_t(db_name);
    db.name = vclock_t<name_string_t>(db_name_string, machine_id);
    database_id_t database_id = generate_uuid();
    metadata.databases.databases.insert(std::make_pair(database_id,
                                                       make_deletable(db)));
    return database_id;
}

void test_rdb_env_t::make_env(scoped_ptr_t<instance_t> *instance_out) {
    instance_out->init(new instance_t(this));
}

test_rdb_env_t::instance_t::instance_t(test_rdb_env_t *test_env) :
    dummy_semilattice_controller(test_env->metadata),
    namespaces_metadata(new semilattice_watchable_t<cow_ptr_t<namespaces_semilattice_metadata_t> >(metadata_field(&cluster_semilattice_metadata_t::rdb_namespaces, dummy_semilattice_controller.get_view()))),
    databases_metadata(new semilattice_watchable_t<databases_semilattice_metadata_t>(metadata_field(&cluster_semilattice_metadata_t::databases, dummy_semilattice_controller.get_view()))),
    extproc_pool(2),
    test_cluster(0),
    rdb_ns_repo()
{
    env.init(new ql::env_t(&extproc_pool,
                           std::string(),
                           &rdb_ns_repo,
                           namespaces_metadata,
                           databases_metadata,
                           dummy_semilattice_controller.get_view(),
                           NULL,
                           &interruptor,
                           test_env->machine_id,
                           ql::protob_t<Query>()));
    rdb_ns_repo.set_env(env.get());

    // Set up any initial datas
    for (auto it = test_env->initial_datas.begin(); it != test_env->initial_datas.end(); ++it) {
        std::map<store_key_t, scoped_cJSON_t*> *data = get_data(it->first);
        data->swap(*it->second);
        delete it->second;
    }
    test_env->initial_datas.clear();
}

ql::env_t *test_rdb_env_t::instance_t::get() {
    return env.get();
}

std::map<store_key_t, scoped_cJSON_t*>* test_rdb_env_t::instance_t::get_data(const namespace_id_t &ns_id) {
    mock_namespace_interface_t *ns_if = rdb_ns_repo.get_ns_if(ns_id);
    guarantee(ns_if != NULL);
    return ns_if->get_data();
}

void test_rdb_env_t::instance_t::interrupt() {
    interruptor.pulse();
}

}

