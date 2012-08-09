#ifndef RDB_PROTOCOL_STREAM_HPP_
#define RDB_PROTOCOL_STREAM_HPP_

#include <algorithm>
#include <list>
#include <string>
#include <vector>

#include "errors.hpp"
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/variant/get.hpp>

#include "clustering/administration/namespace_interface_repository.hpp"
#include "rdb_protocol/exceptions.hpp"
#include "rdb_protocol/protocol.hpp"
#include "rdb_protocol/stream_cache.hpp"

namespace query_language {

typedef std::list<boost::shared_ptr<scoped_cJSON_t> > cJSON_list_t;

class in_memory_stream_t : public json_stream_t {
public:
    template <class iterator>
    in_memory_stream_t(const iterator &begin, const iterator &end)
        : data(begin, end)
    { }

    explicit in_memory_stream_t(json_array_iterator_t it) {
        while (cJSON *json = it.next()) {
            data.push_back(boost::shared_ptr<scoped_cJSON_t>(new scoped_cJSON_t(cJSON_DeepCopy(json))));
        }
    }

    explicit in_memory_stream_t(boost::shared_ptr<json_stream_t> stream) {
        while (boost::shared_ptr<scoped_cJSON_t> json = stream->next()) {
            data.push_back(json);
        }
    }

    template <class Ordering>
    void sort(const Ordering &o) {
        data.sort(o);
    }

    boost::shared_ptr<scoped_cJSON_t> next() {
        if (data.empty()) {
            return boost::shared_ptr<scoped_cJSON_t>();
        } else {
            boost::shared_ptr<scoped_cJSON_t> res = data.front();
            data.pop_front();
            return res;
        }
    }
private:
    cJSON_list_t data;
};

class batched_rget_stream_t : public json_stream_t {
public:
    batched_rget_stream_t(const namespace_repo_t<rdb_protocol_t>::access_t &_ns_access, signal_t *_interruptor, key_range_t _range, int _batch_size,
        backtrace_t _backtrace)
        : ns_access(_ns_access), interruptor(_interruptor), range(_range), batch_size(_batch_size), index(0), finished(false), backtrace(_backtrace) { }

    boost::shared_ptr<scoped_cJSON_t> next() {
        if (data.empty()) {
            if (finished) {
                return boost::shared_ptr<scoped_cJSON_t>();
            }
            read_more();
            if (data.empty()) {
                finished = true;
                return boost::shared_ptr<scoped_cJSON_t>();
            }
        }
        boost::shared_ptr<scoped_cJSON_t> ret = data.front();
        data.pop_front();
        return ret;
    }

private:
    void read_more() {
        rdb_protocol_t::rget_read_t rget_read(range, batch_size);
        rdb_protocol_t::read_t read(rget_read);
        try {
            rdb_protocol_t::read_response_t res = ns_access.get_namespace_if()->read(read, order_token_t::ignore, interruptor);
            rdb_protocol_t::rget_read_response_t *p_res = boost::get<rdb_protocol_t::rget_read_response_t>(&res.response);
            rassert(p_res);

            // todo: just do a straight copy?
            typedef rdb_protocol_t::rget_read_response_t::stream_t stream_t;
            stream_t *stream = boost::get<stream_t>(&p_res->result);
            guarantee(stream);

            for (stream_t::iterator i = stream->begin(); i != stream->end(); ++i) {
                data.push_back(i->second);
                rassert(data.back());
                range.left = i->first;
            }

            if (!range.left.increment()) {
                finished = true;
            }
        } catch (cannot_perform_query_exc_t e) {
            throw runtime_exc_t("cannot perform read: " + std::string(e.what()), backtrace);
        }
    }

    namespace_repo_t<rdb_protocol_t>::access_t ns_access;
    signal_t *interruptor;
    key_range_t range;
    int batch_size;

    cJSON_list_t data;
    int index;
    bool finished;

    backtrace_t backtrace;
};

class union_stream_t : public json_stream_t {
public:
    typedef std::list<boost::shared_ptr<json_stream_t> > stream_list_t;

    explicit union_stream_t(const stream_list_t &_streams)
        : streams(_streams), hd(streams.begin())
    { }

    boost::shared_ptr<scoped_cJSON_t> next() {
        while (hd != streams.end()) {
            if (boost::shared_ptr<scoped_cJSON_t> json = (*hd)->next()) {
                return json;
            } else {
                ++hd;
            }
        }
        return boost::shared_ptr<scoped_cJSON_t>();
    }

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
    range_stream_t(boost::shared_ptr<json_stream_t> _stream, const key_range_t &_range, const std::string &_attrname)
        : stream(_stream), range(_range), attrname(_attrname)
    { }

    boost::shared_ptr<scoped_cJSON_t> next() {
        // TODO: error handling
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

private:
    boost::shared_ptr<json_stream_t> stream;
    key_range_t range;
    std::string attrname;
};


} //namespace query_language

#endif  // RDB_PROTOCOL_STREAM_HPP_
