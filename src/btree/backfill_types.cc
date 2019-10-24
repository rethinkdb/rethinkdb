#include "btree/backfill_types.hpp"

#include "containers/archive/optional.hpp"
#include "containers/archive/stl_types.hpp"


RDB_IMPL_SERIALIZABLE_1_FOR_CLUSTER(backfill_pre_item_t, range);
RDB_IMPL_SERIALIZABLE_3_FOR_CLUSTER(backfill_item_t::pair_t, key, recency, value);
RDB_IMPL_SERIALIZABLE_3_FOR_CLUSTER(backfill_item_t,
    range, pairs, min_deletion_timestamp);

void backfill_item_t::mask_in_place(const key_range_t &m) {
    range = range.intersection(m);
    std::vector<pair_t> new_pairs;
    for (auto &&pair : pairs) {
        if (m.contains_key(pair.key)) {
            new_pairs.push_back(std::move(pair));
        }
    }
    pairs = std::move(new_pairs);
}
