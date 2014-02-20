#include "mock/serializer_filestream.hpp"
#include "serializer/config.hpp"
#include "unittest/gtest.hpp"
#include "unittest/mock_file.hpp"
#include "unittest/unittest_utils.hpp"

namespace unittest {


void run_Boundaries() {
    mock_file_opener_t file_opener;
    standard_serializer_t::create(&file_opener,
                                  standard_serializer_t::static_config_t());
    standard_serializer_t ser(standard_serializer_t::dynamic_config_t(),
                              &file_opener,
                              &get_global_perfmon_collection());

    // Pick lengths large enough so that the read stream will both align with and
    // miss boundaries.
    std::string text1 = rand_string(415000);
    std::string text2 = rand_string(211000);

    {
        mock::serializer_file_write_stream_t stream(&ser);

        int64_t count1 = stream.write(text1.data(), text1.size());
        ASSERT_EQ(text1.size(), count1);
        int64_t count2 = stream.write(text2.data(), text2.size());
        ASSERT_EQ(text2.size(), count2);
    }

    std::string builder;
    {
        mock::serializer_file_read_stream_t stream(&ser);
        // 13 is small, prime and a multiple of 13 * 4 isn't ever going to equal some
        // 4KB block size, with or without block magic.
        const size_t count = 13 * 4;
        char buf[count];
        int64_t lenread;
        while (0 < (lenread = stream.read(buf, count))) {
            builder.append(buf, lenread);
            ASSERT_LE(builder.size(), text1.size() + text2.size());
        }
        ASSERT_EQ(0, lenread);
    }

    ASSERT_EQ(text1 + text2, builder);
}

TEST(SerializerFilestreamTest, Boundaries) {
    run_in_thread_pool(&run_Boundaries);
}

}  // namespace unittest

