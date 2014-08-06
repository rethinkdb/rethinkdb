// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "rdb_protocol/artificial_table/artificial_table.hpp"

#include "rdb_protocol/artificial_table/backend.hpp"
#include "rdb_protocol/env.hpp"
#include "rdb_protocol/func.hpp"

artificial_table_t::artificial_table_t(artificial_table_backend_t *_backend) :
    backend(_backend), primary_key(backend->get_primary_key_name()) { }

const std::string &artificial_table_t::get_pkey() {
    return primary_key;
}

counted_t<const ql::datum_t> artificial_table_t::read_row(ql::env_t *env,
        counted_t<const ql::datum_t> pval, UNUSED bool use_outdated) {
    counted_t<const ql::datum_t> row;
    std::string error;
    if (!backend->read_row(pval, env->interruptor, &row, &error)) {
        throw ql::datum_exc_t(ql::base_exc_t::GENERIC, error);
    }
    if (row.has()) {
#ifndef NDEBUG
        counted_t<const ql::datum_t> pval2 = row->get(get_pkey(), ql::NOTHROW);
        rassert(pval2.has());
        rassert(pval2->print_primary() == pval->print_primary());
#endif /* NDEBUG */
        return row;
    } else {
        return ql::datum_t::null();
    }
}

counted_t<ql::datum_stream_t> artificial_table_t::read_all(
        UNUSED ql::env_t *env,
        UNUSED const std::string &get_all_sindex_id,
        UNUSED const ql::protob_t<const Backtrace> &bt,
        UNUSED const std::string &table_name,
        UNUSED const datum_range_t &range,
        UNUSED sorting_t sorting,
        UNUSED bool use_outdated) {
    rfail_datum(ql::base_exc_t::GENERIC,
        "Artificial tables currently only support point read and replace.");
}

counted_t<ql::datum_stream_t> artificial_table_t::read_row_changes(
        UNUSED ql::env_t *env,
        UNUSED counted_t<const ql::datum_t> pval,
        UNUSED const ql::protob_t<const Backtrace> &bt,
        UNUSED const std::string &table_name) {
    rfail_datum(ql::base_exc_t::GENERIC,
        "Artificial tables currently only support point read and replace.");
}

counted_t<ql::datum_stream_t> artificial_table_t::read_all_changes(
        UNUSED ql::env_t *env,
        UNUSED const ql::protob_t<const Backtrace> &bt,
        UNUSED const std::string &table_name) {
    rfail_datum(ql::base_exc_t::GENERIC,
        "Artificial tables currently only support point read and replace.");
}

counted_t<const ql::datum_t> artificial_table_t::write_batched_replace(
        ql::env_t *env,
        const std::vector<counted_t<const ql::datum_t> > &keys,
        const counted_t<ql::func_t> &func,
        UNUSED return_changes_t return_changes,
        UNUSED durability_requirement_t durability) {
    for (auto key : keys) {
        std::string error;
        counted_t<const ql::datum_t> old_row;
        if (!backend->read_row(key, env->interruptor, &old_row, &error)) {
            rfail_datum(ql::base_exc_t::GENERIC, "%s", error.c_str());
        }
        if (!old_row.has()) {
            old_row = ql::datum_t::null();
        } else {
#ifndef NDEBUG
            counted_t<const ql::datum_t> key2 =
                old_row->get(get_pkey(), ql::NOTHROW);
            rassert(key2.has());
            rassert(key2->print_primary() == key->print_primary());
#endif /* NDEBUG */
        }
        counted_t<const ql::datum_t> new_row =
            func->call(env, old_row, ql::LITERAL_OK)->as_datum();
        guarantee(new_row.has());
        if (new_row->get_type() == ql::datum_t::R_NULL) {
            new_row.reset();
        } else {
            /* TODO: Check that returned value is valid. */
        }
        if (!backend->write_row(key, new_row, env->interruptor, &error)) {
            rfail_datum(ql::base_exc_t::GENERIC, "%s", error.c_str());
        }
    }
    ql::datum_object_builder_t obj_builder;
    obj_builder.overwrite("is_this_a_stub", ql::datum_t::boolean(true));
    return std::move(obj_builder).to_counted();
}

counted_t<const ql::datum_t> artificial_table_t::write_batched_insert(
        UNUSED ql::env_t *env,
        UNUSED std::vector<counted_t<const ql::datum_t> > &&inserts,
        UNUSED conflict_behavior_t conflict_behavior,
        UNUSED return_changes_t return_changes,
        UNUSED durability_requirement_t durability) {
    rfail_datum(ql::base_exc_t::GENERIC,
        "Artificial tables currently only support point read and replace.");
}

bool artificial_table_t::write_sync_depending_on_durability(
        UNUSED ql::env_t *env,
        UNUSED durability_requirement_t durability) {
    rfail_datum(ql::base_exc_t::GENERIC,
        "Artificial tables currently only support point read and replace.");
}

bool artificial_table_t::sindex_create(
        UNUSED ql::env_t *env, UNUSED const std::string &id,
        UNUSED counted_t<ql::func_t> index_func, UNUSED sindex_multi_bool_t multi) {
    rfail_datum(ql::base_exc_t::GENERIC,
        "Can't create a secondary index on an artificial table.");
}

bool artificial_table_t::sindex_drop(UNUSED ql::env_t *env,
        UNUSED const std::string &id) {
    rfail_datum(ql::base_exc_t::GENERIC,
        "Can't drop a secondary index on an artificial table.");
}

sindex_rename_result_t artificial_table_t::sindex_rename(
        UNUSED ql::env_t *env,
        UNUSED const std::string &old_name,
        UNUSED const std::string &new_name,
        UNUSED bool overwrite) {
    rfail_datum(ql::base_exc_t::GENERIC,
        "Can't rename a secondary index on an artificial table.");
}

std::vector<std::string> artificial_table_t::sindex_list(UNUSED ql::env_t *env) {
    return std::vector<std::string>();
}

std::map<std::string, counted_t<const ql::datum_t> > artificial_table_t::sindex_status(
        UNUSED ql::env_t *env, UNUSED const std::set<std::string> &sindexes) {
    return std::map<std::string, counted_t<const ql::datum_t> >();
}

