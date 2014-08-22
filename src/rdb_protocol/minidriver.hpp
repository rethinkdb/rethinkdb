#ifndef RDB_PROTOCOL_MINIDRIVER_HPP_
#define RDB_PROTOCOL_MINIDRIVER_HPP_

#include <string>
#include <vector>
#include <utility>
#include <algorithm>

#include "rdb_protocol/env.hpp"
#include "rdb_protocol/ql2.pb.h"
#include "rdb_protocol/datum.hpp"
#include "rdb_protocol/counted_term.hpp"
#include "rdb_protocol/sym.hpp"
#include "rdb_protocol/pb_utils.hpp"

/** RVALUE_THIS
 *
 * This macro is used to annotate methods that treat *this as an
 * rvalue reference. On compilers that support it, it expands to &&
 * and all uses of the method on non-rvlaue *this are reported as
 * errors.
 *
 * The supported compilers are clang >= 2.9 and gcc >= 4.8.1
 *
 **/
#if defined(__clang__)
#if __has_extension(cxx_rvalue_references)
#define RVALUE_THIS &&
#else
#define RVALUE_THIS
#endif
#elif __GNUC__ > 4 || (__GNUC__ == 4 && \
    (__GNUC_MINOR__ > 8 || (__GNUC_MINOR__ == 8 && \
                            __GNUC_PATCHLEVEL__ > 1)))
#define RVALUE_THIS &&
#else
#define RVALUE_THIS
#endif


namespace ql {

namespace r {

/** reql_t
 *
 * A thin wrapper around scoped_ptr_t<Term> that allows building Terms
 * using the ReQL syntax.
 *
 * Move semantics are used for reql_t to avoid copying the inner Term.
 *
 * Example:
 *
 * This is an error:
 *
 *  reql_t a = expr(1);
 *  reql_t b = a + 1;
 *  reql_t c = array(1,2,3).nth(a);
 *
 * Instead use:
 *
 *  reql_t b = a.copy() + 1;
 *  reql_t c = array(1,2,3).nth(std::move(a));
 *  // ~a()
 *
 **/

class reql_t {
public:

    explicit reql_t(scoped_ptr_t<Term> &&term_);
    explicit reql_t(const double val);
    explicit reql_t(const std::string &val);
    explicit reql_t(const datum_t &d);
    explicit reql_t(const counted_t<const datum_t> &d);
    explicit reql_t(const Datum &d);
    explicit reql_t(const Term &t);
    explicit reql_t(std::vector<reql_t> &&val);
    explicit reql_t(pb::dummy_var_t var);

    reql_t(reql_t &&other);
    reql_t &operator= (reql_t &&other);

    reql_t copy() const;

    template <class... T>
    reql_t(Term_TermType type, T &&... args) : term(make_scoped<Term>()) {
        term->set_type(type);
        add_args(std::forward<T>(args)...);
    }

    template <class... T>
    reql_t call(Term_TermType type, T &&... args) RVALUE_THIS {
        return reql_t(type, std::move(*this), std::forward<T>(args)...);
    }

    Term* release();

    Term &get();

    const Term &get() const;

    protob_t<Term> release_counted();

    void swap(Term &t);

    void copy_optargs_from_term(const Term &from);
    void copy_args_from_term(const Term &from, size_t start_index = 0);

#define REQL_METHOD(name, termtype)                                     \
    template<class... T>                                                \
    reql_t name(T &&... a) RVALUE_THIS                                  \
    { return std::move(*this).call(Term::termtype, std::forward<T>(a)...); }

    REQL_METHOD(operator +, ADD)
    REQL_METHOD(operator /, DIV)
    REQL_METHOD(operator ==, EQ)
    REQL_METHOD(operator (), FUNCALL)
    REQL_METHOD(operator >, GT)
    REQL_METHOD(operator <, LT)
    REQL_METHOD(operator >=, GE)
    REQL_METHOD(operator <=, LE)
    REQL_METHOD(operator &&, ALL)
    REQL_METHOD(count, COUNT)
    REQL_METHOD(map, MAP)
    REQL_METHOD(concat_map, CONCATMAP)
    REQL_METHOD(operator [], GET_FIELD)
    REQL_METHOD(nth, NTH)
    REQL_METHOD(bracket, BRACKET)
    REQL_METHOD(pluck, PLUCK)
    REQL_METHOD(has_fields, HAS_FIELDS)
    REQL_METHOD(coerce_to, COERCE_TO)
    REQL_METHOD(get_all, GET_ALL)
    REQL_METHOD(replace, REPLACE)
    REQL_METHOD(slice, SLICE)
    REQL_METHOD(filter, FILTER)
    REQL_METHOD(contains, CONTAINS)
    REQL_METHOD(merge, MERGE)
    REQL_METHOD(default_, DEFAULT)

    reql_t operator !() RVALUE_THIS;
    reql_t do_(pb::dummy_var_t arg, reql_t &&body) RVALUE_THIS;

    template <class... T>
    void add_args(T &&... args) {
        UNUSED int _[] = { (add_arg(std::forward<T>(args)), 1)... };
    }

    template<class T>
    void add_arg(T &&a) {
        reql_t it(std::forward<T>(a));
        term->mutable_args()->AddAllocated(it.term.release());
    }

private:

    void set_datum(const datum_t &d);

    scoped_ptr_t<Term> term;
};

template <>
inline void reql_t::add_arg(std::pair<std::string, reql_t> &&kv) {
    auto ap = make_scoped<Term_AssocPair>();
    ap->set_key(kv.first);
    ap->mutable_val()->Swap(kv.second.term.get());
    term->mutable_optargs()->AddAllocated(ap.release());
}

template <class T>
reql_t expr(T &&d) {
    return reql_t(std::forward<T>(d));
}

reql_t boolean(bool b);

reql_t fun(reql_t &&body);
reql_t fun(pb::dummy_var_t a, reql_t &&body);
reql_t fun(pb::dummy_var_t a, pb::dummy_var_t b, reql_t &&body);

template<class... Ts>
reql_t array(Ts &&... xs) {
    return reql_t(Term::MAKE_ARRAY, std::forward<Ts>(xs)...);
}

template<class... Ts>
reql_t object(Ts &&... xs) {
    return reql_t(Term::MAKE_OBJ, std::forward<Ts>(xs)...);
}

reql_t null();

template <class T>
std::pair<std::string, reql_t> optarg(const std::string &key, T &&value) {
    return std::pair<std::string, reql_t>(key, reql_t(std::forward<T>(value)));
}

reql_t db(const std::string &name);

template <class T>
reql_t error(T &&message) {
    return reql_t(Term::ERROR, std::forward<T>(message));
}

template<class Cond, class Then, class Else>
reql_t branch(Cond &&a, Then &&b, Else &&c) {
    return reql_t(Term::BRANCH,
                  std::forward<Cond>(a),
                  std::forward<Then>(b),
                  std::forward<Else>(c));
}

reql_t var(pb::dummy_var_t var);
reql_t var(const sym_t &var);

} // namepsace r

} // namespace ql

#endif // RDB_PROTOCOL_MINIDRIVER_HPP_
