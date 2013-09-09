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
    std::string print();
    bool contains(int n) const;
private:
    int min, max; // max may be -1 for unbounded
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

// Almost all terms will inherit from this and use its member functions to
// access their arguments.
class op_term_t : public term_t {
public:
    op_term_t(visibility_env_t *env, protob_t<const Term> term,
              argspec_t argspec, optargspec_t optargspec = optargspec_t({}));
    virtual ~op_term_t();
protected:
    size_t num_args() const; // number of arguments
    counted_t<val_t> arg(scope_env_t *env, size_t i, eval_flags_t flags = NO_FLAGS); // returns argument `i`
    // Tries to get an optional argument, returns `counted_t<val_t>()` if not
    // found.
    counted_t<val_t> optarg(scope_env_t *env, const std::string &key);
    // This reeturns an optarg which is:
    // * lazy -- it's wrapped in a function, so you don't get the value until
    //   you call that function.
    // * literal -- it checks whether this operation has the literal key you
    //   provided and doesn't look anywhere else for optargs (in particular, it
    //   doesn't check global optargs).
    counted_t<func_term_t> lazy_literal_optarg(visibility_env_t *env, const std::string &key);

    // Provides a default implementation, passing off a call to arg terms and optarg
    // terms.  implicit_var_term_t overrides this.  (var_term_t does too, but it's not
    // a subclass).  RSI: Maybe implicit_var_term_t could be a non-subclass.
    virtual void accumulate_captures(var_captures_t *captures) const;

private:
    // RSI: Maybe we should cache whether or not this is deterministic, we probably redundantly compute this.
    virtual bool is_deterministic_impl() const;
    std::vector<counted_t<term_t> > args;

    friend class make_obj_term_t; // needs special access to optargs
    std::map<std::string, counted_t<term_t> > optargs;
};

class bounded_op_term_t : public op_term_t {
public:
    bounded_op_term_t(visibility_env_t *env, protob_t<const Term> term,
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
