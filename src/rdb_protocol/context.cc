// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "rdb_protocol/context.hpp"

#include "rdb_protocol/datum.hpp"

base_namespace_repo_t::access_t::access_t() :
    cache_entry(NULL),
    thread(INVALID_THREAD)
    { }

base_namespace_repo_t::access_t::access_t(base_namespace_repo_t *parent,
                                          const uuid_u &namespace_id,
                                          signal_t *interruptor) :
    thread(get_thread_id())
{
    {
        ASSERT_FINITE_CORO_WAITING;
        cache_entry = parent->get_cache_entry(namespace_id);
        ref_handler.init(cache_entry);
    }
    wait_interruptible(cache_entry->namespace_if.get_ready_signal(), interruptor);
}

base_namespace_repo_t::access_t::access_t(const access_t& access) :
    cache_entry(access.cache_entry),
    thread(access.thread)
{
    if (cache_entry) {
        rassert(get_thread_id() == thread);
        ref_handler.init(cache_entry);
    }
}

base_namespace_repo_t::access_t &base_namespace_repo_t::access_t::operator=(
        const access_t &access) {
    if (this != &access) {
        cache_entry = access.cache_entry;
        ref_handler.reset();
        if (access.cache_entry) {
            ref_handler.init(access.cache_entry);
        }
        thread = access.thread;
    }
    return *this;
}

namespace_interface_t *base_namespace_repo_t::access_t::get_namespace_if() {
    rassert(thread == get_thread_id());
    return cache_entry->namespace_if.wait();
}

base_namespace_repo_t::access_t::ref_handler_t::ref_handler_t() :
    ref_target(NULL) { }

base_namespace_repo_t::access_t::ref_handler_t::~ref_handler_t() {
    reset();
}

void base_namespace_repo_t::access_t::ref_handler_t::init(
        namespace_cache_entry_t *_ref_target) {
    ASSERT_NO_CORO_WAITING;
    guarantee(ref_target == NULL);
    ref_target = _ref_target;
    ref_target->ref_count++;
    if (ref_target->ref_count == 1) {
        if (ref_target->pulse_when_ref_count_becomes_nonzero) {
            ref_target->pulse_when_ref_count_becomes_nonzero->
                pulse_if_not_already_pulsed();
        }
    }
}

void base_namespace_repo_t::access_t::ref_handler_t::reset() {
    ASSERT_NO_CORO_WAITING;
    if (ref_target != NULL) {
        ref_target->ref_count--;
        if (ref_target->ref_count == 0) {
            if (ref_target->pulse_when_ref_count_becomes_zero) {
                ref_target->pulse_when_ref_count_becomes_zero->
                    pulse_if_not_already_pulsed();
            }
        }
    }
}

rdb_context_t::rdb_context_t()
    : extproc_pool(NULL),
      ns_repo(NULL), reql_admin_interface(NULL),
      manager(NULL),
      changefeed_client(NULL),
      ql_stats_membership(
          &get_global_perfmon_collection(), &ql_stats_collection, "query_language"),
      ql_ops_running_membership(&ql_stats_collection, &ql_ops_running, "ops_running"),
      reql_http_proxy()
{ }

rdb_context_t::rdb_context_t(
        extproc_pool_t *_extproc_pool,
        base_namespace_repo_t *_ns_repo,
        reql_admin_interface_t *_reql_admin_interface)
    : extproc_pool(_extproc_pool),
      ns_repo(_ns_repo), reql_admin_interface(_reql_admin_interface),
      manager(NULL),
      changefeed_client(NULL),
      ql_stats_membership(
          &get_global_perfmon_collection(), &ql_stats_collection, "query_language"),
      ql_ops_running_membership(&ql_stats_collection, &ql_ops_running, "ops_running"),
      reql_http_proxy()
{ }

rdb_context_t::rdb_context_t(
        extproc_pool_t *_extproc_pool,
        mailbox_manager_t *_mailbox_manager,
        base_namespace_repo_t *_ns_repo,
        reql_admin_interface_t *_reql_admin_interface,
        boost::shared_ptr< semilattice_readwrite_view_t<auth_semilattice_metadata_t> >
            _auth_metadata,
        perfmon_collection_t *_global_stats,
        const std::string &_reql_http_proxy)
    : extproc_pool(_extproc_pool),
      ns_repo(_ns_repo), reql_admin_interface(_reql_admin_interface),
      auth_metadata(_auth_metadata),
      manager(_mailbox_manager),
      changefeed_client(manager
                        ? make_scoped<ql::changefeed::client_t>(manager)
                        : scoped_ptr_t<ql::changefeed::client_t>()),
      ql_stats_membership(_global_stats, &ql_stats_collection, "query_language"),
      ql_ops_running_membership(&ql_stats_collection, &ql_ops_running, "ops_running"),
      reql_http_proxy(_reql_http_proxy)
{ }

rdb_context_t::~rdb_context_t() { }

