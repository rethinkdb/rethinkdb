#ifndef RDB_PROTOCOL_OP_HPP_
#define RDB_PROTOCOL_OP_HPP_

#include <algorithm>
#include <string>

#include "rdb_protocol/env.hpp"
#include "rdb_protocol/err.hpp"
#include "rdb_protocol/term.hpp"
#include "rdb_protocol/val.hpp"
namespace ql {

// Specifies the range of normal arguments a function can take.
struct argspec_t {
    explicit argspec_t(int n) : min(n), max(n) { }
    argspec_t(int _min, int _max) : min(_min), max(_max) { }
    std::string print() {
        if (min == max) {
            return strprintf("%d argument(s)", min);
        } else if (max == -1) {
            return strprintf("at least %d arguments", min);
        } else {
            return strprintf("between %d and %d arguments", min, max);
        }
    }
    bool contains(int n) const { return min <= n && (max < 0 || n <= max); }
    int min, max; // max may be -1 for unbounded
};

// Specifies the optional arguments a function can take.
struct optargspec_t {
    static optargspec_t make_object() { return optargspec_t(-1, 0); }
    optargspec_t(int n, const char *const *c) : num_legal_args(n), legal_args(c) { }
    template<int n>
    optargspec_t(const char *const (&arr)[n]) : num_legal_args(n), legal_args(arr) { }
    bool contains(const std::string &key) const {
        r_sanity_check(!is_make_object());
        for (int i = 0; i < num_legal_args; ++i) if (key == legal_args[i]) return true;
        return false;
    }
    bool is_make_object() const { return num_legal_args < 0; }
    int num_legal_args;
    const char *const *legal_args;
};

// Almost all terms will inherit from this and use its member functions to
// access their arguments.
class op_term_t : public term_t {
public:
    op_term_t(env_t *env, const Term2 *term,
              argspec_t argspec, optargspec_t optargspec = optargspec_t(0, 0));
    virtual ~op_term_t();
protected:
    size_t num_args() const; // number of arguments
    val_t *arg(size_t i); // returns argument `i`
    // Tries to get an optional argument, returns `def` if not found.
    val_t *optarg(const std::string &key, val_t *def/*ault*/);
private:
    virtual bool is_deterministic_impl() const;
    boost::ptr_vector<term_t> args;

    friend class make_obj_term_t; // needs special access to optargs
    boost::ptr_map<const std::string, term_t> optargs;
};

// There was a good reason for defining both of these, but I forget it.  Maybe
// debugging?  In any case, this macro can be used to name terms that inherit
// from `op_term_t`.
#define RDB_NAME(str) \
    static const char *_name() { return str; } \
    virtual const char *name() const { return str; } \
    typedef int RDB_NAME_MACRO_WAS_NOT_TERMINATED_WITH_SEMICOLON

} // namespace ql
#endif // RDB_PROTOCOL_OP_HPP_
