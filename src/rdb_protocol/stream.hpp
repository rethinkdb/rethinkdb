// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_STREAM_HPP_
#define RDB_PROTOCOL_STREAM_HPP_

#include <algorithm>
#include <list>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "errors.hpp"
#include <boost/enable_shared_from_this.hpp>
#include <boost/function.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/variant/get.hpp>

#include "clustering/administration/namespace_interface_repository.hpp"
#include "rdb_protocol/exceptions.hpp"
#include "rdb_protocol/protocol.hpp"
#include "rdb_protocol/proto_utils.hpp"

enum batch_info_t { MID_BATCH, LAST_OF_BATCH, END_OF_STREAM };

namespace ql { class env_t; }
namespace query_language {

typedef std::list<boost::shared_ptr<scoped_cJSON_t> > json_list_t;
typedef rdb_protocol_t::rget_read_response_t::result_t result_t;

class json_stream_t : public boost::enable_shared_from_this<json_stream_t> {
public:
    json_stream_t() { }
    // Returns a null value when end of stream is reached.
    virtual boost::shared_ptr<scoped_cJSON_t> next() = 0;  // MAY THROW

    virtual MUST_USE boost::shared_ptr<json_stream_t> add_transformation(const rdb_protocol_details::transform_variant_t &, ql::env_t *ql_env, const backtrace_t &backtrace);
    virtual result_t apply_terminal(const rdb_protocol_details::terminal_variant_t &,
                                    ql::env_t *ql_env,
                                    const backtrace_t &backtrace);

    virtual ~json_stream_t() { }

    virtual void reset_interruptor(UNUSED signal_t *new_interruptor) { }

private:
    DISABLE_COPYING(json_stream_t);
};

class in_memory_stream_t : public json_stream_t {
public:
    explicit in_memory_stream_t(json_array_iterator_t it);
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

    boost::shared_ptr<scoped_cJSON_t> next();

    /* Use default implementation of `add_transformation()` and `apply_terminal()` */

private:
    json_list_t data;
};

class transform_stream_t : public json_stream_t {
public:
    transform_stream_t(boost::shared_ptr<json_stream_t> stream, ql::env_t *_ql_env, const rdb_protocol_details::transform_t &tr);

    boost::shared_ptr<scoped_cJSON_t> next();
    boost::shared_ptr<json_stream_t> add_transformation(const rdb_protocol_details::transform_variant_t &, ql::env_t *ql_env, const backtrace_t &backtrace);

private:
    boost::shared_ptr<json_stream_t> stream;
    ql::env_t *ql_env;
    rdb_protocol_details::transform_t transform;
    json_list_t data;
};

class batched_rget_stream_t : public json_stream_t {
public:
    /* Primary key rget. */
    batched_rget_stream_t(const namespace_repo_t<rdb_protocol_t>::access_t &_ns_access,
                          signal_t *_interruptor,
                          counted_t<const ql::datum_t> left_bound,
                          counted_t<const ql::datum_t> right_bound,
                          const std::map<std::string, ql::wire_func_t> &_optargs,
                          bool _use_outdated);

    /* Sindex rget. */
    batched_rget_stream_t(const namespace_repo_t<rdb_protocol_t>::access_t &_ns_access,
                          signal_t *_interruptor, const std::string &_sindex_id,
                          const std::map<std::string, ql::wire_func_t> &_optargs,
                          bool _use_outdated,
                          counted_t<const ql::datum_t> _sindex_start_value,
                          counted_t<const ql::datum_t> _sindex_end_value);

    boost::shared_ptr<scoped_cJSON_t> next();

    boost::shared_ptr<json_stream_t> add_transformation(const rdb_protocol_details::transform_variant_t &t, ql::env_t *ql_env, const backtrace_t &backtrace);
    result_t apply_terminal(const rdb_protocol_details::terminal_variant_t &t,
                            ql::env_t *ql_env,
                            const backtrace_t &backtrace);

    virtual void reset_interruptor(signal_t *new_interruptor) {
        interruptor = new_interruptor;
    };

private:
    rdb_protocol_t::rget_read_t get_rget();
    void read_more();

    rdb_protocol_details::transform_t transform;
    namespace_repo_t<rdb_protocol_t>::access_t ns_access;
    signal_t *interruptor;
    boost::optional<std::string> sindex_id;

    json_list_t data;
    bool finished, started;
    const std::map<std::string, ql::wire_func_t> optargs;
    bool use_outdated;

    counted_t<const ql::datum_t> sindex_start_value;
    counted_t<const ql::datum_t> sindex_end_value;

    key_range_t range;

    boost::optional<backtrace_t> table_scan_backtrace;
};


} //namespace query_language

#endif  // RDB_PROTOCOL_STREAM_HPP_
