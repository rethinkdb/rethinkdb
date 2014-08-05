// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_ARTIFICIAL_TABLE_ARTIFICIAL_TABLE_HPP_
#define RDB_PROTOCOL_ARTIFICIAL_TABLE_ARTIFICIAL_TABLE_HPP_

#include "rdb_protocol/context.hpp"

class artificial_table_backend_t;

class artificial_table_t : public base_table_t {
public:
    artificial_table_t(artificial_table_backend_t *_backend);

    const std::string &get_pkey();

    counted_t<const ql::datum_t> read_row(ql::env_t *env,
        counted_t<const ql::datum_t> pval, bool use_outdated);
    counted_t<ql::datum_stream_t> read_all(
        ql::env_t *env,
        const std::string &get_all_sindex_id,
        const ql::protob_t<const Backtrace> &bt,
        const std::string &table_name,   /* the table's own name, for display purposes */
        const ql::datum_range_t &range,
        sorting_t sorting,
        bool use_outdated);
    counted_t<ql::datum_stream_t> read_row_changes(
        ql::env_t *env,
        counted_t<const ql::datum_t> pval,
        const ql::protob_t<const Backtrace> &bt,
        const std::string &table_name);
    counted_t<ql::datum_stream_t> read_all_changes(
        ql::env_t *env,
        const ql::protob_t<const Backtrace> &bt,
        const std::string &table_name);

    counted_t<const ql::datum_t> write_batched_replace(ql::env_t *env,
        const std::vector<counted_t<const ql::datum_t> > &keys,
        const counted_t<ql::func_t> &func,
        bool _return_vals, durability_requirement_t durability);
    counted_t<const ql::datum_t> write_batched_insert(ql::env_t *env,
        std::vector<counted_t<const ql::datum_t> > &&inserts,
        conflict_behavior_t conflict_behavior, bool return_vals,
        durability_requirement_t durability);
    bool write_sync_depending_on_durability(ql::env_t *env,
        durability_requirement_t durability);

    bool sindex_create(ql::env_t *env, const std::string &id,
        counted_t<ql::func_t> index_func, bool multi);
    bool sindex_drop(ql::env_t *env, const std::string &id);
    std::vector<std::string> sindex_list(ql::env_t *env);
    std::map<std::string, counted_t<const ql::datum_t> > sindex_status(ql::env_t *env,
        const std::set<std::string> &sindexes);

private:
    artificial_table_backend_t *backend;
    std::string primary_key;
};

#endif /* RDB_PROTOCOL_ARTIFICIAL_TABLE_ARTIFICIAL_TABLE_HPP_ */

