#include "rdb_protocol/term.hpp"
#include "rdb_protocol/val.hpp"
#ifndef RDB_PROTOCOL_OP_HPP
#define RDB_PROTOCOL_OP_HPP
namespace ql {

class op_term_t : public term_t {
public:
    op_term_t(env_t *env, const Term2 *term);
    virtual ~op_term_t();
    size_t num_args() const;
    term_t *arg(size_t i);

    void check_no_optargs() const;
    void check_only_optargs(int n_keys, const char **keys) const;
    term_t *optarg(const std::string &key, term_t *def/*ault*/);
private:
    friend class make_obj_term_t; // need special access to optargs
    boost::ptr_vector<term_t> args;
    boost::ptr_map<const std::string, term_t> optargs;
};

class simple_op_term_t : public op_term_t {
public:
    simple_op_term_t(env_t *env, const Term2 *term);
    virtual ~simple_op_term_t();
    virtual val_t *eval_impl();
private:
    virtual val_t *simple_call_impl(std::vector<val_t *> *args) = 0;
};

#define RDB_NAME(str) virtual const char *name() const { return str; }

}
#endif // RDB_PROTOCOL_OP_HPP
