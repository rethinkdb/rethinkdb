// Copyright 2010-2013 RethinkDB, all rights reserved.

#include "containers/archive/string_stream.hpp"
#include "rdb_protocol/datum.hpp"
#include "rdb_protocol/datum_string.hpp"
#include "rdb_protocol/env.hpp"
#include "unittest/gtest.hpp"


namespace unittest {

void test_datum_serialization(const ql::datum_t datum) {
    ql::datum_t deserialized_datum;
    {
        string_stream_t write_stream;
        write_message_t wm;
        serialize<cluster_version_t::LATEST_OVERALL>(&wm, datum);
        int write_res = send_write_message(&write_stream, &wm);
        ASSERT_EQ(0, write_res);

        string_read_stream_t read_stream(std::move(write_stream.str()), 0);
        archive_result_t res
            = deserialize<cluster_version_t::LATEST_OVERALL>(&read_stream,
                                                             &deserialized_datum);
        ASSERT_EQ(archive_result_t::SUCCESS, res);
        ASSERT_EQ(datum, deserialized_datum);
    }

    // Re-serialize the just deserialized datum a second time. This might use
    // a different serialization routine, in case the deserialized datum is in
    // a shared buffer representation.
    {
        string_stream_t write_stream;
        write_message_t wm;
        serialize<cluster_version_t::LATEST_OVERALL>(&wm, deserialized_datum);
        int write_res = send_write_message(&write_stream, &wm);
        ASSERT_EQ(0, write_res);

        string_read_stream_t read_stream(std::move(write_stream.str()), 0);
        ql::datum_t redeserialized_datum;
        archive_result_t res
            = deserialize<cluster_version_t::LATEST_OVERALL>(&read_stream,
                                                             &redeserialized_datum);
        ASSERT_EQ(archive_result_t::SUCCESS, res);
        ASSERT_EQ(deserialized_datum, redeserialized_datum);
    }
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

    std::vector<ql::datum_t> vec;
    for (size_t i = 0; i < sizeof(nums) / sizeof(nums[0]); ++i) {
        auto const positive = ql::datum_t(nums[i]);
        vec.push_back(positive);
        test_datum_serialization(positive);

        auto const negative = ql::datum_t(-nums[i]);
        vec.push_back(negative);
        test_datum_serialization(negative);
    }

    ql::configured_limits_t limits;
    // Make sure things don't get messed up with numbers serialized as part of a
    // larger entity (for example, if serialization wrote extra bytes that the test
    // above didn't detect).
    test_datum_serialization(ql::datum_t(std::move(vec), limits));
}

// Tests our ability to read old arrays and objects that were not serialized
// in the new buffer-backable format yet.
TEST(DatumTest, LegacyDeserialization) {
    // An array of two nulls
    std::vector<char> legacy_array = {0x01, 0x02, 0x03, 0x03};
    ql::datum_t reference_array(
        std::vector<ql::datum_t>
            {ql::datum_t::null(),
             ql::datum_t::null()},
            ql::configured_limits_t::unlimited);
    // An object {a: null, b: null}
    std::vector<char> legacy_object =
        {0x05, 0x02, 0x01, 'a', 0x03, 0x01, 'b', 0x03};
    ql::datum_t reference_object(std::map<datum_string_t, ql::datum_t>
        {std::make_pair(datum_string_t("a"), ql::datum_t::null()),
         std::make_pair(datum_string_t("b"), ql::datum_t::null())});
    {
        ql::datum_t deserialized_array;
        vector_read_stream_t s(std::move(legacy_array));
        archive_result_t res = datum_deserialize(&s, &deserialized_array);
        ASSERT_EQ(archive_result_t::SUCCESS, res);
        ASSERT_EQ(deserialized_array, reference_array);
    }
    {
        ql::datum_t deserialized_object;
        vector_read_stream_t s(std::move(legacy_object));
        archive_result_t res = datum_deserialize(&s, &deserialized_object);
        ASSERT_EQ(archive_result_t::SUCCESS, res);
        ASSERT_EQ(deserialized_object, reference_object);
    }
}

TEST(DatumTest, ObjectSerialization) {
    {
        ql::datum_t test_object((std::map<datum_string_t, ql::datum_t>()));
        test_datum_serialization(test_object);
    }
    {
        ql::datum_t test_object(std::map<datum_string_t, ql::datum_t>
                {std::make_pair(datum_string_t("a"), ql::datum_t::null()),
                 std::make_pair(datum_string_t("b"), ql::datum_t::null())});
        test_datum_serialization(test_object);
    }
    {
        ql::datum_t test_object(std::map<datum_string_t, ql::datum_t>
                {std::make_pair(datum_string_t("a"), ql::datum_t::null()),
                 std::make_pair(datum_string_t("b"), ql::datum_t::null()),
                 std::make_pair(datum_string_t("nested"), ql::datum_t(
                     std::map<datum_string_t, ql::datum_t>
                     {std::make_pair(datum_string_t("a"), ql::datum_t::null()),
                      std::make_pair(datum_string_t("b"), ql::datum_t::null())}))});
        test_datum_serialization(test_object);
    }
}

TEST(DatumTest, ArraySerialization) {
    {
        ql::datum_t test_array(
            std::vector<ql::datum_t>(), ql::configured_limits_t::unlimited);
        test_datum_serialization(test_array);
    }
    {
        ql::datum_t test_array(
            std::vector<ql::datum_t>
                {ql::datum_t::null(),
                 ql::datum_t::null()},
                ql::configured_limits_t::unlimited);
        test_datum_serialization(test_array);
    }
    {
        ql::datum_t test_array(
            std::vector<ql::datum_t>
                {ql::datum_t::null(),
                 ql::datum_t::null(),
                 ql::datum_t(std::vector<ql::datum_t>
                     {ql::datum_t::null(),
                      ql::datum_t::null()},
                     ql::configured_limits_t::unlimited)},
                ql::configured_limits_t::unlimited);
        test_datum_serialization(test_array);
    }
}

// Tests serialization with different offset sizes, up to 32 bit
// (64 bit not tested here, because that would use too much memory for a unit test)
TEST(DatumTest, OffsetScaling) {
    {
        // 8 bit
        ql::datum_t test_string(datum_string_t(std::string(1, 'A')));
        ql::datum_t test_array(
            std::vector<ql::datum_t>{test_string, test_string},
            ql::configured_limits_t::unlimited);
        test_datum_serialization(test_array);
    }
    {
        // Test values around the point where we switch to 16 bit offsets.
        for (size_t sz = 200; sz < 300; ++sz) {
            ql::datum_t test_string(datum_string_t(std::string(sz/2, 'A')));
            ql::datum_t test_array(
                std::vector<ql::datum_t>{test_string, test_string},
                ql::configured_limits_t::unlimited);
            test_datum_serialization(test_array);
        }
    }
    {
        // Test values around the point where we switch to 32 bit offsets.
        for (size_t sz = 65500; sz < 65540; ++sz) {
            ql::datum_t test_string(datum_string_t(std::string(sz/2, 'A')));
            ql::datum_t test_array(
                std::vector<ql::datum_t>{test_string, test_string},
                ql::configured_limits_t::unlimited);
            test_datum_serialization(test_array);
        }
    }
}

}  // namespace unittest
