// Copyright 2010-2014 RethinkDB, all rights reserved.
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
    explicit wire_func_t(const counted_t<func_t> &f);
    ~wire_func_t();
    wire_func_t(const wire_func_t &copyee);
    wire_func_t &operator=(const wire_func_t &assignee);

    // Constructs a wire_func_t with a body and arglist and backtrace, but no scope.  I
    // hope you remembered to propagate the backtrace to body!
    wire_func_t(protob_t<const Term> body, std::vector<sym_t> arg_names,
                protob_t<const Backtrace> backtrace);

    counted_t<func_t> compile_wire_func() const;
    protob_t<const Backtrace> get_bt() const;

    void rdb_serialize(write_message_t &msg) const;  // NOLINT(runtime/references)
    archive_result_t rdb_deserialize(read_stream_t *s);

private:
    counted_t<func_t> func;
};

class group_wire_func_t {
public:
    group_wire_func_t() { }
    group_wire_func_t(std::vector<counted_t<func_t> > &&_funcs, bool _append_index);
    std::vector<counted_t<func_t> > compile_funcs() const;
    bool should_append_index() const;
    RDB_DECLARE_ME_SERIALIZABLE;
private:
    std::vector<wire_func_t> funcs;
    bool append_index;
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

class filter_wire_func_t {
public:
    filter_wire_func_t() { }
    filter_wire_func_t(const ql::wire_func_t &_filter_func,
                       const boost::optional<ql::wire_func_t> &_default_filter_val)
        : filter_func(_filter_func),
          default_filter_val(_default_filter_val) { }
    filter_wire_func_t(const counted_t<func_t> &_filter_func,
                       const boost::optional<ql::wire_func_t> &_default_filter_val)
        : filter_func(_filter_func),
          default_filter_val(_default_filter_val) { }

    ql::wire_func_t filter_func;
    boost::optional<ql::wire_func_t> default_filter_val;
};
RDB_DECLARE_SERIALIZABLE(filter_wire_func_t);

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

// These are fake functions because we don't need to send anything.
struct count_wire_func_t {
    RDB_DECLARE_ME_SERIALIZABLE;
};

class bt_wire_func_t {
public:
    void rdb_serialize(write_message_t &msg) const; // NOLINT(runtime/references)
    archive_result_t rdb_deserialize(read_stream_t *s);
    protob_t<const Backtrace> get_bt() const { return bt; }
protected:
    bt_wire_func_t() : bt(make_counted_backtrace()) { }
    explicit bt_wire_func_t(const protob_t<const Backtrace> &_bt) : bt(_bt) { }
private:
    protob_t<const Backtrace> bt;
};
class sum_wire_func_t : public bt_wire_func_t {
public:
    template<class... Args>
    explicit sum_wire_func_t(Args... args) : bt_wire_func_t(args...) { }
};
class avg_wire_func_t : public bt_wire_func_t {
public:
    template<class... Args>
    explicit avg_wire_func_t(Args... args) : bt_wire_func_t(args...) { }
};

struct min_wire_func_t {
    min_wire_func_t() { }
    explicit min_wire_func_t(const protob_t<const Backtrace> &) { }
    RDB_DECLARE_ME_SERIALIZABLE;
};
struct max_wire_func_t {
    max_wire_func_t() { }
    explicit max_wire_func_t(const protob_t<const Backtrace> &) { }
    RDB_DECLARE_ME_SERIALIZABLE;
};

}  // namespace ql

#endif  // RDB_PROTOCOL_WIRE_FUNC_HPP_
