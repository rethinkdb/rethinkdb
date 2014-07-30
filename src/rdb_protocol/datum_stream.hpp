// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_DATUM_STREAM_HPP_
#define RDB_PROTOCOL_DATUM_STREAM_HPP_

#include <algorithm>
#include <deque>
#include <list>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "errors.hpp"
#include <boost/optional.hpp>

#include "rdb_protocol/context.hpp"
#include "rdb_protocol/shards.hpp"

namespace ql {

class env_t;
class scope_env_t;

class datum_stream_t : public single_threaded_countable_t<datum_stream_t>,
                       public pb_rcheckable_t {
public:
    virtual ~datum_stream_t() { }

    virtual void add_transformation(transform_variant_t &&tv,
                                    const protob_t<const Backtrace> &bt) = 0;
    void add_grouping(transform_variant_t &&tv,
                      const protob_t<const Backtrace> &bt);

    counted_t<val_t> run_terminal(env_t *env, const terminal_variant_t &tv);
    counted_t<val_t> to_array(env_t *env);

    // stream -> stream (always eager)
    counted_t<datum_stream_t> slice(size_t l, size_t r);
    counted_t<datum_stream_t> zip();
    counted_t<datum_stream_t> indexes_of(counted_t<func_t> f);
    counted_t<datum_stream_t> ordered_distinct();

    // Returns false or NULL respectively if stream is lazy.
    virtual bool is_array() = 0;
    virtual counted_t<const datum_t> as_array(env_t *env) = 0;

    bool is_grouped() { return grouped; }

    // Gets the next elements from the stream.  (Returns zero elements only when
    // the end of the stream has been reached.  Otherwise, returns at least one
    // element.)  (Wrapper around `next_batch_impl`.)
    std::vector<counted_t<const datum_t> >
    next_batch(env_t *env, const batchspec_t &batchspec);
    // Prefer `next_batch`.  Cannot be used in conjunction with `next_batch`.
    virtual counted_t<const datum_t> next(env_t *env, const batchspec_t &batchspec);
    virtual bool is_exhausted() const = 0;
    virtual bool is_cfeed() const = 0;

    virtual void accumulate(
        env_t *env, eager_acc_t *acc, const terminal_variant_t &tv) = 0;
    virtual void accumulate_all(env_t *env, eager_acc_t *acc) = 0;

protected:
    bool batch_cache_exhausted() const;
    void check_not_grouped(const char *msg);
    explicit datum_stream_t(const protob_t<const Backtrace> &bt_src);

private:
    virtual std::vector<counted_t<const datum_t> >
    next_batch_impl(env_t *env, const batchspec_t &batchspec) = 0;

    std::vector<counted_t<const datum_t> > batch_cache;
    size_t batch_cache_index;
    bool grouped;
};

class eager_datum_stream_t : public datum_stream_t {
protected:
    explicit eager_datum_stream_t(const protob_t<const Backtrace> &bt)
        : datum_stream_t(bt) { }
    virtual counted_t<const datum_t> as_array(env_t *env);
    bool ops_to_do() { return ops.size() != 0; }

private:
    enum class done_t { YES, NO };

    virtual bool is_array() = 0;

    virtual void add_transformation(transform_variant_t &&tv,
                                    const protob_t<const Backtrace> &bt);
    virtual void accumulate(env_t *env, eager_acc_t *acc, const terminal_variant_t &tv);
    virtual void accumulate_all(env_t *env, eager_acc_t *acc);

    done_t next_grouped_batch(env_t *env, const batchspec_t &bs, groups_t *out);
    virtual std::vector<counted_t<const datum_t> >
    next_batch_impl(env_t *env, const batchspec_t &bs);
    virtual std::vector<counted_t<const datum_t> >
    next_raw_batch(env_t *env, const batchspec_t &bs) = 0;

    std::vector<scoped_ptr_t<op_t> > ops;
};

class wrapper_datum_stream_t : public eager_datum_stream_t {
protected:
    explicit wrapper_datum_stream_t(counted_t<datum_stream_t> _source)
        : eager_datum_stream_t(_source->backtrace()), source(_source) { }
private:
    virtual bool is_array() { return source->is_array(); }
    virtual counted_t<const datum_t> as_array(env_t *env) {
        return source->is_array() && !source->is_grouped()
            ? eager_datum_stream_t::as_array(env)
            : counted_t<const datum_t>();
    }
    virtual bool is_exhausted() const {
        return source->is_exhausted() && batch_cache_exhausted();
    }
    virtual bool is_cfeed() const {
        return source->is_cfeed();
    }

protected:
    const counted_t<datum_stream_t> source;
};

class indexes_of_datum_stream_t : public wrapper_datum_stream_t {
public:
    indexes_of_datum_stream_t(counted_t<func_t> _f, counted_t<datum_stream_t> _source);

private:
    std::vector<counted_t<const datum_t> >
    next_raw_batch(env_t *env, const batchspec_t &batchspec);

    counted_t<func_t> f;
    int64_t index;
};

class ordered_distinct_datum_stream_t : public wrapper_datum_stream_t {
public:
    explicit ordered_distinct_datum_stream_t(counted_t<datum_stream_t> _source);
private:
    std::vector<counted_t<const datum_t> >
    next_raw_batch(env_t *env, const batchspec_t &batchspec);
    counted_t<const datum_t> last_val;
};

class array_datum_stream_t : public eager_datum_stream_t {
public:
    array_datum_stream_t(counted_t<const datum_t> _arr,
                         const protob_t<const Backtrace> &bt_src);
    virtual bool is_exhausted() const;
    virtual bool is_cfeed() const;

private:
    virtual bool is_array();
    virtual counted_t<const datum_t> as_array(env_t *env) {
        return is_array()
            ? (ops_to_do() ? eager_datum_stream_t::as_array(env) : arr)
            : counted_t<const datum_t>();
    }
    virtual std::vector<counted_t<const datum_t> >
    next_raw_batch(env_t *env, UNUSED const batchspec_t &batchspec);
    counted_t<const datum_t> next(env_t *env, const batchspec_t &batchspec);
    counted_t<const datum_t> next_arr_el();

    size_t index;
    counted_t<const datum_t> arr;
};

class slice_datum_stream_t : public wrapper_datum_stream_t {
public:
    slice_datum_stream_t(uint64_t left, uint64_t right, counted_t<datum_stream_t> src);
private:
    virtual std::vector<counted_t<const datum_t> >
    next_raw_batch(env_t *env, const batchspec_t &batchspec);
    virtual bool is_exhausted() const;
    virtual bool is_cfeed() const;
    uint64_t index, left, right;
};

class zip_datum_stream_t : public wrapper_datum_stream_t {
public:
    explicit zip_datum_stream_t(counted_t<datum_stream_t> src);
private:
    virtual std::vector<counted_t<const datum_t> >
    next_raw_batch(env_t *env, const batchspec_t &batchspec);
};

class indexed_sort_datum_stream_t : public wrapper_datum_stream_t {
public:
    indexed_sort_datum_stream_t(
        counted_t<datum_stream_t> stream, // Must be a table with a sorting applied.
        std::function<bool(env_t *,  // NOLINT(readability/casting)
                           profile::sampler_t *,
                           const counted_t<const datum_t> &,
                           const counted_t<const datum_t> &)> lt_cmp);
private:
virtual std::vector<counted_t<const datum_t> >
next_raw_batch(env_t *env, const batchspec_t &batchspec);

std::function<bool(env_t *,  // NOLINT(readability/casting)
                   profile::sampler_t *,
                   const counted_t<const datum_t> &,
                   const counted_t<const datum_t> &)> lt_cmp;
size_t index;
std::vector<counted_t<const datum_t> > data;
};

class union_datum_stream_t : public datum_stream_t {
public:
    union_datum_stream_t(std::vector<counted_t<datum_stream_t> > &&_streams,
                         const protob_t<const Backtrace> &bt_src)
        : datum_stream_t(bt_src), streams(_streams), streams_index(0),
          is_cfeed_union(false) {
        for (auto it = streams.begin(); it != streams.end(); ++it) {
            if ((*it)->is_cfeed()) {
                is_cfeed_union = true;
                break;
            }
        }
    }

    virtual void add_transformation(transform_variant_t &&tv,
                                    const protob_t<const Backtrace> &bt);
    virtual void accumulate(env_t *env, eager_acc_t *acc, const terminal_variant_t &tv);
    virtual void accumulate_all(env_t *env, eager_acc_t *acc);

    virtual bool is_array();
    virtual counted_t<const datum_t> as_array(env_t *env);
    virtual bool is_exhausted() const;
    virtual bool is_cfeed() const;

private:
    std::vector<counted_t<const datum_t> >
    next_batch_impl(env_t *env, const batchspec_t &batchspec);

    std::vector<counted_t<datum_stream_t> > streams;
    size_t streams_index;
    bool is_cfeed_union;
};

template<class T>
T groups_to_batch(std::map<counted_t<const datum_t>, T> *g) {
    if (g->size() == 0) {
        return T();
    } else {
        r_sanity_check(g->size() == 1 && !g->begin()->first.has());
        return std::move(g->begin()->second);
    }
}

} // namespace ql

#endif // RDB_PROTOCOL_DATUM_STREAM_HPP_
