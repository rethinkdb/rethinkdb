// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_TERMS_OBJ_OR_SEQ_HPP_
#define RDB_PROTOCOL_TERMS_OBJ_OR_SEQ_HPP_

#include <functional>

#include "containers/counted.hpp"
#include "rdb_protocol/counted_term.hpp"
#include "rdb_protocol/op.hpp"
#include "rdb_protocol/pb_utils.hpp"
#include "rdb_protocol/minidriver.hpp"
#include "utils.hpp"

namespace ql {

enum poly_type_t {
    MAP = 0,
    FILTER = 1,
    SKIP_MAP = 2
};

class obj_or_seq_op_impl_t {
public:
    obj_or_seq_op_impl_t(const term_t *self,
                         poly_type_t _poly_type,
                         protob_t<const Term> term,
                         std::set<std::string> &&_acceptable_ptypes);

    scoped_ptr_t<val_t> eval_impl_dereferenced(const term_t *target, scope_env_t *env,
                                               args_t *args,
                                               const scoped_ptr_t<val_t> &v0,
                                               std::function<scoped_ptr_t<val_t>()> helper) const;

private:
    poly_type_t poly_type;
    protob_t<Term> func;
    const term_t *parent;
    const std::set<std::string> acceptable_ptypes;

    DISABLE_COPYING(obj_or_seq_op_impl_t);
};

// This term is used for functions that are polymorphic on objects and
// sequences, like `pluck`.  It will handle the polymorphism; terms inheriting
// from it just need to implement evaluation on objects (`obj_eval`).
class obj_or_seq_op_term_t : public grouped_seq_op_term_t {
public:
    obj_or_seq_op_term_t(compile_env_t *env, protob_t<const Term> term,
                         poly_type_t _poly_type, argspec_t argspec);
    obj_or_seq_op_term_t(compile_env_t *env, protob_t<const Term> term,
                         poly_type_t _poly_type, argspec_t argspec,
                         std::set<std::string> &&ptypes);

private:
    virtual scoped_ptr_t<val_t> obj_eval(scope_env_t *env,
                                         args_t *args,
                                         const scoped_ptr_t<val_t> &v0) const = 0;

    virtual scoped_ptr_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const;

    obj_or_seq_op_impl_t impl;
};

} // namespace ql

#endif // RDB_PROTOCOL_TERMS_OBJ_OR_SEQ_HPP_
