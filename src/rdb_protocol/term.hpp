#ifndef RDB_PROTOCOL_TERM_HPP_
#define RDB_PROTOCOL_TERM_HPP_
#include <string>

#include "utils.hpp"

#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/ptr_container/ptr_map.hpp>

#include "containers/ptr_bag.hpp"
#include "containers/scoped.hpp"
#include "containers/uuid.hpp"
#include "rdb_protocol/error.hpp"

#include "rdb_protocol/datum.hpp"
#include "rdb_protocol/ql2.pb.h"

namespace ql {

class datum_stream_t;
class db_t;
class env_t;
class func_t;
class table_t;
class val_t;

class term_t : public ptr_baggable_t, public pb_rcheckable_t {
public:
    explicit term_t(env_t *_env, const Term *_src);
    virtual ~term_t();

    virtual const char *name() const = 0;
    val_t *eval();

    // TODO: this templating/shadowing logic is terrible (I hate implicit
    // conversions) and doesn't save that much typing.  Rip it out.

    // Allocates a new value in the current environment.
    val_t *new_val(datum_t *d); // shadow vvv
    val_t *new_val(const datum_t *d);
    val_t *new_val(datum_t *d, table_t *t); // shadow vvv
    val_t *new_val(const datum_t *d, table_t *t);
    val_t *new_val(datum_stream_t *s);
    val_t *new_val(datum_stream_t *s, table_t *t);
    val_t *new_val(db_t *db); // shadow vvv
    val_t *new_val(const db_t *db);
    val_t *new_val(table_t *t);
    val_t *new_val(func_t *f);
    val_t *new_val_bool(bool b);

    template<class T>
    val_t *new_val(T *t) { return new_val(static_cast<datum_stream_t *>(t)); }
    template<class T>
    val_t *new_val(T t) { return new_val(new datum_t(t)); }

    virtual bool is_deterministic() const;

    const Term *get_src() const;
    env_t *val_t_get_env() const { return env; } // Only `val_t` should call this.
protected:
    env_t *env;

private:
    virtual val_t *eval_impl() = 0;
    virtual bool is_deterministic_impl() const = 0;

    const Term *src;
};

term_t *compile_term(env_t *env, const Term *t);

} // namespace ql

#endif // RDB_PROTOCOL_TERM_HPP_
