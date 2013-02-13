// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef UNITTEST_RDB_ENV_HPP_
#define UNITTEST_RDB_ENV_HPP_

#include "errors.hpp"
#include <boost/variant.hpp>

#include "unittest/test_cluster_group.hpp"
#include "unittest/unittest_utils.hpp"
#include "unittest/dummy_metadata_controller.hpp"
#include "concurrency/watchable.hpp"
#include "concurrency/cross_thread_watchable.hpp"
#include "rpc/semilattice/watchable.hpp"
#include "rpc/semilattice/view/field.hpp"
#include "rpc/directory/read_manager.hpp"
#include "rpc/directory/write_manager.hpp"
#include "clustering/administration/main/ports.hpp"
#include "rpc/connectivity/multiplexer.hpp"
#include "rdb_protocol/env.hpp"
#include "clustering/administration/main/watchable_fields.hpp"
#include <stdexcept>
#include <map>

namespace unittest {

class mock_namespace_repo_t;

class mock_namespace_interface_t : public namespace_interface_t<rdb_protocol_t> {
private:
    std::map<store_key_t, scoped_cJSON_t*> data;
    mock_namespace_repo_t *parent;
    ql::env_t *env;

public:
    mock_namespace_interface_t(mock_namespace_repo_t *_parent);
    virtual ~mock_namespace_interface_t();

    void read(const typename rdb_protocol_t::read_t &query,
              typename rdb_protocol_t::read_response_t *response,
              UNUSED order_token_t tok,
              signal_t *interruptor) THROWS_ONLY(interrupted_exc_t, cannot_perform_query_exc_t);

    void read_outdated(const typename rdb_protocol_t::read_t &query,
                       typename rdb_protocol_t::read_response_t *response,
                       signal_t *interruptor) THROWS_ONLY(interrupted_exc_t, cannot_perform_query_exc_t);

    void write(const typename rdb_protocol_t::write_t &query,
               typename rdb_protocol_t::write_response_t *response,
               UNUSED order_token_t tok,
               signal_t *interruptor) THROWS_ONLY(interrupted_exc_t, cannot_perform_query_exc_t);

    std::map<store_key_t, scoped_cJSON_t*>* get_data();

private:
    cond_t ready_cond;

    struct read_visitor_t : public boost::static_visitor<void> {
        void operator()(const rdb_protocol_t::point_read_t &get);
        void operator()(UNUSED const rdb_protocol_t::rget_read_t &rget);
        void operator()(UNUSED const rdb_protocol_t::distribution_read_t &dg);

        read_visitor_t(std::map<store_key_t, scoped_cJSON_t*> *_data, rdb_protocol_t::read_response_t *_response);

        std::map<store_key_t, scoped_cJSON_t*> *data;
        rdb_protocol_t::read_response_t *response;
    };

    struct write_visitor_t : public boost::static_visitor<void> {
        void operator()(const rdb_protocol_t::point_replace_t &r);
        void operator()(UNUSED const rdb_protocol_t::point_write_t &w);
        void operator()(UNUSED const rdb_protocol_t::point_delete_t &d);

        write_visitor_t(std::map<store_key_t, scoped_cJSON_t*> *_data, ql::env_t *_env, rdb_protocol_t::write_response_t *_response);

        std::map<store_key_t, scoped_cJSON_t*> *data;
        ql::env_t *env;
        rdb_protocol_t::write_response_t *response;
    };
};

// This stuff isn't really designed for concurrency, use for single-threaded tests
class mock_namespace_repo_t : public base_namespace_repo_t<rdb_protocol_t> {
public:
    mock_namespace_repo_t();
    virtual ~mock_namespace_repo_t();

    void set_env(ql::env_t *_env);
    ql::env_t *get_env();

    mock_namespace_interface_t* get_ns_if(const namespace_id_t &ns_id);

private:
    namespace_cache_entry_t *get_cache_entry(const namespace_id_t &ns_id);

    struct mock_namespace_cache_entry_t {
        mock_namespace_cache_entry_t(mock_namespace_repo_t *ns_repo) : 
            mock_ns_if(ns_repo) { }
        namespace_cache_entry_t entry;
        mock_namespace_interface_t mock_ns_if;
    };

    ql::env_t *env;
    std::map<namespace_id_t, mock_namespace_cache_entry_t*> cache;
};

class invalid_name_exc_t : public std::exception {
public:
    invalid_name_exc_t(const std::string& name) :
        error_string(strprintf("invalid name string: %s", name.c_str())) { }
    ~invalid_name_exc_t() throw () { }
    const char *what() const throw () {
        return error_string.c_str();
    }
private:
    const std::string error_string;
};

class test_rdb_env_t {
public:
    test_rdb_env_t();
    ~test_rdb_env_t();

    namespace_id_t add_metadata_table(const std::string &table_name,
                                      const uuid_u &db_id,
                                      const std::string &primary_key,
                                      const std::set<std::map<std::string, std::string> > &initial_data =
                                          std::set<std::map<std::string, std::string> >());
    database_id_t add_metadata_database(const std::string &db_name);

    class instance_t {
    public:
        instance_t(test_rdb_env_t *test_env);

        extproc::pool_group_t *create_pool_group(test_rdb_env_t *test_env);
        ql::env_t *get();
        void interrupt();

        std::map<store_key_t, scoped_cJSON_t*>* get_data(const namespace_id_t &ns_id);

    private:
        dummy_semilattice_controller_t<cluster_semilattice_metadata_t> dummy_semilattice_controller;
        clone_ptr_t<semilattice_watchable_t<cow_ptr_t<namespaces_semilattice_metadata_t<rdb_protocol_t> > > > namespaces_metadata;
        clone_ptr_t<semilattice_watchable_t<databases_semilattice_metadata_t> > databases_metadata;
        scoped_ptr_t<extproc::pool_group_t> pool_group;
        scoped_ptr_t<ql::env_t> env;
        reactor_test_cluster_t<rdb_protocol_t> test_cluster;
        mock_namespace_repo_t rdb_ns_repo;
        cond_t interruptor;
    };

    void make_env(scoped_ptr_t<instance_t> *instance_out);

private:
    friend class instance_t;

    extproc::spawner_info_t spawner_info;

    uuid_u machine_id;
    boost::shared_ptr<js::runner_t> js_runner;
    cluster_semilattice_metadata_t metadata;
    std::map<namespace_id_t, std::map<store_key_t, scoped_cJSON_t*>*> initial_datas;
};

}

#endif // UNITTEST_RDB_ENV_HPP_
