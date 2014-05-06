// Copyright 2010-2013 RethinkDB, all rights reserved.

#include "containers/archive/string_stream.hpp"
#include "rdb_protocol/datum.hpp"
#include "unittest/gtest.hpp"


namespace unittest {

void test_datum_serialization(const counted_t<const ql::datum_t> datum) {
    string_stream_t write_stream;
    write_message_t wm;
    serialize(&wm, datum);
    int write_res = send_write_message(&write_stream, &wm);
    ASSERT_EQ(0, write_res);

    string_read_stream_t read_stream(std::move(write_stream.str()), 0);
    counted_t<const ql::datum_t> deserialized_datum;
    archive_result_t res = deserialize(&read_stream, &deserialized_datum);
    ASSERT_EQ(archive_result_t::SUCCESS, res);
    ASSERT_EQ(datum, deserialized_datum);
}


TEST(DatumTest, NumericSerialization) {
    double nums[] = { 0.0,
                      std::numeric_limits<double>::min(),
                      std::numeric_limits<double>::denorm_min(),
                      std::numeric_limits<double>::max(),
                      0.5,
                      1.0,
                      1.1,
                      2.0,
                      2.5,
                      3.0,
                      (1 << 23) - 1,
                      (1 << 23),
                      (1ull << 52) - 1,
                      (1ull << 52) - 0.5,
                      (1ull << 52),
                      (1ull << 53) - 2,
                      (1ull << 53) - 1,
                      (1ull << 53),
                      (1ull << 53) + 2,
                      (1ull << 63) + 4096,
                      6.02214179e23,
    };

    std::vector<counted_t<const ql::datum_t> > vec;
    for (size_t i = 0; i < sizeof(nums) / sizeof(nums[0]); ++i) {
        auto const positive = make_counted<const ql::datum_t>(nums[i]);
        vec.push_back(positive);
        test_datum_serialization(positive);

        auto const negative = make_counted<const ql::datum_t>(-nums[i]);
        vec.push_back(negative);
        test_datum_serialization(negative);
    }

    // Make sure things don't get messed up with numbers serialized as part of a
    // larger entity (for example, if serialization wrote extra bytes that the test
    // above didn't detect).
    test_datum_serialization(make_counted<ql::datum_t>(std::move(vec)));
}



}  // namespace unittest
