#include "rdb_protocol/stream.hpp"
#include "rdb_protocol/environment.hpp"
#include "rdb_protocol/transform_visitors.hpp"

namespace query_language {

in_memory_stream_t::in_memory_stream_t(json_array_iterator_t it, runtime_environment_t *_env)
    : started(false), env(new runtime_environment_t(*_env))
{
    while (cJSON *json = it.next()) {
        raw_data.push_back(boost::shared_ptr<scoped_cJSON_t>(new scoped_cJSON_t(cJSON_DeepCopy(json))));
    }
}

in_memory_stream_t::in_memory_stream_t(boost::shared_ptr<json_stream_t> stream, runtime_environment_t *_env)
    : started(false), env(new runtime_environment_t(*_env))
{
    while (boost::shared_ptr<scoped_cJSON_t> json = stream->next()) {
        raw_data.push_back(json);
    }
}

boost::shared_ptr<scoped_cJSON_t> in_memory_stream_t::next() {
    started = true;
    while (data.empty()) {
        if (raw_data.empty()) {
            return boost::shared_ptr<scoped_cJSON_t>();
        } else {
            json_list_t accumulator;
            accumulator.push_back(raw_data.front());
            raw_data.pop_front();

            //Apply transforms to the data
            typedef rdb_protocol_details::transform_t::iterator tit_t;
            for (tit_t it  = transform.begin();
                       it != transform.end();
                       ++it) {
                 json_list_t tmp;

                for (json_list_t::iterator jt  = accumulator.begin();
                                           jt != accumulator.end();
                                           ++jt) {
                    boost::apply_visitor(transform_visitor_t(*jt, &tmp, env.get()), *it);
                }
                accumulator.clear();
                accumulator.splice(accumulator.begin(), tmp);
            }
            data.splice(data.begin(), accumulator);
        }
    }

    boost::shared_ptr<scoped_cJSON_t> res = data.front();
    data.pop_front();
    return res;
}

void in_memory_stream_t::add_transformation(const rdb_protocol_details::transform_atom_t &t) {
    transform.push_back(t);
}

result_t in_memory_stream_t::apply_terminal(const rdb_protocol_details::terminal_t &t) {
    result_t res;
    boost::apply_visitor(terminal_initializer_visitor_t(&res, env.get()), t);
    boost::shared_ptr<scoped_cJSON_t> json;
    while ((json = next())) boost::apply_visitor(terminal_visitor_t(json, env.get(), &res), t);
    return res;
}

batched_rget_stream_t::batched_rget_stream_t(const namespace_repo_t<rdb_protocol_t>::access_t &_ns_access,
                      signal_t *_interruptor, key_range_t _range,
                      int _batch_size, backtrace_t _backtrace,
                      const scopes_t &_scopes)
    : ns_access(_ns_access), interruptor(_interruptor),
      range(_range), batch_size(_batch_size), index(0),
      finished(false), started(false), backtrace(_backtrace),
      scopes(_scopes)
{ }

boost::shared_ptr<scoped_cJSON_t> batched_rget_stream_t::next() {
    started = true;
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

void batched_rget_stream_t::add_transformation(const rdb_protocol_details::transform_atom_t &t) {
    guarantee(!started);
    transform.push_back(t);
}

result_t batched_rget_stream_t::apply_terminal(const rdb_protocol_details::terminal_t &t) {
    rdb_protocol_t::rget_read_t rget_read(range, -1);
    rget_read.transform = transform;
    rget_read.terminal = t;
    rdb_protocol_t::read_t read(rget_read);
    try {
        rdb_protocol_t::read_response_t res;
        ns_access.get_namespace_if()->read(read, &res, order_token_t::ignore, interruptor);
        rdb_protocol_t::rget_read_response_t *p_res = boost::get<rdb_protocol_t::rget_read_response_t>(&res.response);
        rassert(p_res);

        /* Re throw an exception if we got one. */
        if (runtime_exc_t *e = boost::get<runtime_exc_t>(&p_res->result)) {
            throw *e;
        }

        return p_res->result;
    } catch (cannot_perform_query_exc_t e) {
        throw runtime_exc_t("cannot perform read: " + std::string(e.what()), backtrace);
    }
}

void batched_rget_stream_t::read_more() {
    rdb_protocol_t::rget_read_t rget_read(range, batch_size, transform, scopes, backtrace);
    rdb_protocol_t::read_t read(rget_read);
    try {
        guarantee(ns_access.get_namespace_if());
        rdb_protocol_t::read_response_t res;
        ns_access.get_namespace_if()->read(read, &res, order_token_t::ignore, interruptor);
        rdb_protocol_t::rget_read_response_t *p_res = boost::get<rdb_protocol_t::rget_read_response_t>(&res.response);
        guarantee(p_res);

        /* Re throw an exception if we got one. */
        if (runtime_exc_t *e = boost::get<runtime_exc_t>(&p_res->result)) {
            throw *e;
        }

        // todo: just do a straight copy?
        typedef rdb_protocol_t::rget_read_response_t::stream_t stream_t;
        stream_t *stream = boost::get<stream_t>(&p_res->result);
        guarantee(stream);

        for (stream_t::iterator i = stream->begin(); i != stream->end(); ++i) {
            data.push_back(i->second);
            rassert(data.back());
        }

        range.left = p_res->last_considered_key;

        if (!range.left.increment()) {
            finished = true;
        }
    } catch (cannot_perform_query_exc_t e) {
        throw runtime_exc_t("cannot perform read: " + std::string(e.what()), backtrace);
    }
}

union_stream_t::union_stream_t(const stream_list_t &_streams)
    : streams(_streams), hd(streams.begin())
{ }

boost::shared_ptr<scoped_cJSON_t> union_stream_t::next() {
    while (hd != streams.end()) {
        if (boost::shared_ptr<scoped_cJSON_t> json = (*hd)->next()) {
            return json;
        } else {
            ++hd;
        }
    }
    return boost::shared_ptr<scoped_cJSON_t>();
}

void union_stream_t::add_transformation(const rdb_protocol_details::transform_atom_t &t) {
    for (stream_list_t::iterator it  = streams.begin();
                                 it != streams.end();
                                 ++it) {
        (*it)->add_transformation(t);
    }
}

result_t union_stream_t::apply_terminal(const rdb_protocol_details::terminal_t &) {
    crash("not implemented");
}

} //namespace query_language 
