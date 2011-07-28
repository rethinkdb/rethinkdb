#ifndef __UNITTEST_SERVER_TEST_HELPER__
#define __UNITTEST_SERVER_TEST_HELPER__

#include "arch/arch.hpp"
#include "utils.hpp"
#include "buffer_cache/types.hpp"
#include "buffer_cache/co_functions.hpp"

class translator_serializer_t;

class server_test_helper_t {
    thread_pool_t thread_pool;
public:
    server_test_helper_t() : thread_pool(1), serializer(NULL) { }
    virtual ~server_test_helper_t() { }
    void run();

protected:
    static const int init_value;
    static const int changed_value;

    // Helper functions
    static buf_t *create(transaction_t *txn);
    static void snap(transaction_t *txn);
    static buf_t *acq(transaction_t *txn, block_id_t block_id, access_t mode = rwi_read);
    static void change_value(buf_t *buf, uint32_t value);
    static uint32_t get_value(buf_t *buf);
    static buf_t *acq_check_if_blocks_until_buf_released(transaction_t *acquiring_txn, buf_t *already_acquired_block, access_t acquire_mode, bool do_release, bool &blocked);
    static void create_two_blocks(transaction_t *txn, block_id_t &block_A, block_id_t &block_B);

protected:
    virtual void run_tests(cache_t *cache) = 0;

    translator_serializer_t *serializer;

private:
    struct acquiring_coro_t {
        buf_t *result;
        bool signaled;
        transaction_t *txn;
        block_id_t block_id;
        access_t mode;
        acquiring_coro_t(transaction_t *txn, block_id_t block_id, access_t mode) : result(NULL), signaled(false), txn(txn), block_id(block_id), mode(mode) { }
        void run() {
            result = acq(txn, block_id, mode);
            signaled = true;
        }
    };

private:
    void setup_server_and_run_tests();
};

#endif //__UNITTEST_SERVER_TEST_HELPER__
