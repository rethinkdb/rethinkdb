// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_CONTEXT_HPP_
#define RDB_PROTOCOL_CONTEXT_HPP_

#include <set>
#include <string>

#include "errors.hpp"
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>

#include "concurrency/promise.hpp"
#include "containers/scoped.hpp"
#include "perfmon/perfmon.hpp"
#include "rdb_protocol/changefeed.hpp"

class auth_semilattice_metadata_t;
class extproc_pool_t;
class name_string_t;
class namespace_interface_t;
template <class> class semilattice_readwrite_view_t;
class uuid_u;

namespace ql {
class db_t : public single_threaded_countable_t<db_t> {
public:
    db_t(uuid_u _id, const std::string &_name) : id(_id), name(_name) { }
    const uuid_u id;
    const std::string name;
};
}   // namespace ql

class reql_admin_interface_t {
public:
    /* All of these methods return `true` on success and `false` on failure; if they
    fail, they will set `*error_out` to a description of the problem. They can all throw
    `interrupted_exc_t`.
    
    These methods are safe to call from any thread, and the calls can overlap
    concurrently in arbitrary ways. By the time a method returns, any changes it makes
    must be visible on every thread. */

    virtual bool db_create(const name_string_t &name,
            signal_t *interruptor, std::string *error_out) = 0; 
    virtual bool db_drop(const name_string_t &name,
            signal_t *interruptor, std::string *error_out) = 0;
    virtual bool db_list(
            signal_t *interruptor,
            std::set<name_string_t> *names_out, std::string *error_out) = 0;
    virtual bool db_find(const name_string_t &name,
            signal_t *interruptor,
            counted_t<const ql::db_t> *db_out, std::string *error_out) = 0;
    virtual bool table_create(const name_string_t &name, counted_t<const ql::db_t> db,
            const boost::optional<name_string_t> &primary_dc, bool hard_durability,
            const std::string &primary_key,
            signal_t *interruptor, uuid_u *namespace_id_out, std::string *error_out) = 0;
    virtual bool table_drop(const name_string_t &name, counted_t<const ql::db_t> db,
            signal_t *interruptor, std::string *error_out) = 0;
    virtual bool table_list(counted_t<const ql::db_t> db,
            signal_t *interruptor, std::set<name_string_t> *names_out,
            std::string *error_out) = 0;
    virtual bool table_find(const name_string_t &name, counted_t<const ql::db_t> db,
            signal_t *interruptor, uuid_u *namespace_id_out,
            std::string *primary_key_out, std::string *error_out) = 0;

protected:
    virtual ~reql_admin_interface_t() { }   // silence compiler warnings
};

class base_namespace_repo_t {
protected:
    struct namespace_cache_entry_t;

public:
    base_namespace_repo_t() { }
    virtual ~base_namespace_repo_t() { }

    class access_t {
    public:
        access_t();
        access_t(base_namespace_repo_t *parent, const uuid_u &ns_id,
                 signal_t *interruptor);
        access_t(const access_t &access);
        access_t &operator=(const access_t &access);

        namespace_interface_t *get_namespace_if();

    private:
        struct ref_handler_t {
        public:
            ref_handler_t();
            ~ref_handler_t();
            void init(namespace_cache_entry_t *_ref_target);
            void reset();
        private:
            namespace_cache_entry_t *ref_target;
        };
        namespace_cache_entry_t *cache_entry;
        ref_handler_t ref_handler;
        threadnum_t thread;
    };

    /* Tests whether a table with the given UUID exists or not. If you construct an
    `access_t` for a namespace that doesn't exist, you will still get a
    `namespace_interface_t` object, but any operations you try to run on it will fail. */
    virtual bool check_namespace_exists(const uuid_u &ns_id, signal_t *interruptor) = 0;

protected:
    virtual namespace_cache_entry_t *get_cache_entry(const uuid_u &ns_id) = 0;

    struct namespace_cache_entry_t {
    public:
        promise_t<namespace_interface_t *> namespace_if;
        int ref_count;
        cond_t *pulse_when_ref_count_becomes_zero;
        cond_t *pulse_when_ref_count_becomes_nonzero;
    };
};

class mailbox_manager_t;

class rdb_context_t {
public:
    // Used by unit tests.
    rdb_context_t();
    // Also used by unit tests.
    rdb_context_t(extproc_pool_t *_extproc_pool,
                  base_namespace_repo_t *_ns_repo,
                  reql_admin_interface_t *_reql_admin_interface);

    // The "real" constructor used outside of unit tests.
    rdb_context_t(extproc_pool_t *_extproc_pool,
                  mailbox_manager_t *_mailbox_manager,
                  base_namespace_repo_t *_ns_repo,
                  reql_admin_interface_t *_reql_admin_interface,
                  boost::shared_ptr<
                    semilattice_readwrite_view_t<
                        auth_semilattice_metadata_t> > _auth_metadata,
                  perfmon_collection_t *_global_stats,
                  const std::string &_reql_http_proxy);

    ~rdb_context_t();

    extproc_pool_t *extproc_pool;
    base_namespace_repo_t *ns_repo;
    reql_admin_interface_t *reql_admin_interface;

    boost::shared_ptr< semilattice_readwrite_view_t<auth_semilattice_metadata_t> >
        auth_metadata;

    mailbox_manager_t *manager;
    scoped_ptr_t<ql::changefeed::client_t> changefeed_client;

    perfmon_collection_t ql_stats_collection;
    perfmon_membership_t ql_stats_membership;
    perfmon_counter_t ql_ops_running;
    perfmon_membership_t ql_ops_running_membership;

    const std::string reql_http_proxy;

private:
    DISABLE_COPYING(rdb_context_t);
};

#endif /* RDB_PROTOCOL_CONTEXT_HPP_ */

