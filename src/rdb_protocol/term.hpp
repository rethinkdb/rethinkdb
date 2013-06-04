#ifndef RDB_PROTOCOL_TERM_HPP_
#define RDB_PROTOCOL_TERM_HPP_

#include <string>

#include "errors.hpp"
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/ptr_container/ptr_map.hpp>

#include "containers/counted.hpp"
#include "containers/uuid.hpp"
#include "rdb_protocol/error.hpp"
#include "rdb_protocol/ql2.pb.h"

namespace ql {

class datum_stream_t;
class datum_t;
class db_t;
class env_t;
class func_t;
class table_t;
class val_t;

class term_t : public slow_atomic_countable_t<term_t>, public pb_rcheckable_t {
public:
    explicit term_t(env_t *_env, protob_t<const Term> _src);
    virtual ~term_t();

    virtual const char *name() const = 0;
    counted_t<val_t> eval();

    // Allocates a new value in the current environment.
    counted_t<val_t> new_val(counted_t<const datum_t> d);
    counted_t<val_t> new_val(counted_t<const datum_t> d, counted_t<table_t> t);
    counted_t<val_t> new_val(counted_t<datum_stream_t> s);
    counted_t<val_t> new_val(counted_t<datum_stream_t> s, counted_t<table_t> t);
    counted_t<val_t> new_val(counted_t<const db_t> db);
    counted_t<val_t> new_val(counted_t<table_t> t);
    counted_t<val_t> new_val(counted_t<func_t> f);
    counted_t<val_t> new_val_bool(bool b);

    bool is_deterministic() const;

    protob_t<const Term> get_src() const;
    void prop_bt(Term *t) const;
    env_t *val_t_get_env() const { return env; } // Only `val_t` should call this.

protected:
    env_t *env;

private:
    virtual counted_t<val_t> eval_impl() = 0;
    virtual bool is_deterministic_impl() const = 0;
    protob_t<const Term> src;
};

counted_t<term_t> compile_term(env_t *env, protob_t<const Term> t);

} // namespace ql

#endif // RDB_PROTOCOL_TERM_HPP_
