// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "rdb_protocol/shards.hpp"

#include "errors.hpp"
#include <boost/variant.hpp>

#include "debug.hpp"
#include "rdb_protocol/func.hpp"
#include "rdb_protocol/profile.hpp"
#include "rdb_protocol/protocol.hpp"

bool reversed(sorting_t sorting) { return sorting == sorting_t::DESCENDING; }

namespace ql {

accumulator_t::accumulator_t() : finished(false) { }
accumulator_t::~accumulator_t() { }
void accumulator_t::mark_finished() { finished = true; }

void accumulator_t::finish(result_t *out) {
    mark_finished();
    // We fill in the result if there have been no errors.
    if (boost::get<exc_t>(out) == NULL) {
        finish_impl(out);
    }
}

#ifndef NDEBUG
// These are really useful if datum terminals aren't doing what they're supposed to.
template<class T>
void dprint(const char *s, const T &) {
    debugf("%s -> ???\n", s);
}

template<>
void dprint(const char *s, const counted_t<const datum_t> &t) {
    if (t.has()) {
        debugf("%s -> %s\n", s, t->print().c_str());
    } else {
        debugf("%s -> NULL\n", s);
    }
}
#endif // NDEBUG

template<class T>
class grouped_acc_t : public accumulator_t {
protected:
    explicit grouped_acc_t(T &&_default_val)
        : default_val(std::move(_default_val)) { }
    virtual ~grouped_acc_t() { }
private:
    virtual done_traversing_t operator()(env_t *env,
                                         groups_t *groups,
                                         const store_key_t &key,
                                         const counted_t<const datum_t> &sindex_val) {
        for (auto it = groups->begin(); it != groups->end(); ++it) {
            auto pair = acc.insert(std::make_pair(it->first, default_val));
            auto t_it = pair.first;
            bool keep = !pair.second;
            for (auto el = it->second.begin(); el != it->second.end(); ++el) {
                keep |= accumulate(env, *el, &t_it->second, key, sindex_val);
            }
            if (!keep) {
                acc.erase(t_it);
            }
        }
        return should_send_batch() ? done_traversing_t::YES : done_traversing_t::NO;
    }
    virtual bool accumulate(env_t *env,
                            const counted_t<const datum_t> &el,
                            T *t,
                            const store_key_t &key,
                            // sindex_val may be NULL
                            const counted_t<const datum_t> &sindex_val) = 0;

    virtual bool should_send_batch() = 0;

    virtual void finish_impl(result_t *out) {
        *out = grouped_t<T>();
        boost::get<grouped_t<T> >(*out).swap(acc);
        guarantee(acc.size() == 0);
    }

    virtual void unshard(env_t *env,
                         const store_key_t &last_key,
                         const std::vector<result_t *> &results) {
        guarantee(acc.size() == 0);
        std::map<counted_t<const datum_t>, std::vector<T *>, counted_datum_less_t>
            vecs(counted_datum_less_t(env->reql_version));
        for (auto res = results.begin(); res != results.end(); ++res) {
            guarantee(*res);
            grouped_t<T> *gres = boost::get<grouped_t<T> >(*res);
            guarantee(gres);
            // `gres`'s ordering doesn't affect things here because we're putting the
            // values into a parallel map.
            for (auto kv = gres->begin(grouped::order_doesnt_matter_t());
                 kv != gres->end(grouped::order_doesnt_matter_t());
                 ++kv) {
                vecs[kv->first].push_back(&kv->second);
            }
        }
        for (auto kv = vecs.begin(); kv != vecs.end(); ++kv) {
            auto t_it = acc.insert(std::make_pair(kv->first, default_val)).first;
            unshard_impl(env, &t_it->second, last_key, kv->second);
        }
    }
    virtual void unshard_impl(env_t *env,
                              T *acc,
                              const store_key_t &last_key,
                              const std::vector<T *> &ts) = 0;

protected:
    const T *get_default_val() { return &default_val; }
    grouped_t<T> *get_acc() { return &acc; }
private:
    const T default_val;
    grouped_t<T> acc;
};

class append_t : public grouped_acc_t<stream_t> {
public:
    append_t(sorting_t _sorting, batcher_t *_batcher)
        : grouped_acc_t<stream_t>(stream_t()),
          sorting(_sorting), key_le(sorting), batcher(_batcher) { }
private:
    virtual bool should_send_batch() {
        return batcher != NULL && batcher->should_send_batch();
    }
    virtual bool accumulate(env_t *,
                            const counted_t<const datum_t> &el,
                            stream_t *stream,
                            const store_key_t &key,
                            // sindex_val may be NULL
                            const counted_t<const datum_t> &sindex_val) {
        if (batcher) batcher->note_el(el);
        // We don't bother storing the sindex if we aren't sorting (this is
        // purely a performance optimization).
        counted_t<const datum_t> rget_sindex_val = (sorting == sorting_t::UNORDERED)
            ? counted_t<const datum_t>()
            : sindex_val;
        stream->push_back(rget_item_t(store_key_t(key), rget_sindex_val, el));
        return true;
    }

    virtual counted_t<const datum_t> unpack(stream_t *) {
        r_sanity_check(false); // We never unpack a stream.
        unreachable();
    }

    virtual void unshard_impl(env_t *,
                              stream_t *out,
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
                    if (key_le.is_le(item->key, last_key)) {
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
                        if (best == NULL || key_le.is_le(it->first->key, (*best)->key)) {
                            best = &it->first;
                        }
                    }
                }
                if (best == NULL || !key_le.is_le((*best)->key, last_key)) break;
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

scoped_ptr_t<accumulator_t> make_append(const sorting_t &sorting, batcher_t *batcher) {
    return make_scoped<append_t>(sorting, batcher);
}

// This can't be a normal terminal because it wouldn't preserve ordering.
// (Also, I'm sorry for this absurd type hierarchy.)
class to_array_t : public eager_acc_t {
public:
    explicit to_array_t(reql_version_t reql_version)
        : groups(counted_datum_less_t(reql_version)), size(0) { }
private:
    virtual void operator()(env_t *env, groups_t *gs) {
        for (auto kv = gs->begin(); kv != gs->end(); ++kv) {
            datums_t *lst1 = &groups[kv->first];
            datums_t *lst2 = &kv->second;
            size += lst2->size();
            rcheck_toplevel(
                size <= env->limits.array_size_limit(), base_exc_t::GENERIC,
                strprintf("Grouped data over size limit %zu.  "
                          "Try putting a reduction (like `.reduce` or `.count`) "
                          "on the end.", env->limits.array_size_limit()).c_str());
            lst1->reserve(lst1->size() + lst2->size());
            std::move(lst2->begin(), lst2->end(), std::back_inserter(*lst1));
        }
        gs->clear();
    }

    virtual void add_res(env_t *env, result_t *res) {
        grouped_t<stream_t> *streams = boost::get<grouped_t<stream_t> >(res);
        r_sanity_check(streams);

        // The order in which we iterate `streams` doesn't matter here because all
        // the `kv->first` values are unique.
        for (auto kv = streams->begin(grouped::order_doesnt_matter_t());
             kv != streams->end(grouped::order_doesnt_matter_t());
             ++kv) {
            datums_t *lst = &groups[kv->first];
            stream_t *stream = &kv->second;
            size += stream->size();
            rcheck_toplevel(
                size <= env->limits.array_size_limit(), base_exc_t::GENERIC,
                strprintf("Grouped data over size limit %zu.  "
                          "Try putting a reduction (like `.reduce` or `.count`) "
                          "on the end.", env->limits.array_size_limit()).c_str());

            for (auto it = stream->begin(); it != stream->end(); ++it) {
                lst->push_back(std::move(it->data));
            }
        }
    }

    virtual counted_t<val_t> finish_eager(protob_t<const Backtrace> bt,
                                          bool is_grouped,
                                          const configured_limits_t &limits) {
        if (is_grouped) {
            counted_t<grouped_data_t> ret(new grouped_data_t());
            for (auto kv = groups.begin(); kv != groups.end(); ++kv) {
                (*ret)[kv->first] = make_counted<const datum_t>(std::move(kv->second),
                                                               limits);
            }
            return make_counted<val_t>(std::move(ret), bt);
        } else if (groups.size() == 0) {
            return make_counted<val_t>(datum_t::empty_array(), bt);
        } else {
            r_sanity_check(groups.size() == 1 && !groups.begin()->first.has());
            return make_counted<val_t>(
                make_counted<const datum_t>(std::move(groups.begin()->second), limits),
                bt);
        }
    }

    groups_t groups;
    size_t size;
};

scoped_ptr_t<eager_acc_t> make_to_array(reql_version_t reql_version) {
    return make_scoped<to_array_t>(reql_version);
}

template<class T>
class terminal_t : public grouped_acc_t<T>, public eager_acc_t {
protected:
    explicit terminal_t(T &&t) : grouped_acc_t<T>(std::move(t)) { }
private:
    virtual void operator()(env_t *env, groups_t *groups) {
        grouped_t<T> *acc = grouped_acc_t<T>::get_acc();
        const T *default_val = grouped_acc_t<T>::get_default_val();
        for (auto it = groups->begin(); it != groups->end(); ++it) {
            auto pair = acc->insert(std::make_pair(it->first, *default_val));
            auto t_it = pair.first;
            bool keep = !pair.second;
            for (auto el = it->second.begin(); el != it->second.end(); ++el) {
                keep |= accumulate(env, *el, &t_it->second);
            }
            if (!keep) {
                acc->erase(t_it);
            }
        }
        groups->clear();
    }

    virtual counted_t<val_t> finish_eager(protob_t<const Backtrace> bt,
                                          bool is_grouped,
                                          UNUSED const configured_limits_t &limits) {
        accumulator_t::mark_finished();
        grouped_t<T> *acc = grouped_acc_t<T>::get_acc();
        const T *default_val = grouped_acc_t<T>::get_default_val();
        counted_t<val_t> retval;
        if (is_grouped) {
            counted_t<grouped_data_t> ret(new grouped_data_t());
            // The order of `acc` doesn't matter here because we're putting stuff
            // into the parallel map, `ret`.
            for (auto kv = acc->begin(grouped::order_doesnt_matter_t());
                 kv != acc->end(grouped::order_doesnt_matter_t());
                 ++kv) {
                ret->insert(std::make_pair(kv->first, unpack(&kv->second)));
            }
            retval = make_counted<val_t>(std::move(ret), bt);
        } else if (acc->size() == 0) {
            T t(*default_val);
            retval = make_counted<val_t>(unpack(&t), bt);
        } else {
            // Order doesnt' matter here because the size is 1.
            r_sanity_check(acc->size() == 1 &&
                           !acc->begin(grouped::order_doesnt_matter_t())->first.has());
            retval = make_counted<val_t>(unpack(&acc->begin(grouped::order_doesnt_matter_t())->second), bt);
        }
        acc->clear();
        return retval;
    }
    virtual counted_t<const datum_t> unpack(T *t) = 0;

    virtual void add_res(env_t *env, result_t *res) {
        grouped_t<T> *acc = grouped_acc_t<T>::get_acc();
        const T *default_val = grouped_acc_t<T>::get_default_val();
        if (auto e = boost::get<exc_t>(res)) {
            throw *e;
        }
        grouped_t<T> *gres = boost::get<grouped_t<T> >(res);
        r_sanity_check(gres);
        if (acc->size() == 0) {
            acc->swap(*gres);
        } else {
            // Order in fact does NOT matter here.  The reason is, each `kv->first`
            // value is different, which means each operation works on a different
            // key/value pair of `acc`.
            for (auto kv = gres->begin(grouped::order_doesnt_matter_t());
                 kv != gres->end(grouped::order_doesnt_matter_t()); ++kv) {
                auto t_it = acc->insert(std::make_pair(kv->first, *default_val)).first;
                unshard_impl(env, &t_it->second, &kv->second);
            }
        }
    }

    virtual bool accumulate(env_t *env,
                            const counted_t<const datum_t> &el,
                            T *t,
                            const store_key_t &,
                            const counted_t<const datum_t> &) {
        return accumulate(env, el, t);
    }
    virtual bool accumulate(env_t *env,
                            const counted_t<const datum_t> &el,
                            T *t) = 0;

    virtual void unshard_impl(env_t *env, T *out, const store_key_t &, const std::vector<T *> &ts) {
        for (auto it = ts.begin(); it != ts.end(); ++it) {
            unshard_impl(env, out, *it);
        }
    }
    virtual void unshard_impl(env_t *env, T *out, T *el) = 0;
    virtual bool should_send_batch() { return false; }
};

class count_terminal_t : public terminal_t<uint64_t> {
public:
    explicit count_terminal_t(const count_wire_func_t &)
        : terminal_t<uint64_t>(0) { }
private:
    virtual bool uses_val() { return false; }
    virtual bool accumulate(env_t *,
                            const counted_t<const datum_t> &,
                            uint64_t *out) {
        *out += 1;
        return true;
    }
    virtual counted_t<const datum_t> unpack(uint64_t *sz) {
        return make_counted<const datum_t>(static_cast<double>(*sz));
    }
    virtual void unshard_impl(env_t *, uint64_t *out, uint64_t *el) {
        *out += *el;
    }
};

class acc_func_t {
public:
    explicit acc_func_t(const counted_t<func_t> &_f) : f(_f) { }
    counted_t<const datum_t> operator()(env_t *env, const counted_t<const datum_t> &el) const {
        return f.has() ? f->call(env, el)->as_datum() : el;
    }
private:
    counted_t<func_t> f;
};

template<class T>
class skip_terminal_t : public terminal_t<T> {
protected:
    skip_terminal_t(const skip_wire_func_t &wf, T &&t)
        : terminal_t<T>(std::move(t)),
          f(wf.compile_wire_func_or_null()),
          bt(wf.bt.get_bt()) { }
    virtual bool accumulate(env_t *env,
                            const counted_t<const datum_t> &el,
                            T *out) {
        try {
            maybe_acc(env, el, out, f);
            return true;
        } catch (const datum_exc_t &e) {
            if (e.get_type() != base_exc_t::NON_EXISTENCE) {
                throw exc_t(e, bt.get());
            }
        } catch (const exc_t &e2) {
            if (e2.get_type() != base_exc_t::NON_EXISTENCE) {
                throw e2;
            }
        }
        return false;
    }
private:
    virtual void maybe_acc(env_t *env,
                           const counted_t<const datum_t> &el,
                           T *out,
                           const acc_func_t &f) = 0;

    acc_func_t f;
    protob_t<const Backtrace> bt;
};

class sum_terminal_t : public skip_terminal_t<double> {
public:
    explicit sum_terminal_t(const sum_wire_func_t &f)
        : skip_terminal_t<double>(f, 0.0L) { }
private:
    virtual void maybe_acc(env_t *env,
                           const counted_t<const datum_t> &el,
                           double *out,
                           const acc_func_t &f) {
        *out += f(env, el)->as_num();
    }
    virtual counted_t<const datum_t> unpack(double *d) {
        return make_counted<const datum_t>(*d);
    }
    virtual void unshard_impl(env_t *, double *out, double *el) {
        *out += *el;
    }
};

class avg_terminal_t : public skip_terminal_t<std::pair<double, uint64_t> > {
public:
    explicit avg_terminal_t(const avg_wire_func_t &f)
        : skip_terminal_t<std::pair<double, uint64_t> >(
            f, std::make_pair(0.0L, 0ULL)) { }
private:
    virtual void maybe_acc(env_t *env,
                           const counted_t<const datum_t> &el,
                           std::pair<double, uint64_t> *out,
                           const acc_func_t &f) {
        out->first += f(env, el)->as_num();
        out->second += 1;
    }
    virtual counted_t<const datum_t> unpack(
        std::pair<double, uint64_t> *p) {
        rcheck_datum(p->second != 0, base_exc_t::NON_EXISTENCE,
                     "Cannot take the average of an empty stream.  (If you passed "
                     "`avg` a field name, it may be that no elements of the stream "
                     "had that field.)");
        return make_counted<const datum_t>(p->first / p->second);
    }
    virtual void unshard_impl(env_t *,
                              std::pair<double, uint64_t> *out,
                              std::pair<double, uint64_t> *el) {
        out->first += el->first;
        out->second += el->second;
    }
};

optimizer_t::optimizer_t() { }
optimizer_t::optimizer_t(const counted_t<const datum_t> &_row,
                         const counted_t<const datum_t> &_val)
    : row(_row), val(_val) { }
void optimizer_t::swap_if_other_better(
    optimizer_t &other, // NOLINT
    reql_version_t reql_version,
    bool (*beats)(reql_version_t reql_version,
                  const counted_t<const datum_t> &val1,
                  const counted_t<const datum_t> &val2)) {
    r_sanity_check(val.has() == row.has());
    r_sanity_check(other.val.has() == other.row.has());
    if (other.val.has()) {
        if (!val.has() || beats(reql_version, other.val, val)) {
            row.swap(other.row);
            val.swap(other.val);
        }
    }
}
counted_t<const datum_t> optimizer_t::unpack(const char *name) {
    r_sanity_check(val.has() == row.has());
    rcheck_datum(
        row.has(), base_exc_t::NON_EXISTENCE,
        strprintf(
            "Cannot take the %s of an empty stream.  (If you passed `%s` a field name, "
            "it may be that no elements of the stream had that field.)",
            name,
            name));
    return row;
}

bool datum_lt(reql_version_t reql_version,
              const counted_t<const datum_t> &val1,
              const counted_t<const datum_t> &val2) {
    r_sanity_check(val1.has() && val2.has());
    return val1->compare_lt(reql_version, *val2);
}

bool datum_gt(reql_version_t reql_version,
              const counted_t<const datum_t> &val1,
              const counted_t<const datum_t> &val2) {
    r_sanity_check(val1.has() && val2.has());
    return val1->compare_gt(reql_version, *val2);
}

class optimizing_terminal_t : public skip_terminal_t<optimizer_t> {
public:
    optimizing_terminal_t(const skip_wire_func_t &f,
                          const char *_name,
                          bool (*_cmp)(reql_version_t,
                                       const counted_t<const datum_t> &val1,
                                       const counted_t<const datum_t> &val2))
        : skip_terminal_t<optimizer_t>(f, optimizer_t()),
          name(_name),
          cmp(_cmp) { }
private:
    virtual void maybe_acc(env_t *env,
                           const counted_t<const datum_t> &el,
                           optimizer_t *out,
                           const acc_func_t &f) {
        optimizer_t other(el, f(env, el));
        out->swap_if_other_better(other, env->reql_version, cmp);
    }
    virtual counted_t<const datum_t> unpack(optimizer_t *el) {
        return el->unpack(name);
    }
    virtual void unshard_impl(env_t *env, optimizer_t *out, optimizer_t *el) {
        out->swap_if_other_better(*el, env->reql_version, cmp);
    }
    const char *name;
    bool (*cmp)(reql_version_t,
                const counted_t<const datum_t> &val1,
                const counted_t<const datum_t> &val2);
};

const char *const empty_stream_msg =
    "Cannot reduce over an empty stream.";

class reduce_terminal_t : public terminal_t<counted_t<const datum_t> > {
public:
    explicit reduce_terminal_t(const reduce_wire_func_t &_f)
        : terminal_t<counted_t<const datum_t> >(counted_t<const datum_t>()),
          f(_f.compile_wire_func()) { }
private:
    virtual bool accumulate(env_t *env,
                            const counted_t<const datum_t> &el,
                            counted_t<const datum_t> *out) {
        try {
            *out = out->has() ? f->call(env, *out, el)->as_datum() : el;
            return true;
        } catch (const datum_exc_t &e) {
            throw exc_t(e, f->backtrace().get(), 1);
        }
    }
    virtual counted_t<const datum_t> unpack(counted_t<const datum_t> *el) {
        rcheck_target(f, base_exc_t::NON_EXISTENCE, el->has(), empty_stream_msg);
        return std::move(*el);
    }
    virtual void unshard_impl(env_t *env,
                              counted_t<const datum_t> *out,
                              counted_t<const datum_t> *el) {
        if (el->has()) accumulate(env, *el, out);
    }

    counted_t<func_t> f;
};

template<class T>
class terminal_visitor_t : public boost::static_visitor<T *> {
public:
    T *operator()(const count_wire_func_t &f) const {
        return new count_terminal_t(f);
    }
    T *operator()(const sum_wire_func_t &f) const {
        return new sum_terminal_t(f);
    }
    T *operator()(const avg_wire_func_t &f) const {
        return new avg_terminal_t(f);
    }
    T *operator()(const min_wire_func_t &f) const {
        return new optimizing_terminal_t(f, "min", datum_lt);
    }
    T *operator()(const max_wire_func_t &f) const {
        return new optimizing_terminal_t(f, "max", datum_gt);
    }
    T *operator()(const reduce_wire_func_t &f) const {
        return new reduce_terminal_t(f);
    }
};

scoped_ptr_t<accumulator_t> make_terminal(const terminal_variant_t &t) {
    return scoped_ptr_t<accumulator_t>(
            boost::apply_visitor(terminal_visitor_t<accumulator_t>(), t));
}

scoped_ptr_t<eager_acc_t> make_eager_terminal(const terminal_variant_t &t) {
    return scoped_ptr_t<eager_acc_t>(
            boost::apply_visitor(terminal_visitor_t<eager_acc_t>(), t));
}

class ungrouped_op_t : public op_t {
protected:
private:
    virtual void operator()(
        env_t *env, groups_t *groups, const counted_t<const datum_t> &sindex_val) {
        for (auto it = groups->begin(); it != groups->end();) {
            lst_transform(env, &it->second, sindex_val);
            if (it->second.size() == 0) {
                groups->erase(it++); // This is important for batching with filter.
            } else {
                ++it;
            }
        }
    }
    // sindex_val may be NULL.
    virtual void lst_transform(
        env_t *env, datums_t *lst, const counted_t<const datum_t> &sindex_val) = 0;
};

class group_trans_t : public op_t {
public:
    explicit group_trans_t(const group_wire_func_t &f)
        : funcs(f.compile_funcs()),
          append_index(f.should_append_index()),
          multi(f.is_multi()),
          bt(f.get_bt()) {
        r_sanity_check((funcs.size() + append_index) != 0);
    }
private:
    virtual void operator()(env_t *env,
                            groups_t *groups,
                            const counted_t<const datum_t> &sindex_val) {
        if (groups->size() == 0) return;
        r_sanity_check(groups->size() == 1 && !groups->begin()->first.has());
        datums_t *ds = &groups->begin()->second;
        for (auto el = ds->begin(); el != ds->end(); ++el) {
            std::vector<counted_t<const datum_t> > arr;
            arr.reserve(funcs.size() + append_index);
            for (auto f = funcs.begin(); f != funcs.end(); ++f) {
                try {
                    try {
                        arr.push_back((*f)->call(env, *el)->as_datum());
                    } catch (const base_exc_t &e) {
                        if (e.get_type() == base_exc_t::NON_EXISTENCE) {
                            arr.push_back(datum_t::null());
                        } else {
                            throw;
                        }
                    }
                } catch (const datum_exc_t &e) {
                    throw exc_t(e, (*f)->backtrace().get(), 1);
                }
            }
            if (append_index) {
                r_sanity_check(sindex_val.has());
                arr.push_back(sindex_val);
            }
            r_sanity_check(arr.size() == (funcs.size() + append_index));

            if (!multi) {
                add(groups, std::move(arr), *el, env->limits);
            } else {
                std::vector<std::vector<counted_t<const datum_t> > > perms(arr.size());
                for (size_t i = 0; i < arr.size(); ++i) {
                    if (arr[i]->get_type() != datum_t::R_ARRAY) {
                        perms[i].push_back(arr[i]);
                    } else {
                        auto subarr = arr[i]->as_array();
                        perms[i].reserve(subarr.size());
                        for (auto it = subarr.begin(); it != subarr.end(); ++it) {
                            perms[i].push_back(*it);
                        }
                    }
                }
                std::vector<counted_t<const datum_t> > instance;
                instance.reserve(perms.size());
                add_perms(groups, &instance, &perms, 0, *el, env->limits);
                r_sanity_check(instance.size() == 0);
            }

            rcheck_src(
                bt.get(), base_exc_t::GENERIC,
                groups->size() <= env->limits.array_size_limit(),
                strprintf("Too many groups (> %zu).", env->limits.array_size_limit()));
        }
        size_t erased = groups->erase(counted_t<const datum_t>());
        r_sanity_check(erased == 1);
    }

    void add(groups_t *groups,
             std::vector<counted_t<const datum_t> > &&arr,
             const counted_t<const datum_t> &el,
             const configured_limits_t &limits) {
        counted_t<const datum_t> group = arr.size() == 1
            ? std::move(arr[0])
            : make_counted<const datum_t>(std::move(arr), limits);
        r_sanity_check(group.has());
        (*groups)[group].push_back(el);
    }

    void add_perms(groups_t *groups,
                   std::vector<counted_t<const datum_t> > *instance,
                   std::vector<std::vector<counted_t<const datum_t> > > *arr,
                   size_t index,
                   const counted_t<const datum_t> &el,
                   const configured_limits_t &limits) {
        r_sanity_check(index == instance->size());
        if (index >= arr->size()) {
            r_sanity_check(instance->size() == arr->size());
            add(groups, std::vector<counted_t<const datum_t> >(*instance), el, limits);
        } else {
            auto vec = (*arr)[index];
            if (vec.size() != 0) {
                for (auto it = vec.begin(); it != vec.end(); ++it) {
                    instance->push_back(std::move(*it));
                    add_perms(groups, instance, arr, index + 1, el, limits);
                    instance->pop_back();
                }
            } else {
                instance->push_back(datum_t::null());
                add_perms(groups, instance, arr, index + 1, el, limits);
                instance->pop_back();
            }
        }
    }

    std::vector<counted_t<func_t> > funcs;
    bool append_index, multi;
    protob_t<const Backtrace> bt;
};

class map_trans_t : public ungrouped_op_t {
public:
    explicit map_trans_t(const map_wire_func_t &_f)
        : f(_f.compile_wire_func()) { }
private:
    virtual void lst_transform(
        env_t *env, datums_t *lst, const counted_t<const datum_t> &) {
        try {
            for (auto it = lst->begin(); it != lst->end(); ++it) {
                *it = f->call(env, *it)->as_datum();
            }
        } catch (const datum_exc_t &e) {
            throw exc_t(e, f->backtrace().get(), 1);
        }
    }
    counted_t<func_t> f;
};

// Note: this removes duplicates ONLY TO SAVE NETWORK TRAFFIC.  It's possible
// for duplicates to survive, either because they're on different shards or
// because they span batch boundaries.  `ordered_distinct_datum_stream_t` in
// `datum_stream.cc` removes any duplicates that survive this `lst_transform`.
class distinct_trans_t : public ungrouped_op_t {
public:
    explicit distinct_trans_t(const distinct_wire_func_t &f) : use_index(f.use_index) { }
private:
    // sindex_val may be NULL
    virtual void lst_transform(
        env_t *, datums_t *lst, const counted_t<const datum_t> &sindex_val) {
        auto it = lst->begin();
        auto loc = it;
        for (; it != lst->end(); ++it) {
            if (use_index) {
                r_sanity_check(sindex_val.has());
                *it = sindex_val;
            }
            if (!last_val.has() || **it != *last_val) {
                loc->swap(*it);
                last_val = *loc;
                ++loc;
            }
        }
        lst->erase(loc, lst->end());
    }
    bool use_index;
    counted_t<const datum_t> last_val;
};


class filter_trans_t : public ungrouped_op_t {
public:
    explicit filter_trans_t(const filter_wire_func_t &_f)
        : f(_f.filter_func.compile_wire_func()),
          default_val(_f.default_filter_val
                      ? _f.default_filter_val->compile_wire_func()
                      : counted_t<func_t>()) { }
private:
    virtual void lst_transform(
        env_t *env, datums_t *lst, const counted_t<const datum_t> &) {
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
    counted_t<func_t> f, default_val;
};

class concatmap_trans_t : public ungrouped_op_t {
public:
    explicit concatmap_trans_t(const concatmap_wire_func_t &_f)
        : f(_f.compile_wire_func()) { }
private:
    virtual void lst_transform(
        env_t *env, datums_t *lst, const counted_t<const datum_t> &) {
        datums_t new_lst;
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
    counted_t<func_t> f;
};

class zip_trans_t : public ungrouped_op_t {
public:
    explicit zip_trans_t(const zip_wire_func_t &) {}
private:
    virtual void lst_transform(env_t *, datums_t *lst,
                               const counted_t<const datum_t> &) {
        for (auto it = lst->begin(); it != lst->end(); ++it) {
            auto left = (*it)->get("left", NOTHROW);
            auto right = (*it)->get("right", NOTHROW);
            rcheck_datum(left.has(), base_exc_t::GENERIC,
                   "ZIP can only be called on the result of a join.");
            *it = right.has() ? left->merge(right) : left;
        }
    }
};

class transform_visitor_t : public boost::static_visitor<op_t *> {
public:
    explicit transform_visitor_t() { }
    op_t *operator()(const map_wire_func_t &f) const {
        return new map_trans_t(f);
    }
    op_t *operator()(const group_wire_func_t &f) const {
        return new group_trans_t(f);
    }
    op_t *operator()(const filter_wire_func_t &f) const {
        return new filter_trans_t(f);
    }
    op_t *operator()(const concatmap_wire_func_t &f) const {
        return new concatmap_trans_t(f);
    }
    op_t *operator()(const distinct_wire_func_t &f) const {
        return new distinct_trans_t(f);
    }
    op_t *operator()(const zip_wire_func_t &f) const {
        return new zip_trans_t(f);
    }
};

scoped_ptr_t<op_t> make_op(const transform_variant_t &tv) {
    return scoped_ptr_t<op_t>(boost::apply_visitor(transform_visitor_t(), tv));
}

RDB_IMPL_ME_SERIALIZABLE_3_SINCE_v1_13(rget_item_t, key, empty_ok(sindex_key), data);

} // namespace ql
