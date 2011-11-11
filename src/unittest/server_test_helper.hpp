#ifndef __UNITTEST_SERVER_TEST_HELPER__
#define __UNITTEST_SERVER_TEST_HELPER__

#include "arch/types.hpp"
#include "utils.hpp"
#include "buffer_cache/types.hpp"
#include "concurrency/access.hpp"

class translator_serializer_t;

namespace unittest {

class server_test_helper_t {
public:
    server_test_helper_t();
    virtual ~server_test_helper_t();
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
    static buf_t *acq_check_if_blocks_until_buf_released(transaction_t *acquiring_txn, buf_t *already_acquired_block, access_t acquire_mode, bool do_release, bool *blocked);
    static void create_two_blocks(transaction_t *txn, block_id_t *block_A, block_id_t *block_B);

protected:
    virtual void run_tests(cache_t *cache) = 0;
    virtual void run_serializer_tests();

    translator_serializer_t *serializer;

private:
    thread_pool_t *thread_pool;

    struct acquiring_coro_t {
        buf_t *result;
        bool signaled;
        transaction_t *txn;
        block_id_t block_id;
        access_t mode;

        acquiring_coro_t(transaction_t *_txn, block_id_t _block_id, access_t _mode)
	    : result(NULL), signaled(false), txn(_txn), block_id(_block_id), mode(_mode) { }

        void run() {
            result = acq(txn, block_id, mode);
            signaled = true;
        }
    };

private:
    void setup_server_and_run_tests();
};

}  // namespace unittest

#endif //__UNITTEST_SERVER_TEST_HELPER__
