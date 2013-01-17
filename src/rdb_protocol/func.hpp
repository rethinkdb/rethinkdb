#ifndef RDB_PROTOCOL_FUNC_HPP_
#define RDB_PROTOCOL_FUNC_HPP_

#include <vector>

#include "utils.hpp"

#include "containers/ptr_bag.hpp"
#include "containers/scoped.hpp"
#include "rdb_protocol/term.hpp"
#include "rpc/serialize_macros.hpp"
#include "protob/protob.hpp"

namespace ql {

//TODO: make datum_t pointers const
class func_t : public ptr_baggable_t {
public:
    func_t(env_t *env, const Term2 *_source);
    val_t *_call(const std::vector<const datum_t *> &args);
    val_t *call(const datum_t *arg);
    val_t *call(const datum_t *arg1, const datum_t *arg2);
private:
    std::vector<const datum_t *> argptrs;
    term_t *body;

    friend class wire_func_t;
    const Term2 *source;
    bool implicit_bound;
};


RDB_MAKE_PROTOB_SERIALIZABLE(Term2);
RDB_MAKE_PROTOB_SERIALIZABLE(Datum);
class wire_func_t {
public:
    wire_func_t();
    virtual ~wire_func_t() { }
    wire_func_t(env_t *env, func_t *_func);
    func_t *compile(env_t *env);
    virtual backtrace_t::frame_t bt() = 0;
protected:
    std::map<env_t *, func_t *> cached_funcs;

    Term2 source;
    std::map<int, Datum> scope;
public:
    //RDB_MAKE_ME_SERIALIZABLE_2(source, scope);
};

#define SIMPLE_FUNC_IMPL(name, frame_num)                                     \
static const int name##_bt_frame = frame_num;                                 \
class name##_wire_func_t : public wire_func_t {                               \
public:                                                                       \
    name##_wire_func_t() : wire_func_t() { }                                  \
    name##_wire_func_t(env_t *env, func_t *func) : wire_func_t(env, func) { } \
    virtual backtrace_t::frame_t bt() { return name##_bt_frame; }             \
    RDB_MAKE_ME_SERIALIZABLE_2(source, scope);                                \
};                                                                            \

SIMPLE_FUNC_IMPL(map, 1);
SIMPLE_FUNC_IMPL(filter, 1);
SIMPLE_FUNC_IMPL(reduce, 1);
SIMPLE_FUNC_IMPL(concatmap, 1);
// Faux functions
class count_wire_func_t { RDB_MAKE_ME_SERIALIZABLE_0() };

// Grouped Map Reduce
static const int gmr_group_bt_frame = 1;
static const int gmr_map_bt_frame = 2;
static const int gmr_reduce_bt_frame = 3;
class gmr_wire_func_t {
public:
    gmr_wire_func_t() { }
    gmr_wire_func_t(env_t *env, func_t *_group, func_t *_map, func_t *_reduce)
        : group(env, _group), map(env, _map), reduce(env, _reduce) { }
    func_t *compile_group(env_t *env) { return group.compile(env); }
    func_t *compile_map(env_t *env) { return map.compile(env); }
    func_t *compile_reduce(env_t *env) { return reduce.compile(env); }
private:
    map_wire_func_t group;
    map_wire_func_t map;
    reduce_wire_func_t reduce;
    RDB_MAKE_ME_SERIALIZABLE_3(group, map, reduce);
};

class func_term_t : public term_t {
public:
    func_term_t(env_t *env, const Term2 *term);
private:
    virtual val_t *eval_impl();
    virtual const char *name() const { return "func"; }
    func_t *func;
};

} // namespace ql
#endif // RDB_PROTOCOL_FUNC_HPP_
