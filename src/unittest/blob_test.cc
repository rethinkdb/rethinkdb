#include "unittest/gtest.hpp"
#include "unittest/server_test_helper.hpp"
#include "unittest/unittest_utils.hpp"
#include "buffer_cache/blob.hpp"

namespace unittest {

class blob_tracker_t {
public:
    blob_tracker_t(block_size_t block_size, size_t maxreflen)
        : expected_("\0"), blob_(block_size, expected_.c_str(), maxreflen) {
        // That's a bit of a hack, because blob_ will only read the
        // first byte or first two bytes if they are zero.
        expected_.clear();
    }

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

    void check(transaction_t *txn) {
        check_region(txn, 0, expected_.size());
    }

    void append(transaction_t *txn, const std::string& x) {
        SCOPED_TRACE("append " + x);
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
        SCOPED_TRACE("prepend " + x);
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
    blob_t blob_;
};

struct blob_tester_t : public server_test_helper_t {
protected:
    void run_tests(cache_t *cache) {
        small_value_test(cache);
        small_value_boundary_test(cache);
    }

private:
    void small_value_test(cache_t *cache) {
        SCOPED_TRACE("small_value_test");
        block_size_t block_size = cache->get_block_size();

        transaction_t txn(cache, rwi_write, 0, repli_timestamp_t::distant_past);

        blob_tracker_t tk(block_size, 251);

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

        blob_tracker_t tk(block_size, 251);

        tk.append(&txn, std::string(250, 'a'));
        tk.append(&txn, "b");
        ASSERT_EQ(1 + 8 + 8 + 4, tk.refsize(block_size));
        tk.unappend(&txn, 1);
        ASSERT_EQ(251, tk.refsize(block_size));

        tk.prepend(&txn, "c");
        ASSERT_EQ(1 + 8 + 8 + 4, tk.refsize(block_size));
        /*
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
        */
    }
};

TEST(BlobTest, all_tests) {
    blob_tester_t().run();
}

}  // namespace unittest
