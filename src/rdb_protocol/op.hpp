// Copyright 2010-2015 RethinkDB, all rights reserved.
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

enum class single_server_t { no, yes };
enum class constant_now_t { no, yes };


class deterministic_t {
public:
    // Is non-deterministic.
    static deterministic_t no() { return deterministic_t(1); }

    // Is non-deterministic if run across the cluster (different cpus, compilers,
    // libc's), but is deterministic on a single server.  Example: geo operations.
    static deterministic_t single_server() { return deterministic_t(2); }

    // Is non-deterministic if r.now is non-constant.
    static deterministic_t constant_now() { return deterministic_t(4); }

    // Is always deterministic.
    static deterministic_t always() { return deterministic_t(0); }

    // Computes the combined deterministic-ness of two expressions.
    deterministic_t join(deterministic_t other) const {
        return deterministic_t(bitset | other.bitset);
    }

    // Params tell the situation:
    //  - ss: are we running the term on a single server?
    //  - cn: is r.now() constant?
    // Returns true if the expression is deterministic (under the given conditions).
    // ("The expression" is whatever expression this deterministic_t value was
    // computed from.)
    bool test(single_server_t ss, constant_now_t cn) const {
        // Turn off the bits that don't apply.
        int mask = (ss == single_server_t::yes ? single_server().bitset : 0)
            | (cn == constant_now_t::yes ? constant_now().bitset : 0);
        int remaining_bits = bitset & ~mask;
        return remaining_bits == 0;
    }

    // True if r.now() is used in the query.
    bool uses_r_now() const {
        return 0 != (bitset & constant_now().bitset);
    }


private:
    explicit deterministic_t(int bits) : bitset(bits) { }

    int bitset;
};

// Specifies the range of normal arguments a function can take (arguments
// provided by `r.args` count toward the total).  You may also optionally
// provide `eval_flags` that should be used when pre-evaluating arguments (which
// `r.args` has to do).
class argspec_t {
public:
    explicit argspec_t(int n);
    argspec_t(int _min, int _max);
    argspec_t(int _min, int _max, eval_flags_t eval_flags);
    std::string print() const;
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

    bool contains(const std::string &key) const;

    optargspec_t with(std::initializer_list<const char *> args) const;

    std::set<std::string>::const_iterator cbegin() const { return legal_args.cbegin(); }
    std::set<std::string>::const_iterator cend() const { return legal_args.cend(); }

private:
    void init(int num_args, const char *const *args);

    std::set<std::string> legal_args;
};

class arg_terms_t;

class op_term_t;

class argvec_t {
public:
    explicit argvec_t(std::vector<counted_t<const runtime_term_t> > &&v);
    // Retrieves the arg.  The arg is removed (leaving an empty pointer in its
    // slot), forcing you to call this function exactly once per argument.
    MUST_USE counted_t<const runtime_term_t> remove(size_t i);
    deterministic_t is_deterministic(size_t i) const;
    size_t size() const { return vec.size(); }
    bool empty() const { return vec.empty(); }
private:
    std::vector<counted_t<const runtime_term_t> > vec;
};

class args_t {
public:
    // number of arguments
    size_t num_args() const;
    // Returns argument `i`.
    scoped_ptr_t<val_t> arg(scope_env_t *env, size_t i, eval_flags_t flags = NO_FLAGS);
    deterministic_t arg_is_deterministic(size_t i) const;
    // Tries to get an optional argument, returns `scoped_ptr_t<val_t>()` if not found.
    scoped_ptr_t<val_t> optarg(scope_env_t *env, const std::string &key) const;

    args_t(const op_term_t *op_term, argvec_t argv);
    args_t(const op_term_t *op_term, argvec_t argv, scoped_ptr_t<val_t> arg0);

private:
    const op_term_t *const op_term;

    argvec_t argv;
    // Sometimes the 0'th argument has already been evaluated, to see if we are doing
    // a grouped operation.
    scoped_ptr_t<val_t> arg0;

    DISABLE_COPYING(args_t);
};

// Calls accumulate_captures on the map entries.
void accumulate_all_captures(
        const std::map<std::string, counted_t<const term_t> > &optargs,
        var_captures_t *captures);

// Almost all terms will inherit from this and use its member functions to
// access their arguments.
class op_term_t : public term_t {
protected:
    op_term_t(compile_env_t *env, const raw_term_t &term,
              argspec_t argspec, optargspec_t optargspec = optargspec_t({}));
    virtual ~op_term_t();

    // This returns an optarg which is:
    // * lazy -- it's wrapped in a function, so you don't get the value until
    //   you call that function.
    // * literal -- it checks whether this operation has the literal key you
    //   provided and doesn't look anywhere else for optargs (in particular, it
    //   doesn't check global optargs).
    counted_t<const func_term_t> lazy_literal_optarg(
            compile_env_t *env, const std::string &key) const;

    // Provides a default implementation, passing off a call to arg terms and optarg
    // terms.  implicit_var_term_t overrides this.  (var_term_t does too, but it's not
    // a subclass).
    virtual void accumulate_captures(var_captures_t *captures) const;

    const std::vector<counted_t<const term_t> > &get_original_args() const;

    virtual deterministic_t is_deterministic() const;

    bool recursive_is_simple_selector() const {
        for (const auto &term : get_original_args()) {
            if (!term->is_simple_selector()) {
                return false;
            }
        }
        for (const auto &optarg_pair : optargs) {
            if (!optarg_pair.second->is_simple_selector()) {
                return false;
            }
        }
        return true;
    }

private:
    friend class args_t;
    // Union term is a friend so we can steal arguments from an array.
    friend class union_term_t;
    // Tries to get an optional argument, returns `scoped_ptr_t<val_t>()` if not found.
    scoped_ptr_t<val_t> optarg(scope_env_t *env, const std::string &key) const;

    // Evaluates args[0] and sets *grouped_data_out to non-nil if it's grouped.
    // (Sets *arg0_out to non-nil if it's not grouped.  Both outputs are set to nil
    // if args is empty.)
    void maybe_grouped_data(scope_env_t *env,
                            argvec_t *argv,
                            eval_flags_t flags,
                            counted_t<grouped_data_t> *grouped_data_out,
                            scoped_ptr_t<val_t> *arg0_out) const;

    virtual scoped_ptr_t<val_t> term_eval(scope_env_t *env,
                                          eval_flags_t eval_flags) const;
    virtual scoped_ptr_t<val_t> eval_impl(scope_env_t *env,
                                          args_t *args,
                                          eval_flags_t eval_flags) const = 0;
    virtual bool can_be_grouped() const;
    virtual bool is_grouped_seq_op() const;

    scoped_ptr_t<const arg_terms_t> arg_terms;

    std::map<std::string, counted_t<const term_t> > optargs;
};

class grouped_seq_op_term_t : public op_term_t {
public:
    template<class... Args>
    explicit grouped_seq_op_term_t(Args... args) :
        op_term_t(std::forward<Args>(args)...) { }
private:
    virtual bool is_grouped_seq_op() const { return true; }
};

class bounded_op_term_t : public op_term_t {
public:
    bounded_op_term_t(compile_env_t *env, const raw_term_t &term,
                      argspec_t argspec, optargspec_t optargspec = optargspec_t({}));

    virtual ~bounded_op_term_t() { }

protected:
    bool is_left_open(scope_env_t *env, args_t *args) const;
    bool is_right_open(scope_env_t *env, args_t *args) const;

private:
    bool open_bool(scope_env_t *env, args_t *args, const std::string &key,
                   bool def/*ault*/) const;
};

}  // namespace ql

#endif // RDB_PROTOCOL_OP_HPP_
