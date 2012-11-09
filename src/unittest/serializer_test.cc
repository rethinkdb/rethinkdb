#include "arch/runtime/starter.hpp"
#include "serializer/config.hpp"
#include "unittest/mock_file.hpp"
#include "unittest/gtest.hpp"

namespace unittest {

// This test is completely vacuous, but should invite expansion in the future.

void run_CreateConstructDestroy() {
    // This serves more as a mock_file_t test than a serializer test.
    mock_file_opener_t file_opener;
    standard_serializer_t::create(&file_opener, standard_serializer_t::static_config_t());
    standard_serializer_t ser(standard_serializer_t::dynamic_config_t(),
                              &file_opener,
                              &get_global_perfmon_collection());
}

TEST(SerializerTest, CreateConstructDestroy) {
    run_in_thread_pool(run_CreateConstructDestroy, 4);
}


}  // namespace unittest
