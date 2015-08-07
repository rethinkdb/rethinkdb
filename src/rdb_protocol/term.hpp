// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_TERM_HPP_
#define RDB_PROTOCOL_TERM_HPP_

#include "containers/counted.hpp"
#include "rdb_protocol/datum.hpp"
#include "rdb_protocol/error.hpp"
#include "rdb_protocol/ql2.pb.h"
#include "rdb_protocol/val.hpp"

namespace ql {

class datum_stream_t;
class datum_t;
class db_t;
class env_t;
class func_t;
class scope_env_t;
class table_t;
class table_slice_t;
class var_captures_t;
class compile_env_t;

enum eval_flags_t {
    NO_FLAGS = 0,
    LITERAL_OK = 1,
};

class runtime_term_t : public slow_atomic_countable_t<runtime_term_t>,
                       public bt_rcheckable_t {
public:
    virtual ~runtime_term_t();
    scoped_ptr_t<val_t> eval(scope_env_t *env, eval_flags_t eval_flags = NO_FLAGS) const;
    virtual bool is_deterministic() const = 0;
    virtual const char *name() const = 0;

    // Allocates a new value in the current environment.
    template<class... Args>
    scoped_ptr_t<val_t> new_val(Args... args) const {
        return make_scoped<val_t>(std::forward<Args>(args)..., backtrace());
    }
    scoped_ptr_t<val_t> new_val_bool(bool b) const {
        return new_val(datum_t::boolean(b));
    }

protected:
    explicit runtime_term_t(backtrace_id_t bt);
private:
    virtual scoped_ptr_t<val_t> term_eval(scope_env_t *env, eval_flags_t) const = 0;
};

class term_t : public runtime_term_t {
public:
    explicit term_t(protob_t<const Term> _src);
    virtual ~term_t();
    protob_t<const Term> get_src() const;
    virtual void accumulate_captures(var_captures_t *captures) const = 0;
private:
    protob_t<const Term> src;
    DISABLE_COPYING(term_t);
};

counted_t<const term_t> compile_term(compile_env_t *env, const protob_t<const Term> t);

} // namespace ql

#endif // RDB_PROTOCOL_TERM_HPP_
