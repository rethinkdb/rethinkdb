#include "unittest/gtest.hpp"
#include "unittest/server_test_helper.hpp"
#include "buffer_cache/blob.hpp"
#include "buffer_cache/buffer_cache.hpp"
#include "containers/buffer_group.hpp"
#include "containers/scoped.hpp"
#include "mock/unittest_utils.hpp"

namespace unittest {

class blob_tracker_t {
public:
    static char *alloc_emptybuf(size_t n) {
        char *ret = new char[n];
        memset(ret, 0, n);
        return ret;
    }

    explicit blob_tracker_t(size_t maxreflen)
        : buf_(alloc_emptybuf(maxreflen), maxreflen), blob_(buf_.data(), maxreflen) { }

    void check_region(transaction_t *txn, int64_t offset, int64_t size) {
        SCOPED_TRACE("check_region");
        ASSERT_LE(0, offset);
        ASSERT_LE(offset, expected_.size());
        ASSERT_LE(0, size);
        ASSERT_LE(offset + size, expected_.size());

        ASSERT_EQ(expected_.size(), blob_.valuesize());

        buffer_group_t bg;
        blob_acq_t bacq;
        blob_.expose_region(txn, rwi_read, offset, size, &bg, &bacq);

        ASSERT_EQ(size, bg.get_size());

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

    void check_normalization(transaction_t *txn) {
        int64_t size = expected_.size();
        size_t rs = blob_.refsize(txn->get_cache()->get_block_size());
        if (size < 251) {
            ASSERT_EQ(1 + size, rs);
        } else if (size <= 4080) {
            ASSERT_EQ(1 + 8 + 8 + 4, rs);
        } else if (size <= int64_t(4080 * ((250 - 8 - 8) / sizeof(block_id_t)))) {
            if (rs != 1 + 8 + 8 + 4) {
                ASSERT_LE(1 + 8 + 8 + 4 * ceil_divide(size, 4080), rs);
                ASSERT_GE(1 + 8 + 8 + 4 * (1 + ceil_divide(size - 1, 4080)), rs);
            } else {
                ASSERT_GT(size, 4080 * ((250 - 8 - 8) / sizeof(block_id_t)) - 4080 + 1);
            }
        } else if (size <= int64_t(4080 * (4080 / 4) * ((250 - 8 - 8) / sizeof(block_id_t)))) {
            ASSERT_LE(1 + 8 + 8 + 4 * ceil_divide(size, 4080 * (4080 / 4)), rs);
            ASSERT_GE(1 + 8 + 8 + 4 * (1 + ceil_divide(size - 1, 4080 * (4080 / 4))), rs);
        } else {
            ASSERT_GT(0, size);
        }
    }

    void check(transaction_t *txn) {
        check_region(txn, 0, expected_.size());
        check_normalization(txn);
    }

    void append(transaction_t *txn, const std::string& x) {
        SCOPED_TRACE(strprintf("append (%zu) ", x.size()) + std::string(x.begin(), x.begin() + std::min<size_t>(x.size(), 50)));
        int64_t n = x.size();

        blob_.append_region(txn, n);

        ASSERT_EQ(expected_.size() + n, blob_.valuesize());

        {
            buffer_group_t bg;
            blob_acq_t bacq;
            blob_.expose_region(txn, rwi_write, expected_.size(), n, &bg, &bacq);

            ASSERT_EQ(n, bg.get_size());

            const_buffer_group_t copyee;
            copyee.add_buffer(x.size(), x.data());

            buffer_group_copy_data(&bg, &copyee);

            expected_ += x;
        }

        check(txn);
    }

    void prepend(transaction_t *txn, const std::string& x) {
        SCOPED_TRACE(strprintf("prepend (%zu) ", x.size()) + std::string(x.begin(), x.begin() + std::min<size_t>(x.size(), 50)));
        int64_t n = x.size();

        blob_.prepend_region(txn, n);

        ASSERT_EQ(n + expected_.size(), blob_.valuesize());

        {
            buffer_group_t bg;
            blob_acq_t bacq;
            blob_.expose_region(txn, rwi_write, 0, n, &bg, &bacq);

            ASSERT_EQ(n, bg.get_size());

            const_buffer_group_t copyee;
            copyee.add_buffer(x.size(), x.data());

            buffer_group_copy_data(&bg, &copyee);

            expected_ = x + expected_;
        }

        check(txn);
    }

    void unprepend(transaction_t *txn, int64_t n) {
        SCOPED_TRACE("unprepend " + strprintf("%ld", n));
        ASSERT_LE(n, expected_.size());

        blob_.unprepend_region(txn, n);
        expected_.erase(0, n);

        check(txn);
    }

    void unappend(transaction_t *txn, int64_t n) {
        SCOPED_TRACE("unappend " + strprintf("%ld", n));
        ASSERT_LE(n, expected_.size());

        blob_.unappend_region(txn, n);
        expected_.erase(expected_.size() - n);

        check(txn);
    }

    size_t refsize(block_size_t block_size) const {
        return blob_.refsize(block_size);
    }

private:
    std::string expected_;
    scoped_array_t<char> buf_;
    blob_t blob_;
};

struct blob_tester_t : public server_test_helper_t {
protected:
    void run_tests(cache_t *cache) {
        // The tests below hard-code constants related to these numbers.
        EXPECT_EQ(251, blob::btree_maxreflen);
        EXPECT_EQ(4080, blob::stepsize(cache->get_block_size(), 1));
        EXPECT_EQ(4080 * (4080 / sizeof(block_id_t)), blob::stepsize(cache->get_block_size(), 2));

        debugf("small_value_test...\n");
        small_value_test(cache);
        debugf("small_value_boundary_test...\n");
        small_value_boundary_test(cache);
        debugf("special_4080_prepend_4081_test...\n");
        special_4080_prepend_4081_test(cache);
        debugf("special_4161...\n");
        special_4161600_prepend_12484801_test(cache);
        debugf("combinations_test...\n");
        combinations_test(cache);
        debugf("finished...\n");
    }

private:
    void small_value_test(cache_t *cache) {
        SCOPED_TRACE("small_value_test");
        UNUSED block_size_t block_size = cache->get_block_size();

        transaction_t txn(cache, rwi_write, 0, repli_timestamp_t::distant_past);

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

    void small_value_boundary_test(cache_t *cache) {
        SCOPED_TRACE("small_value_boundary_test");
        block_size_t block_size = cache->get_block_size();

        transaction_t txn(cache, rwi_write, 0, repli_timestamp_t::distant_past);

        blob_tracker_t tk(251);

        tk.append(&txn, std::string(250, 'a'));
        tk.append(&txn, "b");
        ASSERT_EQ(1 + 8 + 8 + 4, tk.refsize(block_size));
        tk.unappend(&txn, 1);
        ASSERT_EQ(251, tk.refsize(block_size));

        tk.prepend(&txn, "c");
        ASSERT_EQ(1 + 8 + 8 + 4, tk.refsize(block_size));
        tk.unappend(&txn, 1);
        ASSERT_EQ(251, tk.refsize(block_size));
        tk.append(&txn, "d");
        ASSERT_EQ(1 + 8 + 8 + 4, tk.refsize(block_size));
        tk.unprepend(&txn, 1);
        ASSERT_EQ(251, tk.refsize(block_size));
        tk.append(&txn, "e");
        ASSERT_EQ(1 + 8 + 8 + 4, tk.refsize(block_size));
        tk.unprepend(&txn, 2);
        ASSERT_EQ(250, tk.refsize(block_size));
        tk.prepend(&txn, "fffff");
        ASSERT_EQ(1 + 8 + 8 + 4, tk.refsize(block_size));
        tk.unprepend(&txn, 254);
        ASSERT_EQ(1, tk.refsize(block_size));

        tk.append(&txn, std::string(251, 'g'));
        ASSERT_EQ(1 + 8 + 8 + 4, tk.refsize(block_size));

        tk.unappend(&txn, 251);
        ASSERT_EQ(1, tk.refsize(block_size));
        tk.prepend(&txn, std::string(251, 'h'));
        ASSERT_EQ(1 + 8 + 8 + 4, tk.refsize(block_size));
        tk.unappend(&txn, 250);
        ASSERT_EQ(2, tk.refsize(block_size));
        tk.prepend(&txn, std::string(250, 'i'));
        ASSERT_EQ(1 + 8 + 8 + 4, tk.refsize(block_size));
        tk.unprepend(&txn, 250);
        ASSERT_EQ(2, tk.refsize(block_size));
        tk.append(&txn, std::string(250, 'j'));
        ASSERT_EQ(1 + 8 + 8 + 4, tk.refsize(block_size));
        tk.unappend(&txn, 250);
        ASSERT_EQ(2, tk.refsize(block_size));
        tk.unappend(&txn, 1);
        ASSERT_EQ(1, tk.refsize(block_size));
    }

    void special_4080_prepend_4081_test(cache_t *cache) {
        SCOPED_TRACE("special_4080_prepend_4081_test");
        block_size_t block_size = cache->get_block_size();

        ASSERT_EQ(4080, block_size.value() - sizeof(block_magic_t));

        transaction_t txn(cache, rwi_write, 0, repli_timestamp_t::distant_past);

        blob_tracker_t tk(251);

        tk.append(&txn, std::string(4080, 'a'));
        tk.prepend(&txn, "b");
        tk.unappend(&txn, 4081);
    }

    // Regression test - these magic numbers caused failures previously.
    void special_4161600_prepend_12484801_test(cache_t *cache) {
        SCOPED_TRACE("special_4080_prepend_4081_test");
        block_size_t block_size = cache->get_block_size();

        ASSERT_EQ(4080, block_size.value() - sizeof(block_magic_t));

        transaction_t txn(cache, rwi_write, 0, repli_timestamp_t::distant_past);

        blob_tracker_t tk(251);

        int64_t lo_size = 4161600;
        int64_t hi_size = 12484801;
        tk.append(&txn, std::string(lo_size, 'a'));
        tk.prepend(&txn, std::string(hi_size - lo_size, 'b'));
        tk.unappend(&txn, hi_size);
    }

    struct step_t {
        int64_t size;
        bool prepend;
        step_t(int64_t _size, bool _prepend) : size(_size), prepend(_prepend) { }
    };

    void general_journey_test(cache_t *cache, const std::vector<step_t>& steps) {
        UNUSED block_size_t block_size = cache->get_block_size();

        transaction_t txn(cache, rwi_write, 0, repli_timestamp_t::distant_past);
        blob_tracker_t tk(251);

        char v = 'A';
        int64_t size = 0;
        for (int i = 0, n = steps.size(); i < n; ++i) {
            if (steps[i].prepend) {
                if (steps[i].size <= size) {
                    SCOPED_TRACE(strprintf("unprepending from %ld to %ld\n", size, steps[i].size));
                    tk.unprepend(&txn, size - steps[i].size);
                } else {
                    SCOPED_TRACE(strprintf("prepending from %ld to %ld\n", size, steps[i].size));
                    tk.prepend(&txn, std::string(steps[i].size - size, v));
                }
            } else {
                if (steps[i].size <= size) {
                    SCOPED_TRACE(strprintf("unappending from %ld to %ld\n", size, steps[i].size));
                    tk.unappend(&txn, size - steps[i].size);
                } else {
                    SCOPED_TRACE(strprintf("appending from %ld to %ld\n", size, steps[i].size));
                    tk.append(&txn, std::string(steps[i].size - size, v));
                }
            }

            size = steps[i].size;

            v = (v == 'z' ? 'A' : v == 'Z' ? 'a' : v + 1);
        }
        tk.unappend(&txn, size);
    }

    void combinations_test(cache_t *cache) {
        SCOPED_TRACE("combinations_test");
        int64_t inline_sz = 4080 * ((250 - 1 - 8 - 8) / 4);
        //        int64_t l2_sz = 4080 * (4080 / 4);
        int64_t szs[] = { 0, 251, 4080, 4081, inline_sz - 300, inline_sz, inline_sz + 1 };  // for now, until we can make this test faster.  // , l2_sz, l2_sz + 1, l2_sz * 3 + 1 };

        int n = sizeof(szs) / sizeof(szs[0]);

        for (int i = 0; i < n; ++i) {
            debugf("combinations_test: i = %d\n", i);
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
};

TEST(BlobTest, all_tests) {
    blob_tester_t().run();
}

}  // namespace unittest
