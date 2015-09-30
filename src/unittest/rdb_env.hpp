// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef UNITTEST_RDB_ENV_HPP_
#define UNITTEST_RDB_ENV_HPP_

#include <stdexcept>
#include <set>
#include <map>
#include <string>
#include <utility>

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
#include "rpc/directory/read_manager.hpp"
#include "rpc/directory/write_manager.hpp"
#include "rpc/semilattice/view/field.hpp"
#include "rpc/semilattice/watchable.hpp"
#include "unittest/dummy_metadata_controller.hpp"
#include "unittest/unittest_utils.hpp"

namespace unittest {

// These classes are used to provide a mock environment for running reql queries

// The mock namespace interface handles all read and write calls, using a simple in-
//  memory map of store_key_t to datum_t.  The get_data function allows a test to
//  read or modify the dataset to prepare for a query or to check that changes were made.
class mock_namespace_interface_t : public namespace_interface_t {
public:
    explicit mock_namespace_interface_t(datum_string_t _primary_key,
                                        std::map<store_key_t, ql::datum_t> &&_data,
                                        ql::env_t *_env);
    virtual ~mock_namespace_interface_t();

    void read(const read_t &query,
              read_response_t *response,
              UNUSED order_token_t tok,
              signal_t *interruptor) THROWS_ONLY(interrupted_exc_t, cannot_perform_query_exc_t);

    void write(const write_t &query,
               write_response_t *response,
               UNUSED order_token_t tok,
               signal_t *interruptor) THROWS_ONLY(interrupted_exc_t, cannot_perform_query_exc_t);

    std::set<region_t> get_sharding_scheme() THROWS_ONLY(cannot_perform_query_exc_t);

    bool check_readiness(table_readiness_t readiness, signal_t *interruptor);

    std::map<store_key_t, ql::datum_t> *get_data();

    std::string get_primary_key() const;

private:
    cond_t ready_cond;
    datum_string_t primary_key;
    std::map<store_key_t, ql::datum_t> data;
    ql::env_t *env;

    struct read_visitor_t : public boost::static_visitor<void> {
        void operator()(const point_read_t &get);
        void operator()(const dummy_read_t &d);
        void NORETURN operator()(const changefeed_subscribe_t &);
        void NORETURN operator()(const changefeed_limit_subscribe_t &);
        void NORETURN operator()(const changefeed_stamp_t &);
        void NORETURN operator()(const changefeed_point_stamp_t &);
        void NORETURN operator()(UNUSED const rget_read_t &rget);
        void NORETURN operator()(UNUSED const intersecting_geo_read_t &gr);
        void NORETURN operator()(UNUSED const nearest_geo_read_t &gr);
        void NORETURN operator()(UNUSED const distribution_read_t &dg);

        read_visitor_t(mock_namespace_interface_t *parent, read_response_t *_response);

        mock_namespace_interface_t *parent;
        read_response_t *response;
    };

    struct write_visitor_t : public boost::static_visitor<void> {
        void operator()(const batched_replace_t &br);
        void operator()(const batched_insert_t &br);
        void operator()(const dummy_write_t &d);
        void NORETURN operator()(UNUSED const point_write_t &w);
        void NORETURN operator()(UNUSED const point_delete_t &d);
        void NORETURN operator()(UNUSED const sync_t &s);

        write_visitor_t(mock_namespace_interface_t *parent, write_response_t *_response);

        mock_namespace_interface_t *parent;
        write_response_t *response;
    };
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

/* Because of how internal objects are meant to be instantiated, the proper order of
instantiation is to create a test_rdb_env_t at the top-level of the test (before entering
the thread pool), then to call make_env() on the object once inside the thread pool. From
there, the instance can provide a pointer to the ql::env_t. At the moment, metaqueries
will not work, but everything else should be good. That is, you can specify databases and
tables, but you can't create or destroy them using reql in this environment. As such, you
should create any necessary databases and tables BEFORE creating the instance_t by using
the add_table and add_database functions. */
class test_rdb_env_t {
public:
    test_rdb_env_t();
    ~test_rdb_env_t();

    void add_database(const std::string &db_name);
    void add_table(const std::string &db_name,
                   const std::string &table_name,
                   const std::string &primary_key);

    // The initial_data parameter allows a test to provide a starting dataset.  At
    // the moment, it just takes a set of maps of strings to strings, which will be
    // converted into a set of JSON structures.  This means that the JSON values will
    // only be strings, but if a test needs different properties in their objects,
    // this call should be modified.
    void add_table(const std::string &db_name,
                   const std::string &table_name,
                   const std::string &primary_key,
                   const std::set<ql::datum_t, optional_datum_less_t> &initial_data);

    class instance_t : private reql_cluster_interface_t {
    public:
        explicit instance_t(test_rdb_env_t &&test_env);

        ql::env_t *get_env();
        rdb_context_t *get_rdb_context();
        void interrupt();

        std::map<store_key_t, ql::datum_t> *get_data(name_string_t db,
                                                     name_string_t table);


        bool db_create(const name_string_t &name, signal_t *interruptor,
                ql::datum_t *result_out, admin_err_t *error_out);
        bool db_drop(const name_string_t &name, signal_t *interruptor,
                ql::datum_t *result_out, admin_err_t *error_out);
        bool db_list(
                signal_t *interruptor,
                std::set<name_string_t> *names_out, admin_err_t *error_out);
        bool db_find(const name_string_t &name,
                signal_t *interruptor,
                counted_t<const ql::db_t> *db_out, admin_err_t *error_out);
        bool db_config(
                const counted_t<const ql::db_t> &db,
                ql::backtrace_id_t bt,
                ql::env_t *env,
                scoped_ptr_t<ql::val_t> *selection_out,
                admin_err_t *error_out);

        bool table_create(const name_string_t &name, counted_t<const ql::db_t> db,
                const table_generate_config_params_t &config_params,
                const std::string &primary_key, write_durability_t durability,
                signal_t *interruptor, ql::datum_t *result_out, admin_err_t *error_out);
        bool table_drop(const name_string_t &name, counted_t<const ql::db_t> db,
                signal_t *interruptor, ql::datum_t *result_out, admin_err_t *error_out);
        bool table_list(counted_t<const ql::db_t> db,
                signal_t *interruptor,
                std::set<name_string_t> *names_out, admin_err_t *error_out);
        bool table_find(const name_string_t &name, counted_t<const ql::db_t> db,
                boost::optional<admin_identifier_format_t> identifier_format,
                signal_t *interruptor, counted_t<base_table_t> *table_out,
                admin_err_t *error_out);
        bool table_estimate_doc_counts(
                counted_t<const ql::db_t> db,
                const name_string_t &name,
                ql::env_t *env,
                std::vector<int64_t> *doc_counts_out,
                admin_err_t *error_out);
        bool table_config(
                counted_t<const ql::db_t> db,
                const name_string_t &name,
                ql::backtrace_id_t bt,
                ql::env_t *env,
                scoped_ptr_t<ql::val_t> *selection_out,
                admin_err_t *error_out);
        bool table_status(
                counted_t<const ql::db_t> db,
                const name_string_t &name,
                ql::backtrace_id_t bt,
                ql::env_t *env,
                scoped_ptr_t<ql::val_t> *selection_out,
                admin_err_t *error_out);

        bool table_wait(
                counted_t<const ql::db_t> db,
                const name_string_t &name,
                table_readiness_t readiness,
                signal_t *interruptor,
                ql::datum_t *result_out,
                admin_err_t *error_out);
        bool db_wait(
                counted_t<const ql::db_t> db,
                table_readiness_t readiness,
                signal_t *interruptor,
                ql::datum_t *result_out,
                admin_err_t *error_out);

        bool table_reconfigure(
                counted_t<const ql::db_t> db,
                const name_string_t &name,
                const table_generate_config_params_t &params,
                bool dry_run,
                signal_t *interruptor,
                ql::datum_t *result_out,
                admin_err_t *error_out);
        bool db_reconfigure(
                counted_t<const ql::db_t> db,
                const table_generate_config_params_t &params,
                bool dry_run,
                signal_t *interruptor,
                ql::datum_t *result_out,
                admin_err_t *error_out);

        bool table_emergency_repair(
                counted_t<const ql::db_t> db,
                const name_string_t &name,
                emergency_repair_mode_t,
                bool dry_run,
                signal_t *interruptor,
                ql::datum_t *result_out,
                admin_err_t *error_out);

        bool table_rebalance(
                counted_t<const ql::db_t> db,
                const name_string_t &name,
                signal_t *interruptor,
                ql::datum_t *result_out,
                admin_err_t *error_out);
        bool db_rebalance(
                counted_t<const ql::db_t> db,
                signal_t *interruptor,
                ql::datum_t *result_out,
                admin_err_t *error_out);

        bool sindex_create(
                counted_t<const ql::db_t> db,
                const name_string_t &table,
                const std::string &name,
                const sindex_config_t &config,
                signal_t *interruptor,
                admin_err_t *error_out);
        bool sindex_drop(
                counted_t<const ql::db_t> db,
                const name_string_t &table,
                const std::string &name,
                signal_t *interruptor,
                admin_err_t *error_out);
        bool sindex_rename(
                counted_t<const ql::db_t> db,
                const name_string_t &table,
                const std::string &name,
                const std::string &new_name,
                bool overwrite,
                signal_t *interruptor,
                admin_err_t *error_out);
        bool sindex_list(
                counted_t<const ql::db_t> db,
                const name_string_t &table,
                signal_t *interruptor,
                admin_err_t *error_out,
                std::map<std::string, std::pair<sindex_config_t, sindex_status_t> >
                    *configs_and_statuses_out);

    private:
        extproc_pool_t extproc_pool;
        rdb_context_t rdb_ctx;
        dummy_semilattice_controller_t<auth_semilattice_metadata_t> auth_manager;
        std::map<name_string_t, database_id_t> databases;
        std::map<std::pair<database_id_t, name_string_t>,
                 scoped_ptr_t<mock_namespace_interface_t> > tables;
        scoped_ptr_t<ql::env_t> env;
        cond_t interruptor;
    };

    scoped_ptr_t<instance_t> make_env();

private:
    extproc_spawner_t extproc_spawner;

    struct table_data_t {
        datum_string_t primary_key;
        std::map<store_key_t, ql::datum_t> initial_data;
    };

    std::set<name_string_t> databases;

    // Initial data for tables are stored here until the instance_t is constructed, at
    //  which point, it is moved into a mock_namespace_interface_t, and this is cleared.
    std::map<std::pair<name_string_t, name_string_t>, table_data_t> tables;
};

}  // namespace unittest

#endif // UNITTEST_RDB_ENV_HPP_

