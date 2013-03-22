// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_STREAM_HPP_
#define RDB_PROTOCOL_STREAM_HPP_

#include <algorithm>
#include <list>
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
    boost::shared_ptr<scoped_cJSON_t> next(); //MAY THROW    // RSI is anybody using this?  Should they be?

    virtual batch_info_t next_impl(boost::shared_ptr<scoped_cJSON_t> *json_out) = 0;  // MAY THROW

    virtual MUST_USE boost::shared_ptr<json_stream_t> add_transformation(const rdb_protocol_details::transform_variant_t &, ql::env_t *ql_env, const scopes_t &scopes, const backtrace_t &backtrace);
    virtual result_t apply_terminal(const rdb_protocol_details::terminal_variant_t &,
                                    ql::env_t *ql_env,
                                    const scopes_t &scopes,
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

    batch_info_t next_impl(boost::shared_ptr<scoped_cJSON_t> *out);

    /* Use default implementation of `add_transformation()` and `apply_terminal()` */

private:
    json_list_t data;
};

class transform_stream_t : public json_stream_t {
public:
    transform_stream_t(boost::shared_ptr<json_stream_t> stream, ql::env_t *_ql_env, const rdb_protocol_details::transform_t &tr);

    batch_info_t next_impl(boost::shared_ptr<scoped_cJSON_t> *out);
    boost::shared_ptr<json_stream_t> add_transformation(const rdb_protocol_details::transform_variant_t &, ql::env_t *ql_env, const scopes_t &scopes, const backtrace_t &backtrace);

private:
    boost::shared_ptr<json_stream_t> stream;
    ql::env_t *ql_env;
    rdb_protocol_details::transform_t transform;
    json_list_t data;
    batch_info_t data_end_batch_info;  // Is set when data is not empty, is specifically used
                                       // when last element of data is removed.
};

class batched_rget_stream_t : public json_stream_t {
public:
    // batched_rget_stream_t(const namespace_repo_t<rdb_protocol_t>::access_t &_ns_access,
    //                       signal_t *_interruptor, key_range_t _range,
    //                       const backtrace_t &_table_scan_backtrace,
    //                       bool _use_outdated);
    batched_rget_stream_t(const namespace_repo_t<rdb_protocol_t>::access_t &_ns_access,
                          signal_t *_interruptor, key_range_t _range,
                          const std::map<std::string, ql::wire_func_t> &_optargs,
                          bool _use_outdated);

    batch_info_t next_impl(boost::shared_ptr<scoped_cJSON_t> *out);

    boost::shared_ptr<json_stream_t> add_transformation(const rdb_protocol_details::transform_variant_t &t, ql::env_t *ql_env, const scopes_t &scopes, const backtrace_t &backtrace);
    result_t apply_terminal(const rdb_protocol_details::terminal_variant_t &t,
                            ql::env_t *ql_env,
                            const scopes_t &scopes,
                            const backtrace_t &backtrace);

    virtual void reset_interruptor(signal_t *new_interruptor) {
        interruptor = new_interruptor;
    };

private:
    void read_more();

    rdb_protocol_details::transform_t transform;
    namespace_repo_t<rdb_protocol_t>::access_t ns_access;
    signal_t *interruptor;
    key_range_t range;
    // TODO(jdoliner) What were batch_size and index doing being unused?  Does this type actually do
    // batching?  (This code might be removed in the ql-refactor branch, so it might not matter.)
    // int batch_size;

    json_list_t data;
    // See the TODO(jdoliner) above.
    // int index;
    bool finished, started;
    const std::map<std::string, ql::wire_func_t> optargs;
    bool use_outdated;

    boost::optional<backtrace_t> table_scan_backtrace;
};

class union_stream_t : public json_stream_t {
public:
    typedef std::list<boost::shared_ptr<json_stream_t> > stream_list_t;

    explicit union_stream_t(const stream_list_t &_streams);

    batch_info_t next_impl(boost::shared_ptr<scoped_cJSON_t> *out);

    boost::shared_ptr<json_stream_t> add_transformation(const rdb_protocol_details::transform_variant_t &, ql::env_t *ql_env, const scopes_t &scopes, const backtrace_t &backtrace);

    /* TODO: Maybe we can optimize `apply_terminal()`. */

private:
    stream_list_t streams;
    stream_list_t::iterator hd;
};

template <class C>
class distinct_stream_t : public json_stream_t {
public:
    typedef boost::function<bool(boost::shared_ptr<scoped_cJSON_t>)> predicate; // NOLINT
    distinct_stream_t(boost::shared_ptr<json_stream_t> _stream, const C &_c)
        : stream(_stream), seen(_c)
    { }

    batch_info_t next_impl(boost::shared_ptr<scoped_cJSON_t> *out) {
        for (;;) {
            boost::shared_ptr<scoped_cJSON_t> json;
            const batch_info_t res = stream->next_impl(&json);

            if (!json) {
                *out = boost::shared_ptr<scoped_cJSON_t>();
                return res;
            }

            if (seen.insert(json).second) { // was this not already present?
                *out = json;
                return res;
            } else if (res == LAST_OF_BATCH) {
                *out = boost::shared_ptr<scoped_cJSON_t>();
                return LAST_OF_BATCH;
            }
        }
        return boost::shared_ptr<scoped_cJSON_t>();
    }

private:
    boost::shared_ptr<json_stream_t> stream;
    std::set<boost::shared_ptr<scoped_cJSON_t>, C> seen;
};

class slice_stream_t : public json_stream_t {
public:
    slice_stream_t(boost::shared_ptr<json_stream_t> _stream, int _start, bool _unbounded, int _stop)
        : stream(_stream), start(_start), unbounded(_unbounded), stop(_stop)
    {
        guarantee(start >= 0);
        guarantee(stop >= 0);
        guarantee(unbounded || stop >= start);
        stop -= start;
    }

    batch_info_t next_impl(boost::shared_ptr<scoped_cJSON_t> *out) {
        while (start != 0) {
            boost::shared_ptr<scoped_cJSON_t> discard;
            const batch_info_t res = stream->next_impl(&discard);

            if (res == END_OF_STREAM) {
                *out = boost::shared_ptr<scoped_cJSON_t>();
                return END_OF_STREAM;
            }

            if (discard) {
                --start;
            }
        }

        if (unbounded || stop != 0) {
            boost::shared_ptr<scoped_cJSON_t> json;
            const batch_info_t res = stream->next_impl(&json);

            if (json) {
                --stop;
            }

            *out = json;
            return res;
        }

        *out = boost::shared_ptr<scoped_cJSON_t>();
        return END_OF_STREAM;
    }

private:
    boost::shared_ptr<json_stream_t> stream;
    int start;
    bool unbounded;
    int stop;
};

class skip_stream_t : public json_stream_t {
public:
    skip_stream_t(boost::shared_ptr<json_stream_t> _stream, int _offset)
        : stream(_stream), offset(_offset)
    {
        guarantee(offset >= 0);
    }

    // RSI: Shouldn't this be skipping things?
    batch_info_t next_impl(boost::shared_ptr<scoped_cJSON_t> *out) {
        return stream->next_impl(out);
    }

private:
    boost::shared_ptr<json_stream_t> stream;
    int offset;
};

class range_stream_t : public json_stream_t {
public:
    range_stream_t(boost::shared_ptr<json_stream_t> _stream, const key_range_t &_range,
                   const std::string &_attrname, const backtrace_t &_backtrace)
        : stream(_stream), range(_range), attrname(_attrname), backtrace(_backtrace)
    { }

    batch_info_t next_impl(boost::shared_ptr<scoped_cJSON_t> *out) {
        // TODO: ***use an index***
        // TODO: more error handling
        // TODO reevaluate this when we better understand what we're doing for ordering
        for (;;) {
            boost::shared_ptr<scoped_cJSON_t> json;
            const batch_info_t res = stream->next_impl(&json);

            if (!json) {
                *out = boost::shared_ptr<scoped_cJSON_t>();
                return res;
            }

            if (json->type() != cJSON_Object) {
                throw runtime_exc_t(strprintf("Got non-object in RANGE query: %s.", json->Print().c_str()), backtrace);
            }
            scoped_cJSON_t val(cJSON_DeepCopy(json->GetObjectItem(attrname.c_str())));
            if (!val.get()) {
                throw runtime_exc_t(strprintf("Object %s has no attribute %s.", json->Print().c_str(), attrname.c_str()), backtrace);
            } else if (val.type() != cJSON_Number && val.type() != cJSON_String) {
                throw runtime_exc_t(strprintf("Primary key must be a number or string, not %s.", val.Print().c_str()), backtrace);
            } else if (range.contains_key(store_key_t(cJSON_print_primary(val.get(), backtrace)))) {
                *out = json;
                return res;
            } else if (res == LAST_OF_BATCH) {
                // Can't skip LAST_OF_BATCH markers.
                *out = boost::shared_ptr<scoped_cJSON_t>();
                return LAST_OF_BATCH;
            }
        }
    }

private:
    boost::shared_ptr<json_stream_t> stream;
    key_range_t range;
    std::string attrname;
    backtrace_t backtrace;
};

} //namespace query_language

#endif  // RDB_PROTOCOL_STREAM_HPP_
