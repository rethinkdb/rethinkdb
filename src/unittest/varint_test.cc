// Copyright 2010-2014 RethinkDB, all rights reserved.
#include <inttypes.h>

#include "containers/archive/string_stream.hpp"
#include "containers/archive/varint.hpp"
#include "unittest/gtest.hpp"
#include "utils.hpp"

namespace unittest {

archive_result_t run_serialization(uint64_t value,
                                   uint64_t *output_value_out,
                                   size_t *serialized_size_out) {
    write_message_t wm;
    serialize_varint_uint64(&wm, value);

    string_stream_t write_stream;
    int send_res = send_write_message(&write_stream, &wm);
    EXPECT_EQ(0, send_res);

    std::string serialized_value = write_stream.str();
    *serialized_size_out = serialized_value.size();

    string_read_stream_t read_stream(std::move(serialized_value), 0);

    return deserialize_varint_uint64(&read_stream, output_value_out);
}

TEST(VarintTest, Success64) {
    uint64_t vals[] = { 0, 1, 2, 127, 128, 129, 130, 256, 127 * 128, 127 * 128 + 127, 128 * 128,
                        UINT32_MAX, static_cast<uint64_t>(UINT32_MAX) + 1,
                        UINT64_MAX - 1, UINT64_MAX };
    for (size_t i = 0; i < sizeof(vals) / sizeof(*vals); ++i) {
        const uint64_t value = vals[i];
        SCOPED_TRACE("value = " + strprintf("%" PRIu64, value));

        uint64_t output_value;
        size_t serialized_size;
        archive_result_t res = run_serialization(value, &output_value, &serialized_size);
        EXPECT_EQ(varint_uint64_serialized_size(value), serialized_size);

        EXPECT_EQ(archive_result_t::SUCCESS, res);
        EXPECT_EQ(value, output_value);
    }
}

TEST(VarintTest, RangeError) {
    // First we test 1 + UINT64_MAX / 2, to make sure it works.
    std::string s;
    for (size_t i = 0; i < 9; ++i) {
        s.push_back(128);
    }
    s.push_back(1);

    {
        string_read_stream_t read_stream(std::string(s), 0);
        uint64_t output_value;
        archive_result_t res = deserialize_varint_uint64(&read_stream, &output_value);
        EXPECT_EQ(archive_result_t::SUCCESS, res);
        EXPECT_EQ(1 + UINT64_MAX / 2, output_value);
    }

    // Next, we test UINT64_MAX + 1, to make sure we handle 10-byte varints that
    // overflow.
    s.erase(s.end() - 1);
    s.push_back(2);

    {
        string_read_stream_t read_stream(std::string(s), 0);
        uint64_t output_value;
        archive_result_t res = deserialize_varint_uint64(&read_stream, &output_value);
        EXPECT_EQ(archive_result_t::RANGE_ERROR, res);
    }

    // Next, we test an 11-byte varint.
    s.erase(s.end() - 1);
    s.push_back(128);
    s.push_back(1);

    {
        string_read_stream_t read_stream(std::string(s), 0);
        uint64_t output_value;
        archive_result_t res = deserialize_varint_uint64(&read_stream, &output_value);
        EXPECT_EQ(archive_result_t::RANGE_ERROR, res);
    }
}

}  // namespace unittest
