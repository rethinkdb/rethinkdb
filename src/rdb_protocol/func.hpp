// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_FUNC_HPP_
#define RDB_PROTOCOL_FUNC_HPP_

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "errors.hpp"
#include <boost/optional.hpp>
#include <boost/variant/static_visitor.hpp>

#include "containers/counted.hpp"
#include "containers/uuid.hpp"
#include "rdb_protocol/datum.hpp"
#include "rdb_protocol/env.hpp"
#include "rdb_protocol/sym.hpp"
#include "rdb_protocol/term.hpp"
#include "rdb_protocol/term_storage.hpp"
#include "rpc/serialize_macros.hpp"

class js_runner_t;

namespace ql {

class func_visitor_t;

class func_t : public slow_atomic_countable_t<func_t>, public bt_rcheckable_t {
public:
    virtual ~func_t();

    virtual scoped_ptr_t<val_t> call(
        env_t *env,
        const std::vector<datum_t> &args,
        eval_flags_t eval_flags = NO_FLAGS) const = 0;

    virtual boost::optional<size_t> arity() const = 0;

    virtual bool is_deterministic() const = 0;

    // Used by info_term_t.
    virtual std::string print_source() const = 0;

    // Similar to `print_source()`, but prints a JS expression of the form
    // `function(arg1, arg2, ...) { return body; }`. Fails if the func_t has a non-empty
    // captured scope.
    virtual std::string print_js_function() const = 0;

    virtual void visit(func_visitor_t *visitor) const = 0;

    void assert_deterministic(const char *extra_msg) const;

    bool filter_call(env_t *env,
                     datum_t arg,
                     counted_t<const func_t> default_filter_val) const;

    // These are simple, they call the vector version of call.
    scoped_ptr_t<val_t> call(env_t *env, eval_flags_t eval_flags = NO_FLAGS) const;
    scoped_ptr_t<val_t> call(env_t *env,
                             datum_t arg,
                             eval_flags_t eval_flags = NO_FLAGS) const;
    scoped_ptr_t<val_t> call(env_t *env,
                             datum_t arg1,
                             datum_t arg2,
                             eval_flags_t eval_flags = NO_FLAGS) const;

protected:
    explicit func_t(backtrace_id_t bt);

private:
    virtual bool filter_helper(env_t *env, datum_t arg) const = 0;

    DISABLE_COPYING(func_t);
};

class reql_func_t : public func_t {
public:
    // Used when constructing in an existing environment - reusing another term storage
    reql_func_t(const var_scope_t &captured_scope,
                std::vector<sym_t> arg_names,
                counted_t<const term_t> body);

    // Used when constructing from a function read off the wire
    reql_func_t(scoped_ptr_t<term_storage_t> &&_storage,
                const var_scope_t &captured_scope,
                std::vector<sym_t> arg_names,
                counted_t<const term_t> body);

    ~reql_func_t();

    scoped_ptr_t<val_t> call(
        env_t *env,
        const std::vector<datum_t> &args,
        eval_flags_t eval_flags) const;

    boost::optional<size_t> arity() const;

    bool is_deterministic() const;

    std::string print_source() const;
    std::string print_js_function() const;

    void visit(func_visitor_t *visitor) const;

private:
    template <cluster_version_t> friend class wire_func_serialization_visitor_t;
    bool filter_helper(env_t *env, datum_t arg) const;

    // Only contains the parts of the scope that `body` uses.
    var_scope_t captured_scope;

    // The argument names, for the corresponding positional argument number.
    std::vector<sym_t> arg_names;

    // Term storage if this was deserialized off the wire to ensure the lifetime of
    // the raw term tree.
    scoped_ptr_t<term_storage_t> term_storage;

    // The body of the function, which gets ->eval(...) called when call(...) is called.
    counted_t<const term_t> body;

    DISABLE_COPYING(reql_func_t);
};

class js_func_t : public func_t {
public:
    js_func_t(const std::string &_js_source,
              uint64_t timeout_ms,
              backtrace_id_t backtrace);
    ~js_func_t();

    // Some queries, like filter, can take a shortcut object instead of a
    // function as their argument.
    scoped_ptr_t<val_t> call(env_t *env,
                             const std::vector<datum_t> &args,
                             eval_flags_t eval_flags) const;

    boost::optional<size_t> arity() const;

    bool is_deterministic() const;

    std::string print_source() const;
    std::string print_js_function() const;

    void visit(func_visitor_t *visitor) const;

private:
    template <cluster_version_t> friend class wire_func_serialization_visitor_t;
    bool filter_helper(env_t *env, datum_t arg) const;

    std::string js_source;
    uint64_t js_timeout_ms;

    DISABLE_COPYING(js_func_t);
};

class func_visitor_t {
public:
    virtual void on_reql_func(const reql_func_t *reql_func) = 0;
    virtual void on_js_func(const js_func_t *js_func) = 0;
protected:
    func_visitor_t() { }
    virtual ~func_visitor_t() { }
    DISABLE_COPYING(func_visitor_t);
};

// Some queries, like filter, can take a shortcut object instead of a
// function as their argument.
counted_t<const func_t> new_constant_func(datum_t obj, backtrace_id_t bt);
counted_t<const func_t> new_pluck_func(datum_t obj, backtrace_id_t bt);
counted_t<const func_t> new_get_field_func(datum_t obj, backtrace_id_t bt);
counted_t<const func_t> new_eq_comparison_func(datum_t obj, backtrace_id_t bt);
counted_t<const func_t> new_page_func(datum_t method, backtrace_id_t bt);

class js_result_visitor_t : public boost::static_visitor<val_t *> {
public:
    js_result_visitor_t(const std::string &_code,
                        uint64_t _timeout_ms,
                        const bt_rcheckable_t *_parent)
        : code(_code),
          timeout_ms(_timeout_ms),
          parent(_parent) { }

    // The caller needs to take ownership of the return value.  Boost static_visitor
    // can't handle movable types.

    // This JS evaluation resulted in an error
    val_t *operator()(const std::string &err_val) const;
    // This JS call resulted in a JSON value
    val_t *operator()(const datum_t &json_val) const;
    // This JS evaluation resulted in an id for a js function
    val_t *operator()(const id_t id_val) const;

private:
    std::string code;
    uint64_t timeout_ms;
    const bt_rcheckable_t *parent;
};

// Evaluating this returns a `func_t` wrapped in a `val_t`.
class func_term_t : public term_t {
public:
    func_term_t(compile_env_t *env, const raw_term_t &term);

    // eval(scoped_env_t *env) is a dumb wrapper for this.  Evaluates the func_t without
    // going by way of val_t, and without requiring a full-blown env.
    counted_t<const func_t> eval_to_func(const var_scope_t &env_scope) const;

private:
    virtual void accumulate_captures(var_captures_t *captures) const;
    virtual bool is_deterministic() const;
    virtual scoped_ptr_t<val_t> term_eval(scope_env_t *env, eval_flags_t flags) const;
    virtual const char *name() const { return "func"; }

    std::vector<sym_t> arg_names;
    counted_t<const term_t> body;

    var_captures_t external_captures;
};

}  // namespace ql

#endif // RDB_PROTOCOL_FUNC_HPP_
