// Copyright 2010-2013 RethinkDB, all rights reserved.

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "containers/archive/string_stream.hpp"
#include "containers/archive/vector_stream.hpp"
#include "rdb_protocol/configured_limits.hpp"
#include "rdb_protocol/datum.hpp"
#include "rdb_protocol/env.hpp"
#include "rdb_protocol/serialize_datum.hpp"
#include "unittest/gtest.hpp"


namespace unittest {

void test_datum_serialization(const counted_t<const ql::datum_t> datum,
                              bool assert_lazy = false) {
    string_stream_t write_stream;
    write_message_t wm;
    serialize<cluster_version_t::LATEST_OVERALL>(&wm, datum);
    int write_res = send_write_message(&write_stream, &wm);
    ASSERT_EQ(0, write_res);

    string_read_stream_t read_stream(std::move(write_stream.str()), 0);
    counted_t<const ql::datum_t> deserialized_datum;
    archive_result_t res
        = deserialize<cluster_version_t::LATEST_OVERALL>(&read_stream, &deserialized_datum);
    ASSERT_EQ(archive_result_t::SUCCESS, res);
    if (assert_lazy) {
        ASSERT_TRUE(deserialized_datum->is_lazy());
    }
    ASSERT_EQ(*datum, *deserialized_datum);
    // After we have compared the value, the datum cannot be lazy anymore.
    ASSERT_FALSE(deserialized_datum->is_lazy());
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

    ql::configured_limits_t limits;
    // Make sure things don't get messed up with numbers serialized as part of a
    // larger entity (for example, if serialization wrote extra bytes that the test
    // above didn't detect).
    test_datum_serialization(make_counted<ql::datum_t>(std::move(vec), limits));
}

// Tests our ability to read old arrays and objects that were not serialized
// as lazy yet.
TEST(DatumTest, LegacyDeserialization) {
    // An array of two nulls
    std::vector<char> legacy_array = {0x01, 0x02, 0x03, 0x03};
    auto reference_array =
        make_counted<const ql::datum_t>(std::vector<counted_t<const ql::datum_t> >
            {ql::datum_t::null(),
             ql::datum_t::null()},
            ql::configured_limits_t::unlimited);
    // An object {a: null, b: null}
    std::vector<char> legacy_object =
        {0x05, 0x02, 0x01, 'a', 0x03, 0x01, 'b', 0x03};
    auto reference_object =
        make_counted<const ql::datum_t>(std::map<std::string, counted_t<const ql::datum_t> >
            {std::make_pair("a", ql::datum_t::null()),
             std::make_pair("b", ql::datum_t::null())});

    {
        counted_t<const ql::datum_t> deserialized_array;
        vector_read_stream_t s(std::move(legacy_array));
        archive_result_t res = datum_deserialize(&s, &deserialized_array);
        ASSERT_EQ(archive_result_t::SUCCESS, res);
        ASSERT_EQ(*deserialized_array, *reference_array);
    }
    {
        counted_t<const ql::datum_t> deserialized_object;
        vector_read_stream_t s(std::move(legacy_object));
        archive_result_t res = datum_deserialize(&s, &deserialized_object);
        ASSERT_EQ(archive_result_t::SUCCESS, res);
        ASSERT_EQ(*deserialized_object, *reference_object);
    }
}

// Tests that arrays and datums are deserialized lazily
TEST(DatumTest, LazySerialization) {
    auto test_array =
        make_counted<const ql::datum_t>(std::vector<counted_t<const ql::datum_t> >
            {ql::datum_t::null(),
             ql::datum_t::null()},
            ql::configured_limits_t::unlimited);
    test_datum_serialization(test_array, true);
    auto test_object =
        make_counted<const ql::datum_t>(std::map<std::string, counted_t<const ql::datum_t> >
            {std::make_pair("a", ql::datum_t::null()),
             std::make_pair("b", ql::datum_t::null())});
    test_datum_serialization(test_object, true);
}

// Tests that writing a lazy datum works and again results in a lazy datum
TEST(DatumTest, LazyReserialization) {
    // An array of two nulls
    std::vector<char> serialized_array = {0x01, 0x02, 0x03, 0x03};
    auto lazy_array = make_counted<const ql::datum_t>(std::move(serialized_array));
    ASSERT_TRUE(lazy_array->is_lazy());
    test_datum_serialization(lazy_array, true);

    // An object {a: null, b: null}
    std::vector<char> serialized_object =
        {0x05, 0x02, 0x01, 'a', 0x03, 0x01, 'b', 0x03};
    auto lazy_object = make_counted<const ql::datum_t>(std::move(serialized_object));
    ASSERT_TRUE(lazy_object->is_lazy());
    test_datum_serialization(lazy_object, true);
}

// Tests that calling force_deserialization() on a nested lazy array works
TEST(DatumTest, ForceDeserialization) {
    // An array of an array of two nulls
    std::vector<char> serialized_array = {0x01, 0x01, 0x01, 0x02, 0x03, 0x03};
    auto lazy_array = make_counted<const ql::datum_t>(std::move(serialized_array));
    ASSERT_TRUE(lazy_array->is_lazy());
    lazy_array->force_deserialization();
    ASSERT_FALSE(lazy_array->is_lazy());
    ASSERT_FALSE(lazy_array->get(0)->is_lazy());
}

}  // namespace unittest
