// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "btree/backfill.hpp"

#include "containers/archive/boost_types.hpp"
#include "containers/archive/stl_types.hpp"

RDB_IMPL_SERIALIZABLE_1_FOR_CLUSTER(backfill_pre_atom_t, range);
RDB_IMPL_SERIALIZABLE_3_FOR_CLUSTER(backfill_atom_t::pair_t, key, recency, value);
RDB_IMPL_SERIALIZABLE_2_FOR_CLUSTER(backfill_atom_t, range, pairs);

void backfill_atom_t::mask_in_place(const key_range_t &m) {
    range = range.intersection(m);
    std::vector<pair_t> new_pairs;
    for (auto &&pair : pairs) {
        if (m.contains_key(pair.key)) {
            new_pairs.push_back(std::move(pair));
        }
    }
    pairs = std::move(new_pairs);
}

