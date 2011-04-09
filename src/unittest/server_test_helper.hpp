#ifndef __UNITTEST_SERVER_TEST_HELPER__
#define __UNITTEST_SERVER_TEST_HELPER__

#include "arch/arch.hpp"
#include "utils.hpp"
#include "buffer_cache/buffer_cache.hpp"

#include "buffer_cache/transactor.hpp"
#include "buffer_cache/co_functions.hpp"


class server_test_helper_t {
    thread_pool_t thread_pool;
public:
    server_test_helper_t() : thread_pool(1), serializer(NULL) { }
    void run();

protected:
    static const int init_value;
    static const int changed_value;

    // Helper functions
    static buf_t *create(transactor_t& txor);
    static void snap(transactor_t& txor);
    static buf_t *acq(transactor_t& txor, block_id_t block_id, access_t mode = rwi_read);
    static void change_value(buf_t *buf, uint32_t value);
    static uint32_t get_value(buf_t *buf);
    static buf_t *acq_check_if_blocks_until_buf_released(transactor_t& acquiring_txor, buf_t *already_acquired_block, access_t acquire_mode, bool do_release, bool &blocked);
    static void create_two_blocks(transactor_t &txor, block_id_t &block_A, block_id_t &block_B);

protected:
    virtual void run_tests(thread_saver_t& saver, cache_t *cache) = 0;

    translator_serializer_t *serializer;

private:
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

private:
    void setup_server_and_run_tests();
};

#endif //__UNITTEST_SERVER_TEST_HELPER__
