// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_STREAM_HPP_
#define RDB_PROTOCOL_STREAM_HPP_

#include <algorithm>
#include <deque>
#include <list>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "errors.hpp"
#include <boost/enable_shared_from_this.hpp>
#include <boost/function.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/variant/get.hpp>

#include "clustering/administration/namespace_interface_repository.hpp"
#include "rdb_protocol/protocol.hpp"

enum batch_info_t { MID_BATCH, LAST_OF_BATCH, END_OF_STREAM };

namespace ql { class env_t; }

namespace query_language {

typedef rdb_protocol_details::rget_item_t rget_item_t;

enum sorting_hint_t { START, CONTINUE };

struct hinted_datum_t {
    hinted_datum_t() { }
    hinted_datum_t(sorting_hint_t _first, counted_t<const ql::datum_t> _second)
        : first(_first), second(_second) { }

    sorting_hint_t first;
    counted_t<const ql::datum_t> second;
};

class json_stream_t : public boost::enable_shared_from_this<json_stream_t> {
public:
    json_stream_t() { }
    // Returns a null value when end of stream is reached.
    virtual counted_t<const ql::datum_t> next(ql::env_t *env) = 0;
    virtual hinted_datum_t sorting_hint_next(ql::env_t *env) = 0;

    virtual MUST_USE boost::shared_ptr<json_stream_t>
    add_transformation(const rdb_protocol_details::transform_variant_t &) = 0;

    virtual rdb_protocol_t::rget_read_response_t::result_t
    apply_terminal(const rdb_protocol_details::terminal_variant_t &,
                   ql::env_t *env) = 0;

    virtual ~json_stream_t() { }

private:
    DISABLE_COPYING(json_stream_t);
};

class batched_rget_stream_t : public json_stream_t {
public:
    /* Primary key rget. */
    batched_rget_stream_t(const namespace_repo_t<rdb_protocol_t>::access_t &_ns_access,
        counted_t<const ql::datum_t> left_bound, bool left_bound_open,
        counted_t<const ql::datum_t> right_bound, bool right_bound_open,
        const std::map<std::string, ql::wire_func_t> &_optargs,
        bool _use_outdated, sorting_t sorting,
        ql::rcheckable_t *_parent);

    /* Sindex rget. */
    batched_rget_stream_t(const namespace_repo_t<rdb_protocol_t>::access_t &_ns_access,
        const std::string &_sindex_id,
        counted_t<const ql::datum_t> _sindex_start_value, bool start_value_open,
        counted_t<const ql::datum_t> _sindex_end_value, bool end_value_open,
        const std::map<std::string, ql::wire_func_t> &_optargs, bool _use_outdated,
        sorting_t sorting, ql::rcheckable_t *_parent);

    counted_t<const ql::datum_t> next(ql::env_t *env);

    hinted_datum_t sorting_hint_next(ql::env_t *env);

    boost::shared_ptr<json_stream_t> add_transformation(
        const rdb_protocol_details::transform_variant_t &t);

    rdb_protocol_t::rget_read_response_t::result_t
    apply_terminal(const rdb_protocol_details::terminal_variant_t &t,
                   ql::env_t *env);

private:
    boost::optional<rget_item_t> head(ql::env_t *env);
    void pop();
    rdb_protocol_t::rget_read_t get_rget();
    void read_more(ql::env_t *env);
    bool check_and_set_key_in_sorting_buffer(const std::string &key);

    /* Returns true if the passed value is new. */
    bool check_and_set_last_key(const std::string &key);
    bool check_and_set_last_key(counted_t<const ql::datum_t>);

    rdb_protocol_details::transform_t transform;
    namespace_repo_t<rdb_protocol_t>::access_t ns_access;
    boost::optional<std::string> sindex_id;

    /* This needs to include information about the secondary index key of the object
     * which is needed for sorting. */
    // ^^^ hopefully this comment still makes as much sense, it didn't make any sense
    // before...
    std::deque<rget_item_t> data;
    std::deque<rget_item_t> sorting_buffer;

    std::string key_in_sorting_buffer;

    boost::variant<counted_t<const ql::datum_t>, std::string> last_key;

    bool finished, started;
    const std::map<std::string, ql::wire_func_t> optargs;
    bool use_outdated;

    sindex_range_t sindex_range;
    key_range_t range;

    sorting_t sorting;

    ql::rcheckable_t *parent;
};

} // namespace query_language

#endif  // RDB_PROTOCOL_STREAM_HPP_
