#ifndef UNITTEST_SERVER_TEST_HELPER_HPP_
#define UNITTEST_SERVER_TEST_HELPER_HPP_

#include "arch/types.hpp"
#include "utils.hpp"
#include "buffer_cache/types.hpp"
#include "concurrency/access.hpp"
#include "containers/scoped.hpp"

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
    static void snap(transaction_t *txn);
    static void change_value(buf_lock_t *buf, uint32_t value);
    static uint32_t get_value(buf_lock_t *buf);
    static bool acq_check_if_blocks_until_buf_released(buf_lock_t *newly_acquired_block, transaction_t *acquiring_txn, buf_lock_t *already_acquired_block, access_t acquire_mode, bool do_release);
    static void create_two_blocks(transaction_t *txn, block_id_t *block_A, block_id_t *block_B);

protected:
    virtual void run_tests(cache_t *cache) = 0;
    virtual void run_serializer_tests();

    translator_serializer_t *serializer;

private:
    scoped_ptr_t<thread_pool_t> thread_pool;

    struct acquiring_coro_t {
        buf_lock_t *result;
        bool signaled;
        transaction_t *txn;
        block_id_t block_id;
        access_t mode;

        acquiring_coro_t(buf_lock_t *lock, transaction_t *_txn, block_id_t _block_id, access_t _mode)
            : result(lock), signaled(false), txn(_txn), block_id(_block_id), mode(_mode) { rassert(result != NULL); }

        void run();
    };

private:
    void setup_server_and_run_tests();
};

}  // namespace unittest

#endif  // UNITTEST_SERVER_TEST_HELPER_HPP_
