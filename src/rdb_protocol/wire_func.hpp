// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_WIRE_FUNC_HPP_
#define RDB_PROTOCOL_WIRE_FUNC_HPP_

#include <vector>

#include "errors.hpp"
#include <boost/optional.hpp>

#include "containers/counted.hpp"
#include "rdb_protocol/sym.hpp"
#include "rdb_protocol/error.hpp"
#include "rpc/serialize_macros.hpp"
#include "version.hpp"

namespace ql {
class raw_term_t;
class func_t;
class env_t;

class wire_func_t {
public:
    wire_func_t();
    explicit wire_func_t(const counted_t<const func_t> &f);
    ~wire_func_t();
    wire_func_t(const wire_func_t &copyee);
    wire_func_t &operator=(const wire_func_t &assignee);

    // Constructs a wire_func_t with a body and arglist, but no scope.
    wire_func_t(const raw_term_t &body,
                std::vector<sym_t> arg_names);

    counted_t<const func_t> compile_wire_func() const;
    backtrace_id_t get_bt() const;

    template <cluster_version_t W>
    friend void serialize(write_message_t *wm, const wire_func_t &);
    template <cluster_version_t W>
    friend archive_result_t deserialize(read_stream_t *s, wire_func_t *wf);
    template <cluster_version_t W>
    friend archive_result_t deserialize_wire_func(read_stream_t *s, wire_func_t *wf);

    bool is_simple_selector() const;
private:
    friend class maybe_wire_func_t;  // for has().

    bool has() const { return func.has(); }

    counted_t<const func_t> func;
};

class maybe_wire_func_t {
protected:
    template<class... Args>
    explicit maybe_wire_func_t(Args... args) : wrapped(args...) { }

public:
    template <cluster_version_t W>
    friend void serialize(write_message_t *wm, const maybe_wire_func_t &);
    template <cluster_version_t W>
    friend archive_result_t deserialize(read_stream_t *s, maybe_wire_func_t *);

    counted_t<const func_t> compile_wire_func_or_null() const;

private:
    bool has() const { return wrapped.has(); }
    wire_func_t wrapped;
};

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
    filter_wire_func_t(const counted_t<const func_t> &_filter_func,
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

enum class result_hint_t { NO_HINT, AT_MOST_ONE };
class concatmap_wire_func_t : public wire_func_t {
public:
    concatmap_wire_func_t() { }
    template <class... Args>
    explicit concatmap_wire_func_t(result_hint_t _result_hint, Args... args)
        : wire_func_t(args...), result_hint(_result_hint) { }
    // We use this so that terms which rewrite to a `concat_map` but can never
    // produce more than one result can be handled correctly by `changes`.
    result_hint_t result_hint;
};

// These are fake functions because we don't need to send anything.
// TODO: make `count` behave like `sum`, `avg`, etc.
struct count_wire_func_t {
};
RDB_DECLARE_SERIALIZABLE(count_wire_func_t);

class zip_wire_func_t {
};
RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(zip_wire_func_t);

class group_wire_func_t {
public:
    group_wire_func_t() : bt(backtrace_id_t::empty()) { }
    group_wire_func_t(std::vector<counted_t<const func_t> > &&_funcs,
                      bool _append_index, bool _multi);
    std::vector<counted_t<const func_t> > compile_funcs() const;
    bool should_append_index() const;
    bool is_multi() const;
    backtrace_id_t get_bt() const;
    RDB_DECLARE_ME_SERIALIZABLE(group_wire_func_t);
private:
    std::vector<wire_func_t> funcs;
    bool append_index, multi;
    backtrace_id_t bt;
};

class distinct_wire_func_t {
public:
    distinct_wire_func_t() : use_index(false) { }
    explicit distinct_wire_func_t(bool _use_index) : use_index(_use_index) { }
    bool use_index;
};
RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(distinct_wire_func_t);

template <class T>
class skip_terminal_t;

class skip_wire_func_t : public maybe_wire_func_t {
protected:
    skip_wire_func_t() { }
    template <class... Args>
    explicit skip_wire_func_t(backtrace_id_t _bt, Args... args)
        : maybe_wire_func_t(args...), bt(_bt) { }
private:
    template <class T>
    friend class skip_terminal_t;
    backtrace_id_t bt;
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
