// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "unittest/gtest.hpp"
#include "unittest/unittest_utils.hpp"

#include "unittest/dummy_metadata_controller.hpp"
#include "unittest/dummy_directory_manager.hpp"
#include "clustering/administration/metadata.hpp"
#include "clustering/administration/issues/issues_backend.hpp"
#include "clustering/administration/issues/local_issue_aggregator.hpp"
#include "clustering/administration/issues/server.hpp"
#include "clustering/administration/issues/log_write.hpp"
#include "clustering/administration/issues/outdated_index.hpp"

namespace unittest {

class issues_environment_t {
public:
    issues_environment_t();

    server_id_t local_server_id;

    dummy_semilattice_controller_t<cluster_semilattice_metadata_t> cluster_metadata;

    watchable_variable_t<cluster_directory_metadata_t> directory_entry;
    dummy_directory_manager_t<cluster_directory_metadata_t> directory_metadata;

    issues_artificial_table_backend_t issues_backend_name, issues_backend_uuid;
    local_issue_aggregator_t local_issue_aggregator;
    log_write_issue_tracker_t log_write_issue_tracker;
    outdated_index_issue_tracker_t outdated_index_issue_tracker;

    // Handles syncing of local issues into the directory
    field_copier_t<local_issues_t, cluster_directory_metadata_t> local_issue_copier;
};

cluster_directory_metadata_t default_directory_entry(const server_id_t &server_id) {
    cluster_directory_metadata_t entry;
    entry.server_id = server_id;
    entry.peer_id = peer_id_t(generate_uuid());
    entry.version = RETHINKDB_VERSION_STR;
    entry.time_started = current_microtime();
    entry.pid = getpid();
    entry.hostname = str_gethostname();
    entry.cluster_port = 0;
    entry.reql_port = 0;
    entry.peer_type = SERVER_PEER;

    return entry;
}

template <typename T>
void mark_id_deleted(T *map, const uuid_u &id) {
    auto id_it = map->find(id);
    EXPECT_TRUE(id_it != map->end());
    id_it->second.mark_deleted();
}

server_id_t add_server(issues_environment_t *env, const std::string &name) {
    auto view = env->cluster_metadata.get_view();
    cluster_semilattice_metadata_t metadata = view->get();
    server_id_t new_server_id = generate_uuid();
    server_semilattice_metadata_t new_server;
    new_server.name.set(name_string_t::guarantee_valid(name.c_str()));
    *metadata.servers.servers[new_server_id].get_mutable() = new_server;
    view->join(metadata);
    return new_server_id;
}

void delete_server(issues_environment_t *env, const server_id_t &id) {
    auto view = env->cluster_metadata.get_view();
    cluster_semilattice_metadata_t metadata = view->get();
    mark_id_deleted(&metadata.servers.servers, id);
    view->join(metadata);
}

database_id_t add_database(issues_environment_t *env, const std::string &name) {
    auto view = env->cluster_metadata.get_view();
    cluster_semilattice_metadata_t metadata = view->get();
    database_id_t new_db_id = generate_uuid();
    database_semilattice_metadata_t new_database;
    new_database.name.set(name_string_t::guarantee_valid(name.c_str()));
    *metadata.databases.databases[new_db_id].get_mutable() = new_database;
    view->join(metadata);
    return new_db_id;
}

void delete_database(issues_environment_t *env, const database_id_t &id) {
    auto view = env->cluster_metadata.get_view();
    cluster_semilattice_metadata_t metadata = view->get();
    mark_id_deleted(&metadata.databases.databases, id);
    view->join(metadata);
}

namespace_id_t add_table(issues_environment_t *env,
                         const std::string &name,
                         const database_id_t &db) {
    auto view = env->cluster_metadata.get_view();
    cluster_semilattice_metadata_t metadata = view->get();
    namespace_id_t new_table_id = generate_uuid();
    {
        cow_ptr_t<namespaces_semilattice_metadata_t>::change_t
            namespaces_change(&metadata.rdb_namespaces);
        namespace_semilattice_metadata_t new_table;
        new_table.name.set(name_string_t::guarantee_valid(name.c_str()));
        new_table.database.set(db);
        new_table.primary_key.set("dummy");
        *namespaces_change.get()->namespaces[new_table_id].get_mutable() = new_table;
    }
    view->join(metadata);
    return new_table_id;
}

void delete_table(issues_environment_t *env, const namespace_id_t &id) {
    auto view = env->cluster_metadata.get_view();
    cluster_semilattice_metadata_t metadata = view->get();
    {
        cow_ptr_t<namespaces_semilattice_metadata_t>::change_t
            namespaces_change(&metadata.rdb_namespaces);
        mark_id_deleted(&namespaces_change.get()->namespaces, id);
    }
    view->join(metadata);
}

issues_environment_t::issues_environment_t() :
    local_server_id(generate_uuid()),
    cluster_metadata(cluster_semilattice_metadata_t()),
    directory_entry(default_directory_entry(local_server_id)),
    directory_metadata(directory_entry.get_watchable()->get().peer_id,
                       directory_entry.get_watchable()),
    issues_backend_name(
        cluster_metadata.get_view(),
        directory_metadata.get_view(),
        /* We don't test any types of issues that need the `server_config_client_t` */
        nullptr,
        admin_identifier_format_t::name),
    issues_backend_uuid(
        cluster_metadata.get_view(),
        directory_metadata.get_view(),
        nullptr,
        admin_identifier_format_t::uuid),
    log_write_issue_tracker(&local_issue_aggregator),
    outdated_index_issue_tracker(&local_issue_aggregator),
    local_issue_copier(&cluster_directory_metadata_t::local_issues,
                       local_issue_aggregator.get_issues_watchable(),
                       &directory_entry)
{
    // Add an entry for us into the cluster semilattice metadata
    auto view = cluster_metadata.get_view();
    cluster_semilattice_metadata_t metadata = view->get();
    server_semilattice_metadata_t new_server;
    new_server.name.set(name_string_t::guarantee_valid("self"));
    *metadata.servers.servers[local_server_id].get_mutable() = new_server;
    view->join(metadata);
}

void assert_no_issues(issues_environment_t *env) {
    std::string error;
    cond_t interruptor;

    std::vector<ql::datum_t> rows;
    bool res = env->issues_backend_name.read_all_rows_as_vector(
        &interruptor, &rows, &error);
    ASSERT_TRUE(res);
    ASSERT_EQ(static_cast<size_t>(0), rows.size());
}

void assert_one_issue(
        issues_environment_t *env,
        admin_identifier_format_t identifier_format,
        ql::datum_t *issue_out) {
    std::string error;
    cond_t interruptor;

    artificial_table_backend_t *backend =
        (identifier_format == admin_identifier_format_t::name)
            ? &env->issues_backend_name
            : &env->issues_backend_uuid;
    std::vector<ql::datum_t> rows;
    bool res = backend->read_all_rows_as_vector(&interruptor, &rows, &error);
    ASSERT_TRUE(res);
    ASSERT_EQ(static_cast<size_t>(1), rows.size());

    *issue_out = rows.front();
}

TPTEST(ClusteringIssues, OutdatedIndex) {
    issues_environment_t env;
    assert_no_issues(&env);

    // Set up error case
    std::string table_name("defunct");
    std::string db_name("foo");
    database_id_t db_id = add_database(&env, db_name);
    namespace_id_t table_id = add_table(&env, table_name, db_id);
    outdated_index_report_t *index_report =
        env.outdated_index_issue_tracker.create_report(table_id);
    index_report->set_outdated_indexes(
        std::set<std::string>({"one", "two", "three"}));

    let_stuff_happen();
    ql::datum_t issue_with_names, issue_with_uuids;
    assert_one_issue(&env, admin_identifier_format_t::name, &issue_with_names);
    assert_one_issue(&env, admin_identifier_format_t::uuid, &issue_with_uuids);
    ASSERT_TRUE(issue_with_names.has() && issue_with_uuids.has());

    // Check issue data
    ASSERT_FALSE(issue_with_names.get_field("critical").as_bool());
    ASSERT_EQ(std::string("outdated_index"),
        issue_with_names.get_field("type").as_str().to_std());

    ASSERT_EQ(static_cast<size_t>(1),
        issue_with_names.get_field("info").get_field("tables").arr_size());
    ql::datum_t table_with_names =
        issue_with_names.get_field("info").get_field("tables").get(0);
    ASSERT_EQ(db_name,
        table_with_names.get_field("db").as_str().to_std());
    ASSERT_EQ(table_name,
        table_with_names.get_field("table").as_str().to_std());
    ASSERT_EQ(static_cast<size_t>(3),
        table_with_names.get_field("indexes").arr_size());

    ASSERT_EQ(static_cast<size_t>(1),
        issue_with_uuids.get_field("info").get_field("tables").arr_size());
    ql::datum_t table_with_uuids =
        issue_with_uuids.get_field("info").get_field("tables").get(0);
    ASSERT_EQ(uuid_to_str(db_id),
        table_with_uuids.get_field("db").as_str().to_std());
    ASSERT_EQ(uuid_to_str(table_id),
        table_with_uuids.get_field("table").as_str().to_std());

    // Clear error case
    index_report->destroy();

    let_stuff_happen();
    assert_no_issues(&env);
}

TPTEST(ClusteringIssues, DatabaseNameCollision) {
    issues_environment_t env;
    assert_no_issues(&env);

    // Set up error case
    std::string collided_name("nyx");
    database_id_t db1 = add_database(&env, collided_name);
    database_id_t db2 = add_database(&env, collided_name);

    let_stuff_happen();
    ql::datum_t issue;
    assert_one_issue(&env, admin_identifier_format_t::name, &issue);
    ASSERT_TRUE(issue.has());

    // Check issue data
    ASSERT_TRUE(issue.get_field("critical").as_bool());
    ASSERT_EQ(std::string("db_name_collision"), issue.get_field("type").as_str().to_std());

    ql::datum_t info = issue.get_field("info");
    ASSERT_EQ(collided_name, info.get_field("name").as_str().to_std());

    ql::datum_t ids = info.get_field("ids");
    ASSERT_EQ(static_cast<size_t>(2), ids.arr_size());
    ASSERT_EQ(std::set<std::string>({ uuid_to_str(db1), uuid_to_str(db2) }),
              std::set<std::string>({ ids.get(0).as_str().to_std(),
                                      ids.get(1).as_str().to_std() }));

    // Clear error case
    delete_database(&env, db2);

    let_stuff_happen();
    assert_no_issues(&env);
}

TPTEST(ClusteringIssues, TableNameCollision) {
    issues_environment_t env;
    assert_no_issues(&env);

    // Make sure name collisions don't happen cross-database
    std::string collided_name("nyx");
    database_id_t db1 = add_database(&env, "db1");
    database_id_t db2 = add_database(&env, "db2");
    UNUSED namespace_id_t table1 = add_table(&env, collided_name, db1);
    UNUSED namespace_id_t table2 = add_table(&env, collided_name, db2);

    let_stuff_happen();
    assert_no_issues(&env);

    // Set up error case
    namespace_id_t table3 = add_table(&env, collided_name, db1);

    let_stuff_happen();
    ql::datum_t issue;
    assert_one_issue(&env, admin_identifier_format_t::name, &issue);
    ASSERT_TRUE(issue.has());

    // Check issue data
    ASSERT_TRUE(issue.get_field("critical").as_bool());
    ASSERT_EQ(std::string("table_name_collision"), issue.get_field("type").as_str().to_std());

    ql::datum_t info = issue.get_field("info");
    ASSERT_EQ(collided_name, info.get_field("name").as_str().to_std());
    ASSERT_EQ("db1", info.get_field("db").as_str().to_std());

    ql::datum_t ids = info.get_field("ids");
    ASSERT_EQ(static_cast<size_t>(2), ids.arr_size());
    ASSERT_EQ(std::set<std::string>({ uuid_to_str(table1), uuid_to_str(table3) }),
              std::set<std::string>({ ids.get(0).as_str().to_std(),
                                      ids.get(1).as_str().to_std() }));

    // Clear error case
    delete_table(&env, table3);

    let_stuff_happen();
    assert_no_issues(&env);
}

TPTEST(ClusteringIssues, ServerNameCollision) {
    issues_environment_t env;
    assert_no_issues(&env);

    // Set up error case
    std::string collided_name("nyx");
    server_id_t server1 = add_server(&env, collided_name);
    server_id_t server2 = add_server(&env, collided_name);

    peer_id_t server1_peer = peer_id_t(generate_uuid());
    peer_id_t server2_peer = peer_id_t(generate_uuid());
    env.directory_metadata.set_peer_value(server1_peer,
                                          default_directory_entry(server1));
    env.directory_metadata.set_peer_value(server2_peer,
                                          default_directory_entry(server2));

    let_stuff_happen();
    ql::datum_t issue;
    assert_one_issue(&env, admin_identifier_format_t::name, &issue);
    ASSERT_TRUE(issue.has());

    // Check issue data
    ASSERT_TRUE(issue.get_field("critical").as_bool());
    ASSERT_EQ(std::string("server_name_collision"), issue.get_field("type").as_str().to_std());

    ql::datum_t info = issue.get_field("info");
    ASSERT_EQ(collided_name, info.get_field("name").as_str().to_std());

    ql::datum_t ids = info.get_field("ids");
    ASSERT_EQ(static_cast<size_t>(2), ids.arr_size());
    ASSERT_EQ(std::set<std::string>({ uuid_to_str(server1), uuid_to_str(server2) }),
              std::set<std::string>({ ids.get(0).as_str().to_std(),
                                      ids.get(1).as_str().to_std() }));

    // Clear error case
    env.directory_metadata.delete_peer(server2_peer);
    delete_server(&env, server2);

    let_stuff_happen();
    assert_no_issues(&env);
}

}   /* namespace unittest */
