#ifndef __UNITTEST_SERVER_TEST_HELPER__
#define __UNITTEST_SERVER_TEST_HELPER__

#include "arch/arch.hpp"
#include "utils.hpp"
#include "buffer_cache/buffer_cache.hpp"

class server_test_helper_t {
    thread_pool_t thread_pool;
public:
    server_test_helper_t() : thread_pool(1) { }
    void run();

protected:
    virtual void run_tests(thread_saver_t& saver, cache_t *cache) = 0;

private:
    void setup_server_and_run_tests();
};

#endif //__UNITTEST_SERVER_TEST_HELPER__
