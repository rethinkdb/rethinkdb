#ifndef RDB_PROTOCOL_DATUM_STREAM_HPP_
#define RDB_PROTOCOL_DATUM_STREAM_HPP_

#include "rdb_protocol/stream.hpp"

namespace ql {
class datum_stream_t : public ptr_baggable_t {
public:
    datum_stream_t(env_t *_env);
    virtual ~datum_stream_t() { }
    virtual datum_stream_t *filter(func_t *f);
    virtual datum_stream_t *map(func_t *f);
    virtual datum_stream_t *concatmap(func_t *f);
    virtual const datum_t *reduce(val_t *base_val, func_t *f);
    virtual datum_stream_t *slice(size_t l, size_t r);
    virtual const datum_t *next() = 0;

protected:
    void void_add_ptr(ptr_baggable_t *p);
    env_t *env;
};

class lazy_datum_stream_t : public datum_stream_t {
public:
    lazy_datum_stream_t(env_t *env, bool use_outdated,
                        namespace_repo_t<rdb_protocol_t>::access_t *ns_access);
    virtual datum_stream_t *filter(func_t *f);
    virtual datum_stream_t *map(func_t *f);
    virtual datum_stream_t *concatmap(func_t *f);
    virtual const datum_t *reduce(val_t *base_val, func_t *f);
    virtual const datum_t *next();
private:
    lazy_datum_stream_t(lazy_datum_stream_t *src);
    boost::shared_ptr<query_language::json_stream_t> json_stream;

    rdb_protocol_details::transform_variant_t trans;
    rdb_protocol_details::terminal_variant_t terminal;
    query_language::scopes_t _s;
    query_language::backtrace_t _b;
};

class array_datum_stream_t : public datum_stream_t {
public:
    array_datum_stream_t(env_t *env, const datum_t *_arr);
    virtual const datum_t *next();
private:
    size_t index;
    const datum_t *arr;
};

class map_datum_stream_t : public datum_stream_t {
public:
    map_datum_stream_t(env_t *env, func_t *_f, datum_stream_t *_src)
        : datum_stream_t(env), f(_f), src(_src) { guarantee(f && src); }
    virtual const datum_t *next();
private:
    func_t *f;
    datum_stream_t *src;
};

class filter_datum_stream_t : public datum_stream_t {
public:
    filter_datum_stream_t(env_t *env, func_t *_f, datum_stream_t *_src)
        : datum_stream_t(env), f(_f), src(_src) { guarantee(f && src); }
    virtual const datum_t *next();
private:
    func_t *f;
    datum_stream_t *src;
};

class concatmap_datum_stream_t : public datum_stream_t {
public:
    concatmap_datum_stream_t(env_t *env, func_t *_f, datum_stream_t *_src)
        : datum_stream_t(env), f(_f), src(_src), index(0), arr(0) {
        guarantee(f && src);
    }
    virtual const datum_t *next();
private:
    func_t *f;
    datum_stream_t *src;

    size_t index;
    const datum_t *arr;
};

class slice_datum_stream_t : public datum_stream_t {
public:
    slice_datum_stream_t(env_t *_env, size_t _l, size_t _r, datum_stream_t *_src);
    virtual const datum_t *next();
private:
    env_t *env;
    size_t ind, l, r;
    datum_stream_t *src;
};

static const size_t sort_el_limit = 1000000;
template<class T>
class sort_datum_stream_t : public datum_stream_t {
public:
    sort_datum_stream_t(env_t *env, const T &_lt_cmp, datum_stream_t *_src)
        : datum_stream_t(env), lt_cmp(_lt_cmp), src(_src), data_index(-1) {
        guarantee(src);
    }
    virtual const datum_t *next() {
        if (data_index == -1) {
            data_index = 0;
            while (const datum_t *d = src->next()) data.push_back(d);
            rcheck(data.size() <= sort_el_limit,
                   strprintf("Can only sort at most %lu elements.", sort_el_limit));
            std::sort(data.begin(), data.end(), lt_cmp);
        }
        if (data_index >= static_cast<int>(data.size())) return 0;
        //                ^^^^^^^^^^^^^^^^ this is safe because of rcheck above
        return data[data_index++];
    }
private:
    T lt_cmp;
    datum_stream_t *src;

    int data_index;
    std::vector<const datum_t *> data;
};

} // namespace ql

#endif // RDB_PROTOCOL_DATUM_STREAM_HPP_
