#include "rdb_protocol/transform_visitors.hpp"
#include "rdb_protocol/query_language.hpp"


namespace query_language {

transform_visitor_t::transform_visitor_t(boost::shared_ptr<scoped_cJSON_t> _json, json_list_t *_out, query_language::runtime_environment_t *_env, const scopes_t &_scopes, const backtrace_t &_backtrace)
    : json(_json), out(_out), env(_env), scopes(_scopes), backtrace(_backtrace)
{ }

void transform_visitor_t::operator()(const Builtin_Filter &filter) const {
    if (query_language::predicate_t(filter.predicate(), env, scopes, backtrace)(json)) {
        out->push_back(json);
    }
}

void transform_visitor_t::operator()(const Mapping &mapping) const {
    Term t = mapping.body();
    out->push_back(query_language::map(mapping.arg(), &t, env, scopes, backtrace, json));
}

void transform_visitor_t::operator()(const Builtin_ConcatMap &concatmap) const {
    Term t = concatmap.mapping().body();
    boost::shared_ptr<json_stream_t> stream = query_language::concatmap(concatmap.mapping().arg(), &t, env, scopes, backtrace, json);
    while (boost::shared_ptr<scoped_cJSON_t> data = stream->next()) {
        out->push_back(data);
    }
}

void transform_visitor_t::operator()(Builtin_Range range) const {
    boost::shared_ptr<scoped_cJSON_t> lowerbound, upperbound;

    key_range_t key_range;

    /* TODO this is inefficient because it involves recomputing this for each element.
    TODO this is probably also incorrect because the term may be nondeterministic */
    if (range.has_lowerbound()) {
        lowerbound = eval_term_as_json(range.mutable_lowerbound(), env, scopes, backtrace.with("lowerbound"));
    }

    if (range.has_upperbound()) {
        upperbound = eval_term_as_json(range.mutable_upperbound(), env, scopes, backtrace.with("upperbound"));
    }

    if (lowerbound && upperbound) {
        key_range = key_range_t(key_range_t::closed, store_key_t(lowerbound->Print()),
                key_range_t::closed, store_key_t(upperbound->Print()));
    } else if (lowerbound) {
        key_range = key_range_t(key_range_t::closed, store_key_t(lowerbound->Print()),
                key_range_t::none, store_key_t());
    } else if (upperbound) {
        key_range = key_range_t(key_range_t::none, store_key_t(),
                key_range_t::closed, store_key_t(upperbound->Print()));
    }


    cJSON* val = json->GetObjectItem(range.attrname().c_str());

    if (val && key_range.contains_key(store_key_t(cJSON_print_std_string(val)))) {
        out->push_back(json);
    }
}

terminal_initializer_visitor_t::terminal_initializer_visitor_t(rget_read_response_t::result_t *_out,
                                                               query_language::runtime_environment_t *_env,
                                                               const scopes_t &_scopes,
                                                               const backtrace_t &_backtrace)
    : out(_out), env(_env), scopes(_scopes), backtrace(_backtrace)
{ }

void terminal_initializer_visitor_t::operator()(const Builtin_GroupedMapReduce &) const { *out = rget_read_response_t::groups_t(); }

void terminal_initializer_visitor_t::operator()(const Reduction &r) const {
    Term base = r.base();
    *out = eval_term_as_json(&base, env, scopes, backtrace.with("base"));
}

void terminal_initializer_visitor_t::operator()(const rdb_protocol_details::Length &) const {
    rget_read_response_t::length_t l;
    l.length = 0;
    *out = l;
}

void terminal_initializer_visitor_t::operator()(const WriteQuery_ForEach &) const {
    rget_read_response_t::inserted_t i;
    i.inserted = 0;
    *out = i;
}

terminal_visitor_t::terminal_visitor_t(boost::shared_ptr<scoped_cJSON_t> _json,
                   query_language::runtime_environment_t *_env,
                   const scopes_t &_scopes,
                   const backtrace_t &_backtrace,
                   rget_read_response_t::result_t *_out)
    : json(_json), env(_env), scopes(_scopes), backtrace(_backtrace), out(_out)
{ }

void terminal_visitor_t::operator()(const Builtin_GroupedMapReduce &gmr) const {
    //we assume the result has already been set to groups_t
    rget_read_response_t::groups_t *res_groups = boost::get<rget_read_response_t::groups_t>(out);
    guarantee(res_groups);

    //Grab the grouping
    boost::shared_ptr<scoped_cJSON_t> grouping;
    {
        Term body = gmr.group_mapping().body();
        grouping = query_language::map(gmr.group_mapping().arg(), &body, env, scopes, backtrace.with("group_mapping"), json);
    }

    //Apply the mapping
    boost::shared_ptr<scoped_cJSON_t> mapped_value;
    {
        Term body = gmr.value_mapping().body();
        mapped_value = query_language::map(gmr.value_mapping().arg(), &body, env, scopes, backtrace.with("value_mapping"), json);
    }

    //Finally reduce it in
    {
        Term base = gmr.reduction().base(),
             body = gmr.reduction().body();

        scopes_t scopes_copy = scopes;
        new_val_scope_t inner_scope(&scopes_copy.scope);
        scopes_copy.scope.put_in_scope(gmr.reduction().var1(), get_with_default(*res_groups, grouping, eval_term_as_json(&base, env, scopes, backtrace.with("reduction").with("base"))));
        scopes_copy.scope.put_in_scope(gmr.reduction().var2(), mapped_value);
        (*res_groups)[grouping] = eval_term_as_json(&body, env, scopes_copy, backtrace.with("reduction").with("body"));
    }
}

void terminal_visitor_t::operator()(const Reduction &r) const {
    //we assume the result has already been set to groups_t
    rget_read_response_t::atom_t *res_atom = boost::get<rget_read_response_t::atom_t>(out);
    guarantee(res_atom);
    guarantee(*res_atom);

    scopes_t scopes_copy = scopes;
    new_val_scope_t inner_scope(&scopes_copy.scope);
    scopes_copy.scope.put_in_scope(r.var1(), *res_atom);
    scopes_copy.scope.put_in_scope(r.var2(), json);
    Term body = r.body();
    *res_atom = eval_term_as_json(&body, env, scopes_copy, backtrace.with("body"));
}

void terminal_visitor_t::operator()(const rdb_protocol_details::Length &) const {
    rget_read_response_t::length_t *res_length = boost::get<rget_read_response_t::length_t>(out);
    guarantee(res_length);
    res_length->length++;
}

void terminal_visitor_t::operator()(const WriteQuery_ForEach &w) const {
    scopes_t scopes_copy = scopes;
    new_val_scope_t inner_scope(&scopes_copy.scope);
    scopes_copy.scope.put_in_scope(w.var(), json);

    for (int i = 0; i < w.queries_size(); ++i) {
        WriteQuery q = w.queries(i);
        Response r; //TODO we need to actually return this somewhere I suppose.
        execute_write_query(&q, env, &r, scopes_copy, backtrace.with(strprintf("queries:%d", i)));
    }
}

} //namespace query_language
