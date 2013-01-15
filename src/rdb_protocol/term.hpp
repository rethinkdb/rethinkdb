#ifndef RDB_PROTOCOL_TERM_HPP_
#define RDB_PROTOCOL_TERM_HPP_
#include <string>

#include "utils.hpp"

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
class term_t : public ptr_baggable_t {
public:
    term_t(env_t *_env);
    virtual ~term_t();

    virtual const char *name() const = 0;
    val_t *eval(bool _use_cached_val);

    val_t *new_val(datum_t *d); // shadow vvv
    val_t *new_val(const datum_t *d);
    val_t *new_val(datum_t *d, table_t *t); // shadow vvv
    val_t *new_val(const datum_t *d, table_t *t);
    val_t *new_val(datum_stream_t *s);
    val_t *new_val(uuid_t db);
    val_t *new_val(table_t *t);
    val_t *new_val(func_t *f);
    val_t *new_val_bool(bool b) { return new_val(new datum_t(b, b)); }

    val_t *new_val() { return new_val(new datum_t()); }
    template<class T>
    val_t *new_val(T *t) { return new_val(static_cast<datum_stream_t *>(t)); }
    template<class T>
    val_t *new_val(T t) { return new_val(new datum_t(t)); }
    template<class T>
    void set_bt(T t) { frame.init(new backtrace_t::frame_t(t)); }
    bool has_bt() { return frame.has(); }
    backtrace_t::frame_t get_bt() { return *frame.get(); }
protected:
    bool use_cached_val;
    env_t *env;
private:
    virtual val_t *eval_impl() = 0;
    val_t *cached_val;

    scoped_ptr_t<backtrace_t::frame_t> frame;
};
term_t *compile_term(env_t *env, const Term2 *t);

}

#endif // RDB_PROTOCOL_TERM_HPP_
