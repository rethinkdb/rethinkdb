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

mock_namespace_interface_t* mock_namespace_repo_t::get_ns_if(const namespace_id_t &ns_id) {
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

void mock_namespace_interface_t::read(const rdb_protocol_t::read_t &query,
                                      rdb_protocol_t::read_response_t *response,
                                      UNUSED order_token_t tok,
                                      signal_t *interruptor) THROWS_ONLY(interrupted_exc_t, cannot_perform_query_exc_t) {
    read_outdated(query, response, interruptor);
}

void mock_namespace_interface_t::read_outdated(const rdb_protocol_t::read_t &query,
                                               rdb_protocol_t::read_response_t *response,
                                               signal_t *interruptor) THROWS_ONLY(interrupted_exc_t, cannot_perform_query_exc_t) {
    if (interruptor->is_pulsed()) {
        throw interrupted_exc_t();
    }
    read_visitor_t v(&data, response);
    boost::apply_visitor(v, query.read);
}

void mock_namespace_interface_t::write(const rdb_protocol_t::write_t &query,
                                       rdb_protocol_t::write_response_t *response,
                                       UNUSED order_token_t tok,
                                       signal_t *interruptor) THROWS_ONLY(interrupted_exc_t, cannot_perform_query_exc_t) {
    if (interruptor->is_pulsed()) {
        throw interrupted_exc_t();
    }
    write_visitor_t v(&data, parent->get_env(), response);
    boost::apply_visitor(v, query.write);
}

std::map<store_key_t, scoped_cJSON_t*>* mock_namespace_interface_t::get_data() {
    return &data;
}

void mock_namespace_interface_t::read_visitor_t::operator()(const rdb_protocol_t::point_read_t &get) {
    response->response = rdb_protocol_t::point_read_response_t();
    rdb_protocol_t::point_read_response_t &res = boost::get<rdb_protocol_t::point_read_response_t>(response->response);

    if (data->find(get.key) != data->end()) {
        res.data = make_counted<ql::datum_t>(scoped_cJSON_t(data->at(get.key)->DeepCopy()));
    } else {
        res.data = make_counted<ql::datum_t>(ql::datum_t::R_NULL);
    }
}

void NORETURN mock_namespace_interface_t::read_visitor_t::operator()(UNUSED const rdb_protocol_t::rget_read_t &rget) {
    throw cannot_perform_query_exc_t("unimplemented");
}

void NORETURN mock_namespace_interface_t::read_visitor_t::operator()(UNUSED const rdb_protocol_t::distribution_read_t &dg) {
    throw cannot_perform_query_exc_t("unimplemented");
}

void NORETURN mock_namespace_interface_t::read_visitor_t::operator()(UNUSED const rdb_protocol_t::sindex_list_t &sinner) {
    throw cannot_perform_query_exc_t("unimplemented");
}

mock_namespace_interface_t::read_visitor_t::read_visitor_t(std::map<store_key_t, scoped_cJSON_t*> *_data,
                                                           rdb_protocol_t::read_response_t *_response) :
    data(_data), response(_response) {
    // Do nothing
}

void mock_namespace_interface_t::write_visitor_t::operator()(const rdb_protocol_t::point_replace_t &r) {
    response->response = rdb_protocol_t::point_replace_response_t();
    rdb_protocol_t::point_replace_response_t *res = boost::get<rdb_protocol_t::point_replace_response_t>(&response->response);
    ql::map_wire_func_t *f = const_cast<ql::map_wire_func_t *>(&r.f);

    counted_t<const ql::datum_t> num_records = make_counted<ql::datum_t>(1.0);
    ql::datum_ptr_t resp(ql::datum_t::R_OBJECT);

    counted_t<const ql::datum_t> old_val;
    if (data->find(r.key) != data->end()) {
        old_val = make_counted<ql::datum_t>(data->at(r.key)->get());
    } else {
        old_val = make_counted<ql::datum_t>(ql::datum_t::R_NULL);
    }

    counted_t<const ql::datum_t> new_val = f->compile(env)->call(env, old_val)->as_datum();
    data->erase(r.key);

    bool not_added;
    if (new_val->get_type() == ql::datum_t::R_OBJECT) {
        data->insert(std::make_pair(r.key, new scoped_cJSON_t(new_val->as_json())));
        if (old_val->get_type() == ql::datum_t::R_NULL) {
            not_added = resp.add("inserted", num_records);
        } else {
            if (*old_val == *new_val) {
                not_added = resp.add("unchanged", num_records);
            } else {
                not_added = resp.add("replaced", num_records);
            }
        }
    } else if (new_val->get_type() == ql::datum_t::R_NULL) {
        if (old_val->get_type() == ql::datum_t::R_NULL) {
            not_added = resp.add("skipped", num_records);
        } else {
            not_added = resp.add("deleted", num_records);
        }
    } else {
        throw cannot_perform_query_exc_t(
            "value being inserted is neither an object nor an empty value");
    }

    guarantee(!not_added);
    resp->write_to_protobuf(res);
}

void NORETURN mock_namespace_interface_t::write_visitor_t::operator()(const rdb_protocol_t::batched_replaces_t &) {
    throw cannot_perform_query_exc_t("unimplemented");
}

void NORETURN mock_namespace_interface_t::write_visitor_t::operator()(const rdb_protocol_t::point_write_t &) {
    throw cannot_perform_query_exc_t("unimplemented");
}

void NORETURN mock_namespace_interface_t::write_visitor_t::operator()(const rdb_protocol_t::point_delete_t &) {
    throw cannot_perform_query_exc_t("unimplemented");
}

void NORETURN mock_namespace_interface_t::write_visitor_t::operator()(const rdb_protocol_t::sindex_create_t &) {
    throw cannot_perform_query_exc_t("unimplemented");
}

void NORETURN mock_namespace_interface_t::write_visitor_t::operator()(const rdb_protocol_t::sindex_drop_t &) {
    throw cannot_perform_query_exc_t("unimplemented");
}

mock_namespace_interface_t::write_visitor_t::write_visitor_t(std::map<store_key_t, scoped_cJSON_t*> *_data,
                                                             ql::env_t *_env,
                                                             rdb_protocol_t::write_response_t *_response) :
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
    cow_ptr_t<namespaces_semilattice_metadata_t<rdb_protocol_t> >::change_t change(&metadata.rdb_namespaces);
    if (!table_name_string.assign_value(table_name)) throw invalid_name_exc_t(table_name);
    namespace_id_t namespace_id = generate_uuid();
    *change.get()->namespaces[namespace_id].get_mutable() =
        new_namespace<rdb_protocol_t>(machine_id,
                                      db_id,
                                      nil_uuid(),
                                      table_name_string,
                                      primary_key,
                                      port_defaults::reql_port,
                                      GIGABYTE);

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
    namespaces_metadata(new semilattice_watchable_t<cow_ptr_t<namespaces_semilattice_metadata_t<rdb_protocol_t> > >(metadata_field(&cluster_semilattice_metadata_t::rdb_namespaces, dummy_semilattice_controller.get_view()))),
    databases_metadata(new semilattice_watchable_t<databases_semilattice_metadata_t>(metadata_field(&cluster_semilattice_metadata_t::databases, dummy_semilattice_controller.get_view()))),
    extproc_pool(2),
    test_cluster(0),
    rdb_ns_repo()
{
    env.init(new ql::env_t(&extproc_pool,
                           &rdb_ns_repo,
                           namespaces_metadata,
                           databases_metadata,
                           dummy_semilattice_controller.get_view(),
                           NULL,
                           &interruptor,
                           test_env->machine_id,
                           std::map<std::string, ql::wire_func_t>()));
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

