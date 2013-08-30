#ifndef RDB_PROTOCOL_FUNC_HPP_
#define RDB_PROTOCOL_FUNC_HPP_

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "errors.hpp"
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/variant/static_visitor.hpp>

#include "containers/counted.hpp"
#include "containers/uuid.hpp"
#include "rdb_protocol/datum.hpp"
#include "rdb_protocol/sym.hpp"
#include "rdb_protocol/term.hpp"
#include "rpc/serialize_macros.hpp"

class js_runner_t;

namespace ql {

class func_t : public single_threaded_countable_t<func_t>, public pb_rcheckable_t {
public:
    func_t(env_t *env,
           const std::string &_js_source,
           uint64_t timeout_ms,
           counted_t<term_t> parent);
    func_t(env_t *env, protob_t<const Term> _source);
    // Some queries, like filter, can take a shortcut object instead of a
    // function as their argument.
    static counted_t<func_t> new_constant_func(
        env_t *env, counted_t<const datum_t> obj,
        const protob_t<const Backtrace> &root);

    static counted_t<func_t> new_pluck_func(
        env_t *env, counted_t<const datum_t> obj,
        const protob_t<const Backtrace> &bt_src);

    static counted_t<func_t> new_get_field_func(
        env_t *env, counted_t<const datum_t> obj,
        const protob_t<const Backtrace> &bt_src);

    static counted_t<func_t> new_eq_comparison_func(
        env_t *env, counted_t<const datum_t> obj,
        const protob_t<const Backtrace> &bt_src);

    counted_t<val_t> call(const std::vector<counted_t<const datum_t> > &args) const;

    // Prefer these versions of call.
    counted_t<val_t> call() const;
    counted_t<val_t> call(counted_t<const datum_t> arg) const;
    counted_t<val_t> call(counted_t<const datum_t> arg1, counted_t<const datum_t> arg2) const;
    bool filter_call(counted_t<const datum_t> arg, counted_t<func_t> default_filter_val) const;

    void dump_scope(std::map<sym_t, Datum> *out) const;
    bool is_deterministic() const;
    void assert_deterministic(const char *extra_msg) const;

    std::string print_src() const;
    protob_t<const Term> get_source() const;
private:
    // Pointers to this function's arguments.
    scoped_array_t<counted_t<const datum_t> > argptrs;
    counted_t<term_t> body; // body to evaluate with functions bound

    // This is what's serialized over the wire.
    friend class wire_func_t;
    protob_t<const Term> source;

    // TODO: make this smarter (it's sort of slow and shitty as-is)
    std::map<sym_t, counted_t<const datum_t> *> scope;

    // RSI: It seems there are two kinds of functions, this js stuff doesn't get used
    // most of the time.
    counted_t<term_t> js_parent;
    env_t *js_env;
    std::string js_source;
    uint64_t js_timeout_ms;
};


class js_result_visitor_t : public boost::static_visitor<counted_t<val_t> > {
public:
    js_result_visitor_t(env_t *_env,
                        const std::string &_code,
                        uint64_t _timeout_ms,
                        counted_t<term_t> _parent)
        : env(_env),
          code(_code),
          timeout_ms(_timeout_ms),
          parent(_parent) { }
    // This JS evaluation resulted in an error
    counted_t<val_t> operator()(const std::string &err_val) const;
    // This JS call resulted in a JSON value
    counted_t<val_t> operator()(const counted_t<const datum_t> &json_val) const;
    // This JS evaluation resulted in an id for a js function
    counted_t<val_t> operator()(const id_t id_val) const;
private:
    env_t *env;
    std::string code;
    uint64_t timeout_ms;
    counted_t<term_t> parent;
};

// Evaluating this returns a `func_t` wrapped in a `val_t`.
class func_term_t : public term_t {
public:
    func_term_t(env_t *env, const protob_t<const Term> &term);
private:
    virtual bool is_deterministic_impl() const;
    virtual counted_t<val_t> eval_impl(eval_flags_t flags);
    virtual const char *name() const { return "func"; }
    counted_t<func_t> func;
};

} // namespace ql
#endif // RDB_PROTOCOL_FUNC_HPP_
