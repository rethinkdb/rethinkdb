// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "rdb_protocol/datumspec.hpp"

#include "containers/archive/boost_types.hpp"
#include "containers/archive/stl_types.hpp"
#include "containers/object_buffer.hpp"
#include "rdb_protocol/protocol.hpp"

namespace ql {

void debug_print(printf_buffer_t *buf, const datum_range_t &rng) {
    buf->appendf("datum_range_t(%s)", rng.print().c_str());
}

datum_range_t::datum_range_t()
    : left_bound_type(key_range_t::none), right_bound_type(key_range_t::none) { }

datum_range_t::datum_range_t(
    datum_t _left_bound, key_range_t::bound_t _left_bound_type,
    datum_t _right_bound, key_range_t::bound_t _right_bound_type)
    : left_bound(_left_bound), right_bound(_right_bound),
      left_bound_type(_left_bound_type), right_bound_type(_right_bound_type) {
    r_sanity_check(left_bound.has() && right_bound.has());
}

datum_range_t::datum_range_t(datum_t val)
    : left_bound(val), right_bound(val),
      left_bound_type(key_range_t::closed), right_bound_type(key_range_t::closed) {
    r_sanity_check(val.has());
}

datum_range_t datum_range_t::universe() {
    return datum_range_t(datum_t::minval(), key_range_t::open,
                         datum_t::maxval(), key_range_t::open);
}

bool bound_lt(key_range_t::bound_t a_bound, const datum_t &a,
              key_range_t::bound_t b_bound, const datum_t &b) {
    if (a < b) return true;
    if (a > b) return false;
    guarantee(a_bound != key_range_t::none);
    guarantee(b_bound != key_range_t::none);
    return a_bound == key_range_t::closed && b_bound == key_range_t::open;
}

bool datum_range_t::operator<(const datum_range_t &o) const {
    if (bound_lt(left_bound_type, left_bound, o.left_bound_type, o.left_bound)) {
        return true;
    }
    if (bound_lt(o.left_bound_type, o.left_bound, left_bound_type, left_bound)) {
        return false;
    }
    return bound_lt(right_bound_type, right_bound, o.right_bound_type, o.right_bound);
}

bool datum_range_t::contains(datum_t val) const {
    r_sanity_check(left_bound.has() && right_bound.has());

    int left_cmp = left_bound.cmp(val);
    int right_cmp = right_bound.cmp(val);
    return (left_cmp < 0
            || (left_cmp == 0 && left_bound_type == key_range_t::closed)) &&
           (right_cmp > 0
            || (right_cmp == 0 && right_bound_type == key_range_t::closed));
}

bool datum_range_t::is_empty() const {
    r_sanity_check(left_bound.has() && right_bound.has());

    int cmp = left_bound.cmp(right_bound);
    return (cmp > 0 ||
            ((left_bound_type == key_range_t::open ||
              right_bound_type == key_range_t::open) && cmp == 0));
}

bool datum_range_t::is_universe() const {
    r_sanity_check(left_bound.has() && right_bound.has());
    return left_bound.get_type() == datum_t::type_t::MINVAL &&
           left_bound_type == key_range_t::open &&
           right_bound.get_type() == datum_t::type_t::MAXVAL &&
           right_bound_type == key_range_t::open;
}

key_range_t datum_range_t::to_primary_keyrange() const {
    r_sanity_check(left_bound.has() && right_bound.has());

    std::string lb_str = left_bound.print_primary_internal();
    if (lb_str.size() > MAX_KEY_SIZE) {
        lb_str.erase(MAX_KEY_SIZE);
    }

    std::string rb_str = right_bound.print_primary_internal();
    if (rb_str.size() > MAX_KEY_SIZE) {
        rb_str.erase(MAX_KEY_SIZE);
    }

    return key_range_t(left_bound_type, store_key_t(lb_str),
                       right_bound_type, store_key_t(rb_str));
}

key_range_t datum_range_t::to_sindex_keyrange(skey_version_t skey_version) const {
    r_sanity_check(left_bound.has() && right_bound.has());
    object_buffer_t<store_key_t> lb, rb;
    return rdb_protocol::sindex_key_range(
        store_key_t(left_bound.truncated_secondary(skey_version, extrema_ok_t::OK)),
        store_key_t(right_bound.truncated_secondary(skey_version, extrema_ok_t::OK)));
}

datum_range_t datum_range_t::with_left_bound(datum_t d, key_range_t::bound_t type) {
    r_sanity_check(d.has() && right_bound.has());
    return datum_range_t(d, type, right_bound, right_bound_type);
}

datum_range_t datum_range_t::with_right_bound(datum_t d, key_range_t::bound_t type) {
    r_sanity_check(left_bound.has() && d.has());
    return datum_range_t(left_bound, left_bound_type, d, type);
}


ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(key_range_t::bound_t, int8_t,
                                      key_range_t::open, key_range_t::none);
RDB_IMPL_SERIALIZABLE_4(
        datum_range_t, left_bound, right_bound, left_bound_type, right_bound_type);
INSTANTIATE_SERIALIZABLE_FOR_CLUSTER(datum_range_t);

RDB_IMPL_SERIALIZABLE_1(datumspec_t, spec);
INSTANTIATE_SERIALIZABLE_FOR_CLUSTER(datumspec_t);
} // namespace ql
