// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_WIRE_FUNC_HPP_
#define RDB_PROTOCOL_WIRE_FUNC_HPP_

#include <functional>
#include <map>
#include <string>
#include <vector>

#include "containers/uuid.hpp"
#include "rdb_protocol/counted_term.hpp"
#include "rdb_protocol/pb_utils.hpp"
#include "rdb_protocol/sym.hpp"
#include "rdb_protocol/var_types.hpp"
#include "rpc/serialize_macros.hpp"

template <class> class counted_t;
class Datum;
class Term;

namespace ql {
class func_t;
class env_t;

class wire_func_t {
public:
    wire_func_t();
    explicit wire_func_t(counted_t<func_t> f);
    ~wire_func_t();
    wire_func_t(const wire_func_t &copyee);
    wire_func_t &operator=(const wire_func_t &assignee);

    // Constructs a wire_func_t with a body and arglist and backtrace, but no scope.  I
    // hope you remembered to propagate the backtrace to body!
    wire_func_t(protob_t<const Term> body, std::vector<sym_t> arg_names,
                protob_t<const Backtrace> backtrace);

    counted_t<func_t> compile_wire_func() const;
    protob_t<const Backtrace> get_bt() const;

    void rdb_serialize(write_message_t &msg) const;
    archive_result_t rdb_deserialize(read_stream_t *s);

private:
    counted_t<func_t> func;
};

class map_wire_func_t : public wire_func_t {
public:
    template <class... Args>
    explicit map_wire_func_t(Args... args) : wire_func_t(args...) { }

    // Safely constructs a map wire func, that couldn't possibly capture any surprise
    // variables.
    static map_wire_func_t make_safely(
        pb::dummy_var_t dummy_var,
        const std::function<protob_t<Term>(sym_t argname)> &body_generator,
        protob_t<const Backtrace> backtrace);
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
    gmr_wire_func_t(counted_t<func_t> _group,
                    counted_t<func_t> _map,
                    counted_t<func_t> _reduce);
    counted_t<func_t> compile_group() const;
    counted_t<func_t> compile_map() const;
    counted_t<func_t> compile_reduce() const;

    protob_t<const Backtrace> get_bt() const {
        // If this goes wrong at the toplevel, it goes wrong in reduce.
        return reduce.get_bt();
    }

    RDB_MAKE_ME_SERIALIZABLE_3(group, map, reduce);

private:
    map_wire_func_t group;
    map_wire_func_t map;
    reduce_wire_func_t reduce;
};

}  // namespace ql

#endif  // RDB_PROTOCOL_WIRE_FUNC_HPP_
