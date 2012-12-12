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
    term_t *get_arg(size_t i);
    void check_no_optargs() const;
private:
    boost::ptr_vector<term_t> args;
    boost::ptr_map<const std::string, term_t> optargs;
    //std::vector<term_t *> args;
    //std::map<const std::string, term_t *> optargs;
};

class simple_op_term_t : public op_term_t {
public:
    simple_op_term_t(env_t *env, const Term2 *term);
    virtual ~simple_op_term_t();
    virtual val_t *eval_impl();
private:
    virtual val_t *simple_call_impl(std::vector<val_t *> *args) = 0;
};

}
#endif // RDB_PROTOCOL_OP_HPP
