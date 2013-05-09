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

result_t json_stream_t::apply_terminal(
    const rdb_protocol_details::terminal_variant_t &_t,
    ql::env_t *ql_env,
    const scopes_t &scopes,
    const backtrace_t &backtrace) {
    rdb_protocol_details::terminal_variant_t t = _t;
    result_t res;

    terminal_initialize(ql_env, scopes, backtrace, &t, &res);

    for (;;) {
        std::vector<boost::shared_ptr<scoped_cJSON_t> > jsons
            = next_batch(RECOMMENDED_MAX_SIZE);

        if (jsons.empty()) {
            break;
        }

        for (auto it = jsons.begin(); it != jsons.end(); ++it) {
            terminal_apply(ql_env, scopes, backtrace, *it, &t, &res);
            it->reset();
        }
    }

    return res;
}

std::vector<boost::shared_ptr<scoped_cJSON_t> >
pop_a_batch(std::list<boost::shared_ptr<scoped_cJSON_t> > *const data,
            const size_t max_size) {
    std::vector<boost::shared_ptr<scoped_cJSON_t> > ret;
    while (!data->empty() && ret.size() < max_size) {
        ret.push_back(std::move(data->front()));
        data->pop_front();
    }
    return ret;
}

transform_stream_t::transform_stream_t(boost::shared_ptr<json_stream_t> _stream,
                                       ql::env_t *_ql_env,
                                       const rdb_protocol_details::transform_t &tr) :
    stream(_stream),
    ql_env(_ql_env),
    transform(tr) { }

std::vector<boost::shared_ptr<scoped_cJSON_t> >
transform_stream_t::next_batch(const size_t max_size) {
    while (data.empty()) {
        std::vector<boost::shared_ptr<scoped_cJSON_t> > inputs
            = stream->next_batch(max_size);

        if (inputs.empty()) {
            // End of stream reached.
            return inputs;
        }

        std::list<boost::shared_ptr<scoped_cJSON_t> > accumulator(inputs.begin(),
                                                                  inputs.end());
        inputs.clear();

        //Apply transforms to the data
        typedef rdb_protocol_details::transform_t::iterator tit_t;
        for (tit_t it  = transform.begin();
                   it != transform.end();
                   ++it) {
            json_list_t tmp;
            for (auto jt = accumulator.begin(); jt != accumulator.end(); ++jt) {
                transform_apply(ql_env, it->scopes, it->backtrace, *jt, &it->variant, &tmp);
            }

            accumulator = std::move(tmp);
        }

        data = std::move(accumulator);
    }

    // Why not just return the whole batch?  To slow down chains of explosive
    // transformers.
    return pop_a_batch(&data, max_size);
}

boost::shared_ptr<json_stream_t> transform_stream_t::add_transformation(const rdb_protocol_details::transform_variant_t &t, UNUSED ql::env_t *ql_env2, const scopes_t &scopes, const backtrace_t &backtrace) {
    transform.push_back(rdb_protocol_details::transform_atom_t(t, scopes, backtrace));
    return shared_from_this();
}

batched_rget_stream_t::batched_rget_stream_t(
    const namespace_repo_t<rdb_protocol_t>::access_t &_ns_access,
    signal_t *_interruptor,
    counted_t<const ql::datum_t> left_bound,
    counted_t<const ql::datum_t> right_bound,
    const std::map<std::string, ql::wire_func_t> &_optargs,
    bool _use_outdated)
    : ns_access(_ns_access), interruptor(_interruptor),
      finished(false), started(false), optargs(_optargs), use_outdated(_use_outdated),
      range(key_range_t::closed,
            left_bound.has()
              ? store_key_t(left_bound->print_primary())
              : store_key_t::min(),
            key_range_t::closed,
            right_bound.has()
              ? store_key_t(right_bound->print_primary())
              : store_key_t::max()),
      table_scan_backtrace()
{ }

batched_rget_stream_t::batched_rget_stream_t(
    const namespace_repo_t<rdb_protocol_t>::access_t &_ns_access,
    signal_t *_interruptor, const std::string &_sindex_id,
    const std::map<std::string, ql::wire_func_t> &_optargs,
    bool _use_outdated,
    counted_t<const ql::datum_t> _sindex_start_value,
    counted_t<const ql::datum_t> _sindex_end_value)
    : ns_access(_ns_access),
      interruptor(_interruptor),
      sindex_id(_sindex_id),
      finished(false),
      started(false),
      optargs(_optargs),
      use_outdated(_use_outdated),
      sindex_start_value(_sindex_start_value),
      sindex_end_value(_sindex_end_value),
      range(rdb_protocol_t::sindex_key_range(
                _sindex_start_value != NULL
                  ? _sindex_start_value->truncated_secondary()
                  : store_key_t::min(),
                _sindex_end_value != NULL
                  ? _sindex_end_value->truncated_secondary()
                  : store_key_t::max())),
      table_scan_backtrace()
{ }

std::vector<boost::shared_ptr<scoped_cJSON_t> >
batched_rget_stream_t::next_batch(size_t max_size) {
    started = true;
    if (data.empty()) {
        if (finished) {
            return std::vector<boost::shared_ptr<scoped_cJSON_t> >();
        }
        read_more();
        if (data.empty()) {
            finished = true;
            return std::vector<boost::shared_ptr<scoped_cJSON_t> >();
        }
    }

    return pop_a_batch(&data, max_size);
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
    rdb_protocol_t::rget_read_t rget_read = get_rget();
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
        } else if (ql::datum_exc_t *e3 = boost::get<ql::datum_exc_t>(&p_res->result)) {
            throw *e3;
        }

        return p_res->result;
    } catch (const cannot_perform_query_exc_t &e) {
        if (table_scan_backtrace) {
            throw runtime_exc_t("cannot perform read: " + std::string(e.what()), *table_scan_backtrace);
        } else {
            // No backtrace for these.
            throw ql::exc_t("cannot perform read: " + std::string(e.what()),
                            ql::backtrace_t());
        }
    }
}

rdb_protocol_t::rget_read_t batched_rget_stream_t::get_rget() {
    if (!sindex_id) {
        return rdb_protocol_t::rget_read_t(rdb_protocol_t::region_t(range),
                                           transform,
                                           optargs);
    } else {
        return rdb_protocol_t::rget_read_t(rdb_protocol_t::region_t(range),
                                           *sindex_id,
                                           sindex_start_value,
                                           sindex_end_value,
                                           transform,
                                           optargs);
    }
}

void batched_rget_stream_t::read_more() {
    rdb_protocol_t::read_t read(get_rget());
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
        if (auto e = boost::get<runtime_exc_t>(&p_res->result)) {
            throw *e;
        } else if (auto e2 = boost::get<ql::exc_t>(&p_res->result)) {
            throw *e2;
        } else if (auto e3 = boost::get<ql::datum_exc_t>(&p_res->result)) {
            throw *e3;
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
    } catch (const cannot_perform_query_exc_t &e) {
        if (table_scan_backtrace) {
            throw runtime_exc_t("cannot perform read: " + std::string(e.what()), *table_scan_backtrace);
        } else {
            // No backtrace.
            throw ql::exc_t("cannot perform read: " + std::string(e.what()),
                            ql::backtrace_t());
        }
    }
}

} //namespace query_language
