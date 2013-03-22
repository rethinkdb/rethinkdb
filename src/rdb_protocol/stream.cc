// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "rdb_protocol/stream.hpp"

#include "rdb_protocol/ql2.hpp"
#include "rdb_protocol/transform_visitors.hpp"

namespace query_language {

boost::shared_ptr<json_stream_t> json_stream_t::add_transformation(const rdb_protocol_details::transform_variant_t &t, ql::env_t *ql_env, const scopes_t &scopes, const backtrace_t &backtrace) {
    rdb_protocol_details::transform_t transform;
    transform.push_back(rdb_protocol_details::transform_atom_t(t, scopes, backtrace));
    return boost::make_shared<transform_stream_t>(shared_from_this(), ql_env, transform);
}

boost::shared_ptr<scoped_cJSON_t> json_stream_t::next() {
    // We loop until we get a value.  Somethings things need to return a NULL value while
    // returning END_OF_BATCH.
    for (;;) {
        boost::shared_ptr<scoped_cJSON_t> ret;
        const batch_info_t res = next_impl(&ret);
        if (ret || res == END_OF_STREAM) {
            return ret;
        }
    }
}

result_t json_stream_t::apply_terminal(
    const rdb_protocol_details::terminal_variant_t &_t,
    ql::env_t *ql_env,
    const scopes_t &scopes,
    const backtrace_t &backtrace) {
    rdb_protocol_details::terminal_variant_t t = _t;
    result_t res;
    boost::apply_visitor(terminal_initializer_visitor_t(&res, ql_env, scopes, backtrace), t);
    boost::shared_ptr<scoped_cJSON_t> json;
    while ((json = next())) {
        boost::apply_visitor(terminal_visitor_t(json, ql_env,
                                                scopes, backtrace, &res),
                             t);
    }
    return res;
}

in_memory_stream_t::in_memory_stream_t(json_array_iterator_t it) : index(0) {
    while (cJSON *json = it.next()) {
        data.push_back(boost::shared_ptr<scoped_cJSON_t>(new scoped_cJSON_t(cJSON_DeepCopy(json))));
    }
}

in_memory_stream_t::in_memory_stream_t(boost::shared_ptr<json_stream_t> stream) : index(0) {
    while (boost::shared_ptr<scoped_cJSON_t> json = stream->next()) {
        data.push_back(json);
    }
}

batch_info_t in_memory_stream_t::next_impl(boost::shared_ptr<scoped_cJSON_t> *out) {
    if (data.empty()) {
        *out = boost::shared_ptr<scoped_cJSON_t>();
        return END_OF_STREAM;
    } else {
        *out = data.front();
        data.pop_front();
        ++index;
        return index % json_stream_t::RECOMMENDED_BATCH_SIZE == 0 ? LAST_OF_BATCH : MID_BATCH;
    }
}

transform_stream_t::transform_stream_t(boost::shared_ptr<json_stream_t> _stream,
                                       ql::env_t *_ql_env,
                                       const rdb_protocol_details::transform_t &tr) :
    stream(_stream),
    ql_env(_ql_env),
    transform(tr) { }

batch_info_t transform_stream_t::next_impl(boost::shared_ptr<scoped_cJSON_t> *out) {
    while (data.empty()) {
        boost::shared_ptr<scoped_cJSON_t> input;
        const batch_info_t res = stream->next_impl(&input);
        if (!input) {
            *out = input;
            return res;
        } else {
            data_end_batch_info = res;
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
                boost::apply_visitor(transform_visitor_t(*jt, &tmp, ql_env, it->scopes, it->backtrace), it->variant);
            }

            /* Equivalent to `accumulator = tmp`, but without the extra copying */
            std::swap(accumulator, tmp);
        }

        /* Equivalent to `data = accumulator`, but without the extra copying */
        std::swap(data, accumulator);

        // We have to report the last-of-batch marker even if there's no data to come with it.
        // We can (and should) skip mid-batch data though.
        if (res == LAST_OF_BATCH) {
            *out = boost::shared_ptr<scoped_cJSON_t>();
            return LAST_OF_BATCH;
        }
    }

    boost::shared_ptr<scoped_cJSON_t> datum = data.front();
    data.pop_front();
    *out = datum;
    return data.empty() ? data_end_batch_info : MID_BATCH;
}

boost::shared_ptr<json_stream_t> transform_stream_t::add_transformation(const rdb_protocol_details::transform_variant_t &t, UNUSED ql::env_t *ql_env2, const scopes_t &scopes, const backtrace_t &backtrace) {
    transform.push_back(rdb_protocol_details::transform_atom_t(t, scopes, backtrace));
    return shared_from_this();
}

// batched_rget_stream_t::batched_rget_stream_t(const namespace_repo_t<rdb_protocol_t>::access_t &_ns_access,
//                                              signal_t *_interruptor, key_range_t _range,
//                                              const backtrace_t &_table_scan_backtrace,
//                                              bool _use_outdated)
//     : ns_access(_ns_access), interruptor(_interruptor),
//       range(_range),
//       finished(false), started(false), use_outdated(_use_outdated),
//       table_scan_backtrace(_table_scan_backtrace)
// { }

batched_rget_stream_t::batched_rget_stream_t(
    const namespace_repo_t<rdb_protocol_t>::access_t &_ns_access,
    signal_t *_interruptor, key_range_t _range,
    const std::map<std::string, ql::wire_func_t> &_optargs,
    bool _use_outdated)
    : ns_access(_ns_access), interruptor(_interruptor),
      range(_range),
      finished(false), started(false), optargs(_optargs), use_outdated(_use_outdated),
      table_scan_backtrace()
{ }

batch_info_t batched_rget_stream_t::next_impl(boost::shared_ptr<scoped_cJSON_t> *out) {
    started = true;
    if (data.empty()) {
        if (finished) {
            *out = boost::shared_ptr<scoped_cJSON_t>();
            return END_OF_STREAM;
        }
        read_more();
        if (data.empty()) {
            finished = true;
            *out = boost::shared_ptr<scoped_cJSON_t>();
            return END_OF_STREAM;
        }
    }

    *out = data.front();
    data.pop_front();
    return data.empty() ? LAST_OF_BATCH : MID_BATCH;
}

boost::shared_ptr<json_stream_t> batched_rget_stream_t::add_transformation(const rdb_protocol_details::transform_variant_t &t, UNUSED ql::env_t *ql_env2, const scopes_t &scopes, const backtrace_t &per_op_backtrace) {
    guarantee(!started);
    transform.push_back(rdb_protocol_details::transform_atom_t(t, scopes, per_op_backtrace));
    return shared_from_this();
}

result_t batched_rget_stream_t::apply_terminal(
    const rdb_protocol_details::terminal_variant_t &t,
    UNUSED ql::env_t *ql_env,
    const scopes_t &scopes,
    const backtrace_t &per_op_backtrace) {
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
        } else if (ql::exc_t *e2 = boost::get<ql::exc_t>(&p_res->result)) {
            throw *e2;
        }

        return p_res->result;
    } catch (cannot_perform_query_exc_t e) {
        if (table_scan_backtrace) {
            throw runtime_exc_t("cannot perform read: " + std::string(e.what()), *table_scan_backtrace);
        } else {
            // No backtrace for these.
            throw ql::exc_t("cannot perform read: " + std::string(e.what()),
                            ql::backtrace_t());
        }
    }
}

void batched_rget_stream_t::read_more() {
    rdb_protocol_t::rget_read_t rget_read(
        rdb_protocol_t::region_t(range), transform, optargs);
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
            //BREAKPOINT;
            throw *e;
        } else if (ql::exc_t *e2 = boost::get<ql::exc_t>(&p_res->result)) {
            throw *e2;
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
        if (table_scan_backtrace) {
            throw runtime_exc_t("cannot perform read: " + std::string(e.what()), *table_scan_backtrace);
        } else {
            // No backtrace.
            throw ql::exc_t("cannot perform read: " + std::string(e.what()),
                            ql::backtrace_t());
        }
    }
}

union_stream_t::union_stream_t(const stream_list_t &_streams)
    : streams(_streams), hd(streams.begin())
{ }

batch_info_t union_stream_t::next_impl(boost::shared_ptr<scoped_cJSON_t> *out) {
    while (hd != streams.end()) {
        boost::shared_ptr<scoped_cJSON_t> json;
        const batch_info_t res = (*hd)->next_impl(&json);
        if (res != END_OF_STREAM) {
            *out = json;
            return res;
        } else {
            ++hd;
        }
    }
    *out = boost::shared_ptr<scoped_cJSON_t>();
    return END_OF_STREAM;
}

boost::shared_ptr<json_stream_t> union_stream_t::add_transformation(const rdb_protocol_details::transform_variant_t &t, ql::env_t *ql_env, const scopes_t &scopes, const backtrace_t &backtrace) {
    for (stream_list_t::iterator it  = streams.begin();
                                 it != streams.end();
                                 ++it) {
        *it = (*it)->add_transformation(t, ql_env, scopes, backtrace);
    }
    return shared_from_this();
}

} //namespace query_language
