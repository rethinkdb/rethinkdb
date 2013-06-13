#ifndef RDB_PROTOCOL_FUNC_HPP_
#define RDB_PROTOCOL_FUNC_HPP_

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "utils.hpp"

#include "errors.hpp"
#include <boost/optional.hpp>

#include "containers/counted.hpp"
#include "rdb_protocol/datum.hpp"
#include "rdb_protocol/js.hpp"
#include "rdb_protocol/term.hpp"
#include "rpc/serialize_macros.hpp"

namespace ql {

class func_t : public slow_atomic_countable_t<func_t>, public pb_rcheckable_t {
public:
    func_t(env_t *env, js::id_t id, counted_t<term_t> parent);
    func_t(env_t *env, protob_t<const Term> _source);
    // Some queries, like filter, can take a shortcut object instead of a
    // function as their argument.
    static counted_t<func_t> new_identity_func(env_t *env, counted_t<const datum_t> obj,
                                               const protob_t<const Backtrace> &root);
    static counted_t<func_t> new_eq_comparison_func(env_t *env, counted_t<const datum_t> obj,
                                                    const protob_t<const Backtrace> &bt_src);

    counted_t<val_t> call(const std::vector<counted_t<const datum_t> > &args);

    // Prefer these versions of call.
    counted_t<val_t> call();
    counted_t<val_t> call(counted_t<const datum_t> arg);
    counted_t<val_t> call(counted_t<const datum_t> arg1, counted_t<const datum_t> arg2);
    bool filter_call(counted_t<const datum_t> arg);

    void dump_scope(std::map<int64_t, Datum> *out) const;
    bool is_deterministic() const;
    void assert_deterministic(const char *extra_msg) const;

    std::string print_src() const;
    void set_default_filter_val(counted_t<func_t> func);
private:
    // Pointers to this function's arguments.
    scoped_array_t<counted_t<const datum_t> > argptrs;
    counted_t<term_t> body; // body to evaluate with functions bound

    // This is what's serialized over the wire.
    friend class wire_func_t;
    protob_t<const Term> source;
    // This is set by `filter_term_t` and used by `filter_call`.
    // `filter_term_t` will set this if the user provides the `default` optarg,
    // in which case it will be used to handle the case where a non-existence
    // error is produced while filtering a stream.
    counted_t<func_t> default_filter_val;

    // TODO: make this smarter (it's sort of slow and shitty as-is)
    std::map<int64_t, counted_t<const datum_t> *> scope;

    counted_t<term_t> js_parent;
    env_t *js_env;
    js::id_t js_id;
};

class js_result_visitor_t : public boost::static_visitor<counted_t<val_t> > {
public:
    js_result_visitor_t(env_t *_env, counted_t<term_t> _parent) : env(_env), parent(_parent) { }

    // This JS evaluation resulted in an error
    counted_t<val_t> operator()(const std::string err_val) const;

    // This JS call resulted in a JSON value
    counted_t<val_t> operator()(const boost::shared_ptr<scoped_cJSON_t> json_val) const;

    // This JS evaluation resulted in an id for a js function
    counted_t<val_t> operator()(const id_t id_val) const;

private:
    env_t *env;
    counted_t<term_t> parent;
};

// Used to serialize a function (or gmr) over the wire.
class wire_func_t {
public:
    wire_func_t();
    wire_func_t(env_t *env, counted_t<func_t> _func);
    wire_func_t(const Term &_source, const std::map<int64_t, Datum> &_scope);

    counted_t<func_t> compile(env_t *env);

    protob_t<const Backtrace> get_bt() const {
        return source.make_child(&source->GetExtension(ql2::extension::backtrace));
    }

    Term get_term() const {
        return *source;
    }

    std::string debug_str() const {
        return source->DebugString();
    }

    // They're manually implemented because source is now a protob_t<Term>.
    void rdb_serialize(write_message_t &msg) const;  // NOLINT(runtime/references)
    archive_result_t rdb_deserialize(read_stream_t *stream);

private:
    // We cache a separate function for every environment.
    std::map<uuid_u, counted_t<func_t> > cached_funcs;

    // source is never null, even when wire_func_t is default-constructed.
    protob_t<Term> source;
    boost::optional<Term> default_filter_val;
    std::map<int64_t, Datum> scope;
};

void debug_print(printf_buffer_t *buf, const wire_func_t &func);


class map_wire_func_t : public wire_func_t {
public:
    template <class... Args>
    explicit map_wire_func_t(Args... args) : wire_func_t(args...) { }
};

class filter_wire_func_t : public wire_func_t {
public:
    template <class... Args>
    explicit filter_wire_func_t(Args... args) : wire_func_t(args...) { }
};

class reduce_wire_func_t : public wire_func_t {
public:
    template <class... Args>
    explicit reduce_wire_func_t(Args... args) : wire_func_t(args...) { }
};

class concatmap_wire_func_t : public wire_func_t {
public:
    template <class... Args>
    explicit concatmap_wire_func_t(Args... args) : wire_func_t(args...) { }
};

// Count is a fake function because we don't need to send anything.
class count_wire_func_t {
public:
    RDB_MAKE_ME_SERIALIZABLE_0()
};

// Grouped Map Reduce
class gmr_wire_func_t {
public:
    gmr_wire_func_t() { }
    gmr_wire_func_t(env_t *env, counted_t<func_t> _group, counted_t<func_t> _map, counted_t<func_t> _reduce)
        : group(env, _group), map(env, _map), reduce(env, _reduce) { }
    counted_t<func_t> compile_group(env_t *env) { return group.compile(env); }
    counted_t<func_t> compile_map(env_t *env) { return map.compile(env); }
    counted_t<func_t> compile_reduce(env_t *env) { return reduce.compile(env); }

    protob_t<const Backtrace> get_bt() const {
        // If this goes wrong at the toplevel, it goes wrong in reduce.
        return reduce.get_bt();
    }

private:
    map_wire_func_t group;
    map_wire_func_t map;
    reduce_wire_func_t reduce;
public:
    RDB_MAKE_ME_SERIALIZABLE_3(group, map, reduce);
};

// Evaluating this returns a `func_t` wrapped in a `val_t`.
class func_term_t : public term_t {
public:
    func_term_t(env_t *env, protob_t<const Term> term);
private:
    virtual bool is_deterministic_impl() const;
    virtual counted_t<val_t> eval_impl();
    virtual const char *name() const { return "func"; }
    counted_t<func_t> func;
};

} // namespace ql
#endif // RDB_PROTOCOL_FUNC_HPP_
