// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_DATUM_STREAM_HPP_
#define RDB_PROTOCOL_DATUM_STREAM_HPP_

#include <algorithm>
#include <deque>
#include <string>
#include <utility>
#include <vector>

#include "rdb_protocol/stream.hpp"

namespace query_language {
class batched_rget_stream_t;
}

namespace ql {
typedef query_language::sorting_hint_t sorting_hint_t;
typedef query_language::hinted_datum_t hinted_datum_t;

class scope_env_t;

class datum_stream_t : public single_threaded_countable_t<datum_stream_t>,
                       public pb_rcheckable_t {
public:
    virtual ~datum_stream_t() { }

    // stream -> stream
    virtual counted_t<datum_stream_t> filter(counted_t<func_t> f,
                                             counted_t<func_t> default_filter_val) = 0;
    virtual counted_t<datum_stream_t> map(counted_t<func_t> f) = 0;
    virtual counted_t<datum_stream_t> concatmap(counted_t<func_t> f) = 0;

    // stream -> atom
    virtual counted_t<const datum_t> count(env_t *env) = 0;
    virtual counted_t<const datum_t> reduce(env_t *env,
                                            counted_t<val_t> base_val,
                                            counted_t<func_t> f) = 0;
    virtual counted_t<const datum_t> gmr(env_t *env,
                                         counted_t<func_t> g,
                                         counted_t<func_t> m,
                                         counted_t<const datum_t> d,
                                         counted_t<func_t> r) = 0;


    // stream -> stream (always eager)
    counted_t<datum_stream_t> slice(size_t l, size_t r);
    counted_t<datum_stream_t> zip();
    counted_t<datum_stream_t> indexes_of(counted_t<func_t> f);

    // Returns false or NULL respectively if stream is lazy.
    virtual bool is_array() = 0;
    virtual counted_t<const datum_t> as_array(env_t *env) = 0;

    // Gets the next elements from the stream.  (Returns zero elements only when
    // the end of the stream has been reached.  Otherwise, returns at least one
    // element.)  (Wrapper around `next_batch_impl`.)
    std::vector<counted_t<const datum_t> > next_batch(env_t *env);

    // Returns empty `counted_t` at end of stream.  Prefer `next_batch`.
    counted_t<const datum_t> next(env_t *env);

    hinted_datum_t sorting_hint_next(env_t *env); // UNIMPLEMENTED
protected:
    explicit datum_stream_t(const protob_t<const Backtrace> &bt_src)
        : pb_rcheckable_t(bt_src) { }

private:
    static const size_t MAX_BATCH_SIZE = 100;

    virtual std::vector<counted_t<const datum_t> > next_batch_impl(env_t *env) = 0;
    virtual counted_t<const datum_t> next_impl(env_t *env) = 0;
};

class eager_datum_stream_t : public datum_stream_t {
protected:
    explicit eager_datum_stream_t(const protob_t<const Backtrace> &bt_src)
        : datum_stream_t(bt_src) { }

    virtual counted_t<datum_stream_t> filter(counted_t<func_t> f,
                                             counted_t<func_t> default_filter_val);
    virtual counted_t<datum_stream_t> map(counted_t<func_t> f);
    virtual counted_t<datum_stream_t> concatmap(counted_t<func_t> f);

    virtual counted_t<const datum_t> count(env_t *env);
    virtual counted_t<const datum_t> reduce(env_t *env,
                                            counted_t<val_t> base_val,
                                            counted_t<func_t> f);
    virtual counted_t<const datum_t> gmr(env_t *env,
                                         counted_t<func_t> g,
                                         counted_t<func_t> m,
                                         counted_t<const datum_t> d,
                                         counted_t<func_t> r);

    virtual bool is_array() { return true; }
    virtual counted_t<const datum_t> as_array(env_t *env);
private:
    std::vector<counted_t<const datum_t> > next_batch_impl(env_t *env);
};

class wrapper_datum_stream_t : public eager_datum_stream_t {
public:
    explicit wrapper_datum_stream_t(counted_t<datum_stream_t> _source)
        : eager_datum_stream_t(_source->backtrace()), source(_source) { }
    virtual bool is_array() { return source->is_array(); }
    virtual counted_t<const datum_t> as_array(env_t *env) {
        return is_array()
            ? eager_datum_stream_t::as_array(env)
            : counted_t<const datum_t>();
    }

protected:
    const counted_t<datum_stream_t> source;
};

class map_datum_stream_t : public eager_datum_stream_t {
public:
    map_datum_stream_t(counted_t<func_t> _f, counted_t<datum_stream_t> _source);

private:
    counted_t<const datum_t> next_impl(env_t *env);

    counted_t<func_t> f;
    counted_t<datum_stream_t> source;
};

class indexes_of_datum_stream_t : public eager_datum_stream_t {
public:
    indexes_of_datum_stream_t(counted_t<func_t> _f, counted_t<datum_stream_t> _source);

private:
    counted_t<const datum_t> next_impl(env_t *env);

    counted_t<func_t> f;
    counted_t<datum_stream_t> source;

    int64_t index;
};

class filter_datum_stream_t : public eager_datum_stream_t {
public:
    filter_datum_stream_t(counted_t<func_t> _f,
                          counted_t<func_t> _default_filter_val,
                          counted_t<datum_stream_t> _source);

private:
    counted_t<const datum_t> next_impl(env_t *env);

    counted_t<func_t> f;
    counted_t<func_t> default_filter_val;
    counted_t<datum_stream_t> source;
};

class concatmap_datum_stream_t : public eager_datum_stream_t {
public:
    concatmap_datum_stream_t(counted_t<func_t> _f, counted_t<datum_stream_t> _source);

private:
    counted_t<const datum_t> next_impl(env_t *env);

    counted_t<func_t> f;
    counted_t<datum_stream_t> source;
    counted_t<datum_stream_t> subsource;
};

class lazy_datum_stream_t : public datum_stream_t {
public:
    lazy_datum_stream_t(env_t *env, bool use_outdated,
                        namespace_repo_t<rdb_protocol_t>::access_t *ns_access,
                        sorting_t sorting, const protob_t<const Backtrace> &bt_src);
    lazy_datum_stream_t(env_t *env, bool use_outdated,
                        namespace_repo_t<rdb_protocol_t>::access_t *ns_access,
                        const std::string &sindex_id, sorting_t sorting,
                        const protob_t<const Backtrace> &bt_src);
    lazy_datum_stream_t(env_t *env, bool use_outdated,
                        namespace_repo_t<rdb_protocol_t>::access_t *ns_access,
                        counted_t<const datum_t> left_bound, bool left_bound_open,
                        counted_t<const datum_t> right_bound, bool right_bound_open,
                        sorting_t sorting, const protob_t<const Backtrace> &bt_src);
    lazy_datum_stream_t(env_t *env, bool use_outdated,
                        namespace_repo_t<rdb_protocol_t>::access_t *ns_access,
                        counted_t<const datum_t> left_bound, bool left_bound_open,
                        counted_t<const datum_t> right_bound, bool right_bound_open,
                        const std::string &sindex_id, sorting_t sorting,
                        const protob_t<const Backtrace> &bt_src);
    virtual counted_t<datum_stream_t> filter(counted_t<func_t> f,
                                             counted_t<func_t> default_filter_val);
    virtual counted_t<datum_stream_t> map(counted_t<func_t> f);
    virtual counted_t<datum_stream_t> concatmap(counted_t<func_t> f);

    virtual counted_t<const datum_t> count(env_t *env);
    virtual counted_t<const datum_t> reduce(env_t *env,
                                            counted_t<val_t> base_val,
                                            counted_t<func_t> f);
    virtual counted_t<const datum_t> gmr(env_t *env,
                                         counted_t<func_t> g,
                                         counted_t<func_t> m,
                                         counted_t<const datum_t> base,
                                         counted_t<func_t> r);
    virtual bool is_array() { return false; }
    virtual counted_t<const datum_t> as_array(UNUSED env_t *env) {
        return counted_t<const datum_t>();  // Cannot be converted implicitly.
    }

protected:
    virtual hinted_datum_t sorting_hint_next(env_t *env);

private:
    counted_t<const datum_t> next_impl(env_t *env);
    std::vector<counted_t<const datum_t> > next_batch_impl(env_t *env);

    explicit lazy_datum_stream_t(const lazy_datum_stream_t *src);
    // To make the 1.4 release, this class was basically made into a shim
    // between the datum logic and the original json streams.
    counted_t<query_language::batched_rget_stream_t> json_stream;

    rdb_protocol_t::rget_read_response_t::result_t run_terminal(
            env_t *env,
            const rdb_protocol_details::terminal_variant_t &t);
};

class array_datum_stream_t : public eager_datum_stream_t {
public:
    array_datum_stream_t(counted_t<const datum_t> _arr,
                         const protob_t<const Backtrace> &bt_src);

private:
    counted_t<const datum_t> next_impl(env_t *env);

    size_t index;
    counted_t<const datum_t> arr;
};

class slice_datum_stream_t : public wrapper_datum_stream_t {
public:
    slice_datum_stream_t(size_t left, size_t right, counted_t<datum_stream_t> src);
private:
    counted_t<const datum_t> next_impl(env_t *env);
    uint64_t index, left, right;
};

class zip_datum_stream_t : public wrapper_datum_stream_t {
public:
    explicit zip_datum_stream_t(counted_t<datum_stream_t> src);
private:
    counted_t<const datum_t> next_impl(env_t *env);
};

// This has to be constructed explicitly rather than invoking `.sort()`.  There
// was a good reason for this involving header dependencies, but I don't
// remember exactly what it was.
static const size_t sort_el_limit = 1000000; // maximum number of elements we'll sort
template<class T>
class sort_datum_stream_t : public eager_datum_stream_t {
public:
    sort_datum_stream_t(env_t *env,
                        const T &_lt_cmp, counted_t<datum_stream_t> _src,
                        const protob_t<const Backtrace> &bt_src)
        : eager_datum_stream_t(bt_src),
        lt_cmp(_lt_cmp), src(_src), is_arr_(false) {
        r_sanity_check(src.has());
        load_data(env);
    }

    counted_t<const datum_t> next_impl(env_t *env) {
        if (data.empty()) {
            load_data(env);
            if (data.empty()) {
                return counted_t<const datum_t>();
            }
        }

        counted_t<const datum_t> res = data.front();
        data.pop_front();
        return res;
    }

private:
    counted_t<const datum_t> as_array(env_t *env) {
        return is_arr()
            ? eager_datum_stream_t::as_array(env)
            : counted_t<const datum_t>();
    }
    bool is_arr() {
        return is_arr_;
    }
    void load_data(env_t *env) {
        r_sanity_check(data.empty());

        if (counted_t<const datum_t> arr = src->as_array(env)) {
            if (is_arr_) {
                /* We already loaded data from the array which means there's no
                 * more data. */
                return;
            }
            is_arr_ = true;
            rcheck(arr->size() <= sort_el_limit,
                   base_exc_t::GENERIC,
                   strprintf("Can only sort at most %zu elements.",
                             sort_el_limit));
            for (size_t i = 0; i < arr->size(); ++i) {
                data.push_back(arr->get(i));
            }
        } else {
            if (next_element) {
                data.push_back(next_element);
                next_element = counted_t<const datum_t>();
            }

            for (;;) {
                const hinted_datum_t d = src->sorting_hint_next(env);
                if (!d.second) {
                    break;
                }

                if (d.first == query_language::START && !data.empty()) {
                    //debugf("Got a new value:\n %s\n", d.second->print().c_str());
                    next_element = d.second;
                    break;
                } else {
                    data.push_back(d.second);
                    rcheck(data.size() <= sort_el_limit,
                           base_exc_t::GENERIC,
                           strprintf("Can only sort at most %zu elements.",
                                     sort_el_limit));
                }
            }
        }

        std::sort(data.begin(), data.end(),
                  std::bind(lt_cmp, env, std::placeholders::_1, std::placeholders::_2));
    }
    std::function<bool(env_t *,
                       const counted_t<const datum_t> &,
                       const counted_t<const datum_t> &)> lt_cmp;
    counted_t<datum_stream_t> src;

    std::deque<counted_t<const datum_t> > data;
    counted_t<const datum_t> next_element;
    bool is_arr_;
};

class union_datum_stream_t : public datum_stream_t {
public:
    union_datum_stream_t(const std::vector<counted_t<datum_stream_t> > &_streams,
                         const protob_t<const Backtrace> &bt_src)
        : datum_stream_t(bt_src), streams(_streams), streams_index(0) { }

    // stream -> stream
    virtual counted_t<datum_stream_t> filter(counted_t<func_t> f,
                                             counted_t<func_t> default_filter_val);
    virtual counted_t<datum_stream_t> map(counted_t<func_t> f);
    virtual counted_t<datum_stream_t> concatmap(counted_t<func_t> f);

    // stream -> atom
    virtual counted_t<const datum_t> count(env_t *env);
    virtual counted_t<const datum_t> reduce(env_t *env,
                                            counted_t<val_t> base_val,
                                            counted_t<func_t> f);
    virtual counted_t<const datum_t> gmr(env_t *env,
                                         counted_t<func_t> g,
                                         counted_t<func_t> m,
                                         counted_t<const datum_t> base,
                                         counted_t<func_t> r);
    virtual bool is_array();
    virtual counted_t<const datum_t> as_array(env_t *env);

private:
    counted_t<const datum_t> next_impl(env_t *env);
    std::vector<counted_t<const datum_t> > next_batch_impl(env_t *env);

    std::vector<counted_t<datum_stream_t> > streams;
    size_t streams_index;
};

} // namespace ql

#endif // RDB_PROTOCOL_DATUM_STREAM_HPP_
