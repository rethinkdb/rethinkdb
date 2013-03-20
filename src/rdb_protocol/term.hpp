#ifndef RDB_PROTOCOL_TERM_HPP_
#define RDB_PROTOCOL_TERM_HPP_

#include <string>

#include "errors.hpp"
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/ptr_container/ptr_map.hpp>

#include "containers/ptr_bag.hpp"
#include "containers/scoped.hpp"
#include "containers/uuid.hpp"
#include "rdb_protocol/err.hpp"
#include "rdb_protocol/datum.hpp"
#include "rdb_protocol/ql2.pb.h"

namespace ql {
class env_t;
class datum_stream_t;
class func_t;
class val_t;
class table_t;
class term_t : public ptr_baggable_t, public pb_rcheckable_t {
public:
    explicit term_t(env_t *_env, const Term *_src);
    virtual ~term_t();

    virtual const char *name() const = 0;
    val_t *eval(bool _use_cached_val);

    // TODO: this templating/shadowing logic is terrible (I hate implicit
    // conversions) and doesn't save that much typing.  Rip it out.

    // Allocates a new value in the current environment.
    val_t *new_val(datum_t *d); // shadow vvv
    val_t *new_val(const datum_t *d);
    val_t *new_val(datum_t *d, table_t *t); // shadow vvv
    val_t *new_val(const datum_t *d, table_t *t);
    val_t *new_val(scoped_ptr_t<datum_stream_t> &&s);
    val_t *new_val(table_t *t, datum_stream_t *s);
    val_t *new_val(uuid_u db);
    val_t *new_val(table_t *t);
    val_t *new_val(func_t *f);
    val_t *new_val_bool(bool b);

    template<class T>
    val_t *new_val(T *t) { return new_val(static_cast<datum_stream_t *>(t)); }
    template<class T>
    val_t *new_val(T t) { return new_val(new datum_t(t)); }

    virtual bool is_deterministic() const;

protected:
    // `use_cached_val` once had a reason to exist (as did the corresponding
    // argument to `eval`), but it has since disappeared (people are required
    // not to evaluate a term twice unless they want to execute it twice).  This
    // should go away in a future refactor.
    bool use_cached_val;
    env_t *env;
private:
    virtual val_t *eval_impl() = 0;
    virtual bool is_deterministic_impl() const = 0;
    val_t *cached_val;
};

term_t *compile_term(env_t *env, const Term *t);

} // namespace ql

#endif // RDB_PROTOCOL_TERM_HPP_
