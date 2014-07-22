// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef UNITTEST_RDB_ENV_HPP_
#define UNITTEST_RDB_ENV_HPP_

#include <stdexcept>
#include <set>
#include <map>
#include <string>

#include "errors.hpp"
#include <boost/variant.hpp>

#include "clustering/administration/main/ports.hpp"
#include "clustering/administration/main/watchable_fields.hpp"
#include "clustering/administration/metadata.hpp"
#include "concurrency/cross_thread_watchable.hpp"
#include "concurrency/watchable.hpp"
#include "extproc/extproc_pool.hpp"
#include "extproc/extproc_spawner.hpp"
#include "rdb_protocol/env.hpp"
#include "rpc/connectivity/multiplexer.hpp"
#include "rpc/directory/read_manager.hpp"
#include "rpc/directory/write_manager.hpp"
#include "rpc/semilattice/view/field.hpp"
#include "rpc/semilattice/watchable.hpp"
#include "unittest/dummy_metadata_controller.hpp"
#include "unittest/test_cluster_group.hpp"
#include "unittest/unittest_utils.hpp"

namespace unittest {

// These classes are used to provide a mock environment for running reql queries

class mock_namespace_repo_t;

// The mock namespace interface handles all read and write calls, using a simple in-
//  memory map of store_key_t to scoped_cJSON_t.  The get_data function allows a test to
//  read or modify the dataset to prepare for a query or to check that changes were made.
class mock_namespace_interface_t : public namespace_interface_t {
private:
    std::map<store_key_t, scoped_cJSON_t *> data;
    mock_namespace_repo_t *parent;

public:
    explicit mock_namespace_interface_t(mock_namespace_repo_t *_parent);
    virtual ~mock_namespace_interface_t();

    void read(const read_t &query,
              read_response_t *response,
              UNUSED order_token_t tok,
              signal_t *interruptor) THROWS_ONLY(interrupted_exc_t, cannot_perform_query_exc_t);

    void read_outdated(const read_t &query,
                       read_response_t *response,
                       signal_t *interruptor) THROWS_ONLY(interrupted_exc_t, cannot_perform_query_exc_t);

    void write(const write_t &query,
               write_response_t *response,
               UNUSED order_token_t tok,
               signal_t *interruptor) THROWS_ONLY(interrupted_exc_t, cannot_perform_query_exc_t);

    std::map<store_key_t, scoped_cJSON_t *> *get_data();

    std::set<region_t> get_sharding_scheme() THROWS_ONLY(cannot_perform_query_exc_t);

private:
    cond_t ready_cond;

    struct read_visitor_t : public boost::static_visitor<void> {
        void operator()(const point_read_t &get);
        void NORETURN operator()(const changefeed_subscribe_t &);
        void NORETURN operator()(const changefeed_stamp_t &);
        void NORETURN operator()(UNUSED const rget_read_t &rget);
        void NORETURN operator()(UNUSED const distribution_read_t &dg);
        void NORETURN operator()(UNUSED const sindex_list_t &sl);
        void NORETURN operator()(UNUSED const sindex_status_t &ss);

        read_visitor_t(std::map<store_key_t, scoped_cJSON_t *> *_data, read_response_t *_response);

        std::map<store_key_t, scoped_cJSON_t *> *data;
        read_response_t *response;
    };

    struct write_visitor_t : public boost::static_visitor<void> {
        void operator()(const batched_replace_t &br);
        void operator()(const batched_insert_t &br);
        void NORETURN operator()(UNUSED const point_write_t &w);
        void NORETURN operator()(UNUSED const point_delete_t &d);
        void NORETURN operator()(UNUSED const sindex_create_t &s);
        void NORETURN operator()(UNUSED const sindex_drop_t &s);
        void NORETURN operator()(UNUSED const sync_t &s);

        write_visitor_t(std::map<store_key_t, scoped_cJSON_t *> *_data, ql::env_t *_env, write_response_t *_response);

        std::map<store_key_t, scoped_cJSON_t *> *data;
        ql::env_t *env;
        write_response_t *response;
    };
};

// This stuff isn't really designed for concurrency, use for single-threaded tests
class mock_namespace_repo_t : public base_namespace_repo_t {
public:
    mock_namespace_repo_t();
    virtual ~mock_namespace_repo_t();

    void set_env(ql::env_t *_env);
    ql::env_t *get_env();

    mock_namespace_interface_t *get_ns_if(const namespace_id_t &ns_id);

    bool check_namespace_exists(UNUSED const namespace_id_t &ns_id,
                                UNUSED signal_t *interruptor) {
        /* The `mock_namespace_repo_t` creates namespaces on the fly, so they always
        exist */
        return true;
    }

private:
    namespace_cache_entry_t *get_cache_entry(const namespace_id_t &ns_id);

    struct mock_namespace_cache_entry_t {
        explicit mock_namespace_cache_entry_t(mock_namespace_repo_t *ns_repo) :
            mock_ns_if(ns_repo) { }
        namespace_cache_entry_t entry;
        mock_namespace_interface_t mock_ns_if;
    };

    ql::env_t *env;
    std::map<namespace_id_t, mock_namespace_cache_entry_t *> cache;
};

/* This is read-only; it will give an error if you call the `db_create()`,
`table_create()`, etc. methods. The only way to create databases and tables is by
modifying the `databases` and `tables` fields directly. */
class mock_reql_admin_interface_t : public reql_admin_interface_t {
public:
    bool db_create(const name_string_t &name,
            signal_t *interruptor, std::string *error_out); 
    bool db_drop(const name_string_t &name,
            signal_t *interruptor, std::string *error_out);
    bool db_list(
            signal_t *interruptor, std::set<name_string_t> *names_out,
            std::string *error_out);
    bool db_find(const name_string_t &name,
            signal_t *interruptor, counted_t<const ql::db_t> *db_out,
            std::string *error_out);

    bool table_create(const name_string_t &name, counted_t<const ql::db_t> db,
            const boost::optional<name_string_t> &primary_dc, bool hard_durability,
            const std::string &primary_key,
            signal_t *interruptor, uuid_u *namespace_id_out, std::string *error_out);
    bool table_drop(const name_string_t &name, counted_t<const ql::db_t> db,
            signal_t *interruptor, std::string *error_out);
    bool table_list(counted_t<const ql::db_t> db,
            signal_t *interruptor, std::set<name_string_t> *names_out,
            std::string *error_out);
    bool table_find(const name_string_t &name, counted_t<const ql::db_t> db,
            signal_t *interruptor, uuid_u *id_out, std::string *primary_key_out,
            std::string *error_out);

    std::map<name_string_t, database_id_t> databases;
    std::map<std::pair<database_id_t, name_string_t>,
        std::pair<namespace_id_t, std::string> > tables;
};

class invalid_name_exc_t : public std::exception {
public:
    explicit invalid_name_exc_t(const std::string& name) :
        error_string(strprintf("invalid name string: %s", name.c_str())) { }
    ~invalid_name_exc_t() throw () { }
    const char *what() const throw () {
        return error_string.c_str();
    }
private:
    const std::string error_string;
};

// Because of how internal objects are meant to be instantiated, the proper order of
// instantiation is to create a test_rdb_env_t at the top-level of the test (before
// entering the thread pool), then to call make_env() on the object once inside the
// thread pool.  From there, the instance can provide a pointer to the rdb_env_t.  At
// the moment, this mocks everything except the directory (which is a huge bitch to
// do, but you're welcome to try), so metaqueries will not work, but everything else
// should be good.  That is, you can specify databases and tables, but you can't
// create or destroy them using reql in this environment.  As such, you should create
// any necessary databases and tables BEFORE creating the instance_t by using the
// add_table and add_database functions.
class test_rdb_env_t : private mock_reql_admin_interface_t {
public:
    test_rdb_env_t();
    ~test_rdb_env_t();

    // The initial_data parameter allows a test to provide a starting dataset.  At
    // the moment, it just takes a set of maps of strings to strings, which will be
    // converted into a set of JSON structures.  This means that the JSON values will
    // only be strings, but if a test needs different properties in their objects,
    // this call should be modified.
    namespace_id_t add_table(const std::string &table_name,
                             const uuid_u &db_id,
                             const std::string &primary_key,
                             const std::set<std::map<std::string, std::string> > &initial_data);
    database_id_t add_database(const std::string &db_name);

    class instance_t {
    public:
        explicit instance_t(test_rdb_env_t *test_env);

        ql::env_t *get();
        void interrupt();

        std::map<store_key_t, scoped_cJSON_t *> *get_data(const namespace_id_t &ns_id);

    private:
        extproc_pool_t extproc_pool;
        reactor_test_cluster_t test_cluster;
        mock_namespace_repo_t rdb_ns_repo;
        rdb_context_t rdb_ctx;
        scoped_ptr_t<ql::env_t> env;
        cond_t interruptor;
    };

    scoped_ptr_t<instance_t> make_env();

private:
    extproc_spawner_t extproc_spawner;

    mock_reql_admin_interface_t reql_admin_interface;

    // Initial data for tables are stored here until the instance_t is constructed, at
    //  which point, it is moved into a mock_namespace_interface_t, and this is cleared.
    std::map<namespace_id_t, std::map<store_key_t, scoped_cJSON_t *> *> initial_datas;
};

}

#endif // UNITTEST_RDB_ENV_HPP_
