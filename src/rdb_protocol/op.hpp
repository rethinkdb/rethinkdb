#ifndef RDB_PROTOCOL_OP_HPP_
#define RDB_PROTOCOL_OP_HPP_

#include <algorithm>
#include <string>

#include "rdb_protocol/env.hpp"
#include "rdb_protocol/error.hpp"
#include "rdb_protocol/term.hpp"
#include "rdb_protocol/val.hpp"

namespace ql {

// Specifies the range of normal arguments a function can take.
struct argspec_t {
    explicit argspec_t(int n);
    argspec_t(int _min, int _max);
    std::string print();
    bool contains(int n) const;
    int min, max; // max may be -1 for unbounded
};

// Specifies the optional arguments a function can take.
struct optargspec_t {
    optargspec_t(int n, const char *const *c) : num_legal_args(n), legal_args(c) { }
    template<int n>
    optargspec_t(const char *const (&arr)[n]) : num_legal_args(n), legal_args(arr) { }

    static optargspec_t make_object();
    bool is_make_object() const;

    bool contains(const std::string &key) const;
    int num_legal_args;
    const char *const *legal_args;
};

// Almost all terms will inherit from this and use its member functions to
// access their arguments.
class op_term_t : public term_t {
public:
    op_term_t(env_t *env, const Term *term,
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

}  // namespace ql

#endif // RDB_PROTOCOL_OP_HPP_
