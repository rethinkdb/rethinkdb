#include "rdb_protocol/stream.hpp"
#include "rdb_protocol/environment.hpp"
#include "rdb_protocol/transform_visitors.hpp"

namespace query_language {

boost::shared_ptr<json_stream_t> json_stream_t::add_transformation(const rdb_protocol_details::transform_variant_t &t, runtime_environment_t *env, const scopes_t &scopes, const backtrace_t &backtrace) {
    rdb_protocol_details::transform_t transform;
    transform.push_back(rdb_protocol_details::transform_atom_t(t, scopes, backtrace));
    return boost::make_shared<transform_stream_t>(shared_from_this(), env, transform);
}

result_t json_stream_t::apply_terminal(const rdb_protocol_details::terminal_variant_t &t, runtime_environment_t *env, const scopes_t &scopes, const backtrace_t &backtrace) {
    result_t res;
    boost::apply_visitor(terminal_initializer_visitor_t(&res, env, scopes, backtrace), t);
    boost::shared_ptr<scoped_cJSON_t> json;
    while ((json = next())) boost::apply_visitor(terminal_visitor_t(json, env, scopes, backtrace, &res), t);
    return res;
}

in_memory_stream_t::in_memory_stream_t(json_array_iterator_t it) {
    while (cJSON *json = it.next()) {
        data.push_back(boost::shared_ptr<scoped_cJSON_t>(new scoped_cJSON_t(cJSON_DeepCopy(json))));
    }
}

in_memory_stream_t::in_memory_stream_t(boost::shared_ptr<json_stream_t> stream) {
    while (boost::shared_ptr<scoped_cJSON_t> json = stream->next()) {
        data.push_back(json);
    }
}

boost::shared_ptr<scoped_cJSON_t> in_memory_stream_t::next() {
    if (data.empty()) {
        return boost::shared_ptr<scoped_cJSON_t>();
    } else {
        boost::shared_ptr<scoped_cJSON_t> res = data.front();
        data.pop_front();
        return res;
    }
}

transform_stream_t::transform_stream_t(boost::shared_ptr<json_stream_t> stream_, runtime_environment_t *env_, const rdb_protocol_details::transform_t &tr)
    : stream(stream_), env(env_), transform(tr) { }

boost::shared_ptr<scoped_cJSON_t> transform_stream_t::next() {
    while (data.empty()) {
        boost::shared_ptr<scoped_cJSON_t> input = stream->next();
        if (!input) {
            return boost::shared_ptr<scoped_cJSON_t>();
        }

        json_list_t accumulator;
        accumulator.push_back(input);

        //Apply transforms to the data
        typedef rdb_protocol_details::transform_t::iterator tit_t;
        for (tit_t it  = transform.begin();
                   it != transform.end();
                   ++it) {
            json_list_t tmp;
            for (json_list_t::iterator jt  = accumulator.begin();
                                       jt != accumulator.end();
                                       ++jt) {
                boost::apply_visitor(transform_visitor_t(*jt, &tmp, env, it->scopes, it->backtrace), it->variant);
            }

            /* Equivalent to `accumulator = tmp`, but without the extra copying */
            std::swap(accumulator, tmp);
        }

        /* Equivalent to `data = accumulator`, but without the extra copying */
        std::swap(data, accumulator);
    }

    boost::shared_ptr<scoped_cJSON_t> res = data.front();
    data.pop_front();
    return res;
}

boost::shared_ptr<json_stream_t> transform_stream_t::add_transformation(const rdb_protocol_details::transform_variant_t &t, UNUSED runtime_environment_t *env2, const scopes_t &scopes, const backtrace_t &backtrace) {
    transform.push_back(rdb_protocol_details::transform_atom_t(t, scopes, backtrace));
    return shared_from_this();
}

batched_rget_stream_t::batched_rget_stream_t(const namespace_repo_t<rdb_protocol_t>::access_t &_ns_access,
                      signal_t *_interruptor, key_range_t _range,
                      int _batch_size, const backtrace_t &_table_scan_backtrace,
                      bool _use_outdated)
    : ns_access(_ns_access), interruptor(_interruptor),
      range(_range), batch_size(_batch_size), index(0),
      finished(false), started(false), use_outdated(_use_outdated),
      table_scan_backtrace(_table_scan_backtrace)
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

boost::shared_ptr<json_stream_t> batched_rget_stream_t::add_transformation(const rdb_protocol_details::transform_variant_t &t, UNUSED runtime_environment_t *env2, const scopes_t &scopes, const backtrace_t &per_op_backtrace) {
    guarantee(!started);
    transform.push_back(rdb_protocol_details::transform_atom_t(t, scopes, per_op_backtrace));
    return shared_from_this();
}

result_t batched_rget_stream_t::apply_terminal(const rdb_protocol_details::terminal_variant_t &t, UNUSED runtime_environment_t *env2, const scopes_t &scopes, const backtrace_t &per_op_backtrace) {
    rdb_protocol_t::region_t region(range);
    rdb_protocol_t::rget_read_t rget_read(region);
    rget_read.transform = transform;
    rget_read.terminal = rdb_protocol_details::terminal_t(t, scopes, per_op_backtrace);
    rdb_protocol_t::read_t read(rget_read);
    try {
        rdb_protocol_t::read_response_t res;
        if (use_outdated) {
            ns_access.get_namespace_if()->read_outdated(read, &res, interruptor);
        } else {
            ns_access.get_namespace_if()->read(read, &res, order_token_t::ignore, interruptor);
        }
        rdb_protocol_t::rget_read_response_t *p_res = boost::get<rdb_protocol_t::rget_read_response_t>(&res.response);
        guarantee(p_res);

        /* Re throw an exception if we got one. */
        if (runtime_exc_t *e = boost::get<runtime_exc_t>(&p_res->result)) {
            throw *e;
        }

        return p_res->result;
    } catch (cannot_perform_query_exc_t e) {
        throw runtime_exc_t("cannot perform read: " + std::string(e.what()), table_scan_backtrace);
    }
}

void batched_rget_stream_t::read_more() {
    rdb_protocol_t::rget_read_t rget_read(rdb_protocol_t::region_t(range), transform);
    rdb_protocol_t::read_t read(rget_read);
    try {
        guarantee(ns_access.get_namespace_if());
        rdb_protocol_t::read_response_t res;
        if (use_outdated) {
            ns_access.get_namespace_if()->read_outdated(read, &res, interruptor);
        } else {
            ns_access.get_namespace_if()->read(read, &res, order_token_t::ignore, interruptor);
        }
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
            guarantee(i->second);
            data.push_back(i->second);
        }

        range.left = p_res->last_considered_key;

        if (!range.left.increment()) {
            finished = true;
        }
    } catch (cannot_perform_query_exc_t e) {
        throw runtime_exc_t("cannot perform read: " + std::string(e.what()), table_scan_backtrace);
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

boost::shared_ptr<json_stream_t> union_stream_t::add_transformation(const rdb_protocol_details::transform_variant_t &t, runtime_environment_t *env, const scopes_t &scopes, const backtrace_t &backtrace) {
    for (stream_list_t::iterator it  = streams.begin();
                                 it != streams.end();
                                 ++it) {
        *it = (*it)->add_transformation(t, env, scopes, backtrace);
    }
    return shared_from_this();
}

} //namespace query_language 
