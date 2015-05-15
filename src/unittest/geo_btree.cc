// Copyright 2010-2014 RethinkDB, all rights reserved.

#include "rdb_protocol/geo/indexing.hpp"
#include "unittest/gtest.hpp"

using geo::S2CellId;

/* These are internal functions defined in `rdb_protocol/geo/indexing.cc`. */
std::string s2cellid_to_key(S2CellId id);
S2CellId key_to_s2cellid(const std::string &sid);
std::pair<S2CellId, bool> order_btree_key_relative_to_s2cellid_keys(
    const btree_key_t *key_or_null);

namespace unittest {

bool find_pred(S2CellId id, S2CellId *pred_out) {
    guarantee(id.is_valid());
    int steps = 10;
    do {
        id = S2CellId(id.id() - 1);
        if (id.id() == 0) {
            return false;
        }
        if (--steps == 0) {
            return false;
        }
    } while (!id.is_valid());
    *pred_out = id;
    return true;
}

void test_btree_key(const std::string &str_key) {
    SCOPED_TRACE("test_btree_key(" + ::testing::PrintToString(str_key) + ")");
    store_key_t key(str_key);
    std::pair<S2CellId, bool> res =
        order_btree_key_relative_to_s2cellid_keys(key.btree_key());
    if (res.first == S2CellId::Sentinel()) {
        ASSERT_LT(s2cellid_to_key(S2CellId::FromFacePosLevel(5, 0, 0).range_max()),
            str_key);
    } else if (res.second) {
        std::string prefix = s2cellid_to_key(res.first);
        ASSERT_EQ(prefix, str_key.substr(0, prefix.size()));
    } else {
        ASSERT_GT(s2cellid_to_key(res.first), str_key);
        SCOPED_TRACE("note: res.first = " +
            ::testing::PrintToString(s2cellid_to_key(res.first)));
        S2CellId pred;
        if (find_pred(res.first, &pred)) { 
            std::string prefix = s2cellid_to_key(pred);
            ASSERT_LE(prefix, str_key);
            ASSERT_NE(prefix, str_key.substr(0, prefix.size()));
        }
    }
}

S2CellId random_cell_id() {
    while (true) {
        S2CellId cell_id = S2CellId(randuint64(std::numeric_limits<uint64_t>::max()));
        int steps = 10;
        while (!cell_id.is_valid()) {
            cell_id = S2CellId(cell_id.id() + 1);
            if (--steps == 0) {
                break;
            }
        }
        if (!cell_id.is_valid()) {
            continue;
        }
        cell_id = cell_id.parent(randint(cell_id.level() + 1));
        return cell_id;
    }
}

TEST(GeoBtree, OrderBtreeKeyRelativeToS2CellIdKeys) {
    for (int i = 0; i < 100; ++i) {
        S2CellId cell_id = random_cell_id();
        std::string str_key = s2cellid_to_key(cell_id);
        ASSERT_NO_FATAL_FAILURE(test_btree_key(str_key));
        for (int j = 0; j < 10; ++j) {
            std::string variation = str_key;
            int pos = randint(variation.size());
            variation[pos] = randint(0xff);
            ASSERT_NO_FATAL_FAILURE(test_btree_key(variation));
        }
        ASSERT_NO_FATAL_FAILURE(
            test_btree_key(s2cellid_to_key(S2CellId(cell_id.id() - 1))));
        ASSERT_NO_FATAL_FAILURE(
            test_btree_key(s2cellid_to_key(S2CellId(cell_id.id() + 1))));
    }
    for (int i = 0; i < 100; ++i) {
        std::string str_key;
        int len = randint(100);
        str_key.reserve(len);
        for (int j = 0; j < len; ++j) {
            str_key.append(1, randint(0xff));
        }
        ASSERT_NO_FATAL_FAILURE(test_btree_key(str_key));
    }
}

} /* namespace unittest */

