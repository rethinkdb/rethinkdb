#include "unittest/gtest.hpp"

#include <boost/bind.hpp>
#include <cstdlib>
#include <string>
#include <functional>
#include "server/cmd_args.hpp"
#include "serializer/log/log_serializer.hpp"
#include "serializer/translator.hpp"
#include "btree/slice.hpp"
#include "buffer_cache/transactor.hpp"
#include "buffer_cache/co_functions.hpp"
#include "concurrency/count_down_latch.hpp"

namespace unittest {

class temp_file_t {
    char * filename;
public:
    temp_file_t(const char * tmpl) {
        size_t len = strlen(tmpl);
        filename = new char[len+1];
        strncpy(filename, tmpl, len);
        int fd = mkstemp(filename);
        guarantee_err(fd != -1, "Couldn't create a temporary file");
        close(fd);
    }
    operator const char *() {
        return filename;
    }
    ~temp_file_t() {
        unlink(filename);
        delete [] filename;
    }
};

struct tester_t : public thread_message_t {
    thread_pool_t thread_pool;

    tester_t() : thread_pool(1) { }

    void run() {
        thread_pool.run(this);
    }

private:
    static buf_t *create(transactor_t& txor) {
        return txor.transaction()->allocate();
    }
    static void snap(transactor_t& txor) {
        txor.transaction()->snapshot();
    }
    static buf_t *acq(transactor_t& txor, block_id_t block_id, access_t mode = rwi_read) {
        thread_saver_t saver;
        return co_acquire_block(saver, txor.transaction(), block_id, mode);
    }

    static void change_value(buf_t *buf, uint32_t value) {
        buf->set_data(const_cast<void *>(buf->get_data_read()), &value, sizeof(value));
    }

    static uint32_t get_value(buf_t *buf) {
        return *((uint32_t*) buf->get_data_read());
    }

    struct acquiring_coro_t {
        buf_t *result;
        bool signaled;
        transactor_t& txor;
        block_id_t block_id;
        access_t mode;
        acquiring_coro_t(transactor_t& txor, block_id_t block_id, access_t mode) : result(NULL), signaled(false), txor(txor), block_id(block_id), mode(mode) { }
        void run() {
            result = acq(txor, block_id, mode);
            signaled = true;
        }
    };
    static buf_t *acq_check_if_blocks_until_buf_released(transactor_t& acquiring_txor, buf_t *already_acquired_block, access_t acquire_mode, bool do_release, bool &blocked) {
        acquiring_coro_t acq_coro(acquiring_txor, already_acquired_block->get_block_id(), acquire_mode);

        coro_t::spawn(boost::bind(&acquiring_coro_t::run, &acq_coro));
        coro_t::nap(500);
        blocked = !acq_coro.signaled;

        if (do_release) {
            already_acquired_block->release();
            if (blocked) {
                coro_t::nap(500);
                rassert(acq_coro.signaled, "Buf release must have unblocked the coroutine trying to acquire the buf. May be a bug in the test.");
            }
        }

        return acq_coro.result;
    }

    void snapshot_txn_runner(cache_t *cache, count_down_latch_t *stop_latch) {
        thread_saver_t saver;
        transactor_t snap_txor(saver, cache, rwi_read, repli_timestamp::invalid);
        snap_txor.transaction()->snapshot();
        debugf("snapshot.done\n");
        stop_latch->count_down();
    }
    void main() {
        temp_file_t db_file("/tmp/rdb_unittest.XXXXXX");

        {
            cmd_config_t config;
            config.store_dynamic_config.cache.max_dirty_size = config.store_dynamic_config.cache.max_size / 10;

            // Set ridiculously high flush_* values, so that flush lock doesn't block the txn creation
            config.store_dynamic_config.cache.flush_timer_ms = 1000000;
            config.store_dynamic_config.cache.flush_dirty_size = 1000000000;

            log_serializer_private_dynamic_config_t ser_config;
            ser_config.db_filename = db_file;

            log_serializer_t::create(&config.store_dynamic_config.serializer, &ser_config, &config.store_static_config.serializer);
            log_serializer_t serializer(&config.store_dynamic_config.serializer, &ser_config);

            std::vector<serializer_t *> serializers;
            serializers.push_back(&serializer);
            serializer_multiplexer_t::create(serializers, 1);
            serializer_multiplexer_t multiplexer(serializers);

            btree_slice_t::create(multiplexer.proxies[0], &config.store_static_config.cache);
            btree_slice_t slice(multiplexer.proxies[0], &config.store_dynamic_config.cache);

            cache_t *cache = slice.cache();


            int init_value = 0x12345678;
            int changed_value = 0x87654321;

            thread_saver_t saver;

            // t0:create(A), t1:snap(), t1:acq(A) blocks, t0:release(A), t1 unblocks, t1 sees the block.
            {
                transactor_t t0(saver, cache, rwi_write, 0, current_time());
                transactor_t t1(saver, cache, rwi_read, 0, repli_timestamp::invalid);

                buf_t *buf0 = create(t0);
                snap(t1);
                bool blocked = false;
                buf_t *buf1 = acq_check_if_blocks_until_buf_released(t1, buf0, rwi_read, true, blocked);
                EXPECT_TRUE(blocked);
                EXPECT_TRUE(buf1 != NULL);
                if (buf1)
                    buf1->release();

            }

            // t0:create(A), t0:release(A), t1:snap(), t2:acqw(A), t2:change(A), t2:release(A), t1:acq(A), t1 sees the change
            {
                transactor_t t0(saver, cache, rwi_write, 0, current_time());
                transactor_t t1(saver, cache, rwi_read, 0, repli_timestamp::invalid);
                transactor_t t2(saver, cache, rwi_write, 0, current_time());

                buf_t *buf0 = create(t0);
                block_id_t block_A = buf0->get_block_id();
                change_value(buf0, init_value);
                buf0->release();

                snap(t1);

                buf_t *buf2 = acq(t2, block_A, rwi_write);
                change_value(buf2, changed_value);
                buf2->release();

                buf_t *buf1 = acq(t1, block_A, rwi_read);
                EXPECT_EQ(changed_value, get_value(buf1));
                buf1->release();
            }
            // t0:create(A), t0:release(A), t1:snap(), t1:acq(A), t2:acqw(A), t2:change(A), t2:release(A), t3:snap(), t3:acq(A), t1 doesn't see the change, t3 does see the change
            {
                transactor_t t0(saver, cache, rwi_write, 0, current_time());
                transactor_t t1(saver, cache, rwi_read, 0, repli_timestamp::invalid);
                transactor_t t2(saver, cache, rwi_write, 0, current_time());
                transactor_t t3(saver, cache, rwi_read, 0, repli_timestamp::invalid);

                buf_t *buf0 = create(t0);
                block_id_t block_A = buf0->get_block_id();
                change_value(buf0, init_value);
                buf0->release();

                snap(t1);
                buf_t *buf1 = acq(t1, block_A, rwi_read);

                bool blocked = true;
                buf_t *buf2 = acq_check_if_blocks_until_buf_released(t2, buf1, rwi_write, false, blocked);
                EXPECT_FALSE(blocked);

                change_value(buf2, changed_value);

                snap(t3);
                buf_t *buf3 = acq_check_if_blocks_until_buf_released(t2, buf2, rwi_read, true, blocked);
                EXPECT_TRUE(blocked);

                EXPECT_EQ(init_value, get_value(buf1));
                EXPECT_EQ(changed_value, get_value(buf3));
                buf1->release();
                buf3->release();
            }

            // t0:create(A,B), t0:release(A,B), t1:snap(), t1:acq(A), t2:acqw(A) doesn't block, t2:acqw(B), t1:acq(B) doesn't block
            {
                transactor_t t0(saver, cache, rwi_write, 0, current_time());
                transactor_t t1(saver, cache, rwi_read, 0, repli_timestamp::invalid);
                transactor_t t2(saver, cache, rwi_write, 0, current_time());

                buf_t *buf0_A = create(t0);
                buf_t *buf0_B = create(t0);
                block_id_t block_A = buf0_A->get_block_id();
                block_id_t block_B = buf0_B->get_block_id();
                change_value(buf0_A, init_value);
                change_value(buf0_B, init_value);
                buf0_A->release();
                buf0_B->release();

                snap(t1);
                buf_t *buf1_A = acq(t1, block_A, rwi_read);

                bool blocked = true;
                buf_t *buf2_A = acq_check_if_blocks_until_buf_released(t2, buf1_A, rwi_write, false, blocked);
                EXPECT_FALSE(blocked);

                buf_t *buf2_B = acq(t2, block_B, rwi_write);

                buf_t *buf1_B = acq_check_if_blocks_until_buf_released(t1, buf2_B, rwi_read, false, blocked);
                EXPECT_FALSE(blocked);

                buf1_A->release();
                buf2_A->release();
                buf1_B->release();
                buf2_B->release();
            }

            // t0:create(A,B), t0:release(A,B), t1:acqw(A), t1:acqw(B), t1:release(A), t2:snap(), t2:acq(A), t2:release(A), t2:acq(B) blocks, t1:release(B), t2 unblocks
            {
                transactor_t t0(saver, cache, rwi_write, 0, current_time());
                transactor_t t1(saver, cache, rwi_write, 0, current_time());
                transactor_t t2(saver, cache, rwi_read, 0, repli_timestamp::invalid);

                buf_t *buf0_A = create(t0);
                buf_t *buf0_B = create(t0);
                block_id_t block_A = buf0_A->get_block_id();
                block_id_t block_B = buf0_B->get_block_id();
                change_value(buf0_A, init_value);
                change_value(buf0_B, init_value);
                buf0_A->release();
                buf0_B->release();

                buf_t *buf1_A = acq(t1, block_A, rwi_write);
                buf_t *buf1_B = acq(t1, block_B, rwi_write);
                change_value(buf1_A, changed_value);
                change_value(buf1_B, changed_value);
                buf1_A->release();

                snap(t2);
                buf_t *buf2_A = acq(t1, block_A, rwi_read);
                EXPECT_EQ(changed_value, get_value(buf2_A));

                buf2_A->release();
                bool blocked = false;
                buf_t *buf2_B = acq_check_if_blocks_until_buf_released(t2, buf1_B, rwi_read, true, blocked);
                EXPECT_TRUE(blocked);
                EXPECT_EQ(changed_value, get_value(buf2_B));
                buf2_B->release();

            }
            debugf("tests finished\n");
        }
        debugf("thread_pool.shutdown()\n");
        thread_pool.shutdown();
    }

    void on_thread_switch() {
        coro_t::spawn(boost::bind(&tester_t::main, this));
    }
};

TEST(SnapshotsTest, server_start) {
    tester_t test;
    test.run();
}

}
