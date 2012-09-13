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
    virtual MUST_USE boost::shared_ptr<json_stream_t> add_transformation(const rdb_protocol_details::transform_atom_t &);
    virtual result_t apply_terminal(const rdb_protocol_details::terminal_t &, runtime_environment_t *env);

    virtual ~json_stream_t() { }

private:
    DISABLE_COPYING(json_stream_t);
};

class in_memory_stream_t : public json_stream_t {
public:
    in_memory_stream_t(json_array_iterator_t it, runtime_environment_t *env);
    in_memory_stream_t(boost::shared_ptr<json_stream_t> stream, runtime_environment_t *env);

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
    boost::scoped_ptr<runtime_environment_t> env;
};

class transform_stream_t : public json_stream_t {
public:
    transform_stream_t(boost::shared_ptr<json_stream_t> stream, const rdb_protocol_details::transform_t &tr, runtime_environment_t *env);

    boost::shared_ptr<scoped_cJSON_t> next();
    boost::shared_ptr<json_stream_t> add_transformation(const rdb_protocol_details::transform_atom_t &);

private:
    boost::shared_ptr<json_stream_t> stream;
    rdb_protocol_details::transform_t transform;
    json_list_t data;
    boost::scoped_ptr<runtime_environment_t> env;
};

class batched_rget_stream_t : public json_stream_t {
public:
    batched_rget_stream_t(const namespace_repo_t<rdb_protocol_t>::access_t &_ns_access, 
                          signal_t *_interruptor, key_range_t _range, 
                          int _batch_size, backtrace_t _backtrace,
                          const scopes_t &_scopes);

    boost::shared_ptr<scoped_cJSON_t> next();

    boost::shared_ptr<json_stream_t> add_transformation(const rdb_protocol_details::transform_atom_t &t);
    result_t apply_terminal(const rdb_protocol_details::terminal_t &t, runtime_environment_t *env);

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

    backtrace_t backtrace;

    scopes_t scopes;
};

class union_stream_t : public json_stream_t {
public:
    typedef std::list<boost::shared_ptr<json_stream_t> > stream_list_t;

    explicit union_stream_t(const stream_list_t &_streams);

    boost::shared_ptr<scoped_cJSON_t> next();

    boost::shared_ptr<json_stream_t> add_transformation(const rdb_protocol_details::transform_atom_t &);

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

} //namespace query_language

#endif  // RDB_PROTOCOL_STREAM_HPP_
