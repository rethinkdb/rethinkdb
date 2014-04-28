// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_OP_HPP_
#define RDB_PROTOCOL_OP_HPP_

#include <algorithm>
#include <initializer_list>
#include <map>
#include <string>
#include <set>
#include <vector>

#include "rdb_protocol/env.hpp"
#include "rdb_protocol/error.hpp"
#include "rdb_protocol/term.hpp"
#include "rdb_protocol/val.hpp"

namespace ql {

class func_term_t;

// Specifies the range of normal arguments a function can take.
class argspec_t {
public:
    explicit argspec_t(int n);
    argspec_t(int _min, int _max);
    argspec_t(int _min, int _max, eval_flags_t eval_flags);
    std::string print();
    bool contains(int n) const;
    eval_flags_t get_eval_flags() const { return eval_flags; }
private:
    int min, max; // max may be -1 for unbounded
    eval_flags_t eval_flags;
};

// Specifies the optional arguments a function can take.
struct optargspec_t {
public:
    explicit optargspec_t(std::initializer_list<const char *> args);

    static optargspec_t make_object();
    bool is_make_object() const;

    bool contains(const std::string &key) const;

    optargspec_t with(std::initializer_list<const char *> args) const;

private:
    void init(int num_args, const char *const *args);
    explicit optargspec_t(bool _is_make_object_val);
    bool is_make_object_val;

    std::set<std::string> legal_args;
};

class args_t;

// Almost all terms will inherit from this and use its member functions to
// access their arguments.
class op_term_t : public term_t {
protected:
    op_term_t(compile_env_t *env, protob_t<const Term> term,
              argspec_t argspec, optargspec_t optargspec = optargspec_t({}));
    virtual ~op_term_t();

    size_t num_args() const; // number of arguments
    // Returns argument `i`.
    counted_t<val_t> arg(scope_env_t *env, size_t i, eval_flags_t flags = NO_FLAGS);
    // Tries to get an optional argument, returns `counted_t<val_t>()` if not found.
    counted_t<val_t> optarg(scope_env_t *env, const std::string &key);
    // This returns an optarg which is:
    // * lazy -- it's wrapped in a function, so you don't get the value until
    //   you call that function.
    // * literal -- it checks whether this operation has the literal key you
    //   provided and doesn't look anywhere else for optargs (in particular, it
    //   doesn't check global optargs).
    counted_t<func_term_t> lazy_literal_optarg(
        compile_env_t *env, const std::string &key);

    // Provides a default implementation, passing off a call to arg terms and optarg
    // terms.  implicit_var_term_t overrides this.  (var_term_t does too, but it's not
    // a subclass).
    virtual void accumulate_captures(var_captures_t *captures) const;

private:
    // TODO: this interface is a terrible hack.  `term_eval` should be named
    // `eval_impl`, `eval_impl` should be named `op_eval`, and `op_eval` should
    // take `arg0` as one of its arguments.  (Actually, the `arg` function
    // should be passed down too so that the `arg_verifier` it shares with
    // `term_eval` doesn't have to be on this object.)
    virtual counted_t<val_t> term_eval(scope_env_t *env, eval_flags_t eval_flags);
    virtual counted_t<val_t> eval_impl(scope_env_t *env, eval_flags_t eval_flags) = 0;
    virtual bool can_be_grouped();
    virtual bool is_grouped_seq_op();

    virtual bool is_deterministic() const;

    counted_t<term_t> consume(size_t i);

    scoped_ptr_t<args_t> args;

    friend class make_obj_term_t; // needs special access to optargs
    std::map<std::string, counted_t<term_t> > optargs;
};

class grouped_seq_op_term_t : public op_term_t {
public:
    template<class... Args>
    explicit grouped_seq_op_term_t(Args... args) :
        op_term_t(std::forward<Args>(args)...) { }
private:
    virtual bool is_grouped_seq_op() { return true; }
};

class bounded_op_term_t : public op_term_t {
public:
    bounded_op_term_t(compile_env_t *env, protob_t<const Term> term,
                      argspec_t argspec, optargspec_t optargspec = optargspec_t({}));

    virtual ~bounded_op_term_t() { }

protected:
    bool left_open(scope_env_t *env);
    bool right_open(scope_env_t *env);

private:
    bool open_bool(scope_env_t *env, const std::string &key, bool def/*ault*/);
};

}  // namespace ql

#endif // RDB_PROTOCOL_OP_HPP_
