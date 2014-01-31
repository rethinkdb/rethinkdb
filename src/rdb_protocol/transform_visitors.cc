// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "rdb_protocol/transform_visitors.hpp"

namespace ql {

typedef rdb_protocol_t::rget_read_response_t::result_t result_t;
typedef rdb_protocol_t::rget_read_response_t::res_t res_t;
typedef rdb_protocol_t::rget_read_response_t::res_groups_t res_groups_t;
typedef rdb_protocol_t::rget_read_response_t::stream_t stream_t;
typedef rdb_protocol_details::rget_item_t rget_item_t;

void accumulator_t::finish(result_t *out) {
    // We fill in the result if there have been no errors.
    if (boost::get<ql::exc_t>(out) == NULL) {
        finish_impl(out);
    }
    finished = true;
}

template<class T>
class grouped_accumulator_t : public accumulator_t {
protected:
    grouped_accumulator_t(T &&_default_t) : default_t(std::move(_default_t)) { }
    virtual ~grouped_accumulator_t() { r_sanity_check(acc.size() == 0); }
private:
    virtual done_t operator()(groups_t *groups,
                              store_key_t &&key,
                              // sindex_val may be NULL
                              const counted_t<const datum_t> &sindex_val) {
        for (auto it = groups->begin(); it != groups->end(); ++it) {
            // If there's already an entry, this insert won't do anything.
            auto t_it = acc.insert(std::make_pair(it->first, default_t)).first;
            for (auto el = it->second.begin(); el != it->second.end(); ++el) {
                accumulate(*el, &*t_it, key, sindex_val);
            }
        }
        return should_send_batch() ? done_t::YES : done_t::NO;
    }
    virtual bool should_send_batch() = 0;
    virtual void finish_impl(result_t *out) {
        if (T *t = get_ungrouped_val()) {
            *out = res_t(default_t);
            T *t_out = boost::get<T>(boost::get<res_t>(out));
            *t_out = std::move(*t);
        } else {
            *out = res_groups_t();
            for (auto it = acc.begin(); it != acc.end(); ++it) {
                r_sanity_check(it->first.has());
                T *t_out = boost::get<T>(&(*boost::get<res_groups_t>(out))[it->first]);
                *t_out = std::move(it.second);
            }
        }
        acc.clear();
    }
    T *get_ungrouped_val() {
        return (acc.size() == 1 && !acc.begin().first.has())
            ? acc.begin().first.get()
            : NULL;
    }

    virtual void accumulate(const counted_t<const ql::datum_t> &el,
                            T *t,
                            store_key_t &&key,
                            // sindex_val may be NULL
                            counted_t<const datum_t> &&sindex_val) = 0;

    virtual void unshard(const store_key_t &last_key,
                         const std::vector<res_groups_t *> &rgs) {
        std::map<counted_t<const datum_t>, std::vector<T *> > vecs;
        for (auto it = rgs.begin(); it != rgs.end(); ++it) {
            for (auto kv = (*it)->begin(); kv != (*it)->end(); ++kv) {
                vecs[kv->first].push_back(boost::get<T>(&kv->second));
            }
        }
        for (auto kv = vecs.begin(); kv != vecs.end(); ++kv) {
            auto t_it = acc.insert(std::make_pair(kv->first, default_t)).second;
            unshard_impl(&*t_it, last_key, kv->second);
        }
    }
    virtual void unshard_impl(T *acc,
                              const store_key_t &last_key,
                              const std::vector<T *> &ts) = 0;

    const T default_t;
    std::map<counted_t<const ql::datum_t>, T> acc;
};

class append_t : public grouped_accumulator_t<stream_t> {
public:
    append_t(sorting_t _sorting, batcher_t *_batcher)
        : grouped_accumulator_t(stream_t()),
          sorting(_sorting), key_le(sorting), batcher(_batcher) { }
private:
    virtual bool should_send_batch() {
        return batcher != NULL && batcher->should_send_batch();
    }
    virtual void accumulate(const counted_t<const ql::datum_t> &el,
                            stream_t *stream,
                            store_key_t &&key,
                            // sindex_val may be NULL
                            counted_t<const datum_t> &&sindex_val) {
        if (batcher) batcher->note_el(el);
        // We don't bother storing the sindex if we aren't sorting (this is
        // purely a performance optimization).
        counted_t<const datum_t> rget_sindex_val = (sorting == sorting_t::UNORDERED)
            ? counted_t<const datum_t>()
            : std::move(sindex_val);
        stream->push_back(rget_item_t(std::move(key), rget_sindex_val, el));
    }
    virtual void unshard_impl(stream_t *out,
                              const store_key_t &last_key,
                              const std::vector<stream_t *> &streams) {
        size_t sz = 0;
        for (auto it = streams.begin(); it != streams.end(); ++it) {
            sz += it->size();
        }
        out->reserve(sz);
        if (sorting == sorting_t::UNORDERED) {
            for (auto it = streams.begin(); it != streams.end(); ++it) {
                for (auto item = (*it)->begin(); item != (*it)->end(); ++item) {
                    if (key_le(item->key, last_key)) {
                        out->push_back(std::move(*item));
                    }
                }
            }
        } else {
            // We do a merge sort to preserve sorting.
            std::vector<std::pair<stream_t::iterator, stream_t::iterator> > v;
            v.reserve(streams.size);
            for (auto it = streams.begin(); it != streams.end(); ++it) {
                v.push_back(std::make_pair(it->begin(), it->end()));
            }
            for (;;) {
                stream_t::iterator *best = NULL;
                for (auto it = v.begin(); it != v.end(); ++it) {
                    if (it->first != it->second) {
                        if (best == NULL || key_le(it->first->key, (*best)->key)) {
                            best = &it;
                        }
                    }
                }
                if (best == NULL || !key_le(best, last_key)) break;
                out->push_back(std::move(*(*best)++));
            }
        }
    }
private:
    const sorting_t sorting;
    const key_le_t key_le;
    batcher_t *const batcher;
};

accumulator_t *new_append_t() {
    return new append_t();
}

template<class T>
class terminal_t : public grouped_accumulator_t<T> {
protected:
    terminal_t(T &&t) : grouped_accumulator_t(std::move(t)) { }
private:
    virtual void accumulate(
        const counted_t<const ql::datum_t> &el, T *t,
        store_key_t &&, counted_t<const datum_t> &&) {
        accumulate(el, t);
    }
    virtual void accumulate(const counted_t<const ql::datum_t> &el, T *t) = 0;
    virtual void unshard_impl(T *out, const store_key_t &, const std::vector<T *> &ts) {
        unshard_impl(out, ts);
    }
    virtual void unshard_impl(T *out, const std::vector<T *> &ts) = 0;
    virtual bool should_send_batch() { return false; }
};

class count_terminal_t : public terminal_t<size_t> {
public:
    count_terminal_t(ql::env_t *, const count_wire_func_t &) : terminal_t(0) { }
private:
    virtual bool uses_val() { return false; }
    virtual void accumulate(const counted_t<const ql::datum_t> &el, size_t *out) {
        *out += 1;
    }
    virtual void unshard_impl(size_t *out, const std::vector<size_t *> &sizes) {
        for (auto it = sizes.begin(); it != sizes.end(); ++it) {
            *out += **it;
        }
    }
};

class reduce_terminal_t : public terminal_t<counted_t<const ql::datum_t> > {
public:
    reduce_terminal_t(ql::env_t *env, const reduce_wire_func_t &_f)
        : terminal_t(counted_t<const ql::datum_t>()),
          env(_env),
          f(_f.compile_wire_func()) { }
private:
    virtual void accumulate(const counted_t<const ql::datum_t> &el,
                            counted_t<const ql::datum_t> *out) {
        try {
            *out = out->has() ? f->call(env, *out, el) : el;
        } catch (const ql::datum_exc_t &e) {
            throw ql::exc_t(e, f->backtrace().get(), 1);
        }
    }
    virtual void unshard_impl(counted_t<const ql::datum_t> *out,
                              const std::vector<counted_t<const ql::datum_t> *> &ds) {
        for (auto it = ds.begin(); it != ds.end(); ++it) {
            if (it->has()) accumulate(**it, out);
        }
    }

    ql::env_t *env;
    counted_t<ql::func_t> f;
};

class terminal_visitor_t : public boost::static_visitor<accumulator_t *> {
public:
    terminal_visitor_t(ql::env_t *_env) : env(_env) { }
    accumulator_t *operator()(const count_wire_func_t &f) const {
        return new count_terminal_t(env, f);
    }
    accumulator_t *operator()(const reduce_wire_func_t &f) const {
        return new reduce_terminal_t(env, f);
    }
    ql::env_t *env;
};

accumulator_t *make_terminal(const terminal_t &t) {
    boost::apply_visitor(terminal_visitor_t(env), t);
}

class ungrouped_op_t : public op_t {
protected:
private:
    virtual void operator()(groups_t *groups) {
        for (auto it = groups->begin(); it != groups->end(); ++it) {
            lst_transform(&it->second);
        }
    }
    virtual void lst_transform(lst_t *lst) = 0;
};

class map_trans_t : public ungrouped_op_t {
public:
    map_trans_t(ql::env_t *_env, const ql::map_wire_func_t &_f)
        : env(_env), f(_f.compile_wire_func()) { }
private:
    virtual void lst_transform(lst_t *lst) {
        try {
            for (auto it = lst->begin(); it != lst->end(); ++it) {
                *it = f->call(env, *it)->as_datum();
            }
        } catch (const ql::datum_exc_t &e) {
            throw ql::exc_t(e, f->backtrace().get(), 1);
        }
    }
    ql::env_t *env;
    counted_t<ql::func_t> f;
};

class filter_trans_t : public ungrouped_op_t {
public:
    filter_trans_t(ql::env_t *_env, const filter_wire_func_t &_f)
        : env(_env),
          f(_f.compile_wire_func()),
          default_val(_f.default_filter_val
                      ? _f.default_filter_val->compile_wire_func()
                      : counted_t<func_t>()) { }
private:
    virtual void lst_transform(lst_t *lst) {
        auto it = lst->begin();
        auto loc = it;
        try {
            for (it = lst->begin(); it != lst->end(); ++it) {
                if (f->filter_call(env, *it, default_val)) {
                    loc->swap(*it);
                    ++loc;
                }
            }
        } catch (const ql::datum_exc_t &e) {
            throw ql::exc_t(e, f->backtrace().get(), 1);
        }
        lst->erase(loc, lst->end());
    }
    ql::env_t *env;
    counted_t<ql::func_t> f, default_val;
};

class concatmap_trans_t : public ungrouped_op_t {
public:
    concatmap_trans_t(ql::env_t *_env, const concatmap_wire_func_t &_f)
        : env(_env), f(_f.compile_wire_func()) { }
private:
    virtual void lst_transform(lst_t *lst) {
        lst_t new_lst;
        ql::batchspec_t bs = ql::batchspec_t::user(ql::batch_type_t::TERMINAL, env);
        ql::profile::sampler_t sampler("Evaluating CONCAT_MAP elements.", env->trace);
        try {
            for (auto it = lst->begin(); it != lst->end(); ++it) {
                auto ds = f->call(env, *it)->as_seq(env);
                while (auto v = ds->next_batch(env, bs)) {
                    new_lst.reserve(new_lst.size() + v.size());
                    new_lst.insert(new_lst.end(), v.begin(), v.end());
                    sampler.new_sample();
                }
            }
        } catch (const ql::datum_exc_t &e) {
            throw ql::exc_t(e, f->backtrace().get(), 1);
        }
        lst->swap(new_lst);
    }
    ql::env_t *env;
    counted_t<ql::func_t> f;
};

class transform_visitor_t : public boost::static_visitor_t<op_t *> {
    // We need a visitor for the variant, but only `job_data_t` needs to do this.
    friend class job_data_t;
    terminal_visitor_t(ql::env_t *_env) : env(_env) { }
    op_t *operator()(const map_wire_func_t &f) const {
        return new map_trans_t(env, f);
    }
    op_t *operator()(const filter_wire_func_t &f) const {
        return new filter_trans_t(env, f);
    }
    op_t *operator()(const concatmap_wire_func_t &f) const {
        return new concatmap_trans_t(env, f);
    }
    ql::env_t *env;
};

op_t *make_op(env_t *env, const transform_variant_t &tv) {
    return boost::apply_visitor(transform_visitor_t(env), tv);
}

} // namespace ql
