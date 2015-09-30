// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_MINIDRIVER_HPP_
#define RDB_PROTOCOL_MINIDRIVER_HPP_

#include <string>
#include <vector>
#include <utility>
#include <algorithm>

#include "rdb_protocol/datum.hpp"
#include "rdb_protocol/env.hpp"
#include "rdb_protocol/ql2.pb.h"
#include "rdb_protocol/sym.hpp"
#include "rdb_protocol/term_storage.hpp"

namespace ql {

class minidriver_t {
public:
    // Dummy variables are used to identify the variables we use when constructing
    // home-made reql functions.
    //
    // You aren't perfectly safe though -- what if you put your variable inside a term
    // that expands its body into a reql function!  So, we use a bunch of different
    // dummy variable names for use in different types.
    enum class dummy_var_t {
        IGNORED,  // For functions that ignore their parameter.  There's no special
                  // meaning to this value, we just don't need to create separate names
                  // for these instances.
        VAL_UPSERT_REPLACEMENT,
        GROUPBY_MAP_OBJ,
        GROUPBY_MAP_ATTR,
        GROUPBY_REDUCE_A,
        GROUPBY_REDUCE_B,
        GROUPBY_FINAL_OBJ,
        GROUPBY_FINAL_VAL,
        INNERJOIN_N,
        INNERJOIN_M,
        OUTERJOIN_N,
        OUTERJOIN_M,
        OUTERJOIN_LST,
        EQJOIN_ROW,
        EQJOIN_V,
        UPDATE_OLDROW,
        UPDATE_NEWROW,
        DIFFERENCE_ROW,
        SINDEXCREATE_X,
        OBJORSEQ_VARNUM,
        FUNC_GETFIELD,
        FUNC_PLUCK,
        FUNC_EQCOMPARISON,
        FUNC_PAGE,
        DISTINCT_ROW,
        REPLACE_HELPER_ROW
    };

    /** reql_t
     * A class that allows building generated_term_ts using the ReQL syntax.
     **/
    class reql_t {
    public:
        raw_term_t root_term();
        void copy_optargs_from_term(const raw_term_t &from);
        void copy_args_from_term(const raw_term_t &from, size_t start_index);

#define REQL_METHOD(name, termtype)  \
    template <class... T>            \
    reql_t name(T &&... a)           \
    { return reql_t(r, Term::termtype, *this, std::forward<T>(a)...); }

        REQL_METHOD(operator +, ADD)
        REQL_METHOD(operator /, DIV)
        REQL_METHOD(operator ==, EQ)
        REQL_METHOD(operator (), FUNCALL)
        REQL_METHOD(operator >, GT)
        REQL_METHOD(operator <, LT)
        REQL_METHOD(operator >=, GE)
        REQL_METHOD(operator <=, LE)
        REQL_METHOD(operator &&, AND)
        REQL_METHOD(count, COUNT)
        REQL_METHOD(map, MAP)
        REQL_METHOD(concat_map, CONCAT_MAP)
        REQL_METHOD(operator [], GET_FIELD)
        REQL_METHOD(nth, NTH)
        REQL_METHOD(bracket, BRACKET)
        REQL_METHOD(pluck, PLUCK)
        REQL_METHOD(has_fields, HAS_FIELDS)
        REQL_METHOD(coerce_to, COERCE_TO)
        REQL_METHOD(get_, GET)
        REQL_METHOD(get_all, GET_ALL)
        REQL_METHOD(replace, REPLACE)
        REQL_METHOD(insert, INSERT)
        REQL_METHOD(delete_, DELETE)
        REQL_METHOD(slice, SLICE)
        REQL_METHOD(filter, FILTER)
        REQL_METHOD(contains, CONTAINS)
        REQL_METHOD(merge, MERGE)
        REQL_METHOD(default_, DEFAULT)
        REQL_METHOD(table, TABLE)

        reql_t operator !();
        reql_t do_(dummy_var_t arg, const reql_t &body);

        template <class... T>
        reql_t call(Term::TermType type, T &&... args) {
            return reql_t(r, type, *this, std::forward<T>(args)...);
        }

        reql_t(const reql_t &other);
        reql_t &operator= (const reql_t &other);

        template <class T>
        void add_arg(T &&a) {
            add_arg(reql_t(r, std::forward<T>(a)));
        }

    private:
        template <class... T>
        void add_args(T &&... args) {
            UNUSED int _[] = { 0, (add_arg(std::forward<T>(args)), 1)... };
        }

        friend class minidriver_t;
        reql_t(minidriver_t *_r, const raw_term_t &term_);
        reql_t(minidriver_t *_r, const reql_t &other);
        reql_t(minidriver_t *_r, const double val);
        reql_t(minidriver_t *_r, const std::string &val);
        reql_t(minidriver_t *_r, const datum_t &d);
        reql_t(minidriver_t *_r, std::vector<reql_t> &&val);
        reql_t(minidriver_t *_r, dummy_var_t var);

        template <class... T>
        reql_t(minidriver_t *_r, Term::TermType type, T &&... args) :
                r(_r) {
            r_sanity_check(type != Term::DATUM);
            term = make_counted<generated_term_t>(type, r->bt);
            add_args(std::forward<T>(args)...);
        }

        minidriver_t *r;
        counted_t<generated_term_t> term;
    };

    explicit minidriver_t(backtrace_id_t bt);

    template <class T>
    reql_t expr(T &&d) {
        return reql_t(this, std::forward<T>(d));
    }

    reql_t boolean(bool b);

    // Takes n r.var reql_ts, plus a function that takes n arguments
    reql_t fun(const reql_t &body);
    reql_t fun(dummy_var_t a, const reql_t &body);
    reql_t fun(dummy_var_t a, dummy_var_t b, const reql_t &body);

    template <class... T>
    reql_t array(T &&... args) {
        return reql_t(this, Term::MAKE_ARRAY, std::forward<T>(args)...);
    }

    template <class... T>
    reql_t object(T &&... args) {
        return reql_t(this, Term::MAKE_OBJ, std::forward<T>(args)...);
    }

    reql_t null();

    template <class T>
    std::pair<std::string, reql_t> optarg(const std::string &key, T &&value) {
        return std::pair<std::string, reql_t>(key, reql_t(this, std::forward<T>(value)));
    }

    reql_t db(const std::string &name);

    template <class T>
    reql_t error(T &&message) {
        return reql_t(Term::ERROR, std::forward<T>(message));
    }

    template <class Cond, class Then, class Else>
    reql_t branch(Cond &&a, Then &&b, Else &&c) {
        return reql_t(this, Term::BRANCH,
                      std::forward<Cond>(a),
                      std::forward<Then>(b),
                      std::forward<Else>(c));
    }

    reql_t var(dummy_var_t var);
    reql_t var(const sym_t &var);

private:
    friend class reql_t;
    backtrace_id_t bt;

    friend class distinct_term_t;
    // Returns the sym_t corresponding to a dummy_var_t.
    static sym_t dummy_var_to_sym(dummy_var_t dummy_var);
};


// This template-specialization of add_arg is actually adding an optarg
template <>
inline void minidriver_t::reql_t::add_arg(
        std::pair<std::string, minidriver_t::reql_t> &&optarg) {
    term->optargs[optarg.first] = std::move(optarg.second).term;
}

template <>
inline void minidriver_t::reql_t::add_arg(minidriver_t::reql_t &&arg) {
    term->args.push_back(arg.term);
}

}  // namespace ql

#endif  // RDB_PROTOCOL_MINIDRIVER_HPP_
