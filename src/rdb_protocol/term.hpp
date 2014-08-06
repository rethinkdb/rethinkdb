// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_TERM_HPP_
#define RDB_PROTOCOL_TERM_HPP_

#include "containers/counted.hpp"
#include "rdb_protocol/error.hpp"
#include "rdb_protocol/ql2.pb.h"

namespace ql {

class datum_stream_t;
class datum_t;
class db_t;
class env_t;
class func_t;
class scope_env_t;
class table_t;
class val_t;
class var_captures_t;
class compile_env_t;

enum eval_flags_t {
    NO_FLAGS = 0,
    LITERAL_OK = 1,
};

class term_t : public slow_atomic_countable_t<term_t>, public pb_rcheckable_t {
public:
    explicit term_t(protob_t<const Term> _src);
    virtual ~term_t();

    virtual const char *name() const = 0;
    counted_t<val_t> eval(scope_env_t *env, eval_flags_t eval_flags = NO_FLAGS) const;

    // Allocates a new value in the current environment.
    counted_t<val_t> new_val(counted_t<const datum_t> d) const;
    counted_t<val_t> new_val(counted_t<const datum_t> d, counted_t<table_t> t) const;
    counted_t<val_t> new_val(counted_t<const datum_t> d,
                             counted_t<const datum_t> orig_key,
                             counted_t<table_t> t) const;
    counted_t<val_t> new_val(env_t *env, counted_t<datum_stream_t> s) const;
    counted_t<val_t> new_val(counted_t<datum_stream_t> s, counted_t<table_t> t) const;
    counted_t<val_t> new_val(counted_t<const db_t> db) const;
    counted_t<val_t> new_val(counted_t<table_t> t) const;
    counted_t<val_t> new_val(counted_t<func_t> f) const;
    counted_t<val_t> new_val_bool(bool b) const;

    virtual bool is_deterministic() const = 0;

    protob_t<const Term> get_src() const;
    void prop_bt(Term *t) const;

    virtual void accumulate_captures(var_captures_t *captures) const = 0;

private:
    virtual counted_t<val_t> term_eval(scope_env_t *env, eval_flags_t) const = 0;
    protob_t<const Term> src;

    DISABLE_COPYING(term_t);
};

counted_t<const term_t> compile_term(compile_env_t *env, protob_t<const Term> t);

} // namespace ql

#endif // RDB_PROTOCOL_TERM_HPP_
