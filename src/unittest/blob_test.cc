// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "buffer_cache/alt/alt.hpp"
#include "buffer_cache/alt/alt_blob.hpp"
#include "containers/buffer_group.hpp"
#include "containers/scoped.hpp"
#include "unittest/gtest.hpp"
#include "unittest/mock_file.hpp"
#include "unittest/unittest_utils.hpp"
#include "serializer/config.hpp"

using namespace alt;  // RSI

namespace unittest {

static const int expected_cache_block_size = 4080;
static const int size_after_magic = expected_cache_block_size - sizeof(block_magic_t);

class blob_tracker_t {
public:
    static char *alloc_emptybuf(size_t n) {
        char *ret = new char[n];
        memset(ret, 0, n);
        return ret;
    }

    explicit blob_tracker_t(size_t maxreflen)
        : buf_(alloc_emptybuf(maxreflen), maxreflen), blob_(block_size_t::unsafe_make(4096),
                                                            buf_.data(), maxreflen) { }

    void check_region(alt_txn_t *txn, int64_t offset, int64_t size) {
        SCOPED_TRACE("check_region");
        const int64_t expecteds_size = expected_.size();
        ASSERT_LE(0, offset);
        ASSERT_LE(offset, expecteds_size);
        ASSERT_LE(0, size);
        ASSERT_LE(offset + size, expecteds_size);

        ASSERT_EQ(expecteds_size, blob_.valuesize());

        buffer_group_t bg;
        alt::blob_acq_t bacq;
        blob_.expose_region(alt_buf_parent_t(txn), alt_access_t::read, offset, size,
                            &bg, &bacq);

        ASSERT_EQ(size, static_cast<int64_t>(bg.get_size()));

        std::string::iterator p_orig = expected_.begin() + offset, p = p_orig, e = p + size;
        for (size_t i = 0, n = bg.num_buffers(); i < n; ++i) {
            buffer_group_t::buffer_t buffer = bg.get_buffer(i);
            SCOPED_TRACE(strprintf("buffer %zu/%zu (size %zu)", i, n, buffer.size));
            ASSERT_LE(buffer.size, e - p);
            char *data = reinterpret_cast<char *>(buffer.data);
            ASSERT_TRUE(std::equal(data, data + buffer.size, p));
            p += buffer.size;
        }

        ASSERT_EQ(e - p_orig, p - p_orig);
    }

    void check_normalization(alt_txn_t *txn) {
        size_t size = expected_.size();
        uint64_t rs = blob_.refsize(txn->cache()->get_block_size());
        size_t sizesize = buf_.size() <= 255 ? 1 : 2;
        if (size <= buf_.size() - sizesize) {
            ASSERT_EQ(sizesize + size, rs);
        } else if (size <= static_cast<size_t>(size_after_magic)) {
            ASSERT_EQ(sizesize + 8 + 8 + sizeof(block_id_t), rs);
        } else if (size <= int64_t(size_after_magic * ((250 - 8 - 8) / sizeof(block_id_t)))) {
            if (rs != sizesize + 8 + 8 + sizeof(block_id_t)) {
                ASSERT_LE(sizesize + 8 + 8 + sizeof(block_id_t) * ceil_divide(size, size_after_magic), rs);
                ASSERT_GE(sizesize + 8 + 8 + sizeof(block_id_t) * (1 + ceil_divide(size - 1, size_after_magic)), rs);
            } else {
                ASSERT_GT(size, size_after_magic * ((250 - 8 - 8) / sizeof(block_id_t)) - size_after_magic + 1);
            }
        } else if (size <= int64_t(size_after_magic * (size_after_magic / sizeof(block_id_t)) * ((250 - 8 - 8) / sizeof(block_id_t)))) {
            ASSERT_LE(sizesize + 8 + 8 + sizeof(block_id_t) * ceil_divide(size, size_after_magic * (size_after_magic / sizeof(block_id_t))), rs);
            ASSERT_GE(sizesize + 8 + 8 + sizeof(block_id_t) * (1 + ceil_divide(size - 1, size_after_magic * (size_after_magic / sizeof(block_id_t)))), rs);
        } else {
            ASSERT_GT(0u, size);
        }
    }

    void check(alt_txn_t *txn) {
        check_region(txn, 0, expected_.size());
        check_normalization(txn);
    }

    void append(alt_txn_t *txn, const std::string &x) {
        SCOPED_TRACE(strprintf("append (%zu) ", x.size()) + std::string(x.begin(), x.begin() + std::min<size_t>(x.size(), 50)));
        int64_t n = x.size();

        blob_.append_region(alt_buf_parent_t(txn), n);

        ASSERT_EQ(static_cast<int64_t>(expected_.size() + n), blob_.valuesize());

        {
            buffer_group_t bg;
            alt::blob_acq_t bacq;
            blob_.expose_region(alt_buf_parent_t(txn), alt_access_t::write,
                                expected_.size(), n, &bg, &bacq);

            ASSERT_EQ(static_cast<size_t>(n), bg.get_size());

            const_buffer_group_t copyee;
            copyee.add_buffer(x.size(), x.data());

            buffer_group_copy_data(&bg, &copyee);

            expected_ += x;
        }

        check(txn);
    }

    void prepend(alt_txn_t *txn, const std::string &x) {
        SCOPED_TRACE(strprintf("prepend (%zu) ", x.size()) + std::string(x.begin(), x.begin() + std::min<size_t>(x.size(), 50)));
        int64_t n = x.size();

        blob_.prepend_region(alt_buf_parent_t(txn), n);

        ASSERT_EQ(static_cast<int64_t>(n + expected_.size()), blob_.valuesize());

        {
            buffer_group_t bg;
            alt::blob_acq_t bacq;
            blob_.expose_region(alt_buf_parent_t(txn), alt_access_t::write, 0, n,
                                &bg, &bacq);

            ASSERT_EQ(n, static_cast<int64_t>(bg.get_size()));

            const_buffer_group_t copyee;
            copyee.add_buffer(x.size(), x.data());

            buffer_group_copy_data(&bg, &copyee);

            expected_ = x + expected_;
        }

        check(txn);
    }

    void unprepend(alt_txn_t *txn, int64_t n) {
        SCOPED_TRACE("unprepend " + strprintf("%" PRIi64, n));
        ASSERT_LE(n, static_cast<int64_t>(expected_.size()));

        blob_.unprepend_region(alt_buf_parent_t(txn), n);
        expected_.erase(0, n);

        check(txn);
    }

    void unappend(alt_txn_t *txn, int64_t n) {
        SCOPED_TRACE("unappend " + strprintf("%" PRIi64, n));
        ASSERT_LE(n, static_cast<int64_t>(expected_.size()));

        blob_.unappend_region(alt_buf_parent_t(txn), n);
        expected_.erase(expected_.size() - n);

        check(txn);
    }

    void clear(alt_txn_t *txn) {
        SCOPED_TRACE("clear");
        blob_.clear(alt_buf_parent_t(txn));
        expected_.clear();
        check(txn);
    }

    size_t refsize(block_size_t block_size) const {
        return blob_.refsize(block_size);
    }

private:
    std::string expected_;
    scoped_array_t<char> buf_;
    alt::blob_t blob_;
};

void small_value_test(alt_cache_t *cache) {
    SCOPED_TRACE("small_value_test");
    UNUSED block_size_t block_size = cache->get_block_size();

    alt_txn_t txn(cache, write_durability_t::SOFT, repli_timestamp_t::distant_past, 0);

    blob_tracker_t tk(251);

    tk.append(&txn, "a");
    tk.append(&txn, "b");
    tk.prepend(&txn, "c");
    tk.unappend(&txn, 1);
    tk.unappend(&txn, 2);
    tk.append(&txn, std::string(250, 'd'));
    for (int i = 0; i < 250; ++i) {
        tk.unappend(&txn, 1);
    }
    tk.prepend(&txn, std::string(250, 'e'));
    for (int i = 0; i < 125; ++i) {
        tk.unprepend(&txn, 2);
    }

    tk.append(&txn, std::string(125, 'f'));
    tk.prepend(&txn, std::string(125, 'g'));
    tk.unprepend(&txn, 0);
    tk.unappend(&txn, 0);
    tk.unprepend(&txn, 250);
    tk.unappend(&txn, 0);
    tk.unprepend(&txn, 0);
    tk.append(&txn, "");
    tk.prepend(&txn, "");
}

void small_value_boundary_test(alt_cache_t *cache) {
    SCOPED_TRACE("small_value_boundary_test");
    block_size_t block_size = cache->get_block_size();

    alt_txn_t txn(cache, write_durability_t::SOFT,
                  repli_timestamp_t::distant_past, 0);

    blob_tracker_t tk(251);

    tk.append(&txn, std::string(250, 'a'));
    tk.append(&txn, "b");
    ASSERT_EQ(1 + 8 + 8 + sizeof(block_id_t), tk.refsize(block_size));
    tk.unappend(&txn, 1);
    ASSERT_EQ(251u, tk.refsize(block_size));

    tk.prepend(&txn, "c");
    ASSERT_EQ(1 + 8 + 8 + sizeof(block_id_t), tk.refsize(block_size));
    tk.unappend(&txn, 1);
    ASSERT_EQ(251u, tk.refsize(block_size));
    tk.append(&txn, "d");
    ASSERT_EQ(1 + 8 + 8 + sizeof(block_id_t), tk.refsize(block_size));
    tk.unprepend(&txn, 1);
    ASSERT_EQ(251u, tk.refsize(block_size));
    tk.append(&txn, "e");
    ASSERT_EQ(1 + 8 + 8 + sizeof(block_id_t), tk.refsize(block_size));
    tk.unprepend(&txn, 2);
    ASSERT_EQ(250u, tk.refsize(block_size));
    tk.prepend(&txn, "fffff");
    ASSERT_EQ(1 + 8 + 8 + sizeof(block_id_t), tk.refsize(block_size));
    tk.unprepend(&txn, 254);
    ASSERT_EQ(1u, tk.refsize(block_size));

    tk.append(&txn, std::string(251, 'g'));
    ASSERT_EQ(1 + 8 + 8 + sizeof(block_id_t), tk.refsize(block_size));

    tk.unappend(&txn, 251);
    ASSERT_EQ(1u, tk.refsize(block_size));
    tk.prepend(&txn, std::string(251, 'h'));
    ASSERT_EQ(1 + 8 + 8 + sizeof(block_id_t), tk.refsize(block_size));
    tk.unappend(&txn, 250);
    ASSERT_EQ(2u, tk.refsize(block_size));
    tk.prepend(&txn, std::string(250, 'i'));
    ASSERT_EQ(1 + 8 + 8 + sizeof(block_id_t), tk.refsize(block_size));
    tk.unprepend(&txn, 250);
    ASSERT_EQ(2u, tk.refsize(block_size));
    tk.append(&txn, std::string(250, 'j'));
    ASSERT_EQ(1 + 8 + 8 + sizeof(block_id_t), tk.refsize(block_size));
    tk.unappend(&txn, 250);
    ASSERT_EQ(2u, tk.refsize(block_size));
    tk.unappend(&txn, 1);
    ASSERT_EQ(1u, tk.refsize(block_size));
}

void special_4080_prepend_4081_test(alt_cache_t *cache) {
    SCOPED_TRACE("special_4080_prepend_4081_test");
    block_size_t block_size = cache->get_block_size();

    ASSERT_EQ(static_cast<size_t>(size_after_magic), block_size.value() - sizeof(block_magic_t));

    alt_txn_t txn(cache, write_durability_t::SOFT, repli_timestamp_t::distant_past, 0);

    blob_tracker_t tk(251);

    tk.append(&txn, std::string(size_after_magic, 'a'));
    tk.prepend(&txn, "b");
    tk.unappend(&txn, size_after_magic + 1);
}

// Regression test - these magic numbers caused failures previously.
void special_4161600_prepend_12484801_test(alt_cache_t *cache) {
    SCOPED_TRACE("special_4080_prepend_4081_test");
    alt_txn_t txn(cache, write_durability_t::SOFT, repli_timestamp_t::distant_past, 0);

    blob_tracker_t tk(251);

    int64_t lo_size = size_after_magic * (size_after_magic / sizeof(block_id_t));
    int64_t hi_size = 3 * lo_size + 1;
    tk.append(&txn, std::string(lo_size, 'a'));
    tk.prepend(&txn, std::string(hi_size - lo_size, 'b'));
    tk.unappend(&txn, hi_size);
}

struct step_t {
    int64_t size;
    bool prepend;
    step_t(int64_t _size, bool _prepend) : size(_size), prepend(_prepend) { }
};

void general_journey_test(alt_cache_t *cache, const std::vector<step_t>& steps) {
    UNUSED block_size_t block_size = cache->get_block_size();

    alt_txn_t txn(cache, write_durability_t::SOFT, repli_timestamp_t::distant_past, 0);
    blob_tracker_t tk(251);

    char v = 'A';
    int64_t size = 0;
    for (int i = 0, n = steps.size(); i < n; ++i) {
        if (steps[i].prepend) {
            if (steps[i].size <= size) {
                SCOPED_TRACE(strprintf("unprepending from %" PRIi64 " to %" PRIi64, size, steps[i].size));
                tk.unprepend(&txn, size - steps[i].size);
            } else {
                SCOPED_TRACE(strprintf("prepending from %" PRIi64 " to %" PRIi64, size, steps[i].size));
                tk.prepend(&txn, std::string(steps[i].size - size, v));
            }
        } else {
            if (steps[i].size <= size) {
                SCOPED_TRACE(strprintf("unappending from %" PRIi64 " to %" PRIi64, size, steps[i].size));
                tk.unappend(&txn, size - steps[i].size);
            } else {
                SCOPED_TRACE(strprintf("appending from %" PRIi64 " to %" PRIi64, size, steps[i].size));
                tk.append(&txn, std::string(steps[i].size - size, v));
            }
        }

        size = steps[i].size;

        v = (v == 'z' ? 'A' : v == 'Z' ? 'a' : v + 1);
    }
    tk.unappend(&txn, size);
}

void combinations_test(alt_cache_t *cache) {
    SCOPED_TRACE("combinations_test");

    int64_t inline_sz = size_after_magic * ((250 - 1 - 8 - 8) / sizeof(block_id_t));
    //        int64_t l2_sz = size_after_magic * (size_after_magic / sizeof(block_id_t));
    int64_t szs[] = { 0, 251, size_after_magic, size_after_magic, inline_sz - 300, inline_sz, inline_sz + 1 };  // for now, until we can make this test faster.  // , l2_sz, l2_sz + 1, l2_sz * 3 + 1 };

    int n = sizeof(szs) / sizeof(szs[0]);

    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            SCOPED_TRACE(strprintf("i,j = %d,%d", i, j));
            std::vector<step_t> steps;
            steps.push_back(step_t(szs[i], false));
            steps.push_back(step_t(szs[j], false));
            {
                SCOPED_TRACE("ap ap");
                general_journey_test(cache, steps);
            }
            steps[1].prepend = true;
            {
                SCOPED_TRACE("ap prep");
                general_journey_test(cache, steps);
            }
            steps[0].prepend = true;
            {
                SCOPED_TRACE("prep prep");
                general_journey_test(cache, steps);
            }
            steps[1].prepend = false;
            {
                SCOPED_TRACE("prep ap");
                general_journey_test(cache, steps);
            }
        }
    }
}


void run_tests(alt_cache_t *cache) {
    // The tests above hard-code constants related to these numbers.
    EXPECT_EQ(251, alt::blob::btree_maxreflen);
    EXPECT_EQ(4u, sizeof(block_magic_t));
    const int size_sans_magic = expected_cache_block_size - sizeof(block_magic_t);
    EXPECT_EQ(size_sans_magic, alt::blob::stepsize(cache->get_block_size(), 1));
    EXPECT_EQ(size_sans_magic * (size_sans_magic / static_cast<int>(sizeof(block_id_t))),
              alt::blob::stepsize(cache->get_block_size(), 2));

    small_value_test(cache);
    small_value_boundary_test(cache);
    special_4080_prepend_4081_test(cache);
    special_4161600_prepend_12484801_test(cache);
    combinations_test(cache);
}

void run_blob_test() {
    mock_file_opener_t file_opener;
    standard_serializer_t::create(
            &file_opener,
            standard_serializer_t::static_config_t());
    standard_serializer_t log_serializer(
            standard_serializer_t::dynamic_config_t(),
            &file_opener,
            &get_global_perfmon_collection());

    alt_cache_t cache(&log_serializer,
                      alt_cache_config_t(),
                      &get_global_perfmon_collection());

    run_tests(&cache);
}

TEST(BlobTest, all_tests) {
    unittest::run_in_thread_pool(run_blob_test);
}

}  // namespace unittest
