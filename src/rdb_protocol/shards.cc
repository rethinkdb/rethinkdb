// Copyright 2010-2013 RethinkDB, all rights reserved.

#include "boost/variant.hpp"

#include "rdb_protocol/func.hpp"
#include "rdb_protocol/profile.hpp"
#include "rdb_protocol/protocol.hpp"
#include "rdb_protocol/shards.hpp"

namespace ql {

bool reversed(sorting_t sorting) { return sorting == sorting_t::DESCENDING; }

accumulator_t::accumulator_t() : finished(false) { }
accumulator_t::~accumulator_t() {
    if (!std::uncaught_exception()) {
        r_sanity_check(finished);
    }
}
void accumulator_t::mark_finished() { finished = true; }

void accumulator_t::finish(result_t *out) {
    mark_finished();
    // We fill in the result if there have been no errors.
    if (boost::get<exc_t>(out) == NULL) {
        finish_impl(out);
    }
}

template<class T>
class grouped_accumulator_t : public accumulator_t {
protected:
    grouped_accumulator_t(T &&_default_t) : default_t(std::move(_default_t)) { }
    virtual ~grouped_accumulator_t() { }
private:
    virtual done_t operator()(groups_t *groups,
                              store_key_t &&key,
                              counted_t<const datum_t> &&sindex_val) {
        for (auto it = groups->begin(); it != groups->end(); ++it) {
            // If there's already an entry, this insert won't do anything.
            auto t_it = acc.insert(std::make_pair(it->first, default_t)).first;
            for (auto el = it->second.begin(); el != it->second.end(); ++el) {
                accumulate(*el, &t_it->second, std::move(key), std::move(sindex_val));
            }
        }
        return should_send_batch() ? done_t::YES : done_t::NO;
    }
    virtual void accumulate(const counted_t<const datum_t> &el,
                            T *t,
                            store_key_t &&key,
                            // sindex_val may be NULL
                            counted_t<const datum_t> &&sindex_val) = 0;

    virtual bool should_send_batch() = 0;

    virtual void finish_impl(result_t *out) {
        *out = grouped<T>();
        boost::get<grouped<T> >(*out).swap(acc);
        guarantee(acc.size() == 0);
    }

    virtual void unshard(const store_key_t &last_key,
                         const std::vector<result_t *> &results) {
        guarantee(acc.size() == 0);
        std::map<counted_t<const datum_t>, std::vector<T *> > vecs;
        for (auto res = results.begin(); res != results.end(); ++res) {
            guarantee(*res);
            grouped<T> *gres = boost::get<grouped<T> >(*res);
            guarantee(gres);
            for (auto kv = gres->begin(); kv != gres->end(); ++kv) {
                vecs[kv->first].push_back(&kv->second);
            }
        }
        for (auto kv = vecs.begin(); kv != vecs.end(); ++kv) {
            auto t_it = acc.insert(std::make_pair(kv->first, default_t)).first;
            unshard_impl(&t_it->second, last_key, kv->second);
        }
    }
    virtual void unshard_impl(T *acc,
                              const store_key_t &last_key,
                              const std::vector<T *> &ts) = 0;

protected:
    const T *get_default_t() { return &default_t; }
    grouped<T> *get_acc() { return &acc; }
private:
    const T default_t;
    grouped<T> acc;
};

class append_t : public grouped_accumulator_t<stream_t> {
public:
    append_t(sorting_t _sorting, batcher_t *_batcher)
        : grouped_accumulator_t<stream_t>(stream_t()),
          sorting(_sorting), key_le(sorting), batcher(_batcher) { }
private:
    virtual bool should_send_batch() {
        return batcher != NULL && batcher->should_send_batch();
    }
    virtual void accumulate(const counted_t<const datum_t> &el,
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

    virtual counted_t<const datum_t> unpack(stream_t *) {
        r_sanity_check(false); // We never unpack a stream.
        unreachable();
    }

    virtual void unshard_impl(stream_t *out,
                              const store_key_t &last_key,
                              const std::vector<stream_t *> &streams) {
        uint64_t sz = 0;
        for (auto it = streams.begin(); it != streams.end(); ++it) {
            sz += (*it)->size();
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
            v.reserve(streams.size());
            for (auto it = streams.begin(); it != streams.end(); ++it) {
                v.push_back(std::make_pair((*it)->begin(), (*it)->end()));
            }
            for (;;) {
                stream_t::iterator *best = NULL;
                for (auto it = v.begin(); it != v.end(); ++it) {
                    if (it->first != it->second) {
                        if (best == NULL || key_le(it->first->key, (*best)->key)) {
                            best = &it->first;
                        }
                    }
                }
                if (best == NULL || !key_le((*best)->key, last_key)) break;
                out->push_back(std::move(**best));
                ++(*best);
            }
        }
    }
private:
    const sorting_t sorting;
    const key_le_t key_le;
    batcher_t *const batcher;
};

accumulator_t *make_append(const sorting_t &sorting, batcher_t *batcher) {
    return new append_t(sorting, batcher);
}

template<class T>
class terminal_t : public grouped_accumulator_t<T>, public eager_acc_t {
protected:
    terminal_t(T &&t) : grouped_accumulator_t<T>(std::move(t)) { }
private:
    virtual void operator()(groups_t *groups) {
        grouped<T> *acc = grouped_accumulator_t<T>::get_acc();
        const T *default_t = grouped_accumulator_t<T>::get_default_t();
        for (auto it = groups->begin(); it != groups->end(); ++it) {
            auto t_it = acc->insert(std::make_pair(it->first, *default_t)).first;
            for (auto el = it->second.begin(); el != it->second.end(); ++el) {
                accumulate(*el, &t_it->second);
            }
        }
        groups->clear();
    }

    // RSI: empty?
    virtual counted_t<val_t> finish_eager(protob_t<const Backtrace> bt) {
        accumulator_t::mark_finished();
        grouped<T> *acc = grouped_accumulator_t<T>::get_acc();
        const T *default_t = grouped_accumulator_t<T>::get_default_t();
        counted_t<val_t> retval;
        if (acc->size() == 0) {
            T t(*default_t);
            retval = make_counted<val_t>(unpack(&t), bt);
        } else if (acc->size() == 1 && !acc->begin()->first.has()) {
            retval = make_counted<val_t>(unpack(&acc->begin()->second), bt);
        } else {
            std::map<counted_t<const datum_t>, counted_t<const datum_t> > ret;
            for (auto kv = acc->begin(); kv != acc->end(); ++kv) {
                ret.insert(std::make_pair(kv->first, unpack(&kv->second)));
            }
            retval = make_counted<val_t>(std::move(ret), bt);
        }
        acc->clear();
        return retval;
    }
    virtual counted_t<const datum_t> unpack(T *t) = 0;

    virtual void add_res(result_t &&res) {
        grouped<T> *acc = grouped_accumulator_t<T>::get_acc();
        const T *default_t = grouped_accumulator_t<T>::get_default_t();
        if (auto e = boost::get<exc_t>(&res)) {
            throw *e;
        }
        grouped<T> *gres = boost::get<grouped<T> >(&res);
        if (acc->size() == 0) {
            acc->swap(*gres);
        } else {
            for (auto kv = gres->begin(); kv != gres->end(); ++kv) {
                auto t_it = acc->insert(std::make_pair(kv->first, *default_t)).first;
                unshard_impl(&t_it->second, &kv->second);
            }
        }
    }

    virtual void accumulate(
        const counted_t<const datum_t> &el, T *t,
        store_key_t &&, counted_t<const datum_t> &&) {
        accumulate(el, t);
    }
    virtual void accumulate(const counted_t<const datum_t> &el, T *t) = 0;

    virtual void unshard_impl(T *out, const store_key_t &, const std::vector<T *> &ts) {
        for (auto it = ts.begin(); it != ts.end(); ++it) {
            unshard_impl(out, *it);
        }
    }
    virtual void unshard_impl(T *out, T *el) = 0;
    virtual bool should_send_batch() { return false; }
};

class count_terminal_t : public terminal_t<uint64_t> {
public:
    count_terminal_t(env_t *, const count_wire_func_t &) : terminal_t(0) { }
private:
    virtual bool uses_val() { return false; }
    virtual void accumulate(const counted_t<const datum_t> &, uint64_t *out) {
        *out += 1;
    }
    virtual counted_t<const datum_t> unpack(uint64_t *sz) {
        return make_counted<const datum_t>(double(*sz));
    }
    virtual void unshard_impl(uint64_t *out, uint64_t *el) {
        *out += *el;
    }
};

const char *const empty_stream_msg =
    "Cannot reduce over an empty stream with no base.";

class reduce_terminal_t : public terminal_t<counted_t<const datum_t> > {
public:
    reduce_terminal_t(env_t *_env, const reduce_wire_func_t &_f)
        : terminal_t(counted_t<const datum_t>()),
          env(_env),
          f(_f.compile_wire_func()) { }
private:
    virtual void accumulate(const counted_t<const datum_t> &el,
                            counted_t<const datum_t> *out) {
        try {
            *out = out->has() ? f->call(env, *out, el)->as_datum() : el;
        } catch (const datum_exc_t &e) {
            throw exc_t(e, f->backtrace().get(), 1);
        }
    }
    virtual counted_t<const datum_t> unpack(counted_t<const datum_t> *el) {
        rcheck_target(f, base_exc_t::NON_EXISTENCE, el->has(), empty_stream_msg);
        return std::move(*el);
    }
    virtual void unshard_impl(counted_t<const datum_t> *out,
                              counted_t<const datum_t> *el) {
        if (el->has()) accumulate(*el, out);
    }

    env_t *env;
    counted_t<func_t> f;
};

template<class T>
class terminal_visitor_t : public boost::static_visitor<T *> {
public:
    terminal_visitor_t(env_t *_env) : env(_env) { }
    T *operator()(const count_wire_func_t &f) const {
        return new count_terminal_t(env, f);
    }
    T *operator()(const reduce_wire_func_t &f) const {
        return new reduce_terminal_t(env, f);
    }
    env_t *env;
};

accumulator_t *make_terminal(env_t *env, const terminal_variant_t &t) {
    return boost::apply_visitor(terminal_visitor_t<accumulator_t>(env), t);
}

eager_acc_t *make_eager_terminal(env_t *env, const terminal_variant_t &t) {
    return boost::apply_visitor(terminal_visitor_t<eager_acc_t>(env), t);
}

class ungrouped_op_t : public op_t {
protected:
private:
    virtual void operator()(groups_t *groups) {
        for (auto it = groups->begin(); it != groups->end(); ++it) {
            lst_transform(&it->second);
            if (it->second.size() == 0) {
                groups->erase(it); // This is important for batching with filter.
            }
        }
    }
    virtual void lst_transform(lst_t *lst) = 0;
};

class map_trans_t : public ungrouped_op_t {
public:
    map_trans_t(env_t *_env, const map_wire_func_t &_f)
        : env(_env), f(_f.compile_wire_func()) { }
private:
    virtual void lst_transform(lst_t *lst) {
        try {
            for (auto it = lst->begin(); it != lst->end(); ++it) {
                *it = f->call(env, *it)->as_datum();
            }
        } catch (const datum_exc_t &e) {
            throw exc_t(e, f->backtrace().get(), 1);
        }
    }
    env_t *env;
    counted_t<func_t> f;
};

class filter_trans_t : public ungrouped_op_t {
public:
    filter_trans_t(env_t *_env, const filter_wire_func_t &_f)
        : env(_env),
          f(_f.filter_func.compile_wire_func()),
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
        } catch (const datum_exc_t &e) {
            throw exc_t(e, f->backtrace().get(), 1);
        }
        lst->erase(loc, lst->end());
    }
    env_t *env;
    counted_t<func_t> f, default_val;
};

class concatmap_trans_t : public ungrouped_op_t {
public:
    concatmap_trans_t(env_t *_env, const concatmap_wire_func_t &_f)
        : env(_env), f(_f.compile_wire_func()) { }
private:
    virtual void lst_transform(lst_t *lst) {
        lst_t new_lst;
        batchspec_t bs = batchspec_t::user(batch_type_t::TERMINAL, env);
        profile::sampler_t sampler("Evaluating CONCAT_MAP elements.", env->trace);
        try {
            for (auto it = lst->begin(); it != lst->end(); ++it) {
                auto ds = f->call(env, *it)->as_seq(env);
                for (;;) {
                    auto v = ds->next_batch(env, bs);
                    if (v.size() == 0) break;
                    new_lst.reserve(new_lst.size() + v.size());
                    new_lst.insert(new_lst.end(), v.begin(), v.end());
                    sampler.new_sample();
                }
            }
        } catch (const datum_exc_t &e) {
            throw exc_t(e, f->backtrace().get(), 1);
        }
        lst->swap(new_lst);
    }
    env_t *env;
    counted_t<func_t> f;
};

class transform_visitor_t : public boost::static_visitor<op_t *> {
public:
    transform_visitor_t(env_t *_env) : env(_env) { }
    op_t *operator()(const map_wire_func_t &f) const {
        return new map_trans_t(env, f);
    }
    op_t *operator()(const filter_wire_func_t &f) const {
        return new filter_trans_t(env, f);
    }
    op_t *operator()(const concatmap_wire_func_t &f) const {
        return new concatmap_trans_t(env, f);
    }
private:
    env_t *env;
};

op_t *make_op(env_t *env, const transform_variant_t &tv) {
    return boost::apply_visitor(transform_visitor_t(env), tv);
}

RDB_IMPL_ME_SERIALIZABLE_3(rget_item_t, key, empty_ok(sindex_key), data);

} // namespace ql
