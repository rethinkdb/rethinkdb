// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/administration/tables/wait_for_readiness.hpp"

bool wait_for_table_readiness(
        const namespace_id_t &table_id,
        table_readiness_t readiness,
        namespace_repo_t *namespace_repo,
        table_meta_client_t *table_meta_client,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t, no_such_table_exc_t) {
    namespace_interface_access_t ns_if =
        namespace_repo->get_namespace_interface(table_id, interruptor);
    exponential_backoff_t backoff(100, 5000);
    bool immediate = true;
    while (true) {
        if (!table_meta_client->exists(table_id)) {
            throw no_such_table_exc_t();
        }
        bool ok = ns_if.get()->check_readiness(
            std::min(readiness, table_readiness_t::writes),
            interruptor);
        if (ok && readiness == table_readiness_t::finished) {
            try {
                table_meta_client->get_shard_status(table_id, interruptor, nullptr, &ok);
            } catch (const failed_table_op_exc_t &) {
                ok = false;
            }
        }
        if (ok) {
            return immediate;
        }
        immediate = false;
        backoff.failure(interruptor);
    }
}

void wait_for_many_tables_readiness(
        const std::set<namespace_id_t> &original_tables,
        table_readiness_t readiness,
        namespace_repo_t *namespace_repo,
        table_meta_client_t *table_meta_client,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {
    std::set<namespace_id_t> tables = original_tables;
    while (true) {
        bool all_immediate = true;
        for (auto it = tables.begin(); it != tables.end();) {
            try {
                all_immediate &= wait_for_table_readiness(
                    *it, readiness, namespace_repo, table_meta_client, interruptor);
            } catch (const no_such_table_exc_t &) {
                tables.erase(it++);
                continue;
            }
            ++it;
        }
        if (all_immediate) {
            break;
        }
    }
}

