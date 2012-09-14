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
#include "rdb_protocol/stream_cache.hpp"


namespace query_language {

class runtime_environment_t;

typedef std::list<boost::shared_ptr<scoped_cJSON_t> > json_list_t;
typedef rdb_protocol_t::rget_read_response_t::result_t result_t;

class json_stream_t : public boost::enable_shared_from_this<json_stream_t> {
public:
    json_stream_t() { }
    virtual boost::shared_ptr<scoped_cJSON_t> next() = 0; //MAY THROW
    virtual MUST_USE boost::shared_ptr<json_stream_t> add_transformation(const rdb_protocol_details::transform_variant_t &, runtime_environment_t *env, const scopes_t &scopes, const backtrace_t &backtrace);
    virtual result_t apply_terminal(const rdb_protocol_details::terminal_variant_t &, runtime_environment_t *env, const scopes_t &scopes, const backtrace_t &backtrace);

    virtual ~json_stream_t() { }

private:
    DISABLE_COPYING(json_stream_t);
};

class in_memory_stream_t : public json_stream_t {
public:
    in_memory_stream_t(json_array_iterator_t it);
    in_memory_stream_t(boost::shared_ptr<json_stream_t> stream);

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
    transform_stream_t(boost::shared_ptr<json_stream_t> stream, runtime_environment_t *env, const rdb_protocol_details::transform_t &tr);

    boost::shared_ptr<scoped_cJSON_t> next();
    boost::shared_ptr<json_stream_t> add_transformation(const rdb_protocol_details::transform_variant_t &, runtime_environment_t *env, const scopes_t &scopes, const backtrace_t &backtrace);

private:
    boost::shared_ptr<json_stream_t> stream;
    runtime_environment_t *env;
    rdb_protocol_details::transform_t transform;
    json_list_t data;
};

class batched_rget_stream_t : public json_stream_t {
public:
    batched_rget_stream_t(const namespace_repo_t<rdb_protocol_t>::access_t &_ns_access, 
                          signal_t *_interruptor, key_range_t _range, 
                          int _batch_size, const backtrace_t &_table_scan_backtrace);

    boost::shared_ptr<scoped_cJSON_t> next();

    boost::shared_ptr<json_stream_t> add_transformation(const rdb_protocol_details::transform_variant_t &t, runtime_environment_t *env, const scopes_t &scopes, const backtrace_t &backtrace);
    result_t apply_terminal(const rdb_protocol_details::terminal_variant_t &t, runtime_environment_t *env, const scopes_t &scopes, const backtrace_t &backtrace);

private:
    void read_more();

    rdb_protocol_details::transform_t transform;
    namespace_repo_t<rdb_protocol_t>::access_t ns_access;
    signal_t *interruptor;
    key_range_t range;
    int batch_size;

    json_list_t data;
    int index;
    bool finished, started;

    backtrace_t table_scan_backtrace;
};

class union_stream_t : public json_stream_t {
public:
    typedef std::list<boost::shared_ptr<json_stream_t> > stream_list_t;

    explicit union_stream_t(const stream_list_t &_streams);

    boost::shared_ptr<scoped_cJSON_t> next();

    boost::shared_ptr<json_stream_t> add_transformation(const rdb_protocol_details::transform_variant_t &, runtime_environment_t *env, const scopes_t &scopes, const backtrace_t &backtrace);

    /* TODO: Maybe we can optimize `apply_terminal()`. */

private:
    stream_list_t streams;
    stream_list_t::iterator hd;
};

template <class C>
class distinct_stream_t : public json_stream_t {
public:
    typedef boost::function<bool(boost::shared_ptr<scoped_cJSON_t>)> predicate;  // NOLINT
    distinct_stream_t(boost::shared_ptr<json_stream_t> _stream, const C &_c)
        : stream(_stream), seen(_c)
    { }

    boost::shared_ptr<scoped_cJSON_t> next() {
        while (boost::shared_ptr<scoped_cJSON_t> json = stream->next()) {
            if (seen.insert(json).second) { // was this not already present?
                return json;
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

    boost::shared_ptr<scoped_cJSON_t> next() {
        while (start) {
            start--;
            stream->next();
        }
        if (unbounded || stop != 0) {
            stop--;
            return stream->next();
        }
        return boost::shared_ptr<scoped_cJSON_t>();
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

    boost::shared_ptr<scoped_cJSON_t> next() {
        return stream->next();
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

    boost::shared_ptr<scoped_cJSON_t> next() {
        // TODO: more error handling
        // TODO reevaluate this when we better understand what we're doing for ordering
        while (boost::shared_ptr<scoped_cJSON_t> json = stream->next()) {
            rassert(json);
            rassert(json->get());
            if (json->type() != cJSON_Object) {
                throw runtime_exc_t(strprintf("Got non-object in RANGE query: %s.", json->Print().c_str()), backtrace);
            }
            scoped_cJSON_t val(cJSON_DeepCopy(json->GetObjectItem(attrname.c_str())));
            if (!val.get()) {
                throw runtime_exc_t(strprintf("Object %s has not attribute %s.", json->Print().c_str(), attrname.c_str()),backtrace);
            } else if (val.type() != cJSON_Number && val.type() != cJSON_String) {
                throw runtime_exc_t(strprintf("Primary key must be a number or string, not %s.", val.Print().c_str()), backtrace);
            } else if (range.contains_key(store_key_t(val.PrintLexicographic()))) {
                return json;
            }
        }
        return boost::shared_ptr<scoped_cJSON_t>();
    }

private:
    boost::shared_ptr<json_stream_t> stream;
    key_range_t range;
    std::string attrname;
    backtrace_t backtrace;
};

} //namespace query_language

#endif  // RDB_PROTOCOL_STREAM_HPP_
