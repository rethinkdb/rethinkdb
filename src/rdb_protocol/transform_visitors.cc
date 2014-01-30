// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "rdb_protocol/transform_visitors.hpp"

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
                              const store_key_t &key,
                              // sindex_val may be NULL
                              const counted_t<const datum_t> &sindex_val) {
        for (auto it = groups.begin(); it != groups.end(); ++it) {
            // If there's already an entry, this insert won't do anything.
            auto t_it = acc.insert(std::make_pair(it.first, default_t)).first;
            for (auto el = it.second.begin(); el != it.second.end(); ++el) {
                accumulate(*el, &*t_it, key, sindex_val);
            }
        }
        return should_send_batch() ? done_t::YES : done_t::NO;
    }
    virtual bool should_send_batch() = 0;
    virtual void finish_impl(result_t *out, const batcher_t &) {
        if (T *t = get_ungrouped_val()) {
            *out = res_t(default_t);
            T *t_out = boost::get<T>(boost::get<res_t>(out));
            *t_out = std::move(*t);
        } else {
            *out = grouped_res_t();
            for (auto it = acc.begin(); it != acc.end(); ++it) {
                r_sanity_check(it->first.has());
                T *t_out = boost::get<T>(&(*boost::get<grouped_res_t>(out))[it->first]);
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
                            const store_key_t &key,
                            // sindex_val may be NULL
                            const counted_t<const datum_t> &sindex_val) = 0;

    virtual void unshard(const res_groups_t &rgs) {
        for (auto it = rgs.begin(); it != rgs.end(); ++it) {
            auto t_it = acc.insert(std::make_pair(it.first, default_t)).first;
            std::vector<T *> result_ts;
            result_ts.reserve(results.size());
            for (auto el = it.second.begin(); el != it.second.end(); ++el) {
                result_ts.push_back(boost::get<T>(&*el));
            }
            unshard_impl(&*t_it, result_ts);
        }
    }
    virtual void unshard_impl(T *acc, const std::vector<T *> &ts) = 0;

    const T default_t;
    std::map<counted_t<const ql::datum_t>, T> acc;
};

class append_t : public grouped_accumulator_t<stream_t> {
public:
    append_t(sorting_t _sorting, const batcher_t *_batcher)
        : terminal_t(stream_t()), sorting(_sorting), batcher(_batcher) { }
private:
    virtual void finish_impl(result_t *out) {
        grouped_accumulator_t<stream_t>::finish_impl(out);
        // RSI: if batcher == NULL, what should this be?
        out->truncated = should_send_batch() || seen_truncated();
    }
    virtual bool should_send_batch() {
        return batcher != NULL && batcher->should_send_batch();
    }
    virtual void accumulate(const counted_t<const ql::datum_t> &el,
                            stream_t *stream,
                            const store_key_t &key,
                            // sindex_val may be NULL
                            const counted_t<const datum_t> &sindex_val) {
        if (batcher) batcher->note_el(el);
        stream->push_back((sorting != UNORDERED && sindex_val)
                          ? rget_item_t(*key, sindex_val, el)
                          : rget_item_t(*key, el));
    }
    virtual void unshard_impl(stream_t *out,
                              const store_key_t &last_key,
                              const std::vector<stream_t *> &streams) {
        size_t sz = 0;
        for (auto it = streams.begin(); it != streams.end(); ++streams) {
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
    bool key_le(const store_key_t &key1, const store_key_t &key2) const {
        return (!reversed(sorting) && key1 <= key2)
            || (reversed(sorting) && key2 <= key1);
    }
    const key_le_t key_le;
    const sorting_t sorting;
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
        const store_key_t &, const counted_t<const datum_t> &, sorting_t) {
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
    virtual void accumulate(const counted_t<const ql::datum_t> &, size_t *out) {
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

// RSI: finalize these datum maps?
class gmr_terminal_t : public terminal_t<ql::wire_datum_map_t> {
public:
    gmr_terminal_t(ql::env_t *env, const gmr_wire_func_t &gmr)
        : terminal_t(ql::wire_datum_map_t()),
          env(_env),
          f_group(gmr.compile_group()),
          f_map(gmr.compile_map()),
          f_reduce(gmr.compile_reduce()) { }
private:
    virtual void accumulate(const counted_t<const ql::datum_t> &el,
                            ql::wire_datum_map_t *out) {
        try {
            auto key = f_group->call(env, el)->as_datum();
        } catch (const ql::datum_exc_t &e) {
            throw ql::exc_t(e, f_group->backtrace().get(), 1);
        }
        try {
            auto val = f_map->call(env, el)->as_datum();
        } catch (const ql::datum_exc_t &e) {
            throw ql::exc_t(e, f_map->backtrace().get(), 1);
        }
        if (!out->has(key)) {
            out->set(std::move(key), std::move(val));
        } else {
            try {
                out->set(std::move(key),
                         f_reduce->call(env, out->get(key), val)->as_datum());
            } catch (const ql::datum_exc_t &e) {
                throw ql::exc_t(e, f_reduce->backtrace().get(), 1);
            }
        }
    }
    virtual void unshard_impl(ql::wire_datum_map_t *out,
                              const std::vector<ql::wire_datum_map_t *> &maps) {
        for (auto it = maps.begin(); it != maps.end(); ++it) {
            RSI just dump GMR, it's not worth it
        }
    }

    ql::env_t *env;
    counted_t<ql::func_t> f_group, f_map, f_reduce;
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
    accumulator_t *operator()(const gmr_wire_func_t &f) const {
        return new gmr_terminal_t(env, f);
    }
    ql::env_t *env;
};

accumulator_t *make_terminal(const terminal_t &t) {
    boost::apply_visitor(terminal_visitor_t(env), t);
}
