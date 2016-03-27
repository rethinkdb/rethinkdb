// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_DATUMSPEC_HPP_
#define RDB_PROTOCOL_DATUMSPEC_HPP_

#include "errors.hpp"
#include <boost/variant.hpp>

#include "btree/types.hpp"
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
    key_range_t to_sindex_keyrange(reql_version_t reql_version) const;

    // Computes the truncated keys corresponding to `left_bound`/`right_bound`
    // respectively.
    std::string get_left_bound_trunc_key(reql_version_t ver) const;
    std::string get_right_bound_trunc_key(reql_version_t ver) const;

    datum_range_t with_left_bound(datum_t d, key_range_t::bound_t type);
    datum_range_t with_right_bound(datum_t d, key_range_t::bound_t type);

    std::string print() const {
        return strprintf("%c%s,%s%c",
                         left_bound_type == key_range_t::open ? '(' : '[',
                         left_bound.print().c_str(),
                         right_bound.print().c_str(),
                         right_bound_type == key_range_t::open ? ')' : ']');
    }

    key_range_t::bound_t left_bound_type, right_bound_type;

private:
    friend class info_term_t;
    datum_t left_bound, right_bound;
};

void debug_print(printf_buffer_t *buf, const datum_range_t &rng);

template<class T>
class ds_helper_t : public boost::static_visitor<T> {
public:
    ds_helper_t(std::function<T(const datum_range_t &)> _f1,
                std::function<T(const std::map<datum_t, uint64_t> &)> _f2)
        : f1(std::move(_f1)), f2(std::move(_f2)) { }
    T operator()(const datum_range_t &dr) const { return f1(dr); }
    T operator()(const std::map<datum_t, uint64_t> &m) const { return f2(m); }
private:
    std::function<T(const datum_range_t &)> f1;
    std::function<T(const std::map<datum_t, uint64_t> &)> f2;
};

struct datumspec_t {
public:
    template<class T>
    explicit datumspec_t(T t) : spec(std::move(t)) { }

    // Only use this for serialization!
    datumspec_t() { }

    template<class T>
    T visit(std::function<T(const datum_range_t &)> f1,
            std::function<T(const std::map<datum_t, uint64_t> &)> f2) const {
        return boost::apply_visitor(ds_helper_t<T>(std::move(f1), std::move(f2)), spec);
    }

    continue_bool_t iter(sorting_t sorting,
                         const std::function<continue_bool_t(
                             const std::pair<ql::datum_range_t, uint64_t> &,
                             bool)> &cb) const {
        return boost::apply_visitor(
            ds_helper_t<continue_bool_t>(
                [&cb](const datum_range_t &dr) {
                    return cb(std::make_pair(dr, 1), true);
                },
                [sorting, &cb](const std::map<datum_t, uint64_t> &m) {
                    if (!reversed(sorting)) {
                        for (auto it = m.begin(); it != m.end();) {
                            auto this_it = it++;
                            if (cb(std::make_pair(datum_range_t(this_it->first),
                                                  this_it->second),
                                   it == m.end())
                                == continue_bool_t::ABORT) {
                                return continue_bool_t::ABORT;
                            }
                        }
                    } else {
                        for (auto it = m.rbegin(); it != m.rend();) {
                            auto this_it = it++;
                            if (cb(std::make_pair(datum_range_t(this_it->first),
                                                  this_it->second),
                                   it == m.rend())
                                == continue_bool_t::ABORT) {
                                return continue_bool_t::ABORT;
                            }
                        }
                    }
                    return continue_bool_t::CONTINUE;
                }),
            spec);
    }

    datumspec_t trim_secondary(const key_range_t &rng, reql_version_t ver) const;
    bool is_universe() const;
    bool is_empty() const;
    // Try to only call this once since it does work to compute it.
    datum_range_t covering_range() const;
    size_t copies(datum_t key) const;
    boost::optional<std::map<store_key_t, uint64_t> > primary_key_map() const;

    RDB_DECLARE_ME_SERIALIZABLE(datumspec_t);
private:
    boost::variant<datum_range_t, std::map<datum_t, uint64_t> > spec;
};

} // namespace ql

#endif // RDB_PROTOCOL_DATUMSPEC_HPP_

