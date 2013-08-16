// Copyright 2010-2012 RethinkDB, all rights reserved.
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
#include "rdb_protocol/exceptions.hpp"
#include "rdb_protocol/proto_utils.hpp"
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
    virtual counted_t<const ql::datum_t> next() = 0;  // MAY THROW

    virtual hinted_datum_t sorting_hint_next();

    virtual MUST_USE boost::shared_ptr<json_stream_t> add_transformation(const rdb_protocol_details::transform_variant_t &, ql::env_t *ql_env, const backtrace_t &backtrace);
    virtual rdb_protocol_t::rget_read_response_t::result_t
    apply_terminal(const rdb_protocol_details::terminal_variant_t &,
                   ql::env_t *ql_env,
                   const backtrace_t &backtrace);

    virtual ~json_stream_t() { }

    virtual void reset_interruptor(UNUSED signal_t *new_interruptor) { }

private:
    DISABLE_COPYING(json_stream_t);
};

class in_memory_stream_t : public json_stream_t {
public:
    explicit in_memory_stream_t(boost::shared_ptr<json_stream_t> stream);

    template <class Ordering>
    void sort(const Ordering &o) {
        if (data.size() == 1) {
            // We want to do this so that we trigger exceptions consistently.
            o(*data.begin(), *data.begin());
        } else {
            data.sort(o);
        }
    }

    counted_t<const ql::datum_t> next();

    /* Use default implementation of `add_transformation()` and `apply_terminal()` */

private:
    std::list<counted_t<const ql::datum_t> > data;
};

class transform_stream_t : public json_stream_t {
public:
    transform_stream_t(boost::shared_ptr<json_stream_t> stream, ql::env_t *_ql_env, const rdb_protocol_details::transform_t &tr);

    counted_t<const ql::datum_t> next();
    boost::shared_ptr<json_stream_t> add_transformation(const rdb_protocol_details::transform_variant_t &, ql::env_t *ql_env, const backtrace_t &backtrace);

private:
    boost::shared_ptr<json_stream_t> stream;
    ql::env_t *ql_env;
    rdb_protocol_details::transform_t transform;
    std::list<counted_t<const ql::datum_t> > data;
};

class batched_rget_stream_t : public json_stream_t {
public:
    /* Primary key rget. */
    batched_rget_stream_t(const namespace_repo_t<rdb_protocol_t>::access_t &_ns_access,
        signal_t *_interruptor,
        counted_t<const ql::datum_t> left_bound, bool left_bound_open,
        counted_t<const ql::datum_t> right_bound, bool right_bound_open,
        const std::map<std::string, ql::wire_func_t> &_optargs,
        bool _use_outdated, sorting_t sorting,
        ql::rcheckable_t *_parent);

    /* Sindex rget. */
    batched_rget_stream_t(const namespace_repo_t<rdb_protocol_t>::access_t &_ns_access,
        signal_t *_interruptor, const std::string &_sindex_id,
        counted_t<const ql::datum_t> _sindex_start_value, bool start_value_open,
        counted_t<const ql::datum_t> _sindex_end_value, bool end_value_open,
        const std::map<std::string, ql::wire_func_t> &_optargs, bool _use_outdated,
        sorting_t sorting, ql::rcheckable_t *_parent);

    counted_t<const ql::datum_t> next();

    hinted_datum_t sorting_hint_next();

    boost::shared_ptr<json_stream_t> add_transformation(const rdb_protocol_details::transform_variant_t &t, ql::env_t *ql_env, const backtrace_t &backtrace);
    rdb_protocol_t::rget_read_response_t::result_t
    apply_terminal(const rdb_protocol_details::terminal_variant_t &t,
                   ql::env_t *ql_env,
                   const backtrace_t &backtrace);

    virtual void reset_interruptor(signal_t *new_interruptor) {
        interruptor = new_interruptor;
    };

private:
    boost::optional<rget_item_t> head();
    void pop();
    rdb_protocol_t::rget_read_t get_rget();
    void read_more();
    bool check_and_set_key_in_sorting_buffer(const std::string &key);

    /* Returns true if the passed value is new. */
    bool check_and_set_last_key(const std::string &key);
    bool check_and_set_last_key(counted_t<const ql::datum_t>);

    rdb_protocol_details::transform_t transform;
    namespace_repo_t<rdb_protocol_t>::access_t ns_access;
    signal_t *interruptor;
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

    rdb_protocol_t::sindex_range_t sindex_range;
    key_range_t range;

    boost::optional<backtrace_t> table_scan_backtrace;

    sorting_t sorting;

    ql::rcheckable_t *parent;
};

} // namespace query_language

#endif  // RDB_PROTOCOL_STREAM_HPP_
