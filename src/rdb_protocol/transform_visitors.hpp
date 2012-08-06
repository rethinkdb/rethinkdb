#ifndef RDB_PROTOCOL_TRANSFORM_VISITORS_HPP_
#define RDB_PROTOCOL_TRANSFORM_VISITORS_HPP_

typedef std::list<boost::shared_ptr<scoped_cJSON_t> > json_list_t;

/* A visitor for applying a transformation to a bit of json. */
class transform_visitor_t : public boost::static_visitor<void> {
public:
    transform_visitor_t(boost::shared_ptr<scoped_cJSON_t> _json, json_list_t *_out, query_language::runtime_environment_t *_env)
        : json(_json), out(_out), env(_env)
    { }

    void operator()(const Builtin_Filter &filter) const {
        query_language::backtrace_t b; //TODO get this from somewhere
        if (query_language::predicate_t(filter.predicate(), *env, b)(json)) {
            out->push_back(json);
        }
    }

    void operator()(const Builtin_Map &map) const {
        query_language::backtrace_t b; //TODO get this from somewhere
        Term t = map.mapping().body();
        out->push_back(query_language::map(map.mapping().arg(), &t, *env, json, b));
    }

    void operator()(const Builtin_ConcatMap &concatmap) const {
        query_language::backtrace_t b; //TODO get this from somewhere
        Term t = concatmap.mapping().body();
        boost::shared_ptr<json_stream_t> stream = query_language::concatmap(concatmap.mapping().arg(), &t, *env, json, b);
        while (boost::shared_ptr<scoped_cJSON_t> data = stream->next()) {
            out->push_back(data);
        }
    }

    void operator()(Builtin_Range range) const {
        boost::shared_ptr<scoped_cJSON_t> lowerbound, upperbound;
        query_language::backtrace_t b; //TODO get this from somewhere

        key_range_t key_range;

        /* TODO this is inefficient because it involves recomputing this for each element. */
        if (range.has_lowerbound()) {
            lowerbound = eval(range.mutable_lowerbound(), env, b.with("lowerbound"));
        }

        if (range.has_upperbound()) {
            upperbound = eval(range.mutable_upperbound(), env, b.with("upperbound"));
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

private:
    boost::shared_ptr<scoped_cJSON_t> json;
    json_list_t *out;
    query_language::runtime_environment_t *env;
};

/* A visitor for setting the result type based on a terminal. */
class terminal_initializer_visitor_t : public boost::static_visitor<void> {
public:
    terminal_initializer_visitor_t(rget_read_response_t::result_t *_out)
        : out(_out)
    { }

    void operator()(const Builtin_GroupedMapReduce &) const { *out = rget_read_response_t::groups_t(); }
        
    void operator()(const Reduction &) const { *out = rget_read_response_t::atom_t(); }

    void operator()(const rdb_protocol_details::Length &) const { *out = rget_read_response_t::length_t(); }

    void operator()(const WriteQuery_ForEach &) const { *out = rget_read_response_t::inserted_t(); }
private:
    rget_read_response_t::result_t *out;
};

/* A visitor for applying a terminal to a bit of json. */
class terminal_visitor_t : public boost::static_visitor<void> {
public:
    terminal_visitor_t(boost::shared_ptr<scoped_cJSON_t> _json, 
                       query_language::runtime_environment_t *_env, 
                       rget_read_response_t::result_t *_out)
        : json(_json), env(_env), out(_out)
    { }

    void operator()(const Builtin_GroupedMapReduce &gmr) const { 
        boost::shared_ptr<scoped_cJSON_t> json_cpy = json;
        query_language::backtrace_t b; //TODO get this from somewhere
        //we assume the result has already been set to groups_t
        rget_read_response_t::groups_t *res_groups = boost::get<rget_read_response_t::groups_t>(out);
        guarantee(res_groups);

        //Grab the grouping
        boost::shared_ptr<scoped_cJSON_t> grouping;
        {
            query_language::new_val_scope_t scope(&env->scope);
            Term body = gmr.group_mapping().body();

            env->scope.put_in_scope(gmr.group_mapping().arg(), json_cpy);
            grouping = eval(&body, env, b);
        }

        //Apply the mapping
        {
            query_language::new_val_scope_t scope(&env->scope);
            env->scope.put_in_scope(gmr.value_mapping().arg(), json_cpy);

            Term body = gmr.value_mapping().body();
            json_cpy = eval(&body, env, b);
        }

        //Finally reduce it in
        {
            query_language::new_val_scope_t scope(&env->scope);

            Term base = gmr.reduction().base(),
                 body = gmr.reduction().body();

            env->scope.put_in_scope(gmr.reduction().var1(), get_with_default(*res_groups, grouping, eval(&base, env, b)));
            env->scope.put_in_scope(gmr.reduction().var2(), json_cpy);
            (*res_groups)[grouping] = eval(&body, env, b);
        }
    }
        
    void operator()(const Reduction &r) const { 
        query_language::backtrace_t b; //TODO get this from somewhere
        //we assume the result has already been set to groups_t
        rget_read_response_t::atom_t *res_atom = boost::get<rget_read_response_t::atom_t>(out);
        guarantee(res_atom);

        query_language::new_val_scope_t scope(&env->scope);
        env->scope.put_in_scope(r.var1(), *res_atom);
        env->scope.put_in_scope(r.var2(), json);
        Term body = r.body();
        *res_atom = eval(&body, env, b);
    }

    void operator()(const rdb_protocol_details::Length &) const {
        rget_read_response_t::length_t *res_length = boost::get<rget_read_response_t::length_t>(out);
        guarantee(res_length);
        res_length->length++;
    }

    void operator()(const WriteQuery_ForEach &w) const {
        query_language::backtrace_t b; //TODO get this from somewhere

        query_language::new_val_scope_t scope(&env->scope);
        env->scope.put_in_scope(w.var(), json);

        for (int i = 0; i < w.queries_size(); ++i) {
            WriteQuery q = w.queries(i);
            Response r; //TODO we need to actually return this somewhere I suppose.
            execute(&q, env, &r, b);
        }
    }

private:
    boost::shared_ptr<scoped_cJSON_t> json;
    query_language::runtime_environment_t *env;
    rget_read_response_t::result_t *out;
};


#endif
