#ifndef RDB_PROTOCOL_TERMS_JS_HPP_
#define RDB_PROTOCOL_TERMS_JS_HPP_

#include <string>

#include <boost/variant/static_visitor.hpp>

#include "rdb_protocol/op.hpp"
#include "rdb_protocol/err.hpp"
#include "rdb_protocol/js.hpp"

namespace ql {

// The result of evaluating a JS term that defines a function
class js_func_t : public func_t {
public:
    js_func_t(id_t id, env_t *_env, term_t *_parent) : js_func_id(id), env(_env), parent(_parent) { }

    val_t *call(const std::vector<const datum_t *> &args);

    bool is_deterministic() const { return false; }

private:
    id_t js_func_id;
    env_t *env;
    term_t *parent;
};

class js_result_visitor_t : public boost::static_visitor<val_t *> {
public:
    typedef val_t *result_type;

    js_result_visitor_t(env_t *_env, term_t *_parent) : env(_env), parent(_parent) { }

    // This JS evaluation resulted in an error
    result_type operator()(const std::string err_val) const {
        rfail_target(parent, "%s", err_val.c_str());
        unreachable();
    }

    // This JS call resulted in a JSON value
    result_type operator()(const boost::shared_ptr<scoped_cJSON_t> json_val) const {
        return parent->new_val(new datum_t(json_val, env));
    }

    // This JS evaluation resulted in an id for a js function
    result_type operator()(const id_t id_val) const {
        return parent->new_val(new js_func_t(id_val, env, parent));
    }

private:
    env_t *env;
    term_t *parent;
};

// Has to be defined after js_result_visitor_t
val_t *js_func_t::call(const std::vector<const datum_t *> &args) {

    // Convert datum args to cJSON args for the JS runner
    std::vector<boost::shared_ptr<scoped_cJSON_t> > json_args;
    for (const datum_t *arg : args) {
        json_args.push_back(arg->as_json());
    }

    boost::shared_ptr<js::runner_t> js = env->get_js_runner();
    js::js_result_t result = js->call(js_func_id, json_args);

    return boost::apply_visitor(js_result_visitor_t(env, parent), result);
}

class javascript_term_t : public op_term_t {
public:
    javascript_term_t(env_t *env, const Term2 *term) : op_term_t(env, term, argspec_t(1)) { }
private:

    virtual val_t *eval_impl() {
        std::string source = arg(0)->as_datum()->as_str();

        boost::shared_ptr<js::runner_t> js = env->get_js_runner();
        js::js_result_t result = js->eval(source);

        return boost::apply_visitor(js_result_visitor_t(env, this), result);
    }
    RDB_NAME("javascript");
};

}

#endif // RDB_PROTOCOL_TERMS_JS_HPP_
