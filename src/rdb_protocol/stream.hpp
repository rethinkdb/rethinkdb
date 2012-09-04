#ifndef RDB_PROTOCOL_STREAM_HPP_
#define RDB_PROTOCOL_STREAM_HPP_

#include <algorithm>
#include <list>
#include <set>
#include <string>
#include <vector>

#include "errors.hpp"
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

class json_stream_t {
public:
    json_stream_t() { }
    virtual boost::shared_ptr<scoped_cJSON_t> next() = 0; //MAY THROW
    virtual void add_transformation(const rdb_protocol_details::transform_atom_t &) = 0;
    virtual result_t apply_terminal(const rdb_protocol_details::terminal_t &) = 0;

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
        guarantee(!started);
        raw_data.sort(o);
    }


    boost::shared_ptr<scoped_cJSON_t> next();
    void add_transformation(const rdb_protocol_details::transform_atom_t &t);
    result_t apply_terminal(const rdb_protocol_details::terminal_t &t);
private:
    bool started; //We just use this to make sure people don't consume some of the list and then apply a transformation.
    rdb_protocol_details::transform_t transform;

    json_list_t raw_data;
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

    void add_transformation(const rdb_protocol_details::transform_atom_t &t);
    result_t apply_terminal(const rdb_protocol_details::terminal_t &t);

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

    void add_transformation(const rdb_protocol_details::transform_atom_t &);

    result_t apply_terminal(const rdb_protocol_details::terminal_t &);

private:
    stream_list_t streams;
    stream_list_t::iterator hd;
};

template <class P>
class filter_stream_t : public json_stream_t {
public:
    typedef boost::function<bool(boost::shared_ptr<scoped_cJSON_t>)> predicate;  // NOLINT
    filter_stream_t(boost::shared_ptr<json_stream_t> _stream, const P &_p)
        : stream(_stream), p(_p)
    { }

    boost::shared_ptr<scoped_cJSON_t> next() {
        while (boost::shared_ptr<scoped_cJSON_t> json = stream->next()) {
            if (p(json)) {
                return json;
            }
        }
        return boost::shared_ptr<scoped_cJSON_t>();
    }

private:
    boost::shared_ptr<json_stream_t> stream;
    P p;
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

    void add_transformation(const rdb_protocol_details::transform_atom_t &) {
        crash("not implemented");
    }

    result_t apply_terminal(const rdb_protocol_details::terminal_t &) {
        crash("not implemented");
    }

private:
    boost::shared_ptr<json_stream_t> stream;
    std::set<boost::shared_ptr<scoped_cJSON_t>, C> seen;
};

template <class F>
class mapping_stream_t : public json_stream_t {
public:
    mapping_stream_t(boost::shared_ptr<json_stream_t> _stream, const F &_f)
        : stream(_stream), f(_f)
    { }

    boost::shared_ptr<scoped_cJSON_t> next() {
        if (boost::shared_ptr<scoped_cJSON_t> json = stream->next()) {
            return f(json);
        } else {
            return json;
        }
    }

private:
    boost::shared_ptr<json_stream_t> stream;
    F f;
};

template <class F>
class concat_mapping_stream_t : public  json_stream_t {
public:
    concat_mapping_stream_t(boost::shared_ptr<json_stream_t> _stream, const F &_f)
        : stream(_stream), f(_f)
    {
        f = _f;
        if (boost::shared_ptr<scoped_cJSON_t> json = stream->next()) {
            substream = f(json);
        }
    }

    boost::shared_ptr<scoped_cJSON_t> next() {
        boost::shared_ptr<scoped_cJSON_t> res;

        while (!res) {
            if (!substream) {
                return res;
            } else if ((res = substream->next())) {
                continue;
            } else if (boost::shared_ptr<scoped_cJSON_t> json = stream->next()) {
                substream = f(json);
            } else {
                substream.reset();
            }
        }

        return res;
    }

private:
    boost::shared_ptr<json_stream_t> stream, substream;
    F f;
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

    void add_transformation(const rdb_protocol_details::transform_atom_t &t) {
        stream->add_transformation(t);
    }

    result_t apply_terminal(const rdb_protocol_details::terminal_t &) {
        crash("not implemented");
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

    void add_transformation(const rdb_protocol_details::transform_atom_t &t) {
        stream->add_transformation(t);
    }

    result_t apply_terminal(const rdb_protocol_details::terminal_t &) {
        crash("not implemented");
    }

private:
    boost::shared_ptr<json_stream_t> stream;
    int offset;
};

class range_stream_t : public json_stream_t {
public:
    range_stream_t(boost::shared_ptr<json_stream_t> _stream, const key_range_t &_range, const std::string &_attrname)
        : stream(_stream), range(_range), attrname(_attrname)
    { }

    boost::shared_ptr<scoped_cJSON_t> next() {
        // TODO: error handling
        // TODO reevaluate this when we better understand what we're doing for ordering
        while (boost::shared_ptr<scoped_cJSON_t> json = stream->next()) {
            if (json->type() != cJSON_Object) {
                continue;
            }
            cJSON* val = json->GetObjectItem(attrname.c_str());
            if (val && range.contains_key(store_key_t(cJSON_print_std_string(val)))) {
                return json;
            }
        }
        return boost::shared_ptr<scoped_cJSON_t>();
    }

    void add_transformation(const rdb_protocol_details::transform_atom_t &t) {
        stream->add_transformation(t);
    }

    result_t apply_terminal(const rdb_protocol_details::terminal_t &) {
        crash("not implemented");
    }

private:
    boost::shared_ptr<json_stream_t> stream;
    key_range_t range;
    std::string attrname;
};


} //namespace query_language

#endif  // RDB_PROTOCOL_STREAM_HPP_
