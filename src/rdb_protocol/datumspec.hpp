// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_DATUMSPEC_HPP_
#define RDB_PROTOCOL_DATUMSPEC_HPP_

#include "rdb_protocol/datum.hpp"

namespace ql {
class datum_range_t {
public:
    datum_range_t();
    datum_range_t(
        datum_t left_bound,
        key_range_t::bound_t left_bound_type,
        datum_t right_bound,
        key_range_t::bound_t right_bound_type);
    // Range that includes just one value.
    explicit datum_range_t(datum_t val);
    static datum_range_t universe();

    bool operator<(const datum_range_t &o) const;

    bool contains(datum_t val) const;
    bool is_empty() const;
    bool is_universe() const;

    RDB_DECLARE_ME_SERIALIZABLE(datum_range_t);

    // Make sure you know what you're doing if you call these, and think about
    // truncated sindexes.
    key_range_t to_primary_keyrange() const;
    key_range_t to_sindex_keyrange(skey_version_t skey_version) const;

    datum_range_t with_left_bound(datum_t d, key_range_t::bound_t type);
    datum_range_t with_right_bound(datum_t d, key_range_t::bound_t type);

    std::string print() const {
        return strprintf("%c%s,%s%c",
                         left_bound_type == key_range_t::open ? '(' : '[',
                         left_bound.print().c_str(),
                         right_bound.print().c_str(),
                         right_bound_type == key_range_t::open ? ')' : ']');
    }
private:
    friend class info_term_t;
    datum_t left_bound, right_bound;
    key_range_t::bound_t left_bound_type, right_bound_type;
};
void debug_print(printf_buffer_t *buf, const datum_range_t &rng);

template<class T>
class ds_helper_t : public boost::static_visitor<T> {
public:
    ds_helper_t(std::function<T(const datum_range_t &)> _f1,
                std::function<T(const std::map<datum_t, size_t> &)> _f2)
        : f1(std::move(_f1)), f2(std::move(_f2)) { }
    T operator()(const datum_range_t &dr) const { return f1(dr); }
    T operator()(const std::map<datum_t, size_t> &m) const { return f2(m); }
private:
    std::function<T(const datum_range_t &)> f1;
    std::function<T(const std::map<datum_t, size_t> &)> f2;
};

struct datumspec_t {
public:
    template<class T>
    explicit datumspec_t(T t) : spec(std::move(t)) { }

    // Only use this for serialization!
    datumspec_t() { }

    template<class T>
    T visit(std::function<T(const datum_range_t &)> f1,
            std::function<T(const std::map<datum_t, size_t> &)> f2) const {
        return boost::apply_visitor(ds_helper_t<T>(std::move(f1), std::move(f2)), spec);
    }

    datumspec_t trim_secondary(const key_range_t &rng, skey_version_t ver) const {
        return boost::apply_visitor(
            ds_helper_t<datumspec_t>(
                [](const datum_range_t &dr) { return datumspec_t(dr); },
                [&rng, &ver](const std::map<datum_t, size_t> &m) {
                    std::map<datum_t, size_t> ret;
                    for (const auto &pair : m) {
                        if (rng.overlaps(
                                datum_range_t(pair.first).to_sindex_keyrange(ver))) {
                            ret.insert(pair);
                        }
                    }
                    return datumspec_t(std::move(ret));
                }),
            spec);
    }

    template<class T>
    void iter(sorting_t sorting, const T &cb) const {
        return boost::apply_visitor(
            ds_helper_t<void>(
                [&cb](const datum_range_t &dr) {
                    cb(std::make_pair(dr, 1), true);
                },
                [sorting, &cb](const std::map<datum_t, size_t> &m) {
                    if (!reversed(sorting)) {
                        for (auto it = m.begin(); it != m.end();) {
                            auto this_it = it++;
                            if (cb(*this_it, it == m.end())) {
                                break;
                            }
                        }
                    } else {
                        for (auto it = m.rbegin(); it != m.rend();) {
                            auto this_it = it++;
                            if (cb(*this_it, it == m.rend())) {
                                break;
                            }
                        }
                    }
                }),
            spec);
    }

    bool is_universe() const {
        return boost::apply_visitor(
            ds_helper_t<bool>(
                [](const datum_range_t &dr) { return dr.is_universe(); },
                [](const std::map<datum_t, size_t> &) { return false; }),
            spec);
    }
    bool is_empty() const {
        return boost::apply_visitor(
            ds_helper_t<bool>(
                [](const datum_range_t &dr) { return dr.is_empty(); },
                [](const std::map<datum_t, size_t> &m) { return m.size() == 0; }),
            spec);
    }
    // Try to only call this once since it does work to compute it.
    datum_range_t covering_range() const {
        return boost::apply_visitor(
            ds_helper_t<datum_range_t>(
                [](const datum_range_t &dr) { return dr; },
                [](const std::map<datum_t, size_t> &m) {
                    datum_t min = datum_t::maxval(), max = datum_t::minval();
                    for (const auto &pair : m) {
                        if (pair.first < min) min = pair.first;
                        if (pair.first > max) max = pair.first;
                    }
                    return datum_range_t(min, key_range_t::closed,
                                         max, key_range_t::closed);
                }),
            spec);
    }
    size_t copies(datum_t key) const {
        return boost::apply_visitor(
            ds_helper_t<size_t>(
                [&key](const datum_range_t &dr) { return dr.contains(key) ? 1 : 0; },
                [&key](const std::map<datum_t, size_t> &m) {
                    auto it = m.find(key);
                    return it != m.end() ? it->second : 0;
                }),
            spec);
    }
    boost::optional<std::map<store_key_t, size_t> > primary_key_map() const {
        return boost::apply_visitor(
            ds_helper_t<boost::optional<std::map<store_key_t, size_t> > >(
                [](const datum_range_t &) { return boost::none; },
                [](const std::map<datum_t, size_t> &m) {
                    std::map<store_key_t, size_t> ret;
                    for (const auto &pair : m) {
                        ret[store_key_t(pair.first.print_primary())] = pair.second;
                    }
                    return ret;
                }),
            spec);
    }

    RDB_DECLARE_ME_SERIALIZABLE(datumspec_t);
private:
    boost::variant<datum_range_t, std::map<datum_t, size_t> > spec;
};

} // namespace ql

#endif // RDB_PROTOCOL_DATUMSPEC_HPP_

