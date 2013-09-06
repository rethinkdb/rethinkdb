#ifndef RDB_PROTOCOL_DATUM_STREAM_HPP_
#define RDB_PROTOCOL_DATUM_STREAM_HPP_

#include <algorithm>
#include <deque>
#include <string>
#include <utility>
#include <vector>

#include "rdb_protocol/stream.hpp"

namespace query_language {
class json_stream_t;
}

namespace ql {
typedef query_language::sorting_hint_t sorting_hint_t;
typedef query_language::hinted_datum_t hinted_datum_t;

class scope_env_t;

// RSI: It's stupid for datum_stream_t to contain or use env_t at all -- we already
// decided what datums to get, so why would an env_t play any role?  We use some
// things inside of it: the interruptor, for exammple.
class datum_stream_t : public single_threaded_countable_t<datum_stream_t>,
                       public pb_rcheckable_t {
public:
    explicit datum_stream_t(const protob_t<const Backtrace> &bt_src)
        : pb_rcheckable_t(bt_src) { }
    virtual ~datum_stream_t() { }

    // stream -> stream
    virtual counted_t<datum_stream_t> filter(env_t *env,
                                             counted_t<const func_t> f,
                                             counted_t<const func_t> default_filter_val) = 0;
    virtual counted_t<datum_stream_t> map(env_t *env, counted_t<const func_t> f) = 0;
    virtual counted_t<datum_stream_t> concatmap(env_t *env,
                                                counted_t<const func_t> f) = 0;

    // stream -> atom
    virtual counted_t<const datum_t> count(env_t *env) = 0;
    virtual counted_t<const datum_t> reduce(env_t *env,
                                            counted_t<val_t> base_val,
                                            counted_t<const func_t> f) = 0;
    virtual counted_t<const datum_t> gmr(env_t *env,
                                         counted_t<const func_t> g,
                                         counted_t<const func_t> m,
                                         counted_t<const datum_t> d,
                                         counted_t<const func_t> r) = 0;


    // stream -> stream (always eager)
    counted_t<datum_stream_t> slice(size_t l, size_t r);
    counted_t<datum_stream_t> zip();
    counted_t<datum_stream_t> indexes_of(counted_t<const func_t> f);

    // Returns false or NULL respectively if stream is lazy.
    virtual bool is_array() = 0;
    virtual counted_t<const datum_t> as_array(env_t *env) = 0;

    // Gets the next element from the stream.  (Wrapper around `next_impl`.)
    counted_t<const datum_t> next(env_t *env);
    // RSI: This function is possibly silly.
    counted_t<const datum_t> next(const scope_env_t *env);

    // Gets the next elements from the stream.  (Returns zero elements only when
    // the end of the stream has been reached.  Otherwise, returns at least one
    // element.)  (Wrapper around `next_batch_impl`.)
    std::vector<counted_t<const datum_t> > next_batch(env_t *env);

    /* sorting_hint_next returns that same value that next would but in
     * addition it tells you whether or not this is part of a batch which
     * compare equal with respect to an index sorting. For example suppose you
     * have data like so (ommitting the id field):
     *
     * {id: 0, sid: 1}, {id: 1, sid : 1}, {id: 2, sid : 2}, {id: 3, sid : 3} {id: 4, sid : 3}
     *
     * with a secondary index on the attribute "sid" this function will return:
     * (START,    {id: 0, sid : 1})--+
     * (CONTINUE, {id: 1, sid : 1})--+-- These 2 could be swapped
     * (START,    {id: 2, sid : 2})
     * (START,    {id: 3, sid : 3})--+
     * (CONTINUE, {id: 4, sid : 3})--+-- These 2 could be swapped
     * (CONTINUE, NULL)
     *
     * Why is this needed:
     * This is needed in the case where you are sorting by an index but using a
     * second attribute as a tiebreaker. For example:
     *
     * table.order_by("id", index="sid")
     *
     * the sort_datum_stream_t above us needs to get batches which have the
     * same "sid" and then, only within those batches does it order by "id".
     *
     * Note: The only datum_stream_t that implements a meaningful version of
     * this functions is lazy_datum_stream_t, that's because that's the only
     * stream which could be used to do an indexed sort. Other implementations
     * of datum_stream_t always return CONTINUE this is because there data is
     * equivalent to data which has all compared equally and should all be
     * sorted together by sort_datum_stream_t. */
    // RSI: sorting_hint_next implementations don't call env->do_eval_callback().  Should they?
    virtual hinted_datum_t sorting_hint_next(env_t *env);

private:

    static const size_t MAX_BATCH_SIZE = 100;

    // Returns NULL upon end of stream.
    virtual counted_t<const datum_t> next_impl(env_t *env) = 0;
};

class eager_datum_stream_t : public datum_stream_t {
public:
    explicit eager_datum_stream_t(const protob_t<const Backtrace> &bt_src)
        : datum_stream_t(bt_src) { }

    virtual counted_t<datum_stream_t> filter(env_t *env,
                                             counted_t<const func_t> f,
                                             counted_t<const func_t> default_filter_val);
    virtual counted_t<datum_stream_t> map(env_t *env,
                                          counted_t<const func_t> f);
    virtual counted_t<datum_stream_t> concatmap(env_t *env,
                                                counted_t<const func_t> f);

    virtual counted_t<const datum_t> count(env_t *env);
    virtual counted_t<const datum_t> reduce(env_t *env,
                                            counted_t<val_t> base_val,
                                            counted_t<const func_t> f);
    virtual counted_t<const datum_t> gmr(env_t *env,
                                         counted_t<const func_t> g,
                                         counted_t<const func_t> m,
                                         counted_t<const datum_t> d,
                                         counted_t<const func_t> r);

    virtual bool is_array() { return true; }
    virtual counted_t<const datum_t> as_array(env_t *env);
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
    map_datum_stream_t(counted_t<const func_t> _f, counted_t<datum_stream_t> _source);

private:
    counted_t<const datum_t> next_impl(env_t *env);

    counted_t<const func_t> f;
    counted_t<datum_stream_t> source;
};

class indexes_of_datum_stream_t : public eager_datum_stream_t {
public:
    indexes_of_datum_stream_t(counted_t<const func_t> _f, counted_t<datum_stream_t> _source);
private:
    counted_t<const datum_t> next_impl(env_t *env);

    counted_t<const func_t> f;
    counted_t<datum_stream_t> source;

    int64_t index;
};

class filter_datum_stream_t : public eager_datum_stream_t {
public:
    filter_datum_stream_t(counted_t<const func_t> _f,
                          counted_t<const func_t> _default_filter_val,
                          counted_t<datum_stream_t> _source);
private:
    counted_t<const datum_t> next_impl(env_t *env);

    counted_t<const func_t> f;
    counted_t<const func_t> default_filter_val;
    counted_t<datum_stream_t> source;
};

class concatmap_datum_stream_t : public eager_datum_stream_t {
public:
    concatmap_datum_stream_t(counted_t<const func_t> _f, counted_t<datum_stream_t> _source);
private:
    counted_t<const datum_t> next_impl(env_t *env);

    counted_t<const func_t> f;
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
    virtual counted_t<datum_stream_t> filter(env_t *env,
                                             counted_t<const func_t> f,
                                             counted_t<const func_t> default_filter_val);
    virtual counted_t<datum_stream_t> map(env_t *env,
                                          counted_t<const func_t> f);
    virtual counted_t<datum_stream_t> concatmap(env_t *env,
                                                counted_t<const func_t> f);

    virtual counted_t<const datum_t> count(env_t *env);
    virtual counted_t<const datum_t> reduce(env_t *env,
                                            counted_t<val_t> base_val,
                                            counted_t<const func_t> f);
    virtual counted_t<const datum_t> gmr(env_t *env,
                                         counted_t<const func_t> g,
                                         counted_t<const func_t> m,
                                         counted_t<const datum_t> base,
                                         counted_t<const func_t> r);
    virtual bool is_array() { return false; }
    virtual counted_t<const datum_t> as_array(UNUSED env_t *env) {
        return counted_t<const datum_t>();  // Cannot be converted implicitly.
    }
protected:
    virtual hinted_datum_t sorting_hint_next(env_t *env);

private:
    counted_t<const datum_t> next_impl(env_t *env);

    explicit lazy_datum_stream_t(const lazy_datum_stream_t *src);
    // To make the 1.4 release, this class was basically made into a shim
    // between the datum logic and the original json streams.
    boost::shared_ptr<query_language::json_stream_t> json_stream;

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

        std::sort(data.begin(), data.end(), lt_cmp);
    }
    T lt_cmp;
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
    virtual counted_t<datum_stream_t> filter(env_t *env,
                                             counted_t<const func_t> f,
                                             counted_t<const func_t> default_filter_val);
    virtual counted_t<datum_stream_t> map(env_t *env,
                                          counted_t<const func_t> f);
    virtual counted_t<datum_stream_t> concatmap(env_t *env,
                                                counted_t<const func_t> f);

    // stream -> atom
    virtual counted_t<const datum_t> count(env_t *env);
    virtual counted_t<const datum_t> reduce(env_t *env,
                                            counted_t<val_t> base_val,
                                            counted_t<const func_t> f);
    virtual counted_t<const datum_t> gmr(env_t *env,
                                         counted_t<const func_t> g,
                                         counted_t<const func_t> m,
                                         counted_t<const datum_t> base,
                                         counted_t<const func_t> r);
    virtual bool is_array();
    virtual counted_t<const datum_t> as_array(env_t *env);
private:
    counted_t<const datum_t> next_impl(env_t *env);

    std::vector<counted_t<datum_stream_t> > streams;
    size_t streams_index;
};

} // namespace ql

#endif // RDB_PROTOCOL_DATUM_STREAM_HPP_
