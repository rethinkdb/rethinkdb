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

    template <cluster_version_t W>
    void rdb_serialize(write_message_t *wm) const;
    template <cluster_version_t W>
    archive_result_t rdb_deserialize(read_stream_t *s);

private:
    friend class maybe_wire_func_t;  // for has().
    bool has() const { return func.has(); }

    counted_t<func_t> func;
};

RDB_SERIALIZE_OUTSIDE(wire_func_t);

class maybe_wire_func_t {
protected:
    template<class... Args>
    explicit maybe_wire_func_t(Args... args) : wrapped(args...) { }

public:
    template <cluster_version_t W>
    void rdb_serialize(write_message_t *wm) const;
    template <cluster_version_t W>
    archive_result_t rdb_deserialize(read_stream_t *s);

    counted_t<func_t> compile_wire_func_or_null() const;

private:
    wire_func_t wrapped;
};

RDB_SERIALIZE_OUTSIDE(maybe_wire_func_t);

class map_wire_func_t : public wire_func_t {
public:
    template <class... Args>
    explicit map_wire_func_t(Args... args) : wire_func_t(args...) { }
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
// TODO: make `count` behave like `sum`, `avg`, etc.
struct count_wire_func_t {
};
RDB_DECLARE_SERIALIZABLE(count_wire_func_t);

class zip_wire_func_t {
};
RDB_DECLARE_SERIALIZABLE(zip_wire_func_t);

class bt_wire_func_t {
public:
    bt_wire_func_t() : bt(make_counted_backtrace()) { }
    explicit bt_wire_func_t(const protob_t<const Backtrace> &_bt) : bt(_bt) { }

    protob_t<const Backtrace> get_bt() const { return bt; }

    template <cluster_version_t W>
    void rdb_serialize(write_message_t *wm) const;
    template <cluster_version_t W>
    archive_result_t rdb_deserialize(read_stream_t *s);
private:
    protob_t<const Backtrace> bt;
};

RDB_SERIALIZE_OUTSIDE(bt_wire_func_t);

class group_wire_func_t {
public:
    group_wire_func_t() : bt(make_counted_backtrace()) { }
    group_wire_func_t(std::vector<counted_t<func_t> > &&_funcs,
                      bool _append_index, bool _multi);
    std::vector<counted_t<func_t> > compile_funcs() const;
    bool should_append_index() const;
    bool is_multi() const;
    protob_t<const Backtrace> get_bt() const;
    RDB_DECLARE_ME_SERIALIZABLE;
private:
    std::vector<wire_func_t> funcs;
    bool append_index, multi;
    bt_wire_func_t bt;
};

RDB_SERIALIZE_OUTSIDE(group_wire_func_t);

class distinct_wire_func_t {
public:
    distinct_wire_func_t() : use_index(false) { }
    explicit distinct_wire_func_t(bool _use_index) : use_index(_use_index) { }
    bool use_index;
};
RDB_DECLARE_SERIALIZABLE(distinct_wire_func_t);

template <class T>
class skip_terminal_t;

class skip_wire_func_t : public maybe_wire_func_t {
protected:
    skip_wire_func_t() { }
    template <class... Args>
    explicit skip_wire_func_t(const protob_t<const Backtrace> &_bt, Args... args)
        : maybe_wire_func_t(args...), bt(_bt) { }
private:
    template <class T>
    friend class skip_terminal_t;
    bt_wire_func_t bt;
};

class sum_wire_func_t : public skip_wire_func_t {
public:
    template <class... Args>
    explicit sum_wire_func_t(Args... args) : skip_wire_func_t(args...) { }
};
class avg_wire_func_t : public skip_wire_func_t {
public:
    template <class... Args>
    explicit avg_wire_func_t(Args... args) : skip_wire_func_t(args...) { }
};
class min_wire_func_t : public skip_wire_func_t {
public:
    template <class... Args>
    explicit min_wire_func_t(Args... args) : skip_wire_func_t(args...) { }
};
class max_wire_func_t : public skip_wire_func_t {
public:
    template <class... Args>
    explicit max_wire_func_t(Args... args) : skip_wire_func_t(args...) { }
};

}  // namespace ql

#endif  // RDB_PROTOCOL_WIRE_FUNC_HPP_
