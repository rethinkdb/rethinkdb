// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_WIRE_FUNC_HPP_
#define RDB_PROTOCOL_WIRE_FUNC_HPP_

#include <map>
#include <string>

#include "errors.hpp"
#include <boost/optional.hpp>

#include "containers/uuid.hpp"
#include "rdb_protocol/counted_term.hpp"
#include "rpc/serialize_macros.hpp"

template <class> class counted_t;
class Datum;
class Term;

namespace ql {
class func_t;
class env_t;

// Used to serialize a function (or gmr) over the wire.
class wire_func_t {
public:
    wire_func_t();
    wire_func_t(env_t *env, counted_t<func_t> _func);
    wire_func_t(const Term &_source, const std::map<int64_t, Datum> &_scope);

    counted_t<func_t> compile(env_t *env);

    protob_t<const Backtrace> get_bt() const;

    const Term &get_term() const {
        return *source;
    }

    std::string debug_str() const;

    // They're manually implemented because source is now a protob_t<Term>.
    void rdb_serialize(write_message_t &msg) const;  // NOLINT(runtime/references)
    archive_result_t rdb_deserialize(read_stream_t *stream);

private:
    friend class func_cache_t;
    // source is never null, even when wire_func_t is default-constructed.
    protob_t<Term> source;
    boost::optional<Term> default_filter_val;
    std::map<int64_t, Datum> scope;
    // This uuid is used for the func cache in `env_t`.
    uuid_u uuid;
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
    gmr_wire_func_t(env_t *env, counted_t<func_t> _group,
                    counted_t<func_t> _map,
                    counted_t<func_t> _reduce);
    counted_t<func_t> compile_group(env_t *env);
    counted_t<func_t> compile_map(env_t *env);
    counted_t<func_t> compile_reduce(env_t *env);

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

}  // namespace ql

#endif  // RDB_PROTOCOL_WIRE_FUNC_HPP_
