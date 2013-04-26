#ifndef RDB_PROTOCOL_TERM_HPP_
#define RDB_PROTOCOL_TERM_HPP_

#include <string>

#include "errors.hpp"
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/ptr_container/ptr_map.hpp>

#include "containers/counted.hpp"
#include "containers/uuid.hpp"
#include "rdb_protocol/datum.hpp"
#include "rdb_protocol/error.hpp"
#include "rdb_protocol/ql2.pb.h"

namespace ql {
class env_t;
class datum_stream_t;
class func_t;
class val_t;
class table_t;
class term_t : public single_threaded_shared_mixin_t<term_t>, public pb_rcheckable_t {
public:
    explicit term_t(env_t *_env, const Term *_src);
    virtual ~term_t();

    virtual const char *name() const = 0;
    counted_t<val_t> eval();

    // Allocates a new value in the current environment.
    counted_t<val_t> new_val(counted_t<const datum_t> d);
    counted_t<val_t> new_val(counted_t<const datum_t> d, counted_t<table_t> t);
    counted_t<val_t> new_val(counted_t<datum_stream_t> s);
    counted_t<val_t> new_val(counted_t<datum_stream_t> s, counted_t<table_t> t);
    counted_t<val_t> new_val(uuid_u db);
    counted_t<val_t> new_val(counted_t<table_t> t);
    counted_t<val_t> new_val(counted_t<func_t> f);
    counted_t<val_t> new_val_bool(bool b);

    virtual bool is_deterministic() const;

    const Term *get_src() const;
    env_t *val_t_get_env() const { return env; } // Only `val_t` should call this.
protected:
    env_t *env;

private:
    virtual counted_t<val_t> eval_impl() = 0;
    virtual bool is_deterministic_impl() const = 0;
    const Term *src;
};

counted_t<term_t> compile_term(env_t *env, const Term *t);

} // namespace ql

#endif // RDB_PROTOCOL_TERM_HPP_
